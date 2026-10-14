#include "forces/forces.hpp"
#include "simulation/galaxy.hpp"
#include "simulation/scene.hpp"
#include "simulation/validation.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

double median_sorted(std::vector<double> values) {
    REQUIRE(!values.empty());
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

physics::EquilibriumComponents example_model() {
    physics::EquilibriumComponents model;
    model.gravitational_constant = 1;
    model.halo_mass = 2;
    model.halo_scale = 2;
    model.halo_cutoff = 16;
    model.disk_mass = 1;
    model.disk_scale_radius = 1;
    model.disk_height = 0.2;
    model.dispersion_ratio = 2;
    return model;
}

} // namespace

TEST_CASE("the exponential disk rotation curve is the radial derivative of its potential") {
    constexpr double gravitational_constant = 1.0;
    constexpr double disk_mass = 1.0;
    constexpr double scale = 1.0;
    const double radius = 2.0 * scale;
    const double speed_squared =
        physics::exponential_disk_circular_speed_squared(gravitational_constant, disk_mass, scale, radius);
    const double step = 1e-4 * radius;
    const double force =
        (physics::exponential_disk_potential(gravitational_constant, disk_mass, scale, radius + step) -
         physics::exponential_disk_potential(gravitational_constant, disk_mass, scale, radius - step)) /
        (2.0 * step);
    CHECK(speed_squared == doctest::Approx(radius * force).epsilon(1e-3));

    const double far = 60.0 * scale;
    const double potential = physics::exponential_disk_potential(gravitational_constant, disk_mass, scale, far);
    CHECK(-potential * far / (gravitational_constant * disk_mass) == doctest::Approx(1.0).epsilon(0.02));
    const double far_speed =
        physics::exponential_disk_circular_speed_squared(gravitational_constant, disk_mass, scale, far);
    CHECK(far_speed * far / (gravitational_constant * disk_mass) == doctest::Approx(1.0).epsilon(0.05));
}

TEST_CASE("Hernquist dispersion matches an independent Jeans integral") {
    constexpr double gravitational_constant = 1.0;
    constexpr double mass = 1.0;
    constexpr double scale = 1.0;
    constexpr double pi = 3.14159265358979323846;
    auto integrand = [&](double radius) {
        const double density = mass * scale / (2.0 * pi * radius * std::pow(radius + scale, 3));
        const double enclosed = mass * std::pow(radius / (radius + scale), 2);
        return density * gravitational_constant * enclosed / (radius * radius);
    };

    for (double radius : {0.3, 1.0, 4.0}) {
        double integral = 0.0;
        double outer = 1.0e5 * scale;
        double previous = integrand(outer);
        for (int step = 0; step < 400000 && outer > radius; ++step) {
            double inner = std::max(radius, outer * 0.9999);
            const double value = integrand(inner);
            integral += 0.5 * (previous + value) * (outer - inner);
            outer = inner;
            previous = value;
        }
        const double density = mass * scale / (2.0 * pi * radius * std::pow(radius + scale, 3));
        const double reference = integral / density;
        CHECK(physics::hernquist_isotropic_dispersion_squared(gravitational_constant, mass, scale, radius) ==
              doctest::Approx(reference).epsilon(0.02));
    }

    physics::EquilibriumComponents pure;
    pure.gravitational_constant = 1;
    pure.halo_mass = 1;
    pure.halo_scale = 1;
    pure.halo_cutoff = 80;
    pure.disk_mass = 1e-4;
    pure.disk_scale_radius = 1;
    pure.disk_height = 0.1;
    pure.dispersion_ratio = 2;
    CHECK(physics::compound_halo_dispersion_squared(pure, 1.0) ==
          doctest::Approx(physics::hernquist_isotropic_dispersion_squared(1, 1, 1, 1)).epsilon(0.08));
}

