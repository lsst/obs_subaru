"""
SuprimeCam-specific overrides for ProcessCoaddTask
(applied after Subaru overrides in ../processCoadd.py).
"""

import os.path

from lsst.utils import getPackageDir

suprimecamMitConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam-mit")
root.calibrate.photocal.colorterms.load(os.path.join(suprimecamMitConfigDir, 'colorterms.py'))
