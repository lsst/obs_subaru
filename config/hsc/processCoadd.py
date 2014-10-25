"""
HSC-specific overrides for ProcessCoaddTask
(applied after Subaru overrides in ../processCoadd.py).
"""

config.detection.background.useApprox = False
config.detection.background.binSize = 4096
config.detection.background.undersampleStyle = 'REDUCE_INTERP_ORDER'
