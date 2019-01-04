"""
HSC-specific overrides for IsrTask
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.crosstalk import SubaruCrosstalkTask
from lsst.obs.subaru.strayLight import SubaruStrayLightTask

config.datasetType = "raw"
config.fallbackFilterName = "HSC-R"
config.expectWcs = True
config.fwhm = 1.0

config.doConvertIntToFloat = True

config.doSaturation=True
config.saturatedMaskName = "SAT"
config.saturation = float("NaN")
config.growSaturationFootprintSize = 1

config.doSuspect=True
config.suspectMaskName = "SUSPECT"
config.numEdgeSuspect = 0

config.doSetBadRegions = True
config.badStatistic = "MEANCLIP"

config.doOverscan = True
config.overscanFitType = "AKIMA_SPLINE"
config.overscanOrder = 30
config.overscanNumSigmaClip = 3.0
config.overscanNumLeadingColumnsToSkip = 0
config.overscanNumTrailingColumnsToSkip = 0
config.overscanMaxDev = 1000.0
config.overscanBiasJump = False
config.overscanBiasJumpKeyword = "NO_SUCH_KEY"
config.overscanBiasJumpDevices = "NO_SUCH_KEY"
config.overscanBiasJumpLocation = -1

config.doAssembleCcd = True
# Use default ISR assembleCcdTask
config.assembleCcd.doTrim = True
config.assembleCcd.keysToRemove = ["PC001001", "PC001002", "PC002001", "PC002002"]
config.doAssembleIsrExposures = False

config.doBias = True
config.biasDataProductName = "bias"

config.doVariance = True
config.gain = float("NaN")
config.readNoise = 0.0
config.doEmpiricalReadNoise = False

config.doLinearize = True


config.doCrosstalk = True
config.doCrosstalkBeforeAssemble = False
config.crosstalk.retarget(SubaruCrosstalkTask)
# These values from RHL's report on "HSC July Commissioning Data" (2013-08-23)
config.crosstalk.coeffs.values = [
    0.0e-6, -125.0e-6, -149.0e-6, -156.0e-6,
    -124.0e-6, 0.0e-6, -132.0e-6, -157.0e-6,
    -171.0e-6, -134.0e-6, 0.0e-6, -153.0e-6,
    -157.0e-6, -151.0e-6, -137.0e-6, 0.0e-6,
]

config.doWidenSaturationTrails = True

config.doBrighterFatter = True
config.brighterFatterKernelFile = ""
config.brighterFatterMaxIter = 10
config.brighterFatterThreshold = 1000.0
config.brighterFatterApplyGain = True

config.doDefect = True
config.doSaturationInterpolation = True

config.doDark = True
config.darkDataProductName = "dark"

config.doStrayLight = True
config.strayLight.retarget(SubaruStrayLightTask)
config.strayLight.doRotatorAngleCorrection=True

config.doFlat = True
config.flatDataProductName = "flat"
config.flatScalingType = "USER"
config.flatUserScale = 1.0
config.doTweakFlat = False

config.doApplyGains = False
config.normalizeGains = False

config.doFringe = True
config.fringeAfterFlat = True
# Use default ISR fringe correction
config.fringe.filters = ['y', 'N921', 'N926', 'N973', 'N1010']
config.fringe.clip = 3.0
config.fringe.iterations = 20
config.fringe.small = 3
config.fringe.large = 30
config.fringe.num = 30000
config.fringe.pedestal = False
config.fringe.stats.badMaskPlanes = ['SAT', 'NO_DATA', 'SUSPECT', 'BAD']
config.fringe.stats.clip = 3.0
config.fringe.stats.iterations = 3
config.fringe.stats.rngSeedOffset = 0
config.fringe.stats.stat = 32

config.doNanInterpAfterFlat = False

config.doAddDistortionModel = True

config.doMeasureBackground = True

config.doCameraSpecificMasking = False

config.fluxMag0T1 = {'g': 398107170553.49854, 'r': 398107170553.49854, 'i': 275422870333.81744, 'z': 120226443461.74132, 'y': 91201083935.59116, 'N515': 20892961308.54041, 'N816': 15848931924.611174, 'N921': 19054607179.632523}

config.doVignette = True
# Use default ISR vignette construction
config.vignette.xCenter = -100
config.vignette.yCenter = 100
config.vignette.radius = 17500
config.vignette.numPolygonPoints = 100
config.vignette.doWriteVignettePolygon = True

config.doAttachTransmissionCurve = True
config.doUseOpticsTransmission = True
config.doUseFilterTransmission = True
config.doUseSensorTransmission = True
config.doUseAtmosphereTransmission = True

config.doWrite = True
