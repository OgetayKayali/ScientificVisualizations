# Galaxy Merger

An interactive galaxy merger simulator designed for appealing, scientifically informed visualization. Live gravity and gas dynamics shape the encounter; a separate optical renderer supplies stellar light, dust lanes, blue young populations, and illustrative H-alpha/[O III] emission.

**Run the application: `build/bin/Release/physics_view.exe`.** This is the single interactive program. The other build targets are command-line simulation, validation, and test utilities.

## Build and run

Requires CMake 3.24+, Git, a C++20 compiler, and OpenGL 3.3. On Windows, install Visual Studio's Desktop development with C++ workload. CMake downloads pinned GLFW, Dear ImGui, and doctest dependencies on the first configuration; OpenMP is used when available.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\bin\Release\physics_view.exe
```

Windows Release is the tested build. Single-configuration generators place executables directly in `build/bin`. Close the viewer before rebuilding it.

Choose **Galaxy merger**, **Seeded merger**, or **Retrograde merger**, then **Run**. The default Galaxy merger includes initial young populations and continuous star formation. Stop/Continue preserves an in-memory simulation checkpoint. Save recordings to replay them later. Appearance controls and `.look` profiles change the image without changing the dynamics.

Read the [usage guide](GUIDE.md), [optical renderer guide](docs/galaxy_visuals.md), [spiral-arm model](docs/galaxy_spirals.md), and [population and emission controls](docs/stellar_populations.md).

## What is simulated

- Direct softened N-body gravity, live exponential stellar disks, Hernquist bulges/halos, and velocity-Verlet integration.
- Smoothed-particle gas dynamics, cooling, density-triggered star formation, and thermal feedback. Continuous hybrid conversion conserves gravitational mass while tracking stellar cohorts.
- Seeded spiral density perturbations that subsequently evolve under gravity and gas forces. No persistent spiral force holds them in place.
- Optical reconstruction with foreground dust, age-dependent blue light, gas-linked emission, adjustable exposure/bloom, and optional independently integrated massless light tracers.

This is an illustrative model, not a calibrated astrophysical prediction. Scenes use code units. Finite particle counts and softening limit resolved structure; newly formed hybrid stars remain on their gas carrier. Emission colors, brightness, and fading times are adjustable proxies rather than photoionization or population-synthesis calculations. Blue continuum and oxygen emission are controlled separately.

## Reproducible runs and checks

```powershell
.\build\bin\Release\physics_run.exe examples/galaxy_luminous_merger.scene --trajectory build/merger.traj
.\build\bin\Release\physics_view.exe build/merger.traj --render build/merger.png --shot face --time 4 --look examples/nebular.look
.\build\bin\Release\physics_validate.exe examples/galaxy_luminous_merger.scene build/validation --isolate 0 --duration 4 --half-dt
```

The validator retains isolated progenitor controls to distinguish numerical disk evolution from merger effects. The viewer contains merger presets only. `physics_audit` separates numerical energy-error sources; `physics_trace` refines light sampling without changing saved physical trajectories. Direct gravity scales quadratically with particle count.

Optional rendering and CLI checks require Python with Pillow:

```powershell
python tests/render_smoke.py build/bin/Release/physics_view.exe build/render-tests
python tests/population_render_smoke.py build/bin/Release/physics_view.exe build/population-render-tests
python tests/validation_smoke.py build/bin/Release/physics_validate.exe build/validation-smoke
```

Dated reports in `docs/` preserve earlier validation results and identify their model versions. Generated recordings, binaries, and local run artifacts are not included in this source distribution.

## Credits and license

Created and directed by Ogetay Kayali, with GPT development assistance. Part of [ScientificVisualizations](https://github.com/OgetayKayali/ScientificVisualizations), covered by the repository's [license](../LICENSE.md). Dependencies retain their own licenses; see [third-party notices](THIRD_PARTY.md).
