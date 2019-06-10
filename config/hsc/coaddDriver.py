# Load from sub-configurations
import os.path

from lsst.utils import getPackageDir

for sub, filename in (("makeCoaddTempExp", "makeCoaddTempExp"),
                      ("backgroundReference", "backgroundReference"),
                      ("assembleCoadd", "compareWarpAssembleCoadd"),
                      ("detectCoaddSources", "detectCoaddSources")):
    path = os.path.join(getPackageDir("obs_subaru"), "config", "hsc", filename + ".py")
    if os.path.exists(path):
        getattr(config, sub).load(path)
