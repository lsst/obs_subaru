import os.path

from lsst.utils import getPackageDir

config.fgcmBuildStars.load(os.path.join(getPackageDir('obs_subaru'),
                                        'config',
                                        'hsc',
                                        'fgcmBuildStars.py'))
config.fgcmFitCycle.load(os.path.join(getPackageDir('obs_subaru'),
                                      'config',
                                      'hsc',
                                      'fgcmFitCycle.py'))
config.fgcmOutputProducts.load(os.path.join(getPackageDir('obs_subaru'),
                                            'config',
                                            'hsc',
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
