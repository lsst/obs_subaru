# Load from sub-configurations
import os
for sub in ("makeCoaddTempExp", "backgroundReference", "assembleCoadd", "processCoadd"):
    path = os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "suprimecam", sub + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
