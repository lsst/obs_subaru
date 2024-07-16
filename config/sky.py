import os.path

config.isr.load(os.path.join(os.path.dirname(__file__), "isr.py"))

config.largeScaleBackground.load(os.path.join(os.path.dirname(__file__), "focalPlaneBackground.py"))

config.isr.doBrighterFatter = False
