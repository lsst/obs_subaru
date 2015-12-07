import os.path

from lsst.utils import getPackageDir

root.colorterms.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "colorterms.py"))
root.astrometry.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "filterMap.py"))
