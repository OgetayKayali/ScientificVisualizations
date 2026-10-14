# Seeded spiral galaxies

Launch `build/bin/Release/physics_view.exe`, then select **Seeded merger** in the simulation selector and press Run. The existing galaxy presets remain comparison baselines. Use Optical picture, Face-on disk, and the Arm and stream definition control to inspect the structure. Old `.traj` recordings retain their original dynamics; loading one does not add arms retroactively.

## Initial conditions

The seeded scene is `examples/galaxy_spiral_merger.scene`, with a `_fine.scene` version. They retain the existing mass model and encounter geometry, with these explicit changes:

| Setting | New spiral preset | Meaning |
|---|---:|---|
| `target_q` | 1.45 | Radial Shu velocity scale from the softened initial circular curve, independently of vertical support |
| `disk_particle_fraction` | 0.60 | 60% of collisionless particles in the stellar disk; each component retains its authored mass |
| `disk_height` | 0.12 | Initial sech-squared height, relative to disk scale 1 |
| `softening` | 0.10 | Gravity smoothing length, paired with the new disk resolution |
| `spiral_strength` | 0.55 | Initial sinusoidal density contrast; tapers toward the center and outer edge |
| `spiral_arms` | 2 | Two or four seeded arms |
| `spiral_pitch` | 28 degrees | Trailing logarithmic pattern; winding reverses with spin |
| `spiral_phase` | 20 degrees | Initial orientation in the galaxy plane |

Desktop has 1,440 massive stellar-disk particles per galaxy (previously about 727), within the same 2,664 total-body budget. Fine has 2,880 disk particles and 5,328 total bodies per galaxy. More disk samples mean fewer halo samples: halo particles become heavier than disk particles, so numerical heating still needs monitoring. This allocation is not a convergence claim.

The seeded azimuthal density is proportional to `1 + a(R) cos[m(phi - phase(R))]`. The inverse cumulative distribution maps one uniform random angle to this density without changing its radius, height, or mass. `phase(R) = phase0 - spin*cot(pitch)*ln(R/Rd)` away from the regularized center. The radial taper is `(1-exp[-(R/Rd)^2/0.36])*exp[-(R/(7Rd))^4]`. Gas starts on the same arms with amplitude multiplied by 1.3 and capped at 0.85. Thus dust and future star formation begin in a related physical gas distribution.

This is a deliberately perturbed initial condition, not a stationary spiral equilibrium and not a claim of spontaneous arm formation. The potential and velocities are initialized with the existing azimuth-averaged approximations. There is no imposed rotating potential, ongoing arm force, velocity damping, or frozen pattern. The arms can shear, strengthen, fragment, and fade under the existing self-gravity, gas cooling, and feedback.

All new fields default to the legacy behavior: Q target zero uses `dispersion_ratio`; particle fraction zero uses the old equal-mass allocation; strength zero gives an axisymmetric density law. A nonzero Q target overrides the radial dispersion ratio but leaves vertical support separate. Optional Jeans vertical support remains off in these examples.

## Measurements

`physics_validate` now appends these columns to isolated-disk `*_profiles.csv` files:

- `a2`, `a4`: mass-weighted magnitudes of the two- and four-fold azimuthal Fourier modes in each radial bin. A sinusoidal contrast `a` gives amplitude `a/2` in a narrow annulus.
- `phase2`, `phase4`: mode phase in radians, modulo pi or pi/2, in the instantaneous disk plane. The plane basis is projected from a world axis; absolute phases should not be compared across abrupt plane/basis changes.
- `noise`: random-phase RMS amplitude `sqrt(sum(mass^2))/sum(mass)`. This is a sampling reference, not a confidence threshold.
- `circular_speed`, `kappa`: azimuth-averaged softened gravitational probes at the radial midpoint and +/-10% of that radius. All physical masses contribute. The probes use 24 azimuths and derive kappa from the radial circular-speed gradient, not the stellar streaming speed.
- `q`: `sigma_R*kappa/(3.36*G*Sigma_stars)`. Empty bins, fewer than 16 stars, or invalid circular/epicyclic frequencies produce NaN for unavailable quantities. Amplitudes in sparsely populated bins also need caution.

The radial dispersion subtracts the annular mean velocity; coherent spiral flows can still contribute to it. Q is the classical **thin stellar-disk proxy**, not the stability threshold of our thick, softened, star-plus-gas system. See [Toomre (1964)](https://adsabs.harvard.edu/pdf/1964ApJ...139.1217T). The input target is a distribution-function scale; realized Q is measured separately. Spiral response depends on more than Q; see [Michikoshi & Kokubo (2018)](https://arxiv.org/abs/1808.08060).

A2 alone also detects a bar or elongation. Confirm spiral morphology with the radial phase variation and actual frames. The plotter reports a count-weighted mean of annular amplitudes from 1 to 4 disk scale radii, not a single global mode (which can cancel as a spiral winds). It omits bins with fewer than 32 stars and shows initial phases only where A2 exceeds twice the RMS noise reference. This cutoff is illustrative, not a formal detection significance.

```powershell
.\build\bin\Release\physics_validate.exe examples/galaxy_spiral_merger.scene build/spiral/isolated --isolate 0 --duration 24 --trajectories
.\build\bin\Release\physics_validate.exe examples/galaxy_spiral_merger_fine.scene build/spiral/fine --isolate 0 --duration 24 --trajectories
.\build\bin\Release\physics_validate.exe examples/galaxy_spiral_merger.scene build/spiral/merger --merger-only --trajectories
python tools/plot_spirals.py build/spiral/isolated/isolated_0_profiles.csv build/spiral/fine/isolated_0_profiles.csv --output build/spiral/diagnostics.png
```

For the unseeded comparison, copy the same spiral scene and set only `spiral_strength 0`. This preserves the new height, Q target, softening, particle counts, component masses, and radial draws; velocities respond to the altered initial field.

## Optical reconstruction

Arm and stream definition narrows the detail kernel across locally elongated stellar structures while preserving more width along them. It uses current particle-neighbor covariance, not a spiral template, so it follows tidal streams too. Existing normalized kernels preserve each packet's total light. Stellar covariance now uses 32 neighbors, or 96 with optical tracers, to reduce artificial streaks from small particle groups. Existing gas-linked dust and fading young-star light supply additional contrast.

Batch renders accept `--arm-definition 0..1` and `--detail 0..1`. A useful starting point is `--shot face --exposure 6 --detail 0.45 --dust 0.7`. Rendering settings change the image only. Independent massless tracers refine the light without changing physical snapshots, masses, or diagnostics.

```powershell
.\build\bin\Release\physics_trace.exe examples/galaxy_spiral_merger.scene build/spiral/merger/merger.traj build/spiral/spiral_merger_showcase.traj --per-star 4 --dt 0.02 --theta 0.4
python tools/verify_refinement.py build/spiral/merger/merger.traj build/spiral/spiral_merger_showcase.traj --output build/spiral/merger_verification.json
```

Measured run results and previews are in [the spiral run report](galaxy_spiral_results_2026-10-12.md).
