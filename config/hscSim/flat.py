import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hscSim', 'isr.py'))

from lsst.obs.hscSim.detrends import HscFlatCombineTask
root.combination.retarget(HscFlatCombineTask)
