from lsst.obs.subaru.crosstalk import SubaruCrosstalkTask

config.isr.crosstalk.retarget(SubaruCrosstalkTask)

# config = SubaruIsrTask.ConfigClass()
# config.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "isr.py"))
# config.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hsc", "isr.py"))

config.isr.doBias = True
config.isr.doDark = True  # Required especially around CCD 33
config.isr.doFlat = False
config.isr.doFringe = False
config.isr.doLinearize = False
config.isr.doDefect = True

config.isr.doWrite = False

config.isr.doGuider = False
config.isr.doSaturation = True
config.isr.qa.doThumbnailOss = False
config.isr.qa.doThumbnailFlattened = False
config.isr.fringe.filters = ['y', ]
config.isr.overscanFitType = "AKIMA_SPLINE"
config.isr.overscanOrder = 30
# Overscan is fairly efficient at removing bias level, but leaves a line in the middle

config.isr.doStrayLight = False  # added to work around the missing INST-PA throw in some visits
config.isr.crosstalk.value.coeffs.values = [0.0e-6, -125.0e-6, -149.0e-6, -156.0e-6,
                                            -124.0e-6, 0.0e-6, -132.0e-6, -157.0e-6,
                                            -171.0e-6, -134.0e-6, 0.0e-6, -153.0e-6,
                                            -157.0e-6, -151.0e-6, -137.0e-6, 0.0e-6]

# Added by Merlin:
config.isr.doAddDistortionModel = False
config.isr.doUseOpticsTransmission = False
config.isr.doUseFilterTransmission = False
config.isr.doUseSensorTransmission = False
config.isr.doUseAtmosphereTransmission = False
config.isr.doAttachTransmissionCurve = False

config.isrMandatorySteps = ['doAssembleCcd',  # default
                            'doOverscan']  # additional for this obs_package
config.isrForbiddenSteps = ['doApplyGains', 'normalizeGains',  # additional for this obs_package
                            'doGuider', 'doStrayLight', 'doTweakFlat',  # additional for this obs_package
                            'doApplyGains', 'normalizeGains', 'doFlat', 'doFringe',  # remainder are defaults
                            'doAddDistortionModel', 'doBrighterFatter', 'doUseOpticsTransmission',
                            'doUseFilterTransmission', 'doUseSensorTransmission',
                            'doUseAtmosphereTransmission', 'doGuider', 'doStrayLight', 'doTweakFlat']
