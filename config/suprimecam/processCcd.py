"""
SuprimeCam-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam', 'isr.py'))

# color terms
from lsst.meas.photocal.colorterms import Colorterm
from lsst.obs.suprimecam.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("Hamamatsu")
