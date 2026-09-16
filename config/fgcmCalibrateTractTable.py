config.fgcmBuildStars.load("fgcmBuildStarsTable.py")
config.fgcmFitCycle.load("fgcmFitCycle.py")
config.fgcmOutputProducts.load("fgcmOutputProducts.py")
# In tract mode, Do not fit aperture correction terms.
config.fgcmFitCycle.aperCorrFitNBins = 0
# In tract mode, we use "repeatability" metric for cuts for all filters.
config.fgcmFitCycle.useRepeatabilityForExpGrayCutsDict = {
    "N387": True,
    "N395": True,
    "g": True,
    "N515": True,
    "r": True,
    "i": True,
    "N816": True,
    "z": True,
    "N921": True,
    "y": True,
    "N1010": True,
}
