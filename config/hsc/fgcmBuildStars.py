"""
HSC-specific overrides for FgcmBuildStars
"""

config.minPerBand = 2
config.matchRadius = 1.0
config.isolationRadius = 2.0
config.densityCutNside = 128
config.densityCutMaxPerPixel = 1000
config.zeropointDefault = 25.0
config.bands = ('g','r','i','z','y')
config.requiredFlag = (1,1,1,1,0)
config.referenceBand = 'i'
config.referenceCCD = 13


