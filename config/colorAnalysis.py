import os.path

config.load(os.path.join(os.path.dirname(__file__), "srcSchemaMap.py"))
config.load(os.path.join(os.path.dirname(__file__), "extinctionCoeffs.py"))
from lsst.obs.hsc.hscFilters import HSC_FILTER_DEFINITIONS
config.physicalToBandFilterMap = HSC_FILTER_DEFINITIONS.physical_to_band
