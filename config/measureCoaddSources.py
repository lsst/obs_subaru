"""Subaru-specific overrides for MeasureMergedCoaddSourcesTask"""

import os.path

from lsst.utils import getPackageDir

root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "cmodel.py"))
root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsm.py"))

root.deblend.maxNumberOfPeaks = 20
