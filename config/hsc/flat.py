import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsc', 'isr.py'))

from lsst.obs.hsc.detrends import HscFlatCombineTask
root.combination.retarget(HscFlatCombineTask)
root.combination.xCenter = -100
root.combination.yCenter = 100
root.combination.radius = 17500
