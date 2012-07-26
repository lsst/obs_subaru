"""
SuprimeCam (MIT)-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
from lsst.obs.subaru.isr import SubaruIsrTask

root.isr.retarget(SubaruIsrTask)  # custom task that adds guider correction
root.isr.doBias = False
root.isr.doDark = False
root.isr.doWrite = False
