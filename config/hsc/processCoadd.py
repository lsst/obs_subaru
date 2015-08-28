"""
HSC-specific overrides for ProcessCoaddTask
(applied after Subaru overrides in ../processCoadd.py).
"""

import os.path

from lsst.utils import getPackageDir

hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
config.calibrate.photocal.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))

useApprox = False
bgSize = 4096
bgUndersample = 'REDUCE_INTERP_ORDER'
config.calibrate.background.useApprox = useApprox
config.calibrate.background.binSize = bgSize
config.calibrate.background.undersampleStyle = bgUndersample
config.calibrate.detection.background.useApprox = useApprox
config.calibrate.detection.background.binSize = bgSize
config.calibrate.detection.background.undersampleStyle = bgUndersample
config.detection.background.useApprox = useApprox
config.detection.background.binSize = bgSize
config.detection.background.undersampleStyle = bgUndersample

