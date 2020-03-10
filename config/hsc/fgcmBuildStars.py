"""
HSC-specific overrides for FgcmBuildStars
"""

import os.path

from lsst.utils import getPackageDir

config.minPerBand = 2
config.densityCutMaxPerPixel = 2000
config.filterMap = {'g': 'g', 'r': 'r', 'r2': 'r', 'i': 'i', 'i2': 'i',
                    'z': 'z', 'y': 'y',
                    'N387': 'N387', 'N816': 'N816', 'N921': 'N921',
                    'N1010': 'N1010'}
config.primaryBands = ('i', 'r', 'g', 'z', 'y', 'N387', 'N816', 'N921', 'N1010')
config.doSubtractLocalBackground = True

config.fgcmLoadReferenceCatalog.refObjLoader.ref_dataset_name = 'ps1_pv3_3pi_20170110'
config.fgcmLoadReferenceCatalog.refFilterMap = {'g': 'g', 'r': 'r', 'r2': 'r',
                                                'i': 'i', 'i2': 'i', 'z': 'z', 'y': 'y',
                                                'N387': 'g', 'N816': 'i', 'N921': 'z',
                                                'N1010': 'y'}
config.fgcmLoadReferenceCatalog.applyColorTerms = True
hscConfigDir = os.path.join(getPackageDir("obs_subaru"), "config", "hsc")
config.fgcmLoadReferenceCatalog.colorterms.load(os.path.join(hscConfigDir, 'colorterms.py'))
config.fgcmLoadReferenceCatalog.referenceSelector.doSignalToNoise = True
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.fluxField = 'i_flux'
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.errField = 'i_fluxErr'
config.fgcmLoadReferenceCatalog.referenceSelector.signalToNoise.minimum = 10.0

