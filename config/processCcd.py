"""
Subaru-specific overrides for ProcessCcdTask (applied before SuprimeCam- and HSC-specific overrides).
"""

import os

# This was a horrible choice of defaults: only the scaling of the flats
# should determine the relative normalisations of the CCDs!
root.isr.assembleCcd.doRenorm = False

# Cosmic rays and background estimation
root.calibrate.repair.cosmicray.nCrPixelMax = 1000000
root.calibrate.repair.cosmicray.cond3_fac2 = 0.4
root.calibrate.background.binSize = 128
root.calibrate.background.undersampleStyle = 'REDUCE_INTERP_ORDER'
root.calibrate.detection.background.binSize = 128
root.calibrate.detection.background.undersampleStyle='REDUCE_INTERP_ORDER'
root.detection.background.binSize = 128
root.detection.background.undersampleStyle = 'REDUCE_INTERP_ORDER'

# PSF determination
root.calibrate.measurePsf.starSelector.name = 'objectSize'
root.calibrate.measurePsf.starSelector['objectSize'].sourceFluxField = 'base_PsfFlux_flux'
try:
    import lsst.meas.extensions.psfex.psfexPsfDeterminer
    root.calibrate.measurePsf.psfDeterminer["psfex"].spatialOrder = 2
    root.calibrate.measurePsf.psfDeterminer.name = "psfex"
except ImportError as e:
    print "WARNING: Unable to use psfex: %s" % e
    root.calibrate.measurePsf.psfDeterminer.name = "pca"

# Astrometry
from lsst.meas.astrom import AstrometryTask
root.calibrate.astrometry.retarget(AstrometryTask)
root.calibrate.astrometry.refObjLoader.filterMap = {
    'B': 'g',
    'V': 'r',
    'R': 'r',
    'I': 'i',
    'y': 'z',
}

# Reference catalog may not have as good star/galaxy discrimination as our data
root.calibrate.photocal.badFlags += ['base_ClassificationExtendedness_value',]
root.measurement.algorithms['base_ClassificationExtendedness'].fluxRatio = 0.95
# LAM the following had to be set to affect the fluxRatio used in photocal in meas_astrom
root.calibrate.measurement.plugins['base_ClassificationExtendedness'].fluxRatio = 0.95

root.calibrate.photocal.applyColorTerms = True

from lsst.pipe.tasks.setConfigFromEups import setConfigFromEups
menu = { "ps1*": {}, # Defaults are fine
         "sdss*": {"refObjLoader.filterMap": {"y": "z"}}, # No y-band, use z instead
         "2mass*": {"refObjLoader.filterMap": {ff: "J" for ff in "grizy"}}, # No optical bands, use J instead
        }
setConfigFromEups(root.calibrate.photocal, root.calibrate.astrometry, menu)

# Detection
root.detection.isotropicGrow = True
root.detection.returnOriginalFootprints = False

# Measurement
root.doWriteSourceMatches = True

# Activate calibration of measurements: required for aperture corrections
root.calibrate.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'apertures.py'))
# Turn off cmodel until latest fixes (large blends, footprint merging, etc.) are in
# root.calibrate.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'cmodel.py'))
root.calibrate.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'kron.py'))
root.calibrate.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsm.py'))
if "shape.hsm.regauss" in root.calibrate.measurement.algorithms:
    root.calibrate.measurement.algorithms["shape.hsm.regauss"].deblendNChild = "" # no deblending has been done

# Activate deep measurements
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'apertures.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'kron.py'))
root.measurement.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsm.py'))
# Note no CModel: it's slow.

# Enable deblender for processCcd
root.measurement.doReplaceWithNoise = True
root.doDeblend = True
root.deblend.maxNumberOfPeaks = 20
