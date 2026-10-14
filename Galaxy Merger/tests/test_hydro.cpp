#include "core/component.hpp"
#include "forces/forces.hpp"
#include "forces/hydro.hpp"
#include "integrators/integrators.hpp"
#include "simulation/scene.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

physics::BodyDesc gas_body(std::string name, physics::Vec3 position, physics::Vec3 velocity, double energy) {
    physics::BodyDesc body;
    body.name = std::move(name);
    body.particle.mass = 0.02;
    body.particle.component = physics::Component::Gas;
    body.particle.position = position;
    body.particle.velocity = velocity;
    body.particle.internal_energy = energy;
    body.display_radius = 0.05;
    return body;
}

double cold_energy(double sound_speed) {
    constexpr double gamma = 5.0 / 3.0;
    return sound_speed * sound_speed / (gamma * (gamma - 1.0));
}

}  // namespace

TEST_CASE("gas update refreshes forces after cooling and converges without energy rescaling") {
    auto experiment=[](double dt) {
        physics::State s;
        for (int i=0;i<27;++i) {
            physics::Particle p;
            p.mass=0.01; p.component=physics::Component::HotGas; p.internal_energy=0.05;
            p.position={0.1*((i%3)-1),0.1*(((i/3)%3)-1),0.1*((i/9)-1)};
            p.velocity=p.position*-0.1; s.add(p);
        }
        physics::ForceSet f; f.gas_sound_speed=0.05; f.gas_cooling_time=0.2; f.softening_length=0.05;
        physics::prepare_gas(s,f);
        const double initial=physics::diagnose(s,f).energy;
        physics::Accelerations a; physics::compute_accelerations(s,a,f);
        for (int i=0;i<static_cast<int>(std::lround(0.5/dt));++i)
            physics::velocity_verlet_step(s,a,dt,f);
        physics::Accelerations fresh; physics::compute_accelerations(s,fresh,f);
        for (std::size_t i=0;i<s.size();++i) {
            CHECK(a.ax[i]==doctest::Approx(fresh.ax[i]).epsilon(1e-10));
            CHECK(a.ay[i]==doctest::Approx(fresh.ay[i]).epsilon(1e-10));
        }
        return std::abs(physics::diagnose(s,f).energy-initial+s.radiated_energy-s.floor_energy_added)/initial;
    };
    const double coarse=experiment(0.02),fine=experiment(0.01);
    CHECK(coarse<1e-4);
    CHECK(fine<0.4*coarse);
}

TEST_CASE("cooling uses the remainder of a step after feedback delay expires") {
    physics::State s;
    physics::Particle p; p.mass=1; p.component=physics::Component::Gas; p.internal_energy=0.01;
    s.add(p); s.density={1}; s.cooling_delay={0.05};
    physics::ForceSet f; f.gas_sound_speed=0.05; f.gas_cooling_time=0.2; f.gas_reference_density=1;
    auto reference=s; reference.cooling_delay[0]=0;
    physics::cool_gas(s,f,0.1);
    physics::cool_gas(reference,f,0.05);
    CHECK(s.internal_energy[0]==doctest::Approx(reference.internal_energy[0]));
    CHECK(s.radiated_energy==doctest::Approx(reference.radiated_energy));
}

TEST_CASE("gas predictor corrector preserves state and energy budgets across Continue") {
    physics::State state;
    std::vector<physics::BodyDesc> bodies;
    for (int i=0;i<8;++i) {
        auto b=gas_body("gas"+std::to_string(i), {0.15*(i%2),0.15*((i/2)%2),0.15*(i/4)}, {}, 0.04);
        state.add(b.particle); bodies.push_back(b);
    }
    physics::SimulationConfig config;
    config.dt=0.01; config.steps=20; config.sample_every=1;
    config.forces.gas_sound_speed=0.05; config.forces.gas_cooling_time=0.3;
    config.forces.gravity=true; config.forces.gravitational_constant=0.1; config.forces.softening_length=0.05;
    physics::prepare_gas(state,config.forces);
    const auto full=physics::simulate_interruptible(state,bodies,config,0,nullptr,nullptr);
    auto first_config=config; first_config.steps=7;
    auto first=physics::simulate_interruptible(state,bodies,first_config,0,nullptr,nullptr);
    const auto resumed=physics::simulate_interruptible(first.state,bodies,config,7,nullptr,nullptr);
    for (std::size_t i=0;i<state.size();++i) {
        CHECK(physics::norm(full.state.position(i)-resumed.state.position(i))<1e-12);
        CHECK(physics::norm(full.state.velocity(i)-resumed.state.velocity(i))<1e-12);
        CHECK(full.state.internal_energy[i]==doctest::Approx(resumed.state.internal_energy[i]).epsilon(1e-12));
    }
    CHECK(full.state.radiated_energy==doctest::Approx(resumed.state.radiated_energy).epsilon(1e-12));
    CHECK(full.state.floor_energy_added==doctest::Approx(resumed.state.floor_energy_added).epsilon(1e-12));
}

