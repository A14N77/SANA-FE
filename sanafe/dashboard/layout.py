"""Static layout for the SANA-FE dashboard."""

from dash import dcc, html

from sanafe.dashboard import theme as t


def _panel(title: str, body_id: str, body_height: str = "200px") -> html.Div:
    return html.Div(
        [
            html.Div(title, style=t.PANEL_TITLE_STYLE),
            html.Div(id=body_id, style={**t.PANEL_BODY_STYLE, "minHeight": body_height}),
        ],
        style=t.PANEL_STYLE,
    )


def _header() -> html.Header:
    return html.Header(
        [
            html.Div(
                [
                    html.Span("◆", style=t.BRAND_ACCENT_STYLE),
                    html.Span("SANA-FE Dashboard", style=t.BRAND_STYLE),
                ],
                style={"display": "flex", "alignItems": "center"},
            ),
            html.Div(
                [
                    html.Span(
                        "loihi · dvs · 20 timesteps",
                        style={
                            "color": t.TEXT_SECONDARY,
                            "fontSize": "13px",
                            "marginRight": t.SPACE_MD,
                        },
                    ),
                    html.Button("Refresh", id="refresh-btn", n_clicks=0,
                                style=t.BUTTON_STYLE),
                    html.Span(" ", style={"display": "inline-block", "width": t.SPACE_SM}),
                    html.Button("Run", id="run-btn", n_clicks=0,
                                style=t.BUTTON_PRIMARY_STYLE),
                ],
                style={"display": "flex", "alignItems": "center"},
            ),
        ],
        style=t.HEADER_STYLE,
    )


def _controls_bar() -> html.Div:
    return html.Div(
        [
            html.Div(
                [
                    html.Div("Time range", style={
                        "fontSize": "12px",
                        "color": t.TEXT_SECONDARY,
                        "marginBottom": t.SPACE_XS,
                    }),
                    dcc.RangeSlider(
                        id="time-range",
                        min=0, max=20, step=1, value=[0, 20],
                        marks={0: "0", 10: "10", 20: "20"},
                        tooltip={"placement": "bottom", "always_visible": False},
                    ),
                ],
                style={"flex": 1},
            ),
        ],
        style={
            "display": "flex",
            "gap": t.SPACE_LG,
            "padding": f"{t.SPACE_MD} {t.SPACE_LG}",
            "backgroundColor": t.SURFACE,
            "borderBottom": f"1px solid {t.BORDER}",
        },
    )


def build_layout() -> html.Div:
    return html.Div(
        [
            _header(),
            _controls_bar(),
            html.Main(
                [
                    # Left: NoC chip layout (big square)
                    html.Div(
                        _panel("Chip Layout", "noc-panel", body_height="520px"),
                        style={"flex": "0 0 520px"},
                    ),
                    # Right: stacked smaller panels
                    html.Div(
                        [
                            _panel("Energy Breakdown", "energy-panel"),
                            _panel("Throughput", "throughput-panel"),
                            _panel("Spike Raster", "raster-panel"),
                            _panel("Membrane Potentials", "potential-panel"),
                        ],
                        style={
                            "flex": 1,
                            "display": "grid",
                            "gridTemplateColumns": "1fr 1fr",
                            "gap": t.SPACE_MD,
                        },
                    ),
                ],
                style={
                    "display": "flex",
                    "gap": t.SPACE_LG,
                    "padding": t.SPACE_LG,
                },
            ),
        ],
        style=t.APP_STYLE,
    )
