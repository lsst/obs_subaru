import os.path

from lsst.meas.algorithms import ColorLimit
from lsst.meas.astrom import MatchOptimisticBConfig

ObsConfigDir = os.path.dirname(__file__)

bgFile = os.path.join(ObsConfigDir, "background.py")

# Cosmic rays and background estimation
config.repair.cosmicray.nCrPixelMax = 1000000
config.repair.cosmicray.cond3_fac2 = 0.4
config.background.load(bgFile)
config.detection.background.load(bgFile)

# PSF determination
config.measurePsf.reserve.fraction = 0.2
config.measurePsf.starSelector["objectSize"].sourceFluxField = "base_PsfFlux_instFlux"
config.measurePsf.starSelector["objectSize"].widthMin = 0.9
config.measurePsf.starSelector["objectSize"].fluxMin = 4000

# Astrometry
config.refObjLoader.load(os.path.join(ObsConfigDir, "filterMap.py"))

# Set to match defaults curretnly used in HSC production runs (e.g. S15B)
config.catalogCalculation.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95

# Detection
config.detection.isotropicGrow = True

# Activate calibration of measurements: required for aperture corrections
config.load(os.path.join(ObsConfigDir, "cmodel.py"))
config.measurement.load(os.path.join(ObsConfigDir, "apertures.py"))
config.measurement.load(os.path.join(ObsConfigDir, "kron.py"))
config.measurement.load(os.path.join(ObsConfigDir, "convolvedFluxes.py"))
config.measurement.load(os.path.join(ObsConfigDir, "gaap.py"))
config.measurement.load(os.path.join(ObsConfigDir, "hsm.py"))
if "ext_shapeHSM_HsmShapeRegauss" in config.measurement.plugins:
    # no deblending has been done
    config.measurement.plugins["ext_shapeHSM_HsmShapeRegauss"].deblendNChild = ""

# Deblender
config.deblend.maskLimits["NO_DATA"] = 0.25 # Ignore sources that are in the vignetted region
config.deblend.maxFootprintArea = 10000

config.measurement.plugins.names |= ["base_Jacobian", "base_FPPosition"]
config.measurement.plugins["base_Jacobian"].pixelScale = 0.168

# Convolved fluxes can fail for small target seeing if the observation seeing is larger
if "ext_convolved_ConvolvedFlux" in config.measurement.plugins:
    names = config.measurement.plugins["ext_convolved_ConvolvedFlux"].getAllResultNames()
    config.measureApCorr.allowFailure += names

if "ext_gaap_GaapFlux" in config.measurement.plugins:
    names = config.measurement.plugins["ext_gaap_GaapFlux"].getAllGaapResultNames()
    config.measureApCorr.allowFailure += names

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
