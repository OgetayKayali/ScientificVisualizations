#include "core/component.hpp"
#include "visualization/optical.hpp"
#include "forces/hydro.hpp"
#include "frame_export.hpp"
#include "optical_shaders.hpp"
#include "gl_loader.hpp"
#include "io/sphere.hpp"
#include "io/trajectory.hpp"
#include "simulation/scene.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <atomic>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <future>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

Mat4 multiply(Mat4 const& a, Mat4 const& b) {
    Mat4 out{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[column * 4 + k];
            }
            out.m[column * 4 + row] = sum;
        }
    }
    return out;
}

Mat4 perspective(double fovy_radians, double aspect, double near_plane, double far_plane) {
    Mat4 out{};
    const float tan_half = static_cast<float>(std::tan(fovy_radians * 0.5));
    out.m[0] = static_cast<float>(1.0 / (aspect * tan_half));
    out.m[5] = 1.0f / tan_half;
    out.m[10] = static_cast<float>(-(far_plane + near_plane) / (far_plane - near_plane));
    out.m[11] = -1.0f;
    out.m[14] = static_cast<float>(-(2.0 * far_plane * near_plane) / (far_plane - near_plane));
    out.m[15] = 0.0f;
    return out;
}

Mat4 look_at(physics::Vec3 eye, physics::Vec3 target, physics::Vec3 up) {
    const physics::Vec3 forward = physics::normalized(target - eye);
    const physics::Vec3 side = physics::normalized(physics::cross(forward, up));
    const physics::Vec3 lifted = physics::cross(side, forward);
    Mat4 out{};
    out.m[0] = static_cast<float>(side.x);
    out.m[4] = static_cast<float>(side.y);
    out.m[8] = static_cast<float>(side.z);
    out.m[1] = static_cast<float>(lifted.x);
    out.m[5] = static_cast<float>(lifted.y);
    out.m[9] = static_cast<float>(lifted.z);
    out.m[2] = static_cast<float>(-forward.x);
    out.m[6] = static_cast<float>(-forward.y);
    out.m[10] = static_cast<float>(-forward.z);
    out.m[12] = static_cast<float>(-physics::dot(side, eye));
    out.m[13] = static_cast<float>(-physics::dot(lifted, eye));
    out.m[14] = static_cast<float>(physics::dot(forward, eye));
    return out;
}

Mat4 translation_scale(physics::Vec3 translation, double scale) {
    Mat4 out{};
    const float s = static_cast<float>(scale);
    out.m[0] = s;
    out.m[5] = s;
    out.m[10] = s;
    out.m[12] = static_cast<float>(translation.x);
    out.m[13] = static_cast<float>(translation.y);
    out.m[14] = static_cast<float>(translation.z);
    return out;
}

struct Camera {
    physics::Vec3 target{};
    double distance = 4;
    double yaw = 1.1;
    double pitch = 0.8;

    [[nodiscard]] physics::Vec3 eye() const {
        const double cos_pitch = std::cos(pitch);
        return target + distance * physics::Vec3{cos_pitch * std::cos(yaw), cos_pitch * std::sin(yaw), std::sin(pitch)};
    }

    [[nodiscard]] physics::Vec3 right() const {
        return physics::normalized(physics::cross(physics::normalized(target - eye()), physics::Vec3{0, 0, 1}));
    }

    [[nodiscard]] physics::Vec3 up() const {
        return physics::normalized(physics::cross(right(), physics::normalized(target - eye())));
    }
};

struct RunControl {
    std::atomic<bool> cancel{false};
    std::atomic<std::size_t> step{0};
};

struct ColorGroup {
    std::string label;
    std::vector<std::size_t> indices;
    float rgb[3] = {0.75f, 0.78f, 0.82f};
};

struct ViewState {
    physics::Trajectory trajectory;
    Camera camera{};
    double playhead = 0;
    double speed = 1;
    float trail_fraction = 1.0f;
    float key_intensity = 0.85f;
    float fill_intensity = 0.28f;
    float point_scale = 1.0f;
    // Display only. Zero smoothing keeps the sharp points. Dust and gas glow
    // use the gas particles already in the recording.
    float light_smoothing = 3.0f;
    float soften = 10.0f;
    float soften_brightness = 1.0f;
    float dust = 1.25f;
    float dust_structure = 0.55f;
    float reddening = 1.0f;
    float gas_glow = 1.0f;
    // Display only: density reconstruction of the merger particles.
    float field_gain = 1.6f;
    float field_contrast = 0.35f;
    float field_heat = 0.55f;
    // Display only. 0 is one soft cloud. 3 keeps the disks and streams.
    int field_resolution = 2;
    // 0: legacy particles; 1: illustrative volume; 2: optical stellar light and dust.
    int picture_mode = 2;
    float optical_smoothing = 1.0f;
    float exposure = 8.0f;
    float bloom = 0.15f;
    float young_lifetime = 12.0f;
    float ionizing_lifetime = 0.8f, oiii_lifetime = 0.5f;
    float young_boost = 2, association_size = 0.4f;
    float nebula_radius = 0.65f, ionizing_mass_scale = 0.00002f, nebula_density_scale = 0.01f;
    float halpha_strength = 24, oiii_strength = 6, nebula_size = 0.6f;
    float halpha_color[3] = {1, 0.08f, 0.22f}, oiii_color[3] = {0.08f, 0.85f, 0.75f};
    char look_path[512] = "galaxy.look";
    float stellar_texture = 0.04f;
    float stellar_detail = 0.45f;
    float arm_definition = 0.65f;
    bool show_halo = false;
    bool show_hot_gas = false;
    float ui_scale = 1.75f;
    float ui_scale_applied = 0.0f;
    ImGuiStyle base_style{};
    bool show_axes = false;
    bool playing = true;
    bool middle_down = false;
    bool shift_down = false;
    double last_x = 0;
    double last_y = 0;
    double scene_radius = 1;
    double time_begin = 0;
    double time_end = 0;
    bool editable = false;
    int preset_index = -1;
    int quality_preset = -1;
    int window_resolution = -1;
    std::chrono::steady_clock::time_point run_started{};
    std::size_t run_step_origin = 0;
    std::string simulation_name;
    bool busy = false;
    bool resumable = false;
    bool resuming = false;
    std::size_t steps_done = 0;
    physics::State checkpoint;
    physics::Scene scene;
    std::shared_ptr<RunControl> control;
    std::future<physics::SimulationOutcome> pending;
    std::vector<ColorGroup> color_groups;
    std::string status;
    std::string error;
    unsigned recording_epoch = 0;
    int export_width = 1920;
    int export_height = 1080;
    int export_fps = 30;
    int export_format = 0;
    char export_path[512] = "render";
    double export_start = 0;
    double export_end = 0;
    bool export_range_set = false;
    bool export_active = false;
    bool export_stop = false;
    bool export_resumable = false;
    bool export_still = false;
    int export_frame = 0;
    int export_frame_count = 0;
    int export_run_width = 1920;
    int export_run_height = 1080;
    int export_run_fps = 30;
    int export_run_format = 0;
    double export_run_start = 0;
    std::string export_run_path;
    std::string export_status;
    std::string export_error;
    FfmpegPipe export_ffmpeg;
};

GLuint compile_shader(GLenum type, char const* source) {
    const GLuint shader = gl_create_shader(type);
    gl_shader_source(shader, 1, &source, nullptr);
    gl_compile_shader(shader);
    GLint status = 0;
    gl_get_shader_iv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char log[1024] = {};
        gl_get_shader_info_log(shader, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        throw std::runtime_error(log);
    }
    return shader;
}

GLuint link_program(char const* vertex, char const* fragment) {
    const GLuint program = gl_create_program();
    const GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex);
    const GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment);
    gl_attach_shader(program, vertex_shader);
    gl_attach_shader(program, fragment_shader);
    gl_link_program(program);
    gl_delete_shader(vertex_shader);
    gl_delete_shader(fragment_shader);
    GLint status = 0;
    gl_get_program_iv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char log[1024] = {};
        gl_get_program_info_log(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        throw std::runtime_error(log);
    }
    return program;
}

void frame_camera(ViewState& view) {

    if (view.trajectory.samples.empty() || view.trajectory.samples.front().position.empty()) {
        return;
    }
    if (view.picture_mode == 2) {
        std::vector<physics::Vec3> light;
        for (auto const &sample : view.trajectory.samples) {
            for (std::size_t i = 0; i < sample.position.size(); ++i) {
                const auto component = physics::visual::component_of(view.trajectory.bodies[i], &sample, i);
                if (component == physics::Component::Disk || component == physics::Component::Bulge ||
                    component == physics::Component::YoungStar)
                    light.push_back(sample.position[i]);
            }
        }
        if (!light.empty()) {
            physics::Vec3 lower, upper;
            auto quantile = [&](int axis, double fraction) {
                std::vector<double> values;
                values.reserve(light.size());
                for (auto point : light)
                    values.push_back(axis == 0 ? point.x : axis == 1 ? point.y : point.z);
                const auto index = static_cast<std::size_t>((values.size() - 1) * fraction);
                std::nth_element(values.begin(), values.begin() + index, values.end());
                return values[index];
            };
            lower = {quantile(0, 0.01), quantile(1, 0.01), quantile(2, 0.01)};
            upper = {quantile(0, 0.99), quantile(1, 0.99), quantile(2, 0.99)};
            view.camera.target = (lower + upper) * 0.5;
            view.scene_radius = std::max(1.0, physics::norm(upper - lower) * 0.5);
            view.camera.distance = view.scene_radius / std::tan(22.5 * kPi / 180.0) * 1.12;
            return;
        }
    }
    physics::Vec3 center{};
    for (physics::Vec3 position : view.trajectory.samples.front().position) {
        center += position;
    }
    center = center / static_cast<double>(view.trajectory.samples.front().position.size());

    double max_radius = 0.0;
    std::size_t total = 0;
    for (physics::Sample const& sample : view.trajectory.samples) {
        for (physics::Vec3 position : sample.position) {
            max_radius = std::max(max_radius, physics::norm(position - center));
            ++total;
        }
    }
    if (!(max_radius > 0.0) || total == 0) {
        return;
    }
    // A few ejected particles must not set the zoom. Frame the inner 95% when the
    // farthest point is many times farther than that bulk.
    constexpr int kBins = 256;
    std::vector<std::size_t> counts(static_cast<std::size_t>(kBins), 0);
    for (physics::Sample const& sample : view.trajectory.samples) {
        for (physics::Vec3 position : sample.position) {
            const double unit = physics::norm(position - center) / max_radius;
            const int bin = std::min(kBins - 1, static_cast<int>(unit * static_cast<double>(kBins)));
            ++counts[static_cast<std::size_t>(std::max(bin, 0))];
        }
    }
    const std::size_t keep = std::max<std::size_t>(1, (total * 95) / 100);
    std::size_t running = 0;
    int chosen = kBins - 1;
    for (int bin = 0; bin < kBins; ++bin) {
        running += counts[static_cast<std::size_t>(bin)];
        if (running >= keep) {
            chosen = bin;
            break;
        }
    }
    double limit = max_radius * (static_cast<double>(chosen) + 1.0) / static_cast<double>(kBins);
    if (!(max_radius > limit * 4.0)) {
        limit = max_radius;
    }

    bool any = false;
    physics::Vec3 min_corner{};
    physics::Vec3 max_corner{};
    for (physics::Sample const& sample : view.trajectory.samples) {
        for (physics::Vec3 position : sample.position) {
            if (physics::norm(position - center) > limit) {
                continue;
            }
            if (!any) {
                min_corner = max_corner = position;
                any = true;
            } else {
                min_corner.x = std::min(min_corner.x, position.x);
                min_corner.y = std::min(min_corner.y, position.y);
                min_corner.z = std::min(min_corner.z, position.z);
                max_corner.x = std::max(max_corner.x, position.x);
                max_corner.y = std::max(max_corner.y, position.y);
                max_corner.z = std::max(max_corner.z, position.z);
            }
        }
    }
    if (!any) {
        return;
    }
    view.camera.target = (min_corner + max_corner) * 0.5;
    view.scene_radius = std::max(0.05, 0.5 * physics::norm(max_corner - min_corner));
    const double fovy = 45.0 * kPi / 180.0;
    view.camera.distance = view.scene_radius / std::tan(fovy * 0.5) * 1.35;
}

std::string color_group_label(std::string const& name) {
    const std::size_t dot = name.rfind('.');
    if (dot == std::string::npos || dot + 1 >= name.size()) {
        return name;
    }
    for (std::size_t index = dot + 1; index < name.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(name[index]))) {
            return name;
        }
    }
    return name.substr(0, dot);
}


// Set a fixed composition from the current recorded state; playback does not chase noisy particles.
void compose_stellar_view(ViewState &view, std::string const &shot) {
    if (view.trajectory.samples.empty()) return;
    if (shot=="passage" || shot=="tails") {
        view.playhead=physics::visual::first_passage_time(view.trajectory);
        if (shot=="tails") view.playhead=std::min(view.time_end,view.playhead+0.18*(view.time_end-view.time_begin));
        view.camera.yaw=1.1; view.camera.pitch=0.8;
    }
    const auto &frames=view.trajectory.samples;
    auto it=std::lower_bound(frames.begin(),frames.end(),view.playhead,
                            [](physics::Sample const &s,double t){return s.time<t;});
    if (it==frames.end()) it=frames.end()-1;
    const bool oriented=shot=="face" || shot=="edge";
    const auto frame=physics::visual::stellar_frame(view.trajectory,*it,oriented);
    if (!frame.valid) return;
    view.camera.target=frame.center; view.scene_radius=frame.radius;
    view.camera.distance=frame.radius/std::tan(22.5*kPi/180)*1.12;
    if (oriented) {
        auto direction=frame.normal;
        if (shot=="edge") {
            direction=physics::cross(frame.normal,{0,0,1});
            if (physics::norm(direction)<1e-6) direction={1,0,0};
            direction=physics::normalized(direction);
        }
        view.camera.yaw=std::atan2(direction.y,direction.x);
        view.camera.pitch=std::asin(std::clamp(direction.z,-0.99999999,0.99999999));
    }
}

void rebuild_color_groups(ViewState& view) {
    view.color_groups.clear();
    for (std::size_t index = 0; index < view.trajectory.bodies.size(); ++index) {
        const std::string label = color_group_label(view.trajectory.bodies[index].name);
        auto found = std::find_if(view.color_groups.begin(), view.color_groups.end(),
                                  [&](ColorGroup const& group) { return group.label == label; });
        if (found == view.color_groups.end()) {
            ColorGroup group;
            group.label = label;
            const physics::Vec3 color = view.trajectory.bodies[index].color;
            group.rgb[0] = static_cast<float>(color.x);
            group.rgb[1] = static_cast<float>(color.y);
            group.rgb[2] = static_cast<float>(color.z);
            group.indices.push_back(index);
            view.color_groups.push_back(std::move(group));
        } else {
            found->indices.push_back(index);
        }
    }
    for (std::size_t index = 0; index < view.trajectory.bodies.size(); ++index) {
        const physics::BodyRecord& body = view.trajectory.bodies[index];
        if (!(body.formed_star_emission > 0.0)) {
            continue;
        }
        const std::string source = color_group_label(body.name);
        const std::size_t dot = source.rfind('.');
        const std::string owner = dot == std::string::npos ? source : source.substr(0, dot);
        const std::string label = owner + ".young";
        auto found = std::find_if(view.color_groups.begin(), view.color_groups.end(),
                                  [&](ColorGroup const& group) { return group.label == label; });
        if (found == view.color_groups.end()) {
            ColorGroup group;
            group.label = label;
            group.rgb[0] = static_cast<float>(body.formed_star_color.x);
            group.rgb[1] = static_cast<float>(body.formed_star_color.y);
            group.rgb[2] = static_cast<float>(body.formed_star_color.z);
            group.indices.push_back(index);
            view.color_groups.push_back(std::move(group));
        } else {
            found->indices.push_back(index);
        }
    }
}

void paint_group(ViewState& view, ColorGroup const& group) {
    const physics::Vec3 color{group.rgb[0], group.rgb[1], group.rgb[2]};
    const std::size_t dot = group.label.rfind('.');
    const std::string kind = dot == std::string::npos ? group.label : group.label.substr(dot + 1);
    if (kind == "young") {
        for (std::size_t index : group.indices) {
            view.trajectory.bodies[index].formed_star_color = color;
        }
    } else {
        for (std::size_t index : group.indices) {
            view.trajectory.bodies[index].color = color;
        }
    }
    const std::string owner = dot == std::string::npos ? std::string{} : group.label.substr(0, dot);
    for (physics::GalaxySpec& galaxy : view.scene.galaxies) {
        if (galaxy.name != owner) {
            continue;
        }
        if (kind == "disk") {
            galaxy.color = color;
        } else if (kind == "halo") {
            galaxy.halo_color = color;
            galaxy.halo_color_set = true;
        } else if (kind == "bulge") {
            galaxy.bulge_color = color;
            galaxy.bulge_color_set = true;
        } else if (kind == "gas") {
            galaxy.gas_color = color;
            galaxy.gas_color_set = true;
        } else if (kind == "hot") {
            galaxy.hot_color = color;
            galaxy.hot_color_set = true;
        } else if (kind == "young") {
            galaxy.young_star_color = color;
            galaxy.young_color_set = true;
        }
    }
    for (std::size_t index = 0; index < view.scene.authored_bodies && index < view.scene.bodies.size(); ++index) {
        if (view.scene.bodies[index].name == group.label) {
            view.scene.bodies[index].color = color;
        }
    }
}

void apply_scene_colors(ViewState& view) {
    if (!view.editable) {
        return;
    }
    for (physics::BodyRecord& body : view.trajectory.bodies) {
        const std::string label = color_group_label(body.name);
        const std::size_t dot = label.rfind('.');
        const std::string owner = dot == std::string::npos ? std::string{} : label.substr(0, dot);
        const std::string kind = dot == std::string::npos ? label : label.substr(dot + 1);
        for (physics::GalaxySpec const& galaxy : view.scene.galaxies) {
            if (galaxy.name != owner) {
                continue;
            }
            if (kind == "disk") {
                body.color = galaxy.color;
                body.formed_star_color = galaxy.young_star_color;
                body.formed_star_emission = galaxy.young_star_emission;
            } else if (kind == "halo") {
                body.color = galaxy.halo_color;
            } else if (kind == "bulge") {
                body.color = galaxy.bulge_color;
            } else if (kind == "gas") {
                body.color = galaxy.gas_color;
                body.formed_star_color = galaxy.young_star_color;
                body.formed_star_emission = galaxy.young_star_emission;
            } else if (kind == "hot") {
                body.color = galaxy.hot_color;
            }
        }
        for (std::size_t index = 0; index < view.scene.authored_bodies && index < view.scene.bodies.size(); ++index) {
            if (view.scene.bodies[index].name == label) {
                body.color = view.scene.bodies[index].color;
            }
        }
    }
}

void adopt_trajectory(ViewState& view, physics::Trajectory trajectory) {
    ++view.recording_epoch;
    view.trajectory = std::move(trajectory);
    view.time_begin = view.trajectory.samples.front().time;
    view.time_end = view.trajectory.samples.back().time;
    view.playhead = view.time_begin;
    view.speed = std::max((view.time_end - view.time_begin) / 12.0, 1e-6);
    view.playing = true;
    frame_camera(view);
    rebuild_color_groups(view);
    if (!view.export_range_set && !view.trajectory.samples.empty()) {
        view.export_start = view.trajectory.samples.front().time;
        view.export_end = view.trajectory.samples.back().time;
    }
}

