"""
HSC-specific overrides for FgcmFitCycle
"""

config.bands = ('g','r','i','z','Y')
config.fitFlag = (1,1,1,1,0)
config.nCore = 4
config.cycleNumber = 0
config.utBoundary = 0.0
config.washMjds = (0.0,)
config.epochMjds = (0.0,)
config.latitude = 19.8256
config.cameraGain = 3.0
config.pixelScale = 0.17
config.expGrayPhotometricCut = (-0.05,-0.05,-0.05,-0.05,-0.05)
config.aperCorrFitNBins = 0
config.sedFudgeFactors = (1.0,1.0,1.0,1.0,1.0)
config.starColorCuts = ('g,r,-0.25,2.25','r,i,-0.50,2.25','i,z,-0.50,1.00','g,i,0.0,3.5')
