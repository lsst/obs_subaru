"""Subaru-specific overrides for MeasureMergedCoaddSourcesTask"""

import os.path

from lsst.utils import getPackageDir

config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
# Turn off cmodel until latest fixes (large blends, footprint merging, etc.) are in
# config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "cmodel.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsm.py"))

config.deblend.load(os.path.join(getPackageDir("obs_subaru"), "config", "deblend.py"))
