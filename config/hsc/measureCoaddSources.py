import os
from lsst.utils import getPackageDir

config.match.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc",
                                            "filterMap.py"))

import lsst.obs.subaru.filterFraction
config.measurement.plugins.names.add("subaru_FilterFraction")
