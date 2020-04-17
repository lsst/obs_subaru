# Config override for lsst.ap.pipe.ApPipeTask
import os.path

hscConfigDir = os.path.dirname(__file__)

config.ccdProcessor.load(os.path.join(hscConfigDir, "processCcd.py"))
