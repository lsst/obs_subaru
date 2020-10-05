"""
hsc-specific overrides for SingleFrameDriverTask
"""
import os.path


config.doMakeSourceTable = True
config.doSaveWideSourceTable = True
config.processCcd.load(os.path.join(os.path.dirname(__file__), "processCcd.py"))
config.transformSourceTable.load(os.path.join(os.path.dirname(__file__), "transformSourceTable.py"))
config.ignoreCcdList = [9, 104, 105, 106, 107, 108, 109, 110, 111]
