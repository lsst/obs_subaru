import os.path

from lsst.utils import getPackageDir
from lsst.meas.algorithms import ColorLimit

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")

charImFile = os.path.join(ObsConfigDir, "charImage.py")
config.load(charImFile)
