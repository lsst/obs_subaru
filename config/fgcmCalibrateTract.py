import os.path

config.fgcmBuildStars.load(os.path.join(os.path.dirname(__file__),
                                        'fgcmBuildStars.py'))
config.fgcmFitCycle.load(os.path.join(os.path.dirname(__file__),
                                      'fgcmFitCycle.py'))
config.fgcmOutputProducts.load(os.path.join(os.path.dirname(__file__),
                                            'fgcmOutputProducts.py'))
# In tract mode, Do not fit aperture correction terms.
config.fgcmFitCycle.aperCorrFitNBins = 0
# In tract mode, we use "repeatability" metric for cuts for all filters.
config.fgcmFitCycle.useRepeatabilityForExpGrayCutsDict = {'N387': True,
                                                          'g': True,
                                                          'r': True,
                                                          'i': True,
                                                          'N816': True,
                                                          'z': True,
                                                          'N921': True,
                                                          'y': True,
                                                          'N1010': True}
