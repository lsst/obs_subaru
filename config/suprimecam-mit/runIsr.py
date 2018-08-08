"""
SuprimeCam (MIT)-specific overrides for RunIsrTask
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SuprimeCamIsrTask

obsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam-mit")

config.isr.retarget(SuprimeCamIsrTask)
config.isr.load(os.path.join(obsConfigDir, "isr.py"))
