"""
SuprimeCam-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
from lsst.obs.subaru.isr import SuprimeCamIsrTask

root.isr.retarget(SuprimeCamIsrTask)  # custom task that adds guider correction
root.isr.doBias = False
root.isr.doDark = False
root.isr.doWrite = False

root.calibrate.measurePsf.psfDeterminer["pca"].kernelSize = 10
