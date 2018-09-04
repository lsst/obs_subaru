import os.path

from lsst.utils import getPackageDir
from lsst.meas.base import CircularApertureFluxAlgorithm

config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "apertures.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "kron.py"))
config.measurement.load(os.path.join(getPackageDir("obs_subaru"), "config", "convolvedFluxes.py"))
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "cmodel.py"))

config.measurement.slots.gaussianFlux = None

config.measurement.plugins['base_PixelFlags'].masksFpCenter.append('BRIGHT_OBJECT')
config.measurement.plugins['base_PixelFlags'].masksFpAnywhere.append('BRIGHT_OBJECT')

config.catalogCalculation.plugins.names = ["base_ClassificationExtendedness"]
config.measurement.slots.psfFlux = "base_PsfFlux"

def doUndeblended(config, algName, fluxList=None):
    """Activate undeblended measurements of algorithm

    Parameters
    ----------
    algName : `str`
        Algorithm name.
    fluxList : `list` of `str`, or `None`
        List of flux columns to register for aperture correction. If `None`,
        then this will be the `algName` appended with `_instFlux`.
    """
    if algName not in config.measurement.plugins:
        return
    if fluxList is None:
        fluxList = [algName + "_instFlux"]
    config.measurement.undeblended.names.add(algName)
    config.measurement.undeblended[algName] = config.measurement.plugins[algName]
    for flux in fluxList:
        config.applyApCorr.proxies["undeblended_" + flux] = flux


doUndeblended(config, "base_PsfFlux")
doUndeblended(config, "ext_photometryKron_KronFlux")
doUndeblended(config, "base_CircularApertureFlux", [])  # No aperture correction for circular apertures
doUndeblended(config, "ext_convolved_ConvolvedFlux",
              config.measurement.plugins["ext_convolved_ConvolvedFlux"].getAllResultNames())
# Disable registration for apCorr of undeblended convolved; apCorr will be done through the deblended proxy
config.measurement.undeblended["ext_convolved_ConvolvedFlux"].registerForApCorr = False
