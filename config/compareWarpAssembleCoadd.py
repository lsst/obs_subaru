import os.path
from lsst.utils import getPackageDir

# Load configs from base assembleCoadd 
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "assembleCoadd.py"))

# 200 rows (since patch width is typically < 10k pixels
config.assembleStaticSkyModel.subregionSize = (10000, 200)
