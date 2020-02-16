"""
HSC-specific overrides for FgcmFitCycle
"""

from lsst.fgcmcal import Sedterm, Sedboundaryterm

config.outfileBase = 'fgcmHscCalibrations'
config.bands = ('N387', 'g', 'r', 'i', 'N816', 'z', 'N921', 'y')
config.fitFlag = (1, 1, 1, 1, 1, 1, 1, 1)
config.requiredFlag = (0, 0, 0, 0, 0, 0, 0, 0)
config.filterMap = {'g': 'g', 'r': 'r', 'r2': 'r', 'i': 'i', 'i2': 'i',
                    'z': 'z', 'y': 'y',
                    'N387': 'N387', 'N816': 'N816', 'N921': 'N921'}
config.maxIterBeforeFinalCycle = 30
config.nCore = 4
config.cycleNumber = 0
config.utBoundary = 0.0
config.washMjds = (56650.0, 57500.0, 57700.0, 58050.0)
config.epochMjds = (56650., 57420., 57606., 59000.)
config.coatingMjds = (56650.0, 58050.0)
config.latitude = 19.8256
config.expGrayPhotometricCut = (-0.05, -0.05, -0.05, -0.05, -0.05, -0.05, -0.05, -0.05)
config.expGrayHighCut = (0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2)
config.aperCorrFitNBins = 10
config.aperCorrInputSlopes = (-1.0150, -0.9694, -1.7229, -1.4549, -1.1998, -1.0, -1.0, -1.0)
config.starColorCuts = ('g,r,-0.25,2.25',
                        'r,i,-0.50,2.25',
                        'i,z,-0.50,1.00',
                        'g,i,0.0,3.5')
config.colorSplitIndices = (1, 3)
config.freezeStdAtmosphere = True
config.precomputeSuperStarInitialCycle = True
config.superStarSubCcdChebyshevOrder = 2
config.ccdGraySubCcd = True
config.instrumentParsPerBand = True
config.minStarPerExp = 100
config.expVarGrayPhotometricCut = 0.0025
config.minExpPerNight = 3

config.useRepeatabilityForExpGrayCuts = (False, False, False, False, False, True, True, True)
config.sigFgcmMaxEGray = (0.05, 0.05, 0.05, 0.05, 0.05, 0.15, 0.15, 0.15)

from lsst.fgcmcal import Sedterm, Sedboundaryterm
config.sedboundaryterms.data = {'gr': Sedboundaryterm(primary='g', secondary='r'),
                                'ri': Sedboundaryterm(primary='r', secondary='i'),
                                'iz': Sedboundaryterm(primary='i', secondary='z'),
                                'zy': Sedboundaryterm(primary='z', secondary='y'),
                                'N387g': Sedboundaryterm(primary='N387', secondary='g'),
                                'N816i': Sedboundaryterm(primary='N816', secondary='i'),
                                'N921z': Sedboundaryterm(primary='N921', secondary='z')}
config.sedterms.data = {'g': Sedterm(primaryTerm='gr', secondaryTerm='ri', constant=1.6),
                        'r': Sedterm(primaryTerm='gr', secondaryTerm='ri', constant=0.9),
                        'i': Sedterm(primaryTerm='ri', secondaryTerm='iz', constant=1.0),
                        'z': Sedterm(primaryTerm='iz', secondaryTerm='zy', constant=1.0),
                        'y': Sedterm(primaryTerm='zy', secondaryTerm='iz', constant=0.25,
                                     extrapolated=True, primaryBand='y', secondaryBand='z',
                                     tertiaryBand='i'),
                        'N387': Sedterm(primaryTerm='N387g', constant=1.0),
                        'N816': Sedterm(primaryTerm='N816i', constant=1.0),
                        'N921': Sedterm(primaryTerm='N921z', constant=1.0)}




