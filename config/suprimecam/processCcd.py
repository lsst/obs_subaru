"""
SuprimeCam-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SuprimeCamIsrTask

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam")

config.isr.retarget(SuprimeCamIsrTask)
config.isr.load(os.path.join(ObsConfigDir, 'isr.py'))

config.calibrate.photocal.colorterms.load(os.path.join(ObsConfigDir, 'colorterms.py'))

config.charImage.measurement.plugins["base_Jacobian"].pixelScale = 0.2
config.calibrate.measurement.plugins["base_Jacobian"].pixelScale = 0.2
