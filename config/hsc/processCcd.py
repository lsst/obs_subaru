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
config.calibrate.photoCal.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))
config.charImage.measurePsf.starSelector["objectSize"].widthMin = 0.9
config.charImage.measurePsf.starSelector["objectSize"].fluxMin = 4000
for refObjLoader in (config.calibrate.astromRefObjLoader,
                     config.calibrate.photoRefObjLoader,
                     config.charImage.refObjLoader,
                     ):
    refObjLoader.load(os.path.join(hscConfigDir, "filterMap.py"))

# Set to match defaults curretnly used in HSC production runs (e.g. S15B)
config.calibrate.astrometry.matcher.sourceSelector.active.sourceFluxType = 'Psf'
config.calibrate.astrometry.matcher.allowedNonperpDeg = 0.2
config.calibrate.astrometry.matcher.maxRotationDeg = 1.145916
config.calibrate.astrometry.wcsFitter.numRejIter = 3
config.calibrate.astrometry.wcsFitter.order = 3
config.calibrate.astrometry.matcher.maxMatchDistArcSec = 2.0
config.calibrate.astrometry.matcher.maxOffsetPix = 250

# Do not use NO_DATA pixels for fringe subtraction.
config.isr.fringe.stats.badMaskPlanes = ['SAT', 'NO_DATA']

config.charImage.measurement.plugins["base_Jacobian"].pixelScale = 0.168
config.calibrate.measurement.plugins["base_Jacobian"].pixelScale = 0.168
