import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hscSim', 'isr.py'))

root.darkTime = None

root.isr.doBias = True
root.repair.cosmicray.nCrPixelMax = 1000000
root.repair.cosmicray.minSigma = 5.0
root.repair.cosmicray.min_DN = 50.0