void take_outcome(ViewState& view, physics::SimulationOutcome outcome) {
    if (!view.resuming || view.trajectory.samples.empty()) {
        view.trajectory = std::move(outcome.trajectory);
        if (!view.trajectory.samples.empty()) {
            view.time_begin = view.trajectory.samples.front().time;
            view.time_end = view.trajectory.samples.back().time;
            view.playhead = view.time_begin;
            view.speed = std::max((view.time_end - view.time_begin) / 12.0, 1e-6);
            view.playing = true;
            frame_camera(view);
        }
    } else {
        view.trajectory.stellar_cohorts = std::move(outcome.trajectory.stellar_cohorts);
        for (physics::Sample& sample : outcome.trajectory.samples) {
            if (sample.time > view.trajectory.samples.back().time) {
                view.trajectory.samples.push_back(std::move(sample));
            }
        }
        if (!view.trajectory.samples.empty()) {
            view.time_end = view.trajectory.samples.back().time;
        }
    }
    ++view.recording_epoch;
    apply_scene_colors(view);
    rebuild_color_groups(view);
    view.checkpoint = std::move(outcome.state);
    view.steps_done = outcome.steps_completed;
    view.resumable = outcome.stopped;
    if (outcome.stopped) {
        view.status = "Stopped at step " + std::to_string(outcome.steps_completed) + " of " +
                      std::to_string(view.scene.config.steps);
    } else {
        view.status.clear();
    }
    if (!view.export_range_set && !view.trajectory.samples.empty()) {
        view.export_start = view.trajectory.samples.front().time;
        view.export_end = view.trajectory.samples.back().time;
    }
}

std::string simulation_eta(ViewState const& view, std::size_t step);

void launch_simulation(ViewState& view, bool resume) {
    if (view.busy) {
        return;
    }
    if (resume && !view.resumable) {
        return;
    }
    if (!resume) {
        try {
            physics::apply_galaxies(view.scene);
            view.error.clear();
        } catch (std::exception const& error) {
            view.error = error.what();
            return;
        }
        view.steps_done = 0;
        view.resumable = false;
    }

    view.control = std::make_shared<RunControl>();
    view.control->step.store(view.steps_done);
    view.busy = true;
    view.resuming = resume;
    view.run_started = std::chrono::steady_clock::now();
    view.run_step_origin = view.steps_done;
    view.status = "Simulating step " + std::to_string(view.steps_done) + " of " + std::to_string(view.scene.config.steps) +
                  ". Estimating time…";
    if (!resume) {
        view.scene.config.forces.gas_reference_density = 0.0;
        view.scene.config.forces.gas_resolved_star_density = 0.0;
    }
    physics::State start = resume ? view.checkpoint : view.scene.make_state();
    if (!(view.scene.config.forces.gas_reference_density > 0.0)) {
        physics::prepare_gas(start, view.scene.config.forces);
    }
    const std::size_t step_offset = view.steps_done;
    const std::vector<physics::BodyDesc> bodies = view.scene.bodies;
    const physics::SimulationConfig config = view.scene.config;
    const std::shared_ptr<RunControl> control = view.control;
    view.pending = std::async(std::launch::async, [start = std::move(start), bodies, config, step_offset, control]() {
        return physics::simulate_interruptible(start, bodies, config, step_offset, &control->cancel, &control->step);
    });
}

void poll_simulation(ViewState& view) {
    if (!view.busy || !view.pending.valid()) {
        return;
    }
    if (view.control) {
        const std::size_t step = view.control->step.load(std::memory_order_relaxed);
        if (view.control->cancel.load(std::memory_order_relaxed)) {
            view.status = "Stopping at step " + std::to_string(step);
        } else {
            view.status = "Simulating step " + std::to_string(step) + " of " + std::to_string(view.scene.config.steps) +
                          ". " + simulation_eta(view, step);
        }
    }
    if (view.pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return;
    }
    try {
        take_outcome(view, view.pending.get());
    } catch (std::exception const& error) {
        view.error = error.what();
        view.resumable = false;
    }
    view.busy = false;
}

bool edit_double(char const* label, double& value, float speed, double minimum, double maximum) {
    return ImGui::DragScalar(label, ImGuiDataType_Double, &value, speed, &minimum, &maximum, "%.5g");
}

bool edit_vec3(char const* label, physics::Vec3& value, float speed) {
    double components[3] = {value.x, value.y, value.z};
    if (ImGui::DragScalarN(label, ImGuiDataType_Double, components, 3, speed, nullptr, nullptr, "%.4g")) {
        value = {components[0], components[1], components[2]};
        return true;
    }
    return false;
}

void place_panel(float x, float y, float width, float height) {
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
}

struct QualityPreset {
    char const* name;
    char const* detail;
    std::size_t particles;
    std::size_t gas_particles;
    std::size_t hot_particles;
};

QualityPreset const kQualityPresets[] = {
    {"Minimal", "Fewer particles. Geometry, softening and time step stay fixed.", 400, 64, 48},
    {"Optimal", "Desktop particle budget. Geometry and numerical settings stay fixed.", 2400, 400, 64},
    {"Fine", "Twice the desktop particle count. Allow about four times the integration cost.", 4800, 800, 128},
    {"Ultra", "Many more particles. Geometry and numerical settings stay fixed.", 8000, 1200, 400},
};
constexpr int kQualityCount = static_cast<int>(sizeof(kQualityPresets) / sizeof(kQualityPresets[0]));

void apply_quality_preset(ViewState& view, int index) {
    if (index < 0 || index >= kQualityCount || view.scene.galaxies.empty()) {
        return;
    }
    QualityPreset const& preset = kQualityPresets[index];
    for (physics::GalaxySpec& galaxy : view.scene.galaxies) {
        galaxy.particles = preset.particles;
        if (galaxy.gas_mass > 0.0) {
            galaxy.gas_particles = preset.gas_particles;
        }
        if (galaxy.hot_gas_mass > 0.0) {
            galaxy.hot_gas_particles = preset.hot_particles;
        }
    }
    view.quality_preset = index;
    view.resumable = false;
    view.status = std::string(preset.name) + " resolution is set. Run starts from the initial conditions.";
}

std::string format_remaining(double seconds) {
    if (!(seconds > 0.0) || !std::isfinite(seconds)) {
        return "Finishing…";
    }
    const int rounded = static_cast<int>(std::lround(seconds));
    if (rounded < 90) {
        return "about " + std::to_string(std::max(rounded, 1)) + " s left";
    }
    const int minutes = std::max(1, static_cast<int>(std::lround(seconds / 60.0)));
    if (minutes < 90) {
        return "about " + std::to_string(minutes) + " min left";
    }
    const int hours = minutes / 60;
    const int rest = minutes % 60;
    if (rest == 0) {
        return "about " + std::to_string(hours) + " h left";
    }
    return "about " + std::to_string(hours) + " h " + std::to_string(rest) + " min left";
}

std::string simulation_eta(ViewState const& view, std::size_t step) {
    const std::size_t total = view.scene.config.steps;
    if (step >= total) {
        return "Finishing…";
    }
    const std::size_t done = step >= view.run_step_origin ? step - view.run_step_origin : 0;
    if (done < 2) {
        return "Estimating time…";
    }
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - view.run_started).count();
    if (!(elapsed > 0.0)) {
        return "Estimating time…";
    }
    const double remaining = elapsed * static_cast<double>(total - step) / static_cast<double>(done);
    return format_remaining(remaining);
}

void draw_parameters(ViewState& view) {
    if (!view.editable) {
        ImGui::TextUnformatted("This is a saved recording. Choose a simulation to set parameters and run a new one.");
        return;
    }

        ImGui::TextUnformatted("Gravity is a direct pair sum. Gas adds pressure, cooling, and star formation.");
    
    const bool locked = view.busy;
    if (locked) {
        ImGui::BeginDisabled();
    }

    if (!view.scene.galaxies.empty()) {
        ImGui::TextUnformatted("Resolution");
        for (int index = 0; index < kQualityCount; ++index) {
            if (index > 0) {
                ImGui::SameLine();
            }
            if (ImGui::RadioButton(kQualityPresets[index].name, view.quality_preset == index)) {
                apply_quality_preset(view, index);
            }
        }
        const char* detail = view.quality_preset >= 0 && view.quality_preset < kQualityCount
                                 ? kQualityPresets[view.quality_preset].detail
                                 : "Minimal is a short preview. Ultra thins the disk and raises the particle count.";
        ImGui::TextWrapped("%s", detail);
    }
    bool edited = false;
    edited = edit_double("dt", view.scene.config.dt, 0.0001f, 1e-6, 1.0) || edited;
    int steps = static_cast<int>(view.scene.config.steps);
    if (ImGui::DragInt("Steps", &steps, 1.0f, 1, 100000)) {
        view.scene.config.steps = static_cast<std::size_t>(std::max(steps, 1));
        edited = true;
    }
    int sample_every = static_cast<int>(view.scene.config.sample_every);
    if (ImGui::DragInt("Sample every", &sample_every, 0.2f, 1, 1000)) {
        view.scene.config.sample_every = static_cast<std::size_t>(std::max(sample_every, 1));
        edited = true;
    }
    if (view.scene.config.forces.gravity) {
        edited = edit_double("G", view.scene.config.forces.gravitational_constant, 0.001f, 1e-8, 100.0) || edited;
        edited = edit_double("Softening", view.scene.config.forces.softening_length, 0.001f, 0.0, 5.0) || edited;
    }

    if (view.scene.authored_bodies > 0 && view.scene.authored_bodies <= 8) {
        for (std::size_t index = 0; index < view.scene.authored_bodies; ++index) {
            physics::BodyDesc& body = view.scene.bodies[index];
            ImGui::PushID(body.name.c_str());
            if (ImGui::CollapsingHeader(body.name.c_str())) {
                edited = edit_double("Mass", body.particle.mass, 0.01f, 1e-8, 1e6) || edited;
                edited = edit_vec3("Position", body.particle.position, 0.01f) || edited;
                edited = edit_vec3("Velocity", body.particle.velocity, 0.01f) || edited;
                edited = edit_double("Display radius", body.display_radius, 0.001f, 0.001, 10.0) || edited;
                edited = edit_double("Emission", body.emission, 0.01f, 0.0, 20.0) || edited;
            }
            ImGui::PopID();
        }
    }
    for (physics::GalaxySpec& galaxy : view.scene.galaxies) {
        ImGui::PushID(galaxy.name.c_str());
        if (!ImGui::CollapsingHeader(galaxy.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PopID();
            continue;
        }
        int particles = static_cast<int>(galaxy.particles);
        if (ImGui::DragInt("Particles", &particles, 1.0f, 4, 20000)) {
            galaxy.particles = static_cast<std::size_t>(std::max(particles, 4));
            edited = true;
        }
        edited = edit_double("Halo mass", galaxy.halo_mass, 0.01f, 1e-6, 1e6) || edited;
        edited = edit_double("Halo scale", galaxy.halo_scale, 0.01f, 1e-4, 1e4) || edited;
        edited = edit_double("Halo cutoff", galaxy.halo_cutoff, 0.05f, 0.0, 1e4) || edited;
        edited = edit_double("Disk mass", galaxy.disk_mass, 0.01f, 1e-6, 1e6) || edited;
        edited = edit_double("Disk scale", galaxy.disk_scale_radius, 0.01f, 1e-4, 1e4) || edited;
        edited = edit_double("Disk height", galaxy.disk_height, 0.001f, 1e-4, 10.0) || edited;
        edited = edit_double("Dispersion ratio", galaxy.dispersion_ratio, 0.01f, 0.1, 10.0) || edited;
        edited = edit_double("Target stellar Q (0 = ratio)", galaxy.target_q, 0.01f, 0.0, 5.0) || edited;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sets radial random motion independently of height. An initial target, not a stability guarantee.");
        edited = edit_double("Disk particle fraction (0 = equal mass)", galaxy.disk_particle_fraction, 0.01f, 0.0, 0.9) || edited;
        edited = edit_double("Initial spiral strength", galaxy.spiral_strength, 0.01f, 0.0, 0.85) || edited;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Seeds a density pattern in stars and gas. Gravity evolves it freely after initialization.");
        int arms = galaxy.spiral_arms == 4 ? 1 : 0;
        if (ImGui::Combo("Initial arms", &arms, "Two\0Four\0")) { galaxy.spiral_arms = arms ? 4 : 2; edited = true; }
        edited = edit_double("Spiral pitch (degrees)", galaxy.spiral_pitch_degrees, 0.1f, 5.0, 60.0) || edited;
        edited = edit_double("Spiral phase (degrees)", galaxy.spiral_phase_degrees, 0.5f, -360.0, 360.0) || edited;
        edited = ImGui::Checkbox("Height-dependent vertical support", &galaxy.jeans_vertical) || edited;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Experimental Jeans support. Requires isolation validation for your chosen galaxy.");
        edited = edit_double("Stellar population variety", galaxy.population_variation, 0.01f, 0.0, 1.0) || edited;
        edited = edit_double("Initial young mass fraction (peak)", galaxy.initial_young_fraction, 0.005f, 0.0, 1.0) || edited;
        edited = edit_double("Initial age spread", galaxy.initial_young_age, 0.1f, 0.0, 1000.0) || edited;
        edited = edit_double("Young population arm contrast", galaxy.young_arm_contrast, 0.01f, 0.0, 1.0) || edited;
        edited = edit_double("Bulge mass", galaxy.bulge_mass, 0.01f, 0.0, 1e6) || edited;
        edited = edit_double("Bulge scale", galaxy.bulge_scale, 0.01f, 0.0, 1e4) || edited;
        edited = edit_double("Gas mass", galaxy.gas_mass, 0.01f, 0.0, 1e6) || edited;
        int gas_particles = static_cast<int>(galaxy.gas_particles);
        if (ImGui::DragInt("Gas particles", &gas_particles, 1.0f, 0, 20000)) {
            galaxy.gas_particles = static_cast<std::size_t>(std::max(gas_particles, 0));
            edited = true;
        }
        edited = edit_double("Gas height", galaxy.gas_height, 0.001f, 0.0, 10.0) || edited;
        edited = edit_double("Hot gas mass", galaxy.hot_gas_mass, 0.01f, 0.0, 1e6) || edited;
        int hot_particles = static_cast<int>(galaxy.hot_gas_particles);
        if (ImGui::DragInt("Hot gas particles", &hot_particles, 1.0f, 0, 20000)) {
            galaxy.hot_gas_particles = static_cast<std::size_t>(std::max(hot_particles, 0));
            edited = true;
        }
        edited = edit_double("Hot emission", galaxy.hot_emission, 0.01f, 0.0, 20.0) || edited;
        edited = edit_double("Sound speed", galaxy.sound_speed, 0.001f, 1e-4, 5.0) || edited;
        edited = edit_double("Cooling time", galaxy.cooling_time, 0.01f, 0.0, 1e4) || edited;
        edited = edit_double("Star-formation time", galaxy.star_formation_time, 0.01f, 1e-3, 1e4) || edited;
        edited = ImGui::Checkbox("Continuous star formation", &galaxy.continuous_star_formation) || edited;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Converts a fraction of gas each step. Stars share their gas carrier's orbit; total mass is conserved.");
        edited = edit_double("Star-formation density (0 = auto)", galaxy.star_formation_density, 0.001f, 0.0, 1e4) || edited;
        edited = edit_double("Formation temperature / cold floor", galaxy.star_formation_temperature, 0.01f, 1.0, 100.0) || edited;
        edited = ImGui::Checkbox("Require converging gas", &galaxy.star_formation_converging) || edited;
        edited = edit_double("Feedback energy", galaxy.feedback_energy, 0.01f, 0.0, 100.0) || edited;
        edited = edit_double("Feedback time", galaxy.feedback_time, 0.01f, 0.0, 1e4) || edited;
        edited = edit_double("Inclination", galaxy.inclination_degrees, 0.2f, -180.0, 180.0) || edited;
        edited = edit_double("Disk node", galaxy.node_degrees, 0.2f, -180.0, 180.0) || edited;
        bool reversed = galaxy.spin < 0;
        if (ImGui::Checkbox("Reverse spin", &reversed)) { galaxy.spin = reversed ? -1 : 1; edited = true; }
        edited = edit_vec3("Center", galaxy.center, 0.02f) || edited;
        edited = edit_vec3("Velocity", galaxy.velocity, 0.005f) || edited;
        if (ImGui::InputScalar("Seed", ImGuiDataType_U64, &galaxy.seed)) {
            edited = true;
        }
        edited = edit_double("Display radius", galaxy.display_radius, 0.005f, 0.01, 20.0) || edited;
        edited = edit_double("Disk emission", galaxy.emission, 0.01f, 0.0, 20.0) || edited;
        edited = edit_double("Halo emission", galaxy.halo_emission, 0.01f, 0.0, 20.0) || edited;
        edited = edit_double("Bulge emission", galaxy.bulge_emission, 0.01f, 0.0, 20.0) || edited;
        edited = edit_double("Gas emission", galaxy.gas_emission, 0.01f, 0.0, 20.0) || edited;
        edited = edit_double("Young-star emission", galaxy.young_star_emission, 0.01f, 0.0, 20.0) || edited;
        ImGui::PopID();
    }
    if (edited) {
        view.quality_preset = -1;
        if (view.resumable) {
            view.resumable = false;
            view.status = "Parameters changed. Run starts over.";
        }
    }
    if (locked) {
        ImGui::EndDisabled();
    }
}



struct LookParameter { char const* name; float* value; float low, high; };
std::vector<LookParameter> look_parameters(ViewState& view) {
    return {
        {"exposure", &view.exposure, 0.0f, 1000.0f},
        {"bloom", &view.bloom, 0.0f, 1.0f},
        {"smoothing", &view.optical_smoothing, 0.01f, 10.0f},
        {"detail", &view.stellar_detail, 0.0f, 1.0f},
        {"arm-definition", &view.arm_definition, 0.0f, 1.0f},
        {"dust", &view.dust, 0.0f, 100.0f},
        {"dust-structure", &view.dust_structure, 0.0f, 1.0f},
        {"reddening", &view.reddening, 0.0f, 1.0f},
        {"texture", &view.stellar_texture, 0.0f, 0.3f},
        {"gas-glow", &view.gas_glow, 0.0f, 100.0f},
        {"blue-lifetime", &view.young_lifetime, 1e-6f, 1000000.0f},
        {"ionizing-lifetime", &view.ionizing_lifetime, 1e-6f, 1000000.0f},
        {"oiii-lifetime", &view.oiii_lifetime, 1e-6f, 1000000.0f},
        {"young-boost", &view.young_boost, 0.0f, 1000.0f},
        {"association-size", &view.association_size, 0.001f, 100.0f},
        {"nebula-radius", &view.nebula_radius, 0.001f, 100.0f},
        {"ionizing-mass-scale", &view.ionizing_mass_scale, 1e-12f, 1000000.0f},
        {"nebula-density-scale", &view.nebula_density_scale, 1e-12f, 1000000.0f},
        {"halpha-strength", &view.halpha_strength, 0.0f, 1000.0f},
        {"oiii-strength", &view.oiii_strength, 0.0f, 1000.0f},
        {"nebula-size", &view.nebula_size, 0.001f, 100.0f},
        {"halpha-r", &view.halpha_color[0], 0.0f, 1.0f},
        {"halpha-g", &view.halpha_color[1], 0.0f, 1.0f},
        {"halpha-b", &view.halpha_color[2], 0.0f, 1.0f},
        {"oiii-r", &view.oiii_color[0], 0.0f, 1.0f},
        {"oiii-g", &view.oiii_color[1], 0.0f, 1.0f},
        {"oiii-b", &view.oiii_color[2], 0.0f, 1.0f},
    };
}
void save_look(ViewState& view, std::string const& path) {
    for (auto p : look_parameters(view))
        if (!std::isfinite(*p.value) || *p.value < p.low || *p.value > p.high)
            throw std::runtime_error(std::string("Cannot save invalid look parameter: ") + p.name);
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot save look: " + path);
    out << "# PhysicsEngine optical look 1; code units\n" << std::setprecision(9);
    for (auto p : look_parameters(view)) out << p.name << ' ' << *p.value << '\n';
    if (!out) throw std::runtime_error("Failed writing look: " + path);
}
void load_look(ViewState& view, std::string const& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open look: " + path);
    const auto parameters = look_parameters(view);
    std::vector<std::pair<float*, float>> pending;
    std::string line;
    while (std::getline(in, line)) {
        line = line.substr(0, line.find('#'));
        std::istringstream row(line);
        std::string key, extra; float value = 0;
        if (!(row >> key)) continue;
        const auto p = std::find_if(parameters.begin(), parameters.end(), [&](auto const& v) { return key == v.name; });
        if (p == parameters.end() || !(row >> value) || (row >> extra) || !std::isfinite(value) || value < p->low || value > p->high)
            throw std::runtime_error("Invalid look parameter: " + key);
        pending.emplace_back(p->value, value);
    }
    if (in.bad()) throw std::runtime_error("Failed reading look: " + path);
    for (auto [target, value] : pending) *target = value;
}

void draw_appearance(ViewState& view) {
    ImGui::SliderFloat("UI scale", &view.ui_scale, 1.0f, 3.0f, "%.2f");
    ImGui::Checkbox("Axes", &view.show_axes);
    const bool particle_scene = (!view.trajectory.bodies.empty() || !view.scene.bodies.empty() || !view.scene.galaxies.empty());
    if (particle_scene) {
        const char* pictures[] = {"Particles", "Illustrative volume", "Optical"};
        ImGui::Combo("Picture", &view.picture_mode, pictures, 3);
        ImGui::TextWrapped(
            "Optical draws stellar light and foreground dust. Illustrative volume uses a stylized density palette. Appearance does not change the simulation.");
    }
    const bool field_view = particle_scene && view.picture_mode == 1;
    if (field_view) {
        if (particle_scene) {
            const char* resolutions[] = {"Soft", "Coarse", "Standard", "Fine"};
            ImGui::Combo("Resolution", &view.field_resolution, resolutions, 4);
        }
        ImGui::SliderFloat("Field brightness", &view.field_gain, 0.2f, 4.0f);
        ImGui::SliderFloat("Field contrast", &view.field_contrast, 0.0f, 1.0f);
        ImGui::SliderFloat("Field heat", &view.field_heat, 0.0f, 1.0f);
        ImGui::TextWrapped("Resolution controls the reconstructed galaxy density field. Brightness, contrast and heat affect its appearance only.");
    }
    if (!field_view && view.picture_mode == 2) {
        if (view.trajectory.tracer_softening > 0)
            ImGui::TextWrapped("Refined starlight: massless tracers follow the recorded gravity field. Their light does not add gravitational mass.");
        if (ImGui::Button("Frame current light")) compose_stellar_view(view,"wide");
        if (ImGui::Button("Face-on disk")) compose_stellar_view(view,"face");
        ImGui::SameLine(); if (ImGui::Button("Edge-on disk")) compose_stellar_view(view,"edge");
        if (ImGui::Button("First passage")) compose_stellar_view(view,"passage");
        ImGui::SameLine(); if (ImGui::Button("After passage")) compose_stellar_view(view,"tails");
        ImGui::TextWrapped("Disk views select the first galaxy. Compositions stay fixed during playback; After passage is a suggested tail view.");
        ImGui::SliderFloat("Exposure", &view.exposure, 0.5f, 30.0f);
        ImGui::SliderFloat("Bloom", &view.bloom, 0.0f, 0.6f);
        ImGui::SliderFloat("Structure smoothing", &view.optical_smoothing, 0.5f, 1.8f);
        ImGui::SliderFloat("Fine stellar structure", &view.stellar_detail, 0.0f, 1.0f);
        ImGui::SliderFloat("Arm and stream definition", &view.arm_definition, 0.0f, 1.0f);
        ImGui::SliderFloat("Dust lane structure", &view.dust_structure, 0.0f, 1.0f);
        ImGui::SliderFloat("Dust", &view.dust, 0.0f, 5.0f);
        ImGui::SliderFloat("Reddening", &view.reddening, 0.0f, 1.0f);
        ImGui::SliderFloat("Nebula emission", &view.gas_glow, 0.0f, 3.0f);
        if (ImGui::TreeNode("Stellar populations and emission lines")) {
            ImGui::TextWrapped("Age clocks are in simulation units. Blue stellar light lasts longer than ionizing light. Emission lines are illustrative proxies.");
            ImGui::SliderFloat("Blue-light lifetime", &view.young_lifetime, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Ionizing lifetime", &view.ionizing_lifetime, 0.01f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("OIII source lifetime", &view.oiii_lifetime, 0.01f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Young-light boost", &view.young_boost, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Association size", &view.association_size, 0.05f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Ionizing reach", &view.nebula_radius, 0.05f, 5.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Ionizing mass scale", &view.ionizing_mass_scale, 0.000001f, 0.1f, "%.6f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Emission density scale", &view.nebula_density_scale, 0.00001f, 1.0f, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("H-alpha strength", &view.halpha_strength, 0.0f, 50.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("OIII strength", &view.oiii_strength, 0.0f, 50.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Nebula size", &view.nebula_size, 0.05f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::ColorEdit3("H-alpha palette", view.halpha_color);
            ImGui::ColorEdit3("OIII palette", view.oiii_color);
            if (ImGui::Button("Broadband look")) { view.halpha_strength = 8; view.oiii_strength = 2; view.gas_glow = .45f; view.young_boost = 1; view.ionizing_mass_scale = .0002f; }
            ImGui::SameLine();
            if (ImGui::Button("Emission emphasis")) { view.halpha_strength = 24; view.oiii_strength = 6; view.gas_glow = 1; view.young_boost = 2; view.ionizing_mass_scale = .00002f; }
            ImGui::TreePop();
        }
        ImGui::SliderFloat("Stellar texture", &view.stellar_texture, 0.0f, 0.3f);
        ImGui::InputText("Look file", view.look_path, sizeof(view.look_path));
        if (ImGui::Button("Save look")) {
            try { save_look(view, view.look_path); view.status = "Saved optical look."; view.error.clear(); }
            catch (std::exception const& e) { view.error = e.what(); }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load look")) {
            try { load_look(view, view.look_path); view.status = "Loaded optical look."; view.error.clear(); }
            catch (std::exception const& e) { view.error = e.what(); }
        }
        ImGui::Checkbox("Show dynamical halo", &view.show_halo);
        ImGui::Checkbox("Show hot atmosphere", &view.show_hot_gas);
        ImGui::TextWrapped("Lifetime is in simulation units. Halo and atmosphere light are illustrative overlays. Dust absorbs stars behind it; young clusters fade toward warm starlight.");
    }
    if (!field_view && view.picture_mode != 2) {
    ImGui::SliderFloat("Point scale", &view.point_scale, 0.2f, 8.0f);
    ImGui::SliderFloat("Light smoothing", &view.light_smoothing, 0.0f, 8.0f);
    ImGui::SliderFloat("Soften", &view.soften, 0.0f, 28.0f);
    ImGui::SliderFloat("Soften brightness", &view.soften_brightness, 0.0f, 8.0f);
    ImGui::SliderFloat("Dust", &view.dust, 0.0f, 5.0f);
    ImGui::SliderFloat("Reddening", &view.reddening, 0.0f, 1.0f);
    ImGui::SliderFloat("Gas glow", &view.gas_glow, 0.0f, 3.0f);
    ImGui::TextWrapped(
        "Smoothing reaches toward the next particle, so crowded regions stay small. Soften is the width of the smooth galaxy underneath those details. Soften brightness lifts only that smooth layer, so a wide one stays visible on the dark background. Nearby gas adds into one cloud, and a neck appears only where the clouds overlap. Dust darkens that smooth body along the cloud. Reddening takes the blue out first and returns a little of the blocked light as brown.");
    ImGui::SliderFloat("Trail", &view.trail_fraction, 0.02f, 1.0f);
    ImGui::SliderFloat("Key light", &view.key_intensity, 0.0f, 3.0f);
    ImGui::SliderFloat("Fill light", &view.fill_intensity, 0.0f, 3.0f);
    }
    if (!field_view && view.color_groups.empty()) {
        ImGui::TextUnformatted("Colors appear when particles are on screen.");
    }
    for (ColorGroup& group : view.color_groups) {
        ImGui::PushID(group.label.c_str());
        if (ImGui::ColorEdit3(group.label.c_str(), group.rgb)) {
            paint_group(view, group);
        }
        ImGui::PopID();
    }
}

void sample_at(physics::Trajectory const& trajectory, double time, std::vector<physics::Vec3>& positions,
               physics::Diagnostics& diagnostics) {
    auto const& samples = trajectory.samples;
    if (time <= samples.front().time) {
        positions = samples.front().position;
        diagnostics = samples.front().diagnostics;
        return;
    }
    if (time >= samples.back().time) {
        positions = samples.back().position;
        diagnostics = samples.back().diagnostics;
        return;
    }
    auto upper = std::lower_bound(samples.begin(), samples.end(), time,
                                  [](physics::Sample const& sample, double value) { return sample.time < value; });
    auto const& after = *upper;
    auto const& before = *(upper - 1);
    const double span = after.time - before.time;
    const double blend = span == 0.0 ? 0.0 : (time - before.time) / span;
    positions.resize(before.position.size());
    for (std::size_t i = 0; i < positions.size(); ++i) {
        positions[i] = before.velocity.size() == positions.size() && after.velocity.size() == positions.size()
            ? physics::visual::interpolate_position(before.position[i], before.velocity[i], after.position[i],
                                                     after.velocity[i], span, blend)
            : before.position[i] * (1.0 - blend) + after.position[i] * blend;
    }
    auto lerp = [blend](double a, double b) { return a * (1.0 - blend) + b * blend; };
    diagnostics.kinetic = lerp(before.diagnostics.kinetic, after.diagnostics.kinetic);
    diagnostics.potential = lerp(before.diagnostics.potential, after.diagnostics.potential);
    diagnostics.energy = lerp(before.diagnostics.energy, after.diagnostics.energy);
    diagnostics.linear_momentum =
        before.diagnostics.linear_momentum * (1.0 - blend) + after.diagnostics.linear_momentum * blend;
    diagnostics.angular_momentum =
        before.diagnostics.angular_momentum * (1.0 - blend) + after.diagnostics.angular_momentum * blend;
}

char const* kLitVertex = R"(#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 v_normal;
out vec3 v_world;
void main() {
    vec4 world = u_model * vec4(a_position, 1.0);
    v_world = world.xyz;
    v_normal = mat3(u_model) * a_normal;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

char const* kLitFragment = R"(#version 330 core
in vec3 v_normal;
in vec3 v_world;
uniform vec3 u_color;
uniform float u_roughness;
uniform vec3 u_key_dir;
uniform vec3 u_fill_dir;
uniform float u_key_intensity;
uniform float u_fill_intensity;
uniform vec3 u_eye;
out vec4 fragment_color;

vec3 shade(vec3 normal, vec3 view, vec3 light_dir, float intensity) {
    vec3 light = normalize(light_dir);
    float diffuse = max(dot(normal, light), 0.0);
    float shininess = mix(96.0, 6.0, clamp(u_roughness, 0.0, 1.0));
    vec3 half_vector = normalize(light + view);
    float specular = pow(max(dot(normal, half_vector), 0.0), shininess) * (1.0 - clamp(u_roughness, 0.0, 1.0));
    return intensity * (u_color * diffuse + vec3(specular) * 0.35);
}

void main() {
    vec3 normal = normalize(v_normal);
    vec3 view = normalize(u_eye - v_world);
    vec3 color = u_color * 0.08 + shade(normal, view, u_key_dir, u_key_intensity) +
                 shade(normal, view, u_fill_dir, u_fill_intensity);
    fragment_color = vec4(color, 1.0);
}
)";

char const* kColorVertex = R"(#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
uniform mat4 u_view_projection;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_view_projection * vec4(a_position, 1.0);
}
)";

