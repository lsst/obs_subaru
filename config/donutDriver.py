"""
Subaru-specific overrides for DonutDriverTask (applied before SuprimeCam- and HSC-specific overrides).
"""
import os.path

from lsst.utils import getPackageDir

config.processDonut.load(os.path.join(getPackageDir("obs_subaru"), "config", "processDonut.py"))
config.ccdKey = 'ccd'
