"""Plot measured disk Q, two-/four-arm amplitudes and arm phases from validation CSVs.
Requires numpy and matplotlib. Amplitudes describe all non-axisymmetric structure,
including bars; a spiral also needs a coherent phase change with radius.
"""
import argparse
import csv
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("profiles", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    fig, axes = plt.subplots(2, 2, figsize=(13, 8), layout="constrained")
    for path in args.profiles:
        with path.open(newline="") as f:
            rows = list(csv.DictReader(f))
        if not rows or "q" not in rows[0]:
            parser.error(f"{path} lacks the new spiral diagnostics")
        label = path.parent.name
        times = sorted({float(r["time"]) for r in rows})
        selected = lambda t: [r for r in rows if float(r["time"]) == t and int(r["count"]) >= 32]
        # Exclude center and faint outskirts; report mean ANNULAR amplitude, not a global mode.
        scale = max(float(r["outer"]) for r in rows) / 10
        amplitudes = []
        for t in times:
            bins = [r for r in selected(t) if float(r["inner"]) >= scale and float(r["outer"]) <= 4*scale]
            amplitudes.append([np.average([float(r[key]) for r in bins], weights=[int(r["count"]) for r in bins])
                               if bins else np.nan for key in ["a2", "a4", "noise"]])
        color = axes[0, 1].plot(times, np.array(amplitudes)[:, 0], label=label)[0].get_color()
        axes[0, 1].plot(times, np.array(amplitudes)[:, 2], color=color, linestyle=":", alpha=.6)
        axes[1, 1].plot(times, np.array(amplitudes)[:, 1], color=color, label=label)
        for t, style in [(times[0], "--"), (times[-1], "-")]:
            bins = [r for r in selected(t) if float(r["inner"]) >= .5*scale and float(r["outer"]) <= 4*scale]
            radius = [(float(r["inner"])+float(r["outer"]))/2/scale for r in bins]
            axes[0, 0].plot(radius, [float(r["q"]) for r in bins], linestyle=style, color=color,
                            marker="o", markersize=3, label=f"{label}: t={t:g}")
        # Initial phase profile relative to first populated outer bin; phase is modulo pi.
        bins = [r for r in selected(times[0]) if float(r["inner"]) >= .5*scale and float(r["outer"]) <= 4*scale and float(r["a2"]) > 2*float(r["noise"])]
        radii = [(float(r["inner"])+float(r["outer"]))/2/scale for r in bins]
        phase = np.unwrap([2*float(r["phase2"]) for r in bins])/2
        if len(phase):
            axes[1, 0].plot(radii, (phase-phase[0])*180/np.pi, marker="o", color=color, label=label)
    titles = ["Measured stellar Q: initial dashed / final solid", "Mean annular A2: 1–4 disk scale radii",
              "Initial two-arm phase (A2 > twice RMS noise)", "Mean annular A4: 1–4 disk scale radii"]
    for ax, title in zip(axes.flat, titles):
        ax.set_title(title, loc="left", fontsize=11)
        ax.grid(alpha=.2)
        ax.spines[["top", "right"]].set_visible(False)
        ax.legend(fontsize=8)
    for ax in axes[:, 0]:
        ax.set_xlabel("Radius / initial disk scale")
    for ax in axes[:, 1]:
        ax.set_xlabel("Time [code units]")
        ax.set_ylim(bottom=0)
    axes[0, 0].set_ylabel("Thin stellar-disk Q proxy")
    axes[1, 0].set_ylabel("Phase [degrees]")
    axes[0, 0].axhline(1, color="gray", linestyle=":")
    fig.suptitle("Spiral structure and disk heating\nOriginal massive disk only; dotted A2 curves show random-phase RMS noise", fontsize=13)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=160)
    print(args.output.resolve())

if __name__ == "__main__":
    main()
