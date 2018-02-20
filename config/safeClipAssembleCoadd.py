import os.path
from lsst.utils import getPackageDir

# Load configs from base assembleCoadd
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "assembleCoadd.py"))
