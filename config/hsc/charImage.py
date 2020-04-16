"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path


from lsst.meas.astrom import MatchOptimisticBConfig

ObsConfigDir = os.path.join(os.path.dirname(__file__))

config.measurePsf.starSelector["objectSize"].widthMin = 0.9
config.measurePsf.starSelector["objectSize"].fluxMin = 4000
for refObjLoader in (config.refObjLoader,
                     ):
    refObjLoader.load(os.path.join(ObsConfigDir, "filterMap.py"))


config.ref_match.sourceSelector.name = 'matcher'
for matchConfig in (config.ref_match,
                    ):
    matchConfig.sourceFluxType = 'Psf'
    matchConfig.sourceSelector.active.sourceFluxType = 'Psf'
    matchConfig.matcher.maxRotationDeg = 1.145916
    matchConfig.matcher.maxOffsetPix = 250
    if isinstance(matchConfig.matcher, MatchOptimisticBConfig):
        matchConfig.matcher.allowedNonperpDeg = 0.2
        matchConfig.matcher.maxMatchDistArcSec = 2.0
        matchConfig.sourceSelector.active.excludePixelFlags = False

config.measurement.plugins["base_Jacobian"].pixelScale = 0.168

# For aperture correction modeling, only use objects that were used in the
# PSF model and have psf flux signal-to-noise > 200.
config.measureApCorr.sourceSelector['science'].doFlags = True
config.measureApCorr.sourceSelector['science'].doUnresolved = False
config.measureApCorr.sourceSelector['science'].doSignalToNoise = True
config.measureApCorr.sourceSelector['science'].flags.good = ["calib_psf_used"]
config.measureApCorr.sourceSelector['science'].flags.bad = []
config.measureApCorr.sourceSelector['science'].signalToNoise.minimum = 200.0
config.measureApCorr.sourceSelector['science'].signalToNoise.maximum = None
config.measureApCorr.sourceSelector['science'].signalToNoise.fluxField = "base_PsfFlux_instFlux"
config.measureApCorr.sourceSelector['science'].signalToNoise.errField = "base_PsfFlux_instFluxErr"
config.measureApCorr.sourceSelector.name = "science"
