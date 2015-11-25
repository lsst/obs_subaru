import os.path

from lsst.utils import getPackageDir

config.load(os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam-mit", "isr.py"))
config.isr.doGuider = False

config.combination.clip = 2.5
