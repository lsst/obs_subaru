import os.path

filterMapFile = os.path.join(os.path.dirname(__file__), "filterMap.py")
config.photometryRefObjLoader.load(filterMapFile)
config.astrometryRefObjLoader.load(filterMapFile)
config.applyColorTerms = True

config.colorterms.load(os.path.join(os.path.dirname(__file__), "colorterms.py"))
