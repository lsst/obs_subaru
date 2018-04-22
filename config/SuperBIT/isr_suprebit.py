# Configuration for SuperBIT ISR
# Note that config should be an instance of SubaruIsrConfig
import os.path

from lsst.utils import getPackageDir

from lsst.obs.subaru.crosstalk import CrosstalkTask
config.crosstalk.retarget(CrosstalkTask)

config.overscanFitType = "AKIMA_SPLINE"
config.overscanOrder = 30
config.doBias = True # Overscan is fairly efficient at removing bias level, but leaves a line in the middle
config.doDark = True # Required especially around CCD 33
config.doFringe = False
config.doWrite = False ##changed 
config.doCrosstalk = False
config.doGuider = False
config.doBrighterFatter = False
config.doFlat=True