TEST_CASE("warm gas cools and much hotter gas keeps its heat") {
    const double floor_energy = cold_energy(0.05);
    physics::State state;
    physics::Particle warm;
    warm.mass = 0.05;
    warm.component = physics::Component::Gas;
    warm.internal_energy = 10.0 * floor_energy;
    state.add(warm);
    physics::Particle hot;
    hot.mass = 0.05;
    hot.component = physics::Component::HotGas;
    hot.internal_energy = 400.0 * floor_energy;
    state.add(hot);
    state.density = {1.0, 1.0};
    physics::ForceSet forces;
    forces.gas_sound_speed = 0.05;
    forces.gas_cooling_time = 0.05;
    forces.gas_reference_density = 1.0;
    for (int step = 0; step < 20; ++step) {
        physics::cool_gas(state, forces, 0.05);
    }
    CHECK(state.internal_energy[0] < 2.0 * floor_energy);
    CHECK(state.internal_energy[1] > 80.0 * floor_energy);
    CHECK(state.component[1] == static_cast<std::uint8_t>(physics::Component::HotGas));
}

TEST_CASE("cooling brings gas down to the sound-speed floor and not below it") {
    const double floor_energy = cold_energy(0.05);
    std::vector<physics::BodyDesc> bodies;
    physics::State state;
    for (int index = 0; index < 48; ++index) {
        const double x = 0.02 * static_cast<double>(index % 4);
        const double y = 0.02 * static_cast<double>((index / 4) % 4);
        const double z = 0.02 * static_cast<double>(index / 16);
        physics::BodyDesc body = gas_body("g" + std::to_string(index), {x, y, z}, {}, 30.0 * floor_energy);
        state.add(body.particle);
        bodies.push_back(std::move(body));
    }
    physics::SimulationConfig config;
    config.dt = 0.02;
    config.steps = 80;
    config.sample_every = 80;
    config.forces.gas_sound_speed = 0.05;
    config.forces.gas_cooling_time = 0.05;
    config.forces.gas_star_time = 1000.0;
    config.forces.gas_star_density = 1.0e9;
    const physics::SimulationOutcome outcome =
        physics::simulate_interruptible(state, bodies, config, 0, nullptr, nullptr);
    const physics::State& end = outcome.state;
    for (std::size_t index = 0; index < end.size(); ++index) {
        CHECK(end.internal_energy[index] >= floor_energy * 0.999);
        CHECK(end.internal_energy[index] < 2.0 * floor_energy);
        CHECK(end.component[index] == static_cast<std::uint8_t>(physics::Component::Gas));
    }
}

TEST_CASE("cold dense converging gas becomes stars and a hot particle does not") {
    const double floor_energy = cold_energy(0.05);
    physics::State state;
    for (int index = 0; index < 16; ++index) {
        physics::Particle particle;
        particle.mass = 0.05;
        particle.component = physics::Component::Gas;
        particle.position = {0.1 * static_cast<double>(index % 4), 0.1 * static_cast<double>(index / 4), 0.0};
        particle.internal_energy = index == 0 ? 40.0 : floor_energy;
        state.add(particle);
        state.density[index] = 8.0;
        state.divergence[index] = index == 1 ? 0.2 : -1.0;
    }
    physics::ForceSet forces;
    forces.gas_sound_speed = 0.05;
    forces.gas_resolved_star_density = 1.0;
    forces.gas_star_time = 0.01;
    forces.gas_seed = 11;
    const int formed = physics::form_stars(state, forces, 0.05, 3);
    CHECK(formed == 14);
    CHECK(state.component[0] == static_cast<std::uint8_t>(physics::Component::Gas));
    CHECK(state.component[1] == static_cast<std::uint8_t>(physics::Component::Gas));
    for (std::size_t index = 2; index < state.size(); ++index) {
        CHECK(state.component[index] == static_cast<std::uint8_t>(physics::Component::YoungStar));
        CHECK(state.internal_energy[index] == 0.0);
    }
    const double mass_after = [&] {
        double mass = 0.0;
        for (double value : state.mass) {
            mass += value;
        }
        return mass;
    }();
    CHECK(mass_after == doctest::Approx(0.8));
}

