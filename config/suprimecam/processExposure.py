import os
root.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "suprimecam", "processCcd.py"))

root.instrument = "suprimecam"

