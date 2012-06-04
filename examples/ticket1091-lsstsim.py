import matplotlib
matplotlib.use('Agg')
import lsst.pipe.tasks.processCcdLsstSim as procCcd
from lsst.obs.lsstSim import LsstSimMapper

from utils import *
from ticket1091 import *

def _getLsstMapper(**kwargs):
    mapper = LsstSimMapper(**kwargs)
    return mapper
    #return WrapperMapper(mapper)

def _getLsstButler(rootdir=None, calibdir=None, outrootdir=None):
    mapper = _getLsstMapper(root=rootdir, calibRoot=calibdir, outputRoot=outrootdir)
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    return butler

def getLsstDataref(visit, raft, sensor, single=True, rootdir=None, calibdir=None, outrootdir=None):
    butler = _getLsstButler(rootdir=rootdir, calibdir=calibdir, outrootdir=outrootdir)
    print 'Butler', butler
    dataRef = butler.subset('raw', dataId = dict(visit=visit, raft=raft, sensor=sensor,))
                                                 #snap=0))
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

def tweak_config(conf):
    if conf.doIsr:
        conf.doSnapCombine

def main():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--root', '-r', dest='root', help='Root directory for LSST sim data')
    parser.add_option('--outroot', '-o', dest='outroot', help='Output root directory for LSST sim data')
    parser.add_option('--visit', dest='visit', type=int, default=0, help='LSST visit id')
    parser.add_option('--raft', dest='raft', type=str, default='2,2', help='LSST raft, default "%default"')
    parser.add_option('--sensor', dest='sensor', type=str, default='2,2', help='LSST sensor, default "%default"')
    addToParser(parser)
    opt,args = parser.parse_args()

    #if True:
    #    mapper = LsstSimMapper(root=opt.root, outputRoot=opt.outroot)
    #    print 'visitCCD', mapper.map('visitCCD', dict(visit=opt.visit, raft=opt.raft, sensor=opt.sensor, snap=0))
    #    print 'visitCCD', mapper.map('visitCCD', dict(visit=opt.visit, raft=opt.raft, sensor=opt.sensor))
    #     #print 'visitCCD', datarefToMapper(dr).map('visitCCD', dr.dataId)
    #     #butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    #     #butler = butlerFactory.create()


    dr = getLsstDataref(visit=opt.visit, raft=opt.raft, sensor=opt.sensor,
                        rootdir=opt.root, outrootdir=opt.outroot)

    if opt.heavypat == 'yes':
        opt.heavypat = 'heavy-v%(visit)i-r%(raft)s-s%(sensor)s-%(id)04i.fits'

    patargs=dict(visit=opt.visit, raft=opt.raft, sensor=opt.sensor)

    conf = procCcd.ProcessCcdLsstSimConfig()
    conf.doSnapCombine = False
    conf.doWriteIsr = False
    conf.doWriteSnapCombine = True
    proc = procCcd.ProcessCcdLsstSimTask

    t1091main(dr, opt, conf, proc, patargs=patargs,
              rundeblendargs=dict(calibInput='postISRCCD', tweak_config=tweak_config))
              #rundeblendargs=dict(calibInput='visitCCD', tweak_config=tweak_config))
              

if __name__ == '__main__':
    main()
    
