#include "core/spatial_tree.hpp"
#include "io/trajectory.hpp"
#include "visualization/optical.hpp"
#include "visualization/tracers.hpp"
#include <doctest/doctest.h>
#include <filesystem>
#include <numeric>

using namespace physics;

TEST_CASE("spatial tree reproduces exact neighbor queries and bounded gravity approximation") {
    std::vector<Vec3> positions;
    std::vector<double> mass;
    std::vector<std::size_t> ids;
    for (std::size_t i = 0; i < 300; ++i) {
        positions.push_back({std::sin(0.7 * i), std::cos(1.3 * i), std::sin(1.1 * i)});
        mass.push_back(0.01 + 0.001 * (i % 7));
        ids.push_back(i);
    }
    SpatialTree tree(positions, ids, &mass);
    for (std::size_t query = 0; query < 20; ++query) {
        const auto p = positions[query];
        std::vector<std::pair<double, std::size_t>> brute;
        Vec3 exact{};
        for (auto i : ids) {
            auto d = positions[i] - p;
            const double r2 = dot(d, d) + 0.01;
            exact += d * (mass[i] / (r2 * std::sqrt(r2)));
            if (i != query)
                brute.emplace_back(dot(d, d), i);
        }
        std::sort(brute.begin(), brute.end());
        brute.resize(16);
        CHECK(tree.nearest(p, 16, query) == brute);
        CHECK(norm(tree.acceleration(p, 1, 0.1, 0) - exact) < 1e-12);
        CHECK(norm(tree.acceleration(p, 1, 0.1, 0.4) - exact) < 0.02 * (norm(exact) + 0.001));
    }
}

static Trajectory orbit_source() {
    Trajectory t;
    BodyRecord center;
    center.name = "central";
    center.mass = 1;
    t.bodies.push_back(center);
    for (int i = 0; i < 2; ++i) {
        BodyRecord disk;
        disk.name = "test.disk." + std::to_string(i);
        disk.mass = 1e-12;
        disk.emission = 2;
        t.bodies.push_back(disk);
    }
    const double speed = std::sqrt(1 / std::pow(1.01, 1.5));
    for (int i = 0; i <= 20; ++i) {
        Sample s;
        s.time = i * 0.1;
        s.position = {{0, 0, 0}, {1, 0, 0}, {1, 0, 0}};
        s.velocity = {{}, {0, speed, 0}, {0, speed, 0}};
        s.component = {0, 3, 3};
        t.samples.push_back(s);
    }
    return t;
}

TEST_CASE("massless light follows independent orbits without modifying source dynamics or luminosity") {
    const auto source = orbit_source();
    visual::TracerOptions o;
    o.per_star = 2;
    o.dt = 0.04;
    o.softening = 0.1;
    o.theta = 0;
    const auto coarse = visual::refine_starlight(source, 1, o);
    o.dt = 0.02;
    const auto fine = visual::refine_starlight(source, 1, o);
    REQUIRE(fine.bodies.size() == 7);
    for (std::size_t f = 0; f < source.samples.size(); ++f) {
        for (std::size_t i = 0; i < 3; ++i) {
            CHECK(fine.samples[f].position[i] == source.samples[f].position[i]);
            CHECK(fine.samples[f].velocity[i] == source.samples[f].velocity[i]);
            CHECK(fine.bodies[i].mass == source.bodies[i].mass);
        }
        CHECK(fine.samples[f].diagnostics.energy == source.samples[f].diagnostics.energy);
    }
    double light = 0;
    for (auto const &b : fine.bodies)
        light += (b.light_weight >= 0 ? b.light_weight : b.mass) * b.emission;
    CHECK(light / 4e-12 == doctest::Approx(1).epsilon(1e-12));
    for (std::size_t i = 3; i < fine.bodies.size(); ++i)
        CHECK(fine.bodies[i].mass == 0);
    const double omega = std::sqrt(1 / std::pow(1.01, 1.5));
    const Vec3 exact{std::cos(2 * omega), std::sin(2 * omega), 0};
    const double coarse_error = norm(coarse.samples.back().position[3] - exact);
    const double fine_error = norm(fine.samples.back().position[3] - exact);
    CHECK(fine_error < 0.5 * coarse_error);
    CHECK(norm(fine.samples.back().position[3] - fine.samples.back().position[1]) > 1);
    const auto path = std::filesystem::temp_directory_path() / "physics_tracers_v6.traj";
    write_trajectory(fine, path.string());
    const auto loaded = read_trajectory(path.string());
    CHECK(loaded.tracer_softening == o.softening);
    CHECK(loaded.tracer_dt == o.dt);
    CHECK(loaded.bodies[3].mass == 0);
    CHECK(loaded.bodies[3].light_weight == fine.bodies[3].light_weight);
    CHECK(loaded.samples.back().position == fine.samples.back().position);
    CHECK_THROWS(visual::refine_starlight(fine, 1, o));
    o.per_star = 3;
    CHECK_THROWS(visual::refine_starlight(source, 1, o));
}
