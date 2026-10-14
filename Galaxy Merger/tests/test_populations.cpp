#include "forces/hydro.hpp"
#include "io/trajectory.hpp"
#include "simulation/galaxy.hpp"
#include "visualization/optical.hpp"
#include "visualization/tracers.hpp"
#include <doctest/doctest.h>
#include <cmath>
#include <filesystem>

using namespace physics;

TEST_CASE("fractional formation conserves mass momentum and thermal feedback budgets") {
    State s;
    for (int i = 0; i < 3; ++i) {
        Particle p;
        p.mass = .02 * (i + 1); p.component = Component::Gas;
        p.position = {.1 * i, 0, 0}; p.velocity = {1, -.1 * i, .2}; p.internal_energy = .00225;
        s.add(p);
    }
    ForceSet f;
    f.gas_sound_speed = .05; f.gas_star_time = 2; f.gas_resolved_star_density = 1;
    f.gas_continuous_stars = true; f.gas_feedback_energy = .1;
    s.density = {4, 4, 4}; s.divergence = {-1, -1, -1};
    const auto initial = diagnose(s, f).energy;
    const auto momentum = linear_momentum(s);
    REQUIRE(form_stars(s, f, .01, 1) == 3);
    REQUIRE(s.stellar_cohorts.size() == 3);
    double formed = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        CHECK(s.component[i] == static_cast<std::uint8_t>(Component::Gas));
        CHECK(s.gas_fraction[i] == doctest::Approx(std::exp(-.01)));
        CHECK(s.gas_mass(i) + s.stellar_cohorts[i].mass == doctest::Approx(s.mass[i]));
        CHECK(s.stellar_cohorts[i].time == .01);
        formed += s.stellar_cohorts[i].mass;
    }
    CHECK(norm(linear_momentum(s) - momentum) < 1e-14);
    CHECK(s.feedback_energy == doctest::Approx(formed * f.gas_feedback_energy));
    CHECK(s.formation_energy_removed == doctest::Approx(formed * .00225));
    CHECK(diagnose(s, f).energy + s.formation_energy_removed - s.feedback_energy == doctest::Approx(initial));
    // Exhaustion retains precisely the advertised tiny gas reservoir.
    f.gas_star_time = 1e-20; f.gas_feedback_energy = 0;
    s.internal_energy.assign(3, .00225);
    REQUIRE(form_stars(s, f, .01, 2) == 3);
    for (std::size_t i = 0; i < 3; ++i) {
        double mass = s.gas_mass(i);
        for (auto c : s.stellar_cohorts) if (c.body == i) mass += c.mass;
        CHECK(mass == doctest::Approx(s.mass[i]));
        CHECK(s.gas_fraction[i] == doctest::Approx(1e-6));
    }
}

TEST_CASE("hybrid SPH exchanges momentum and energy with total carrier inertia") {
    State s;
    for (int i = 0; i < 4; ++i) {
        Particle p; p.mass = .01 * (i + 1); p.component = Component::Gas;
        p.internal_energy = .03; p.position = {.08 * i, .01 * (i % 2), 0}; p.velocity = {-.02 * i, .01, 0};
        s.add(p); s.gas_fraction.back() = .2 * (i + 1);
    }
    ForceSet f; f.gas_sound_speed = .05; f.softening_length = .05;
    Accelerations a; a.resize(s.size()); add_hydro_forces(s, a, f);
    Vec3 force{}; double work = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const Vec3 acceleration{a.ax[i], a.ay[i], a.az[i]};
        force += acceleration * s.mass[i];
        work += s.mass[i] * dot(acceleration, s.velocity(i)) + s.gas_mass(i) * s.hydro_dudt[i];
    }
    CHECK(norm(force) < 1e-12);
    CHECK(std::abs(work) < 1e-12);
}

TEST_CASE("initial stellar history is deterministic and does not change the dynamical realization") {
    GalaxySpec g;
    g.name = "history"; g.particles = 128; g.halo_mass = 2; g.halo_scale = 2;
    g.disk_mass = 1; g.disk_scale_radius = 1; g.disk_height = .12;
    g.disk_particle_fraction = .6; g.spiral_strength = .55;
    const auto original = realize_galaxy(g, 1, .1);
    g.initial_young_fraction = .2; g.initial_young_age = 8; g.young_arm_contrast = .9;
    const auto changed = realize_galaxy(g, 1, .1), repeated = realize_galaxy(g, 1, .1);
    double young_mass = 0;
    for (std::size_t i = 0; i < original.size(); ++i) {
        CHECK(changed[i].particle.mass == original[i].particle.mass);
        CHECK(changed[i].particle.position == original[i].particle.position);
        CHECK(changed[i].particle.velocity == original[i].particle.velocity);
        CHECK(changed[i].initial_young_age == repeated[i].initial_young_age);
        CHECK(changed[i].initial_young_fraction <= .2);
        CHECK(changed[i].initial_young_age <= 8);
        young_mass += changed[i].particle.mass * changed[i].initial_young_fraction;
    }
    CHECK(young_mass > 0);
    g.initial_young_fraction = 1.1;
    CHECK_THROWS((void)realize_galaxy(g, 1, .1));
}

