# Config override for lsst.ap.pipe.ApPipeTask
import os.path

subaruConfigDir = os.path.dirname(__file__)

config.ccdProcessor.load(os.path.join(subaruConfigDir, "processCcd.py"))
