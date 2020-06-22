"""
Subaru-specific overrides for ProcessCcdWithFakesTask (applied before SuprimeCam- and HSC-specific overrides).
"""
import os.path

from lsst.utils import getPackageDir
from lsst.meas.algorithms import ColorLimit

ObsConfigDir = os.path.join(getPackageDir("obs_subaru"), "config")

calFile = os.path.join(ObsConfigDir, "calibrate.py")

config.calibrate.load(calFile)
