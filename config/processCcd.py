"""
Subaru-specific overrides for ProcessCcdTask (applied before SuprimeCam- and HSC-specific overrides).
"""
import os.path

from lsst.utils import getPackageDir

# This was a horrible choice of defaults: only the scaling of the flats
# should determine the relative normalisations of the CCDs!
config.isr.assembleCcd.doRenorm = False

# Cosmic rays and background estimation
config.calibrate.repair.cosmicray.nCrPixelMax = 1000000
config.calibrate.repair.cosmicray.cond3_fac2 = 0.4
config.calibrate.background.binSize = 128
config.calibrate.background.undersampleStyle = 'REDUCE_INTERP_ORDER'
config.calibrate.detection.background.binSize = 128
config.calibrate.detection.background.undersampleStyle='REDUCE_INTERP_ORDER'
config.detection.background.binSize = 128
config.detection.background.undersampleStyle = 'REDUCE_INTERP_ORDER'

# PSF determination
config.calibrate.measurePsf.starSelector.name = 'objectSize'
config.calibrate.measurePsf.starSelector['objectSize'].sourceFluxField = 'base_PsfFlux_flux'
try:
    import lsst.meas.extensions.psfex.psfexPsfDeterminer
    config.calibrate.measurePsf.psfDeterminer["psfex"].spatialOrder = 2
    config.calibrate.measurePsf.psfDeterminer.name = "psfex"
except ImportError as e:
    print "WARNING: Unable to use psfex: %s" % e
    config.calibrate.measurePsf.psfDeterminer.name = "pca"

# Astrometry
config.calibrate.astrometry.refObjLoader.filterMap = {
    'B': 'g',
    'V': 'r',
    'R': 'r',
    'I': 'i',
    'y': 'z',
}

config.measurement.plugins.names |= ["base_Jacobian", "base_FocalPlane"]
config.measurement.plugins['base_Jacobian'].pixelScale = 0.168
config.measurement.algorithms['base_ClassificationExtendedness'].fluxRatio = 0.95
# LAM the following had to be set to affect the fluxRatio used in photocal in meas_astrom
config.calibrate.measurement.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95

config.calibrate.photocal.applyColorTerms = True

from lsst.pipe.tasks.setConfigFromEups import setConfigFromEups
menu = { "ps1*": {}, # Defaults are fine
         "sdss*": {"refObjLoader.filterMap": {"y": "z"}}, # No y-band, use z instead
         "2mass*": {"refObjLoader.filterMap": {ff: "J" for ff in "grizy"}}, # No optical bands, use J instead
        }
setConfigFromEups(config.calibrate.photocal, config.calibrate.astrometry, menu)

# Demand astrometry and photoCal succeed
config.calibrate.requireAstrometry = True
config.calibrate.requirePhotoCal = True

# Detection
config.detection.isotropicGrow = True
config.detection.returnOriginalFootprints = False

# Measurement
config.doWriteSourceMatches = True

# Activate calibration of measurements: required for aperture corrections
config.calibrate.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
# Turn off cmodel until latest fixes (large blends, footprint merging, etc.) are in
# config.calibrate.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "cmodel.py"))
config.calibrate.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
config.calibrate.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsm.py"))
if "ext_shapeHSM_HsmShapeRegauss" in config.calibrate.measurement.algorithms:
    # no deblending has been done
    config.calibrate.measurement.algorithms["ext_shapeHSM_HsmShapeRegauss"].deblendNChild = ""

# Activate deep measurements
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsm.py"))
# Note no CModel: it's slow.

# Enable deblender for processCcd
config.measurement.doReplaceWithNoise = True
config.doDeblend = True
config.deblend.load(os.path.join(getPackageDir("obs_subaru"), "config", "deblend.py"))
config.deblend.maskLimits["NO_DATA"] = 0.25 # Ignore sources that are in the vignetted region
config.deblend.maxFootprintArea = 10000
