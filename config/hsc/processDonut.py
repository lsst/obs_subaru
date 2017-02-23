"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir
from lsst.obs.subaru.isr import SubaruIsrTask

hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
config.isr.retarget(SubaruIsrTask)
config.isr.load(os.path.join(hscConfigDir, 'isr.py'))
config.charImage.measurePsf.starSelector["objectSize"].widthMin = 0.9
config.charImage.measurePsf.starSelector["objectSize"].fluxMin = 4000
for refObjLoader in (config.charImage.refObjLoader,
                     ):
    refObjLoader.load(os.path.join(hscConfigDir, "filterMap.py"))

# Do not use NO_DATA pixels for fringe subtraction.
config.isr.fringe.stats.badMaskPlanes = ['SAT', 'NO_DATA']

config.charImage.measurement.plugins["base_Jacobian"].pixelScale = 0.168
