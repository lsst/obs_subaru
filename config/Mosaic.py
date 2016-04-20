import os
from lsst.utils import getPackageDir
config.astrom.load(os.path.join(getPackageDir("obs_subaru"), "config", "filterMap.py"))

from lsst.pipe.tasks.setConfigFromEups import setPhotocalConfigFromEups
setPhotocalConfigFromEups(config)
