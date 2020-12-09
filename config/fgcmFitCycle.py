from lsst.fgcmcal import Sedterm, Sedboundaryterm

config.outfileBase = 'fgcmHscCalibrations'
config.bands = ['N387', 'g', 'r', 'i', 'N816', 'z', 'N921', 'y', 'N1010']
config.fitBands = ['N387', 'g', 'r', 'i', 'N816', 'z', 'N921', 'y', 'N1010']
config.filterMap = {'g': 'g', 'r': 'r', 'r2': 'r', 'i': 'i', 'i2': 'i',
                    'z': 'z', 'y': 'y',
                    'N387': 'N387', 'N816': 'N816', 'N921': 'N921',
                    'N1010': 'N1010'}
config.maxIterBeforeFinalCycle = 75
config.nCore = 4
config.cycleNumber = 0
config.utBoundary = 0.0
config.washMjds = [56650.0, 57500.0, 57700.0, 58050.0]
config.epochMjds = [56650., 57420., 57606., 59000.]
config.coatingMjds = [56650.0, 58050.0]
config.latitude = 19.8256
config.expGrayPhotometricCutDict = {'N387': -0.05,
                                    'g': -0.05,
                                    'r': -0.05,
                                    'i': -0.05,
                                    'N816': -0.05,
                                    'z': -0.05,
                                    'N921': -0.05,
                                    'y': -0.05,
                                    'N1010': -0.05}
config.expGrayHighCutDict = {'N387': 0.2,
                             'g': 0.2,
                             'r': 0.2,
                             'i': 0.2,
                             'N816': 0.2,
                             'z': 0.2,
                             'N921': 0.2,
                             'y': 0.2,
                             'N1010': 0.2}
config.aperCorrFitNBins = 10
config.aperCorrInputSlopeDict = {'N387': -1.0,
                                 'g': -1.1579,
                                 'r': -1.3908,
                                 'i': -1.1436,
                                 'N816': -1.8149,
                                 'z': -1.6974,
                                 'N921': -1.3310,
                                 'y': -1.2057,
                                 'N1010': -1.0}
config.starColorCuts = ['g,r,-0.25,2.25',
                        'r,i,-0.50,2.25',
                        'i,z,-0.50,1.00',
                        'g,i,0.0,3.5']
config.colorSplitBands = ['g', 'i']
config.freezeStdAtmosphere = True
config.precomputeSuperStarInitialCycle = True
# The shallow narrow-band filters (N387, N1010) do not get
# reliable fits for the super-star flat (illumination correction)
# on a per-ccd level.
config.superStarSubCcdDict = {'N387': False,
                              'g': True,
                              'r': True,
                              'i': True,
                              'N816': True,
                              'z': True,
                              'N921': True,
                              'y': True,
                              'N1010': False}
config.superStarSubCcdChebyshevOrder = 2
# The shallow narrow-band filters (N387, N1010) do not get
# reliable fits for the per-ccd gray correction.
config.ccdGraySubCcdDict = {'N387': False,
                            'g': True,
                            'r': True,
                            'i': True,
                            'N816': True,
                            'z': True,
                            'N921': True,
                            'y': True,
                            'N1010': False}
# All the filters, including the shallow narrow-band filters,
# can be fit per-visit for the full focal plane, improving
# ccd-to-ccd continuity.
config.ccdGrayFocalPlaneDict = {'N387': True,
                                'g': True,
                                'r': True,
                                'i': True,
                                'N816': True,
                                'z': True,
                                'N921': True,
                                'y': True,
                                'N1010': True}
# Only do the full focal plane fitting if over 50% of the focal plane
# is covered on a given visit
config.ccdGrayFocalPlaneFitMinCcd = 50
config.instrumentParsPerBand = True
config.minStarPerExp = 100
config.expVarGrayPhotometricCutDict = {'N387': 0.05,
                                       'g': 0.0025,
                                       'r': 0.0025,
                                       'i': 0.0025,
                                       'N816': 0.05,
                                       'z': 0.0025,
                                       'N921': 0.05,
                                       'y': 0.0025,
                                       'N1010': 0.05}
config.minExpPerNight = 3
config.useRepeatabilityForExpGrayCutsDict = {'N387': True,
                                             'g': False,
                                             'r': False,
                                             'i': False,
                                             'N816': True,
                                             'z': False,
                                             'N921': True,
                                             'y': False,
                                             'N1010': True}
config.sigFgcmMaxEGrayDict = {'N387': 0.15,
                              'g': 0.05,
                              'r': 0.05,
                              'i': 0.05,
                              'N816': 0.15,
                              'z': 0.05,
                              'N921': 0.15,
                              'y': 0.05,
                              'N1010': 0.15}
config.approxThroughputDict = {'N387': 1.0,
                              'g': 1.0,
                              'r': 1.0,
                              'i': 1.0,
                              'N816': 1.0,
                              'z': 1.0,
                              'N921': 1.0,
                              'y': 1.0,
                              'N1010': 1.0}

config.sedboundaryterms.data = {'gr': Sedboundaryterm(primary='g', secondary='r'),
                                'ri': Sedboundaryterm(primary='r', secondary='i'),
                                'iz': Sedboundaryterm(primary='i', secondary='z'),
                                'zy': Sedboundaryterm(primary='z', secondary='y'),
                                'N387g': Sedboundaryterm(primary='N387', secondary='g'),
                                'N816i': Sedboundaryterm(primary='N816', secondary='i'),
                                'N921z': Sedboundaryterm(primary='N921', secondary='z'),
                                'N1010y': Sedboundaryterm(primary='N1010', secondary='y')}
config.sedterms.data = {'g': Sedterm(primaryTerm='gr', secondaryTerm='ri', constant=1.6),
                        'r': Sedterm(primaryTerm='gr', secondaryTerm='ri', constant=0.9),
                        'i': Sedterm(primaryTerm='ri', secondaryTerm='iz', constant=1.0),
                        'z': Sedterm(primaryTerm='iz', secondaryTerm='zy', constant=1.0),
                        'y': Sedterm(primaryTerm='zy', secondaryTerm='iz', constant=0.25,
                                     extrapolated=True, primaryBand='y', secondaryBand='z',
                                     tertiaryBand='i'),
                        'N387': Sedterm(primaryTerm='N387g', constant=1.0),
                        'N816': Sedterm(primaryTerm='N816i', constant=0.7),
                        'N921': Sedterm(primaryTerm='N921z', constant=0.5),
                        'N1010': Sedterm(primaryTerm='N1010y', constant=1.0)}
