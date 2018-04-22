"""
superbit-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.superBIT.isr import SuperBITIsrTask

superbitConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "SuperBIT")
config.isr.retarget(SuperBITIsrTask)
config.isr.load(os.path.join(superbitConfigDir, 'isr_suprebit.py'))

###only do the isr, output images are saved in "IMAGE"