TEST_CASE("disk streaming stays below circular speed and the example disk is Toomre-stable") {
    const physics::EquilibriumComponents model = example_model();
    const double radius = model.disk_scale_radius;
    const double circular = physics::compound_circular_speed_squared(model, radius);
    const double streaming = physics::compound_streaming_speed(model, radius);
    CHECK(circular > 0.0);
    CHECK(streaming > 0.0);
    CHECK(streaming * streaming < circular);
    CHECK(physics::compound_toomre_q(model, radius) > 1.2);
    CHECK(physics::compound_toomre_q(model, 2.0 * radius) > 1.0);
}

TEST_CASE("a realized galaxy has the disk and halo profiles and repeats from the seed") {
    physics::GalaxySpec spec;
    spec.name = "left";
    spec.particles = 1800;
    spec.halo_mass = 2;
    spec.halo_scale = 2;
    spec.halo_cutoff = 16;
    spec.disk_mass = 1;
    spec.disk_scale_radius = 1;
    spec.disk_height = 0.2;
    spec.dispersion_ratio = 2;
    spec.center = {3, -1, 2};
    spec.velocity = {0.05, 0, 0};
    spec.seed = 42;

    const std::vector<physics::BodyDesc> bodies = physics::realize_galaxy(spec, 1.0, 0.03);
    REQUIRE(bodies.size() == 1800);

    double mass = 0.0;
    physics::Vec3 center_of_mass{};
    std::vector<double> disk_radius;
    std::vector<double> disk_height;
    std::size_t disks = 0;
    for (physics::BodyDesc const &body : bodies) {
        mass += body.particle.mass;
        center_of_mass += body.particle.mass * body.particle.position;
        if (body.name.find(".disk.") != std::string::npos) {
            ++disks;
            const physics::Vec3 relative = body.particle.position - spec.center;
            disk_radius.push_back(std::sqrt(relative.x * relative.x + relative.y * relative.y));
            disk_height.push_back(std::abs(relative.z));
            CHECK(body.emission == doctest::Approx(spec.emission));
        }
    }
    center_of_mass = center_of_mass / mass;
    CHECK(mass == doctest::Approx(3.0));
    CHECK(disks == 600);
    CHECK(median_sorted(disk_radius) == doctest::Approx(1.678).epsilon(0.12));
    CHECK(median_sorted(disk_height) == doctest::Approx(0.2 * std::atanh(0.5)).epsilon(0.2));
    CHECK(center_of_mass.x == doctest::Approx(3.0).epsilon(0.15));
    CHECK(center_of_mass.y == doctest::Approx(-1.0).epsilon(0.25));
    CHECK(center_of_mass.z == doctest::Approx(2.0).epsilon(0.25));

    const physics::Vec3 again = physics::realize_galaxy(spec, 1.0, 0.03)[10].particle.position;
    CHECK(again.x == doctest::Approx(bodies[10].particle.position.x));
    CHECK(again.y == doctest::Approx(bodies[10].particle.position.y));
    CHECK(again.z == doctest::Approx(bodies[10].particle.position.z));
}

TEST_CASE("disk streaming follows the softened circular speed") {
    physics::GalaxySpec spec;
    spec.name = "spin";
    spec.particles = 1800;
    spec.halo_mass = 2;
    spec.halo_scale = 2;
    spec.halo_cutoff = 16;
    spec.disk_mass = 1;
    spec.disk_scale_radius = 1;
    spec.disk_height = 0.2;
    spec.dispersion_ratio = 2;
    spec.seed = 7;
    const std::vector<physics::BodyDesc> bodies = physics::realize_galaxy(spec, 1.0, 0.03);

    physics::State state;
    for (physics::BodyDesc const &body : bodies) {
        state.add(body.particle);
    }
    physics::ForceSet forces;
    forces.gravity = true;
    forces.gravitational_constant = 1;
    forces.softening_length = 0.03;
    physics::Accelerations accelerations;
    physics::compute_accelerations(state, accelerations, forces);

    double mean_streaming = 0;
    double mean_circular = 0;
    int count = 0;
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        if (bodies[index].name.find(".disk.") == std::string::npos) {
            continue;
        }
        const physics::Vec3 point = bodies[index].particle.position;
        const double radius = std::sqrt(point.x * point.x + point.y * point.y);
        if (radius < 0.8 || radius > 1.4 || std::abs(point.z) > spec.disk_height) {
            continue;
        }
        const double radial = (accelerations.ax[index] * point.x + accelerations.ay[index] * point.y) / radius;
        const double circular = std::sqrt(std::max(0.0, -radius * radial));
        const double phi = std::atan2(point.y, point.x);
        const physics::Vec3 velocity = bodies[index].particle.velocity;
        const double streaming = -velocity.x * std::sin(phi) + velocity.y * std::cos(phi);
        mean_streaming += streaming;
        mean_circular += circular;
        ++count;
    }
    REQUIRE(count >= 40);
    mean_streaming /= static_cast<double>(count);
    mean_circular /= static_cast<double>(count);
    CHECK(mean_streaming > 0.4 * mean_circular);
    CHECK(mean_streaming < 1.05 * mean_circular);
}