char const* kColorFragment = R"(#version 330 core
in vec4 v_color;
out vec4 fragment_color;
void main() {
    fragment_color = v_color;
}
)";

#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif

char const* kStarVertex = R"(#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in float a_size;
layout(location = 3) in float a_emission;
uniform mat4 u_view_projection;
uniform float u_viewport_height;
out vec3 v_color;
out float v_emission;
void main() {
    vec4 clip = u_view_projection * vec4(a_position, 1.0);
    gl_Position = clip;
    float pixels = a_size * u_viewport_height / max(clip.w, 0.001);
    gl_PointSize = clamp(pixels, 1.0, 64.0);
    v_color = a_color;
    v_emission = a_emission;
}
)";

char const* kStarFragment = R"(#version 330 core
in vec3 v_color;
in float v_emission;
out vec4 fragment_color;
void main() {
    vec2 point = gl_PointCoord * 2.0 - 1.0;
    float radius_squared = dot(point, point);
    if (radius_squared > 1.0) {
        discard;
    }
    float core = exp(-4.5 * radius_squared);
    float glow = exp(-1.6 * radius_squared);
    vec3 color = v_color * v_emission * (0.35 * glow + 1.15 * core);
    fragment_color = vec4(color, 1.0);
}
)";

// Area-normalized splat. The integral of one particle stays put when the kernel grows,
// so a disk fills in instead of becoming a field of brighter dots.
char const* kFieldVertex = R"(#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;
layout(location = 2) in float a_size;
layout(location = 3) in float a_emission;
uniform mat4 u_view_projection;
uniform float u_viewport_height;
uniform float u_gain;
uniform float u_max_point;
uniform float u_profile;
out vec3 v_color;
out float v_energy;
out float v_profile;
void main() {
    vec4 clip = u_view_projection * vec4(a_position, 1.0);
    gl_Position = clip;
    float pixels = a_size * u_viewport_height / max(clip.w, 0.001);
    gl_PointSize = clamp(pixels, 1.0, u_max_point);
    v_color = a_color;
    // Brightness stays with the particle. A wide patch fills a gap instead of fading out.
    v_energy = a_emission * u_gain;
    v_profile = u_profile;
}
)";

char const* kFieldFragment = R"(#version 330 core
in vec3 v_color;
in float v_energy;
in float v_profile;
out vec4 fragment_color;
void main() {
    vec2 point = gl_PointCoord * 2.0 - 1.0;
    float radius_squared = dot(point, point);
    if (radius_squared > 1.0) {
        discard;
    }
    float radius = length(point);
    // Stars keep a flat patch. Gas falls off smoothly, so two overlapping clouds grow a neck.
    float kernel = v_profile > 0.5 ? pow(1.0 - radius_squared, 2.0) : 1.0 - smoothstep(0.55, 1.0, radius);
    float weight = v_energy * kernel;
    fragment_color = vec4(v_color * weight, weight);
}
)";

char const* kCompositeVertex = R"(#version 330 core
out vec2 v_uv;
void main() {
    vec2 vertices[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
    v_uv = 0.5 * gl_Position.xy + 0.5;
}
)";

char const* kCompositeFragment = R"(#version 330 core
in vec2 v_uv;
uniform sampler2D u_light;
uniform sampler2D u_body;
uniform sampler2D u_gas;
uniform float u_dust;
uniform float u_reddening;
uniform float u_gas_glow;
uniform float u_soften_brightness;
out vec4 fragment_color;
void main() {
    vec3 sharp = texture(u_light, v_uv).rgb;
    vec3 body = texture(u_body, v_uv).rgb;
    vec4 gas = texture(u_gas, v_uv);
    vec3 detail = max(sharp - body, vec3(0.0));
    body *= max(u_soften_brightness, 0.0);
    // One cloud peaks near 1. The sum crosses the floor between them only when the clouds overlap.
    float field = gas.a;
    float lobe = smoothstep(0.22, 0.72, field);
    vec3 gas_color = gas.rgb / max(field, 0.001);
    float column = field / (1.0 + field);
    // Broadband dust relative to V. The mean stays 1, so Dust still sets how dark the lane is.
    vec3 extinction = vec3(0.80, 1.00, 1.30);
    extinction /= dot(extinction, vec3(1.0)) / 3.0;
    float reddening = clamp(u_reddening, 0.0, 1.0);
    float tau = u_dust * column * 0.75;
    vec3 transmission = exp(-tau * mix(vec3(1.0), extinction, reddening));
    vec3 through = body * transmission;
    vec3 scattered = (body - through) * (0.40 * reddening) * vec3(0.62, 0.40, 0.20);
    vec3 gas_light = gas_color * lobe * (0.65 + 0.5 * column) * u_gas_glow;
    vec3 color = through + scattered + detail + gas_light;
    vec3 mapped = color / (1.0 + color);
    fragment_color = vec4(mapped, 1.0);
}
)";

char const* kBlurFragment = R"(#version 330 core
in vec2 v_uv;
uniform sampler2D u_image;
uniform vec2 u_texel;
uniform vec2 u_span;
out vec4 fragment_color;
void main() {
    if (u_span.x > 1.5 || u_span.y > 1.5) {
        int count_x = clamp(int(u_span.x + 0.5), 1, 16);
        int count_y = clamp(int(u_span.y + 0.5), 1, 16);
        vec4 color = vec4(0.0);
        for (int y = 0; y < 16; ++y) {
            if (y >= count_y) {
                break;
            }
            for (int x = 0; x < 16; ++x) {
                if (x >= count_x) {
                    break;
                }
                vec2 offset = (vec2(float(x), float(y)) + 0.5) / vec2(float(count_x), float(count_y)) - 0.5;
                color += texture(u_image, v_uv + offset * u_texel * u_span);
            }
        }
        fragment_color = color / float(count_x * count_y);
        return;
    }
    if (dot(u_texel, u_texel) == 0.0) {
        fragment_color = texture(u_image, v_uv);
        return;
    }
    float weights[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec4 color = texture(u_image, v_uv) * weights[0];
    for (int sample_index = 1; sample_index < 5; ++sample_index) {
        vec2 offset = u_texel * float(sample_index);
        color += texture(u_image, v_uv + offset) * weights[sample_index];
        color += texture(u_image, v_uv - offset) * weights[sample_index];
    }
    fragment_color = color;
}
)";

char const* kVolumeVertex = R"(#version 330 core
layout(location = 0) in vec2 a_clip;
out vec2 v_ndc;
void main() {
    v_ndc = a_clip;
    gl_Position = vec4(a_clip, 0.0, 1.0);
}
)";

char const* kVolumeFragment = R"(#version 330 core
in vec2 v_ndc;
uniform sampler3D u_field;
uniform vec3 u_eye;
uniform vec3 u_forward;
uniform vec3 u_right;
uniform vec3 u_up;
uniform vec2 u_scale;
uniform vec3 u_origin;
uniform vec3 u_extent;
uniform float u_mean;
uniform float u_floor;
uniform float u_gain;
uniform float u_contrast;
uniform float u_heat;
uniform float u_extinction;
out vec4 fragment_color;
void main() {
    vec3 direction = normalize(u_forward + u_right * v_ndc.x * u_scale.x + u_up * v_ndc.y * u_scale.y);
    vec3 origin = u_eye;
    vec3 inv_direction = 1.0 / direction;
    vec3 box_min = u_origin;
    vec3 box_max = u_origin + u_extent;
    vec3 near_hit = (box_min - origin) * inv_direction;
    vec3 far_hit = (box_max - origin) * inv_direction;
    vec3 enter = min(near_hit, far_hit);
    vec3 leave = max(near_hit, far_hit);
    float t0 = max(enter.x, max(enter.y, enter.z));
    float t1 = min(leave.x, min(leave.y, leave.z));
    if (t1 <= max(t0, 0.0)) {
        fragment_color = vec4(0.0);
        return;
    }
    t0 = max(t0, 0.0);
    const int steps = 128;
    float march = (t1 - t0) / float(steps);
    vec3 color = vec3(0.0);
    float alpha = 0.0;
    for (int step = 0; step < steps; ++step) {
        vec3 position = origin + direction * (t0 + (float(step) + 0.5) * march);
        vec3 uv = (position - u_origin) / u_extent;
        vec2 sample_field = texture(u_field, uv).rg;
        float density = sample_field.r / max(u_mean, 1e-6);
        float heat = sample_field.g / max(u_floor, 1e-8);
        float column = log(max(density, 1e-4));
        float width = mix(1.8, 0.28, clamp(u_contrast, 0.0, 1.0));
        float center = mix(-0.35, 1.35, clamp(u_contrast, 0.0, 1.0));
        float tau = smoothstep(center - width, center + width, column);
        float hot = clamp((heat - 1.0) / mix(12.0, 1.2, clamp(u_heat, 0.0, 1.0)), 0.0, 1.0);
        float dense = clamp((column + 0.15) / 2.4, 0.0, 1.0);
        float tone = smoothstep(0.18, 0.92, clamp(max(hot, dense * 0.45), 0.0, 1.0));
        vec3 cool = vec3(0.32, 0.12, 0.55);
        vec3 warm = vec3(0.95, 0.40, 0.12);
        vec3 core = vec3(1.0, 0.86, 0.62);
        vec3 tint = mix(mix(cool, warm, tone), core, smoothstep(0.72, 1.0, hot));
        float absorb = 1.0 - exp(-tau * u_gain * u_extinction);
        color += (1.0 - alpha) * tint * absorb;
        alpha += (1.0 - alpha) * absorb;
        if (alpha > 0.98) {
            break;
        }
    }
    fragment_color = vec4(color, alpha);
}
)";

}  // namespace

