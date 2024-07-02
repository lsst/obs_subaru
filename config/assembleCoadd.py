import os.path
from lsst.pipe.tasks.selectImages import PsfWcsSelectImagesTask

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(os.path.dirname(__file__), "coaddBase.py"))

# 200 rows (since patch width is typically < 10k pixels)
config.subregionSize = (10000, 200)
config.doMaskBrightObjects = True
config.removeMaskPlanes.append("CROSSTALK")
config.doNImage = True
config.badMaskPlanes += ["SUSPECT"]
config.doAttachTransmissionCurve = True
# Saturation trails are usually oriented east-west, so along rows
config.interpImage.transpose = True
config.coaddPsf.warpingKernelName = "lanczos5"

config.select.retarget(PsfWcsSelectImagesTask)
