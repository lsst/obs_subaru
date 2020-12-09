import os.path

ObsConfigDir = os.path.dirname(__file__)
calFile = os.path.join(ObsConfigDir, "calibrate.py")
config.calibrate.load(calFile)
