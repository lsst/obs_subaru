"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

from lsst.meas.astrom import MatchOptimisticBConfig

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")

config.photoCal.colorterms.load(os.path.join(ObsConfigDir, 'colorterms.py'))
for refObjLoader in (config.astromRefObjLoader,
                     config.photoRefObjLoader,
                     ):
    refObjLoader.load(os.path.join(ObsConfigDir, "filterMap.py"))

# Set to match defaults curretnly used in HSC production runs (e.g. S15B)
config.astrometry.wcsFitter.numRejIter = 3
config.astrometry.wcsFitter.order = 3

for matchConfig in (config.astrometry,
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
