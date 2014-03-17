"""
SuprimeCam-specific overrides for ProcessStackTask
(applied after Subaru overrides in ../processStack.py).
"""

import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam', 'colorterms.py'))

