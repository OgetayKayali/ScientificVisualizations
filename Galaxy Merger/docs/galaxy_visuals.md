# Optical galaxy visualizations

Galaxies now open in **Appearance → Picture → Optical**. This is a stylized optical-light reconstruction of the live simulation. It preserves distinct stellar populations and foreground dust without putting the whole galaxy in one coarse volume grid. The other views remain available as **Particles** and **Illustrative volume**.

## Try it

Open `build/bin/Release/physics_view.exe`, select **Galaxy merger**, and press **Run**. The current luminous preset has two inclined prograde disks on a bound grazing approach, with 2,400 collisionless particles (1,440 in the stellar disk), 400 cold-gas particles, and 64 hot-gas particles per galaxy. It uses dt=0.005 over 80 code-time units. See [stellar populations](stellar_populations.md) for its controls and validation limits. The commands below use the unseeded comparison, `examples/galaxy_merger.scene`; its older desktop budget is 200 cold-gas particles and approximately 727 stellar-disk particles per galaxy.

For batch use:

```powershell
.\build\bin\Release\physics_run.exe examples/galaxy_merger.scene --trajectory build/galaxy_optical.traj
.\build\bin\Release\physics_view.exe build/galaxy_optical.traj
```

The viewer can also render a PNG using a hidden OpenGL window and exit. A `.scene` input is simulated first; a `.traj` input reuses its recording:

```powershell
Start-Process -Wait -WindowStyle Hidden -FilePath .\build\bin\Release\physics_view.exe -ArgumentList @(
  'build/galaxy_optical.traj', '--render', 'build/merger.png',
  '--time', '55', '--width', '1920', '--height', '1080',
  '--yaw', '1.1', '--pitch', '0.8', '--distance', '42'
)
```

Camera angles are radians. Omit `--distance` for automatic framing. `--picture` accepts `optical`, `particles`, or `volume`; `--dust` and `--exposure` override those appearance settings. Width and height must be between 16 and 8192. The output directory must exist. Batch failures return a nonzero exit code and an error on stderr.

## Appearance

| Control | Effect |
| --- | --- |
| Exposure | Lifts faint stellar material before tone mapping. |
| Bloom | A restrained glow from bright regions; applied after dust. |
| Structure smoothing | Changes local stellar/gas footprints without creating light. |
| Fine stellar structure | Redistributes diffuse disk light into a narrower kernel. Reduce it when sparse tails look patchy. |
| Dust | Strength of gas-derived optical absorption. |
| Reddening | Preferentially absorbs blue light. |
| Nebula emission | Local gas emission near young stars; does not make every gas cloud glow. |
| Young-star lifetime | Fading time in simulation units, not a calibrated lifetime in years. |
| Stellar texture | Strength of compact concentrations on the same particle orbits. Stable particle IDs vary their brightness and size; zero removes them. |
| Show dynamical halo | Illustrative halo emission, off by default. |
| Show hot atmosphere | Illustrative hot-gas emission, off by default. |

Population colors remain editable. Axes start hidden. Automatic optical framing uses the central 98% of visible stellar positions over the movie; hidden halos cannot force the camera far away. The framing stays fixed during playback. Very distant debris may require manual zooming out.

**Minimal / Optimal / Fine / Ultra** now change particle budgets only. Disk thickness, softening, time step, duration, and display radius remain as authored. These are still numerical resolution changes: a low-particle disk can heat more strongly. Check an isolated control with matching parameters when changing the budget.

## Scene orientation

`inclination` tilts a galaxy about X, then `node` rotates it about Z; both are in degrees. `spin` is exactly `1` or `-1` and reverses the disk's azimuthal velocities without changing its density profile. Both stellar and cold-gas disks use the same spin. The GUI exposes **Disk node** and **Reverse spin**. The showcase uses `spin -1` because its orbital angular momentum points toward negative Z.

## Rendering and scientific limits