TEST_CASE("galaxy parameters rebuild the particles and leave hand-written bodies") {
    const char *text = R"(integrator velocity_verlet
dt 0.05
steps 2
gravitational_constant 1
seed 3
force gravity
softening 0.05
body anchor
mass 2
position 1 2 3
velocity 0 0 0
galaxy clump
particles 20
halo_mass 1
halo_scale 1
disk_mass 1
disk_scale_radius 1
disk_height 0.2
center 0 0 0
velocity 0 0 0
)";
    physics::Scene scene = physics::load_scene(text);
    REQUIRE(scene.authored_bodies == 1);
    REQUIRE(scene.galaxies.size() == 1);
    REQUIRE(scene.bodies.size() == 21);
    CHECK(scene.bodies.front().name == "anchor");

    scene.galaxies.front().particles = 40;
    scene.galaxies.front().disk_height = 0.3;
    physics::apply_galaxies(scene);
    REQUIRE(scene.bodies.size() == 41);
    CHECK(scene.bodies.front().name == "anchor");
    CHECK(scene.bodies.front().particle.mass == doctest::Approx(2.0));
    CHECK(scene.bodies.front().particle.position.x == doctest::Approx(1.0));

    const physics::Vec3 placed = scene.bodies[5].particle.position;
    physics::apply_galaxies(scene);
    CHECK(scene.bodies[5].particle.position.x == doctest::Approx(placed.x));
    CHECK(scene.bodies[5].particle.position.y == doctest::Approx(placed.y));
    CHECK(scene.bodies[5].particle.position.z == doctest::Approx(placed.z));
}

