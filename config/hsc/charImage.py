"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

from lsst.meas.astrom import MatchOptimisticBConfig

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")

config.measurePsf.starSelector["objectSize"].widthMin = 0.9
config.measurePsf.starSelector["objectSize"].fluxMin = 4000
for refObjLoader in (config.refObjLoader,
                     ):
    refObjLoader.load(os.path.join(ObsConfigDir, "filterMap.py"))


config.ref_match.sourceSelector.name = 'matcher'
for matchConfig in (config.ref_match,
                    ):
    matchConfig.sourceFluxType = 'Psf'
    matchConfig.sourceSelector.active.sourceFluxType = 'Psf'
    matchConfig.matcher.maxRotationDeg = 1.145916
    matchConfig.matcher.maxOffsetPix = 250
    if isinstance(matchConfig.matcher, MatchOptimisticBConfig):
        matchConfig.matcher.allowedNonperpDeg = 0.2
        matchConfig.matcher.maxMatchDistArcSec = 2.0
        matchConfig.sourceSelector.active.excludePixelFlags = False

config.measurement.plugins["base_Jacobian"].pixelScale = 0.168
