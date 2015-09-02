# Configuration for HSC ISR

from lsst.obs.subaru.isr import SubaruIsrTask
config.isr.retarget(SubaruIsrTask)
from lsst.obs.subaru.crosstalk import CrosstalkTask
config.isr.crosstalk.retarget(CrosstalkTask)

config.isr.overscanFitType = "AKIMA_SPLINE"
config.isr.overscanOrder = 30
config.isr.doBias = True # Overscan is fairly efficient at removing bias level, but leaves a line in the middle
config.isr.doDark = True # Required especially around CCD 33
config.isr.doFringe = True
config.isr.fringe.filters = ['y',]
config.isr.doWrite = False
config.isr.doCrosstalk = True
config.isr.doGuider = False

# These values from RHL's report on "HSC July Commissioning Data" (2013-08-23)
config.isr.crosstalk.coeffs.values = [
       0.0e-6, -125.0e-6, -149.0e-6, -156.0e-6,
    -124.0e-6,    0.0e-6, -132.0e-6, -157.0e-6,
    -171.0e-6, -134.0e-6,    0.0e-6, -153.0e-6,
    -157.0e-6, -151.0e-6, -137.0e-6,    0.0e-6,
       ]
