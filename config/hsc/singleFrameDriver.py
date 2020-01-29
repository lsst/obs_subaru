"""
hsc-specific overrides for SingleFrameDriverTask 
"""
import os.path

from lsst.utils import getPackageDir

config.processCcd.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "processCcd.py"))
config.ignoreCcdList = [9, 104, 105, 106, 107, 108, 109, 110, 111]
