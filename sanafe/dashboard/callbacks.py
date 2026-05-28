"""Dash callbacks wiring controls to panel updates."""

from __future__ import annotations

from dash import Input, Output

from sanafe.dashboard.data import mock_sim_results, mock_tile_energy
from sanafe.dashboard.panels import (
    noc_panel,
    raster_panel,
    potential_panel,
    energy_panel,
    throughput_panel,
)


def _slice_results(results: dict, time_range: list[int]) -> dict:
    start, stop = time_range
    sliced = dict(results)
    if "spike_trace" in results:
        sliced["spike_trace"] = results["spike_trace"][start:stop]
    if "potential_trace" in results:
        sliced["potential_trace"] = results["potential_trace"][start:stop]
    if "message_trace" in results:
        sliced["message_trace"] = results["message_trace"][start:stop]
    if "perf_trace" in results:
        sliced["perf_trace"] = {
            k: v[start:stop] for k, v in results["perf_trace"].items()
        }
    return sliced


def register(app, results=None):
    """Attach refresh + time-range callbacks. ``results`` is fetched lazily."""
    cache = {"results": results, "tile_energy": None, "seed": 7}

    def _get_results():
        if cache["results"] is None:
            cache["results"] = mock_sim_results(seed=cache["seed"])
            cache["tile_energy"] = mock_tile_energy(seed=cache["seed"])
        elif cache["tile_energy"] is None:
            cache["tile_energy"] = mock_tile_energy(seed=cache["seed"])
        return cache["results"], cache["tile_energy"]

    @app.callback(
        Output("noc-panel", "children"),
        Output("energy-panel", "children"),
        Output("throughput-panel", "children"),
        Output("raster-panel", "children"),
        Output("potential-panel", "children"),
        Input("refresh-btn", "n_clicks"),
        Input("run-btn", "n_clicks"),
        Input("time-range", "value"),
    )
    def _update_all(refresh_clicks, run_clicks, time_range):
        if run_clicks and run_clicks > 0:
            cache["seed"] += 1
            cache["results"] = None
            cache["tile_energy"] = None
        results, tile_energy = _get_results()
        sliced = _slice_results(results, time_range or [0, 20])
        return (
            noc_panel(tile_energy),
            energy_panel(sliced),
            throughput_panel(sliced),
            raster_panel(sliced),
            potential_panel(sliced),
        )

    return app
