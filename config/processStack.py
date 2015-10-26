"""
Subaru-specific overrides for ProcessStackTask (applied before SuprimeCam- and HSC-specific overrides).
"""

# Cosmic rays and background estimation
config.calibrate.repair.doCosmicRay = False
config.calibrate.background.binSize = 1024
config.calibrate.detection.background.binSize = 1024
config.detection.background.binSize = 1024
config.detection.thresholdValue = 3.5

# PSF determination
config.calibrate.measurePsf.starSelector.name = "objectSize"
config.calibrate.measurePsf.psfDeterminer.name = "pca"
config.calibrate.measurePsf.starSelector["secondMoment"].clumpNSigma = 2.0
config.calibrate.measurePsf.psfDeterminer["pca"].nEigenComponents = 4
config.calibrate.measurePsf.psfDeterminer["pca"].spatialOrder = 2
config.calibrate.measurePsf.psfDeterminer["pca"].kernelSizeMin = 25
config.calibrate.measurePsf.psfDeterminer["pca"].kernelScaling = 10.0

# Astrometry (just use meas_astrom since we just need to match, not solve)
config.calibrate.astrometry.forceKnownWcs = True

# Enable deblender
config.measurement.doReplaceWithNoise = True
config.doDeblend = True
config.deblend.maxNumberOfPeaks = 20
config.doWriteHeavyFootprintsInSources = True

# Measurement
config.measurement.algorithms["base_GaussianFlux"].shiftmax = 10.0
try:
    import lsst.meas.extensions.multiShapelet
    config.measurement.algorithms.names |= lsst.meas.extensions.multiShapelet.algorithms
    config.measurement.slots.modelFlux = "ext_multiShapelet_ComboFlux"
except ImportError:
    print "meas_extensions_multiShapelet is not setup; disabling model mags"
