"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""

import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hscSim', 'isr.py'))

root.calibrate.measurePsf.starSelector.name='objectSize'
root.calibrate.measurePsf.starSelector['objectSize'].widthMin=1.0

root.calibrate.astrometry.solver.sipOrder = 3
root.calibrate.astrometry.solver.catalogMatchDist = 2.0

root.measurement.algorithms["jacobian"].pixelScale = 0.168

# color terms
from lsst.meas.photocal.colorterms import Colorterm
from lsst.obs.hscSim.colorterms import colortermsData
Colorterm.setColorterms(colortermsData)
Colorterm.setActiveDevice("Hamamatsu")
