#include "forces/forces.hpp"
#include "integrators/integrators.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

TEST_CASE("pairwise Newtonian acceleration is G m / r^2 along the separation") {
    physics::State state;
    state.add(physics::Particle{2.0, physics::Vec3{0.0, 0.0, 0.0}, {}});
    state.add(physics::Particle{3.0, physics::Vec3{2.0, 0.0, 0.0}, {}});
    physics::ForceSet forces;
    forces.gravity = true;
    forces.gravitational_constant = 1.5;

    physics::Accelerations accelerations;
    physics::compute_accelerations(state, accelerations, forces);

    // a_0 = G m_1 / r^2 toward +x. r = 2, so a = 1.5 * 3 / 4 = 1.125.
    CHECK(accelerations.ax[0] == doctest::Approx(1.125));
    CHECK(accelerations.ay[0] == doctest::Approx(0.0));
    CHECK(accelerations.ax[1] == doctest::Approx(-1.5 * 2.0 / 4.0));
    CHECK(accelerations.az[1] == doctest::Approx(0.0));
}

TEST_CASE("direct N-body gravity matches an independent pair loop") {
    physics::State state;
    for (int i = 0; i < 80; ++i) {
        const double x = static_cast<double>(i % 10) * 0.5;
        const double y = static_cast<double>((i / 10) % 10) * 0.5;
        const double z = static_cast<double>(i / 50) * 0.25;
        state.add(physics::Particle{1.0 + 0.01 * i, physics::Vec3{x, y, z}, {}});
    }
    const double gravitational_constant = 0.7;
    physics::ForceSet forces;
    forces.gravity = true;
    forces.gravitational_constant = gravitational_constant;
    physics::Accelerations accelerations;
    physics::compute_accelerations(state, accelerations, forces);

    for (int i = 0; i < 80; ++i) {
        double ax = 0.0;
        double ay = 0.0;
        double az = 0.0;
        for (int j = 0; j < 80; ++j) {
            if (i == j) {
                continue;
            }
            const double dx = state.x[j] - state.x[i];
            const double dy = state.y[j] - state.y[i];
            const double dz = state.z[j] - state.z[i];
            const double radius_squared = dx * dx + dy * dy + dz * dz;
            const double inverse_radius = 1.0 / std::sqrt(radius_squared);
            const double scale = gravitational_constant * state.mass[j] * inverse_radius * inverse_radius * inverse_radius;
            ax += scale * dx;
            ay += scale * dy;
            az += scale * dz;
        }
        CHECK(accelerations.ax[i] == doctest::Approx(ax).epsilon(1e-12));
        CHECK(accelerations.ay[i] == doctest::Approx(ay).epsilon(1e-12));
        CHECK(accelerations.az[i] == doctest::Approx(az).epsilon(1e-12));
    }
}

TEST_CASE("softened gravity stays finite and matches the Plummer kernel") {
    physics::State stacked;
    stacked.add(physics::Particle{1.0, physics::Vec3{0.0, 0.0, 0.0}, {}});
    stacked.add(physics::Particle{1.0, physics::Vec3{0.0, 0.0, 0.0}, {}});
    physics::ForceSet forces;
    forces.gravity = true;
    forces.gravitational_constant = 2.0;
    forces.softening_length = 0.5;

    physics::Accelerations accelerations;
    physics::compute_accelerations(stacked, accelerations, forces);
    CHECK(accelerations.ax[0] == doctest::Approx(0.0).scale(1.0).epsilon(1e-12));
    CHECK(accelerations.ay[0] == doctest::Approx(0.0).scale(1.0).epsilon(1e-12));
    CHECK(accelerations.az[0] == doctest::Approx(0.0).scale(1.0).epsilon(1e-12));
    // U = -G m m / ε at r = 0.
    CHECK(physics::potential_energy(stacked, forces) == doctest::Approx(-4.0));

    physics::State separated;
    separated.add(physics::Particle{1.0, physics::Vec3{0.0, 0.0, 0.0}, {}});
    separated.add(physics::Particle{1.0, physics::Vec3{3.0, 0.0, 0.0}, {}});
    forces.softening_length = 4.0;
    physics::compute_accelerations(separated, accelerations, forces);
    // a = G m dx / (r^2 + ε^2)^{3/2} = 2 * 3 / 125.
    CHECK(accelerations.ax[0] == doctest::Approx(6.0 / 125.0));
    CHECK(accelerations.ax[1] == doctest::Approx(-6.0 / 125.0));
    CHECK(physics::potential_energy(separated, forces) == doctest::Approx(-0.4));
}

