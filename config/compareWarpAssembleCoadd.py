import os.path

# Load configs from base assembleCoadd
config.load(os.path.join(os.path.dirname(__file__), "assembleCoadd.py"))

# 200 rows (since patch width is typically < 10k pixels
config.assembleStaticSkyModel.subregionSize = (10000, 200)
config.assembleStaticSkyModel.doApplyExternalPhotoCalib = True
config.assembleStaticSkyModel.externalPhotoCalibName = 'fgcm'
config.assembleStaticSkyModel.doApplyExternalSkyWcs = True
