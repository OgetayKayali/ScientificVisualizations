# Stellar populations and emission regions

Run `build/bin/Release/physics_view.exe`, choose **Galaxy merger**, then **Run**. Use **Picture → Optical**, **Face-on disk**, and **Stellar populations and emission lines**. The `_fine.scene` variants have more particles. These are new presets; the previously validated seeded-spiral scene files retain their original dynamics.

Create a recording with the luminous merger scene using the commands in the README. Older particle recordings remain readable, but have no newly invented initial stellar history or fractional formation history. Run the Galaxy merger preset to obtain those features.

## What changed

- Existing disk mass can include an initial young population. Its fraction follows the initial arm crests, with deterministic sampled ages. Ages and colors travel with the particles, rather than being repainted onto a fixed spiral. Changing these initial population labels does not change mass, positions, velocities, or the dynamics RNG.
- Continuous star formation converts a fraction of eligible cold gas each integration step. Total gravitational mass and momentum stay unchanged. Gas density, pressure acceleration, cooling, thermal budgets, and feedback use the remaining gas mass consistently. Newly formed stellar cohorts record their individual formation times and masses.
- Blue continuum, H-alpha ionizing sources, and harder OIII sources have separate fading clocks. The blue continuum mixes young and old light by luminosity, rather than interpolating their colors without accounting for their different brightness.
- H-alpha and [O III] are independent emission layers with independent colors and strengths. They need both gas and nearby ionizing populations. Source weights include population mass and configured young-star luminosity, including the divided weights of display tracers. No per-particle ionizing boost is added when resolution increases.
- Cold-gas sampling is 400 particles per galaxy in the new desktop presets and 800 in Fine (previously 200/400). Total gas mass is unchanged. The general Optimal/Fine quality buttons use these updated budgets too.

## Simulation and initial-population controls

Edit these in **Parameters** before Run, or in a `.scene` file. Two gas-bearing galaxies must share the same cooling, formation, and feedback law. Initial population fractions and ages can differ.

| Scene key | New preset | Meaning |
| --- | ---: | --- |
| `initial_young_fraction` | 0.18 | Peak fraction of existing disk mass in recent stars; range 0–1. Zero disables initial young populations. |
| `initial_young_age` | 8 | Ages sampled uniformly between zero and this value. Code time units. |
| `young_arm_contrast` | 0.95 | 0: uniform fraction; 1: strongest concentration at initial arm crests. No imposed arm modulation when spiral strength is zero. |
| `population_variation` | 0.25 | Additional artistic color variation in the older disk population. |
| `young_color` | 0.28 0.52 1.00 | Blue population palette. Editable through the galaxy's young color group. |
| `young_emission` | 12 | Young-population light per unit mass; also scales its ionizing proxy relative to the legacy value 6.5. |
| `star_formation_mode` | `continuous` | Gradual hybrid conversion. `stochastic` retains whole-particle conversion used by older scenes. |
| `star_formation_time` | 5 | Gas-consumption timescale at the threshold density. |
| `star_formation_density` | 0 | Zero uses four times the initial median cold-gas density; positive values set a threshold in code units. |
| `star_formation_temperature` | 1.5 | Maximum specific internal energy divided by the cold-floor energy; minimum 1. |
| `star_formation_converging` | `true` | Require negative velocity divergence. Set `false` to permit formation in dense cold gas without that gate. |
| `feedback_energy`, `feedback_time` | 4, 1 | Energy returned per new stellar mass; suspension of cooling after feedback. |
| `gas_particles` | 400 / 800 Fine | Resolution, at fixed total cold-gas mass. |

The continuous rule is `dMstar = Mgas * (1 - exp(-dt / tau))`, with `tau = star_formation_time * sqrt(threshold / density)`, after the density/temperature/convergence gates. A residual gas fraction of 1e-6 is retained to keep the carrier's SPH calculation well conditioned. Formation removes exactly the consumed gas's thermal energy, tracked separately; feedback adds energy proportional to the converted mass. Events are timestamped at the end of the accepted step for playback; conversion is applied before its drift.

## Live visual controls

These work during playback and in PNG/MP4 export without rerunning the dynamics. All clocks use **simulation time units**, not calibrated Myr. Brightness and palette choices are illustrative.

| Picture control / batch option | Default | Effect |
| --- | ---: | --- |
| Blue-light lifetime / `--blue-lifetime` | 12 | Exponential fade of young stellar continuum. |
| Ionizing lifetime / `--ionizing-lifetime` | 0.8 | Fade of H-alpha source illumination. |
| OIII source lifetime / `--oiii-lifetime` | 0.5 | Fade of harder source illumination; bounded by available ionizing illumination. |
| Young-light boost / `--young-boost` | 2 | Display-only continuum multiplier; does not change ionization or star formation. |
| Association size / `--association-size` | 0.4 | Footprint multiplier for newly formed, gas-carried populations and legacy young-star particles. Initial disk populations keep their disk footprint. |
| Nebula emission / `--gas-glow` | 1 | Master emission-line multiplier; zero turns lines off. |
| Ionizing reach / `--nebula-radius` | 0.65 | Gaussian distance scale from stellar sources to gas, in code length units. |
| Ionizing mass scale / `--ionizing-mass-scale` | 0.00002 | Illumination at half saturation. Lower makes faint ionizing populations more visible. |
| Emission density scale / `--nebula-density-scale` | 0.01 | Gas density at half emission response. |
| H-alpha strength / `--halpha-strength` | 24 | Independent H-alpha contrast, including zero. |
| OIII strength / `--oiii-strength` | 6 | Independent [O III] contrast, including zero. |
| Nebula size / `--nebula-size` | 0.6 | Emission footprint relative to the local gas kernel. |
| H-alpha palette / `--halpha-color R G B` | 1, 0.08, 0.22 | Illustrative pink-red mapping. |
| OIII palette / `--oiii-color R G B` | 0.08, 0.85, 0.75 | Illustrative cyan-green mapping. |

