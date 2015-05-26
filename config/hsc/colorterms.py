"""Set color terms for HSC"""

from lsst.pipe.tasks.colorterms import Colorterm
from lsst.obs.hsc.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("Hamamatsu")
