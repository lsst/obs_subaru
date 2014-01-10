"""
SuprimeCam-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam', 'isr.py'))
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam', 'colorterms.py'))

root.measurement.algorithms["jacobian"].pixelScale = 0.2