- Local, component-separated neighbor covariances make elliptical kernels. Stars combine broad envelopes with narrower detail kernels; the default detail fraction is 45%. Gas uses a 3D covariance and five conditional depth slices to follow tilted structures. Their sizes do not depend on a global box enclosing the halo. Compact kernels avoid painting light across arbitrarily large empty regions, although sparse tails remain approximate.
- Each ellipse integrates to one; luminosity is proportional to particle mass and authored population emission. Smoothing and particle count no longer directly create brightness. Compact texture redistributes existing light rather than adding it.
- Stellar emission and gas absorption are composed from back to front into a floating-point light buffer. Dust transmission is wavelength-dependent. It attenuates young and old stars alike. Each gas packet distributes its opacity across five depth slices with normalized weights, so an embedded star receives partial attenuation. Stellar packets still emit in their center plane; this is approximate layered transfer, not a resolved dust volume or full radiative transfer.
- A cold-gas energy proxy reduces dust opacity in heated gas. Young stars illuminate nearby gas. These are artistic approximations, not metallicity-dependent dust chemistry or ionization calculations.
- Newly formed stars fade exponentially from their young color and emission toward warm stellar light. The new luminous presets add sampled initial young ages and fractional star-formation histories; see [stellar populations](stellar_populations.md). These remain simplified age-dependent proxies, not full population synthesis.
- Bounded Hermite interpolation uses saved velocities for smooth optical and particle playback. The legacy illustrative volume still uses snapshot fields and a synthetic component palette; its heat control is not an optical temperature measurement.
- HDR exposure, luminance-based tone mapping, bloom, and a final sRGB conversion complete the optical image. Bloom width scales with output height. A minimum pixel footprint reduces aliasing of compact concentrations.

The gravitational force model is unchanged. Hot gas now receives its pressure-work internal-energy kicks, correcting the previous omission. The optional `physics_trace` pass now integrates a separate massless stellar-light population. A spatial tree accelerates tracer forces and rendering neighbor queries; massive-particle gravity is still a direct sum. Automatic camera choreography remains future work. Pixel resolution cannot substitute for additional dynamical structure in a sparse simulation.

## Recordings and verification

