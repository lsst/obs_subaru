"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")

for sub in ("isr", "charImage", "calibrate"):
    path = os.path.join(ObsConfigDir, sub + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
