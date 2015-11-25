"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os.path

from lsst.utils import getPackageDir

hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
config.load(os.path.join(hscConfigDir, 'isr.py'))
config.calibrate.photocal.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))
config.calibrate.measurePsf.starSelector.name='objectSize'
config.calibrate.measurePsf.starSelector['objectSize'].widthMin=0.9
config.calibrate.measurePsf.starSelector['objectSize'].fluxMin=4000

config.calibrate.astrometry.wcsFitter.order = 3
config.calibrate.astrometry.matcher.maxMatchDistArcSec = 2.0
config.calibrate.astrometry.matcher.maxOffsetPix = 750

# Do not use NO_DATA pixels for fringe subtraction.
config.isr.fringe.stats.badMaskPlanes=['SAT', 'NO_DATA']
