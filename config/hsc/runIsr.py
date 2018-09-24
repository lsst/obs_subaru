"""
HSC-specific overrides for RunIsrTask
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SubaruIsrTask

obsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")

config.isr.retarget(SubaruIsrTask)
config.isr.load(os.path.join(obsConfigDir, "isr.py"))
