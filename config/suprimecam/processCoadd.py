"""
SuprimeCam-specific overrides for ProcessCoaddTask
(applied after Subaru overrides in ../processCoadd.py).
"""

import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam', 'colorterms.py'))
