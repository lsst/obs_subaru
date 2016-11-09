import os.path

from lsst.utils import getPackageDir

config.processCcd.load(os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam", "processCcd.py"))

config.instrument = "suprimecam"
