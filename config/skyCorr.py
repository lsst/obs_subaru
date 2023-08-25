import os.path

config.bgModel1.load(os.path.join(os.path.dirname(__file__), "focalPlaneBackground.py"))
config.bgModel2.xSize = 256 * 0.015  # in mm
config.bgModel2.ySize = 256 * 0.015  # in mm
config.bgModel2.pixelSize = 0.015  # mm per pixel

# Detection overrides to keep results the same post DM-39796
config.maskObjects.detection.doTempLocalBackground = False
config.maskObjects.detection.thresholdType = "stdev"
config.maskObjects.detection.isotropicGrow = False