struct SceneGpu {
    GLuint optical_program = 0;
    GLuint optical_post = 0;
    GLuint optical_vao = 0;
    GLuint optical_buffer = 0;
    GLuint lit_program = 0;
    GLuint color_program = 0;
    GLuint star_program = 0;
    GLuint sphere_vao = 0;
    GLsizei sphere_indices = 0;
    GLuint color_vao = 0;
    GLuint color_buffer = 0;
    GLuint star_vao = 0;
    GLuint star_buffer = 0;
    GLint mvp_location = -1;
    GLint model_location = -1;
    GLint color_location = -1;
    GLint roughness_location = -1;
    GLint key_dir_location = -1;
    GLint fill_dir_location = -1;
    GLint key_intensity_location = -1;
    GLint fill_intensity_location = -1;
    GLint eye_location = -1;
    GLint view_projection_location = -1;
    GLint star_view_location = -1;
    GLint star_height_location = -1;
    GLuint field_program = 0;
    GLuint composite_program = 0;
    GLuint field_vao = 0;
    GLint field_view_location = -1;
    GLint field_height_location = -1;
    GLint field_gain_location = -1;
    GLint field_max_point_location = -1;
    GLint field_profile_location = -1;
    GLint composite_light_location = -1;
    GLint composite_body_location = -1;
    GLint composite_gas_location = -1;
    GLint composite_dust_location = -1;
    GLint composite_reddening_location = -1;
    GLint composite_glow_location = -1;
    GLint composite_soften_brightness_location = -1;
    GLuint blur_program = 0;
    GLint blur_image_location = -1;
    GLint blur_texel_location = -1;
    GLint blur_span_location = -1;
    GLuint low_a_fbo = 0;
    GLuint low_a_tex = 0;
    GLuint low_b_fbo = 0;
    GLuint low_b_tex = 0;
    int low_width = 0;
    int low_height = 0;
    GLuint light_fbo = 0;
    GLuint light_tex = 0;
    GLuint body_fbo = 0;
    GLuint body_tex = 0;
    GLuint gas_fbo = 0;
    GLuint gas_tex = 0;
    GLuint scratch_fbo = 0;
    GLuint scratch_tex = 0;
    int field_width = 0;
    int field_height = 0;
    GLuint volume_program = 0;
    GLuint volume_vao = 0;
    GLuint volume_texture = 0;
    int volume_n = 0;
    double volume_time = -1.0;
    unsigned volume_epoch = 0;
    int volume_kind = 0;
    int volume_resolution = -1;
    GLint volume_field_location = -1;
    GLint volume_eye_location = -1;
    GLint volume_forward_location = -1;
    GLint volume_right_location = -1;
    GLint volume_up_location = -1;
    GLint volume_scale_location = -1;
    GLint volume_origin_location = -1;
    GLint volume_extent_location = -1;
    GLint volume_mean_location = -1;
    GLint volume_floor_location = -1;
    GLint volume_gain_location = -1;
    GLint volume_contrast_location = -1;
    GLint volume_heat_location = -1;
    GLint volume_extinction_location = -1;
    float volume_mean = 1.0f;
    float volume_floor = 1.0f;
    physics::Vec3 volume_origin{};
    physics::Vec3 volume_extent{1.0, 1.0, 1.0};
};

struct ExportTarget {
    GLuint fbo = 0;
    GLuint color = 0;
    GLuint depth = 0;
    int width = 0;
    int height = 0;

    void destroy() {
        if (fbo != 0) {
            gl_delete_framebuffers(1, &fbo);
            fbo = 0;
        }
        if (color != 0) {
            glDeleteTextures(1, &color);
            color = 0;
        }
        if (depth != 0) {
            gl_delete_renderbuffers(1, &depth);
            depth = 0;
        }
        width = 0;
        height = 0;
    }

    bool ensure(int w, int h, std::string& error) {
        if (fbo != 0 && width == w && height == h) {
            gl_bind_framebuffer(GL_FRAMEBUFFER, fbo);
            return true;
        }
        destroy();
        gl_gen_framebuffers(1, &fbo);
        gl_bind_framebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &color);
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl_framebuffer_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
        gl_gen_renderbuffers(1, &depth);
        gl_bind_renderbuffer(GL_RENDERBUFFER, depth);
        gl_renderbuffer_storage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        gl_framebuffer_renderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        const GLenum draw = GL_COLOR_ATTACHMENT0;
        gl_draw_buffers(1, &draw);
        const GLenum status = gl_check_framebuffer_status(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            error = "could not create a render target at this resolution";
            destroy();
            gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
        width = w;
        height = h;
        return true;
    }
};

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D 0x806F
#endif
#ifndef GL_TEXTURE_WRAP_R
#define GL_TEXTURE_WRAP_R 0x8072
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif

void destroy_field_targets(SceneGpu& gpu) {
    if (gpu.light_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.light_fbo);
        gpu.light_fbo = 0;
    }
    if (gpu.gas_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.gas_fbo);
        gpu.gas_fbo = 0;
    }
    if (gpu.light_tex != 0) {
        glDeleteTextures(1, &gpu.light_tex);
        gpu.light_tex = 0;
    }
    if (gpu.body_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.body_fbo);
        gpu.body_fbo = 0;
    }
    if (gpu.body_tex != 0) {
        glDeleteTextures(1, &gpu.body_tex);
        gpu.body_tex = 0;
    }
    if (gpu.gas_tex != 0) {
        glDeleteTextures(1, &gpu.gas_tex);
        gpu.gas_tex = 0;
    }
    if (gpu.scratch_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.scratch_fbo);
        gpu.scratch_fbo = 0;
    }
    if (gpu.scratch_tex != 0) {
        glDeleteTextures(1, &gpu.scratch_tex);
        gpu.scratch_tex = 0;
    }
    if (gpu.low_a_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.low_a_fbo);
        gpu.low_a_fbo = 0;
    }
    if (gpu.low_b_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.low_b_fbo);
        gpu.low_b_fbo = 0;
    }
    if (gpu.low_a_tex != 0) {
        glDeleteTextures(1, &gpu.low_a_tex);
        gpu.low_a_tex = 0;
    }
    if (gpu.low_b_tex != 0) {
        glDeleteTextures(1, &gpu.low_b_tex);
        gpu.low_b_tex = 0;
    }
    gpu.low_width = 0;
    gpu.low_height = 0;
    gpu.field_width = 0;
    gpu.field_height = 0;
}

GLuint make_field_texture(int width, int height, GLenum internal) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

