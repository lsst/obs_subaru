# Enable measurement of convolved fluxes
# 'config' is a SourceMeasurementConfig
try:
    import lsst.meas.extensions.convolved  # noqa: Load flux.convolved algorithm
except ImportError as exc:
    print("Cannot import lsst.meas.extensions.convolved (%s): disabling convolved flux measurements" % (exc,))
else:
    config.plugins.names.add("ext_convolved_ConvolvedFlux")
    # Default values for 'seeing' and 'aperture.radii' are suitable for HSC.
