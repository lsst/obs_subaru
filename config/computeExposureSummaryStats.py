import os.path

config_dir = os.path.dirname(__file__)

config.load(os.path.join(config_dir, "fiducialPsfSigma.py"))
config.load(os.path.join(config_dir, "fiducialSkyBackground.py"))
config.load(os.path.join(config_dir, "fiducialZeroPoint.py"))