TEST_CASE("gas pressure pushes two clouds apart and keeps momentum") {
    std::vector<physics::BodyDesc> bodies;
    physics::State state;
    for (int index = 0; index < 32; ++index) {
        const bool right = index >= 16;
        const int local = index % 16;
        physics::Vec3 position{right ? 0.04 : -0.04, 0.015 * static_cast<double>(local % 4),
                               0.015 * static_cast<double>(local / 4)};
        physics::BodyDesc body = gas_body("c" + std::to_string(index), position, {}, 1.0);
        state.add(body.particle);
        bodies.push_back(std::move(body));
    }
    physics::ForceSet forces;
    forces.gas_sound_speed = 0.01;
    physics::Accelerations accelerations;
    physics::compute_accelerations(state, accelerations, forces);
    double left_acceleration = 0.0;
    double right_acceleration = 0.0;
    for (int index = 0; index < 32; ++index) {
        if (index < 16) {
            left_acceleration += accelerations.ax[static_cast<std::size_t>(index)];
        } else {
            right_acceleration += accelerations.ax[static_cast<std::size_t>(index)];
        }
    }
    CHECK(left_acceleration < 0.0);
    CHECK(right_acceleration > 0.0);
    CHECK(left_acceleration + right_acceleration == doctest::Approx(0.0).epsilon(1e-8));

    physics::SimulationConfig config;
    config.dt = 0.002;
    config.steps = 30;
    config.sample_every = 30;
    config.forces = forces;
    const physics::SimulationOutcome outcome = physics::simulate_interruptible(state, bodies, config, 0, nullptr, nullptr);
    double left = 0.0;
    double right = 0.0;
    for (std::size_t index = 0; index < outcome.state.size(); ++index) {
        if (index < 16) {
            left += outcome.state.x[index];
        } else {
            right += outcome.state.x[index];
        }
    }
    CHECK(left / 16.0 < -0.04);
    CHECK(right / 16.0 > 0.04);
    CHECK(physics::norm(outcome.trajectory.samples.back().diagnostics.linear_momentum) < 1e-8);
}

TEST_CASE("a galaxy scene places cold gas on top of the bulge and the disk") {
    const char* text = R"(integrator velocity_verlet
dt 0.02
steps 2
sample_every 1
gravitational_constant 1
seed 5
force gravity
softening 0.05
galaxy mix
particles 180
halo_mass 2
halo_scale 2
halo_cutoff 16
bulge_mass 0.3
bulge_scale 0.2
disk_mass 1
disk_scale_radius 1
disk_height 0.2
gas_mass 0.15
gas_particles 48
gas_height 0.05
sound_speed 0.05
cooling_time 0.5
star_formation_time 8
feedback_energy 6
feedback_time 2
)";
    const physics::Scene scene = physics::load_scene(text);
    REQUIRE(scene.bodies.size() == 228);
    int gas = 0;
    int bulge = 0;
    for (physics::BodyDesc const& body : scene.bodies) {
        if (body.particle.component == physics::Component::Gas) {
            ++gas;
            CHECK(body.particle.internal_energy > 0.0);
            CHECK(body.formed_star_emission > 0.0);
            CHECK(body.name.find(".gas.") != std::string::npos);
        }
        if (body.particle.component == physics::Component::Bulge) {
            ++bulge;
        }
    }
    CHECK(gas == 48);
    CHECK(bulge > 8);
    CHECK(scene.config.forces.gas_sound_speed == doctest::Approx(0.05));
    CHECK(scene.config.forces.gas_cooling_time == doctest::Approx(0.5));
    CHECK(scene.config.forces.gas_feedback_energy == doctest::Approx(6.0));
    CHECK(scene.config.forces.gas_feedback_time == doctest::Approx(2.0));
}

