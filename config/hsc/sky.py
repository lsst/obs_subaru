import os.path

from lsst.utils import getPackageDir

config.isr.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "isr.py"))

config.largeScaleBackground.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc",
                                              "focalPlaneBackground.py"))

config.isr.doBrighterFatter = False
