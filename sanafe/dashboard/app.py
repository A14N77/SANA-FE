"""Dash app instance for the SANA-FE dashboard."""

import dash

from sanafe.dashboard.layout import build_layout
from sanafe.dashboard import theme as t


_INDEX_TEMPLATE = """<!DOCTYPE html>
<html>
  <head>
    {%metas%}
    <title>{%title%}</title>
    {%favicon%}
    {%css%}
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
    <style>
      html, body { margin: 0; padding: 0; background: __BG__; }
      * { box-sizing: border-box; }
      button:hover { box-shadow: __SHADOW_HOVER__; }
    </style>
  </head>
  <body>
    {%app_entry%}
    <footer>{%config%}{%scripts%}{%renderer%}</footer>
  </body>
</html>
""".replace("__BG__", t.OFF_WHITE).replace("__SHADOW_HOVER__", t.SHADOW_HOVER)


def create_app() -> dash.Dash:
    app = dash.Dash(__name__, title="SANA-FE Dashboard", update_title=None)
    app.index_string = _INDEX_TEMPLATE
    app.layout = build_layout()
    return app


def main(host: str = "127.0.0.1", port: int = 8050, debug: bool = False) -> None:
    app = create_app()
    app.run(host=host, port=port, debug=debug)


if __name__ == "__main__":
    main()
