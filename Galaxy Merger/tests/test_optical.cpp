#include "io/trajectory.hpp"
#include "simulation/validation.hpp"
#include "visualization/optical.hpp"
#include <cmath>
#include <doctest/doctest.h>
#include <filesystem>

using namespace physics;

TEST_CASE("optical kernels preserve luminosity as their footprint changes") {
    for (auto axes : {Vec3{0.2, 0.1, 0}, Vec3{2, 0.3, 0}, Vec3{1, 1, 0}}) {
        double integral = 0;
        constexpr int steps = 10000;
        for (int i = 0; i < steps; ++i) {
            const double r = (i + 0.5) / steps;
            integral +=
                visual::kernel_density(r * r, axes.x, axes.y) * 2 * 3.141592653589793 * axes.x * axes.y * r / steps;
        }
        CHECK(integral == doctest::Approx(1.0).epsilon(1e-6));
    }
}

TEST_CASE("finite depth dust conserves its column and surrounds embedded stars") {
    Trajectory t;
    Sample sample;
    std::vector<Vec3> positions;
    for (int i = 0; i < 17; ++i) {
        BodyRecord body;
        body.name = "test.gas." + std::to_string(i);
        body.mass = 0.01;
        body.emission = 0;
        t.bodies.push_back(body);
        positions.push_back({0.1 * (i - 8), 0, 0.08 * (i - 8)});
        sample.component.push_back(static_cast<std::uint8_t>(Component::Gas));
    }
    visual::OpticalSettings settings;
    const auto splats =
        visual::make_splats(t, &sample, positions, 0, {0, 0, 10}, {1, 0, 0}, {0, 1, 0}, {0, 0, -1}, settings);
    REQUIRE(splats.size() == 170);
    double tau = 0;
    for (auto const &s : splats) {
        tau += s.optical_depth;
        CHECK(std::isfinite(s.major));
        CHECK(s.minor > 0);
    }
    CHECK(tau == doctest::Approx(17 * 0.01 * 12));
    CHECK(splats.front().depth > 10);
    CHECK(splats.back().depth < 10);
}

TEST_CASE("multiscale stellar detail and varied texture conserve emitted light") {
    Trajectory t;
    Sample sample;
    std::vector<Vec3> positions;
    double expected = 0;
    for (int i = 0; i < 32; ++i) {
        BodyRecord b;
        b.name = "test.disk." + std::to_string(i);
        b.mass = 0.01;
        b.emission = 1 + 0.1 * i;
        expected += b.mass * b.emission;
        t.bodies.push_back(b);
        positions.push_back({0.1 * i, 0.01 * (i % 3), 0});
        sample.component.push_back(static_cast<std::uint8_t>(Component::Disk));
    }
    for (double detail : {0.0, 0.45, 1.0}) {
        visual::OpticalSettings settings;
        settings.detail = detail;
        settings.texture = 0.3;
        const auto splats =
            visual::make_splats(t, &sample, positions, 0, {0, 0, 10}, {1, 0, 0}, {0, 1, 0}, {0, 0, -1}, settings);
        double luminosity = 0;
        for (auto const &s : splats)
            luminosity += s.luminosity;
        CHECK(luminosity == doctest::Approx(expected));
    }
}

TEST_CASE("matched control uses actual merger particles and removes only imposed bulk motion") {
    auto scene = load_scene_file(std::string(PHYSICS_SOURCE_DIR) + "/examples/galaxy_merger.scene");
    const auto isolated = isolated_control(scene, 1);
    REQUIRE(isolated.bodies.size() * 2 == scene.bodies.size());
    for (std::size_t i = 0; i < isolated.bodies.size(); ++i) {
        const auto &parent = scene.bodies[i + isolated.bodies.size()].particle;
        const auto &child = isolated.bodies[i].particle;
        CHECK(norm(child.position + scene.galaxies[1].center - parent.position) < 1e-12);
        CHECK(norm(child.velocity + scene.galaxies[1].velocity - parent.velocity) < 1e-12);
        CHECK(child.mass == parent.mass);
        CHECK(child.internal_energy == parent.internal_energy);
    }
    CHECK(isolated.config.forces.gas_reference_density > 0);
    CHECK(isolated.config.dt == scene.config.dt);
    CHECK(isolated.config.steps == scene.config.steps);
    CHECK_THROWS(isolated_control(scene, 2));
}

