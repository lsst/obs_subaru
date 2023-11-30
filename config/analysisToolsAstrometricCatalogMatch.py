import os.path
from lsst.pipe.tasks.postprocess import TransformObjectCatalogConfig


# By default loop over all the same bands that are present in the Object Table
objectConfig = TransformObjectCatalogConfig()
objectConfig.load(os.path.join(os.path.dirname(__file__), "transformObjectCatalog.py"))

config.bands = ["g", "r", "i", "z", "y"]
config.filterNames = ["HSC-G", "HSC-R", "HSC-I", "HSC-Z", "HSC-Y"]
