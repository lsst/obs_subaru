"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")

config.isr.load(os.path.join(ObsConfigDir, 'isr.py'))

config.calibrate.photoCal.colorterms.load(os.path.join(ObsConfigDir, 'colorterms.py'))
config.charImage.measurePsf.starSelector["objectSize"].widthMin = 0.9
config.charImage.measurePsf.starSelector["objectSize"].fluxMin = 4000
for refObjLoader in (config.calibrate.astromRefObjLoader,
                     config.calibrate.photoRefObjLoader,
                     config.charImage.refObjLoader,
                     ):
    refObjLoader.load(os.path.join(ObsConfigDir, "filterMap.py"))

# Set to match defaults curretnly used in HSC production runs (e.g. S15B)
config.calibrate.astrometry.wcsFitter.numRejIter = 3
config.calibrate.astrometry.wcsFitter.order = 3
for matcher in (config.charImage.ref_match.matcher,
                config.calibrate.astrometry.matcher,
                ):
    matcher.sourceSelector.active.sourceFluxType = 'Psf'
    matcher.allowedNonperpDeg = 0.2
    matcher.maxRotationDeg = 1.145916
    matcher.maxMatchDistArcSec = 2.0
    matcher.maxOffsetPix = 250

config.charImage.measurement.plugins["base_Jacobian"].pixelScale = 0.168
config.calibrate.measurement.plugins["base_Jacobian"].pixelScale = 0.168
