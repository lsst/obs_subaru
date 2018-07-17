import os.path

from lsst.utils import getPackageDir

config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "srcSchemaMap.py"))
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "extinctionCoeffs.py"))
