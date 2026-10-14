#include "io/trajectory.hpp"
#include "visualization/tracers.hpp"
#include <filesystem>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    try {
        if (argc < 4)
            throw std::invalid_argument("usage: physics_trace source.scene source.traj output.traj [--per-star 4] "
                                        "[--dt 0.02] [--softening 0.08] [--theta 0.4] [--restyle]");
        if ((std::filesystem::exists(argv[3]) && std::filesystem::equivalent(argv[2],argv[3])) ||
            std::filesystem::absolute(argv[2]).lexically_normal() ==
            std::filesystem::absolute(argv[3]).lexically_normal())
            throw std::invalid_argument("output must differ from the source recording");
        auto scene = physics::load_scene_file(argv[1]);
        if (!scene.config.forces.gravity)
            throw std::invalid_argument("tracers require a gravity-only external force model");
        physics::visual::TracerOptions options;
        options.dt = scene.config.dt;
        options.softening = 2 * scene.config.forces.softening_length;
        bool restyle = false;
        for (int i = 4; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--restyle") {
                restyle = true;
                continue;
            }
            if (i + 1 >= argc)
                throw std::invalid_argument("missing option value");
            const std::string value = argv[++i];
            std::size_t used = 0;
            const double number = std::stod(value, &used);
            if (used != value.size() || !std::isfinite(number))
                throw std::invalid_argument("invalid numeric option");
            if (arg == "--per-star") {
                if (number < 2 || number > 32 || std::floor(number) != number)
                    throw std::invalid_argument("invalid per-star count");
                options.per_star = static_cast<std::size_t>(number);
            } else if (arg == "--dt")
                options.dt = number;
            else if (arg == "--softening")
                options.softening = number;
            else if (arg == "--theta")
                options.theta = number;
            else
                throw std::invalid_argument("unknown option: " + arg);
        }
        auto source = physics::read_trajectory(argv[2]);
        if (source.samples.empty() || scene.bodies.size() != source.bodies.size())
            throw std::invalid_argument("scene and recording do not match");
        for (std::size_t i = 0; i < scene.bodies.size(); ++i) {
            if (scene.bodies[i].name != source.bodies[i].name ||
                std::abs(scene.bodies[i].particle.mass - source.bodies[i].mass) > 1e-12 ||
                physics::norm(scene.bodies[i].particle.position - source.samples[0].position[i]) > 1e-8 ||
                physics::norm(scene.bodies[i].particle.velocity - source.samples[0].velocity[i]) > 1e-8)
                throw std::invalid_argument("recording must start from the supplied scene's initial conditions");
            if (restyle) {
                const auto &style=scene.bodies[i];
                auto &body=source.bodies[i];
                body.color=style.color; body.emission=style.emission;
                body.formed_star_color=style.formed_star_color;
                body.formed_star_emission=style.formed_star_emission;
                body.initial_young_fraction=style.initial_young_fraction;
                body.initial_young_age=style.initial_young_age;
                body.display_radius=style.display_radius; body.roughness=style.roughness;
            }
        }
        auto result = physics::visual::refine_starlight(
            source, scene.config.forces.gravitational_constant, options, [](std::size_t frame, std::size_t total) {
                if (frame % 100 == 0 || frame == total)
                    std::cout << "Tracers: " << frame << '/' << total << " snapshots" << std::endl;
            });
        physics::write_trajectory(result, argv[3]);
        std::cout << "Wrote " << result.bodies.size() - source.bodies.size() << " massless display tracers to "
                  << argv[3] << '\n';
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
