#include "forces/hydro.hpp"
#include "io/trajectory.hpp"
#include "simulation/validation.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace physics;

double quantile(std::vector<double> values, double fraction) {
    if (values.empty())
        return 0;
    std::sort(values.begin(), values.end());
    const double index = fraction * static_cast<double>(values.size() - 1);
    const auto low = static_cast<std::size_t>(index);
    return values[low] +
           (values[std::min(low + 1, values.size() - 1)] - values[low]) * (index - static_cast<double>(low));
}

struct Shape {
    double r50 = 0, height50 = 0;
};
Shape shape(Sample const &sample, std::vector<BodyDesc> const &bodies) {
    Vec3 center{}, velocity{}, axis{};
    double mass = 0;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].particle.component != Component::Disk)
            continue;
        const double m = bodies[i].particle.mass;
        center += sample.position[i] * m;
        velocity += sample.velocity[i] * m;
        mass += m;
    }
    if (!(mass > 0))
        return {};
    center = center / mass;
    velocity = velocity / mass;
    for (std::size_t i = 0; i < bodies.size(); ++i)
        if (bodies[i].particle.component == Component::Disk)
            axis += cross(sample.position[i] - center, sample.velocity[i] - velocity) * bodies[i].particle.mass;
    axis = norm(axis) > 1e-12 ? axis / norm(axis) : Vec3{0, 0, 1};
    std::vector<double> radius, height;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].particle.component != Component::Disk)
            continue;
        const auto p = sample.position[i] - center;
        const double z = dot(p, axis);
        radius.push_back(std::sqrt(std::max(0.0, dot(p, p) - z * z)));
        height.push_back(std::abs(z));
    }
    return {quantile(radius, 0.5), quantile(height, 0.5)};
}

