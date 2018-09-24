"""
Suprimecam-specific overrides for RunIsrTask
(applied after Subaru overrides in ../processCcd.py)
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SuprimeCamIsrTask

obsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam")

config.isr.retarget(SuprimeCamIsrTask)
config.isr.load(os.path.join(obsConfigDir, "isr.py"))
