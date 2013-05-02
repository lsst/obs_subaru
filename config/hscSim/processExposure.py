import os
root.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hscSim", "processCcd.py"))

root.instrument = "hsc"
root.processCcd.ignoreCcdList = range(104, 112) # Focus/guide CCDs
