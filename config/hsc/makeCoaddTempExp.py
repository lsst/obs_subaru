import os.path

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(os.path.dirname(__file__), "coaddBase.py"))

config.doApplySkyCorr = True
config.warpAndPsfMatch.warp.warpingKernelName = 'lanczos5'
config.coaddPsf.warpingKernelName = 'lanczos5'
