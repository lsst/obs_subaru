import os.path

from lsst.utils import getPackageDir

config.processCcd.load(getPackageDir("obs_subaru"), "config", "suprimecam-mit", "processCcd.py")

config.instrument="suprimecam-mit"
config.processCcd.ignoreCcdList=[0] # Low QE, bad CTE
