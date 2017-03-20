import os
from lsst.utils import getPackageDir
from lsst.meas.algorithms import LoadIndexedReferenceObjectsTask

config.loadAstrom.retarget(LoadIndexedReferenceObjectsTask)
config.loadAstrom.ref_dataset_name = "ps1_pv3_3pi_20170110"
config.loadAstrom.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))
config.photoCatName = "ps1_pv3_3pi_20170110"
