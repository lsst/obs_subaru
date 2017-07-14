import os.path

from lsst.utils import getPackageDir

config.colorterms.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "colorterms.py"))
config.refObjLoader.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "filterMap.py"))
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "srcSchemaMap.py"))
