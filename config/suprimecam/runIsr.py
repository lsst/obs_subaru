"""
Suprimecam-specific overrides for RunIsrTask
(applied after Subaru overrides in ../processCcd.py)
"""
import os.path

from lsst.utils import getPackageDir

obsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam")

config.isr.load(os.path.join(obsConfigDir, "isr.py"))
