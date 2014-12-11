"""Subaru-specific overrides for MultiBandTask"""

import os
for sub in ("detectCoaddSources", "mergeCoaddDetections", "measureCoaddSources", "mergeCoaddMeasurements",
            "forcedPhotCoadd"):
    path = os.path.join(os.environ["OBS_SUBARU_DIR"], "config", sub + ".py")
    if os.path.exists(path):
        getattr(root, sub).load(path)