bool attach_field_target(GLuint& framebuffer, GLuint texture) {
    gl_gen_framebuffers(1, &framebuffer);
    gl_bind_framebuffer(GL_FRAMEBUFFER, framebuffer);
    gl_framebuffer_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    const GLenum draw = GL_COLOR_ATTACHMENT0;
    gl_draw_buffers(1, &draw);
    return gl_check_framebuffer_status(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

bool ensure_field_targets(SceneGpu& gpu, int width, int height) {
    if (gpu.light_fbo != 0 && gpu.field_width == width && gpu.field_height == height) {
        return true;
    }
    destroy_field_targets(gpu);
    const GLenum internal = GL_RGBA16F;
    gpu.light_tex = make_field_texture(width, height, internal);
    gpu.body_tex = make_field_texture(width, height, internal);
    gpu.gas_tex = make_field_texture(width, height, internal);
    gpu.scratch_tex = make_field_texture(width, height, internal);
    const bool light_ok = attach_field_target(gpu.light_fbo, gpu.light_tex);
    const bool body_ok = attach_field_target(gpu.body_fbo, gpu.body_tex);
    const bool gas_ok = attach_field_target(gpu.gas_fbo, gpu.gas_tex);
    const bool scratch_ok = attach_field_target(gpu.scratch_fbo, gpu.scratch_tex);
    if (!light_ok || !body_ok || !gas_ok || !scratch_ok) {
        destroy_field_targets(gpu);
        gpu.light_tex = make_field_texture(width, height, GL_RGBA8);
        gpu.body_tex = make_field_texture(width, height, GL_RGBA8);
        gpu.gas_tex = make_field_texture(width, height, GL_RGBA8);
        gpu.scratch_tex = make_field_texture(width, height, GL_RGBA8);
        if (!attach_field_target(gpu.light_fbo, gpu.light_tex) || !attach_field_target(gpu.body_fbo, gpu.body_tex) ||
            !attach_field_target(gpu.gas_fbo, gpu.gas_tex) || !attach_field_target(gpu.scratch_fbo, gpu.scratch_tex)) {
            destroy_field_targets(gpu);
            gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
    }
    gpu.field_width = width;
    gpu.field_height = height;
    gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void splat_field(SceneGpu const& gpu, std::vector<float> const& vertices, float gain, float max_point, float profile,
                  Mat4 const& view_projection, int height, GLuint framebuffer, bool clear) {
    if (!clear && vertices.empty()) {
        return;
    }
    gl_bind_framebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, gpu.field_width, gpu.field_height);
    if (clear) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    if (vertices.empty()) {
        return;
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    gl_use_program(gpu.field_program);
    gl_uniform_matrix4_fv(gpu.field_view_location, 1, GL_FALSE, view_projection.m);
    gl_uniform_1f(gpu.field_height_location, static_cast<float>(height));
    gl_uniform_1f(gpu.field_gain_location, gain);
    gl_uniform_1f(gpu.field_max_point_location, max_point);
    gl_uniform_1f(gpu.field_profile_location, profile);
    gl_bind_vertex_array(gpu.star_vao);
    gl_bind_buffer(GL_ARRAY_BUFFER, gpu.star_buffer);
    gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(vertices.size() * sizeof(float)), vertices.data(),
                   GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices.size() / 8));
}

void blur_pass(SceneGpu const& gpu, GLuint source_tex, GLuint dest_framebuffer, int width, int height, float texel_x,
               float texel_y, float span_x, float span_y) {
    gl_bind_framebuffer(GL_FRAMEBUFFER, dest_framebuffer);
    glViewport(0, 0, width, height);
    glBindTexture(GL_TEXTURE_2D, source_tex);
    gl_uniform_2f(gpu.blur_texel_location, texel_x, texel_y);
    gl_uniform_2f(gpu.blur_span_location, span_x, span_y);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool ensure_low_targets(SceneGpu& gpu, int width, int height) {
    if (gpu.low_a_fbo != 0 && gpu.low_width == width && gpu.low_height == height) {
        return true;
    }
    if (gpu.low_a_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.low_a_fbo);
        gpu.low_a_fbo = 0;
    }
    if (gpu.low_b_fbo != 0) {
        gl_delete_framebuffers(1, &gpu.low_b_fbo);
        gpu.low_b_fbo = 0;
    }
    if (gpu.low_a_tex != 0) {
        glDeleteTextures(1, &gpu.low_a_tex);
        gpu.low_a_tex = 0;
    }
    if (gpu.low_b_tex != 0) {
        glDeleteTextures(1, &gpu.low_b_tex);
        gpu.low_b_tex = 0;
    }
    gpu.low_width = 0;
    gpu.low_height = 0;
    auto make_pair = [&](GLenum internal) {
        gpu.low_a_tex = make_field_texture(width, height, internal);
        gpu.low_b_tex = make_field_texture(width, height, internal);
        return attach_field_target(gpu.low_a_fbo, gpu.low_a_tex) && attach_field_target(gpu.low_b_fbo, gpu.low_b_tex);
    };
    if (!make_pair(GL_RGBA16F)) {
        if (gpu.low_a_fbo != 0) {
            gl_delete_framebuffers(1, &gpu.low_a_fbo);
            gpu.low_a_fbo = 0;
        }
        if (gpu.low_b_fbo != 0) {
            gl_delete_framebuffers(1, &gpu.low_b_fbo);
            gpu.low_b_fbo = 0;
        }
        if (gpu.low_a_tex != 0) {
            glDeleteTextures(1, &gpu.low_a_tex);
            gpu.low_a_tex = 0;
        }
        if (gpu.low_b_tex != 0) {
            glDeleteTextures(1, &gpu.low_b_tex);
            gpu.low_b_tex = 0;
        }
        if (!make_pair(GL_RGBA8)) {
            gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
    }
    gpu.low_width = width;
    gpu.low_height = height;
    gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

// Leaves `source_tex` unchanged. Samples sit one pixel apart, so a wide blur stays a smooth
// sheet instead of a lattice of the original points. A radius below one pixel is a copy.
void blur_separable(SceneGpu& gpu, GLuint source_tex, GLuint dest_framebuffer, GLuint dest_tex, float radius_px,
                    int iterations) {
    if (gpu.blur_program == 0 || gpu.scratch_fbo == 0) {
        return;
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    gl_use_program(gpu.blur_program);
    gl_uniform_1i(gpu.blur_image_location, 0);
    gl_active_texture(GL_TEXTURE0);
    gl_bind_vertex_array(gpu.field_vao);
    const int field_width = std::max(gpu.field_width, 1);
    const int field_height = std::max(gpu.field_height, 1);
    if (!(radius_px >= 1.0f) || iterations < 1) {
        blur_pass(gpu, source_tex, dest_framebuffer, field_width, field_height, 0.0f, 0.0f, 1.0f, 1.0f);
        return;
    }
    // One full pass of the five-tap kernel has this sigma, in pixels of whichever image it runs on.
    constexpr float kTapSigma = 1.689f;
    const float sigma = kTapSigma * radius_px * std::sqrt(static_cast<float>(iterations));
    int factor = 1;
    while (factor < 16 && sigma / static_cast<float>(factor) > 3.4f) {
        factor *= 2;
    }
    const float sigma_low = sigma / static_cast<float>(factor);
    int passes = static_cast<int>(std::lround((sigma_low / kTapSigma) * (sigma_low / kTapSigma)));
    passes = std::clamp(passes, 1, 8);
    auto separable = [&](GLuint source, GLuint scratch_tex, GLuint scratch_framebuffer, GLuint current_tex,
                         GLuint current_framebuffer, int width, int height) {
        const float texel_x = 1.0f / static_cast<float>(std::max(width, 1));
        const float texel_y = 1.0f / static_cast<float>(std::max(height, 1));
        blur_pass(gpu, source, scratch_framebuffer, width, height, texel_x, 0.0f, 1.0f, 1.0f);
        blur_pass(gpu, scratch_tex, current_framebuffer, width, height, 0.0f, texel_y, 1.0f, 1.0f);
        for (int pass = 1; pass < passes; ++pass) {
            blur_pass(gpu, current_tex, scratch_framebuffer, width, height, texel_x, 0.0f, 1.0f, 1.0f);
            blur_pass(gpu, scratch_tex, current_framebuffer, width, height, 0.0f, texel_y, 1.0f, 1.0f);
        }
    };
    if (factor == 1) {
        separable(source_tex, gpu.scratch_tex, gpu.scratch_fbo, dest_tex, dest_framebuffer, field_width, field_height);
        return;
    }
    const int low_width = std::max(1, field_width / factor);
    const int low_height = std::max(1, field_height / factor);
    if (!ensure_low_targets(gpu, low_width, low_height)) {
        separable(source_tex, gpu.scratch_tex, gpu.scratch_fbo, dest_tex, dest_framebuffer, field_width, field_height);
        return;
    }
    blur_pass(gpu, source_tex, gpu.low_a_fbo, low_width, low_height, 1.0f / static_cast<float>(field_width),
              1.0f / static_cast<float>(field_height), static_cast<float>(field_width) / static_cast<float>(low_width),
              static_cast<float>(field_height) / static_cast<float>(low_height));
    separable(gpu.low_a_tex, gpu.low_b_tex, gpu.low_b_fbo, gpu.low_a_tex, gpu.low_a_fbo, low_width, low_height);
    blur_pass(gpu, gpu.low_a_tex, dest_framebuffer, field_width, field_height, 0.0f, 0.0f, 1.0f, 1.0f);
}

// Distance to the nearest other sample. Crowded regions return a small distance.
void nearest_spacing(std::vector<physics::Vec3> const& positions, std::vector<std::size_t> const& indices,
                     std::vector<float>& spacing, float cell) {
    spacing.assign(indices.size(), cell);
    struct Node {
        std::int64_t cell = 0;
        int local = 0;
    };
    std::vector<Node> nodes(indices.size());
    auto pack = [](int ix, int iy, int iz) {
        return (static_cast<std::int64_t>(ix + 200000) << 42) | (static_cast<std::int64_t>(iy + 200000) << 21) |
               static_cast<std::int64_t>(iz + 200000);
    };
    for (int local = 0; local < static_cast<int>(indices.size()); ++local) {
        const physics::Vec3& position = positions[indices[static_cast<std::size_t>(local)]];
        nodes[static_cast<std::size_t>(local)] = {
            pack(static_cast<int>(std::floor(position.x / cell)), static_cast<int>(std::floor(position.y / cell)),
                 static_cast<int>(std::floor(position.z / cell))),
            local};
    }
    std::sort(nodes.begin(), nodes.end(), [](Node const& a, Node const& b) { return a.cell < b.cell; });
    auto range = [&](std::int64_t id) {
        const auto lower = std::lower_bound(nodes.begin(), nodes.end(), id,
                                            [](Node const& node, std::int64_t value) { return node.cell < value; });
        auto upper = lower;
        while (upper != nodes.end() && upper->cell == id) {
            ++upper;
        }
        return std::pair<std::vector<Node>::const_iterator, std::vector<Node>::const_iterator>{lower, upper};
    };
    for (int local = 0; local < static_cast<int>(indices.size()); ++local) {
        const physics::Vec3& position = positions[indices[static_cast<std::size_t>(local)]];
        const int ix = static_cast<int>(std::floor(position.x / cell));
        const int iy = static_cast<int>(std::floor(position.y / cell));
        const int iz = static_cast<int>(std::floor(position.z / cell));
        double best = 1.0e30;
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const auto [begin, end] = range(pack(ix + dx, iy + dy, iz + dz));
                    for (auto node = begin; node != end; ++node) {
                        if (node->local == local) {
                            continue;
                        }
                        const physics::Vec3& other = positions[indices[static_cast<std::size_t>(node->local)]];
                        const double dxp = other.x - position.x;
                        const double dyp = other.y - position.y;
                        const double dzp = other.z - position.z;
                        best = std::min(best, dxp * dxp + dyp * dyp + dzp * dzp);
                    }
                }
            }
        }
        spacing[static_cast<std::size_t>(local)] =
            best < 1.0e29 ? static_cast<float>(std::sqrt(best)) : cell * 2.0f;
    }
}

void draw_volume_grid(SceneGpu& gpu, ViewState const& view, float const* density, float const* energy, int n,
                      physics::Vec3 origin, physics::Vec3 extent, bool repeat, int kind, double stamp, float extinction,
                      physics::Vec3 eye, int width, int height) {
    const bool cached = gpu.volume_texture != 0 && gpu.volume_n == n && gpu.volume_time == stamp &&
                        gpu.volume_epoch == view.recording_epoch && gpu.volume_kind == kind;
    if ((!cached && (density == nullptr || energy == nullptr)) || n <= 1 || !(extent.x > 0.0) || !(extent.y > 0.0) ||
        !(extent.z > 0.0)) {
        return;
    }
    const std::size_t count = static_cast<std::size_t>(n) * static_cast<std::size_t>(n) * static_cast<std::size_t>(n);
    if (!cached) {
        std::vector<float> pixels(count * 4);
        for (std::size_t index = 0; index < count; ++index) {
            pixels[index * 4] = density[index];
            pixels[index * 4 + 1] = energy[index];
        }
        if (gpu.volume_texture == 0) {
            glGenTextures(1, &gpu.volume_texture);
        }
        const GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
        glBindTexture(GL_TEXTURE_3D, gpu.volume_texture);
        gl_tex_image_3d(GL_TEXTURE_3D, 0, GL_RGBA16F, n, n, n, 0, GL_RGBA, GL_FLOAT, pixels.data());
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);
        gpu.volume_n = n;
        gpu.volume_time = stamp;
        gpu.volume_epoch = view.recording_epoch;
        gpu.volume_kind = kind;
        gpu.volume_resolution = view.field_resolution;
        gpu.volume_origin = origin;
        gpu.volume_extent = extent;
        std::vector<float> occupied;
        occupied.reserve(count / 8);
        float floor_energy = 0.0f;
        bool have_floor = false;
        for (std::size_t index = 0; index < count; ++index) {
            if (!(density[index] > 0.0f)) {
                continue;
            }
            occupied.push_back(density[index]);
            if (!have_floor || energy[index] < floor_energy) {
                floor_energy = energy[index];
                have_floor = true;
            }
        }
        if (occupied.empty()) {
            gpu.volume_time = -1.0;
            return;
        }
        const std::size_t mark = std::min(occupied.size() - 1, occupied.size() * 60 / 100);
        std::nth_element(occupied.begin(), occupied.begin() + static_cast<std::ptrdiff_t>(mark), occupied.end());
        gpu.volume_mean = std::max(occupied[mark], 1e-6f);
        gpu.volume_floor = std::max(floor_energy, 1e-8f);
    }
    const float mean = gpu.volume_mean;
    const physics::Vec3 forward = physics::normalized(view.camera.target - eye);
    const physics::Vec3 right = view.camera.right();
    const physics::Vec3 up = view.camera.up();
    const double aspect = static_cast<double>(width) / static_cast<double>(std::max(height, 1));
    const float tan_half = static_cast<float>(std::tan(22.5 * kPi / 180.0));
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    gl_use_program(gpu.volume_program);
    gl_active_texture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, gpu.volume_texture);
    gl_uniform_1i(gpu.volume_field_location, 0);
    gl_uniform_3f(gpu.volume_eye_location, static_cast<float>(eye.x), static_cast<float>(eye.y), static_cast<float>(eye.z));
    gl_uniform_3f(gpu.volume_forward_location, static_cast<float>(forward.x), static_cast<float>(forward.y),
                  static_cast<float>(forward.z));
    gl_uniform_3f(gpu.volume_right_location, static_cast<float>(right.x), static_cast<float>(right.y), static_cast<float>(right.z));
    gl_uniform_3f(gpu.volume_up_location, static_cast<float>(up.x), static_cast<float>(up.y), static_cast<float>(up.z));
    gl_uniform_2f(gpu.volume_scale_location, tan_half * static_cast<float>(aspect), tan_half);
    gl_uniform_3f(gpu.volume_origin_location, static_cast<float>(origin.x), static_cast<float>(origin.y),
                  static_cast<float>(origin.z));
    gl_uniform_3f(gpu.volume_extent_location, static_cast<float>(extent.x), static_cast<float>(extent.y),
                  static_cast<float>(extent.z));
    gl_uniform_1f(gpu.volume_mean_location, mean);
    gl_uniform_1f(gpu.volume_floor_location, gpu.volume_floor);
    gl_uniform_1f(gpu.volume_gain_location, view.field_gain);
    gl_uniform_1f(gpu.volume_contrast_location, view.field_contrast);
    gl_uniform_1f(gpu.volume_heat_location, view.field_heat);
    gl_uniform_1f(gpu.volume_extinction_location, extinction);
    gl_bind_vertex_array(gpu.volume_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTexture(GL_TEXTURE_3D, 0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

struct ParticleFieldScale {
    int grid = 96;
    double neighbor_scale = 1.65;
    double min_cells = 3.0;
    double cap_fraction = 0.055;
    float extinction = 0.07f;
};

ParticleFieldScale particle_field_scale(int level) {
    static ParticleFieldScale const scales[] = {
        {48, 2.6, 6.5, 0.14, 0.10f},
        {64, 2.0, 4.5, 0.09, 0.085f},
        {96, 1.65, 3.0, 0.055, 0.07f},
        {128, 1.35, 2.0, 0.035, 0.06f},
    };
    const int index = std::clamp(level, 0, 3);
    return scales[index];
}

// The continuous picture of a particle galaxy. Each body fills the volume out to
// its neighbor, and the ray march draws that one field.
bool draw_particle_volume(SceneGpu& gpu, ViewState const& view, std::vector<physics::Vec3> const& positions,
                          physics::Sample const* shown, double stamp, physics::Vec3 eye, int width, int height) {
    const ParticleFieldScale scale = particle_field_scale(view.field_resolution);
    const int kGrid = scale.grid;
    const std::vector<physics::Vec3>& cloud =
        shown != nullptr && shown->position.size() == positions.size() ? shown->position : positions;
    const double cloud_stamp = shown != nullptr ? shown->time : stamp;
    if (gpu.volume_texture != 0 && gpu.volume_n == kGrid && gpu.volume_kind == 2 && gpu.volume_resolution == view.field_resolution &&
        gpu.volume_time == cloud_stamp && gpu.volume_epoch == view.recording_epoch) {
        draw_volume_grid(gpu, view, nullptr, nullptr, kGrid, gpu.volume_origin, gpu.volume_extent, false, 2, cloud_stamp,
                         scale.extinction, eye, width, height);
        return true;
    }
    struct Contributor {
        std::size_t body = 0;
        float weight = 1.0f;
        float heat = 1.0f;
        float cell = 0.4f;
    };
    std::vector<Contributor> contributors;
    contributors.reserve(positions.size());
    auto named = [](std::string const& name, char const* token) { return name.find(token) != std::string::npos; };
    for (std::size_t body = 0; body < cloud.size(); ++body) {
        const bool has_component = shown != nullptr && body < shown->component.size();
        const auto component = has_component ? shown->component[body] : static_cast<std::uint8_t>(physics::Component::Body);
        const std::string& name = view.trajectory.bodies[body].name;
        Contributor item;
        item.body = body;
        if (component == static_cast<std::uint8_t>(physics::Component::Gas) || (!has_component && named(name, ".gas."))) {
            item.weight = 1.8f;
            item.heat = 4.0f;
            item.cell = 0.35f;
        } else if (component == static_cast<std::uint8_t>(physics::Component::HotGas) || named(name, ".hot.")) {
            item.weight = 0.85f;
            item.heat = 1.5f;
            item.cell = 1.5f;
        } else if (component == static_cast<std::uint8_t>(physics::Component::YoungStar)) {
            item.weight = 1.4f;
            item.heat = 6.0f;
            item.cell = 0.3f;
        } else if (component == static_cast<std::uint8_t>(physics::Component::Bulge) || named(name, ".bulge.")) {
            item.weight = 1.5f;
            item.heat = 4.2f;
            item.cell = 0.3f;
        } else if (component == static_cast<std::uint8_t>(physics::Component::Halo) || named(name, ".halo.")) {
            item.weight = 0.07f;
            item.heat = 0.4f;
            item.cell = 1.2f;
        } else if (component == static_cast<std::uint8_t>(physics::Component::Disk) || named(name, ".disk.") ||
                   view.trajectory.bodies[body].emission > 0.0) {
            item.weight = 1.0f;
            item.heat = 2.3f;
            item.cell = 0.4f;
        } else {
            continue;
        }
        contributors.push_back(item);
    }
    if (contributors.size() < 8) {
        return false;
    }
    physics::Vec3 center{};
    for (Contributor const& item : contributors) {
        center += cloud[item.body];
    }
    center = center / static_cast<double>(contributors.size());
    double max_radius = 0.0;
    for (Contributor const& item : contributors) {
        max_radius = std::max(max_radius, physics::norm(cloud[item.body] - center));
    }
    if (!(max_radius > 0.0)) {
        return false;
    }
    constexpr int kBins = 64;
    std::vector<int> counts(kBins, 0);
    for (Contributor const& item : contributors) {
        const double unit = physics::norm(cloud[item.body] - center) / max_radius;
        const int bin = std::min(kBins - 1, static_cast<int>(unit * kBins));
        ++counts[static_cast<std::size_t>(std::max(bin, 0))];
    }
    const int keep = std::max(1, static_cast<int>(contributors.size() * 95 / 100));
    int running = 0;
    int chosen = kBins - 1;
    for (int bin = 0; bin < kBins; ++bin) {
        running += counts[static_cast<std::size_t>(bin)];
        if (running >= keep) {
            chosen = bin;
            break;
        }
    }
    double limit = max_radius * (static_cast<double>(chosen) + 1.0) / kBins;
    if (!(max_radius > limit * 4.0)) {
        limit = max_radius;
    }
    bool any = false;
    physics::Vec3 min_corner{};
    physics::Vec3 max_corner{};
    for (Contributor const& item : contributors) {
        if (physics::norm(cloud[item.body] - center) > limit) {
            continue;
        }
        const physics::Vec3& position = cloud[item.body];
        if (!any) {
            min_corner = max_corner = position;
            any = true;
        } else {
            min_corner.x = std::min(min_corner.x, position.x);
            min_corner.y = std::min(min_corner.y, position.y);
            min_corner.z = std::min(min_corner.z, position.z);
            max_corner.x = std::max(max_corner.x, position.x);
            max_corner.y = std::max(max_corner.y, position.y);
            max_corner.z = std::max(max_corner.z, position.z);
        }
    }
    if (!any) {
        return false;
    }
    const physics::Vec3 middle = (min_corner + max_corner) * 0.5;
    double side = std::max(max_corner.x - min_corner.x, std::max(max_corner.y - min_corner.y, max_corner.z - min_corner.z));
    side = std::max(side * 1.2, 1e-3);
    const physics::Vec3 origin = middle - physics::Vec3{side, side, side} * 0.5;
    const physics::Vec3 extent{side, side, side};

    std::vector<std::size_t> gas_ids;
    std::vector<std::size_t> hot_ids;
    std::vector<std::size_t> star_ids;
    std::vector<std::size_t> halo_ids;
    for (Contributor const& item : contributors) {
        if (item.cell > 1.0f && item.heat < 1.0f) {
            halo_ids.push_back(item.body);
        } else if (item.heat >= 3.5f && item.weight > 1.6f) {
            gas_ids.push_back(item.body);
        } else if (item.cell > 1.0f) {
            hot_ids.push_back(item.body);
        } else {
            star_ids.push_back(item.body);
        }
    }
    const double dx = side / static_cast<double>(kGrid);
    std::vector<float> gap(cloud.size(), static_cast<float>(side * 0.1));
    const double hash_cell = std::max(side / 16.0, dx);
    auto pack_cell = [](int ix, int iy, int iz) {
        return (static_cast<std::int64_t>(ix + 200000) << 42) | (static_cast<std::int64_t>(iy + 200000) << 21) |
               static_cast<std::int64_t>(iz + 200000);
    };
    auto fill_gap = [&](std::vector<std::size_t> const& ids) {
        if (ids.size() < 2) {
            return;
        }
        struct Node {
            std::int64_t cell = 0;
            int local = 0;
        };
        std::vector<Node> nodes(ids.size());
        for (int local = 0; local < static_cast<int>(ids.size()); ++local) {
            const physics::Vec3& position = cloud[ids[static_cast<std::size_t>(local)]];
            nodes[static_cast<std::size_t>(local)] = {
                pack_cell(static_cast<int>(std::floor(position.x / hash_cell)),
                          static_cast<int>(std::floor(position.y / hash_cell)),
                          static_cast<int>(std::floor(position.z / hash_cell))),
                local};
        }
        std::sort(nodes.begin(), nodes.end(), [](Node const& a, Node const& b) { return a.cell < b.cell; });
        auto range = [&](std::int64_t id) {
            const auto lower = std::lower_bound(nodes.begin(), nodes.end(), id,
                                                [](Node const& node, std::int64_t value) { return node.cell < value; });
            auto upper = lower;
            while (upper != nodes.end() && upper->cell == id) {
                ++upper;
            }
            return std::pair<std::vector<Node>::const_iterator, std::vector<Node>::const_iterator>{lower, upper};
        };
        std::vector<int> missed;
        for (int local = 0; local < static_cast<int>(ids.size()); ++local) {
            const physics::Vec3& position = cloud[ids[static_cast<std::size_t>(local)]];
            const int ix = static_cast<int>(std::floor(position.x / hash_cell));
            const int iy = static_cast<int>(std::floor(position.y / hash_cell));
            const int iz = static_cast<int>(std::floor(position.z / hash_cell));
            double best = 1.0e30;
            for (int dz = -1; dz <= 1; ++dz) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx_cell = -1; dx_cell <= 1; ++dx_cell) {
                        const auto [begin, end] = range(pack_cell(ix + dx_cell, iy + dy, iz + dz));
                        for (auto node = begin; node != end; ++node) {
                            if (node->local == local) {
                                continue;
                            }
                            const physics::Vec3& other = cloud[ids[static_cast<std::size_t>(node->local)]];
                            const double dxp = other.x - position.x;
                            const double dyp = other.y - position.y;
                            const double dzp = other.z - position.z;
                            best = std::min(best, dxp * dxp + dyp * dyp + dzp * dzp);
                        }
                    }
                }
            }
            if (best < 1.0e29) {
                gap[ids[static_cast<std::size_t>(local)]] = static_cast<float>(std::sqrt(best));
            } else {
                missed.push_back(local);
            }
        }
        for (int local : missed) {
            const physics::Vec3& position = cloud[ids[static_cast<std::size_t>(local)]];
            double best = 1.0e30;
            for (std::size_t other_index = 0; other_index < ids.size(); ++other_index) {
                if (static_cast<int>(other_index) == local) {
                    continue;
                }
                const physics::Vec3& other = cloud[ids[other_index]];
                const double dxp = other.x - position.x;
                const double dyp = other.y - position.y;
                const double dzp = other.z - position.z;
                best = std::min(best, dxp * dxp + dyp * dyp + dzp * dzp);
            }
            if (best < 1.0e29) {
                gap[ids[static_cast<std::size_t>(local)]] = static_cast<float>(std::sqrt(best));
            }
        }
    };
    fill_gap(gas_ids);
    fill_gap(hot_ids);
    fill_gap(star_ids);
    fill_gap(halo_ids);

    const std::size_t cells = static_cast<std::size_t>(kGrid) * kGrid * kGrid;
    int worker_count = 1;
#ifdef _OPENMP
    worker_count = std::min(8, std::max(1, omp_get_max_threads()));
#endif
    std::vector<std::vector<float>> density_workers(static_cast<std::size_t>(worker_count), std::vector<float>(cells, 0.0f));
    std::vector<std::vector<float>> heat_workers(static_cast<std::size_t>(worker_count), std::vector<float>(cells, 0.0f));
    const int contributor_count = static_cast<int>(contributors.size());
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 8) num_threads(worker_count)
#endif
    for (int item_index = 0; item_index < contributor_count; ++item_index) {
        const Contributor& item = contributors[static_cast<std::size_t>(item_index)];
        const physics::Vec3& position = cloud[item.body];
        if (position.x < origin.x || position.y < origin.y || position.z < origin.z || position.x > origin.x + side ||
            position.y > origin.y + side || position.z > origin.z + side) {
            continue;
        }
        const double reach =
            std::min(std::max(static_cast<double>(gap[item.body]) * scale.neighbor_scale, dx * scale.min_cells),
                     side * scale.cap_fraction);
        const int i0 = std::max(0, static_cast<int>(std::floor((position.x - origin.x - reach) / dx)));
        const int j0 = std::max(0, static_cast<int>(std::floor((position.y - origin.y - reach) / dx)));
        const int k0 = std::max(0, static_cast<int>(std::floor((position.z - origin.z - reach) / dx)));
        const int i1 = std::min(kGrid - 1, static_cast<int>(std::ceil((position.x - origin.x + reach) / dx)));
        const int j1 = std::min(kGrid - 1, static_cast<int>(std::ceil((position.y - origin.y + reach) / dx)));
        const int k1 = std::min(kGrid - 1, static_cast<int>(std::ceil((position.z - origin.z + reach) / dx)));
        int worker = 0;
#ifdef _OPENMP
        worker = omp_get_thread_num();
#endif
        std::vector<float>& density_local = density_workers[static_cast<std::size_t>(worker)];
        std::vector<float>& heat_local = heat_workers[static_cast<std::size_t>(worker)];
        for (int k = k0; k <= k1; ++k) {
            for (int j = j0; j <= j1; ++j) {
                for (int i = i0; i <= i1; ++i) {
                    const double px = origin.x + (static_cast<double>(i) + 0.5) * dx;
                    const double py = origin.y + (static_cast<double>(j) + 0.5) * dx;
                    const double pz = origin.z + (static_cast<double>(k) + 0.5) * dx;
                    const double distance = std::sqrt((px - position.x) * (px - position.x) + (py - position.y) * (py - position.y) +
                                                      (pz - position.z) * (pz - position.z));
                    const double unit = distance / reach;
                    if (unit >= 1.0) {
                        continue;
                    }
                    const float kernel = static_cast<float>((1.0 - unit * unit) * (1.0 - unit * unit) * item.weight);
                    const std::size_t index = static_cast<std::size_t>(i + kGrid * (j + kGrid * k));
                    density_local[index] += kernel;
                    heat_local[index] += kernel * item.heat;
                }
            }
        }
    }
    std::vector<float> density(cells, 0.0f);
    std::vector<float> heat_mass(cells, 0.0f);
    for (int worker = 0; worker < worker_count; ++worker) {
        std::vector<float> const& density_local = density_workers[static_cast<std::size_t>(worker)];
        std::vector<float> const& heat_local = heat_workers[static_cast<std::size_t>(worker)];
        for (std::size_t index = 0; index < cells; ++index) {
            density[index] += density_local[index];
            heat_mass[index] += heat_local[index];
        }
    }
    std::vector<float> energy(cells, 0.0f);
    for (std::size_t index = 0; index < cells; ++index) {
        if (density[index] > 0.0f) {
            energy[index] = heat_mass[index] / density[index];
        }
    }
    draw_volume_grid(gpu, view, density.data(), energy.data(), kGrid, origin, extent, false, 2, cloud_stamp, scale.extinction,
                     eye, width, height);
    return true;
}

