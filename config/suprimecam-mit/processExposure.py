import os
config.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "suprimecam-mit", "processCcd.py"))

config.instrument = "suprimecam-mit"
config.processCcd.ignoreCcdList = [0] # Low QE, bad CTE
