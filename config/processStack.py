"""
Subaru-specific overrides for ProcessStackTask (applied before SuprimeCam- and HSC-specific overrides).
"""

# Cosmic rays and background estimation
root.calibrate.repair.doCosmicRay = False
root.calibrate.background.binSize = 1024
root.calibrate.detection.background.binSize = 1024
root.detection.background.binSize = 1024
root.detection.thresholdValue = 3.5

# PSF determination
root.calibrate.measurePsf.starSelector.name = "objectSize"
root.calibrate.measurePsf.psfDeterminer.name = "pca"
root.calibrate.measurePsf.starSelector["secondMoment"].clumpNSigma = 2.0
root.calibrate.measurePsf.psfDeterminer["pca"].nEigenComponents = 4
root.calibrate.measurePsf.psfDeterminer["pca"].spatialOrder = 2
root.calibrate.measurePsf.psfDeterminer["pca"].kernelSizeMin = 25
root.calibrate.measurePsf.psfDeterminer["pca"].kernelScaling = 10.0

# Astrometry (just use meas_astrom since we just need to match, not solve)
root.calibrate.astrometry.forceKnownWcs = True
root.calibrate.astrometry.solver.calculateSip = False

# Enable deblender
root.measurement.doReplaceWithNoise = True
root.doDeblend = True
root.deblend.maxNumberOfPeaks = 20
root.doWriteHeavyFootprintsInSources = True

# Measurement
root.measurement.algorithms["flux.gaussian"].shiftmax = 10.0
try:
    import lsst.meas.extensions.multiShapelet
    root.measurement.algorithms.names |= lsst.meas.extensions.multiShapelet.algorithms
    root.measurement.slots.modelFlux = "multishapelet.combo.flux"
except ImportError:
    print "meas_extensions_multiShapelet is not setup; disabling model mags"
