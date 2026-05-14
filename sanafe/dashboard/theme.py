"""UT-themed palette and shared style tokens for the SANA-FE dashboard."""

UT_BURNT_ORANGE = "#BF5700"
UT_BURNT_ORANGE_DARK = "#8B3F00"
UT_BURNT_ORANGE_SOFT = "#E08A4A"

WHITE = "#FFFFFF"
OFF_WHITE = "#FAF8F5"
SURFACE = "#FFFFFF"
SURFACE_ALT = "#F4F2EE"
BORDER = "#E5E2DC"

TEXT_PRIMARY = "#1F1F1F"
TEXT_SECONDARY = "#6B6B6B"
TEXT_MUTED = "#A0A0A0"

ACCENT = UT_BURNT_ORANGE
ACCENT_HOVER = UT_BURNT_ORANGE_DARK

FONT_FAMILY = (
    '"Inter", "Helvetica Neue", -apple-system, BlinkMacSystemFont, '
    '"Segoe UI", Arial, sans-serif'
)

RADIUS = "10px"
RADIUS_SM = "6px"
SHADOW = "0 1px 2px rgba(20, 20, 20, 0.04), 0 2px 8px rgba(20, 20, 20, 0.04)"
SHADOW_HOVER = "0 2px 4px rgba(20, 20, 20, 0.06), 0 6px 16px rgba(191, 87, 0, 0.08)"

SPACE_XS = "6px"
SPACE_SM = "10px"
SPACE_MD = "16px"
SPACE_LG = "24px"
SPACE_XL = "32px"


APP_STYLE = {
    "fontFamily": FONT_FAMILY,
    "backgroundColor": OFF_WHITE,
    "color": TEXT_PRIMARY,
    "minHeight": "100vh",
    "margin": 0,
    "padding": 0,
}

HEADER_STYLE = {
    "display": "flex",
    "alignItems": "center",
    "justifyContent": "space-between",
    "padding": f"{SPACE_MD} {SPACE_LG}",
    "backgroundColor": WHITE,
    "borderBottom": f"1px solid {BORDER}",
}

BRAND_STYLE = {
    "fontSize": "18px",
    "fontWeight": 600,
    "letterSpacing": "-0.01em",
}

BRAND_ACCENT_STYLE = {
    "color": ACCENT,
    "marginRight": SPACE_XS,
}

PANEL_STYLE = {
    "backgroundColor": SURFACE,
    "border": f"1px solid {BORDER}",
    "borderRadius": RADIUS,
    "padding": SPACE_MD,
    "boxShadow": SHADOW,
}

PANEL_TITLE_STYLE = {
    "fontSize": "13px",
    "fontWeight": 600,
    "textTransform": "uppercase",
    "letterSpacing": "0.06em",
    "color": TEXT_SECONDARY,
    "marginBottom": SPACE_SM,
}

PANEL_BODY_STYLE = {
    "color": TEXT_PRIMARY,
    "minHeight": "180px",
}

BUTTON_STYLE = {
    "backgroundColor": WHITE,
    "color": TEXT_PRIMARY,
    "border": f"1px solid {BORDER}",
    "borderRadius": RADIUS_SM,
    "padding": f"{SPACE_XS} {SPACE_SM}",
    "cursor": "pointer",
    "fontFamily": FONT_FAMILY,
    "fontSize": "13px",
}

BUTTON_PRIMARY_STYLE = {
    **BUTTON_STYLE,
    "backgroundColor": ACCENT,
    "color": WHITE,
    "border": f"1px solid {ACCENT}",
}

PLOTLY_COLORWAY = [
    UT_BURNT_ORANGE,
    "#1F4E79",
    "#5C8A3B",
    "#A05195",
    "#665191",
    UT_BURNT_ORANGE_SOFT,
]

PLOTLY_LAYOUT = {
    "font": {"family": FONT_FAMILY, "color": TEXT_PRIMARY, "size": 12},
    "paper_bgcolor": SURFACE,
    "plot_bgcolor": SURFACE,
    "colorway": PLOTLY_COLORWAY,
    "margin": {"l": 48, "r": 16, "t": 24, "b": 36},
    "xaxis": {"gridcolor": BORDER, "zerolinecolor": BORDER},
    "yaxis": {"gridcolor": BORDER, "zerolinecolor": BORDER},
}
