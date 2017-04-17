"""
HSC-specific overrides for ProcessDonutTask
(applied after Subaru overrides in ../processDonut.py).
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SubaruIsrTask

hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
config.isr.retarget(SubaruIsrTask)
config.isr.load(os.path.join(hscConfigDir, 'isr.py'))

# Do not use NO_DATA pixels for fringe subtraction.
config.isr.fringe.stats.badMaskPlanes = ['SAT', 'NO_DATA']