TEST_CASE("playback follows curved motion without overshooting unresolved loops") {
    const Vec3 p0{1, 0, 0}, p1{0, 1, 0}, v0{0, 1, 0}, v1{-1, 0, 0};
    CHECK(visual::interpolate_position(p0, v0, p1, v1, 1, 0) == p0);
    CHECK(visual::interpolate_position(p0, v0, p1, v1, 1, 1) == p1);
    const auto midpoint = visual::interpolate_position(p0, v0, p1, v1, 1, 0.5);
    CHECK(midpoint.x > 0.5);
    CHECK(midpoint.y > 0.5);
    CHECK(norm(visual::interpolate_position(p0, {100, 0, 0}, p0, {-100, 0, 0}, 1, 0.5) - p0) < 1e-12);
    CHECK(visual::interpolate_position({0, 0, 0}, {2, 0, 0}, {2, 0, 0}, {2, 0, 0}, 1, 0.25).x == doctest::Approx(0.5));
}

TEST_CASE("optical light ignores dynamical halos and sorts foreground dust last") {
    Trajectory t;
    Sample s;
    for (auto c : {Component::Disk, Component::Gas, Component::Halo}) {
        BodyRecord b;
        b.mass = 0.01;
        b.emission = 1;
        t.bodies.push_back(b);
        s.component.push_back(static_cast<std::uint8_t>(c));
    }
    const std::vector<Vec3> positions = {{0, 0, 0}, {0, 0, 2}, {0, 0, 1000}};
    visual::OpticalSettings settings;
    settings.texture = 0;
    auto splats = visual::make_splats(t, &s, positions, 0, {0, 0, 10}, {1, 0, 0}, {0, 1, 0}, {0, 0, -1}, settings);
    REQUIRE(splats.size() == 6);
    CHECK(splats[0].luminosity > 0);
    CHECK(splats[0].optical_depth == 0);
    CHECK(splats[1].optical_depth > 0);
    CHECK(splats[1].luminosity == 0);
    CHECK(splats[0].major < 0.2); // A distant halo cannot set the stellar smoothing scale.
    settings.texture = 0.2;
    const auto textured =
        visual::make_splats(t, &s, positions, 0, {0, 0, 10}, {1, 0, 0}, {0, 1, 0}, {0, 0, -1}, settings);
    double total = 0;
    for (auto const &item : textured)
        total += item.luminosity;
    CHECK(total == doctest::Approx(splats[0].luminosity));
}

TEST_CASE("young populations fade and optical metadata survives recording") {
    CHECK(visual::young_fraction(3, 4, 6) == 0);
    CHECK(visual::young_fraction(4, 4, 6) == 1);
    CHECK(visual::young_fraction(10, 4, 6) == doctest::Approx(std::exp(-1.0)));
    Trajectory t;
    BodyRecord b;
    b.mass = 0.007;
    b.name = "disk.gas.0";
    t.bodies.push_back(b);
    Sample s;
    s.position = {{1, 2, 3}};
    s.velocity = {{0, 1, 0}};
    s.component = {5};
    s.birth_time = {2.5};
    s.gas_density = {0.25};
    s.internal_energy = {0.04};
    t.samples.push_back(s);
    const auto path = std::filesystem::temp_directory_path() / "physics_optical_metadata.traj";
    write_trajectory(t, path.string());
    const auto loaded = read_trajectory(path.string());
    REQUIRE(loaded.samples.size() == 1);
    CHECK(loaded.bodies[0].mass == b.mass);
    CHECK(loaded.samples[0].birth_time == s.birth_time);
    CHECK(loaded.samples[0].gas_density == s.gas_density);
    CHECK(loaded.samples[0].internal_energy == s.internal_energy);
}

