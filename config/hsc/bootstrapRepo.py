import os.path
from lsst.utils import getPackageDir
from lsst.obs.base.gen3 import BootstrapRepoSkyMapConfig

config.skymaps["hsc_rings_v1"] = BootstrapRepoSkyMapConfig()
config.skymaps["hsc_rings_v1"].load(os.path.join(getPackageDir("obs_subaru"), "config",
                                                 "makeSkyMap.py"))
config.skymaps["hsc_rings_v1"].load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc",
                                                 "makeSkyMap.py"))
config.raws.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "ingest-gen3.py"))
config.brightObjectMasks.skymap = "hsc_rings_v1"
config.brightObjectMasks.collection = "masks/hsc"
config.calibrations.collection = "calibs/hsc/default"
