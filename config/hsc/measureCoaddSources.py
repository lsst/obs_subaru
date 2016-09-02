import os
from lsst.utils import getPackageDir

config.match.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc",
                                                          "filterMap.py"))
