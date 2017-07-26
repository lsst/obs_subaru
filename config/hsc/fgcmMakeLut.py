"""
HSC-specific overrides for FgcmMakeLut
"""

config.bands = ('g','r','i','z','y')
config.pmbRange = (770.0, 790.0)
config.pmbSteps = 2
config.pwvRange = (0.1,12.0)
config.pwvSteps = 2
config.o3Range = (220.0,310.0)
config.o3Steps = 2
config.tauRange = (0.002,0.25)
config.tauSteps = 2
config.alphaRange = (0.0,2.0)
config.alphaSteps = 2
config.zenithRange = (0.0,70.0)
config.zenithSteps = 2
config.pmbStd = 778.0
config.pwvStd = 3.0
config.o3Std = 263.0
config.tauStd = 0.030
config.alphaStd = 1.0
config.airmassStd = 1.1

config.elevation = 4139.0

config.lambdaNorm = 7750.0
config.lambdaStep = 0.5
config.lambdaRange = (3900.0,11000.0)
