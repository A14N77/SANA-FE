"""Plotly panel renderers wired to ``sanafe.data`` conversions."""

from __future__ import annotations

import numpy as np
import plotly.graph_objects as go
from dash import dcc

from sanafe.data import (
    spikes_to_dataframe,
    potentials_to_dataframe,
    performance_to_dataframe,
    messages_to_dataframe,
)
from sanafe.dashboard import theme as t


def _base_layout(**overrides) -> dict:
    layout = {k: v for k, v in t.PLOTLY_LAYOUT.items()}
    layout.update(overrides)
    return layout


def noc_panel(tile_energy: np.ndarray, height: int = 500) -> dcc.Graph:
    """8x4 mesh chip layout colored by per-tile energy."""
    n_rows, n_cols = tile_energy.shape
    fig = go.Figure(
        data=go.Heatmap(
            z=tile_energy * 1e12,  # to pJ
            colorscale=[
                [0.0, t.OFF_WHITE],
                [0.4, t.UT_BURNT_ORANGE_SOFT],
                [1.0, t.UT_BURNT_ORANGE],
            ],
            colorbar={
                "title": {"text": "Energy (pJ)", "font": {"size": 11}},
                "thickness": 12,
                "outlinewidth": 0,
            },
            xgap=4, ygap=4,
            hovertemplate="tile (%{x},%{y})<br>%{z:.1f} pJ<extra></extra>",
        )
    )
    fig.update_layout(_base_layout(
        height=height,
        margin={"l": 32, "r": 16, "t": 8, "b": 32},
        xaxis={"showgrid": False, "zeroline": False, "tickvals": list(range(n_cols))},
        yaxis={"showgrid": False, "zeroline": False, "tickvals": list(range(n_rows)),
               "scaleanchor": "x", "scaleratio": 1},
    ))
    return dcc.Graph(figure=fig, config={"displayModeBar": False})


def raster_panel(results: dict, height: int = 220) -> dcc.Graph:
    df = spikes_to_dataframe(results)
    groups = sorted(df["group"].unique())
    palette = t.PLOTLY_COLORWAY
    color_of = {g: palette[i % len(palette)] for i, g in enumerate(groups)}

    # Stable per-group y ordering
    y_of: dict = {}
    cursor = 0
    for g in groups:
        offsets = sorted(df.loc[df["group"] == g, "neuron_offset"].unique())
        for o in offsets:
            y_of[(g, int(o))] = cursor
            cursor += 1

    df = df.assign(
        y=[y_of[(r.group, int(r.neuron_offset))] for r in df.itertuples()],
        color=[color_of[g] for g in df["group"]],
    )

    fig = go.Figure()
    for g in groups:
        sub = df[df["group"] == g]
        fig.add_trace(go.Scatter(
            x=sub["timestep"], y=sub["y"],
            mode="markers",
            marker={"symbol": "line-ns", "size": 8, "color": color_of[g],
                    "line": {"width": 1.5, "color": color_of[g]}},
            name=f"group {g}",
            hovertemplate="t=%{x}<br>n=%{y}<extra></extra>",
        ))
    fig.update_layout(_base_layout(
        height=height,
        showlegend=False,
        xaxis={"title": {"text": "timestep", "font": {"size": 11}}, "gridcolor": t.BORDER},
        yaxis={"title": {"text": "neuron", "font": {"size": 11}}, "gridcolor": t.BORDER},
    ))
    return dcc.Graph(figure=fig, config={"displayModeBar": False})


