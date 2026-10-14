# Gas integration and stellar tracers — 12 October 2026

> Historical run report. Generated files under `build/` were local development artifacts and are not included in this source distribution. Presets and solver behavior may have changed since this run; use the current guides to reproduce new measurements.

The subsequent [stability and appearance pass](galaxy_stability_2026-10-12.md) changes the showcase softening and adds a Fine budget. The results below describe the original 0.03-softening solver audit.

This follow-up supersedes the numerical findings in the earlier same-day validation report. The gas update was replaced, controlled error sources were tested, and the merger plus exact isolated controls were rerun at three timesteps with cooling, star formation, and feedback enabled. No global energy correction was applied.

## Controlled audit

The old gas update used stale velocity/thermal derivatives. In the adiabatic fixture its energy error approximately halved when the timestep halved. The new mechanical predictor/corrector refreshes pressure after thermal updates; cooling is split around the mechanical step, including the portion of a step remaining after a feedback delay expires. Collisionless gravity retains velocity Verlet.

| Fixture, timestep 0.02 | Before: energy residual | After: energy residual |
| --- | ---: | ---: |
| gravity | -0.53619% | -0.53619% |
| adiabatic | +1.52185% | +0.000233402% |
| cooling | +0.415404% | +4.06911e-05% |
| feedback | +1.19933% | +5.86415e-05% |

Both controlled audit CSVs contain four timestep levels. Stochastic birth histories and nonlinear cooling prevent interpreting the full feedback suite as a formal convergence-order measurement.

## Full-duration comparison

The input used for this series is preserved in `build/convergence/source.scene`: 80 time units, 5,328 massive bodies in the merger, 2,664 per isolated control, seed 20261009, gravitational softening 0.03. Each refinement keeps output spacing at 0.1.

| Run | Step | Corrected energy residual | Disk radius change | Disk thickness ratio | New stars |
| --- | ---: | ---: | ---: | ---: | ---: |
| merger | 0.02 | +10.1358% | — | — | 143 |
| isolated_0 | 0.02 | +1.8763% | -3.5% | 1.94× | 67 |
| isolated_1 | 0.02 | +1.2034% | -10.4% | 1.83× | 59 |
| merger_half_dt | 0.01 | +1.0251% | — | — | 140 |
| isolated_0_half_dt | 0.01 | +0.1265% | -3.2% | 1.79× | 66 |
| isolated_1_half_dt | 0.01 | +0.2537% | -1.1% | 1.92× | 69 |
| merger_quarter_dt | 0.005 | +0.2392% | — | — | 144 |
| isolated_0_quarter_dt | 0.005 | +0.0147% | +5.5% | 1.97× | 63 |
| isolated_1_quarter_dt | 0.005 | +0.0826% | -3.8% | 1.93× | 72 |

Energy residual is `(Efinal - Einitial + radiated + formation_removed - feedback - floor_added) / abs(Einitial)`. Table signs are retained; the plot uses absolute values. Source budgets, momenta, and raw shape measurements are in `build/convergence/summary.csv`. Disk shape is measured about its current angular-momentum plane and center of mass.

Local development preview (not distributed): `../build/convergence/energy_convergence.png` (Energy convergence).

The isolated disks still thicken: timestep improvement does not remove particle-resolution heating or guarantee equilibrium. Stochastic star formation changes with timestep and particle reindexing. These runs support the smaller integration step, but do not establish particle-count convergence or full astrophysical accuracy.

## Independent light tracers

`physics_trace` adds 5,816 massless tracers with four tracers per original stellar disk particle. The originals keep 15% of their light and the tracers share 85%; physical mass, source phase space, source diagnostics, and intrinsic stellar luminosity are preserved. Tracers evolve independently in the interpolated recorded potential using their own velocity-Verlet integration. They are not attached to parents.

The delivered refinement uses maximum tracer step 0.02, softening 0.08, and monopole tree opening angle 0.4. These are display approximations in code units. The initial local phase-space reconstruction, smoothed potential, finite source cadence, and tree approximation limit physical fidelity. Tracers do not contribute self-gravity or recover missing dynamical structure. The Optical renderer uses exact tree neighbor queries and a wider local neighbor sample for refined recordings.

V6 recordings preserve explicit light weights and tracer settings; V1–V5 playback is retained. Refinement is an offline CLI operation, and the output can be loaded into the viewer. See [the workflow](galaxy_visuals.md#independently-integrated-light-tracers).

## Verification

64 C++ tests pass, including exact neighbor search, tree force comparison, tracer orbital convergence, source preservation, luminosity accounting, fresh post-cooling accelerations, feedback-delay timing, gas Stop/Continue equivalence, and V6 round trips. Real OpenGL checks cover V3/V5/V6 playback, massless light, dust ordering, embedded attenuation, and sub-snapshot motion. Full-resolution merger frames were inspected before and after refinement.


## Delivered showcase

The merger example now defaults to `dt 0.005`, 16,000 steps, and sampling every 20 steps. The final refined recording is `build/convergence/galaxy_showcase_refined.traj`; the original physical recording is `build/convergence/merger_quarter_dt.traj`. The tracer pass does not run automatically when pressing Run: load the refined recording or use the documented CLI refinement command.

```powershell
.\build\bin\Release\physics_view.exe build/convergence/galaxy_showcase_refined.traj
```

All 801 snapshots of the delivered refinement were checked against the source. Diagnostics, positions, velocities, components, birth times, densities, and internal energies of the massive particles are exactly preserved. Masses are unchanged and the additional tracer masses are zero. The intrinsic authored-light difference is approximately -5.9e-13 in code luminosity units, from floating-point summation. Results are saved in `build/convergence/tracer_verification.json`.

Local development preview (not distributed): `../build/convergence/showcase_55.png` (Refined merger at time 55).
