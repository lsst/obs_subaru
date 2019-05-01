"""
Subaru-specific overrides for ProcessCcdTask (applied before SuprimeCam- and HSC-specific overrides).
"""
import os.path

from lsst.utils import getPackageDir

for sub in ("isr", "charImage", "calibrate"):
    path = os.path.join(getPackageDir("obs_subaru"), "config", sub + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
