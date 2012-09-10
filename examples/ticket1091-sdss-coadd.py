import matplotlib
matplotlib.use('Agg')

import lsst.pipe.tasks.processCoadd as procCcd
import lsst.obs.sdss as obsSdss

from utils import *
from ticket1091 import *

'''

python examples/ticket1091-sdss-coadd.py -r /lsst7/stripe82/dr7-coadds/v3/run0d \
  --force-deblend --force-detection --no-deblend-plots --no-measure-plots --threads 8 \
  --overview=sdss-t0p351-0.png -o scout --nsigma 10 -v

'''

class CoaddWrapper(object):
    def __init__(self, real, prefix):
        self.real = real
        self.prefix = prefix
    def map(self, x, *args, **kwargs):
        print 'Map', x, args, kwargs
        xorig = x
        if not x.startswith(self.prefix):
            x = self.prefix + x
        try:
            return self.real.map(x, *args, **kwargs)
        except:
            print 'Mapping', x, 'failed; falling back to', xorig
            return self.real.map(xorig, *args, **kwargs)
        
    def __getattr__(self, x):
        return getattr(self.real, x)
        

def main():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--root', '-r', dest='root', help='Root directory for data')
    parser.add_option('--outroot', '-o', dest='outroot', help='Output root directory')
    parser.add_option('--tract', dest='tract', type=int, default=0, help='Coadd tract')
    parser.add_option('--patch', dest='patch', type=str, default="351,0", help='Coadd patch: int,int')
    parser.add_option('--filter', dest='filter', type=str, default='r', help='SDSS band')
    parser.add_option('--type', dest='coaddType', type=str, default='goodSeeing', help='Coadd type: "deep" or "goodSeeing"')
    parser.add_option('--nsigma', dest='nsigma', type=float, default=None, help='N sigma for detection')
    addToParser(parser)
    opt,args = parser.parse_args()

    kwargs = {}

    mapper = obsSdss.SdssMapper(root=opt.root, calibRoot=None, outputRoot=opt.outroot)
    mapper = WrapperMapper(mapper)
    mapper = CoaddWrapper(mapper, opt.coaddType + 'Coadd_')
    butlerFactory = dafPersist.ButlerFactory(mapper = mapper)
    butler = butlerFactory.create()
    dataRef = butler.dataRef(opt.coaddType + 'Coadd',
                             dataId = dict(tract=opt.tract, patch=opt.patch, filter=opt.filter))
    print 'dataRef:', dataRef
    dr = dataRef

    #print 'Getting calexp:', dr.get('calexp')

    if opt.heavypat == 'yes':
        opt.heavypat = 'heavy-%(filter)s-%(tract)i-%(patch)s-%(id)04i.fits'

    patargs=dict(filter=opt.filter, tract=opt.tract, patch=opt.patch)
    conf = procCcd.ProcessCoaddConfig()
    conf.coaddName = opt.coaddType
    if opt.nsigma:
        conf.detection.thresholdValue = opt.nsigma
        print 'Set detection threhold to', opt.nsigma, 'sigma'
    conf.load(os.path.join(os.environ.get('OBS_SDSS_DIR'), 'config', 'processCoadd.py'))
    proc = procCcd.ProcessCoaddTask

    t1091main(dr, opt, conf, proc, patargs=patargs,
              rundeblendargs=dict(printMap=False, hasIsr=False))

if __name__ == '__main__':
    main()
    
