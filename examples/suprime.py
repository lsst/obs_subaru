import lsst.obs.suprimecam as obsSc
from utils import *

def _getSuprimeMapper(rootdir=None, calibdir=None, outrootdir=None):
    if rootdir is None:
        rootdir = os.path.join(os.environ['HOME'], 'lsst', 'ACT-data')
    if calibdir is None:
        calibdir = os.path.join(rootdir, 'CALIB')
    mapperArgs = dict(root=rootdir, calibRoot=calibdir, outputRoot=outrootdir)
    mapper = obsSc.SuprimecamMapper(**mapperArgs)
    #return mapper
    wrap = WrapperMapper(mapper)
    return wrap

def _getSuprimeButler(rootdir=None, calibdir=None, outrootdir=None):
    mapper = _getSuprimeMapper(rootdir, calibdir, outrootdir)
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    return butler

def getSuprimeDataref(visit, ccd, single=True, rootdir=None, calibdir=None, outrootdir=None):
    butler = _getSuprimeButler(rootdir=rootdir, calibdir=calibdir, outrootdir=outrootdir)
    print 'Butler', butler
    dataRef = butler.subset('raw', dataId = dict(visit=visit, ccd=ccd))
    print 'dataRef:', dataRef
    print 'len(dataRef):', len(dataRef)
    for dr in dataRef:
        print '  ', dr
    if single:
        assert(len(dataRef) == 1)
        # dataRef doesn't support indexing, but it does support iteration?
        dr = None
        for dr in dataRef:
            break
        assert(dr)
        return dr
    return dataRef
