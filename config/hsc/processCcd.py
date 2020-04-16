"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path


ObsConfigDir = os.path.dirname(__file__)

for sub in ("isr", "charImage", "calibrate"):
    path = os.path.join(ObsConfigDir, sub + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
