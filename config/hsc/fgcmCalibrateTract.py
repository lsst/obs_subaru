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
