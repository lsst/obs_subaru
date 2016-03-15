"""Subaru-specific overrides for MeasureMergedCoaddSourcesTask"""

import os.path

from lsst.utils import getPackageDir

config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
# Turn off cmodel until latest fixes (large blends, footprint merging, etc.) are in
# config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "cmodel.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsm.py"))

config.deblend.load(os.path.join(getPackageDir("obs_subaru"), "config", "deblend.py"))

try:
    # AstrometryTask has a refObjLoader subtask which accepts the filter map.
    config.astrometry.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))
except AttributeError:
    # ANetAstrometryTask does not have a retargetable refObjLoader, but its
    # solver subtask can load the filter map.
    config.astrometry.solver.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))
