import os.path

# `load()` appends to the filterMaps: we need them to be empty, so that only
# the specified filter mappings are used.
config.photometryRefObjLoader.filterMap = {}
config.astrometryRefObjLoader.filterMap = {}
filterMapFile = os.path.join(os.path.dirname(__file__), "filterMap.py")
config.photometryRefObjLoader.load(filterMapFile)
config.astrometryRefObjLoader.load(filterMapFile)
# jointcal default is for Gaia DR2, so we need to clear `anyFilterMapsToThis`.
config.astrometryRefObjLoader.anyFilterMapsToThis = None

config.applyColorTerms = True

config.colorterms.load(os.path.join(os.path.dirname(__file__), "colorterms.py"))
