import os.path

ObsConfigDir = os.path.dirname(__file__)

charImFile = os.path.join(ObsConfigDir, "charImage.py")
config.load(charImFile)
