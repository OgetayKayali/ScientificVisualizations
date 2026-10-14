#include "io/gltf.hpp"
#include "io/trajectory.hpp"
#include "simulation/scene.hpp"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef PHYSICS_SOURCE_DIR
#error "PHYSICS_SOURCE_DIR must be set"
#endif

TEST_CASE("merger scenes load reproducibly and removed simulation modes are rejected") {
    auto scene = physics::load_scene_file(std::string(PHYSICS_SOURCE_DIR) + "/examples/galaxy_luminous_merger.scene");
    REQUIRE(scene.galaxies.size() == 2);
    CHECK(scene.config.forces.gravity);
    scene.config.steps = 2;
    scene.config.sample_every = 1;
    const auto trajectory = physics::run_scene(scene);
    CHECK(trajectory.samples.size() == 3);
    CHECK(trajectory.samples.back().time == doctest::Approx(2 * scene.config.dt));
    for (auto mode : {"rk4", "boris"})
        CHECK_THROWS((void)physics::load_scene(std::string("integrator ") + mode + "\ndt .01\nsteps 1\n"));
    for (auto text : {"flow\n", "force harmonic\n", "force uniform_field\n"})
        CHECK_THROWS((void)physics::load_scene(std::string("integrator velocity_verlet\ndt .01\nsteps 1\n") + text));
}

TEST_CASE("trajectory files round-trip and glTF keeps one animated node per body") {
    const char* text = R"(integrator velocity_verlet
dt 0.05
steps 20
sample_every 10
gravitational_constant 1
body sun
mass 1
display_radius 0.2
color 1 0.8 0.2
position 0 0 0
velocity 0 0 0
body planet
mass 0.01
display_radius 0.05
color 0.2 0.4 1
position 0 1 0
velocity -1 0 0
force gravity
)";
    const physics::Trajectory trajectory = physics::run_scene(physics::load_scene(text));
    const auto path = std::filesystem::temp_directory_path() / "physics_engine_roundtrip.traj";
    physics::write_trajectory(trajectory, path.string());
    const physics::Trajectory loaded = physics::read_trajectory(path.string());
    REQUIRE(loaded.bodies.size() == trajectory.bodies.size());
    REQUIRE(loaded.samples.size() == trajectory.samples.size());
    CHECK(loaded.bodies[1].name == "planet");
    CHECK(loaded.bodies[1].display_radius == doctest::Approx(0.05));
    CHECK(loaded.samples.back().position[1].y == doctest::Approx(trajectory.samples.back().position[1].y));
    CHECK(loaded.samples.back().diagnostics.energy ==
          doctest::Approx(trajectory.samples.back().diagnostics.energy));

    const physics::GltfExport gltf = physics::build_gltf(trajectory);
    CHECK(gltf.body_count == 2);
    CHECK(gltf.animated_bodies == 2);
    CHECK(gltf.json.find("\"animations\"") != std::string::npos);
    CHECK(gltf.json.find("\"path\": \"translation\"") != std::string::npos);
    CHECK(gltf.json.find("PhysicsEngine") != std::string::npos);
    REQUIRE(gltf.translations[1].size() >= 3);
    // Physics (0, 1, 0) maps to glTF Y-up (x, z, -y) = (0, 0, -1).
    CHECK(gltf.translations[1][0] == doctest::Approx(0.0).scale(1.0).epsilon(1e-6));
    CHECK(gltf.translations[1][1] == doctest::Approx(0.0).scale(1.0).epsilon(1e-6));
    CHECK(gltf.translations[1][2] == doctest::Approx(-1.0).epsilon(1e-5));

    const auto gltf_path = std::filesystem::temp_directory_path() / "physics_engine_roundtrip.gltf";
    physics::write_gltf(trajectory, gltf_path.string());
    CHECK(std::filesystem::file_size(gltf_path) > 1000);
}

TEST_CASE("a run can stop between steps and continue from that state") {
    physics::State state;
    std::vector<physics::BodyDesc> bodies;
    for (int index = 0; index < 128; ++index) {
        physics::BodyDesc body;
        body.name = "p" + std::to_string(index);
        body.particle.mass = 0.01;
        body.particle.position = {static_cast<double>(index % 8), static_cast<double>((index / 8) % 8),
                                  static_cast<double>(index / 64)};
        state.add(body.particle);
        bodies.push_back(std::move(body));
    }
    physics::SimulationConfig config;
    config.dt = 0.01;
    config.steps = 8000;
    config.sample_every = 20;
    config.forces.gravity = true;
    config.forces.gravitational_constant = 0.001;
    config.forces.softening_length = 0.2;

    std::atomic<bool> cancel{true};
    physics::SimulationOutcome held =
        physics::simulate_interruptible(state, bodies, config, 0, &cancel, nullptr);
    CHECK(held.stopped);
    CHECK(held.steps_completed == 0);
    CHECK(held.trajectory.samples.size() == 1);

    std::atomic<bool> later{false};
    std::atomic<std::size_t> step{0};
    auto worker = std::async(std::launch::async, [&] {
        return physics::simulate_interruptible(state, bodies, config, 0, &later, &step);
    });
    while (step.load(std::memory_order_relaxed) < 40) {
        if (worker.wait_for(std::chrono::milliseconds(2)) == std::future_status::ready) {
            break;
        }
    }
    later.store(true, std::memory_order_relaxed);
    physics::SimulationOutcome partial = worker.get();
    REQUIRE(partial.stopped);
    CHECK(partial.steps_completed >= 1);
    CHECK(partial.steps_completed < config.steps);
    const double stopped_at = partial.trajectory.samples.back().time;

    physics::SimulationOutcome rest = physics::simulate_interruptible(std::move(partial.state), bodies, config,
                                                                      partial.steps_completed, nullptr, nullptr);
    CHECK(!rest.stopped);
    CHECK(rest.steps_completed == config.steps);
    CHECK(rest.trajectory.samples.back().time == doctest::Approx(config.steps * config.dt));
    CHECK(rest.trajectory.samples.front().time > stopped_at);
}


TEST_CASE("HDR stellar populations export legal glTF material factors without losing emission") {
    physics::Trajectory trajectory;
    physics::BodyRecord body;
    body.name = "bright.disk.0";
    body.color = {2.0, 1.0, 0.5};
    body.emission = 3.0;
    trajectory.bodies.push_back(body);
    physics::Sample sample;
    sample.position = {{0, 0, 0}};
    sample.velocity = {{0, 0, 0}};
    trajectory.samples.push_back(sample);
    const auto result = physics::build_gltf(trajectory);
    CHECK(result.json.find("\"extensionsUsed\": [\"KHR_materials_emissive_strength\"]") != std::string::npos);
    CHECK(result.json.find("\"baseColorFactor\": [1, 0.5, 0.25, 1]") != std::string::npos);
    CHECK(result.json.find("\"emissiveFactor\": [1, 0.5, 0.25]") != std::string::npos);
    CHECK(result.json.find("\"emissiveStrength\": 6") != std::string::npos);
}
