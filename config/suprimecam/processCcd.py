"""
SuprimeCam-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

suprimecamConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam")
config.load(os.path.join(suprimecamConfigDir, 'isr.py'))
config.calibrate.photocal.colorterms.load(os.path.join(suprimecamConfigDir, 'colorterms.py'))

config.charImage.detectAndMeasure.measurement.plugins["base_Jacobian"].pixelScale = 0.2
config.calibrate.detectAndMeasure.measurement.plugins["base_Jacobian"].pixelScale = 0.2

