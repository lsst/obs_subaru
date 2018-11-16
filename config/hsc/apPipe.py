# Config override for lsst.ap.pipe.ApPipeTask
import os.path
from lsst.utils import getPackageDir

hscConfigDir = os.path.join(getPackageDir('obs_subaru'), 'config', 'hsc')

config.ccdProcessor.load(os.path.join(hscConfigDir, "processCcd.py"))
