import os
root.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "suprimecam-mit", "processCcd.py"))

root.instrument = "suprimecam-mit"
root.processCcd.ignoreCcdList = [0] # Low QE, bad CTE