TEST_CASE("a hot halo is hotter than the disk and stays extended") {
    const char* text = R"(integrator velocity_verlet
dt 0.02
steps 120
sample_every 120
gravitational_constant 1
seed 5
force gravity
softening 0.05
galaxy halo
particles 80
halo_mass 2
halo_scale 2
halo_cutoff 12
bulge_mass 0.2
bulge_scale 0.2
disk_mass 0.6
disk_scale_radius 1
disk_height 0.2
gas_mass 0.1
gas_particles 64
sound_speed 0.05
cooling_time 1
star_formation_time 8
hot_gas_mass 0.05
hot_gas_particles 48
)";
    const physics::Scene scene = physics::load_scene(text);
    const double floor_energy = cold_energy(0.05);
    int cold = 0;
    int hot = 0;
    double hot_radius = 0.0;
    double cold_energy_sum = 0.0;
    for (physics::BodyDesc const& body : scene.bodies) {
        if (body.particle.component == physics::Component::Gas) {
            ++cold;
            cold_energy_sum += body.particle.internal_energy;
            CHECK(body.formed_star_emission > 0.0);
        }
        if (body.particle.component == physics::Component::HotGas) {
            ++hot;
            hot_radius += physics::norm(body.particle.position);
            CHECK(body.particle.internal_energy > 50.0 * floor_energy);
            CHECK(body.name.find(".hot.") != std::string::npos);
            CHECK(body.formed_star_emission == 0.0);
        }
    }
    CHECK(cold == 64);
    CHECK(hot == 48);
    CHECK(scene.bodies.size() == 80 + 64 + 48);
    CHECK(cold_energy_sum / 64.0 < 2.0 * floor_energy);
    const double initial_mean = hot_radius / 48.0;
    CHECK(initial_mean > 0.4);
    CHECK(scene.config.forces.gas_hot_kernel_limit == doctest::Approx(2.0));

    physics::State state = scene.make_state();
    const physics::SimulationOutcome outcome =
        physics::simulate_interruptible(state, scene.bodies, scene.config, 0, nullptr, nullptr);
    double final_radius = 0.0;
    int final_hot = 0;
    for (std::size_t index = 0; index < outcome.state.size(); ++index) {
        if (outcome.state.component[index] != static_cast<std::uint8_t>(physics::Component::HotGas)) {
            continue;
        }
        ++final_hot;
        final_radius += std::sqrt(outcome.state.x[index] * outcome.state.x[index] +
                                  outcome.state.y[index] * outcome.state.y[index] +
                                  outcome.state.z[index] * outcome.state.z[index]);
        CHECK(outcome.state.internal_energy[index] > 40.0 * floor_energy);
    }
    CHECK(final_hot == 48);
    CHECK(final_radius / 48.0 > 0.45 * initial_mean);
    CHECK(final_radius / 48.0 < 3.0 * initial_mean);
}

TEST_CASE("two galaxies with different feedback are rejected") {
    const char* text = R"(integrator velocity_verlet
dt 0.02
steps 1
sample_every 1
gravitational_constant 1
seed 5
force gravity
softening 0.05
galaxy left
particles 40
halo_mass 2
halo_scale 2
disk_mass 1
disk_scale_radius 1
disk_height 0.2
bulge_mass 0.3
bulge_scale 0.2
gas_mass 0.15
gas_particles 48
sound_speed 0.05
cooling_time 1
star_formation_time 5
feedback_energy 4
feedback_time 1
galaxy right
particles 40
halo_mass 2
halo_scale 2
disk_mass 1
disk_scale_radius 1
disk_height 0.2
bulge_mass 0.3
bulge_scale 0.2
gas_mass 0.15
gas_particles 48
sound_speed 0.05
cooling_time 1
star_formation_time 5
feedback_energy 8
feedback_time 1
)";
    CHECK_THROWS_AS([&] {
                        physics::Scene rejected = physics::load_scene(text);
                        (void)rejected;
                    }(),
                    std::runtime_error);
}

