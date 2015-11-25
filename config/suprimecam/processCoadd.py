"""
SuprimeCam-specific overrides for ProcessCoaddTask
(applied after Subaru overrides in ../processCoadd.py).
"""
try:
    detection = config.detectCoaddSources.detection
except:
    detection = config.detection

detection.background.useApprox = False
detection.background.binSize = 4096
detection.background.undersampleStyle = 'REDUCE_INTERP_ORDER'