TEST_CASE("coincident particles make Newtonian gravity fail loudly") {
    physics::State state;
    state.add(physics::Particle{1.0, physics::Vec3{1.0, 2.0, 3.0}, {}});
    state.add(physics::Particle{1.0, physics::Vec3{1.0, 2.0, 3.0}, {}});
    physics::ForceSet forces;
    forces.gravity = true;
    forces.gravitational_constant = 1.0;
    physics::Accelerations accelerations;
    CHECK_THROWS_AS(physics::compute_accelerations(state, accelerations, forces), std::runtime_error);
}

TEST_CASE("a two-body N-body integration reproduces the circular Kepler orbit") {
    using namespace physics;
    const double separation = 1.0;
    const double period = 2.0 * std::numbers::pi * std::sqrt(std::pow(separation, 3.0) / 2.0);
    const double speed = 0.5 * std::sqrt(2.0);
    const int steps_per_period = 500;
    const double dt = period / static_cast<double>(steps_per_period);
    ForceSet forces;
    forces.gravity = true;
    forces.gravitational_constant = 1.0;
    State state;
    state.add(Particle{1.0, Vec3{-0.5, 0.0, 0.0}, Vec3{0.0, speed, 0.0}});
    state.add(Particle{1.0, Vec3{0.5, 0.0, 0.0}, Vec3{0.0, -speed, 0.0}});
    const Diagnostics initial = diagnose(state, forces);
    advance_velocity_verlet(state, forces, dt, static_cast<std::size_t>(8 * steps_per_period));
    const Diagnostics final_state = diagnose(state, forces);

    const double dt_over_period = 1.0 / 500.0;
    const double position_tolerance = 40.0 * 2.0 * std::numbers::pi * 8.0 * dt_over_period * dt_over_period;
    CHECK(std::abs(state.x[0] + 0.5 * separation) <= position_tolerance);
    CHECK(std::abs(state.y[0]) <= position_tolerance);
    CHECK(std::abs(final_state.energy - initial.energy) / std::abs(initial.energy) <
          80.0 * dt_over_period * dt_over_period);
    CHECK(norm(final_state.linear_momentum) < 1e-9);
}

TEST_CASE("a three-body run conserves momentum and keeps energy in a band") {
    using namespace physics;
    ForceSet forces;
    forces.gravity = true;
    forces.gravitational_constant = 1.0;
    State state;
    state.add(Particle{1.0, Vec3{0.0, 0.0, 0.0}, Vec3{0.05, 0.02, 0.0}});
    state.add(Particle{1.0, Vec3{1.2, 0.0, 0.0}, Vec3{-0.03, 0.04, 0.0}});
    state.add(Particle{1.0, Vec3{0.2, 1.4, 0.0}, Vec3{-0.02, -0.06, 0.01}});
    const Diagnostics initial = diagnose(state, forces);
    const double dt = 1.0e-3;
    const int steps = 400;
    Accelerations accelerations;
    compute_accelerations(state, accelerations, forces);
    double max_relative_energy = 0.0;
    double max_momentum_error = 0.0;
    double max_angular_error = 0.0;
    for (int step = 0; step < steps; ++step) {
        velocity_verlet_step(state, accelerations, dt, forces);
        const Diagnostics diagnostics = diagnose(state, forces);
        max_relative_energy =
            std::max(max_relative_energy, std::abs(diagnostics.energy - initial.energy) / std::abs(initial.energy));
        max_momentum_error = std::max(max_momentum_error, norm(diagnostics.linear_momentum - initial.linear_momentum));
        max_angular_error =
            std::max(max_angular_error, norm(diagnostics.angular_momentum - initial.angular_momentum));
    }
    INFO("energy ", max_relative_energy);
    CHECK(max_relative_energy < 1e-6);
    CHECK(max_momentum_error < 1e-10);
    CHECK(max_angular_error < 1e-9);
}