TEST_CASE("an isolated galaxy keeps its disk for more than a rotation period") {
    const char *text = R"(integrator velocity_verlet
dt 0.02
steps 700
sample_every 100
gravitational_constant 1
seed 11
force gravity
softening 0.03
galaxy disk
particles 1800
halo_mass 2
halo_scale 2
halo_cutoff 16
disk_mass 1
disk_scale_radius 1
disk_height 0.2
dispersion_ratio 2
center 0 0 0
velocity 0 0 0
)";
    const physics::Scene scene = physics::load_scene(text);
    REQUIRE(scene.bodies.size() == 1800);
    CHECK(scene.bodies.front().name == "disk.halo.0");

    std::vector<double> initial_radius;
    std::vector<double> initial_height;
    for (physics::BodyDesc const &body : scene.bodies) {
        if (body.name.find(".disk.") == std::string::npos) {
            continue;
        }
        initial_radius.push_back(std::sqrt(body.particle.position.x * body.particle.position.x +
                                           body.particle.position.y * body.particle.position.y));
        initial_height.push_back(std::abs(body.particle.position.z));
    }
    const double starting_radius = median_sorted(initial_radius);
    const double starting_height = median_sorted(initial_height);

    const physics::Trajectory trajectory = physics::run_scene(scene);
    const physics::Vec3 start_momentum = trajectory.samples.front().diagnostics.linear_momentum;
    const physics::Vec3 end_momentum = trajectory.samples.back().diagnostics.linear_momentum;
    CHECK(physics::norm(end_momentum - start_momentum) < 1e-6);
    const double energy_change =
        std::abs(trajectory.samples.back().diagnostics.energy - trajectory.samples.front().diagnostics.energy);
    CHECK(energy_change < 0.05 * std::abs(trajectory.samples.front().diagnostics.energy));

    std::vector<double> final_radius;
    std::vector<double> final_height;
    const std::vector<physics::Vec3> &positions = trajectory.samples.back().position;
    for (std::size_t index = 0; index < scene.bodies.size(); ++index) {
        if (scene.bodies[index].name.find(".disk.") == std::string::npos) {
            continue;
        }
        const physics::Vec3 position = positions[index];
        final_radius.push_back(std::sqrt(position.x * position.x + position.y * position.y));
        final_height.push_back(std::abs(position.z));
    }
    const double radius_ratio = median_sorted(final_radius) / starting_radius;
    const double height_ratio = median_sorted(final_height) / starting_height;
    CHECK(radius_ratio > 0.7);
    CHECK(radius_ratio < 1.45);
    CHECK(height_ratio > 0.5);
    CHECK(height_ratio < 2.0);

    CHECK_THROWS_AS(
        [&] {
            physics::Scene rejected = physics::load_scene("integrator velocity_verlet\n"
                                                          "dt 0.02\n"
                                                          "steps 4\n"
                                                          "gravitational_constant 1\n"
                                                          "force gravity\n"
                                                          "galaxy bare\n"
                                                          "particles 8\n"
                                                          "mass 1\n"
                                                          "scale_radius 0.5\n");
            (void)rejected;
        }(),
        std::runtime_error);
}

TEST_CASE("disk speeds follow the Shu function's epicycle anisotropy") {
    physics::GalaxySpec spec;
    spec.name = "shu";
    spec.particles = 1800;
    spec.halo_mass = 2;
    spec.halo_scale = 2;
    spec.halo_cutoff = 16;
    spec.disk_mass = 1;
    spec.disk_scale_radius = 1;
    spec.disk_height = 0.2;
    spec.dispersion_ratio = 2;
    spec.seed = 9;
    const std::vector<physics::BodyDesc> bodies = physics::realize_galaxy(spec, 1.0, 0.03);

    double mean_phi = 0.0;
    int count = 0;
    for (physics::BodyDesc const &body : bodies) {
        if (body.name.find(".disk.") == std::string::npos) {
            continue;
        }
        const physics::Vec3 point = body.particle.position;
        const double radius = std::sqrt(point.x * point.x + point.y * point.y);
        if (radius < 0.8 || radius > 1.4 || std::abs(point.z) > spec.disk_height) {
            continue;
        }
        const double phi = std::atan2(point.y, point.x);
        const double azimuth = -body.particle.velocity.x * std::sin(phi) + body.particle.velocity.y * std::cos(phi);
        mean_phi += azimuth;
        ++count;
    }
    REQUIRE(count >= 40);
    mean_phi /= static_cast<double>(count);
    double radial_variance = 0.0;
    double azimuth_variance = 0.0;
    int retrograde = 0;
    for (physics::BodyDesc const &body : bodies) {
        if (body.name.find(".disk.") == std::string::npos) {
            continue;
        }
        const physics::Vec3 point = body.particle.position;
        const double radius = std::sqrt(point.x * point.x + point.y * point.y);
        if (radius < 0.8 || radius > 1.4 || std::abs(point.z) > spec.disk_height) {
            continue;
        }
        const double phi = std::atan2(point.y, point.x);
        const double radial = body.particle.velocity.x * std::cos(phi) + body.particle.velocity.y * std::sin(phi);
        const double azimuth = -body.particle.velocity.x * std::sin(phi) + body.particle.velocity.y * std::cos(phi);
        radial_variance += radial * radial;
        azimuth_variance += (azimuth - mean_phi) * (azimuth - mean_phi);
        if (azimuth < 0.0) {
            ++retrograde;
        }
        CHECK(body.particle.component == physics::Component::Disk);
    }
    radial_variance /= static_cast<double>(count);
    azimuth_variance /= static_cast<double>(count);
    CHECK(azimuth_variance < radial_variance);
    CHECK(azimuth_variance > 0.15 * radial_variance);
    CHECK(retrograde < count / 20);
}

