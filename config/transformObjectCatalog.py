import os.path

from lsst.utils import getPackageDir

# Always produce these output bands in the Parquet coadd tables, no matter
# what bands are in the input.
config.outputBands = ["g", "r", "i", "z", "y"]

# Use the environment variable to prevent hardcoding of paths
# into quantum graphs.
config.functorFile = os.path.join('$OBS_SUBARU_DIR', 'policy', 'Object.yaml')
