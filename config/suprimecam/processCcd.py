"""
SuprimeCam-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

suprimecamConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam")
root.load(os.path.join(suprimecamConfigDir, 'isr.py'))
root.calibrate.photocal.colorterms.load(os.path.join(suprimecamConfigDir, 'colorterms.py'))

root.measurement.algorithms["jacobian"].pixelScale = 0.2

