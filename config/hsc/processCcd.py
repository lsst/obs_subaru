"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

from lsst.meas.astrom import MatchOptimisticBConfig

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

config.charImage.ref_match.sourceSelector.name = 'matcher'
for matchConfig in (config.charImage.ref_match,
                    config.calibrate.astrometry,
                    ):
    matchConfig.sourceFluxType = 'Psf'
    matchConfig.sourceSelector.active.sourceFluxType = 'Psf'
    matchConfig.matcher.maxRotationDeg = 1.145916
    matchConfig.matcher.maxOffsetPix = 250
    if isinstance(matchConfig.matcher, MatchOptimisticBConfig):
        matchConfig.matcher.allowedNonperpDeg = 0.2
        matchConfig.matcher.maxMatchDistArcSec = 2.0
        matchConfig.sourceSelector.active.excludePixelFlags = False

config.charImage.measurement.plugins["base_Jacobian"].pixelScale = 0.168
config.calibrate.measurement.plugins["base_Jacobian"].pixelScale = 0.168
