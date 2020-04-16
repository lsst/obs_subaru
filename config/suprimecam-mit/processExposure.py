import os.path


config.processCcd.load(os.path.dirname(__file__), "processCcd.py")

config.instrument="suprimecam-mit"
config.processCcd.ignoreCcdList=[0] # Low QE, bad CTE
