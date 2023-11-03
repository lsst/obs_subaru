import os.path

from lsst.meas.algorithms import ColorLimit
from lsst.meas.astrom import MatchOptimisticBConfig

ObsConfigDir = os.path.dirname(__file__)

# Detection overrides to keep results the same post DM-39796
config.detection.thresholdType = "stdev"
config.detection.doTempLocalBackground = False
config.astrometry.doMagnitudeOutlierRejection = False

# Use PS1 for both astrometry and photometry.
config.connections.astromRefCat = "ps1_pv3_3pi_20170110"
config.astromRefObjLoader.load(os.path.join(ObsConfigDir, "filterMap.py"))
# Use the filterMap instead of the "any" filter (as is used for Gaia.
config.astromRefObjLoader.anyFilterMapsToThis = None

config.connections.photoRefCat = "ps1_pv3_3pi_20170110"
config.photoRefObjLoader.load(os.path.join(ObsConfigDir, "filterMap.py"))

# Set to match defaults currently used in HSC production runs (e.g. S15B)
config.astrometry.wcsFitter.numRejIter = 3
config.astrometry.wcsFitter.order = 3

for matchConfig in (config.astrometry,
                    ):
    matchConfig.matcher.maxRotationDeg = 1.145916
    if isinstance(matchConfig.matcher, MatchOptimisticBConfig):
        matchConfig.sourceSelector.active.excludePixelFlags = False

config.photoCal.applyColorTerms = True
config.photoCal.photoCatName = "ps1_pv3_3pi_20170110"
colors = config.photoCal.match.referenceSelection.colorLimits
colors["g-r"] = ColorLimit(primary="g_flux", secondary="r_flux", minimum=0.0)
colors["r-i"] = ColorLimit(primary="r_flux", secondary="i_flux", maximum=0.5)
config.photoCal.match.referenceSelection.doMagLimit = True
config.photoCal.match.referenceSelection.magLimit.fluxField = "i_flux"
config.photoCal.match.referenceSelection.magLimit.maximum = 22.0
config.photoCal.colorterms.load(os.path.join(ObsConfigDir, 'colorterms.py'))

config.measurement.load(os.path.join(ObsConfigDir, "apertures.py"))
config.measurement.load(os.path.join(ObsConfigDir, "kron.py"))
config.measurement.load(os.path.join(ObsConfigDir, "hsm.py"))

config.measurement.plugins.names |= ["base_Jacobian", "base_FPPosition"]
config.measurement.plugins["base_Jacobian"].pixelScale = 0.168
