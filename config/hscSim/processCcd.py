"""
HSC-specific overrides for ProcessCcdTask
(applied after Subaru overrides in ../processCcd.py).
"""

import os
root.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hscSim', 'isr.py'))

root.calibrate.measurePsf.starSelector.name = "secondMoment" # "objectSize" has problems with corner CCDs

