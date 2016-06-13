"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
config.load(os.path.join(hscConfigDir, 'isr.py'))
config.calibrate.photoCal.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))
config.charImage.measurePsf.starSelector["objectSize"].widthMin = 0.9
config.charImage.measurePsf.starSelector["objectSize"].fluxMin = 4000
config.calibrate.refObjLoader.load(os.path.join(hscConfigDir, "filterMap.py"))

config.calibrate.astrometry.wcsFitter.order = 3
config.calibrate.astrometry.matcher.maxMatchDistArcSec = 2.0
config.calibrate.astrometry.matcher.maxOffsetPix = 750

# Do not use NO_DATA pixels for fringe subtraction.
config.isr.fringe.stats.badMaskPlanes = ['SAT', 'NO_DATA']

config.charImage.detectAndMeasure.measurement.plugins["base_Jacobian"].pixelScale = 0.168
config.calibrate.detectAndMeasure.measurement.plugins["base_Jacobian"].pixelScale = 0.168
