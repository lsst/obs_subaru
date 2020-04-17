import os.path

from lsst.meas.algorithms import ColorLimit

ObsConfigDir = os.path.join(os.path.dirname(__file__))

charImFile = os.path.join(ObsConfigDir, "charImage.py")
config.load(charImFile)