TEST_CASE("spin and node control stellar angular momentum independently of density") {
    const std::string base =
        "integrator velocity_verlet\ndt 0.02\nsteps 1\ngravitational_constant 1\nforce gravity\nsoftening 0.03\ngalaxy "
        "test\nparticles 100\nhalo_mass 2\nhalo_scale 2\ndisk_mass 1\ndisk_scale_radius 1\ndisk_height 0.2\n";
    const auto positive = load_scene(base + "spin 1\n");
    const auto negative = load_scene(base + "spin -1\nnode 90\n");
    double lz_positive = 0, lz_negative = 0;
    for (std::size_t i = 0; i < positive.bodies.size(); ++i) {
        const auto &a = positive.bodies[i].particle;
        const auto &b = negative.bodies[i].particle;
        CHECK(norm(a.position) == doctest::Approx(norm(b.position)));
        if (a.component == Component::Disk) {
            lz_positive += cross(a.position, a.velocity).z;
            lz_negative += cross(b.position, b.velocity).z;
        }
    }
    CHECK(lz_positive > 0);
    CHECK(lz_negative == doctest::Approx(-lz_positive));
    CHECK_THROWS((void)load_scene(base + "spin 0\n"));
}

TEST_CASE("stellar birth times survive resumed integration") {
    BodyDesc body;
    body.name = "test.gas.0";
    body.particle.mass = 0.01;
    body.particle.component = Component::YoungStar;
    State state;
    state.add(body.particle);
    state.birth_time[0] = 0.2;
    state.radiated_energy = 0.3;
    state.feedback_energy = 0.5;
    state.floor_energy_added = 0.01;
    state.formation_energy_removed = 0.04;
    SimulationConfig config;
    config.dt = 0.1;
    config.steps = 12;
    config.sample_every = 1;
    const auto result = simulate_interruptible(state, {body}, config, 10, nullptr, nullptr);
    REQUIRE(result.trajectory.samples.size() == 2);
    CHECK(result.state.birth_time[0] == 0.2);
    CHECK(result.state.radiated_energy == 0.3);
    CHECK(result.state.feedback_energy == 0.5);
    CHECK(result.state.floor_energy_added == 0.01);
    CHECK(result.state.formation_energy_removed == 0.04);
    CHECK(result.trajectory.samples.back().birth_time[0] == 0.2);
    CHECK(result.trajectory.samples.back().time == doctest::Approx(1.2));
    CHECK(result.trajectory.bodies[0].mass == 0.01);
}

TEST_CASE("radial profiles recover a rotating tilted ring independent of bulk motion") {
    std::vector<BodyDesc> bodies(64);
    Sample sample;
    for (int i = 0; i < 64; ++i) {
        const double angle = 2 * 3.141592653589793 * i / 64;
        bodies[i].particle.component = Component::Disk;
        bodies[i].particle.mass = 1.0 / 64;
        sample.position.push_back(
            {3 + 2 * std::cos(angle), 4 + 2 * std::sin(angle) * 0.6, 5 + 2 * std::sin(angle) * 0.8});
        sample.velocity.push_back({0.1 - std::sin(angle), 0.2 + std::cos(angle) * 0.6, 0.3 + std::cos(angle) * 0.8});
    }
    const auto bins = disk_profiles(sample, bodies, {0, 1, 3});
    REQUIRE(bins.size() == 2);
    CHECK(bins[0].count == 0);
    CHECK(bins[1].count == 64);
    CHECK(bins[1].rotation == doctest::Approx(1));
    CHECK(bins[1].height50 < 1e-12);
    CHECK(bins[1].sigma_r < 1e-7);
    CHECK(bins[1].sigma_z < 1e-7);
    CHECK(bins[1].surface_density == doctest::Approx(1 / (8 * 3.141592653589793)));
    CHECK_THROWS(disk_profiles(sample, bodies, {0, 3, 1}));
}

