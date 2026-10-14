# Galaxy stability and optical refinement — 12 October 2026

> Historical run report. Generated files under `build/` were local development artifacts and are not included in this source distribution. Presets and solver behavior may have changed since this run; use the current guides to reproduce new measurements.

The implemented pass adds resolution diagnostics, tests vertical support and gravitational softening, and improves population light, dust lanes, bulge reconstruction, and camera composition. The default showcase uses softening 0.20, timestep 0.005, and the existing 2,400 collisionless / 200 cold-gas / 64 hot-gas particles per galaxy. Fine doubles all three counts.

## Resolution evidence

The original left progenitor was isolated at 1x, 2x, and 4x massive-particle counts for 20 code-time units. All components scale together; total model mass, timestep 0.005, softening 0.03, geometry, and component laws stay fixed. Counts cause small integer-rounding changes to collisionless component fractions. Each level is a fresh same-seed draw, not a nested realization. The source is retained in `build/stability/source_before.scene`.

| Particles | Thickness change | Median radius change | Corrected energy residual |
| ---: | ---: | ---: | ---: |
| 2,664 | +29.8% | -1.0% | +0.000096% |
| 5,328 | +22.3% | -0.9% | +0.000392% |
| 10,656 | +11.6% | +3.4% | +0.000319% |

The reduction with particle count supports a substantial finite-resolution contribution. It does not isolate every mechanism: the halo, stellar disk, and gas all change resolution, and collective bending and imperfect equilibrium can also change measured thickness. The collisionless gravity-only control at the original resolution thickened by 27.5% over the same interval, so cooling and feedback are not required to produce most of that drift.

Local development preview (not distributed): `../build/stability/resolution.png` (Resolution histories and radial profiles).

## Full-duration choices

These controls cover 80 code-time units, matching the merger. The previous 0.03 result is reused from the completed solver audit; its saved trajectory was reanalyzed and the endpoint checked against that audit’s CSV. Other rows are new runs.

| Control | Particles | Softening | Thickness change | Median radius change | Energy residual |
| --- | ---: | ---: | ---: | ---: | ---: |
| Previous left | 2,664 | 0.03 | +97.5% | +5.5% | +0.014693% |
| Left | 2,664 | 0.08 | +78.5% | +2.4% | +0.001141% |
| Left | 2,664 | 0.12 | +57.7% | -2.0% | +0.000486% |
| Revised left | 2,664 | 0.20 | +41.9% | -6.7% | +0.000166% |
| Revised right | 2,664 | 0.20 | +62.8% | -9.0% | +0.000310% |
| Fine left | 5,328 | 0.20 | +21.9% | -3.6% | +0.000196% |

Increasing softening trades central force resolution for reduced particle scattering. At 0.20 the gravitational smoothing equals the initial disk scale height and the bulge scale radius; sub-softening dynamics are unresolved. This is an explicit coarse galaxy model, not a claim to resolve individual stellar encounters. The 0.005 step remains unchanged. The reference desktop budget remains available, while Fine provides a tested higher-particle-count option at approximately four times the direct-gravity cost.

**These are improved, not perfectly stationary galaxies.** Residual disk heating, bending, and radial redistribution remain. A single realization per resolution, and one Fine progenitor over 80 units, do not establish ensemble convergence or stability over arbitrary times.

Local development preview (not distributed): `../build/stability/full_duration.png` (Full-duration structure comparison).

## Initializer experiment

