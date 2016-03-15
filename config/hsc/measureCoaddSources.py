import os
from lsst.utils import getPackageDir

try:
    # AstrometryTask has a refObjLoader subtask which accepts the filter map.
    config.astrometry.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc",
                                                     "filterMap.py"))
except AttributeError:
    # ANetAstrometryTask does not have a retargetable refObjLoader, but its
    # solver subtask can load the filter map.
    config.astrometry.solver.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "filterMap.py"))
