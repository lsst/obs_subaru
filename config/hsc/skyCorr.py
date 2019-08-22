import os.path
from lsst.utils import getPackageDir

config.bgModel.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "focalPlaneBackground.py"))
config.bgModel2.xSize = 256*0.015  # in mm
config.bgModel2.ySize = 256*0.015  # in mm
config.bgModel2.pixelSize = 0.015  # mm per pixel
