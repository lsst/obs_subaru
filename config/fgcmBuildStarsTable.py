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

config.fgcmLoadReferenceCatalog.refObjLoader.ref_dataset_name = 'ps1_pv3_3pi_20170110'
# This is the mapping from HSC short filter name to the closest PS1 filter from
# the reference catalog.  PS1 does not have narrow-band filters.
config.fgcmLoadReferenceCatalog.refFilterMap = {'HSC-G': 'g', 'HSC-R': 'r', 'HSC-R2': 'r2',
                                                'HSC-I': 'i', 'HSC-I2': 'i', 'HSC-Z': 'z',
                                                'HSC-Y': 'y', 'NB0387': 'g', 'NB0816': 'i',
                                                'NB0921': 'z', 'NB1010': 'y'}
config.fgcmLoadReferenceCatalog.applyColorTerms = True
hscConfigDir = os.path.join(os.path.dirname(__file__))
config.fgcmLoadReferenceCatalog.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))
config.fgcmLoadReferenceCatalog.referenceSelector.doSignalToNoise = True
# Choose reference catalog signal-to-noise based on the PS1 i-band.
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.fluxField = 'i_flux'
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.errField = 'i_fluxErr'
# Minimum signal-to-noise cut for a reference star to be considered a match.
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.minimum = 10.0
