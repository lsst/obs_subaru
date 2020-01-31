import os.path
from lsst.utils import getPackageDir

config.recalibrate.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "recalibrate.py"))
