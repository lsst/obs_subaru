"""
Subaru-specific overrides for ProcessExposureTask (applied before SuprimeCam- and HSC-specific overrides).
"""
import os.path

from lsst.utils import getPackageDir

config.processCcd.load(os.path.join(getPackageDir("obs_subaru"), "config", "processCcd.py"))
