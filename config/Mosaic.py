import os
from lsst.utils import getPackageDir

config.loadAstrom.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))
