"""CLI regression for resolution scaling, duration, mode selection, and radial units.

python tests/validation_smoke.py build/bin/Release/physics_validate.exe build/validation-smoke
"""
import csv
from pathlib import Path
import subprocess
import sys

exe = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)
scene = out / "source.scene"
header = "integrator velocity_verlet\ndt 0.005\nsteps 2\nsample_every 1\ngravitational_constant 1\nforce gravity\nsoftening 0.08\n"
blocks = []
for name, scale in [("left", 1), ("right", 2)]:
    blocks.append(f"""galaxy {name}
particles 30
halo_mass 2
halo_scale 2
disk_mass 1
disk_scale_radius {scale}
disk_height 0.2
gas_mass 0.15
gas_particles 48
hot_gas_mass 0.08
hot_gas_particles 48
population_variation 0.65
target_q 1.45
disk_particle_fraction 0.6
spiral_strength 0.5
spiral_pitch 28
spiral_phase 20
spiral_arms 2
vertical_support legacy
""")
scene.write_text(header + "\n".join(blocks), encoding="utf-8")
subprocess.run([str(exe), str(scene), str(out / "study"), "--isolate", "1", "--resolution-study",
                "--duration", "0.01", "--mode", "gravity"], check=True)
with (out / "study/summary.csv").open(newline="") as f:
    results = list(csv.DictReader(f))
assert len(results) == 3
for factor, row in zip([1, 2, 4], results):
    assert int(row["bodies"]) == 126 * factor
    assert float(row["duration"]) == 0.01
    assert float(row["radiated"]) == 0
    assert int(row["young_stars"]) == 0
    with (out / f"study/{row['run']}_profiles.csv").open(newline="") as f:
        profiles = list(csv.DictReader(f))
    assert all(key in profiles[0] for key in ["a2", "a4", "phase2", "phase4", "noise", "circular_speed", "kappa", "q"])
    assert float(profiles[0]["outer"]) == 1.0  # Half the SECOND galaxy's scale radius.
    assert {float(r["time"]) for r in profiles} == {0, .005, .01}
for options in [["--duration", "nan"], ["--duration", "0.006"], ["--isolate", "-1"],
                ["--isolate", "2"], ["--merger-only", "--resolution-study"]]:
    result = subprocess.run([str(exe), str(scene), str(out / "invalid"), *options], capture_output=True)
    assert result.returncode != 0, options
for key, value in [("target_q", "-1"), ("disk_particle_fraction", "1"), ("spiral_strength", "1"), ("spiral_pitch", "0"), ("spiral_arms", "3")]:
    invalid_scene = out / "invalid.scene"
    invalid_scene.write_text(scene.read_text() + f"\n{key} {value}\n")
    result = subprocess.run([str(exe), str(invalid_scene), str(out / "invalid")], capture_output=True)
    assert result.returncode != 0, key
print("PASS: CLI resolution scaling, duration, gravity control, radial units and invalid options")
