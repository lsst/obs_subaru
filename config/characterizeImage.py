import os.path

from lsst.meas.algorithms import ColorLimit
from lsst.meas.astrom import MatchOptimisticBConfig

ObsConfigDir = os.path.dirname(__file__)

# PSF determination
config.measurePsf.reserve.fraction = 0.2

# Detection overrides to keep results the same post DM-39796
config.detection.doTempLocalBackground = False
config.detection.thresholdType = "stdev"
config.measurePsf.starSelector['objectSize'].doFluxLimit = True
config.measurePsf.starSelector['objectSize'].fluxMin = 4000.0
config.measurePsf.starSelector['objectSize'].doSignalToNoiseLimit = False

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

config.measurement.plugins.names |= ["base_Jacobian", "base_FPPosition"]
config.measurement.plugins["base_Jacobian"].pixelScale = 0.168

# Convolved fluxes can fail for small target seeing if the observation seeing is larger
if "ext_convolved_ConvolvedFlux" in config.measurement.plugins:
    names = config.measurement.plugins["ext_convolved_ConvolvedFlux"].getAllResultNames()
    config.measureApCorr.allowFailure += names

if "ext_gaap_GaapFlux" in config.measurement.plugins:
    names = config.measurement.plugins["ext_gaap_GaapFlux"].getAllGaapResultNames()
    config.measureApCorr.allowFailure += names
