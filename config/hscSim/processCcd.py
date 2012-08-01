"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
from lsst.obs.subaru.isr import SubaruIsrTask

root.isr.retarget(SubaruIsrTask)
root.isr.doBias = False
root.isr.doDark = False
root.isr.doWrite = False
root.isr.doCrosstalk = False
root.isr.doGuider = False

root.calibrate.measurePsf.starSelector.name = "secondMoment" # "objectSize" has problems with corner CCDs

