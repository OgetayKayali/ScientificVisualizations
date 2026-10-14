#include "io/gltf.hpp"
#include "io/trajectory.hpp"
#include "simulation/scene.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
    std::cerr << "usage: physics_run scene.scene [--trajectory file.traj] [--gltf file.gltf]\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 1;
    }
    std::string trajectory_path;
    std::string gltf_path;
    for (int index = 2; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--trajectory" && index + 1 < argc) {
            trajectory_path = argv[++index];
        } else if (argument == "--gltf" && index + 1 < argc) {
            gltf_path = argv[++index];
        } else {
            usage();
            return 1;
        }
    }

    try {
        const physics::Scene scene = physics::load_scene_file(argv[1]);
        const physics::Trajectory trajectory = physics::run_scene(scene);
        const std::filesystem::path scene_path(argv[1]);
        const std::string stem = scene_path.stem().string();
        if (trajectory_path.empty() && gltf_path.empty()) {
            trajectory_path = stem + ".traj";
            gltf_path = stem + ".gltf";
        }
        if (!trajectory_path.empty()) {
            physics::write_trajectory(trajectory, trajectory_path);
            std::cout << "wrote " << trajectory_path << " (" << trajectory.samples.size() << " samples)\n";
        }
        if (!gltf_path.empty() && !trajectory.bodies.empty()) {
            physics::write_gltf(trajectory, gltf_path);
            std::cout << "wrote " << gltf_path << "\n";
        }
    } catch (std::exception const& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
