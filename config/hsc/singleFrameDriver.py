"""
hsc-specific overrides for SingleFrameDriverTask 
"""
import os.path

from lsst.utils import getPackageDir

config.doMakeSourceTable = True
config.processCcd.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "processCcd.py"))
config.transformSourceTable.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "transformSourceTable.py"))
config.ignoreCcdList = [9, 104, 105, 106, 107, 108, 109, 110, 111]
