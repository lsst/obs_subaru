"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""

import os.path

from lsst.utils import getPackageDir

hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
root.load(os.path.join(hscConfigDir, 'isr.py'))

# root.isr.fwhm=1.8
# root.isr.fwhmForBadColumnInterpolation=1.8
root.calibrate.initialPsf.fwhm=1.0
root.calibrate.detection.includeThresholdMultiplier=5.0

root.calibrate.photocal.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))
root.calibrate.photocal.applyColorTerms = False
# root.calibrate.photocal.refCatName = os.path.join(hscConfigDir, 'colorterms.py')
root.calibrate.measurePsf.starSelector.name='objectSize'
root.calibrate.measurePsf.starSelector['objectSize'].widthMin=0.9
root.calibrate.measurePsf.starSelector['objectSize'].fluxMin=4000

root.calibrate.astrometry.solver.sipOrder = 3
root.calibrate.astrometry.solver.catalogMatchDist = 2.0

# Do not use NO_DATA pixels for fringe subtraction.
root.isr.fringe.stats.badMaskPlanes=['SAT', 'NO_DATA']
