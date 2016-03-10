"""
SuprimeCam (MIT)-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

suprimecamMitConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam-mit")
config.load(os.path.join(suprimecamMitConfigDir, 'isr.py'))
config.calibrate.photocal.colorterms.load(os.path.join(suprimecamMitConfigDir, 'colorterms.py'))

config.charImage.detectAndMeasure.measurement.algorithms["base_Jacobian"].pixelScale = 0.2
config.calibrate.detectAndMeasure.measurement.algorithms["base_Jacobian"].pixelScale = 0.2
