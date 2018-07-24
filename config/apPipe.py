# Config override for lsst.ap.pipe.ApPipeTask
import os.path
from lsst.utils import getPackageDir

subaruConfigDir = os.path.join(getPackageDir('obs_subaru'), 'config')

config.ccdProcessor.load(os.path.join(subaruConfigDir, "processCcd.py"))