// Back-to-front, wavelength-dependent dust and normalized stellar emission in HDR.
bool draw_optical(SceneGpu &gpu, ViewState const &view, std::vector<physics::Vec3> const &positions,
                  physics::Sample const *shown, double time, Mat4 const &matrix, int width, int height,
                  GLuint framebuffer) {
    if (!ensure_field_targets(gpu, width, height))
        return false;
    if (!gpu.optical_program) {
        gpu.optical_program = link_program(kOpticalVertex, kOpticalFragment);
        gpu.optical_post = link_program(kCompositeVertex, kOpticalPost);
        gl_gen_vertex_arrays(1, &gpu.optical_vao);
        gl_gen_buffers(1, &gpu.optical_buffer);
        gl_bind_vertex_array(gpu.optical_vao);
        gl_bind_buffer(GL_ARRAY_BUFFER, gpu.optical_buffer);
        int sizes[] = {3, 2, 3, 2};
        int offset = 0;
        for (GLuint i = 0; i < 4; ++i) {
            gl_vertex_attrib_pointer(i, sizes[i], GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                                     reinterpret_cast<void *>(static_cast<std::size_t>(offset) * sizeof(float)));
            gl_enable_vertex_attrib_array(i);
            offset += sizes[i];
        }
    }
    const auto right = view.camera.right(), up = view.camera.up(), eye = view.camera.eye();
    const auto forward = physics::normalized(view.camera.target - eye);
    physics::visual::OpticalSettings settings;
    settings.smoothing = view.optical_smoothing;
    settings.dust = view.dust;
    settings.dust_structure = view.dust_structure;
    settings.gas_glow = view.gas_glow;
    settings.lifetime = view.young_lifetime;
    settings.ionizing_lifetime = view.ionizing_lifetime;
    settings.oiii_lifetime = view.oiii_lifetime;
    settings.young_boost = view.young_boost;
    settings.association_size = view.association_size;
    settings.nebula_radius = view.nebula_radius;
    settings.ionizing_mass_scale = view.ionizing_mass_scale;
    settings.nebula_density_scale = view.nebula_density_scale;
    settings.halpha_strength = view.halpha_strength;
    settings.oiii_strength = view.oiii_strength;
    settings.nebula_size = view.nebula_size;
    settings.halpha_color = {view.halpha_color[0], view.halpha_color[1], view.halpha_color[2]};
    settings.oiii_color = {view.oiii_color[0], view.oiii_color[1], view.oiii_color[2]};
    settings.texture = view.stellar_texture;
    settings.detail = view.stellar_detail;
    settings.arm_definition = view.arm_definition;
    settings.show_halo = view.show_halo;
    settings.show_hot_gas = view.show_hot_gas;
    const auto splats =
        physics::visual::make_splats(view.trajectory, shown, positions, time, eye, right, up, forward, settings);
    std::vector<float> vertices;
    vertices.reserve(splats.size() * 60);
    constexpr float corners[6][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1}};
    for (auto const &splat : splats) {
        // A minimum pixel footprint suppresses twinkling of unresolved concentrations.
        const double pixel = std::max(0.0, splat.depth) * 2 * std::tan(22.5 * kPi / 180.0) / height;
        const double major = std::max(splat.major, 0.75 * pixel), minor = std::max(splat.minor, 0.75 * pixel);
        const double normalization = 3.0 / (kPi * major * minor);
        for (auto const &uv : corners) {
            const auto point = splat.position + right * (uv[0] * major * splat.cosine - uv[1] * minor * splat.sine) +
                               up * (uv[0] * major * splat.sine + uv[1] * minor * splat.cosine);
            const float vertex[] = {static_cast<float>(point.x),
                                    static_cast<float>(point.y),
                                    static_cast<float>(point.z),
                                    uv[0],
                                    uv[1],
                                    static_cast<float>(splat.color.x),
                                    static_cast<float>(splat.color.y),
                                    static_cast<float>(splat.color.z),
                                    static_cast<float>(splat.luminosity * normalization),
                                    static_cast<float>(splat.optical_depth * normalization)};
            vertices.insert(vertices.end(), std::begin(vertex), std::end(vertex));
        }
    }
    gl_bind_framebuffer(GL_FRAMEBUFFER, gpu.light_fbo);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    gl_use_program(gpu.optical_program);
    gl_uniform_matrix4_fv(gl_get_uniform_location(gpu.optical_program, "u_view_projection"), 1, GL_FALSE, matrix.m);
    gl_uniform_1f(gl_get_uniform_location(gpu.optical_program, "u_reddening"), view.reddening);
    const GLint dust_pass = gl_get_uniform_location(gpu.optical_program, "u_dust_pass");
    gl_bind_vertex_array(gpu.optical_vao);
    gl_bind_buffer(GL_ARRAY_BUFFER, gpu.optical_buffer);
    gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(vertices.size() * sizeof(float)), vertices.data(),
                   GL_DYNAMIC_DRAW);
    for (std::size_t i = 0; i < splats.size();) {
        if (splats[i].optical_depth > 0) {
            glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            gl_uniform_1i(dust_pass, 1);
            glDrawArrays(GL_TRIANGLES, static_cast<GLint>(i * 6), 6);
        }
        const std::size_t begin = i++;
        while (i < splats.size() && splats[i].optical_depth <= 0)
            ++i;
        glBlendFunc(GL_ONE, GL_ONE);
        gl_uniform_1i(dust_pass, 0);
        glDrawArrays(GL_TRIANGLES, static_cast<GLint>(begin * 6), static_cast<GLsizei>((i - begin) * 6));
    }
    glDisable(GL_BLEND);
    auto post = [&](GLuint target, bool extract) {
        gl_bind_framebuffer(GL_FRAMEBUFFER, target);
        glViewport(0, 0, width, height);
        gl_use_program(gpu.optical_post);
        gl_uniform_1i(gl_get_uniform_location(gpu.optical_post, "u_light"), 0);
        gl_uniform_1i(gl_get_uniform_location(gpu.optical_post, "u_bloom"), 1);
        gl_uniform_1f(gl_get_uniform_location(gpu.optical_post, "u_exposure"), view.exposure);
        gl_uniform_1f(gl_get_uniform_location(gpu.optical_post, "u_bloom_gain"), view.bloom);
        gl_uniform_1i(gl_get_uniform_location(gpu.optical_post, "u_extract"), extract ? 1 : 0);
        gl_active_texture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gpu.light_tex);
        gl_active_texture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, extract ? 0 : gpu.gas_tex);
        gl_bind_vertex_array(gpu.field_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    };
    post(gpu.body_fbo, true);
    // Bloom width scales with output resolution so exports match the viewport.
    blur_separable(gpu, gpu.body_tex, gpu.gas_fbo, gpu.gas_tex, 3.0f * height / 1080.0f, 1);
    post(framebuffer, false);
    gl_active_texture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    gl_active_texture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    return true;
}

void draw_scene(SceneGpu& gpu, ViewState const& view, std::vector<physics::Vec3> const& positions, double shown_time,
                int width, int height, GLuint framebuffer) {
    gl_bind_framebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
    glClearColor(0.012f, 0.014f, 0.018f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (positions.size() != view.trajectory.bodies.size() || width <= 0 || height <= 0) {
        return;
    }
    const physics::Sample* shown = nullptr;
    for (physics::Sample const& sample : view.trajectory.samples) {
        if (sample.time > shown_time) {
            break;
        }
        shown = &sample;
    }
    auto look_of = [&](std::size_t body) {
        const physics::BodyRecord& record = view.trajectory.bodies[body];
        physics::Vec3 color = record.color;
        double emission = record.emission;
        if (shown != nullptr && body < shown->component.size() &&
            shown->component[body] == static_cast<std::uint8_t>(physics::Component::YoungStar) &&
            record.formed_star_emission > 0.0) {
            color = record.formed_star_color;
            emission = record.formed_star_emission;
        }
        return std::pair<physics::Vec3, double>{color, emission};
    };

    const double aspect = static_cast<double>(width) / static_cast<double>(height);
    const double near_plane = std::max(view.camera.distance * 1e-3, 1e-4);
    const double far_plane = view.camera.distance + view.scene_radius * 20.0;
    const Mat4 projection = perspective(45.0 * kPi / 180.0, aspect, near_plane, far_plane);
    const physics::Vec3 eye = view.camera.eye();
    const Mat4 view_matrix = look_at(eye, view.camera.target, physics::Vec3{0, 0, 1});
    const Mat4 view_projection = multiply(projection, view_matrix);

    bool continuous = false;
    if (view.picture_mode == 1 && shown != nullptr) {
        continuous = draw_particle_volume(gpu, view, positions, shown, shown_time, eye, width, height);
    }

    if (!continuous && view.picture_mode == 2 && shown != nullptr) {
        continuous = draw_optical(gpu,view,positions,shown,shown_time,view_projection,width,height,framebuffer);
    }
    if (view.show_axes) {
        gl_use_program(gpu.color_program);
        gl_uniform_matrix4_fv(gpu.view_projection_location, 1, GL_FALSE, view_projection.m);
        const float axis_length = static_cast<float>(view.scene_radius * 0.45);
        const float axes[] = {
            0, 0, 0, 0.85f, 0.25f, 0.22f, 1, axis_length, 0, 0, 0.85f, 0.25f, 0.22f, 1,
            0, 0, 0, 0.30f, 0.75f, 0.35f, 1, 0, axis_length, 0, 0.30f, 0.75f, 0.35f, 1,
            0, 0, 0, 0.30f, 0.55f, 0.90f, 1, 0, 0, axis_length, 0.30f, 0.55f, 0.90f, 1,
        };
        gl_bind_vertex_array(gpu.color_vao);
        gl_bind_buffer(GL_ARRAY_BUFFER, gpu.color_buffer);
        gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(sizeof(axes)), axes, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, 6);
    }

    gl_use_program(gpu.lit_program);
    gl_uniform_3f(gpu.key_dir_location, 0.45f, 0.25f, 0.85f);
    gl_uniform_3f(gpu.fill_dir_location, -0.65f, -0.20f, 0.35f);
    gl_uniform_1f(gpu.key_intensity_location, view.key_intensity);
    gl_uniform_1f(gpu.fill_intensity_location, view.fill_intensity);
    gl_uniform_3f(gpu.eye_location, static_cast<float>(eye.x), static_cast<float>(eye.y), static_cast<float>(eye.z));
    gl_bind_vertex_array(gpu.sphere_vao);
    for (std::size_t body = 0; body < view.trajectory.bodies.size(); ++body) {
        const auto look = look_of(body);
        if (view.picture_mode == 2 && physics::visual::component_of(view.trajectory.bodies[body], shown, body) !=
                                         physics::Component::Body) continue;
        if (look.second > 0.0) {
            continue;
        }
        const Mat4 model = translation_scale(positions[body], view.trajectory.bodies[body].display_radius *
                                                                  static_cast<double>(view.point_scale));
        const Mat4 mvp = multiply(view_projection, model);
        gl_uniform_matrix4_fv(gpu.model_location, 1, GL_FALSE, model.m);
        gl_uniform_matrix4_fv(gpu.mvp_location, 1, GL_FALSE, mvp.m);
        const physics::Vec3 color = look.first;
        gl_uniform_3f(gpu.color_location, static_cast<float>(color.x), static_cast<float>(color.y),
                      static_cast<float>(color.z));
        gl_uniform_1f(gpu.roughness_location, static_cast<float>(view.trajectory.bodies[body].roughness));
        glDrawElements(GL_TRIANGLES, gpu.sphere_indices, GL_UNSIGNED_INT, nullptr);
    }

    const double history_start =
        view.time_begin + (1.0 - static_cast<double>(view.trail_fraction)) * (shown_time - view.time_begin);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    const physics::Vec3 right = view.camera.right();
    const double head_width = view.scene_radius * 0.012;
    gl_use_program(gpu.color_program);
    gl_uniform_matrix4_fv(gpu.view_projection_location, 1, GL_FALSE, view_projection.m);
    gl_bind_vertex_array(gpu.color_vao);
    for (std::size_t body = 0; body < view.trajectory.bodies.size(); ++body) {
        const auto trail_look = look_of(body);
        if (view.picture_mode == 2 && physics::visual::component_of(view.trajectory.bodies[body], shown, body) !=
                                         physics::Component::Body) continue;
        if (trail_look.second > 0.0) {
            continue;
        }
        std::vector<float> strip;
        std::vector<physics::Vec3> trail_points;
        for (physics::Sample const& sample : view.trajectory.samples) {
            if (sample.time < history_start || sample.time > shown_time) {
                continue;
            }
            if (trail_points.empty() ||
                physics::norm(sample.position[body] - trail_points.back()) > view.scene_radius * 1e-4) {
                trail_points.push_back(sample.position[body]);
            }
        }
        if (trail_points.empty() || physics::norm(positions[body] - trail_points.back()) > view.scene_radius * 1e-5) {
            trail_points.push_back(positions[body]);
        }
        if (trail_points.size() > 800) {
            const std::size_t stride = trail_points.size() / 800;
            std::vector<physics::Vec3> reduced;
            for (std::size_t i = 0; i < trail_points.size(); i += stride) {
                reduced.push_back(trail_points[i]);
            }
            reduced.push_back(trail_points.back());
            trail_points.swap(reduced);
        }
        if (trail_points.size() < 2) {
            continue;
        }
        const physics::Vec3 color = trail_look.first;
        for (std::size_t i = 0; i < trail_points.size(); ++i) {
            const float age = static_cast<float>(i) / static_cast<float>(trail_points.size() - 1);
            const physics::Vec3 side = right * (head_width * static_cast<double>(age));
            const physics::Vec3 a = trail_points[i] + side;
            const physics::Vec3 b = trail_points[i] - side;
            const float alpha = 0.15f + 0.85f * age;
            const float values[] = {static_cast<float>(a.x),
                                    static_cast<float>(a.y),
                                    static_cast<float>(a.z),
                                    static_cast<float>(color.x),
                                    static_cast<float>(color.y),
                                    static_cast<float>(color.z),
                                    alpha,
                                    static_cast<float>(b.x),
                                    static_cast<float>(b.y),
                                    static_cast<float>(b.z),
                                    static_cast<float>(color.x),
                                    static_cast<float>(color.y),
                                    static_cast<float>(color.z),
                                    alpha};
            strip.insert(strip.end(), std::begin(values), std::end(values));
        }
        gl_bind_buffer(GL_ARRAY_BUFFER, gpu.color_buffer);
        gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(strip.size() * sizeof(float)), strip.data(),
                       GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(trail_points.size() * 2));
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);

    if (continuous) {
        return;
    }
    const bool smooth = view.light_smoothing > 0.05f && gpu.field_program != 0 &&
                        ensure_field_targets(gpu, width, height);
    std::vector<std::size_t> star_ids;
    std::vector<std::size_t> gas_ids;
    struct GlowParticle {
        std::size_t body = 0;
        bool gas = false;
        bool young = false;
    };
    std::vector<GlowParticle> glows;
    glows.reserve(view.trajectory.bodies.size());
    for (std::size_t body = 0; body < view.trajectory.bodies.size(); ++body) {
        if (!(look_of(body).second > 0.0)) {
            continue;
        }
        const bool has_component = shown != nullptr && body < shown->component.size();
        const bool named_gas = view.trajectory.bodies[body].name.find(".gas.") != std::string::npos;
        GlowParticle glow;
        glow.body = body;
        glow.gas = has_component ? shown->component[body] == static_cast<std::uint8_t>(physics::Component::Gas) : named_gas;
        glow.young = has_component && shown->component[body] == static_cast<std::uint8_t>(physics::Component::YoungStar);
        if (glow.gas) {
            gas_ids.push_back(body);
        } else {
            star_ids.push_back(body);
        }
        glows.push_back(glow);
    }
    std::vector<float> star_gap;
    std::vector<float> gas_gap;
    if (smooth) {
        nearest_spacing(positions, star_ids, star_gap, 0.4f);
        nearest_spacing(positions, gas_ids, gas_gap, 0.4f);
    }
    std::vector<float> star_spacing(view.trajectory.bodies.size(), 0.0f);
    std::vector<float> gas_spacing(view.trajectory.bodies.size(), 0.0f);
    for (std::size_t index = 0; index < star_ids.size() && index < star_gap.size(); ++index) {
        star_spacing[star_ids[index]] = star_gap[index];
    }
    for (std::size_t index = 0; index < gas_ids.size() && index < gas_gap.size(); ++index) {
        gas_spacing[gas_ids[index]] = gas_gap[index];
    }
    std::vector<float> stars;
    std::vector<float> young;
    std::vector<float> gas;
    auto push_glow = [&](std::vector<float>& destination, std::size_t body, float kernel, float weight, physics::Vec3 color) {
        const physics::Vec3 position = positions[body];
        const float values[] = {static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z),
                                static_cast<float>(color.x),    static_cast<float>(color.y),    static_cast<float>(color.z),
                                kernel,                         weight};
        destination.insert(destination.end(), std::begin(values), std::end(values));
    };
    for (GlowParticle const& glow : glows) {
        const auto look = look_of(glow.body);
        const float display = static_cast<float>(view.trajectory.bodies[glow.body].display_radius *
                                                  static_cast<double>(view.point_scale));
        float kernel = display;
        if (smooth) {
            const float gap = glow.gas ? gas_spacing[glow.body] : star_spacing[glow.body];
            const float reach = (gap > 0.0f ? gap : 0.45f) * view.light_smoothing * view.point_scale;
            if (glow.young) {
                kernel = std::min(reach * 0.4f, display * 1.4f);
                kernel = std::clamp(kernel, display * 0.25f, 8.0f * view.point_scale);
            } else if (glow.gas) {
                // Diameter reaches the next gas particle, and no farther, so a real gap stays open.
                const float neighbor = gap > 0.0f ? gap : 0.45f;
                kernel = std::min(neighbor * 1.6f * view.point_scale, 4.0f * view.point_scale);
                kernel = std::max(kernel, display * 0.25f);
            } else {
                kernel = std::clamp(reach, display * 0.25f, 8.0f * view.point_scale);
            }
        }
        const float weight = smooth && glow.gas ? 1.0f : static_cast<float>(look.second);
        std::vector<float>& destination = !smooth ? stars : glow.gas ? gas : glow.young ? young : stars;
        push_glow(destination, glow.body, kernel, weight, look.first);
    }
    if (smooth && (!stars.empty() || !young.empty() || !gas.empty())) {
        constexpr float kMaxPoint = 256.0f;
        splat_field(gpu, stars, 0.55f, kMaxPoint, 0.0f, view_projection, height, gpu.light_fbo, true);
        const int body_passes = view.soften >= 6.0f ? 2 : 1;
        blur_separable(gpu, gpu.light_tex, gpu.body_fbo, gpu.body_tex, view.soften, body_passes);
        splat_field(gpu, gas, 1.0f, kMaxPoint, 1.0f, view_projection, height, gpu.gas_fbo, true);
        blur_separable(gpu, gpu.gas_tex, gpu.gas_fbo, gpu.gas_tex, 1.5f, 1);
        gl_bind_framebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, width, height);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        gl_use_program(gpu.composite_program);
        gl_active_texture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gpu.light_tex);
        gl_uniform_1i(gpu.composite_light_location, 0);
        gl_active_texture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gpu.body_tex);
        gl_uniform_1i(gpu.composite_body_location, 1);
        gl_active_texture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gpu.gas_tex);
        gl_uniform_1i(gpu.composite_gas_location, 2);
        gl_uniform_1f(gpu.composite_dust_location, view.dust);
        gl_uniform_1f(gpu.composite_reddening_location, view.reddening);
        gl_uniform_1f(gpu.composite_glow_location, view.gas_glow);
        gl_uniform_1f(gpu.composite_soften_brightness_location, view.soften_brightness);
        gl_bind_vertex_array(gpu.field_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        splat_field(gpu, young, 0.22f, kMaxPoint, 0.0f, view_projection, height, framebuffer, false);
        gl_active_texture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        gl_active_texture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        gl_active_texture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    } else if (!stars.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(GL_FALSE);
        gl_use_program(gpu.star_program);
        gl_uniform_matrix4_fv(gpu.star_view_location, 1, GL_FALSE, view_projection.m);
        gl_uniform_1f(gpu.star_height_location, static_cast<float>(height));
        gl_bind_vertex_array(gpu.star_vao);
        gl_bind_buffer(GL_ARRAY_BUFFER, gpu.star_buffer);
        gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(stars.size() * sizeof(float)), stars.data(),
                       GL_DYNAMIC_DRAW);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(stars.size() / 8));
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

void read_rgb_top_down(int width, int height, std::vector<unsigned char>& pixels) {
    const std::size_t stride = static_cast<std::size_t>(width) * 3;
    std::vector<unsigned char> raw(stride * static_cast<std::size_t>(height));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, raw.data());
    pixels.resize(raw.size());
    for (int row = 0; row < height; ++row) {
        const unsigned char* source = raw.data() + stride * static_cast<std::size_t>(height - 1 - row);
        std::copy(source, source + stride, pixels.data() + stride * static_cast<std::size_t>(row));
    }
}

int export_frame_count(double start, double end, int fps) {
    if (!(end > start) || fps < 1) {
        return 1;
    }
    return std::max(1, static_cast<int>(std::lround((end - start) * static_cast<double>(fps))));
}

std::string export_directory(std::string const& path) {
    std::filesystem::path file(path);
    if (file.extension() == ".png" || file.extension() == ".mp4") {
        file = file.parent_path();
    }
    if (file.empty()) {
        file = "render";
    }
    return file.string();
}

std::string export_movie_file(std::string const& path) {
    std::filesystem::path file(path);
    if (file.extension() == ".mp4") {
        return file.string();
    }
    if (file.extension().empty()) {
        return (file / "movie.mp4").string();
    }
    file.replace_extension(".mp4");
    return file.string();
}

