import os.path

from lsst.utils import getPackageDir

config.processCcd.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "processCcd.py"))

config.instrument = "hsc"
