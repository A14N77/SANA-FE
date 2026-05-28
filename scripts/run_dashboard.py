#!/usr/bin/env python3
"""Launch the SANA-FE dashboard against mock simulation data.

    python scripts/run_dashboard.py
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO))

from sanafe.dashboard.app import main


if __name__ == "__main__":
    main(debug=True)
