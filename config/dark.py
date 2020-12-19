import os.path

config.isr.load(os.path.join(os.path.dirname(__file__), "isr.py"))

config.isr.doBias = True
config.repair.cosmicray.nCrPixelMax = 1000000
config.repair.cosmicray.minSigma = 5.0
config.repair.cosmicray.min_DN = 50.0

config.isr.doBrighterFatter = False
config.isr.doStrayLight = False
