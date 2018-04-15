"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SubaruIsrTask

hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
config.isr.retarget(SubaruIsrTask)
config.isr.load(os.path.join(hscConfigDir, 'isr.py'))

###only do the isr, output images are saved in "IMAGE"
