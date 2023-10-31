import os.path

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(os.path.dirname(__file__), "coaddBase.py"))

config.doSigmaClip = False
# 200 rows (since patch width is typically < 10k pixels)
config.assembleCoadd.subregionSize = (10000, 200)
config.assembleCoadd.doMaskBrightObjects = True
config.assembleCoadd.removeMaskPlanes.append("CROSSTALK")
config.assembleCoadd.doNImage = True
config.assembleCoadd.badMaskPlanes += ["SUSPECT"]
config.assembleCoadd.doAttachTransmissionCurve = True
# Saturation trails are usually oriented east-west, so along rows
config.interpImage.transpose = True
config.coaddPsf.warpingKernelName = 'lanczos5'

from lsst.pipe.tasks.selectImages import PsfWcsSelectImagesTask
config.select.retarget(PsfWcsSelectImagesTask)
