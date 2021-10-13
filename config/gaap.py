# Enable GAaP (Gaussian Aperture and PSF) colors
# 'config' is typically a SourceMeasurementConfig
try:
    import lsst.meas.extensions.gaap  # noqa
    config.plugins.names.add("ext_gaap_GaapFlux")
    # Enable PSF photometry after PSF-Gaussianization in the `ext_gaap_GaapFlux` plugin
    config.plugins["ext_gaap_GaapFlux"].doPsfPhotometry = True
except ImportError as exc:
    print("Cannot import lsst.meas.extensions.gaap (%s): disabling GAaP flux measurements" % (exc,))