TEST_CASE("a new star heats the remaining gas and leaves a distant particle alone") {
    const double floor_energy = cold_energy(0.05);
    physics::State state;
    const int count = 8;
    for (int index = 0; index < count; ++index) {
        physics::Particle particle;
        particle.mass = 0.05;
        particle.component = physics::Component::Gas;
        particle.internal_energy = floor_energy;
        if (index == 0) {
            particle.position = {};
        } else if (index == count - 1) {
            particle.position = {3.0, 0.0, 0.0};
        } else {
            const double angle = 2.0 * 3.14159265358979323846 * static_cast<double>(index - 1) / 6.0;
            particle.position = {0.08 * std::cos(angle), 0.08 * std::sin(angle), 0.0};
        }
        state.add(particle);
        state.density[static_cast<std::size_t>(index)] = index == 0 ? 20.0 : 1.0;
        state.divergence[static_cast<std::size_t>(index)] = index == 0 ? -1.0 : 0.0;
    }
    physics::ForceSet forces;
    forces.gas_sound_speed = 0.05;
    forces.gas_resolved_star_density = 5.0;
    forces.gas_star_time = 0.01;
    forces.gas_feedback_energy = 4.0;
    forces.gas_feedback_time = 1.25;
    forces.gas_seed = 3;
    const int formed = physics::form_stars(state, forces, 0.05, 1);
    CHECK(formed == 1);
    CHECK(state.component[0] == static_cast<std::uint8_t>(physics::Component::YoungStar));
    CHECK(state.birth_time[0] == 0.0);
    CHECK(state.birth_time[1] == -1.0);
    CHECK(state.internal_energy[0] == 0.0);
    CHECK(state.cooling_delay[0] == 0.0);
    CHECK(state.vx[0] == 0.0);
    double injected = 0.0;
    for (std::size_t index = 1; index < state.size(); ++index) {
        CHECK(state.component[index] == static_cast<std::uint8_t>(physics::Component::Gas));
        CHECK(state.vx[index] == 0.0);
        injected += state.mass[index] * (state.internal_energy[index] - floor_energy);
    }
    CHECK(injected == doctest::Approx(forces.gas_feedback_energy * state.mass[0]));
    CHECK(state.feedback_energy == doctest::Approx(injected));
    CHECK(state.formation_energy_removed == doctest::Approx(state.mass[0] * floor_energy));
    CHECK(state.internal_energy.back() == doctest::Approx(floor_energy));
    CHECK(state.cooling_delay.back() == 0.0);
    for (std::size_t index = 1; index + 1 < state.size(); ++index) {
        CHECK(state.internal_energy[index] > floor_energy);
        CHECK(state.cooling_delay[index] == doctest::Approx(forces.gas_feedback_time));
    }

    physics::State isolated;
    for (int index = 0; index < 2; ++index) {
        physics::Particle particle;
        particle.mass = 0.05;
        particle.component = physics::Component::Gas;
        particle.internal_energy = floor_energy;
        particle.position = {index == 0 ? 0.0 : 2.0, 0.0, 0.0};
        isolated.add(particle);
        isolated.density[static_cast<std::size_t>(index)] = index == 0 ? 20.0 : 1.0;
        isolated.divergence[static_cast<std::size_t>(index)] = index == 0 ? -1.0 : 0.0;
    }
    CHECK(physics::form_stars(isolated, forces, 0.05, 1) == 1);
    CHECK(isolated.mass[1] * (isolated.internal_energy[1] - floor_energy) ==
          doctest::Approx(forces.gas_feedback_energy * isolated.mass[0]));
    CHECK(isolated.cooling_delay[1] == doctest::Approx(forces.gas_feedback_time));
}

TEST_CASE("hot and cold gas both receive compression and expansion work") {
    for (double rate : {-0.4, 0.6, -4.0}) {
        physics::State state;
        for (auto component : {physics::Component::Gas, physics::Component::HotGas, physics::Component::Disk}) {
            physics::Particle p;
            p.mass = 2;
            p.internal_energy = 1;
            p.component = component;
            state.add(p);
        }
        state.hydro_dudt = {rate, rate, rate};
        physics::kick_internal_energy(state, 0.5);
        CHECK(state.internal_energy[0] == doctest::Approx(std::max(0.0, 1 + rate * 0.5)));
        CHECK(state.internal_energy[1] == state.internal_energy[0]);
        CHECK(state.internal_energy[2] == 1);
        CHECK(state.floor_energy_added == doctest::Approx(4 * std::max(0.0, -(1 + rate * 0.5))));
    }
}

TEST_CASE("thermal source accounting closes cooling and floor energy changes") {
    physics::State state;
    for (double energy : {0.0001, 0.1}) {
        physics::Particle p;
        p.mass = 0.3;
        p.internal_energy = energy;
        p.component = physics::Component::HotGas;
        state.add(p);
    }
    state.density = {1, 1};
    physics::ForceSet forces;
    forces.gas_sound_speed = 0.05;
    forces.gas_cooling_time = 1;
    forces.gas_reference_density = 1;
    const auto before = physics::diagnose(state, forces).energy;
    physics::cool_gas(state, forces, 0.2);
    const auto after = physics::diagnose(state, forces).energy;
    CHECK(state.radiated_energy > 0);
    CHECK(state.floor_energy_added > 0);
    CHECK(after - before + state.radiated_energy - state.floor_energy_added == doctest::Approx(0).epsilon(1e-12));
}

