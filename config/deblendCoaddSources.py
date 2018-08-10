"""Subaru-specific overrides for DeblendCoaddSourcesTask"""

import os.path
from lsst.utils import getPackageDir
from lsst.meas.algorithms import LoadIndexedReferenceObjectsTask


config.singleBandDeblend.load(os.path.join(getPackageDir("obs_subaru"), "config", "singleBandDeblend.py"))
config.multiBandDeblend.load(os.path.join(getPackageDir("obs_subaru"), "config", "multiBandDeblend.py"))
