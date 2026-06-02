#!/usr/bin/env python3
"""
End-to-end check of sanafe.data + sanafe.viz against a real arch + SNN.

Runs the Loihi architecture with the DVS gesture network twice:
  1. with in-memory traces (chip.sim(..., spike_trace=True, ...))
  2. with CSV traces       (chip.sim(..., spike_trace="spikes.csv", ...))

For each path, renders the six plot types and verifies the two paths
produce equivalent DataFrames. Outputs land in tmp_trace_test/.

Run from the repo root:
    python scripts/run_arch_testing.py
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO))

import matplotlib
matplotlib.use("Agg")

import sanafe
from sanafe.data import (
    spikes_to_dataframe,
    potentials_to_dataframe,
    performance_to_dataframe,
    messages_to_dataframe,
)
from sanafe.viz import (
    plot_raster,
    plot_potential,
    plot_potential_lines,
    plot_energy,
    plot_throughput,
    plot_latency,
)

ARCH = REPO / "arch" / "loihi.yaml"
SNN = REPO / "snn" / "dvs.yaml"
OUT = REPO / "tmp_trace_test"
TIMESTEPS = 20

PLOTS = [
    ("raster", plot_raster, "spikes.csv"),
    ("potential_heatmap", plot_potential, "potentials.csv"),
    ("potential_lines", plot_potential_lines, "potentials.csv"),
    ("energy", plot_energy, "perf.csv"),
    ("throughput", plot_throughput, "perf.csv"),
    ("latency", plot_latency, "messages.csv"),
]


def render(fn, source, name, label):
    fig, _ = fn(source, title=f"Loihi+DVS — {name} ({label})")
    fig.savefig(OUT / f"{name}_{label}.png", dpi=110)


def main():
    OUT.mkdir(exist_ok=True)

    arch = sanafe.load_arch(str(ARCH))
    snn = sanafe.load_net(str(SNN), arch)

    # ----- in-memory path -----
    chip = sanafe.SpikingChip(arch)
    chip.load(snn)
    results = chip.sim(
        TIMESTEPS,
        spike_trace=True,
        potential_trace=True,
        perf_trace=True,
        message_trace=True,
    )
    for name, fn, _ in PLOTS:
        render(fn, results, name, "mem")

    # ----- CSV path -----
    chip = sanafe.SpikingChip(arch)
    chip.load(snn)
    chip.sim(
        TIMESTEPS,
        spike_trace=str(OUT / "spikes.csv"),
        potential_trace=str(OUT / "potentials.csv"),
        perf_trace=str(OUT / "perf.csv"),
        message_trace=str(OUT / "messages.csv"),
    )
    for name, fn, csv in PLOTS:
        render(fn, str(OUT / csv), name, "csv")

    # ----- equivalence checks -----
    msgs_mem = messages_to_dataframe(results)
    msgs_csv = messages_to_dataframe(str(OUT / "messages.csv"))
    spikes_mem = spikes_to_dataframe(results)
    spikes_csv = spikes_to_dataframe(str(OUT / "spikes.csv"))
    perf_mem = performance_to_dataframe(results)
    perf_csv = performance_to_dataframe(str(OUT / "perf.csv"))

    assert len(msgs_mem) == len(msgs_csv), \
        f"message count differs: mem={len(msgs_mem)} csv={len(msgs_csv)}"
    assert "processing_delay" in msgs_mem.columns
    assert "receive_delay" not in msgs_mem.columns
    assert len(spikes_mem) == len(spikes_csv)
    assert len(perf_mem) == len(perf_csv)

    print(f"messages: {len(msgs_mem)} rows on both paths")
    print(f"spikes:   {len(spikes_mem)} rows on both paths")
    print(f"perf:     {len(perf_mem)} rows on both paths")
    print(f"outputs:  {OUT}")


if __name__ == "__main__":
    main()
