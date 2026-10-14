"""Plot physics_validate structure histories and radial profiles (requires matplotlib).

python tools/plot_isolation.py build/stability/baseline --output build/stability/resolution.png
Empty and poorly sampled radial bins are omitted. All axes use simulation code units.
"""
import argparse
import csv
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def rows(path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directories", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--title", default="Isolated stellar disks")
    args = parser.parse_args()
    fig, axes = plt.subplots(2, 2, figsize=(13, 8.5), layout="constrained")
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    count = 0
    for directory in args.directories:
        for run in rows(directory / "summary.csv"):
            name = run["run"]
            history_path = directory / f"{name}_history.csv"
            if not history_path.exists():
                continue
            history = rows(history_path)
            profiles = rows(directory / f"{name}_profiles.csv")
            color = colors[count % len(colors)]
            label = f"{directory.name}/{name} ({int(run['bodies']):,})"
            time = [float(row["time"]) for row in history]
            for axis, key in [(axes[0, 0], "height50"), (axes[0, 1], "r50")]:
                initial = float(history[0][key])
                axis.plot(time, [float(row[key]) / initial for row in history],
                          color=color, label=label, linewidth=1.4)
            for final in [False, True]:
                selected = [row for row in profiles
                            if float(row["time"]) == time[-1 if final else 0] and int(row["count"]) >= 16]
                radius = [(float(row["inner"]) + float(row["outer"])) / 2 for row in selected]
                for axis, key in [(axes[1, 0], "surface_density"), (axes[1, 1], "rotation")]:
                    axis.plot(radius, [float(row[key]) for row in selected],
                              color=color, linestyle="-" if final else "--",
                              alpha=1 if final else .5, marker="o" if final else None, markersize=3)
            count += 1
    if not count:
        raise SystemExit("No isolated history/profile files found")
    for axis, title in zip(axes.flat, ["Median thickness / initial", "Median radius / initial",
                                      "Surface density: initial dashed, final solid", "Mean rotation: initial dashed, final solid"]):
        axis.set_title(title, loc="left", fontsize=11)
        axis.grid(alpha=.18)
        axis.spines[["top", "right"]].set_visible(False)
    for axis in axes[0]:
        axis.set_xlabel("Time [code units]")
        axis.axhline(1, color="black", alpha=.25, linewidth=.8)
    for axis in axes[1]:
        axis.set_xlabel("Cylindrical radius [code units]")
    axes[1, 0].set_yscale("log")
    axes[1, 0].set_ylabel("Mass / area [code units]")
    axes[1, 1].set_ylabel("Speed [code units]")
    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="outside lower center", ncol=min(2, count), fontsize=8)
    fig.suptitle(args.title + "\nOriginal disk; instantaneous angular-momentum plane; radial bins require 16 stars",
                 fontsize=13)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=160)
    plt.close(fig)
    print(args.output.resolve())


if __name__ == "__main__":
    main()