New recordings use `PHYSTRJ7`, adding initial young populations, remaining gas fractions, and stellar formation cohorts to V6. They retain the V5 optical fields: per-body mass and per-sample stellar birth times, gas density, and internal energy accompany the existing phase-space and component data. Grid recordings still store their density/energy fields. V1–V6 files remain readable. Legacy galaxy masses use approximate per-galaxy population weights and stellar births are inferred from recorded component transitions; re-simulate for precise metadata. Birth times survive Stop/Continue through the saved simulation state.

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
python tests/render_smoke.py build/bin/Release/physics_view.exe build/visual-tests
```

The optional image checks require Pillow and an OpenGL 3.3-capable environment. They check emitted light, foreground/background dust ordering, legacy V3 playback, and motion between recorded samples using the actual renderer. C++ tests cover kernel-integrated luminosity, bounded curved playback, hidden halos, stellar fading, optical recording metadata, and spin/orientation. These checks establish implementation behavior; they do not certify astrophysical accuracy or optimal artistic tuning.

## Matched controls and the gas energy audit

Build and run the new validation executable:

```powershell
.\build\bin\Release\physics_validate.exe examples/galaxy_merger.scene build/validation --half-dt --trajectories
```

It runs the merger and one isolated control per galaxy. Controls extract the **exact realized particles** from the parent scene and subtract only the imposed bulk position and velocity; they retain mass, orientation, spin, dispersion, softening, duration, and the parent's resolved gas reference density and star-formation threshold. `--half-dt` repeats all runs at half the step size; `--quarter-dt` runs the original, half, and quarter steps. Physical duration and snapshot cadence stay fixed. `--mode gravity` converts gas to collisionless particles of the same mass and switches off thermal sources; `--mode adiabatic` retains gas pressure but disables cooling and star formation. The default `--mode full` includes cooling and feedback. `--trajectories` saves the recordings for visual comparison; omit it for CSV-only output. Large scenes can take several minutes per run.

`summary.csv` contains initial/final disk median cylindrical radius and median absolute height, measured about the current disk angular-momentum plane and disk center of mass, for isolated controls. Merger shape cells are deliberately empty because the combined disks do not have one useful plane. It also reports momentum changes, newly formed star counts, and extensive thermal-energy budgets. All values use scene code units.

The corrected energy residual is:

```
E_final - E_initial + radiated + formation_removed - feedback - floor_added
```

- `radiated` counts thermal energy actually removed by the cooling operator.
- `feedback` counts energy actually deposited into surviving gas; an unabsorbed feedback event adds nothing.
- `formation_removed` records thermal energy removed with gas converted into stars (whole particles or fractional hybrid populations).
- `floor_added` records energy supplied by the zero-energy clamp and cooling equilibrium floor.

These counters survive in-memory Stop/Continue. Final values are saved in the validation CSV; trajectory files do not contain the counters. They make errors visible rather than enforcing energy conservation. Adaptive smoothing, the Jeans pressure floor, and time integration can still leave residuals. Particle reindexing in controls and different timesteps also change stochastic birth histories; compare bulk behavior, not individual birth events. This test checks timestep sensitivity; particle-count convergence remains separate work.

## Gas integration and controlled audit

For gas-bearing states, the `velocity_verlet` scene setting now selects a second-order mechanical predictor/corrector with half-cooling steps before and after it. Positions use initial acceleration, velocities and internal energy use initial and predicted endpoint derivatives, and gas forces are refreshed after the accepted thermal update. Collisionless integration keeps ordinary velocity Verlet. The gas algorithm is not symplectic, and no global energy rescaling is used. Cooling after a feedback delay now acts on the part of the timestep remaining after the delay expires.

Run the small controlled suite with:

```powershell
.\build\bin\Release\physics_audit.exe build/audit.csv
```

It separates gravity, adiabatic gas, cooling, and feedback at four steps. Energy convergence in these fixtures checks the implementation; it does not establish a convergence order for stochastic star formation or a galaxy encounter. The full matched-control suite is still required.

## Independently integrated light tracers

After producing an unrefined recording, run:

```powershell
.\build\bin\Release\physics_trace.exe examples/galaxy_merger.scene build/galaxy_optical.traj build/galaxy_refined.traj --per-star 4 --dt 0.02 --softening 0.08
.\build\bin\Release\physics_view.exe build/galaxy_refined.traj
```

The source scene supplies and verifies initial conditions and gravity units. The recording must start at those initial conditions and contain physical masses (V5 or later). Refinement uses its recorded massive-particle potential; it does not resimulate or alter the source trajectory. The default tracer softening is twice the scene's gravity softening, and the default tracer step is the scene's step. The explicit example values above are artistic smoothing choices in the showcase's code units.

`--per-star` accepts even counts from 2 to 32. Four adds 5,816 tracers to the showcase. Each initial disk packet keeps 15% of its light; its tracers share the other 85%. They carry zero gravitational mass, so total dynamical mass and recorded diagnostics remain unchanged, as does total intrinsic disk luminosity. Bulge, gas, and newly formed stars are not refined. The initial tracer offsets are deterministic, antithetic local position/velocity samples; they preserve each packet's initial mean, but are an approximate reconstruction rather than a new equilibrium distribution function.

Tracers subsequently follow their own velocity-Verlet orbits. They feel the evolving, softened gravity of **all** recorded massive particles, interpolated between snapshots, with no parent tether and no backreaction. A median-split monopole tree uses opening angle `--theta 0.4`; `--theta 0` evaluates the exact softened sum. Tree approximation and finite source snapshot spacing introduce separate errors from tracer timestep error. Increasing the tracer count cannot recover missing self-gravitating structure or make the underlying simulation more accurate.

Use the Optical picture for refined light. It uses explicit light weights, exact tree neighbor queries, and 96 neighbors for refined stellar kernels to avoid excessive grain. The older illustrative pictures are diagnostic alternatives. Refinement is currently an offline CLI pass; load its output into the viewer. V6 stores the tracer settings, and the Optical panel identifies refined recordings. Source files are preserved; output must be a different path.


## Isolation profiles, populations, and composition

`physics_validate scene output --isolate 0 --resolution-study --duration 20 --trajectories`
runs the selected progenitor with 1x, 2x, and 4x the massive-particle counts. It scales
collisionless, cold-gas, and hot-gas counts together, retaining total component-model
mass, timestep, geometry, and softening. Integer allocation can slightly change the
collisionless component mass fractions. Each level is a fresh draw from the same seed,
not nested particles. Gas density reference values are remeasured for each resolution.
Without `--isolate`, a resolution study tests every progenitor, without rerunning the
merger. `--duration` must be a whole number of timesteps. `--merger-only` runs only the
encounter and cannot be combined with the isolation or resolution flags.

Each isolated run writes `_history.csv` (median radius and absolute height at every
snapshot) and `_profiles.csv` (annular surface density, median absolute height, mean
rotation, radial and vertical velocity dispersions, and particle counts). Measurements
use the **original stellar disk**, its center of mass, and its instantaneous
angular-momentum plane. Velocity moments and annular thickness are mass weighted.
Bulk translation and tilt do not count as heating. Warps and genuine internal bending
can still contribute to the measured thickness; these are not pure two-body-heating
estimators. Empty bins carry count zero and should be omitted from interpretation.

```powershell
python tools/plot_isolation.py build/stability/baseline --output build/stability/resolution.png
```

The plotting tool needs matplotlib; it omits radial bins containing fewer than 16 stars.
A single seed and a short integration can identify a useful direction, but cannot
establish numerical convergence or long-term equilibrium.

Galaxy blocks accept `population_variation 0.65` (range 0 to 1; default 0). This gives
the initial stellar disk a warmer inner population and a cooler outer mix with stable
particle-to-particle variation. Colors are assigned once, consume no dynamics RNG,
and move with the stars into streams. Emission and broadband luminance are preserved.
These are artistic population labels, not measured ages or metallicities. Young stars
still use their recorded birth time and existing fading model. Bulges use a smoother
32-neighbor light kernel with reduced reconstruction anisotropy to reduce sparse
central streaks without adding light. Fixed sub-code-value dither at the final sRGB output reduces contour banding in smooth bright cores; empty backgrounds remain black.

`Dust lane structure` moves a fraction of the existing cold-gas optical depth into a
narrower, gas-aligned kernel. The fraction increases with density relative to the initial
median cold-gas density. Every finite-depth layer conserves its integrated optical
depth and emitted light; this is still approximate radiative transfer. At zero, the
original broad gas kernels are restored. The CLI equivalent is `--dust-structure 0.55`.

The optical panel has **Frame current light**, **Face-on disk**, **Edge-on disk**,
**First passage**, and **After passage** buttons. Disk views select the first named
galaxy and use its stellar angular momentum. Dark matter cannot determine the crop.
First passage finds the first minimum in smoothed progenitor-core separation (bulge
centroids, or disk centroids when no two bulges exist). After passage advances by 18%
of the recording duration; it is a suggested tail composition, not a measured physical
event. Cameras remain fixed during playback. `--shot wide|face|edge|passage|tails`
provides the same presets for PNG export. Passage and tails override `--time`; other
shots use it. `--distance` overrides the preset distance. Face/edge override yaw/pitch.

An optional `vertical_support jeans` initializer integrates
`sigma_z^2(R,z) = integral_z^infinity rho(R,z') Kz(R,z') dz' / rho(R,z)` for the authored
sech-squared density. It averages 16 rotated, midplane-reflected copies of the realized
softened field, includes the hot atmosphere, and interpolates a 36-radius, 65-height
table extending to eight disk scale heights. It neglects the radial-vertical stress
tilt term and draws Gaussian vertical speeds; this is a Jeans-moment construction,
not an exact equilibrium distribution function. The in-plane Shu prescription is
unchanged. The governing moment equation is described in
[Bovy's vertical-equilibrium treatment](https://galaxiesbook.org/chapters/II-04.-Equilibria-of-Galactic-Disks_4-The-vertical-equilibrium-state-of-disks.html).
`vertical_support legacy` remains the default: the new option did not improve the
short isolation controls and is exposed for experiments, not silently enabled.


To iterate on appearance without repeating the physical simulation, add `--restyle`
to `physics_trace`. It copies the matching scene's current colors, emission, young-star
appearance, display radius, and roughness into a new recording before adding tracers.
Names, masses, and initial positions/velocities must still match; the source recording
is preserved. Without this flag, recorded appearance is kept. This also supports
changes to `population_variation` on an existing physical run.

```powershell
physics_trace examples/galaxy_luminous_merger.scene build/merger.traj build/merger_refined.traj --per-star 4 --dt 0.02 --softening 0.2 --theta 0.4 --restyle
python tools/verify_refinement.py build/merger.traj build/merger_refined.traj --output build/merger_verification.json
```

The verifier compares every recorded physical state and diagnostic byte across all
snapshots, checks original masses and zero tracer masses, and reports the intrinsic
broadband luminosity change. Restyling emission can intentionally change that last
quantity; the showcase population-color recipe conserves it.


HDR population colors also export to glTF with unit-range base/emissive factors and
[`KHR_materials_emissive_strength`](https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_emissive_strength/README.md)
for the extra emission. Export no longer writes out-of-range material color factors.
The native Optical picture remains the intended renderer for density-shaped starlight
and dust; a glTF sphere export does not reproduce those effects.
