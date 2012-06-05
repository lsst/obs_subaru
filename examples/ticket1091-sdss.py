import matplotlib
matplotlib.use('Agg')
import lsst.pipe.tasks.processCcd as procCcd
import lsst.obs.sdss as obsSdss

from utils import *
from ticket1091 import *

def getSdssDataref(run, frame, camcol, band, root=None, calibRoot=None, outputRoot=None,
                   **kwargs):
    mapper = obsSdss.SdssMapper(root=root, calibRoot=calibRoot, outputRoot=outputRoot)
    mapper = WrapperMapper(mapper)
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    dataRef = butler.subset('fpC', dataId = dict(run=run, frame=frame, camcol=camcol, band=band))
    print 'dataRef:', dataRef
    for dr in dataRef:
        print '  ', dr.dataId
        return dr
    raise RuntimeError('No data found')
    #return dataRef

def main():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--root', '-r', dest='root', help='Root directory for data')
    parser.add_option('--outroot', '-o', dest='outroot', help='Output root directory')
    parser.add_option('--run', dest='run', type=int, default=6360, help='SDSS run')
    parser.add_option('--frame', dest='frame', type=int, default=237, help='SDSS frame')
    parser.add_option('--camcol', dest='camcol', type=int, default=3, help='SDSS camcol')
    parser.add_option('--band', dest='band', type=str, default='r', help='SDSS band')
    addToParser(parser)
    opt,args = parser.parse_args()

    kwargs = {}
    #if opt.root:
    #    kwargs['registry'] = os.path.join(opt.root, 'registry.sqlite3')

    dr = getSdssDataref(opt.run, opt.frame, opt.camcol, opt.band,
                        root=opt.root, outputRoot=opt.outroot, **kwargs)
    print dr

    if opt.heavypat == 'yes':
        opt.heavypat = 'heavy-%(run)04i-%(camcol)i%(band)s-%(frame)04i-%(id)04i.fits'

    patargs=dict(run=opt.run, frame=opt.frame, camcol=opt.camcol, band=opt.band)

    conf = procCcd.ProcessCcdConfig()
    conf.load(os.path.join(os.environ.get('OBS_SDSS_DIR'), 'config', 'processCcd.py'))
    proc = procCcd.ProcessCcdTask

    t1091main(dr, opt, conf, proc, patargs=patargs)

if __name__ == '__main__':
    main()
    
