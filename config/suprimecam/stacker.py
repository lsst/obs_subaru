# Load from sub-configurations
import os.path

from lsst.utils import getPackageDir

for sub in ("makeCoaddTempExp", "backgroundReference", "assembleCoadd", "processCoadd"):
    path = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam", sub + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
