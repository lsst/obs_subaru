# Configuration for HSC ISR
# Note that config should be an instance of SubaruIsrConfig
import os.path

from lsst.utils import getPackageDir

from lsst.obs.subaru.crosstalk import CrosstalkTask
config.crosstalk.retarget(CrosstalkTask)

config.overscanFitType = "AKIMA_SPLINE"
config.overscanOrder = 30
config.doBias = True # Overscan is fairly efficient at removing bias level, but leaves a line in the middle
config.doDark = True # Required especially around CCD 33
config.doFringe = True
config.fringe.filters = ['y', 'N921', 'N926', 'N973', 'N1010']
config.doWrite = False
config.doCrosstalk = True
config.doGuider = False
config.doBrighterFatter = True

# These values from RHL's report on "HSC July Commissioning Data" (2013-08-23)
config.crosstalk.coeffs.values = [
    0.0e-6, -125.0e-6, -149.0e-6, -156.0e-6,
    -124.0e-6, 0.0e-6, -132.0e-6, -157.0e-6,
    -171.0e-6, -134.0e-6, 0.0e-6, -153.0e-6,
    -157.0e-6, -151.0e-6, -137.0e-6, 0.0e-6,
]

config.vignette.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "vignette.py"))
