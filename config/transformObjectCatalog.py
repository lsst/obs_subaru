import os.path

from lsst.utils import getPackageDir

# Always produce these output bands in the Parquet coadd tables, no matter
# what bands are in the input.
config.outputBands = ["g", "r", "i", "z", "y"]

config.functorFile = os.path.join(getPackageDir("obs_subaru"), 'policy', 'Object.yaml')
