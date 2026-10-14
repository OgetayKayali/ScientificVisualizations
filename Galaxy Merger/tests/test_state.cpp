#include "core/state.hpp"
#include "core/units.hpp"

#include <doctest/doctest.h>

#include <stdexcept>

TEST_CASE("named unit conversions use the exact SI and IAU definitions") {
    using namespace physics::units;
    CHECK(au == 149597870700.0);
    CHECK(day == 86400.0);
    CHECK(julian_year == 365.25 * day);
    CHECK(kilometre == 1000.0);
    CHECK(G_codata_2018 == 6.67430e-11);
}

TEST_CASE("momentum diagnostics match r cross p for a single particle") {
    physics::State state;
    state.add(physics::Particle{2.0, physics::Vec3{1.0, 0.0, 0.0}, physics::Vec3{0.0, 1.0, 0.0}});

    const physics::Vec3 momentum = physics::linear_momentum(state);
    CHECK(momentum.x == doctest::Approx(0.0));
    CHECK(momentum.y == doctest::Approx(2.0));
    CHECK(momentum.z == doctest::Approx(0.0));

    const physics::Vec3 angular = physics::angular_momentum(state);
    CHECK(angular.x == doctest::Approx(0.0));
    CHECK(angular.y == doctest::Approx(0.0));
    CHECK(angular.z == doctest::Approx(2.0));

    CHECK(physics::kinetic_energy(state) == doctest::Approx(1.0));
}

TEST_CASE("a non-positive mass is rejected") {
    physics::State state;
    CHECK_THROWS_AS(state.add(physics::Particle{0.0, {}, {}}), std::invalid_argument);
    CHECK_THROWS_AS(state.add(physics::Particle{-1.0, {}, {}}), std::invalid_argument);
}