TEST_CASE("a bulge is a compact Hernquist component") {
    physics::GalaxySpec spec;
    spec.name = "core";
    spec.particles = 600;
    spec.halo_mass = 2;
    spec.halo_scale = 2;
    spec.halo_cutoff = 16;
    spec.bulge_mass = 0.3;
    spec.bulge_scale = 0.2;
    spec.bulge_cutoff = 1.6;
    spec.disk_mass = 1;
    spec.disk_scale_radius = 1;
    spec.disk_height = 0.2;
    spec.seed = 4;
    const std::vector<physics::BodyDesc> bodies = physics::realize_galaxy(spec, 1.0, 0.03);
    std::vector<double> radius;
    double mass = 0.0;
    for (physics::BodyDesc const &body : bodies) {
        mass += body.particle.mass;
        if (body.particle.component != physics::Component::Bulge) {
            continue;
        }
        CHECK(body.name.find(".bulge.") != std::string::npos);
        const double r = physics::norm(body.particle.position);
        CHECK(r < 1.6);
        radius.push_back(r);
    }
    CHECK(mass == doctest::Approx(3.3));
    REQUIRE(radius.size() > 20);
    const double median = median_sorted(radius);
    CHECK(median > 0.1);
    CHECK(median < 0.9);
}

TEST_CASE("vertical Jeans quadrature recovers an isothermal sech squared sheet") {
    std::vector<double> z, force;
    constexpr double h = 0.2, variance = 0.04;
    for (int i = 0; i <= 800; ++i) {
        const double height = 8 * h * i / 800;
        z.push_back(height);
        force.push_back(2 * variance / h * std::tanh(height / h));
    }
    const auto result = physics::vertical_jeans_variance(h, z, force);
    for (std::size_t i = 0; i < result.size(); ++i)
        CHECK(result[i] == doctest::Approx(variance).epsilon(0.0001));
    CHECK_THROWS(physics::vertical_jeans_variance(0, z, force));
    CHECK_THROWS(physics::vertical_jeans_variance(h, {0, 0}, {0, 1}));
}

TEST_CASE("stellar population colors preserve dynamics and emitted luminance") {
    physics::GalaxySpec spec;
    spec.name = "pop";
    spec.particles = 120;
    spec.halo_mass = 2;
    spec.halo_scale = 2;
    spec.disk_mass = 1;
    spec.disk_scale_radius = 1;
    spec.disk_height = 0.2;
    const auto plain = physics::realize_galaxy(spec, 1, 0.03);
    spec.population_variation = 0.8;
    const auto varied = physics::realize_galaxy(spec, 1, 0.03);
    int changed = 0;
    for (std::size_t i = 0; i < plain.size(); ++i) {
        CHECK(plain[i].particle.position == varied[i].particle.position);
        CHECK(plain[i].particle.velocity == varied[i].particle.velocity);
        CHECK(plain[i].emission == varied[i].emission);
        CHECK(physics::dot(plain[i].color, {0.2126, 0.7152, 0.0722}) ==
              doctest::Approx(physics::dot(varied[i].color, {0.2126, 0.7152, 0.0722})));
        if (!(plain[i].color == varied[i].color))
            ++changed;
    }
    CHECK(changed > 20);
    spec.population_variation = 2;
    CHECK_THROWS((void)physics::realize_galaxy(spec, 1, 0.03));
}

