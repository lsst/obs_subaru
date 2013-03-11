"""
Subaru-specific overrides for ProcessExposureTask (applied before SuprimeCam- and HSC-specific overrides).
"""

import os
root.processCcd.load(os.path.join(os.environ["OBS_SUBARU_DIR"], "config", "processCcd.py"))