**Broadband look** reduces line emphasis and blue boost. **Emission emphasis** restores the defaults above. Adjust exposure, dust, and smoothing using the existing controls.

The emission proxy uses remaining gas mass times `rho / (rho + density_scale)` times `illumination / (illumination + mass_scale)`. Source illumination sums mass-weighted age-dependent luminosities with a Gaussian distance kernel. OIII uses the harder subset of that illumination. This produces gas-linked regions, not a calibrated emission measure, metallicity diagnostic, or line-ratio prediction. It deliberately does not equate broadband blue arm light with oxygen emission.

## Save and reuse a look

Enter a path in **Look file**, then **Save look** or **Load look**. `.look` files are plain `key value` text and preserve the numeric optical controls and line palettes, including exposure, bloom, smoothing, dust, and stellar detail. Initial populations and galaxy colors belong to the scene/recording; camera and visibility toggles are separate. Invalid look files are rejected without partially applying them.

```powershell
.\build\bin\Release\physics_view.exe build/merger.traj --render build/preview.png --shot face --time 4 --width 1200 --height 1000 --look examples/nebular.look
```

Batch options are processed in order: put `--look` before any overrides. `--save-look path.look` saves the final settings. The same drawing path handles screen, PNG, and video export. An explicit command for a quieter picture is:

```powershell
.\build\bin\Release\physics_view.exe build/merger.traj --render build/broadband.png --shot face --time 4 --look examples/broadband.look
```

## Recording and approximation limits

New recordings are **PHYSTRJ7**. In addition to V6 data, each body stores its initial young fraction and age, each snapshot stores gas fractions, and the recording stores a single list of formation cohorts. Stop/Continue retains the whole history. The tracer pass preserves cohorts and physical gas fractions while dividing initial stellar light between parent and tracer particles. V1–V6 remain readable; missing histories stay missing. The glTF sphere export and older particle/volume picture modes do not reproduce the optical population renderer; use Optical PNG/MP4 for the final appearance.

The hybrid approximation keeps formed stars co-moving with their residual gas carrier. They do not acquire independently integrated collisionless orbits. Initial young disk particles and their display tracers do have collisionless motion. Stellar evolution is an exponential proxy, not an IMF/population-synthesis table. Emission uses no oxygen abundance, ionization balance, photon conservation, or photoionization heating. It is an aesthetic, gas-linked visualization of a simplified simulation. No continuing spiral potential is imposed, so arms can shear or disperse and a galaxy with little new formation naturally loses ionizing light.

## Historical verification on 2026-10-12

These results predate the merger-only cleanup. Local generated artifacts mentioned below are not included in this source distribution; the historical isolated preset is no longer shipped. Use the validator to extract current isolated controls.

- All 85 C++ test cases pass. Coverage includes fractional mass/momentum/thermal-feedback accounting, hybrid SPH pairwise work and momentum, deterministic initial populations, distinct light lifetimes, mass-weighted illumination, absent-gas/source behavior, no future cohorts illuminating earlier frames, V7 roundtrip, and exact in-memory continuation.
- Actual OpenGL PNG checks cover independent H-alpha/OIII colors, blue continuum, exact saved-look pixel reproduction, invalid look rejection, plus existing foreground/background/embedded dust and V3/V5/V6 playback checks.
- The 2,864-body isolated preset through time 8 formed 0.000867918 stellar mass out of 0.15 initial cold gas; remaining gas was 0.149132082. The budget-corrected energy residual was -0.00009809%.
- Through time 4, the residual decreased from -0.00012062% at dt=0.005 to -0.00000841% at dt=0.0025. Formed masses were 0.000526245 and 0.000537960 respectively (about 2.2% difference). This checks this run, not a general convergence order for thresholded star formation.
- A 5,728-body merger through time 2 had a residual of -0.00002356%; formed mass plus cold gas remained 0.30. The new model has **not** been validated through the entire 80-unit encounter in this change.
- The refinement checker verifies every physical snapshot byte, gas fraction, and cohort against the source recording. The eight-unit preview adds 5,760 massless tracers, with unchanged physical trajectories. The preview uses the new young-star palette through `--restyle`.

Diagnostics and recordings are under `build/luminous/`. The original seeded-spiral validation reports describe the earlier model and should not be treated as validation of the new star-formation mode.
