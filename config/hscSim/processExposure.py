import os
root.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "hscSim", "processCcd.py"))

from lsst.obs.hscSim.processCcd import HscProcessCcdTask
root.processCcd.retarget(HscProcessCcdTask)

root.instrument = "hsc"
