import os
config.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hsc", "processCcd.py"))

config.instrument = "hsc"
