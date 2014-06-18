import os
root.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hsc", "processCcd.py"))

root.instrument = "hsc"
