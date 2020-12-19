import os.path

config.colorterms.load(os.path.join(os.path.dirname(__file__), "colorterms.py"))
config.refObjLoader.load(os.path.join(os.path.dirname(__file__), "filterMap.py"))
config.load(os.path.join(os.path.dirname(__file__), "srcSchemaMap.py"))
