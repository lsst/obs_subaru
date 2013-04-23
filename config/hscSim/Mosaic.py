# color terms
from lsst.meas.photocal.colorterms import Colorterm
from lsst.obs.hscSim.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("Hamamatsu")
