from lsst.utils import getPackageDir
import os

configDir = os.path.join(getPackageDir("obs_subaru"), "config")
bgFile = os.path.join(configDir, "background.py")

config.detection.background.load(bgFile)
config.subtractBackground.load(bgFile)
