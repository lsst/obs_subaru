import os.path

from lsst.utils import getPackageDir

config.load(os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam", "isr.py"))
config.darkTime = "EXP1TIME"
config.isr.doGuider = False

config.combination.clip = 2.5