std::string export_still_file(std::string const& path) {
    std::filesystem::path file(path);
    if (file.extension() == ".png") {
        return file.string();
    }
    if (file.extension() == ".mp4") {
        file.replace_extension(".png");
        return file.string();
    }
    return (file / "frame.png").string();
}

void finish_export(ViewState& view, bool completed) {
    view.export_ffmpeg.finish();
    view.export_active = false;
    view.export_stop = false;
    if (!view.export_error.empty()) {
        view.export_status.clear();
        view.export_resumable = false;
        return;
    }
    if (completed) {
        view.export_resumable = false;
        view.export_status = "Wrote " + std::to_string(view.export_frame) + " frames to " + view.export_run_path;
        return;
    }
    view.export_resumable = view.export_run_format == 0 && view.export_frame > 0 && view.export_frame < view.export_frame_count;
    view.export_status = "Stopped at frame " + std::to_string(view.export_frame) + " of " +
                         std::to_string(view.export_frame_count);
    if (view.export_run_format == 1) {
        view.export_status += ". The MP4 file holds those frames.";
    }
}

bool capture_rgb(SceneGpu& gpu, ViewState const& view, ExportTarget& target, std::vector<physics::Vec3> const& positions,
                 double shown_time, int width, int height, std::vector<unsigned char>& pixels, std::string& error) {
    if (!target.ensure(width, height, error)) {
        return false;
    }
    draw_scene(gpu, view, positions, shown_time, width, height, target.fbo);
    read_rgb_top_down(width, height, pixels);
    gl_bind_framebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void pump_export(ViewState& view, SceneGpu& gpu, ExportTarget& target, std::vector<unsigned char>& pixels) {
    if (!view.export_active) {
        return;
    }
    if (view.export_stop || view.export_frame >= view.export_frame_count) {
        finish_export(view, view.export_frame >= view.export_frame_count);
        return;
    }
    const double shown =
        view.export_run_start + static_cast<double>(view.export_frame) / static_cast<double>(view.export_run_fps);
    std::vector<physics::Vec3> positions;
    physics::Diagnostics diagnostics;
    sample_at(view.trajectory, shown, positions, diagnostics);
    if (!capture_rgb(gpu, view, target, positions, shown, view.export_run_width, view.export_run_height, pixels,
                     view.export_error)) {
        finish_export(view, false);
        return;
    }
    if (view.export_run_format == 0) {
        const std::string file = frame_png_path(view.export_run_path, view.export_frame);
        if (!write_png_rgb(file, view.export_run_width, view.export_run_height, pixels.data(), view.export_error)) {
            finish_export(view, false);
            return;
        }
    } else if (!view.export_ffmpeg.write_frame(pixels.data(), view.export_run_width, view.export_run_height,
                                               view.export_error)) {
        finish_export(view, false);
        return;
    }
    ++view.export_frame;
    view.playhead = shown;
    view.export_status = "Exporting frame " + std::to_string(view.export_frame) + " of " +
                         std::to_string(view.export_frame_count);
}

bool start_export(ViewState& view, bool resume) {
    if (view.trajectory.samples.empty()) {
        view.export_error = "Nothing to export yet.";
        return false;
    }
    view.export_error.clear();
    if (!resume) {
        view.export_width = std::clamp(view.export_width, 16, 8192);
        view.export_height = std::clamp(view.export_height, 16, 8192);
        view.export_fps = std::clamp(view.export_fps, 1, 120);
        if (!(view.export_end > view.export_start)) {
            view.export_error = "The export end time has to be after the start.";
            return false;
        }
        if (view.export_format == 1 && (view.export_width % 2 != 0 || view.export_height % 2 != 0)) {
            view.export_error = "MP4 needs an even width and height.";
            return false;
        }
        view.export_frame = 0;
        view.export_run_width = view.export_width;
        view.export_run_height = view.export_height;
        view.export_run_fps = view.export_fps;
        view.export_run_format = view.export_format;
        view.export_run_start = view.export_start;
        view.export_frame_count = export_frame_count(view.export_start, view.export_end, view.export_fps);
        view.export_run_path =
            view.export_format == 0 ? export_directory(view.export_path) : export_movie_file(view.export_path);
        if (view.export_format == 1 &&
            !view.export_ffmpeg.start(view.export_run_width, view.export_run_height, view.export_run_fps, view.export_run_path,
                                      view.export_error)) {
            return false;
        }
    }
    view.export_active = true;
    view.export_stop = false;
    view.export_resumable = false;
    view.playing = false;
    view.export_status = "Exporting frame 0 of " + std::to_string(view.export_frame_count);
    return true;
}

void invalidate_export_resume(ViewState& view) {
    if (!view.export_active) {
        view.export_resumable = false;
    }
}

void draw_export(ViewState& view) {
    // CollapsingHeader does not push an ID scope. Keep export controls separate
    // from the "Export" header and the simulation's "Continue" button.
    ImGui::PushID("export_controls");
    ImGui::TextUnformatted("Renders the view, without this panel. Camera, colors, lights, trails, and axes are the ones on screen.");
    if (!view.export_status.empty()) {
        ImGui::TextWrapped("%s", view.export_status.c_str());
    }
    if (!view.export_error.empty()) {
        ImGui::TextWrapped("%s", view.export_error.c_str());
    }
    const bool locked = view.export_active;
    if (view.export_active) {
        if (ImGui::Button("Stop export")) {
            view.export_stop = true;
        }
    } else {
        if (ImGui::Button("Export")) {
            start_export(view, false);
        }
        if (view.export_resumable) {
            ImGui::SameLine();
            if (ImGui::Button("Continue")) {
                start_export(view, true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save frame")) {
            view.export_still = true;
            view.export_error.clear();
        }
    }
    if (locked) {
        ImGui::BeginDisabled();
    }
    char preset[32] = "Custom";
    if (view.export_width == 1280 && view.export_height == 720) {
        std::snprintf(preset, sizeof(preset), "1280 x 720");
    } else if (view.export_width == 1920 && view.export_height == 1080) {
        std::snprintf(preset, sizeof(preset), "1920 x 1080");
    } else if (view.export_width == 2560 && view.export_height == 1440) {
        std::snprintf(preset, sizeof(preset), "2560 x 1440");
    } else if (view.export_width == 3840 && view.export_height == 2160) {
        std::snprintf(preset, sizeof(preset), "3840 x 2160");
    }
    if (ImGui::BeginCombo("Preset", preset)) {
        auto pick = [&](char const* label, int width, int height) {
            if (ImGui::Selectable(label)) {
                view.export_width = width;
                view.export_height = height;
                invalidate_export_resume(view);
            }
        };
        pick("1280 x 720", 1280, 720);
        pick("1920 x 1080", 1920, 1080);
        pick("2560 x 1440", 2560, 1440);
        pick("3840 x 2160", 3840, 2160);
        ImGui::EndCombo();
    }
    if (ImGui::DragInt("Width", &view.export_width, 1.0f, 16, 8192)) {
        invalidate_export_resume(view);
    }
    if (ImGui::DragInt("Height", &view.export_height, 1.0f, 16, 8192)) {
        invalidate_export_resume(view);
    }
    if (ImGui::DragInt("FPS", &view.export_fps, 0.2f, 1, 120)) {
        invalidate_export_resume(view);
    }
    const char* formats[] = {"PNG sequence", "MP4 (ffmpeg)"};
    if (ImGui::Combo("Format", &view.export_format, formats, 2)) {
        invalidate_export_resume(view);
    }
    if (ImGui::InputText("Output", view.export_path, sizeof(view.export_path))) {
        invalidate_export_resume(view);
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        std::string chosen;
        if (browse_export_path(view.export_format == 0, view.export_path, chosen)) {
            std::snprintf(view.export_path, sizeof(view.export_path), "%s", chosen.c_str());
            invalidate_export_resume(view);
        }
    }
    if (ImGui::DragScalar("From", ImGuiDataType_Double, &view.export_start, 0.01f, nullptr, nullptr, "%.5g")) {
        view.export_range_set = true;
        invalidate_export_resume(view);
    }
    if (ImGui::DragScalar("To", ImGuiDataType_Double, &view.export_end, 0.01f, nullptr, nullptr, "%.5g")) {
        view.export_range_set = true;
        invalidate_export_resume(view);
    }
    const int frames = export_frame_count(view.export_start, view.export_end, std::max(view.export_fps, 1));
    if (view.export_format == 0) {
        ImGui::Text("%d PNG frames in %s", frames, export_directory(view.export_path).c_str());
    } else {
        ImGui::Text("%d frames to %s", frames, export_movie_file(view.export_path).c_str());
    }
    ImGui::TextUnformatted("PNG sequence can continue after Stop. MP4 is closed as a finished file.");
    if (locked) {
        ImGui::EndDisabled();
    }
    ImGui::PopID();
}

struct BuiltinSimulation {
    char const* name;
    char const* file;
};

BuiltinSimulation const kSimulations[] = {
    {"Galaxy merger", "galaxy_luminous_merger.scene"},
    {"Seeded merger", "galaxy_spiral_merger.scene"},
    {"Retrograde merger", "galaxy_merger_retrograde.scene"},
};
constexpr int kSimulationCount = static_cast<int>(sizeof(kSimulations) / sizeof(kSimulations[0]));

std::filesystem::path program_directory() {
#if defined(_WIN32)
    char buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(buffer).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

std::filesystem::path examples_directory() {
    const std::filesystem::path program = program_directory();
    const std::filesystem::path candidates[] = {
        program / "examples",
        program / ".." / ".." / ".." / "examples",
        std::filesystem::current_path() / "examples",
    };
    for (std::filesystem::path const& candidate : candidates) {
        std::error_code code;
        if (std::filesystem::exists(candidate / "galaxy_luminous_merger.scene", code)) {
            return candidate;
        }
    }
    return {};
}

void clear_recording(ViewState& view) {
    ++view.recording_epoch;
    view.trajectory = {};
    view.color_groups.clear();
    view.playhead = 0;
    view.time_begin = 0;
    view.time_end = 0;
    view.resumable = false;
    view.steps_done = 0;
    view.export_range_set = false;
    view.export_resumable = false;
}

void load_scene_into_view(ViewState& view, physics::Scene scene, std::string name, int preset) {
    if (scene.galaxies.size() != 2)
        throw std::runtime_error("Choose a merger scene containing two galaxies. Isolation is available in physics_validate.");
    view.scene = std::move(scene);
    view.editable = true;
    view.preset_index = preset;
    view.quality_preset = -1;
    view.simulation_name = std::move(name);
    view.error.clear();
    clear_recording(view);

        view.status = view.simulation_name + " is loaded. Run integrates it.";
        view.picture_mode = view.scene.galaxies.empty() ? 0 : 2;
    
}

void load_builtin(ViewState& view, int index) {
    if (view.busy || index < 0 || index >= kSimulationCount) {
        return;
    }
    const std::filesystem::path root = examples_directory();
    if (root.empty()) {
        view.error = "The built-in simulations were not found next to the program.";
        return;
    }
    try {
        load_scene_into_view(view, physics::load_scene_file((root / kSimulations[index].file).string()),
                             kSimulations[index].name, index);
    } catch (std::exception const& error) {
        view.error = error.what();
    }
}

void load_user_file(ViewState& view, std::string const& path_text) {
    if (view.busy) {
        return;
    }
    const std::filesystem::path path(path_text);
    std::string extension = path.extension().string();
    for (char& character : extension) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    try {
        if (extension == ".scene") {
            load_scene_into_view(view, physics::load_scene_file(path.string()), path.stem().string(), -1);
        } else if (extension == ".traj") {
            physics::Trajectory trajectory = physics::read_trajectory(path.string());
            if (trajectory.samples.empty() || trajectory.bodies.empty()) {
                view.error = "That recording has no samples.";
                return;
            }
            view.editable = false;
            view.preset_index = -1;
            view.simulation_name = path.stem().string();
            view.resumable = false;
            view.error.clear();
            adopt_trajectory(view, std::move(trajectory));
            bool galaxy = false;
            {
                for (std::size_t i=0;i<view.trajectory.bodies.size();++i) {
                    if (physics::visual::component_of(view.trajectory.bodies[i], &view.trajectory.samples.front(), i) !=
                        physics::Component::Body) {
                        galaxy = true;
                        break;
                    }
                }
            }
            view.picture_mode = galaxy ? 2 : 0;
            frame_camera(view);
            view.status = "Loaded recording " + path.filename().string();
        } else {
            view.error = "Choose a .scene simulation or a .traj recording.";
        }
    } catch (std::exception const& error) {
        view.error = error.what();
    }
}

void save_recording(ViewState& view) {
    if (view.trajectory.samples.empty()) {
        view.error = "Run a simulation before saving a recording.";
        return;
    }
    const std::string suggested = view.simulation_name.empty() ? "recording.traj" : view.simulation_name + ".traj";
    std::string chosen;
    if (!browse_save_recording(suggested, chosen)) {
        return;
    }
    if (std::filesystem::path(chosen).extension().empty()) {
        chosen += ".traj";
    }
    try {
        physics::write_trajectory(view.trajectory, chosen);
        view.error.clear();
        view.status = "Saved " + chosen;
    } catch (std::exception const& error) {
        view.error = error.what();
    }
}

void draw_playback(ViewState& view, bool has_samples, physics::Diagnostics const& diagnostics) {

        ImGui::TextUnformatted("Point masses. Sphere size and star size are display only.");
    
    if (!has_samples) {
        ImGui::TextUnformatted("No recording yet. Choose a simulation and press Run.");
        return;
    }
    ImGui::Text("t = %.6g", view.playhead);

    ImGui::Text("energy = %.8g", diagnostics.energy);
    ImGui::Text("|P| = %.3g", physics::norm(diagnostics.linear_momentum));
    ImGui::Text("|L| = %.6g", physics::norm(diagnostics.angular_momentum));
    if (ImGui::Button(view.playing ? "Pause" : "Play")) {
        view.playing = !view.playing;
    }
    ImGui::SameLine();
    if (ImGui::Button("Frame")) {
        frame_camera(view);
    }
    float time_slider = static_cast<float>(view.playhead);
    if (ImGui::SliderFloat("Time", &time_slider, static_cast<float>(view.time_begin), static_cast<float>(view.time_end))) {
        view.playhead = time_slider;
    }
    float speed_slider = static_cast<float>(view.speed);
    const float speed_max =
        static_cast<float>(std::max((view.time_end - view.time_begin) / 2.0, std::max(view.speed * 4.0, 1e-3)));
    if (ImGui::SliderFloat("Speed", &speed_slider, 0.0f, speed_max)) {
        view.speed = speed_slider;
    }
}

struct ResolutionChoice {
    char const* label;
    int width;
    int height;
};

ResolutionChoice const kResolutions[] = {
    {"1280 x 720", 1280, 720},
    {"1600 x 900", 1600, 900},
    {"1920 x 1080", 1920, 1080},
    {"2560 x 1440", 2560, 1440},
    {"3840 x 2160", 3840, 2160},
};
constexpr int kResolutionCount = static_cast<int>(sizeof(kResolutions) / sizeof(kResolutions[0]));

void apply_window_resolution(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0) {
        glfwMaximizeWindow(window);
        return;
    }
    glfwRestoreWindow(window);
    int area_x = 0;
    int area_y = 0;
    int area_w = width;
    int area_h = height;
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        glfwGetMonitorWorkarea(monitor, &area_x, &area_y, &area_w, &area_h);
    }
    const int fitted_w = std::min(width, std::max(area_w, 640));
    const int fitted_h = std::min(height, std::max(area_h, 480));
    glfwSetWindowSize(window, fitted_w, fitted_h);
    glfwSetWindowPos(window, area_x + std::max(0, (area_w - fitted_w) / 2), area_y + std::max(0, (area_h - fitted_h) / 2));
}

void draw_settings_menu(GLFWwindow* window, ViewState& view) {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu("Settings")) {
        if (ImGui::BeginMenu("Resolution")) {
            if (ImGui::MenuItem("Maximized", nullptr, view.window_resolution < 0)) {
                view.window_resolution = -1;
                apply_window_resolution(window, 0, 0);
            }
            ImGui::Separator();
            for (int index = 0; index < kResolutionCount; ++index) {
                if (ImGui::MenuItem(kResolutions[index].label, nullptr, view.window_resolution == index)) {
                    view.window_resolution = index;
                    apply_window_resolution(window, kResolutions[index].width, kResolutions[index].height);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void draw_studio(GLFWwindow* window, ViewState& view, bool has_samples, physics::Diagnostics const& diagnostics) {
    draw_settings_menu(window, view);
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float margin = 16.0f * view.ui_scale;
    const float top = margin + ImGui::GetFrameHeight();
    const float panel_w = std::min(520.0f * view.ui_scale, display.x * 0.38f);
    place_panel(margin, top, panel_w, std::max(200.0f, display.y - top - margin));
    ImGui::Begin("Physics");
    char current[160] = "Choose a simulation";
    if (!view.simulation_name.empty()) {
        std::snprintf(current, sizeof(current), "%s", view.simulation_name.c_str());
    }
    if (view.busy) {
        ImGui::BeginDisabled();
    }
    if (ImGui::BeginCombo("Simulation", current)) {
        for (int index = 0; index < kSimulationCount; ++index) {
            const bool selected = index == view.preset_index;
            if (ImGui::Selectable(kSimulations[index].name, selected) && index != view.preset_index) {
                load_builtin(view, index);
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Load scene")) {
        std::string chosen;
        if (browse_open_scene(chosen)) {
            load_user_file(view, chosen);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load recording")) {
        std::string chosen;
        if (browse_open_recording(chosen)) {
            load_user_file(view, chosen);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save recording")) {
        save_recording(view);
    }
    if (view.busy) {
        ImGui::EndDisabled();
    }
    if (!view.status.empty()) {
        ImGui::TextWrapped("%s", view.status.c_str());
    }
    if (!view.error.empty()) {
        ImGui::TextWrapped("%s", view.error.c_str());
    }
    if (view.busy && view.control && view.scene.config.steps > 0) {
        const float step = static_cast<float>(view.control->step.load(std::memory_order_relaxed));
        const float total = static_cast<float>(view.scene.config.steps);
        ImGui::ProgressBar(total > 0.0f ? step / total : 0.0f, ImVec2(-1.0f, 0.0f));
    }
    if (view.editable && view.busy) {
        if (ImGui::Button("Stop")) {
            if (view.control) {
                view.control->cancel.store(true, std::memory_order_relaxed);
            }
        }
    } else if (view.editable) {
        if (ImGui::Button("Run")) {
            launch_simulation(view, false);
        }
        if (view.resumable) {
            ImGui::SameLine();
            if (ImGui::Button("Continue")) {
                launch_simulation(view, true);
            }
        }
    }
    if (ImGui::CollapsingHeader("Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
        draw_playback(view, has_samples, diagnostics);
    }
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        draw_parameters(view);
    }
    if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
        draw_appearance(view);
    }
    if (ImGui::CollapsingHeader("Export")) {
        draw_export(view);
    }
    ImGui::TextUnformatted("Middle mouse orbits. Shift+middle mouse pans. Scroll zooms. Space pauses. F frames. Esc closes.");
    ImGui::End();
}

int main(int argc, char** argv) {
    bool glfw_ready = false;
    bool batch_render = std::any_of(argv + 1, argv + argc, [](char const* value) {
        return std::string(value) == "--render";
    });
    try {
        ViewState view;
        std::string startup_file;
        if (argc >= 2) {
            startup_file = argv[1];
        }
        std::string render_path, render_shot, save_look_path;
        double render_time = 0, render_yaw = 1.1, render_pitch = 0.8, render_distance = 0;
        int render_width = 1600, render_height = 900, render_picture = 2;
        for (int i = 2; i < argc; ++i) {
            const std::string option = argv[i];
            auto next = [&]() -> std::string {
                if (++i >= argc)
                    throw std::runtime_error("missing value for " + option);
                return argv[i];
            };
            if (option == "--render") {
                batch_render = true;
                render_path = next();
            } else if (option == "--shot") {
                render_shot=next();
                if (render_shot!="wide" && render_shot!="face" && render_shot!="edge" && render_shot!="passage" && render_shot!="tails")
                    throw std::runtime_error("shot must be wide, face, edge, passage, or tails");
            } else if (option == "--look")
                load_look(view, next());
            else if (option == "--save-look")
                save_look_path = next();
            else if (option == "--dust-structure")
                view.dust_structure=std::stof(next());
            else if (option == "--arm-definition")
                view.arm_definition=std::stof(next());
            else if (option == "--detail")
                view.stellar_detail=std::stof(next());
            else if (option == "--blue-lifetime")
                view.young_lifetime = std::stof(next());
            else if (option == "--ionizing-lifetime")
                view.ionizing_lifetime = std::stof(next());
            else if (option == "--oiii-lifetime")
                view.oiii_lifetime = std::stof(next());
            else if (option == "--young-boost")
                view.young_boost = std::stof(next());
            else if (option == "--association-size")
                view.association_size = std::stof(next());
            else if (option == "--nebula-radius")
                view.nebula_radius = std::stof(next());
            else if (option == "--ionizing-mass-scale")
                view.ionizing_mass_scale = std::stof(next());
            else if (option == "--nebula-density-scale")
                view.nebula_density_scale = std::stof(next());
            else if (option == "--halpha-strength")
                view.halpha_strength = std::stof(next());
            else if (option == "--oiii-strength")
                view.oiii_strength = std::stof(next());
            else if (option == "--nebula-size")
                view.nebula_size = std::stof(next());
            else if (option == "--gas-glow")
                view.gas_glow = std::stof(next());
            else if (option == "--halpha-color") {
                for (float& value : view.halpha_color) value = std::stof(next());
            }
            else if (option == "--oiii-color") {
                for (float& value : view.oiii_color) value = std::stof(next());
            }
            else if (option == "--time")
                render_time = std::stod(next());
            else if (option == "--width")
                render_width = std::stoi(next());
            else if (option == "--height")
                render_height = std::stoi(next());
            else if (option == "--yaw")
                render_yaw = std::stod(next());
            else if (option == "--pitch")
                render_pitch = std::stod(next());
            else if (option == "--distance")
                render_distance = std::stod(next());
            else if (option == "--dust")
                view.dust = std::stof(next());
            else if (option == "--exposure")
                view.exposure = std::stof(next());
            else if (option == "--picture") {
                const auto name = next();
                if (name == "optical")
                    render_picture = 2;
                else if (name == "volume")
                    render_picture = 1;
                else if (name == "particles")
                    render_picture = 0;
                else
                    throw std::runtime_error("picture must be optical, volume or particles");
            } else
                throw std::runtime_error("unknown option: " + option);
        }
        if (render_width < 16 || render_width > 8192 || render_height < 16 || render_height > 8192 ||
            !std::isfinite(render_time) || !std::isfinite(render_yaw) || !std::isfinite(render_pitch) ||
            !std::isfinite(render_distance) || !std::isfinite(view.dust) || !std::isfinite(view.exposure) || !std::isfinite(view.dust_structure) || view.dust_structure<0 || view.dust_structure>1 ||
            !std::isfinite(view.arm_definition) || view.arm_definition<0 || view.arm_definition>1 ||
            !std::isfinite(view.stellar_detail) || view.stellar_detail<0 || view.stellar_detail>1)
            throw std::runtime_error("invalid render dimensions or settings");

        for (auto p : look_parameters(view))
            if (!std::isfinite(*p.value) || *p.value < p.low || *p.value > p.high)
                throw std::runtime_error(std::string("invalid optical setting: ") + p.name);

        if (!save_look_path.empty()) save_look(view, save_look_path);

        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("glfwInit failed");
        }
        glfw_ready = true;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_MAXIMIZED, batch_render ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, batch_render ? GLFW_FALSE : GLFW_TRUE);
        GLFWwindow* window = glfwCreateWindow(1600, 900, "Galaxy Merger", nullptr, nullptr);
        if (window == nullptr) {
            throw std::runtime_error("could not create an OpenGL 3.3 window");
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        load_gl([](char const* name) -> void* { return reinterpret_cast<void*>(glfwGetProcAddress(name)); });
        glfwSetWindowUserPointer(window, &view);

        glfwSetScrollCallback(window, [](GLFWwindow* glfw_window, double, double y_offset) {
            auto* state = static_cast<ViewState*>(glfwGetWindowUserPointer(glfw_window));
            if (ImGui::GetIO().WantCaptureMouse) {
                return;
            }
            state->camera.distance *= std::pow(0.9, y_offset);
            state->camera.distance = std::clamp(state->camera.distance, state->scene_radius * 0.02, state->scene_radius * 80.0);
        });
        glfwSetMouseButtonCallback(window, [](GLFWwindow* glfw_window, int button, int action, int) {
            auto* state = static_cast<ViewState*>(glfwGetWindowUserPointer(glfw_window));
            if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                state->middle_down = action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse;
                glfwGetCursorPos(glfw_window, &state->last_x, &state->last_y);
            }
        });
        glfwSetCursorPosCallback(window, [](GLFWwindow* glfw_window, double x, double y) {
            auto* state = static_cast<ViewState*>(glfwGetWindowUserPointer(glfw_window));
            const double dx = x - state->last_x;
            const double dy = y - state->last_y;
            state->last_x = x;
            state->last_y = y;
            if (!state->middle_down) {
                return;
            }
            if (glfwGetKey(glfw_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(glfw_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
                const double scale = state->camera.distance * 0.0016;
                state->camera.target += state->camera.right() * (-dx * scale) + state->camera.up() * (dy * scale);
            } else {
                state->camera.yaw -= dx * 0.005;
                state->camera.pitch = std::clamp(state->camera.pitch + dy * 0.005, -1.45, 1.45);
            }
        });

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::StyleColorsDark();
        view.base_style = ImGui::GetStyle();
        float content_scale = 1.0f;
        float content_scale_y = 1.0f;
        glfwGetWindowContentScale(window, &content_scale, &content_scale_y);
        content_scale = std::max(content_scale, content_scale_y);
        view.ui_scale = std::clamp(std::max(content_scale, 1.75f), 1.0f, 3.0f);
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
        if (!startup_file.empty()) {
            load_user_file(view, startup_file);
        } else {
            load_builtin(view, 0);
        }

        if (batch_render) {
            if (!view.error.empty()) throw std::runtime_error(view.error);
            if (view.editable) adopt_trajectory(view,physics::run_scene(view.scene));
            if (view.trajectory.samples.empty()) throw std::runtime_error("render requires a scene or recording");
            view.picture_mode=render_picture;
            view.camera.yaw=render_yaw;view.camera.pitch=std::clamp(render_pitch,-1.45,1.45);
            frame_camera(view);
            view.playhead=std::clamp(render_time,view.time_begin,view.time_end);
            if (!render_shot.empty()) compose_stellar_view(view,render_shot);
            render_time=view.playhead;
            if(render_distance>0) view.camera.distance=render_distance;
        }
        const GLuint lit_program = link_program(kLitVertex, kLitFragment);
        const GLuint color_program = link_program(kColorVertex, kColorFragment);
        const GLuint star_program = link_program(kStarVertex, kStarFragment);
        const GLuint field_program = link_program(kFieldVertex, kFieldFragment);
        const GLuint composite_program = link_program(kCompositeVertex, kCompositeFragment);
        const GLuint blur_program = link_program(kCompositeVertex, kBlurFragment);
        const GLuint volume_program = link_program(kVolumeVertex, kVolumeFragment);
        const physics::SphereMesh sphere = physics::make_uv_sphere(24, 36, physics::SphereUp::Z);

        GLuint sphere_vao = 0;
        GLuint sphere_buffers[3] = {};
        gl_gen_vertex_arrays(1, &sphere_vao);
        gl_bind_vertex_array(sphere_vao);
        gl_gen_buffers(3, sphere_buffers);
        gl_bind_buffer(GL_ARRAY_BUFFER, sphere_buffers[0]);
        gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(sphere.positions.size() * sizeof(float)),
                       sphere.positions.data(), GL_STATIC_DRAW);
        gl_vertex_attrib_pointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        gl_enable_vertex_attrib_array(0);
        gl_bind_buffer(GL_ARRAY_BUFFER, sphere_buffers[1]);
        gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(sphere.normals.size() * sizeof(float)),
                       sphere.normals.data(), GL_STATIC_DRAW);
        gl_vertex_attrib_pointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        gl_enable_vertex_attrib_array(1);
        gl_bind_buffer(GL_ELEMENT_ARRAY_BUFFER, sphere_buffers[2]);
        gl_buffer_data(GL_ELEMENT_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(sphere.indices.size() * sizeof(std::uint32_t)),
                       sphere.indices.data(), GL_STATIC_DRAW);

        GLuint color_vao = 0;
        GLuint color_buffer = 0;
        gl_gen_vertex_arrays(1, &color_vao);
        gl_bind_vertex_array(color_vao);
        gl_gen_buffers(1, &color_buffer);
        gl_bind_buffer(GL_ARRAY_BUFFER, color_buffer);
        gl_vertex_attrib_pointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(7 * sizeof(float)), nullptr);
        gl_enable_vertex_attrib_array(0);
        gl_vertex_attrib_pointer(1, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(7 * sizeof(float)),
                                 reinterpret_cast<void*>(3 * sizeof(float)));
        gl_enable_vertex_attrib_array(1);

        GLuint star_vao = 0;
        GLuint star_buffer = 0;
        gl_gen_vertex_arrays(1, &star_vao);
        gl_bind_vertex_array(star_vao);
        gl_gen_buffers(1, &star_buffer);
        gl_bind_buffer(GL_ARRAY_BUFFER, star_buffer);
        const GLsizei star_stride = static_cast<GLsizei>(8 * sizeof(float));
        gl_vertex_attrib_pointer(0, 3, GL_FLOAT, GL_FALSE, star_stride, nullptr);
        gl_enable_vertex_attrib_array(0);
        gl_vertex_attrib_pointer(1, 3, GL_FLOAT, GL_FALSE, star_stride, reinterpret_cast<void*>(3 * sizeof(float)));
        gl_enable_vertex_attrib_array(1);
        gl_vertex_attrib_pointer(2, 1, GL_FLOAT, GL_FALSE, star_stride, reinterpret_cast<void*>(6 * sizeof(float)));
        gl_enable_vertex_attrib_array(2);
        gl_vertex_attrib_pointer(3, 1, GL_FLOAT, GL_FALSE, star_stride, reinterpret_cast<void*>(7 * sizeof(float)));
        gl_enable_vertex_attrib_array(3);

        const GLint mvp_location = gl_get_uniform_location(lit_program, "u_mvp");
        const GLint model_location = gl_get_uniform_location(lit_program, "u_model");
        const GLint color_location = gl_get_uniform_location(lit_program, "u_color");
        const GLint roughness_location = gl_get_uniform_location(lit_program, "u_roughness");
        const GLint key_dir_location = gl_get_uniform_location(lit_program, "u_key_dir");
        const GLint fill_dir_location = gl_get_uniform_location(lit_program, "u_fill_dir");
        const GLint key_intensity_location = gl_get_uniform_location(lit_program, "u_key_intensity");
        const GLint fill_intensity_location = gl_get_uniform_location(lit_program, "u_fill_intensity");
        const GLint eye_location = gl_get_uniform_location(lit_program, "u_eye");
        const GLint view_projection_location = gl_get_uniform_location(color_program, "u_view_projection");
        const GLint star_view_location = gl_get_uniform_location(star_program, "u_view_projection");
        const GLint star_height_location = gl_get_uniform_location(star_program, "u_viewport_height");
        const GLint field_view_location = gl_get_uniform_location(field_program, "u_view_projection");
        const GLint field_height_location = gl_get_uniform_location(field_program, "u_viewport_height");
        const GLint field_gain_location = gl_get_uniform_location(field_program, "u_gain");
        const GLint field_max_point_location = gl_get_uniform_location(field_program, "u_max_point");
        const GLint field_profile_location = gl_get_uniform_location(field_program, "u_profile");
        const GLint composite_light_location = gl_get_uniform_location(composite_program, "u_light");
        const GLint composite_body_location = gl_get_uniform_location(composite_program, "u_body");
        const GLint composite_gas_location = gl_get_uniform_location(composite_program, "u_gas");
        const GLint composite_dust_location = gl_get_uniform_location(composite_program, "u_dust");
        const GLint composite_reddening_location = gl_get_uniform_location(composite_program, "u_reddening");
        const GLint composite_glow_location = gl_get_uniform_location(composite_program, "u_gas_glow");
        const GLint composite_soften_brightness_location = gl_get_uniform_location(composite_program, "u_soften_brightness");
        const GLint blur_image_location = gl_get_uniform_location(blur_program, "u_image");
        const GLint blur_texel_location = gl_get_uniform_location(blur_program, "u_texel");
        const GLint blur_span_location = gl_get_uniform_location(blur_program, "u_span");
        GLuint field_vao = 0;
        gl_gen_vertex_arrays(1, &field_vao);
        SceneGpu gpu;
        gpu.lit_program = lit_program;
        gpu.color_program = color_program;
        gpu.star_program = star_program;
        gpu.sphere_vao = sphere_vao;
        gpu.sphere_indices = static_cast<GLsizei>(sphere.indices.size());
        gpu.color_vao = color_vao;
        gpu.color_buffer = color_buffer;
        gpu.star_vao = star_vao;
        gpu.star_buffer = star_buffer;
        gpu.mvp_location = mvp_location;
        gpu.model_location = model_location;
        gpu.color_location = color_location;
        gpu.roughness_location = roughness_location;
        gpu.key_dir_location = key_dir_location;
        gpu.fill_dir_location = fill_dir_location;
        gpu.key_intensity_location = key_intensity_location;
        gpu.fill_intensity_location = fill_intensity_location;
        gpu.eye_location = eye_location;
        gpu.view_projection_location = view_projection_location;
        gpu.star_view_location = star_view_location;
        gpu.star_height_location = star_height_location;
        gpu.field_program = field_program;
        gpu.composite_program = composite_program;
        gpu.field_vao = field_vao;
        gpu.field_view_location = field_view_location;
        gpu.field_height_location = field_height_location;
        gpu.field_gain_location = field_gain_location;
        gpu.field_max_point_location = field_max_point_location;
        gpu.field_profile_location = field_profile_location;
        gpu.composite_light_location = composite_light_location;
        gpu.composite_body_location = composite_body_location;
        gpu.composite_gas_location = composite_gas_location;
        gpu.composite_dust_location = composite_dust_location;
        gpu.composite_reddening_location = composite_reddening_location;
        gpu.composite_glow_location = composite_glow_location;
        gpu.composite_soften_brightness_location = composite_soften_brightness_location;
        gpu.blur_program = blur_program;
        gpu.blur_image_location = blur_image_location;
        gpu.blur_texel_location = blur_texel_location;
        gpu.blur_span_location = blur_span_location;
        gpu.volume_program = volume_program;
        gpu.volume_field_location = gl_get_uniform_location(volume_program, "u_field");
        gpu.volume_eye_location = gl_get_uniform_location(volume_program, "u_eye");
        gpu.volume_forward_location = gl_get_uniform_location(volume_program, "u_forward");
        gpu.volume_right_location = gl_get_uniform_location(volume_program, "u_right");
        gpu.volume_up_location = gl_get_uniform_location(volume_program, "u_up");
        gpu.volume_scale_location = gl_get_uniform_location(volume_program, "u_scale");
        gpu.volume_origin_location = gl_get_uniform_location(volume_program, "u_origin");
        gpu.volume_extent_location = gl_get_uniform_location(volume_program, "u_extent");
        gpu.volume_mean_location = gl_get_uniform_location(volume_program, "u_mean");
        gpu.volume_floor_location = gl_get_uniform_location(volume_program, "u_floor");
        gpu.volume_gain_location = gl_get_uniform_location(volume_program, "u_gain");
        gpu.volume_contrast_location = gl_get_uniform_location(volume_program, "u_contrast");
        gpu.volume_heat_location = gl_get_uniform_location(volume_program, "u_heat");
        gpu.volume_extinction_location = gl_get_uniform_location(volume_program, "u_extinction");
        const float volume_triangle[] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
        gl_gen_vertex_arrays(1, &gpu.volume_vao);
        gl_bind_vertex_array(gpu.volume_vao);
        GLuint volume_buffer = 0;
        gl_gen_buffers(1, &volume_buffer);
        gl_bind_buffer(GL_ARRAY_BUFFER, volume_buffer);
        gl_buffer_data(GL_ARRAY_BUFFER, static_cast<std::ptrdiff_t>(sizeof(volume_triangle)), volume_triangle, GL_STATIC_DRAW);
        gl_vertex_attrib_pointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        gl_enable_vertex_attrib_array(0);
        ExportTarget export_target;
        std::vector<unsigned char> export_pixels;

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        double previous_time = glfwGetTime();
        std::vector<physics::Vec3> positions;
        bool space_was_down = false;
        bool frame_was_down = false;

        if (batch_render) {
            view.playhead=std::clamp(render_time,view.time_begin,view.time_end);
            physics::Diagnostics diagnostics;
            sample_at(view.trajectory,view.playhead,positions,diagnostics);
            std::string error;
            if (!capture_rgb(gpu,view,export_target,positions,view.playhead,render_width,render_height,export_pixels,error) ||
                !write_png_rgb(render_path,render_width,render_height,export_pixels.data(),error))
                throw std::runtime_error(error);
            if(glGetError()!=GL_NO_ERROR) throw std::runtime_error("OpenGL error during batch render");
            glfwSetWindowShouldClose(window,GLFW_TRUE);
        }
        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            const double now = glfwGetTime();
            const double frame_dt = std::min(now - previous_time, 0.05);
            previous_time = now;
            glfwPollEvents();

            const bool space_down =
                glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !ImGui::GetIO().WantCaptureKeyboard;
            if (space_down && !space_was_down) {
                view.playing = !view.playing;
            }
            space_was_down = space_down;
            const bool frame_down = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !ImGui::GetIO().WantCaptureKeyboard;
            if (frame_down && !frame_was_down) {
                frame_camera(view);
            }
            frame_was_down = frame_down;
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !ImGui::GetIO().WantCaptureKeyboard) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            poll_simulation(view);

            const bool has_samples = !view.trajectory.samples.empty();
            if (has_samples && view.playing && !view.export_active && view.time_end > view.time_begin) {
                view.playhead += view.speed * frame_dt;
                if (view.playhead > view.time_end) {
                    view.playhead = view.time_begin;
                }
            }
            if (view.export_active) {
                pump_export(view, gpu, export_target, export_pixels);
            }

            physics::Diagnostics diagnostics;
            if (has_samples) {
                sample_at(view.trajectory, view.playhead, positions, diagnostics);
            }

            int width = 1;
            int height = 1;
            glfwGetFramebufferSize(window, &width, &height);
            if (view.export_still && has_samples) {
                view.export_still = false;
                const std::string file = export_still_file(view.export_path);
                if (capture_rgb(gpu, view, export_target, positions, view.playhead, view.export_width, view.export_height,
                                export_pixels, view.export_error)) {
                    if (write_png_rgb(file, view.export_width, view.export_height, export_pixels.data(), view.export_error)) {
                        view.export_status = "Wrote " + file;
                    }
                }
            }
            draw_scene(gpu, view, positions, view.playhead, width, height, 0);


            if (view.ui_scale != view.ui_scale_applied) {
                ImGui::GetStyle() = view.base_style;
                ImGui::GetStyle().ScaleAllSizes(view.ui_scale);
                ImGui::GetIO().FontGlobalScale = view.ui_scale;
                view.ui_scale_applied = view.ui_scale;
            }
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            draw_studio(window, view, has_samples, diagnostics);
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    } catch (std::exception const& error) {
#if defined(_WIN32)
        if (!batch_render) MessageBoxA(nullptr, error.what(), "Galaxy Merger", MB_OK | MB_ICONERROR);
        std::cerr << error.what() << '\n';
#else
        std::cerr << error.what() << '\n';
#endif
        if (glfw_ready) {
            glfwTerminate();
        }
        return 1;
    }
    return 0;
}
