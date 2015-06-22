"""Subaru-specific overrides for MultiBandTask"""

import os.path

from lsst.utils import getPackageDir

for sub in ("detectCoaddSources", "mergeCoaddDetections", "measureCoaddSources", "mergeCoaddMeasurements",
            "forcedPhotCoadd"):
    path = os.path.join(getPackageDir("obs_subaru"), "config", sub + ".py")
    if os.path.exists(path):
        getattr(root, sub).load(path)