TEST_CASE("heated gas skips cooling until the delay runs out") {
    const double floor_energy = cold_energy(0.05);
    physics::State state;
    physics::Particle particle;
    particle.mass = 0.05;
    particle.component = physics::Component::Gas;
    particle.internal_energy = 2.0;
    state.add(particle);
    state.density[0] = 4.0;
    state.cooling_delay[0] = 0.3;
    physics::ForceSet forces;
    forces.gas_sound_speed = 0.05;
    forces.gas_cooling_time = 0.05;
    forces.gas_reference_density = 1.0;
    physics::cool_gas(state, forces, 0.1);
    CHECK(state.internal_energy[0] == doctest::Approx(2.0));
    CHECK(state.cooling_delay[0] == doctest::Approx(0.2));

    forces.gas_cooling_time = 0.0;
    physics::cool_gas(state, forces, 0.05);
    CHECK(state.internal_energy[0] == doctest::Approx(2.0));
    CHECK(state.cooling_delay[0] == doctest::Approx(0.15));

    forces.gas_cooling_time = 0.05;
    state.cooling_delay[0] = 0.0;
    physics::cool_gas(state, forces, 0.05);
    CHECK(state.internal_energy[0] < 2.0);
    CHECK(state.internal_energy[0] >= floor_energy);
}

TEST_CASE("feedback pressure expands a gas cloud and keeps momentum") {
    const double floor_energy = cold_energy(0.05);
    physics::State state;
    const int neighbors = 8;
    for (int index = 0; index < neighbors + 1; ++index) {
        physics::Particle particle;
        particle.mass = 0.05;
        particle.component = physics::Component::Gas;
        particle.internal_energy = floor_energy;
        if (index == 0) {
            particle.position = {};
        } else {
            const double angle = 2.0 * 3.14159265358979323846 * static_cast<double>(index - 1) / neighbors;
            particle.position = {0.08 * std::cos(angle), 0.08 * std::sin(angle), 0.0};
        }
        state.add(particle);
        state.density[static_cast<std::size_t>(index)] = index == 0 ? 20.0 : 1.0;
        state.divergence[static_cast<std::size_t>(index)] = index == 0 ? -1.0 : 0.0;
    }
    physics::ForceSet forces;
    forces.gas_sound_speed = 0.05;
    forces.gas_cooling_time = 0.05;
    forces.gas_reference_density = 1.0;
    forces.gas_resolved_star_density = 5.0;
    forces.gas_star_time = 0.01;
    forces.gas_feedback_energy = 12.0;
    forces.gas_feedback_time = 20.0;
    forces.softening_length = 0.02;
    CHECK(physics::form_stars(state, forces, 0.05, 1) == 1);

    auto rms = [](physics::State const& current) {
        double sum = 0.0;
        int count = 0;
        for (std::size_t index = 0; index < current.size(); ++index) {
            if (current.component[index] != static_cast<std::uint8_t>(physics::Component::Gas)) {
                continue;
            }
            sum += current.x[index] * current.x[index] + current.y[index] * current.y[index] +
                   current.z[index] * current.z[index];
            ++count;
        }
        return std::sqrt(sum / static_cast<double>(count));
    };
    physics::Accelerations accelerations;
    physics::compute_accelerations(state, accelerations, forces);
    const double radius_before = rms(state);
    for (int step = 0; step < 40; ++step) {
        physics::velocity_verlet_step(state, accelerations, 0.02, forces);
    }
    CHECK(rms(state) > 2.0 * radius_before);
    double px = 0.0;
    double py = 0.0;
    double pz = 0.0;
    for (std::size_t index = 0; index < state.size(); ++index) {
        px += state.mass[index] * state.vx[index];
        py += state.mass[index] * state.vy[index];
        pz += state.mass[index] * state.vz[index];
    }
    CHECK(px == doctest::Approx(0.0).epsilon(1e-8));
    CHECK(py == doctest::Approx(0.0).epsilon(1e-8));
    CHECK(pz == doctest::Approx(0.0).epsilon(1e-8));
}
