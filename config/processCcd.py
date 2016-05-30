"""
Subaru-specific overrides for ProcessCcdTask (applied before SuprimeCam- and HSC-specific overrides).
"""
import os.path

from lsst.utils import getPackageDir

configDir = os.path.join(getPackageDir("obs_subaru"), "config")
bgFile = os.path.join(configDir, "background.py")

# Cosmic rays and background estimation
config.charImage.repair.cosmicray.nCrPixelMax = 1000000
config.charImage.repair.cosmicray.cond3_fac2 = 0.4
config.charImage.background.load(bgFile)
config.charImage.detectAndMeasure.detection.background.load(bgFile)
config.calibrate.detectAndMeasure.detection.background.load(bgFile)

# PSF determination
config.charImage.measurePsf.reserveFraction = 0.2
config.charImage.measurePsf.starSelector.sourceFluxField = 'base_PsfFlux_flux'
try:
    import lsst.meas.extensions.psfex.psfexPsfDeterminer
    config.charImage.measurePsf.psfDeterminer["psfex"].spatialOrder = 2
    config.charImage.measurePsf.psfDeterminer.name = "psfex"
except ImportError as e:
    print "WARNING: Unable to use psfex: %s" % e
    config.charImage.measurePsf.psfDeterminer.name = "pca"

# Astrometry
config.calibrate.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config",
                                                "filterMap.py"))


config.calibrate.detectAndMeasure.measurement.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95
# LAM the following had to be set to affect the fluxRatio used in photoCal in meas_astrom
config.calibrate.detectAndMeasure.measurement.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95

config.calibrate.photoCal.applyColorTerms = True

from lsst.pipe.tasks.setConfigFromEups import setConfigFromEups
menu = { "ps1*": {}, # Defaults are fine
         "sdss*": {"refObjLoader.filterMap": {"y": "z"}}, # No y-band, use z instead
         "2mass*": {"refObjLoader.filterMap": {ff: "J" for ff in "grizy"}}, # No optical bands, use J instead
         "10*": {}, # Match the empty astrometry_net_data version for use without a ref catalog
        }
setConfigFromEups(config.calibrate.photoCal, config.calibrate.astrometry, menu)

# Demand astrometry and photoCal succeed
config.calibrate.requireAstrometry = True
config.calibrate.requirePhotoCal = True

# Detection
config.charImage.detectAndMeasure.detection.isotropicGrow = True
config.calibrate.detectAndMeasure.detection.isotropicGrow = True

# Activate calibration of measurements: required for aperture corrections
config.calibrate.detectAndMeasure.measurement.load(os.path.join(configDir, "apertures.py"))
# Turn off cmodel until latest fixes (large blends, footprint merging, etc.) are in
# config.calibrate.detectAndMeasure.measurement.load(os.path.join(configDir, "cmodel.py"))
config.calibrate.detectAndMeasure.measurement.load(os.path.join(configDir, "kron.py"))
config.calibrate.detectAndMeasure.measurement.load(os.path.join(configDir, "hsm.py"))
if "ext_shapeHSM_HsmShapeRegauss" in config.calibrate.detectAndMeasure.measurement.plugins:
    # no deblending has been done
    config.calibrate.detectAndMeasure.measurement.plugins["ext_shapeHSM_HsmShapeRegauss"].deblendNChild = ""

# Deblender
config.charImage.detectAndMeasure.deblend.maskLimits["NO_DATA"] = 0.25 # Ignore sources that are in the vignetted region
config.charImage.detectAndMeasure.deblend.maxFootprintArea = 10000
config.calibrate.detectAndMeasure.deblend.maskLimits["NO_DATA"] = 0.25 # Ignore sources that are in the vignetted region
config.calibrate.detectAndMeasure.deblend.maxFootprintArea = 10000

config.charImage.detectAndMeasure.measurement.plugins.names |= ["base_Jacobian"]
config.calibrate.detectAndMeasure.measurement.plugins.names |= ["base_Jacobian"]
