import os.path

# `load()` appends to the filterMaps: we need them to be empty for HSC, so that
# only the specified filter mappings are used.
config.photometryRefObjLoader.filterMap = {}
filterMapFile = os.path.join(os.path.dirname(__file__), "filterMap.py")
config.photometryRefObjLoader.load(filterMapFile)
# We have PS1 colorterms for HSC.
config.applyColorTerms = True
config.colorterms.load(os.path.join(os.path.dirname(__file__), "colorterms.py"))

# HSC needs a higher order polynomial to track the steepness of the optical
# distortions along the edge of the field. Emperically, this provides a
# measurably better fit than the default order=5.
config.astrometryVisitOrder = 7
