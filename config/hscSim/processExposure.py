from lsst.obs.hscSim.processCcd import HscProcessCcdTask
root.processCcd.retarget(HscProcessCcdTask)

import os
root.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hscSim", "processCcd.py"))

root.instrument = "hsc"
