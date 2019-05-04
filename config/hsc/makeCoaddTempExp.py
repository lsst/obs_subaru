import os.path
from lsst.utils import getPackageDir

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "coaddBase.py"))

config.doApplySkyCorr = True
config.warpAndPsfMatch.warp.warpingKernelName = 'lanczos5'
config.coaddPsf.warpingKernelName = 'lanczos5'
