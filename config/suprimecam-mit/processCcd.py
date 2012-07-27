"""
SuprimeCam (MIT)-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
from lsst.obs.subaru.isr import SubaruIsrTask

root.isr.retarget(SubaruIsrTask)
root.isr.doBias = False
root.isr.doDark = False
root.isr.doCrosstalk = False
root.isr.doLinearize = False
root.isr.doGuider = False
root.isr.doWrite = False

# color terms
from lsst.meas.photocal.colorterms import Colorterm
from lsst.obs.suprimecam.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("MIT")
