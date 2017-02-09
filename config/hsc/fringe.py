import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SubaruIsrTask

config.isr.retarget(SubaruIsrTask)
config.isr.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "isr.py"))

config.isr.doBrighterFatter = False
