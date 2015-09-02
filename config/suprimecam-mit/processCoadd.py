import os.path

from lsst.utils import getPackageDir

suprimecamMitConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "suprimecam-mit")
config.calibrate.photocal.colorterms.load(os.path.join(suprimecamMitConfigDir, 'colorterms.py'))
