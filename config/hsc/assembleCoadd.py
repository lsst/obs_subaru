import os.path
from lsst.utils import getPackageDir

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "coaddBase.py"))

config.doAttachTransmissionCurve = True
config.interpImage.transpose = True  # Saturation trails are usually oriented east-west, so along rows
config.coaddPsf.warpingKernelName = 'lanczos5'