def potential_panel(results: dict, height: int = 220) -> dcc.Graph:
    df = potentials_to_dataframe(results)
    fig = go.Figure(
        data=go.Heatmap(
            z=df.values.T,
            colorscale=[
                [0.0, "#2C3E70"],
                [0.5, t.OFF_WHITE],
                [1.0, t.UT_BURNT_ORANGE],
            ],
            zmid=0.0,
            colorbar={"thickness": 10, "outlinewidth": 0},
            hovertemplate="t=%{x}<br>n=%{y}<br>v=%{z:.2f}<extra></extra>",
        )
    )
    fig.update_layout(_base_layout(
        height=height,
        xaxis={"title": {"text": "timestep", "font": {"size": 11}}},
        yaxis={"title": {"text": "neuron", "font": {"size": 11}}},
    ))
    return dcc.Graph(figure=fig, config={"displayModeBar": False})


def energy_panel(results: dict, height: int = 220) -> dcc.Graph:
    df = performance_to_dataframe(results)
    components = ["synapse_energy", "dendrite_energy", "soma_energy", "network_energy"]
    labels = ["Synapse", "Dendrite", "Soma", "Network"]
    colors = t.PLOTLY_COLORWAY
    timesteps = df["timestep"].tolist()

    fig = go.Figure()
    cumulative = np.zeros(len(df))
    for comp, label, color in zip(components, labels, colors):
        values = df[comp].values * 1e12  # to pJ
        fig.add_trace(go.Scatter(
            x=timesteps, y=(cumulative + values).tolist(),
            mode="lines", name=label,
            line={"width": 0.5, "color": color},
            fill="tonexty" if cumulative.any() else "tozeroy",
            fillcolor=color, opacity=0.85,
            hovertemplate=f"{label}: %{{y:.2f}} pJ<extra></extra>",
        ))
        cumulative = cumulative + values
    fig.update_layout(_base_layout(
        height=height,
        showlegend=True,
        legend={"orientation": "h", "y": -0.18, "font": {"size": 10}},
        xaxis={"title": {"text": "timestep", "font": {"size": 11}}},
        yaxis={"title": {"text": "energy (pJ)", "font": {"size": 11}}},
    ))
    return dcc.Graph(figure=fig, config={"displayModeBar": False})


def throughput_panel(results: dict, height: int = 220) -> dcc.Graph:
    df = performance_to_dataframe(results)
    timesteps = df["timestep"].tolist()
    fig = go.Figure()
    for metric, color in zip(("fired", "spikes", "hops"),
                             t.PLOTLY_COLORWAY):
        fig.add_trace(go.Scatter(
            x=timesteps, y=df[metric].tolist(),
            mode="lines+markers", name=metric.title(),
            line={"width": 2, "color": color},
            marker={"size": 5, "color": color},
        ))
    fig.update_layout(_base_layout(
        height=height,
        legend={"orientation": "h", "y": -0.18, "font": {"size": 10}},
        xaxis={"title": {"text": "timestep", "font": {"size": 11}}},
        yaxis={"title": {"text": "count", "font": {"size": 11}}},
    ))
    return dcc.Graph(figure=fig, config={"displayModeBar": False})


def latency_panel(results: dict, height: int = 220) -> dcc.Graph:
    df = messages_to_dataframe(results)
    if "mid" in df.columns:
        df = df[df["mid"] >= 0]
    metrics = ["generation_delay", "processing_delay", "network_delay", "blocking_delay"]
    fig = go.Figure()
    for metric, color in zip(metrics, t.PLOTLY_COLORWAY):
        vals = df[metric].values.astype(float)
        vals = vals[np.isfinite(vals)] * 1e9  # to ns
        if len(vals) == 0:
            continue
        fig.add_trace(go.Histogram(
            x=vals, name=metric.replace("_delay", "").title(),
            marker={"color": color, "line": {"width": 0.5, "color": "white"}},
            opacity=0.65, nbinsx=30,
        ))
    fig.update_layout(_base_layout(
        height=height,
        barmode="overlay",
        legend={"orientation": "h", "y": -0.18, "font": {"size": 10}},
        xaxis={"title": {"text": "delay (ns)", "font": {"size": 11}}},
        yaxis={"title": {"text": "count", "font": {"size": 11}}},
    ))
    return dcc.Graph(figure=fig, config={"displayModeBar": False})
