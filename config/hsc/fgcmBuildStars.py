"""
HSC-specific overrides for FgcmBuildStars
"""

config.minPerBand = 2
config.matchRadius = 1.0
config.isolationRadius = 2.0
config.densityCutNside = 128
config.densityCutMaxPerPixel = 1000
config.zeropointDefault = 25.0
config.filterToBand = {'g':'g', 'r':'r', 'i':'i', 'i2':'i', 'z':'z', 'y':'y'}
config.requiredBands = ['g','r','i','z']
config.referenceBand = 'i'
config.referenceCCD = 13
config.matchNside = 4096



