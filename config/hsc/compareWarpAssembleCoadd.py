import os.path
from lsst.utils import getPackageDir

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "assembleCoadd.py"))

config.assembleStaticSkyModel.doApplyUberCal = True
