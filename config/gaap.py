# Enable GAaP (Gaussian Aperture and PSF) colors
# 'config' is typically a SourceMeasurementConfig
try:
    import lsst.meas.extensions.gaap  # noqa
    config.plugins.names.add("ext_gaap_GaapFlux")
except ImportError as exc:
    print("Cannot import lsst.meas.extensions.gaap (%s): disabling GAaP flux measurements" % (exc,))
