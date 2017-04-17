"""
Subaru-specific overrides for ProcessDonutTask (applied before SuprimeCam- and HSC-specific overrides).
"""
from __future__ import absolute_import, division, print_function
import os.path

from lsst.utils import getPackageDir

configDir = os.path.join(getPackageDir("obs_subaru"), "config")
bgFile = os.path.join(configDir, "background.py")

# Cosmic rays and background estimation
config.charImage.repair.cosmicray.nCrPixelMax = 1000000
config.charImage.repair.cosmicray.cond3_fac2 = 0.4
config.charImage.background.load(bgFile)
config.charImage.detection.background.load(bgFile)

# Detection
config.charImage.detection.isotropicGrow = True
