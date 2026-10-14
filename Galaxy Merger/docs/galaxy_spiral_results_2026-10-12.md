# Spiral validation — 2026-10-12

> Historical run report. Generated files under `build/` were local development artifacts and are not included in this source distribution. Presets and solver behavior may have changed since this run; use the current guides to reproduce new measurements.

The new presets seed a trailing two-arm density pattern in stars and gas, then evolve it freely. They use a cooler radial velocity distribution, a thinner disk, and more massive-particle samples of the stellar disk. The implementation and reproducible commands are in [the spiral workflow](galaxy_spirals.md).

## Isolated controls

All controls use dt = 0.005 and 24 code-time units. The unseeded control changes only initial spiral strength to zero. Fine doubles all component counts; it is a new same-seed draw, not a nested particle realization. These are single-realization comparisons, not ensemble convergence.

| Run | Bodies | Initial A2 | A2 at t=8 | A2 at t=24 | Thickness change | Radius change | Corrected energy residual |
|---|---:|---:|---:|---:|---:|---:|---:|
| isolated | 2,664 | 0.317 | 0.261 | 0.178 | +72.1% | -3.9% | -0.000030% |
| unseeded | 2,664 | 0.112 | 0.241 | 0.213 | +80.5% | -0.9% | -0.000005% |
| fine | 5,328 | 0.256 | 0.216 | 0.156 | +36.4% | -1.6% | +0.000099% |

A2 is the count-weighted mean annular amplitude between 1 and 4 initial disk scale radii. It is not a global Fourier coefficient. The random-phase RMS reference is approximately 0.06–0.07 at desktop resolution and 0.05 for Fine. A2 can include bars and tidal elongations; it must be read alongside phase and morphology.

The seeded desktop model begins with measured Q about 1.3–1.6 across R=0.5–4. Fine begins around 1.5–1.6. The seeded phase profiles wind coherently with radius. Initial seeding makes the starting two-arm structure stronger and more controlled; even the unseeded cooler disk develops non-axisymmetric structure later. The lower-resolution late-time amplitude is larger, so it should not be mistaken for a more accurate result.

The disks heat and thicken substantially, and Q rises with time. Fine reduces the thickness drift in this comparison but does not preserve the initial spiral indefinitely. Gravitational response, phase mixing, and finite-particle effects are all present; this experiment does not isolate their individual contributions. No velocity damping or imposed rotating arm potential is applied.

Local development preview (not distributed): `../build/spiral/diagnostics.png` (Spiral diagnostics).

## Verification

- Release build and 78 C++ test cases passed, including the seeded Fourier modes, reverse winding, unchanged radial/vertical positions and masses, component mass budgets, radial-temperature response, measured epicyclic frequency, and optical light conservation.
- CLI smoke checks cover parsing, invalid spiral parameters, diagnostics columns, isolation, particle scaling, duration and gravity-only controls.
- Real OpenGL checks cover emission, dust ordering, legacy/new recordings, massless light and playback motion.
- Rendered frames were visually inspected. Arms are broad and irregular at this particle budget; the output is not a calibrated galaxy observation.


## Delivered isolated recordings

- `build/spiral/spiral_showcase.traj`: 2,664 physical bodies and 5,760 massless light tracers; 241 snapshots, t=0–24.
- `build/spiral/spiral_fine_showcase.traj`: 5,328 physical bodies and 11,520 massless light tracers; 241 snapshots, t=0–24.

The verification JSONs confirm exact preservation of all original phase-space samples, gas fields, diagnostics, names, and masses. Intrinsic broadband luminosity changes only at floating-point rounding level (less than 2e-13 relative). Tracers use gravitational softening 0.10, substep 0.02, and tree opening parameter 0.4. They improve light sampling, not physical resolution. The examples default to 80 time units; the isolated validation recordings cover the explicitly tested first 24.

Fine isolated preview at t=4, using the native optical renderer:

Local development preview (not distributed): `../build/spiral/fine_4.png` (Fine seeded spiral at t=4).

## Full desktop merger

The seeded merger ran all 80 code-time units (16,000 steps, 801 snapshots, 5,328 physical bodies). The corrected mechanical-energy residual, accounting for recorded gas cooling, feedback, formation losses and floor heating, is **0.005461%** of the initial energy magnitude. It formed 138 young-star particles. Momentum change is 9.72e-16 and angular-momentum change is 5e-14, in code units.

This validates a completed run and a small energy-budget residual; it is not a timestep-convergence study. The Fine merger scene is supplied but was not integrated for this report. Early seeded arms are allowed to wind and change before first passage; late tails should not be described as surviving undisturbed spiral arms.


The ready merger recording is `build/spiral/spiral_merger_showcase.traj`: 5,328 physical bodies plus 5,760 massless light tracers, all 801 snapshots from t=0–80. The merger uses two tracers per disk particle; the isolated showcases use four. The verification JSON confirms exact preservation of all physical samples and diagnostics. The source physical recording is `build/spiral/merger/merger.traj`.

Local development preview (not distributed): `../build/spiral/preview.png` (Isolated spiral and merger preview).

Load the desired recording in `build/bin/Release/physics_view.exe` for playback. Choose Face-on disk for the isolated galaxy, or First passage / After passage for the merger. There is still only one application executable; no viewer backup copies were created.