namespace {
Trajectory population_fixture(int sources = 1) {
    Trajectory t; Sample s;
    BodyRecord gas; gas.name = "g.gas.0"; gas.mass = .01; gas.emission = .4;
    gas.formed_star_emission = 6.5;
    t.bodies.push_back(gas); s.position.push_back({0, 0, 0}); s.velocity.push_back({});
    s.component.push_back(static_cast<std::uint8_t>(Component::Gas)); s.gas_density.push_back(.1);
    s.gas_fraction.push_back(1);
    for (int i = 0; i < sources; ++i) {
        BodyRecord star; star.name = "g.disk." + std::to_string(i); star.mass = 0;
        star.light_weight = .001 / sources; star.emission = 1.7; star.formed_star_emission = 6.5;
        star.initial_young_fraction = 1;
        t.bodies.push_back(star); s.position.push_back({0, 0, 0}); s.velocity.push_back({});
        s.component.push_back(static_cast<std::uint8_t>(Component::Disk)); s.gas_density.push_back(0);
        s.gas_fraction.push_back(0);
    }
    t.samples.push_back(s); return t;
}
Vec3 flux(Trajectory const& t, double time, visual::OpticalSettings settings) {
    Vec3 result{};
    for (auto const& p : visual::make_splats(t, &t.samples[0], t.samples[0].position, time,
            {0, 0, 10}, {1, 0, 0}, {0, 1, 0}, {0, 0, -1}, settings)) result += p.color * p.luminosity;
    return result;
}
}

TEST_CASE("nebular light is mass weighted and needs gas density and young sources") {
    auto t = population_fixture(); auto split = population_fixture(8);
    visual::OpticalSettings settings; settings.texture = 0; settings.detail = 0;
    settings.ionizing_mass_scale = .0002; // Keep this decay probe below deep saturation.
    settings.halpha_color = {1, 0, 0}; settings.oiii_color = {0, 1, 0};
    const auto lit = flux(t, 0, settings);
    CHECK(norm(lit - flux(split, 0, settings)) < 1e-12);
    auto no_lines = settings; no_lines.gas_glow = 0;
    CHECK(lit.x > flux(t, 0, no_lines).x);
    CHECK(lit.y > flux(t, 0, no_lines).y);
    auto no_gas = t; no_gas.samples[0].gas_fraction[0] = 0;
    CHECK(norm(flux(no_gas, 0, settings) - flux(t, 0, no_lines)) < 1e-12);
    auto no_density = t; no_density.samples[0].gas_density[0] = 0;
    CHECK(norm(flux(no_density, 0, settings) - flux(t, 0, no_lines)) < 1e-12);
    const auto early_lines = lit - flux(t, 0, no_lines);
    const auto late_lines = flux(t, 4, settings) - flux(t, 4, no_lines);
    CHECK(late_lines.x < early_lines.x * .05);
    CHECK(late_lines.y < early_lines.y * .01);
    CHECK(flux(t, 4, no_lines).z > flux(t, 0, no_lines).z * .5);
    auto exhausted = settings; exhausted.ionizing_lifetime = .001; exhausted.oiii_lifetime = 100;
    CHECK(norm(flux(t, 4, exhausted) - flux(t, 4, no_lines)) < 1e-12);
    settings.halpha_strength = 0; settings.oiii_strength = 0;
    CHECK(norm(flux(t, 0, settings) - flux(t, 0, no_lines)) < 1e-12);
}

