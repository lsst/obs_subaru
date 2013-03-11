"""
SuprimeCam-specific overrides for ProcessStackTask
(applied after Subaru overrides in ../processStack.py).
"""

# color terms
from lsst.meas.photocal.colorterms import Colorterm
from lsst.obs.suprimecam.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("Hamamatsu")

