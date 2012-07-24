"""
Subaru-specific overrides for ProcessCcdTask (applied before SuprimeCam- and HSC-specific overrides).
"""

# Cosmic rays and background estimation
root.calibrate.repair.doCosmicRay = True
root.calibrate.repair.cosmicray.nCrPixelMax = 1000000
root.calibrate.background.binSize = 1024

# Detection
root.calibrate.detection.includeThresholdMultiplier = 10.0

# PSF determination
import lsst.meas.astrom.catalogStarSelector
root.calibrate.measurePsf.starSelector.name = "catalog"
root.calibrate.measurePsf.psfDeterminer.name = "pca"
root.calibrate.measurePsf.starSelector["secondMoment"].clumpNSigma = 2.0
root.calibrate.measurePsf.psfDeterminer["pca"].nEigenComponents = 4
root.calibrate.measurePsf.psfDeterminer["pca"].spatialOrder = 2
root.calibrate.measurePsf.psfDeterminer["pca"].kernelSizeMin = 25

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

# Model Mags
try:
    import lsst.meas.extensions.multiShapelet
    root.measurement.algorithms.names += ("multishapelet.psf", "multishapelet.exp", "multishapelet.dev",
                                          "multishapelet.combo")
    root.measurement.slots.modelFlux = "multishapelet.combo.flux"
except ImportError:
    print "meas_extensions_multiShapelet is not setup; disabling model mags"


from lsst.meas.photocal.colorterms import Colorterm
from lsst.obs.suprimecam.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)

Colorterm.setActiveDevice("Hamamatsu")
