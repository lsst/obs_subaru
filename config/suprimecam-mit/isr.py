"""
SuprimeCam (MIT)-specific overrides for IsrTask
"""
config.doBias = False
config.doDark = False
config.doCrosstalk = False
config.doLinearize = False
config.doWrite = False
config.doDefect = True

config.doFringe = True
config.fringe.filters = ["I", "i", "z"]
