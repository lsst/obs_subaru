import os
from lsst.utils import getPackageDir

config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "filterMap.py"))
config.functorFile = os.path.join(getPackageDir("obs_subaru"), 'policy', 'Object.yaml')
