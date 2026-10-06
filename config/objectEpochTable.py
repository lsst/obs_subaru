from lsst.pipe.tasks.postprocess import TransformObjectCatalogConfig

# By default loop over all the same bands that are present in the Object Table
objectConfig = TransformObjectCatalogConfig()
objectConfig.load("transformObjectCatalog.py")
config.bands = objectConfig.outputBands