TEST_CASE("hybrid cohorts do not illuminate earlier playback and V7 preserves their history") {
    auto t = population_fixture();
    t.bodies[1].initial_young_fraction = .25; t.bodies[1].initial_young_age = 2;
    t.stellar_cohorts = {{0, 1, .002}, {0, 2, .001}};
    t.samples[0].gas_fraction[0] = .7;
    t.samples[0].time = 2;
    visual::OpticalSettings settings;
    auto empty = t; empty.stellar_cohorts.clear();
    CHECK(norm(flux(t, 0, settings) - flux(empty, 0, settings)) < 1e-12);
    CHECK(norm(flux(t, 2, settings)) > norm(flux(empty, 2, settings)));
    const auto path = std::filesystem::temp_directory_path() / "physics_populations_v7.traj";
    write_trajectory(t, path.string());
    const auto read = read_trajectory(path.string());
    CHECK(read.samples[0].gas_fraction == t.samples[0].gas_fraction);
    CHECK(read.bodies[1].initial_young_fraction == .25);
    CHECK(read.bodies[1].initial_young_age == 2);
    REQUIRE(read.stellar_cohorts.size() == 2);
    CHECK(read.stellar_cohorts[1].body == 0);
    CHECK(read.stellar_cohorts[1].time == 2);
    CHECK(read.stellar_cohorts[1].mass == .001);
    CHECK(norm(flux(read, 2, settings) - flux(t, 2, settings)) < 1e-12);
    std::filesystem::remove(path);
}

TEST_CASE("continuous formation and cohorts continue exactly from an in-memory checkpoint") {
    std::vector<BodyDesc> bodies(8);
    State s;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        auto& b = bodies[i]; b.particle.mass = .01; b.particle.component = Component::Gas;
        b.particle.position = {.1 * static_cast<double>(i), 0, 0}; b.particle.internal_energy = .00225;
        s.add(b.particle);
    }
    SimulationConfig config; config.dt = .005; config.steps = 20; config.sample_every = 2;
    auto& f = config.forces;
    f.gas_sound_speed = .05; f.softening_length = .05; f.gas_cooling_time = .1;
    f.gas_star_time = 5; f.gas_star_density = .001; f.gas_continuous_stars = true;
    f.gas_star_converging = false; f.gas_star_temperature = 100;
    prepare_gas(s, f);
    const auto full = simulate_interruptible(s, bodies, config, 0, nullptr, nullptr);
    auto half = config; half.steps = 10;
    auto start = simulate_interruptible(s, bodies, half, 0, nullptr, nullptr);
    const auto end = simulate_interruptible(start.state, bodies, config, 10, nullptr, nullptr);
    CHECK(end.state.x == full.state.x);
    CHECK(end.state.gas_fraction == full.state.gas_fraction);
    REQUIRE(end.trajectory.stellar_cohorts.size() == full.trajectory.stellar_cohorts.size());
    REQUIRE(!end.trajectory.stellar_cohorts.empty());
    for (std::size_t i = 0; i < end.trajectory.stellar_cohorts.size(); ++i) {
        CHECK(end.trajectory.stellar_cohorts[i].mass == full.trajectory.stellar_cohorts[i].mass);
        CHECK(end.trajectory.stellar_cohorts[i].time == full.trajectory.stellar_cohorts[i].time);
    }
}

TEST_CASE("formation gates and scene population parameters are explicit and validated") {
    const std::string scene = "integrator velocity_verlet\ndt .01\nsteps 2\ngravitational_constant 1\nforce gravity\nsoftening .1\n"
        "galaxy g\nparticles 64\nhalo_mass 2\nhalo_scale 2\ndisk_mass 1\ndisk_scale_radius 1\n"
        "gas_mass .1\ngas_particles 48\nstar_formation_mode continuous\nstar_formation_converging false\n"
        "initial_young_fraction .2\ninitial_young_age 3\nyoung_arm_contrast .5\nstar_formation_temperature 2\n";
    const auto loaded = load_scene(scene);
    CHECK(loaded.config.forces.gas_continuous_stars);
    CHECK_FALSE(loaded.config.forces.gas_star_converging);
    CHECK(loaded.config.forces.gas_star_temperature == 2);
    CHECK(loaded.galaxies[0].initial_young_fraction == .2);
    CHECK_THROWS((void)load_scene(scene + "initial_young_fraction -1\n"));
    CHECK_THROWS((void)load_scene(scene + "star_formation_mode other\n"));
    CHECK_THROWS((void)load_scene(scene + "star_formation_converging perhaps\n"));
    State s; Particle p; p.mass = .1; p.component = Component::Gas; p.internal_energy = .004;
    s.add(p); s.density[0] = 2; s.divergence[0] = 1;
    ForceSet f; f.gas_continuous_stars = true; f.gas_resolved_star_density = 1;
    f.gas_star_time = 1; f.gas_sound_speed = .05;
    CHECK(form_stars(s, f, .01, 1) == 0);
    f.gas_star_converging = false;
    CHECK(form_stars(s, f, .01, 1) == 0); // still too warm
    f.gas_star_temperature = 2;
    CHECK(form_stars(s, f, .01, 1) == 1);
}
