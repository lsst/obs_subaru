import os

config.match.refObjLoader.load(os.path.join(os.path.dirname(__file__),
                                            "filterMap.py"))

import lsst.obs.subaru.filterFraction
config.measurement.plugins.names.add("subaru_FilterFraction")
