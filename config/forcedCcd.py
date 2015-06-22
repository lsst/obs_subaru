import os.path

from lsst.utils import getPackageDir

root.forcedPhotCcd.load(os.path.join(getPackageDir("obs_subaru"), "config", "forcedPhotCcd.py"))
