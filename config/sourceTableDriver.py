import os.path
from lsst.utils import getPackageDir

config.ccdKey = 'ccd'
config.ignoreCcdList = [9, 104, 105, 106, 107, 108, 109, 110, 111]
config.transformSourceTable.functorFile = os.path.join(getPackageDir("obs_subaru"), 'policy', 'SourcePDR2.yaml')
