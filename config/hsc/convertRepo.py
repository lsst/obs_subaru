import os.path
from lsst.obs.base.gen2to3 import ConvertRepoSkyMapConfig
from lsst.obs.subaru import HyperSuprimeCam

config.runs["brightObjectMask"] = HyperSuprimeCam.makeCollectionName("masks")
config.skyMaps["hsc_rings_v1"] = ConvertRepoSkyMapConfig()
config.skyMaps["hsc_rings_v1"].load(os.path.join(os.path.dirname(__file__), "..", "makeSkyMap.py"))
config.skyMaps["hsc_rings_v1"].load(os.path.join(os.path.dirname(__file__), "makeSkyMap.py"))
# If there's no skymap in the root repo, but some dataset defined on
# tracts/patches is present there (i.e. brightObjectMask), assume this
# skymap.
config.rootSkyMapName = "hsc_rings_v1"

config.defineVisits.load(os.path.join(os.path.join(os.path.dirname(__file__), "defineVisits.py")))

config.refCats.append("ps1_pv3_3pi_20170110")
config.runs["ps1_pv3_3pi_20170110"] = "refcats"

# ForcedPhotCoadd writes its configs to a filename that doesn't include a
# coaddName prefix, which means the conversion tools can't infer the right
# dataset type from the filename alone.  Because the vast majority of HSC coadd
# processing is on "deep" coadds, we explicitly ignore the other
# <prefix>Coadd_forced_config datasets.  Users who know what is in their own
# repositories can of course override.
config.datasetIgnorePatterns.extend(["dcrCoadd_forced_config",
                                     "goodSeeingCoadd_forced_config",
                                     "psfMatchedCoadd_forced_config"])
# Same problem, with assembleCoadd variant metadata; we assume
# "deep_compareWarpAssembleCoadd_metadata" is the one we want.
config.datasetIgnorePatterns.extend(["deep_assembleCoadd_metadata",
                                     "deep_safeClipAssembleCoadd_metadata",
                                     "deep_dcrAssembleCoadd_metadata",])
