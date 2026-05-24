"""SANA-FE interactive dashboard (Plotly Dash).

Dash and Plotly are optional dependencies. Import submodules directly
(e.g. ``from sanafe.dashboard.app import main``) or run the dashboard
with ``python -m sanafe.dashboard``.
"""

from sanafe.dashboard import theme

__all__ = ["theme"]
