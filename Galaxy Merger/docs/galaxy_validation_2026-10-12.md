# Galaxy validation — 12 October 2026

> Historical run report. Generated files under `build/` were local development artifacts and are not included in this source distribution. Presets and solver behavior may have changed since this run; use the current guides to reproduce new measurements.

All six runs completed for `examples/galaxy_merger.scene`: 80 code-time units, 5,328 bodies in the merger and 2,664 in each exact isolated control. The original step is 0.02; the comparison uses 0.01. The source configuration is preserved at `build/validation/source.scene`.

The new hot-gas pressure-work fix is included in every run. The controls use the parent realization and parent gas density thresholds. These results do not establish numerical convergence.

| Run | Step | Disk radius change | Disk thickness ratio | Corrected energy residual | New stars |
| --- | ---: | ---: | ---: | ---: | ---: |
| merger | 0.02 | — | — | +2.20% | 136 |
| isolated_0 | 0.02 | -3.6% | 2.16× | -3.85% | 61 |
| isolated_1 | 0.02 | -5.1% | 2.04× | -5.72% | 71 |
| merger_half_dt | 0.01 | — | — | -7.22% | 149 |
| isolated_0_half_dt | 0.01 | -0.3% | 1.87× | -3.31% | 70 |
| isolated_1_half_dt | 0.01 | -8.7% | 1.96× | -5.85% | 73 |

Radius is the median cylindrical radius; thickness is the median absolute height about the instantaneous stellar-disk angular-momentum plane and disk center of mass. Combined merger shape measures are omitted because the two disks have no single useful reference plane.

The energy column is `(Efinal - Einitial + radiated + formation_removed - feedback - floor_added) / abs(Einitial)`. Signs are retained. Raw energies, source budgets, and momentum changes are in `build/validation/summary.csv`.

## Interpretation

- Isolated disks thicken by roughly 1.9–2.2 times. Merger-driven thickening should therefore be judged against these controls, not against the pristine initial disk.
- Halving the timestep does not consistently reduce the corrected energy residual. In the merger it changes from +2.20% to -7.22%; the original smaller residual cannot be treated as evidence of greater accuracy.
- Linear and angular momentum changes remain near floating-point roundoff in these runs (largest absolute angular-momentum change below 5e-14). This is a useful independent check, but does not resolve the energy problem.
- Star formation uses step/index-based random draws, so the half-step runs and reindexed isolated controls do not preserve identical birth histories. These comparisons diagnose sensitivity, not a formal convergence order.

## Follow-up

The next numerical investigation should isolate gas pressure-work integration from cooling and star formation, then test collisionless particle-count sensitivity. The present velocity-Verlet implementation retains hydro accelerations from before the final energy/cooling update; its treatment of velocity- and energy-dependent forces deserves a dedicated audit. This is a candidate cause, not an established explanation of the residuals. Do not tune the appearance to conceal this diagnostic.

## Verified implementation

59 C++ tests pass. Real OpenGL checks pass for emitted light, foreground/background/embedded dust, V3 playback, and motion between stored samples. Fresh 1920×1080 frames were inspected at times 0, 42, and 55. The optical changes preserve integrated packet light and dust column weights; sparse tails are still limited by simulation sampling.

Run instructions and model limits: [galaxy_visuals.md](galaxy_visuals.md).
