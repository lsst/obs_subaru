# Enable Kron mags
# 'root' is a SourceMeasurementConfig

try:
    import lsst.meas.extensions.photometryKron
    root.algorithms.names |= ["flux.kron"]
except ImportError:
    print "Cannot import lsst.meas.extensions.photometryKron: disabling Kron measurements"