TEST_CASE("height dependent vertical support produces finite repeatable inclined galaxies") {
    physics::GalaxySpec spec;
    spec.name = "jeans";
    spec.particles = 120;
    spec.halo_mass = 2;
    spec.halo_scale = 2;
    spec.disk_mass = 1;
    spec.disk_scale_radius = 1;
    spec.disk_height = 0.2;
    spec.jeans_vertical = true;
    spec.inclination_degrees = 37;
    const auto a = physics::realize_galaxy(spec, 1, 0.03), b = physics::realize_galaxy(spec, 1, 0.03);
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(std::isfinite(physics::norm(a[i].particle.velocity)));
        CHECK(a[i].particle.velocity == b[i].particle.velocity);
    }
}


TEST_CASE("spiral CDF produces the requested modes and reverses winding with spin") {
    physics::GalaxySpec spec;
    spec.disk_scale_radius = 1;
    spec.spiral_strength = 0.6;
    constexpr double pi = 3.141592653589793;
    for (int arms : {2, 4}) {
        spec.spiral_arms = arms;
        double c = 0, s = 0;
        const double r = 2;
        const double phase = -std::log(r) / std::tan(spec.spiral_pitch_degrees * pi / 180);
        for (int i = 0; i < 4096; ++i) {
            const double u = 2*pi*(i+0.5)/4096;
            const double phi = physics::spiral_azimuth(u, r, spec);
            CHECK(phi >= 0); CHECK(phi <= 2*pi);
            c += std::cos(arms * (phi - phase)); s += std::sin(arms * (phi - phase));
        }
        const double expected = .3 * (1-std::exp(-r*r/.36)) * std::exp(-std::pow(r/7,4));
        CHECK(c/4096 == doctest::Approx(expected).epsilon(1e-5));
        CHECK(std::abs(s/4096) < 1e-6);
        const double positive = physics::spiral_azimuth(1.2, r, spec);
        spec.spin = -1;
        const double negative = physics::spiral_azimuth(2*pi-1.2, r, spec);
        CHECK(negative == doctest::Approx(2*pi-positive));
        spec.spin = 1;
    }
    spec.spiral_strength = 0;
    CHECK(physics::spiral_azimuth(1.234, 2, spec) == 1.234);
}

TEST_CASE("disk allocation preserves component masses and target Q controls radial motion") {
    physics::GalaxySpec spec;
    spec.name="spiral"; spec.particles=600; spec.halo_mass=2; spec.halo_scale=2;
    spec.disk_mass=1; spec.disk_scale_radius=1; spec.disk_height=.12;
    spec.bulge_mass=.3; spec.disk_particle_fraction=.6; spec.target_q=1.3;
    const auto cool=physics::realize_galaxy(spec,1,.1);
    std::size_t count=0; double disk=0, halo=0, bulge=0, radial_cool=0, radial_hot=0;
    spec.target_q=2.2;
    const auto hot=physics::realize_galaxy(spec,1,.1);
    for (std::size_t i=0;i<cool.size();++i) {
        const auto &p=cool[i].particle;
        CHECK(p.position == hot[i].particle.position);
        CHECK(std::isfinite(physics::norm(hot[i].particle.velocity)));
        if(p.component==physics::Component::Halo) halo+=p.mass;
        if(p.component==physics::Component::Bulge) bulge+=p.mass;
        if(p.component==physics::Component::Disk) {
            ++count; disk+=p.mass;
            const double r=std::hypot(p.position.x,p.position.y);
            if(r>.7 && r<3) {
                auto radial=physics::Vec3{p.position.x/r,p.position.y/r,0};
                radial_cool+=std::pow(physics::dot(p.velocity,radial),2);
                radial_hot+=std::pow(physics::dot(hot[i].particle.velocity,radial),2);
            }
        }
    }
    CHECK(count==360); CHECK(disk==doctest::Approx(1));
    CHECK(halo==doctest::Approx(2)); CHECK(bulge==doctest::Approx(.3));
    CHECK(radial_hot>radial_cool*1.4);
    spec.spiral_strength=.9;
    CHECK_THROWS((void)physics::realize_galaxy(spec,1,.1));
    spec.spiral_strength=.5; spec.spiral_pitch_degrees=0;
    CHECK_THROWS((void)physics::realize_galaxy(spec,1,.1));
}

