from hsc.pipe.tasks.onsiteDb import SuprimecamOnsiteDbTask
config.onsiteDb.retarget(SuprimecamOnsiteDbTask)

# Load regular processCcd configuration
import os.path

from lsst.utils import getPackageDir
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam", "processCcd.py"))
