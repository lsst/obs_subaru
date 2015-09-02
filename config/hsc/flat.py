import os
config.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsc', 'isr.py'))

from lsst.obs.hsc.detrends import HscFlatCombineTask
config.combination.retarget(HscFlatCombineTask)
config.combination.xCenter = -100
config.combination.yCenter = 100
config.combination.radius = 17500
