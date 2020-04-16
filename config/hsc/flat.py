import os.path


config.isr.load(os.path.join(os.path.dirname(__file__), "isr.py"))

from lsst.obs.hsc.calibs import HscFlatCombineTask
config.combination.retarget(HscFlatCombineTask)
config.combination.vignette.load(os.path.join(os.path.dirname(__file__), 'vignette.py'))

config.isr.doBrighterFatter = False
config.isr.doStrayLight = False
