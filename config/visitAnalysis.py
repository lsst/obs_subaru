import os.path

config.colorterms.load(os.path.join(os.path.dirname(__file__), "colorterms.py"))
config.astromRefObjLoader.load(os.path.join(os.path.dirname(__file__), "filterMap.py"))
config.photomRefObjLoader.load(os.path.join(os.path.dirname(__file__), "filterMap.py"))
config.load(os.path.join(os.path.dirname(__file__), "srcSchemaMap.py"))

config.externalPhotoCalibName = 'fgcm'