TEST_CASE("radial mode diagnostics distinguish an axisymmetric ring and two arms") {
    std::vector<physics::BodyDesc> bodies(256);
    physics::Sample sample;
    for (int i=0;i<256;++i) {
        const double phi=2*3.141592653589793*i/256;
        bodies[i].particle.component=physics::Component::Disk;
        bodies[i].particle.mass=(1+.6*std::cos(2*phi))/256;
        sample.position.push_back({2*std::cos(phi),2*std::sin(phi),0});
        sample.velocity.push_back({-std::sin(phi),std::cos(phi),0});
    }
    auto bins=physics::disk_profiles(sample,bodies,{0,1,3});
    CHECK(bins[1].a2==doctest::Approx(.3)); CHECK(bins[1].a4<1e-10);
    CHECK(std::abs(bins[1].phase2)<1e-10);
    CHECK(std::isnan(bins[1].q));
    for(auto &b:bodies) b.particle.mass=1./256;
    bins=physics::disk_profiles(sample,bodies,{0,1,3});
    CHECK(bins[1].a2<1e-10); CHECK(bins[1].a4<1e-10);
    CHECK(bins[1].noise==doctest::Approx(1./16));
}

TEST_CASE("Q diagnostic uses the gravitational circular curve rather than streaming speed") {
    std::vector<physics::BodyDesc> bodies(129);
    physics::Sample sample;
    bodies[0].particle.component=physics::Component::Bulge; bodies[0].particle.mass=1;
    sample.position.push_back({}); sample.velocity.push_back({});
    for(int i=0;i<128;++i) {
        const double phi=2*3.141592653589793*i/128;
        bodies[i+1].particle.component=physics::Component::Disk;
        bodies[i+1].particle.mass=1e-8;
        const physics::Vec3 r{std::cos(phi),std::sin(phi),0};
        sample.position.push_back(r*2);
        sample.velocity.push_back(physics::Vec3{-r.y,r.x,0}*.4+r*(.1*std::cos(4*phi)));
    }
    physics::ForceSet force; force.gravity=true; force.gravitational_constant=1; force.softening_length=.1;
    const auto b=physics::disk_profiles(sample,bodies,{1.5,2.5},&force)[0];
    CHECK(b.circular_speed==doctest::Approx(std::sqrt(4/std::pow(4.01,1.5))).epsilon(1e-4));
    CHECK(b.kappa==doctest::Approx(std::sqrt(4.04/std::pow(4.01,2.5))).epsilon(.02));
    CHECK(b.q==doctest::Approx(b.sigma_r*b.kappa/(3.36*b.surface_density)));
    CHECK(b.circular_speed>b.rotation*1.5);
}


TEST_CASE("spiral seeding preserves each radial profile and adds no mass or height") {
    physics::GalaxySpec spec;
    spec.name="profile"; spec.particles=240; spec.halo_mass=2; spec.halo_scale=2;
    spec.disk_mass=1; spec.disk_scale_radius=1; spec.disk_height=.12;
    spec.gas_mass=.15; spec.gas_particles=64;
    const auto plain=physics::realize_galaxy(spec,1,.1);
    spec.spiral_strength=.55;
    const auto seeded=physics::realize_galaxy(spec,1,.1);
    REQUIRE(plain.size()==seeded.size());
    int disk_changes=0, gas_changes=0;
    for(std::size_t i=0;i<plain.size();++i) {
        const auto &a=plain[i].particle, &b=seeded[i].particle;
        CHECK(a.mass==b.mass); CHECK(a.position.z==b.position.z);
        CHECK(physics::norm(a.position)==doctest::Approx(physics::norm(b.position)));
        if(!(a.position==b.position)) {
            if(a.component==physics::Component::Disk) ++disk_changes;
            if(a.component==physics::Component::Gas) ++gas_changes;
        }
    }
    CHECK(disk_changes>50); CHECK(gas_changes==64);
}
