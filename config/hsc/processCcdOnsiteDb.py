from hsc.pipe.tasks.onsiteDb import HscOnsiteDbTask
config.onsiteDb.retarget(HscOnsiteDbTask)

# Load regular processCcd configuration
import os.path

from lsst.utils import getPackageDir

config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "processCcd.py"))
