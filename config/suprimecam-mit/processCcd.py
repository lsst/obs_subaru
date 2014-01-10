"""
SuprimeCam (MIT)-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""
import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam-mit', 'isr.py'))
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'suprimecam-mit', 'colorterms.py'))

root.measurement.algorithms["jacobian"].pixelScale = 0.2