An optional `vertical_support jeans` integrates the vertical Jeans moment equation for the authored sech-squared density in the azimuth-averaged, midplane-reflected softened field. The new table includes the hot atmosphere, varies with height, and retains the existing radial Shu sampler. It omits the radial-vertical stress tilt term and samples Gaussian velocities, so it is not an exact distribution function. The implementation recovers an analytic isothermal-sheet fixture. The governing equation is documented in [Bovy’s vertical-equilibrium treatment](https://galaxiesbook.org/chapters/II-04.-Equilibria-of-Galactic-Disks_4-The-vertical-equilibrium-state-of-disks.html).

| 20-unit left control | Thickness change |
| --- | ---: |
| Legacy support, softening 0.03 | +29.8% |
| Jeans support, softening 0.03 | +40.4% |
| Legacy support, softening 0.08 | +15.5% |
| Jeans support, softening 0.08 | +28.8% |

The Jeans option did not improve these controls. It remains opt-in, with `vertical_support legacy` as the default. No artificial velocity damping, frozen disk plane, or energy renormalization was introduced.

## Visual delivery

- Persistent radial and individual stellar color variation, with unchanged dynamics, emission, and broadband luminance. These are artistic population labels, not calibrated ages or metallicities.
- Sharper cold-gas dust lanes, conserving integrated optical depth across broad/narrow kernels and finite-depth layers.
- Smoother 32-neighbor bulge light with less sparse-particle streaking; fixed final-output dither reduces 8-bit banding.
- Fixed current-light, face-on, edge-on, first-passage, and post-passage compositions in the viewer and the `--shot` export option.
- Revised isolated and merger examples, matching Fine examples, and the old isolated demo retained as `galaxy_isolated_legacy.scene`.

## New merger

The revised desktop encounter completed all 80 time units with 5,328 massive bodies and a corrected energy residual of +0.000519%. The recording is `build/stability/merger/merger.traj`. No disk-thickness number is assigned to the mixed merger because it has no single meaningful disk plane. The Fine merger scene is supplied but was not separately run through the full encounter in this pass.

## Reproduce

```powershell
.\build\bin\Release\physics_validate.exe build/stability/source_before.scene build/recheck_resolution --isolate 0 --resolution-study --duration 20 --trajectories
.\build\bin\Release\physics_validate.exe examples/galaxy_merger.scene build/recheck_desktop --trajectories
.\build\bin\Release\physics_validate.exe examples/galaxy_merger_fine.scene build/recheck_fine --isolate 0 --duration 80 --trajectories
python tools/plot_isolation.py build/recheck_resolution --output build/recheck_resolution/profiles.png
```

`_history.csv` contains global median disk radius and absolute height at every snapshot. `_profiles.csv` contains annular surface density, mass-weighted median absolute height, rotation, radial and vertical dispersions, and counts. Measurements follow the original disk’s center of mass and instantaneous angular-momentum plane. Empty bins must be ignored; the plotter omits bins below 16 stars. All quantities use code units.

Ready isolated recording: `build/stability/isolated_fine_showcase.traj`. It adds massless light tracers and the current population palette to the completed Fine physical run. The tracer softening is 0.20, substep 0.02, and tree opening parameter 0.4. The desktop equivalent is `build/stability/isolated_color.traj`. Both are optical refinements, not additional physical resolution.

Validation: Release build; all 72 C++ cases; CLI resolution/duration/mode/invalid-option smoke checks; actual OpenGL emission, dust ordering, V3/V5/V6 playback, massless light, and motion checks. Rendered galaxy frames were visually inspected.

## Delivered recordings and previews

- `build/stability/isolated_fine_showcase.traj`: 5,328 physical bodies plus 5,820 independent light tracers.
- `build/stability/merger_showcase.traj`: revised desktop merger, 5,328 physical bodies plus 5,816 independent light tracers.
- Both cover all 801 snapshots. `fine_verification.json` and `merger_verification.json` confirm byte-exact preservation of every physical sample and diagnostic, unchanged physical masses, zero tracer masses, and intrinsic luminance changes below 2e-13 relative.
- Restyling uses the final population palette while preserving the physical run. `physics_trace --restyle` applies appearance from a matching scene before refining the light.
- HDR colors also export legally to glTF using [KHR_materials_emissive_strength](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_emissive_strength/README.md); the regression checks unit-range material factors and retained radiance.

Local development preview (not distributed): `../build/stability/showcase_contact.png` (Isolated galaxy and merger previews).

Individual previews are `fine_face_40.png`, `fine_edge_0.png`, `merger_passage.png`, and `merger_55.png`. Their framing differs, so use the profile plots for quantitative size comparisons. Dust and starlight remain finite-particle reconstructions; unresolved central structure is illustrative.

Load the recordings in the rebuilt viewer in Optical mode. For fresh runs, use the revised examples or the Fine budget. The experimental Jeans switch remains off by default.
