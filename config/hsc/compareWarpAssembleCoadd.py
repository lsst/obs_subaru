import os.path
from lsst.utils import getPackageDir

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "assembleCoadd.py"))

config.assembleStaticSkyModel.doApplyUberCal = True
# Continue to default to the meas_mosaic output while the DRP team investigates
# differences between it and jointcal.
config.assembleStaticSkyModel.useMeasMosaic = True
