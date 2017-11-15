import os.path
from lsst.utils import getPackageDir

config.bgModel.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "focalPlaneBackground.py"))
