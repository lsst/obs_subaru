import os.path

from lsst.utils import getPackageDir

config.load(os.path.join(getPackageDir("obs_subaru"), "config", "hsc", "colorterms.py"))

config.fittingOrder = 9
config.fluxFitOrder = 7
config.fluxFitAbsolute = True
config.internalFitting = True
config.commonFluxCorr = False
config.minNumMatch = 5
config.fluxFitSolveCcd = True
