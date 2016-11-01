import os.path

from lsst.utils import getPackageDir

config.colorterms.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "colorterms.py"))
config.loadAstrom.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "filterMap.py"))
config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "srcSchemaMap.py"))

config.fittingOrder = 9
config.fluxFitOrder = 7
config.fluxFitAbsolute = True
config.internalFitting = True
config.commonFluxCorr = False
config.fluxFitSolveCcd = True
