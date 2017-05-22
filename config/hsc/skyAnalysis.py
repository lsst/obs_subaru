import os.path

from lsst.utils import getPackageDir

config.colorterms.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "colorterms.py"))
config.refObjLoaderConfig.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))
config.refObjLoaderConfig.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "filterMap.py"))
