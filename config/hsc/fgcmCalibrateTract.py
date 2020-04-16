import os.path

config.fgcmBuildStars.load(os.path.join(os.path.dirname(__file__),
                                        'fgcmBuildStars.py'))
config.fgcmFitCycle.load(os.path.join(os.path.dirname(__file__),
                                      'fgcmFitCycle.py'))
config.fgcmOutputProducts.load(os.path.join(os.path.dirname(__file__),
                                            'fgcmOutputProducts.py'))
config.fgcmFitCycle.aperCorrFitNBins = 0
config.fgcmFitCycle.useRepeatabilityForExpGrayCutsDict = {'N387': True,
                                                          'g': True,
                                                          'r': True,
                                                          'i': True,
                                                          'N816': True,
                                                          'z': True,
                                                          'N921': True,
                                                          'y': True,
                                                          'N1010': True}
