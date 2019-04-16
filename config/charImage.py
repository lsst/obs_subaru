import os.path

from lsst.utils import getPackageDir
from lsst.meas.algorithms import ColorLimit

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config")

bgFile = os.path.join(ObsConfigDir, "background.py")

# Cosmic rays and background estimation
config.repair.cosmicray.nCrPixelMax = 1000000
config.repair.cosmicray.cond3_fac2 = 0.4
config.background.load(bgFile)
config.detection.background.load(bgFile)

# PSF determination
config.measurePsf.reserve.fraction = 0.2
config.measurePsf.starSelector["objectSize"].sourceFluxField = "base_PsfFlux_instFlux"
try:
    import lsst.meas.extensions.psfex.psfexPsfDeterminer
    config.measurePsf.psfDeterminer["psfex"].spatialOrder = 2
    config.measurePsf.psfDeterminer["psfex"].psfexBasis = "PIXEL_AUTO"
    config.measurePsf.psfDeterminer["psfex"].samplingSize = 0.5
    config.measurePsf.psfDeterminer["psfex"].kernelSize = 81
    config.measurePsf.psfDeterminer.name = "psfex"
except ImportError as e:
    print("WARNING: Unable to use psfex: %s" % e)
    config.measurePsf.psfDeterminer.name = "pca"

# Astrometry
config.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))
config.refObjLoader.ref_dataset_name = "ps1_pv3_3pi_20170110"

# Set to match defaults curretnly used in HSC production runs (e.g. S15B)
config.catalogCalculation.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95

# Detection
config.detection.isotropicGrow = True

# Activate calibration of measurements: required for aperture corrections
config.load(os.path.join(ObsConfigDir, "cmodel.py"))
config.measurement.load(os.path.join(ObsConfigDir, "apertures.py"))
config.measurement.load(os.path.join(ObsConfigDir, "kron.py"))
config.measurement.load(os.path.join(ObsConfigDir, "convolvedFluxes.py"))
config.measurement.load(os.path.join(ObsConfigDir, "hsm.py"))
if "ext_shapeHSM_HsmShapeRegauss" in config.measurement.plugins:
    # no deblending has been done
    config.measurement.plugins["ext_shapeHSM_HsmShapeRegauss"].deblendNChild = ""

# Deblender
config.deblend.maskLimits["NO_DATA"] = 0.25 # Ignore sources that are in the vignetted region
config.deblend.maxFootprintArea = 10000

config.measurement.plugins.names |= ["base_Jacobian", "base_FPPosition"]

# Convolved fluxes can fail for small target seeing if the observation seeing is larger
if "ext_convolved_ConvolvedFlux" in config.measurement.plugins:
    names = config.measurement.plugins["ext_convolved_ConvolvedFlux"].getAllResultNames()
    config.measureApCorr.allowFailure += names
