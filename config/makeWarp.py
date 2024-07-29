config.makePsfMatched = True
config.doApplySkyCorr = True

config.modelPsf.defaultFwhm = 7.7
config.warpAndPsfMatch.psfMatch.kernel["AL"].kernelSize = 29
config.warpAndPsfMatch.psfMatch.kernel["AL"].alardSigGauss = [1.0, 2.0, 4.5]
config.warpAndPsfMatch.warp.warpingKernelName = "lanczos5"
config.coaddPsf.warpingKernelName = "lanczos5"
