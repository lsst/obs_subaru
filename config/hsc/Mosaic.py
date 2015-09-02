import os
config.load(os.path.join(os.environ['OBS_SUBARU_DIR'], 'config', 'hsc', 'colorterms.py'))

config.fittingOrder = 9
config.fluxFitOrder = 7
config.fluxFitAbsolute = True
config.internalFitting = True
config.commonFluxCorr = False
config.minNumMatch = 5
config.fluxFitSolveCcd = True
