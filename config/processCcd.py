"""
Subaru-specific overrides for ProcessCcdTask (applied before SuprimeCam- and HSC-specific overrides).
"""

# This was a horrible choice of defaults: only the scaling of the flats
# should determine the relative normalisations of the CCDs!
root.isr.assembleCcd.doRenorm = False

# Cosmic rays and background estimation
root.calibrate.repair.cosmicray.nCrPixelMax = 1000000
root.calibrate.repair.cosmicray.cond3_fac2 = 0.4
root.calibrate.background.binSize = 1024
root.calibrate.detection.background.binSize = 1024
root.detection.background.binSize = 1024

# PSF determination
root.calibrate.measurePsf.starSelector.name = "objectSize"
root.calibrate.measurePsf.psfDeterminer.name = "pca"

# Astrometry
try:
    # the next line will fail if hscAstrom is not setup; in that case we just use lsst.meas.astrm
    from lsst.obs.subaru.astrometry import SubaruAstrometryTask
    root.calibrate.astrometry.retarget(SubaruAstrometryTask)
    root.calibrate.astrometry.solver.filterMap = { 'B': 'g',
                                                   'V': 'r',
                                                   'R': 'r',
                                                   'I': 'i',
                                                   'y': 'z',
                                                   }
except ImportError:
    print "hscAstrom is not setup; using LSST's meas_astrom instead"


# Detection
root.detection.isotropicGrow = True
root.detection.returnOriginalFootprints = False

# Measurement
root.doWriteSourceMatches = True

# Enable deblender for processCcd
root.measurement.doReplaceWithNoise = True
root.doDeblend = True
root.deblend.maxNumberOfPeaks = 20
