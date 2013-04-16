# Configuration for Suprime-Cam-MIT ISR

from lsst.obs.subaru.isr import SuprimeCamMitIsrTask
root.isr.retarget(SuprimeCamMitIsrTask)

root.isr.doBias = False
root.isr.doDark = False
root.isr.doCrosstalk = False
root.isr.doLinearize = False
root.isr.doWrite = False
root.isr.doDefect = True
