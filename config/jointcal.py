import os.path

if hasattr(config.astrometryRefObjLoader, "ref_dataset_name"):
    config.astrometryRefObjLoader.ref_dataset_name = 'ps1_pv3_3pi_20170110'
if hasattr(config.photometryRefObjLoader, "ref_dataset_name"):
    config.photometryRefObjLoader.ref_dataset_name = 'ps1_pv3_3pi_20170110'
# existing PS1 refcat does not have coordinate errors
config.astrometryReferenceErr = 10

filterMapFile = os.path.join(os.path.dirname(__file__), "filterMap.py")
config.photometryRefObjLoader.load(filterMapFile)
config.astrometryRefObjLoader.load(filterMapFile)

config.astrometryVisitOrder = 7
