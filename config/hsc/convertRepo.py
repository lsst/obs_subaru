import os.path
from lsst.utils import getPackageDir
from lsst.obs.base.gen2to3 import ConvertRepoSkyMapConfig

# Register translators for data IDs.  This is unfortunately a bit fragile; it
# probably won't survive persisting the config.
import lsst.obs.subaru.gen3.hsc.gen2to3

config.collections["raw"] = "raw/hsc"
config.collections["brightObjectMask"] = "masks/hsc"
config.skyMaps["hsc_rings_v1"] = ConvertRepoSkyMapConfig()
config.skyMaps["hsc_rings_v1"].load(os.path.join(getPackageDir("obs_subaru"), "config",
                                                 "makeSkyMap.py"))
config.skyMaps["hsc_rings_v1"].load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc",
                                                 "makeSkyMap.py"))
config.raws.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "ingest-gen3.py"))
config.refCats.append("ps1_pv3_3pi_20170110")
config.collections["ps1_pv3_3pi_20170110"] = "ref_cats"

# ForcedPhotCoadd writes its configs to a filename that doesn't include a
# coaddName prefix, which means the conversion tools can't infer the right
# dataset type from the filename alone.  Because the vast majority of HSC coadd
# processing is on "deep" coadds, we explicitly ignore the other
# <prefix>Coadd_forced_config datasets.  Users who know what is in their own
# repositories can of course override.
config.datasetIgnorePatterns.extend(["dcrCoadd_forced_config",
                                     "goodSeeingCoadd_forced_config",
                                     "psfMatchedCoadd_forced_config"])
