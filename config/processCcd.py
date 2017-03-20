"""
Subaru-specific overrides for ProcessCcdTask (applied before SuprimeCam- and HSC-specific overrides).
"""
from __future__ import print_function
import os.path

from lsst.utils import getPackageDir

configDir = os.path.join(getPackageDir("obs_subaru"), "config")
bgFile = os.path.join(configDir, "background.py")

# Cosmic rays and background estimation
config.charImage.repair.cosmicray.nCrPixelMax = 1000000
config.charImage.repair.cosmicray.cond3_fac2 = 0.4
config.charImage.background.load(bgFile)
config.charImage.detection.background.load(bgFile)
config.calibrate.detection.background.load(bgFile)

# PSF determination
config.charImage.measurePsf.reserveFraction = 0.2
config.charImage.measurePsf.starSelector["objectSize"].sourceFluxField = 'base_PsfFlux_flux'
try:
    import lsst.meas.extensions.psfex.psfexPsfDeterminer
    config.charImage.measurePsf.psfDeterminer["psfex"].spatialOrder = 2
    config.charImage.measurePsf.psfDeterminer.name = "psfex"
except ImportError as e:
    print("WARNING: Unable to use psfex: %s" % e)
    config.charImage.measurePsf.psfDeterminer.name = "pca"

# Astrometry
for refObjLoader in (config.calibrate.astromRefObjLoader,
                     config.calibrate.photoRefObjLoader,
                     config.charImage.refObjLoader,
                     ):
    refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))

# Better astrometry matching
config.calibrate.astrometry.matcher.numBrightStars = 150
config.calibrate.photoCal.matcher.numBrightStars = 150

# Set to match defaults curretnly used in HSC production runs (e.g. S15B)
config.charImage.catalogCalculation.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95
config.calibrate.catalogCalculation.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95

config.calibrate.photoCal.applyColorTerms = True

from lsst.pipe.tasks.setConfigFromEups import setConfigFromEups
menu = {"ps1*": {}, # Defaults are fine
        "sdss*": {"photoRefObjLoader.filterMap": {"y": "z"},
                  "astromRefObjLoader.filterMap": {"y": "z"}}, # No y-band, use z instead
        "2mass*": {"photoRefObjLoader.filterMap": {ff: "J" for ff in "grizy"},
                   "astromRefObjLoader.filterMap": {ff: "J" for ff in "grizy"}}, # No optical, use J instead
        "10*": {}, # Match the empty astrometry_net_data version for use without a ref catalog
        }
setConfigFromEups(config.calibrate.photoCal, config.calibrate, menu)

# Demand astrometry and photoCal succeed
config.calibrate.requireAstrometry = True
config.calibrate.requirePhotoCal = True

# Detection
config.charImage.detection.isotropicGrow = True
config.calibrate.detection.isotropicGrow = True

# Activate calibration of measurements: required for aperture corrections
config.charImage.load(os.path.join(configDir, "cmodel.py"))
config.charImage.measurement.load(os.path.join(configDir, "apertures.py"))
config.charImage.measurement.load(os.path.join(configDir, "kron.py"))
config.charImage.measurement.load(os.path.join(configDir, "convolvedFluxes.py"))
config.charImage.measurement.load(os.path.join(configDir, "hsm.py"))
if "ext_shapeHSM_HsmShapeRegauss" in config.charImage.measurement.plugins:
    # no deblending has been done
    config.charImage.measurement.plugins["ext_shapeHSM_HsmShapeRegauss"].deblendNChild = ""

config.calibrate.measurement.load(os.path.join(configDir, "apertures.py"))
config.calibrate.measurement.load(os.path.join(configDir, "kron.py"))
config.calibrate.measurement.load(os.path.join(configDir, "hsm.py"))

# Deblender
config.charImage.deblend.maskLimits["NO_DATA"] = 0.25 # Ignore sources that are in the vignetted region
config.charImage.deblend.maxFootprintArea = 10000
config.calibrate.deblend.maskLimits["NO_DATA"] = 0.25 # Ignore sources that are in the vignetted region
config.calibrate.deblend.maxFootprintArea = 10000

config.charImage.measurement.plugins.names |= ["base_Jacobian", "base_FPPosition"]
config.calibrate.measurement.plugins.names |= ["base_Jacobian", "base_FPPosition"]

# Convolved fluxes can fail for small target seeing if the observation seeing is larger
if "ext_convolved_ConvolvedFlux" in config.charImage.measurement.plugins:
    names = config.charImage.measurement.plugins["ext_convolved_ConvolvedFlux"].getAllResultNames()
    config.charImage.measureApCorr.allowFailure += names
