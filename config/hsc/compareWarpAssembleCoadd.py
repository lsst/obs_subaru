import os.path

# Load configs shared between assembleCoadd and makeCoaddTempExp
config.load(os.path.join(os.path.dirname(__file__), "assembleCoadd.py"))

config.assembleStaticSkyModel.doApplyExternalPhotoCalib = True
config.assembleStaticSkyModel.doApplyExternalSkyWcs = True
