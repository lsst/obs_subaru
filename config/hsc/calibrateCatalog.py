import os
from lsst.utils import getPackageDir

config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "srcSchemaMap.py"))
