"""Set color terms for Suprime-Cam MITLL"""

from lsst.pipe.tasks.colorterms import Colorterm
from lsst.obs.suprimecam.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("MIT")
