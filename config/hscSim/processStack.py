"""
HSC-specific overrides for ProcessStackTask
(applied after Subaru overrides in ../processStack.py).
"""

root.calibrate.measurePsf.starSelector.name = "secondMoment" # "objectSize" has problems with corner CCDs
