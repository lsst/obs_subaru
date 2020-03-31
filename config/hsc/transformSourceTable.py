import os
from lsst.utils import getPackageDir

config.functorFile = os.path.join(getPackageDir("obs_subaru"), 'policy', 'Source.yaml')
