import os.path

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(os.path.dirname(__file__), "coaddBase.py"))

config.doAttachTransmissionCurve = True
config.interpImage.transpose = True  # Saturation trails are usually oriented east-west, so along rows
config.coaddPsf.warpingKernelName = 'lanczos5'
