# Configuration for HSC ISR

from lsst.obs.subaru.isr import SubaruIsrTask
root.isr.retarget(SubaruIsrTask)

root.isr.doBias = False # XXX For now
root.isr.doDark = False
root.isr.doWrite = False
root.isr.doCrosstalk = False
root.isr.doGuider = False


# nonlinearity for SuprimeCam
root.isr.linearizationCoefficient = 2.5e-7

