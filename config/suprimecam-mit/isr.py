"""
Suprimecam (MIT)-specific overrides for IsrTask
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.crosstalkYagi import YagiCrosstalkTask
from lsst.obs.subaru.masking import SubaruMaskingTask, SubaruMaskingConfig

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
config.assembleCcd.doTrim = True
config.assembleCcd.keysToRemove = ["PC001001", "PC001002", "PC002001", "PC002002"]
config.doAssembleIsrExposures = False

config.doBias = False
config.biasDataProductName = "bias"

config.doVariance = True
config.gain = float("NaN")
config.readNoise = 0.0
config.doEmpiricalReadNoise = False

config.doLinearize = False

config.doCrosstalk = False
config.crosstalk.retarget(YagiCrosstalkTask)

if False:
    # Crosstalk coefficients for SuprimeCam, as crudely measured by RHL
    config.crosstalk.coeffs = [
        0.00e+00, -8.93e-05, -1.11e-04, -1.18e-04,
        -8.09e-05, 0.00e+00, -7.15e-06, -1.12e-04,
        -9.90e-05, -2.28e-05, 0.00e+00, -9.64e-05,
        -9.59e-05, -9.85e-05, -8.77e-05, 0.00e+00,
    ]

# Crosstalk coefficients derived from Yagi+ 2012
config.crosstalk.coeffs.crossTalkCoeffs1 = [
    0, -0.000148, -0.000162, -0.000167,   # cAA,cAB,cAC,cAD
    -0.000148, 0, -0.000077, -0.000162,     # cBA,cBB,cBC,cBD
    -0.000162, -0.000077, 0, -0.000148,     # cCA,cCB,cCC,cCD
    -0.000167, -0.000162, -0.000148, 0,     # cDA,cDB,cDC,cDD
]
config.crosstalk.coeffs.crossTalkCoeffs2 = [
    0, 0.000051, 0.000050, 0.000053,
    0.000051, 0, 0, 0.000050,
    0.000050, 0, 0, 0.000051,
    0.000053, 0.000050, 0.000051, 0,
]
config.crosstalk.coeffs.relativeGainsPreampAndSigboard = [
    0.949, 0.993, 0.976, 0.996,
    0.973, 0.984, 0.966, 0.977,
    1.008, 0.989, 0.970, 0.976,
    0.961, 0.966, 1.008, 0.967,
    0.967, 0.984, 0.998, 1.000,
    0.989, 1.000, 1.034, 1.030,
    0.957, 1.019, 0.952, 0.979,
    0.974, 1.015, 0.967, 0.962,
    0.972, 0.932, 0.999, 0.963,
    0.987, 0.985, 0.986, 1.012,
]

config.doWidenSaturationTrails = True

config.doBrighterFatter = True
config.brighterFatterKernelFile = ""
config.brighterFatterMaxIter = 10
config.brighterFatterThreshold = 1000.0
config.brighterFatterApplyGain = True

config.doDefect = True
config.doSaturationInterpolation = True

config.doDark = False
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
config.fringe.filters = ['I', 'i', 'z']
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

config.doMeasureBackground = False

config.doCameraSpecificMasking = True
config.masking.retarget(SubaruMaskingTask)
config.masking.suprimeMaskLimitSetA = [1, 4, 5]
config.masking.suprimeMaskLimitSetB = [0, 9]

config.fluxMag0T1 = {'g': 398107170553.49854, 'r': 398107170553.49854, 'i': 275422870333.81744, 'z': 120226443461.74132, 'y': 91201083935.59116, 'N515': 20892961308.54041, 'N816': 15848931924.611174, 'N921': 19054607179.632523}

config.doVignette = False
# Use default ISR vignette construction
config.vignette.xCenter = -100
config.vignette.yCenter = 100
config.vignette.radius = 17500
config.vignette.numPolygonPoints = 100
config.vignette.doWriteVignettePolygon = False

config.doAttachTransmissionCurve = True
config.doUseOpticsTransmission = True
config.doUseFilterTransmission = True
config.doUseSensorTransmission = True
config.doUseAtmosphereTransmission = True

config.doWrite = False
