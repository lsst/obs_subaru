"""
HSC-specific overrides for ProcessCoaddTask
(applied after Subaru overrides in ../processCoadd.py).
"""

import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsc', 'colorterms.py'))

bgSize = 4096
bgUndersample = 'REDUCE_INTERP_ORDER'
root.calibrate.background.binSize = bgSize
root.calibrate.background.undersampleStyle = bgUndersample
root.calibrate.detection.background.binSize = bgSize
root.calibrate.detection.background.undersampleStyle = bgUndersample
root.detection.background.binSize = bgSize
root.detection.background.undersampleStyle = bgUndersample
