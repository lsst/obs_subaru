import os
from lsst.utils import getPackageDir

for filt, filtShort in [("HSC-G", "g"), ("HSC-R", "r"), ("HSC-I", "i"),
                        ("HSC-Z", "z"), ("HSC-Y", "y")]:
    config.filterMap[filt] = filtShort
config.functorFile = os.path.join(getPackageDir("obs_subaru"), 'policy', 'Object.yaml')
