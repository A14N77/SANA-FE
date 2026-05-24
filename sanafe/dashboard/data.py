"""Realistic mock simulation data for the dashboard shell.

Mirrors the dict shape returned by ``chip.sim()`` so the panels can be
wired against ``sanafe.data.*`` functions today and swapped to a real
simulation result later with zero changes to the panel code.
"""

from __future__ import annotations

from types import SimpleNamespace
from typing import Any, Dict

import numpy as np


_LOIHI_TILES = 32  # 8 x 4 mesh
_LOIHI_CORES_PER_TILE = 4
_DEFAULT_TIMESTEPS = 20
_DEFAULT_GROUPS = ("0", "1", "2")
_NEURONS_PER_GROUP = (32, 24, 16)


def _spike_trace(rng, n_timesteps, groups, neurons_per_group):
    trace = []
    rates = (0.18, 0.12, 0.22)
    for t in range(n_timesteps):
        row = []
        for g, n_count, rate in zip(groups, neurons_per_group, rates):
            firing = rng.random(n_count) < rate
            offsets = np.flatnonzero(firing)
            for off in offsets:
                row.append(SimpleNamespace(group_name=g, neuron_offset=int(off)))
        trace.append(row)
    return trace


def _potential_trace(rng, n_timesteps, n_probed):
    base = rng.uniform(-0.5, 0.5, size=n_probed)
    trace = []
    v = base.copy()
    for _ in range(n_timesteps):
        v = 0.9 * v + rng.normal(0.05, 0.15, size=n_probed)
        v = np.clip(v, -1.2, 1.5)
        trace.append(v.tolist())
    return trace


def _perf_trace(rng, n_timesteps):
    fired = rng.integers(40, 120, size=n_timesteps).tolist()
    spikes = (np.array(fired) + rng.integers(0, 40, size=n_timesteps)).tolist()
    hops = (np.array(spikes) * rng.integers(2, 5, size=n_timesteps)).tolist()
    synapse_e = (rng.uniform(8e-12, 2.4e-11, size=n_timesteps)).tolist()
    dendrite_e = (rng.uniform(2e-12, 6e-12, size=n_timesteps)).tolist()
    soma_e = (rng.uniform(1.5e-11, 3.8e-11, size=n_timesteps)).tolist()
    network_e = (rng.uniform(3e-12, 1.1e-11, size=n_timesteps)).tolist()
    total_e = [s + d + so + n for s, d, so, n
               in zip(synapse_e, dendrite_e, soma_e, network_e)]
    sim_time = (np.arange(1, n_timesteps + 1) * 8e-9).tolist()
    return {
        "timestep": list(range(1, n_timesteps + 1)),
        "fired": fired,
        "updated": rng.integers(400, 800, size=n_timesteps).tolist(),
        "hops": hops,
        "spikes": spikes,
        "sim_time": sim_time,
        "synapse_energy": synapse_e,
        "dendrite_energy": dendrite_e,
        "soma_energy": soma_e,
        "network_energy": network_e,
        "total_energy": total_e,
    }


def _message_trace(rng, n_timesteps, n_messages_per_step):
    trace = []
    mid = 0
    for t in range(1, n_timesteps + 1):
        row = []
        for _ in range(n_messages_per_step):
            src_tile = int(rng.integers(0, _LOIHI_TILES))
            dst_tile = int(rng.integers(0, _LOIHI_TILES))
            src_x, src_y = src_tile % 8, src_tile // 8
            dst_x, dst_y = dst_tile % 8, dst_tile // 8
            hops = abs(src_x - dst_x) + abs(src_y - dst_y)
            row.append({
                "timestep": t,
                "mid": mid,
                "src_neuron_group_id": str(rng.integers(0, 3)),
                "src_neuron_offset": int(rng.integers(0, 32)),
                "src_tile_id": src_tile,
                "src_core_offset": int(rng.integers(0, _LOIHI_CORES_PER_TILE)),
                "dest_tile_id": dst_tile,
                "dest_core_offset": int(rng.integers(0, _LOIHI_CORES_PER_TILE)),
                "src_x": src_x, "src_y": src_y,
                "dest_x": dst_x, "dest_y": dst_y,
                "hops": hops,
                "spikes": int(rng.integers(1, 4)),
                "generation_delay": float(rng.uniform(5e-9, 1.5e-8)),
                "processing_delay": float(rng.uniform(2e-9, 1.2e-8)),
                "network_delay": float(hops * rng.uniform(4e-9, 7e-9)),
                "blocking_delay": float(rng.uniform(0, 4e-9)),
                "send_timestamp": float(t * 8e-9),
                "received_timestamp": float(t * 8e-9 + hops * 5e-9),
                "processed_timestamp": float(t * 8e-9 + hops * 5e-9 + 6e-9),
            })
            mid += 1
        trace.append(row)
    return trace


def mock_sim_results(
    seed: int = 7,
    n_timesteps: int = _DEFAULT_TIMESTEPS,
    groups=_DEFAULT_GROUPS,
    neurons_per_group=_NEURONS_PER_GROUP,
    messages_per_step: int = 60,
) -> Dict[str, Any]:
    """Return a realistic ``chip.sim()``-shaped dict for dashboard previews."""
    rng = np.random.default_rng(seed)
    n_probed = sum(neurons_per_group)
    perf = _perf_trace(rng, n_timesteps)
    return {
        "timestep_start": 1,
        "timesteps_executed": n_timesteps,
        "spike_trace": _spike_trace(rng, n_timesteps, groups, neurons_per_group),
        "potential_trace": _potential_trace(rng, n_timesteps, n_probed),
        "perf_trace": perf,
        "message_trace": _message_trace(rng, n_timesteps, messages_per_step),
        "energy": {
            "total": sum(perf["total_energy"]),
            "synapse": sum(perf["synapse_energy"]),
            "dendrite": sum(perf["dendrite_energy"]),
            "soma": sum(perf["soma_energy"]),
            "network": sum(perf["network_energy"]),
        },
        "spikes": int(sum(perf["spikes"])),
        "neurons_fired": int(sum(perf["fired"])),
        "neurons_updated": int(sum(perf["updated"])),
        "packets_sent": int(sum(perf["spikes"])),
        "sim_time": perf["sim_time"][-1],
    }


def mock_tile_energy(seed: int = 7) -> np.ndarray:
    """Per-tile energy values for the 8x4 NoC heatmap."""
    rng = np.random.default_rng(seed + 1)
    base = rng.uniform(2e-11, 9e-11, size=(4, 8))
    # Bias a hot region to make the layout visually interesting.
    base[1:3, 2:5] *= 1.6
    return base
