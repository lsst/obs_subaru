"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""

import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hscSim', 'isr.py'))

root.calibrate.measurePsf.starSelector.name = "secondMoment" # "objectSize" has problems with corner CCDs

root.calibrate.astrometry.solver.sipOrder = 3
root.calibrate.astrometry.solver.catalogMatchDist = 2.0
root.calibrate.astrometry.solver.rotationAllowedInRad = 0.01

# color terms
from lsst.meas.photocal.colorterms import Colorterm
from lsst.obs.hscSim.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("Hamamatsu")
