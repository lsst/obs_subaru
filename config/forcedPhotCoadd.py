# Add another aperture flux - it shouldn't be this painful to do (see LSST #2465)
if False: # This doesn't pickle
    from lsst.meas.algorithms.algorithmRegistry import AlgorithmRegistry, SincFluxConfig
    AlgorithmRegistry.register("flux.sinc2", target=SincFluxConfig.Control, ConfigClass=SincFluxConfig)
    root.measurement.algorithms["flux.sinc2"].radius = 5.0
    root.measurement.algorithms.names |= ["flux.sinc2"]

root.measurement.algorithms.names |= ["flux.aperture"]
# Roughly (1.0, 1.4, 2.0, 2.8, 4.0, 5.7, 8.0, 11.3, 16.0, 22.6 arcsec) in diameter: 2**(0.5*i)
# (assumes 0.168 arcsec pixels on coadd)
root.measurement.algorithms["flux.aperture"].radii = [3.0, 4.5, 6.0, 9.0, 12.0, 17.0, 25.0, 35.0, 50.0, 70.0]

root.measurement.slots.instFlux = None

try:
    import lsst.meas.extensions.photometryKron
    root.measurement.algorithms.names |= ["flux.kron"]
except ImportError:
    print "Unable to import lsst.meas.extensions.photometryKron: Kron fluxes disabled"

try:
    import lsst.meas.extensions.multiShapelet
    root.measurement.algorithms.names |= lsst.meas.extensions.multiShapelet.algorithms
    root.measurement.slots.modelFlux = "multishapelet.combo.flux"
except ImportError:
    print "meas_extensions_multiShapelet is not setup; disabling model mags"

