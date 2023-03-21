import os.path

# Demand at least 2 observations of a star to be considered for calibration
config.minPerBand = 2
# High density cut to keep high density regions from dominating statistics.
# 2000 is chosen to be reasonable for typical HSC depths
config.densityCutMaxPerPixel = 2000
# The mapping from HSC short filter name to FGCM "band".  This mapping
# ensures that stars observed in HSC-R + HSC-R2 can be matched, and
# HSC-I + HSC-I2 can be matched.
from lsst.obs.hsc.hscFilters import HSC_FILTER_DEFINITIONS
config.physicalFilterMap = HSC_FILTER_DEFINITIONS.physical_to_band
config.primaryBands = ('i', 'r', 'g', 'z', 'y', 'N387', 'N816', 'N921', 'N1010')

config.doSubtractLocalBackground = True
config.sourceSelector["science"].doFlags = True
config.sourceSelector["science"].flags.bad.append("localBackground_flag")

hscConfigDir = os.path.join(os.path.dirname(__file__))
config.fgcmLoadReferenceCatalog.load(os.path.join(hscConfigDir, 'filterMap.py'))
config.fgcmLoadReferenceCatalog.applyColorTerms = True
config.fgcmLoadReferenceCatalog.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))
config.fgcmLoadReferenceCatalog.referenceSelector.doSignalToNoise = True
# Choose reference catalog signal-to-noise based on the PS1 i-band.
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.fluxField = 'i_flux'
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.errField = 'i_fluxErr'
# Minimum signal-to-noise cut for a reference star to be considered a match.
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.minimum = 10.0

config.connections.ref_cat = 'ps1_pv3_3pi_20170110'
