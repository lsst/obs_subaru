"""Subaru-specific overrides for MeasureMergedCoaddSourcesTask"""

import os.path

from lsst.utils import getPackageDir

root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
# Turn off cmodel until latest fixes (large blends, footprint merging, etc.) are in
# root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "cmodel.py"))
root.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsm.py"))

root.deblend.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "deblend.py"))
