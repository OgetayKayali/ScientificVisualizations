#include "forces/hydro.hpp"
#include "simulation/scene.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace physics;

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: physics_audit output.csv\n";
        return 1;
    }
    try {
        std::ofstream out(argv[1]);
        out.exceptions(std::ios::badbit | std::ios::failbit);
        out << "mode,dt,relative_energy_residual,momentum_change,formed_stars\n" << std::setprecision(12);
        for (const std::string mode : {"gravity", "adiabatic", "cooling", "feedback"}) {
            for (double dt : {0.02, 0.01, 0.005, 0.0025}) {
                State state;
                std::vector<BodyDesc> bodies;
                for (int i = 0; i < 64; ++i) {
                    Particle p;
                    p.mass = 0.01;
                    p.position = {0.12 * ((i % 4) - 1.5), 0.12 * (((i / 4) % 4) - 1.5), 0.12 * ((i / 16) - 1.5)};
                    p.velocity = p.position * -0.2 + cross({0, 0, 0.4}, p.position);
                    p.component = mode == "gravity" ? Component::Disk : Component::Gas;
                    p.internal_energy = mode == "gravity" ? 0 : (mode == "feedback" ? 0.00225 : 0.05);
                    state.add(p);
                    BodyDesc b;
                    b.particle = p;
                    bodies.push_back(b);
                }
                SimulationConfig config;
                config.dt = dt;
                config.steps = static_cast<std::size_t>(std::lround(2 / dt));
                config.sample_every = config.steps;
                auto &f = config.forces;
                f.gravity = mode == "gravity";
                f.gravitational_constant = 1;
                f.softening_length = 0.08;
                f.gas_sound_speed = 0.05;
                f.gas_cooling_time = mode == "cooling" || mode == "feedback" ? 0.2 : 0;
                f.gas_star_time = mode == "feedback" ? 0.15 : 0;
                f.gas_feedback_energy = mode == "feedback" ? 0.2 : 0;
                f.gas_feedback_time = 0.1;
                prepare_gas(state, f);
                if (mode == "feedback")
                    f.gas_resolved_star_density = f.gas_reference_density;
                const auto initial = diagnose(state, f);
                auto result = simulate_interruptible(state, bodies, config, 0, nullptr, nullptr);
                const auto &end = result.state;
                const auto final = diagnose(end, f);
                const double error = (final.energy - initial.energy + end.radiated_energy +
                                      end.formation_energy_removed - end.feedback_energy - end.floor_energy_added) /
                                     std::abs(initial.energy);
                out << mode << ',' << dt << ',' << error << ',' << norm(final.linear_momentum - initial.linear_momentum)
                    << ','
                    << std::count(end.component.begin(), end.component.end(),
                                  static_cast<std::uint8_t>(Component::YoungStar))
                    << '\n';
                std::cout << mode << " dt=" << dt << " residual=" << error << '\n';
            }
        }
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
