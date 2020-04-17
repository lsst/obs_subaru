import os.path

config.bgModel.load(os.path.join(os.path.dirname(__file__), "focalPlaneBackground.py"))
config.bgModel2.xSize = 256*0.015  # in mm
config.bgModel2.ySize = 256*0.015  # in mm
config.bgModel2.pixelSize = 0.015  # mm per pixel