TEST_CASE("gas lane structure conserves optical depth and follows gas density") {
    Trajectory t;
    Sample sample;
    std::vector<Vec3> positions;
    for (int i = 0; i < 20; ++i) {
        BodyRecord b;
        b.name = "lane.gas." + std::to_string(i);
        b.mass = 0.01;
        t.bodies.push_back(b);
        positions.push_back({0.1 * i, 0.02 * (i % 2), 0});
        sample.component.push_back(static_cast<std::uint8_t>(Component::Gas));
        sample.gas_density.push_back(1);
        sample.internal_energy.push_back(0.002);
    }
    t.samples.push_back(sample);
    double baseline = 0;
    for (double strength : {0.0, 0.55, 1.0}) {
        visual::OpticalSettings settings;
        settings.dust_structure = strength;
        const auto splats =
            visual::make_splats(t, &sample, positions, 0, {0, 0, 10}, {1, 0, 0}, {0, 1, 0}, {0, 0, -1}, settings);
        double tau = 0;
        for (const auto &s : splats) {
            tau += s.optical_depth;
            CHECK(s.minor > 0);
        }
        if (strength == 0)
            baseline = tau;
        CHECK(tau == doctest::Approx(baseline));
        CHECK(tau == doctest::Approx(20 * 0.01 * 12 / (1 + 0.002 / 0.04)));
    }
}

TEST_CASE("stellar camera excludes distant dark matter and aligns to disk angular momentum") {
    Trajectory t;
    Sample sample;
    for (int i = 0; i < 64; ++i) {
        const double a = 2 * 3.141592653589793 * i / 64;
        BodyRecord body;
        body.name = "first.disk." + std::to_string(i);
        body.mass = 1;
        t.bodies.push_back(body);
        sample.component.push_back(3);
        sample.position.push_back({2 * std::cos(a), 1.2 * std::sin(a), 1.6 * std::sin(a)});
        sample.velocity.push_back({-std::sin(a), .6 * std::cos(a), .8 * std::cos(a)});
    }
    BodyRecord halo;
    halo.name = "first.halo.0";
    halo.mass = 100;
    t.bodies.push_back(halo);
    sample.component.push_back(1);
    sample.position.push_back({1000, 0, 0});
    sample.velocity.push_back({0, 0, 0});
    const auto f = visual::stellar_frame(t, sample, true);
    REQUIRE(f.valid);
    CHECK(norm(f.center) < .02);
    CHECK(f.radius < 4);
    CHECK(dot(f.normal, {0, -.8, .6}) == doctest::Approx(1));
}

TEST_CASE("encounter shots select the first passage rather than a later coalescence") {
    Trajectory t;
    for (auto name : {"left.bulge.0", "right.bulge.0"}) {
        BodyRecord b;
        b.name = name;
        b.mass = 1;
        t.bodies.push_back(b);
    }
    for (int i = 0; i <= 40; ++i) {
        const double d = i <= 10 ? 12 - i : i <= 20 ? 2 + (i - 10) * .5 : 7 - (i - 20) * .3;
        Sample s;
        s.time = i;
        s.position = {{-d * .5, 0, 0}, {d * .5, 0, 0}};
        s.component = {2, 2};
        t.samples.push_back(s);
    }
    CHECK(visual::first_passage_time(t) >= 9);
    CHECK(visual::first_passage_time(t) <= 12);
    t.bodies.resize(1);
    for (auto &s : t.samples) {
        s.position.resize(1);
        s.component.resize(1);
    }
    CHECK(visual::first_passage_time(t) == 0);
}


TEST_CASE("arm definition narrows real elongated light without changing luminosity") {
    Trajectory t; Sample sample; std::vector<Vec3> positions;
    for(int i=0;i<64;++i) {
        BodyRecord b; b.name="arm.disk."+std::to_string(i); b.mass=.01; b.emission=1;
        t.bodies.push_back(b); positions.push_back({.06*i,.008*std::sin(i),0});
        sample.component.push_back(static_cast<std::uint8_t>(Component::Disk));
    }
    double minor[2]{}, light[2]{};
    for(int k=0;k<2;++k) {
        visual::OpticalSettings settings; settings.detail=1; settings.texture=0; settings.arm_definition=k;
        const auto splats=visual::make_splats(t,&sample,positions,0,{0,0,10},{1,0,0},{0,1,0},{0,0,-1},settings);
        for(const auto &s:splats) { light[k]+=s.luminosity; minor[k]+=s.minor*s.luminosity; }
    }
    CHECK(light[0]==doctest::Approx(.64)); CHECK(light[1]==doctest::Approx(.64));
    CHECK(minor[1]<minor[0]*.9);
}
