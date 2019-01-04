"""
SuprimeCam (MIT)-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam-mit")

config.isr.load(os.path.join(ObsConfigDir, 'isr.py'))

config.calibrate.photoCal.colorterms.load(os.path.join(ObsConfigDir, 'colorterms.py'))

config.charImage.measurement.plugins["base_Jacobian"].pixelScale = 0.2
config.calibrate.measurement.plugins["base_Jacobian"].pixelScale = 0.2
