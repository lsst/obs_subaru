# Configuration for Suprime-Cam-MIT ISR

from lsst.obs.subaru.isr import SuprimeCamMitIsrTask
config.isr.retarget(SuprimeCamMitIsrTask)

config.isr.doBias = False
config.isr.doDark = False
config.isr.doCrosstalk = False
config.isr.doLinearize = False
config.isr.doWrite = False
config.isr.doDefect = True

config.isr.doFringe = True
config.isr.fringe.filters = ["I", "i", "z"]