int main(int argc, char **argv) {
    try {
        if (argc < 3)
            throw std::invalid_argument("usage: physics_validate scene output_directory [--half-dt|--quarter-dt] "
                                        "[--mode full|gravity|adiabatic] [--trajectories] [--isolate INDEX] "
                                        "[--resolution-study] [--duration T] [--merger-only]");
        int refinements = 0;
        bool trajectories = false, resolution_study = false, merger_only = false;
        int isolate_index = -1;
        double duration = 0;
        std::string mode = "full";
        for (int i = 3; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--half-dt")
                refinements = std::max(refinements, 1);
            else if (arg == "--quarter-dt")
                refinements = 2;
            else if (arg == "--mode" && i + 1 < argc)
                mode = argv[++i];
            else if (arg == "--merger-only")
                merger_only = true;
            else if (arg == "--resolution-study")
                resolution_study = true;
            else if (arg == "--isolate" && i + 1 < argc) {
                const std::string value = argv[++i];
                std::size_t used = 0;
                isolate_index = std::stoi(value, &used);
                if (used != value.size() || isolate_index < 0)
                    throw std::invalid_argument("invalid isolate index");
            } else if (arg == "--duration" && i + 1 < argc) {
                const std::string value = argv[++i];
                std::size_t used = 0;
                duration = std::stod(value, &used);
                if (used != value.size() || !std::isfinite(duration) || duration <= 0)
                    throw std::invalid_argument("invalid duration");
            } else if (arg == "--trajectories")
                trajectories = true;
            else
                throw std::invalid_argument("unknown option: " + arg);
        }
        if (merger_only && (isolate_index >= 0 || resolution_study))
            throw std::invalid_argument("merger-only cannot be combined with isolation or resolution study");
        auto source = load_scene_file(argv[1]);
        if (mode != "full" && mode != "gravity" && mode != "adiabatic")
            throw std::invalid_argument("mode must be full, gravity, or adiabatic");
        if (source.galaxies.empty())
            throw std::invalid_argument("validation requires a particle galaxy scene");
        if (isolate_index >= static_cast<int>(source.galaxies.size()))
            throw std::invalid_argument("isolate index out of range");
        if (duration > 0) {
            const double steps = duration / source.config.dt;
            if (steps < 1 || steps > 10000000 || std::abs(steps - std::round(steps)) > 1e-7)
                throw std::invalid_argument("duration must be a whole number of timesteps, at most 10000000");
            source.config.steps = static_cast<std::size_t>(std::llround(steps));
        }
        const std::filesystem::path out(argv[2]);
        std::filesystem::create_directories(out);
        std::ofstream csv(out / "summary.csv");
        if (!csv)
            throw std::runtime_error("cannot open summary.csv");
        csv << std::setprecision(12)
            << "run,bodies,dt,duration,initial_r50,final_r50,initial_height50,final_height50,initial_energy,final_"
               "energy,radiated,feedback,formation_removed,floor_added,corrected_energy_residual,relative_residual,"
               "momentum_change,angular_momentum_change,young_stars,formed_stellar_mass,remaining_cold_gas_mass\n";
        auto run = [&](Scene scene, std::string const &name, bool isolate, double disk_scale = 1.0) {
            auto state = scene.make_state();
            prepare_gas(state, scene.config.forces);
            std::cout << "Running " << name << " (" << state.size() << " bodies, dt=" << scene.config.dt << ")"
                      << std::endl;
            auto result = simulate_interruptible(std::move(state), scene.bodies, scene.config, 0, nullptr, nullptr);
            const auto &first = result.trajectory.samples.front();
            const auto &last = result.trajectory.samples.back();
            const auto &end = result.state;
            const double residual = last.diagnostics.energy - first.diagnostics.energy + end.radiated_energy +
                                    end.formation_energy_removed - end.feedback_energy - end.floor_energy_added;
            const double relative = residual / std::max(std::abs(first.diagnostics.energy), 1e-12);
            csv << name << ',' << end.size() << ',' << scene.config.dt << ',' << last.time << ',';
            if (isolate) {
                const auto a = shape(first, scene.bodies), b = shape(last, scene.bodies);
                csv << a.r50 << ',' << b.r50 << ',' << a.height50 << ',' << b.height50 << ',';
            } else
                csv << ",,,,"; // Disk thickness has no single useful plane for a merger.
            csv << first.diagnostics.energy << ',' << last.diagnostics.energy << ',' << end.radiated_energy << ','
                << end.feedback_energy << ',' << end.formation_energy_removed << ',' << end.floor_energy_added << ','
                << residual << ',' << relative << ','
                << norm(last.diagnostics.linear_momentum - first.diagnostics.linear_momentum) << ','
                << norm(last.diagnostics.angular_momentum - first.diagnostics.angular_momentum) << ','
                << std::count(end.component.begin(), end.component.end(),
                              static_cast<std::uint8_t>(Component::YoungStar))
                << ',';
            double formed_mass = 0, remaining_gas = 0;
            for (auto const& cohort : end.stellar_cohorts) formed_mass += cohort.mass;
            for (std::size_t i = 0; i < end.size(); ++i) {
                if (end.component[i] == static_cast<std::uint8_t>(Component::YoungStar)) formed_mass += end.mass[i];
                if (end.component[i] == static_cast<std::uint8_t>(Component::Gas)) remaining_gas += end.gas_mass(i);
            }
            csv << formed_mass << ',' << remaining_gas << '\n';
            csv.flush();
            if (isolate) {
                std::ofstream profiles(out / (name + "_profiles.csv"));
                profiles << std::setprecision(12)
                         << "time,inner,outer,count,surface_density,height50,rotation,sigma_r,sigma_z,"
                            "a2,a4,phase2,phase4,noise,circular_speed,kappa,q\n";
                if (!profiles)
                    throw std::runtime_error("cannot write radial profiles");
                std::ofstream history(out / (name + "_history.csv"));
                if (!history)
                    throw std::runtime_error("cannot write structure history");
                history << std::setprecision(12) << "time,r50,height50\n";
                const double scale = disk_scale;
                std::vector<double> edges{0, 0.5, 1, 1.5, 2, 3, 4, 6, 10};
                for (auto &edge : edges)
                    edge *= scale;
                for (const auto &sample : result.trajectory.samples) {
                    const auto measured = shape(sample, scene.bodies);
                    history << sample.time << ',' << measured.r50 << ',' << measured.height50 << '\n';
                    for (const auto &bin : disk_profiles(sample, scene.bodies, edges, &scene.config.forces))
                        profiles << sample.time << ',' << bin.inner << ',' << bin.outer << ',' << bin.count << ','
                                 << bin.surface_density << ',' << bin.height50 << ',' << bin.rotation << ','
                                 << bin.sigma_r << ',' << bin.sigma_z << ',' << bin.a2 << ',' << bin.a4 << ','
                                 << bin.phase2 << ',' << bin.phase4 << ',' << bin.noise << ','
                                 << bin.circular_speed << ',' << bin.kappa << ',' << bin.q << '\n';
                }
            }
            if (trajectories)
                write_trajectory(result.trajectory, (out / (name + ".traj")).string());
            std::cout << "  Corrected energy residual: " << relative * 100 << "%" << std::endl;
        };
        for (int level = 0; level < (resolution_study ? 3 : 1); ++level) {
            auto resolved = source;
            const std::size_t factor = std::size_t{1} << level;
            if (level) {
                for (auto &g : resolved.galaxies) {
                    const double stellar = g.halo_mass + g.disk_mass + g.bulge_mass;
                    if (g.gas_mass > 0 && g.gas_particles == 0)
                        g.gas_particles = std::max<std::size_t>(
                            64, static_cast<std::size_t>(std::lround(g.particles * g.gas_mass / stellar)));
                    if (g.hot_gas_mass > 0 && g.hot_gas_particles == 0)
                        g.hot_gas_particles = 64;
                    if (g.particles > 1000000 / factor || g.gas_particles > 1000000 / factor ||
                        g.hot_gas_particles > 1000000 / factor)
                        throw std::invalid_argument("resolution study exceeds particle limit");
                    g.particles *= factor;
                    g.gas_particles *= factor;
                    g.hot_gas_particles *= factor;
                }
                apply_galaxies(resolved);
            }
            for (int refinement = 0; refinement <= refinements; ++refinement) {
                auto scene = resolved;
                if (mode != "full") {
                    scene.config.forces.gas_cooling_time = 0;
                    scene.config.forces.gas_star_time = 0;
                    scene.config.forces.gas_feedback_energy = 0;
                }
                if (mode == "gravity") {
                    for (auto &b : scene.bodies)
                        if (b.particle.component == Component::Gas || b.particle.component == Component::HotGas) {
                            b.particle.component = Component::Body;
                            b.particle.internal_energy = 0;
                        }
                }

                std::string suffix = refinement == 2 ? "_quarter_dt" : refinement == 1 ? "_half_dt" : "";
                if (resolution_study)
                    suffix += "_n" + std::to_string(factor);
                for (int r = 0; r < refinement; ++r) {
                    scene.config.dt *= 0.5;
                    scene.config.steps *= 2;
                    scene.config.sample_every *= 2;
                }
                if (isolate_index < 0 && !resolution_study)
                    run(scene, "merger" + suffix, false);
                for (std::size_t i = 0; i < scene.galaxies.size(); ++i)
                    if (!merger_only && (isolate_index < 0 || i == static_cast<std::size_t>(isolate_index)))
                        run(isolated_control(scene, i), "isolated_" + std::to_string(i) + suffix, true,
                            scene.galaxies[i].disk_scale_radius);
            }
        }
        std::ofstream notes(out / "README.txt");
        notes << "Source: " << std::filesystem::absolute(argv[1]).string() << "\n"
              << "Mode: " << mode << "\n"
              << "Resolution study: " << resolution_study
              << "; counts of every component scale together; total masses, softening and dt stay fixed (component "
                 "allocation has integer rounding). New draws, not "
                 "nested particles.\n"
              << "Profiles: original stellar disk in instantaneous disk plane; surface density and velocity moments "
                 "are mass weighted. Empty bins have count=0.\n"
              << "Controls extract the same realized particles, remove imposed bulk position/velocity, and retain the "
                 "merger's initial gas thresholds.\n"
              << "r50: median cylindrical radius. height50: median absolute height about the current disk "
                 "angular-momentum plane, centered on disk center of mass. Code units throughout.\n"
              << "Energy residual = Efinal - Einitial + radiated + formation_removed - feedback - floor_added.\n"
              << "This is a diagnostic, not a conservation guarantee: pressure floors and time integration still "
                 "introduce error.\n"
              << "Gas energy counters survive in-memory Continue but are not stored in trajectory files. CSV retains "
                 "final budgets.\n"
              << "Particle indices and timestep affect stochastic star formation; compare ensemble morphology, not "
                 "identical birth histories.\n";
        return 0;
    } catch (std::exception const &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
