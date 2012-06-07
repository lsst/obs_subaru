import matplotlib
matplotlib.use('Agg')
import lsst.pipe.tasks.processCcd as procCcd
import lsst.obs.suprimecam as obsSc

from utils import *
from suprime import *
from ticket1091 import *

def tweak_config(conf):
    # the default Cosmic Ray parameters don't work for Subaru images
    cr = conf.calibrate.repair.cosmicray
    cr.minSigma = 10.
    cr.min_DN = 500.
    cr.niteration = 3
    cr.nCrPixelMax = 1000000

def main():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--root', '-r', dest='root', help='Root directory for Subaru data')
    parser.add_option('--outroot', '-o', dest='outroot', help='Output root directory for Subaru data')
    parser.add_option('--visit', dest='visit', type=int, default=108792, help='Suprimecam visit id')
    parser.add_option('--ccd', dest='ccd', type=int, default=5, help='Suprimecam CCD number')
    addToParser(parser)
    opt,args = parser.parse_args()

    dr = getSuprimeDataref(visit=opt.visit, ccd=opt.ccd, rootdir=opt.root, outrootdir=opt.outroot)
    print dr

    if opt.heavypat == 'yes':
        opt.heavypat = 'heavy-%(visit)i-%(ccd)i-%(id)04i.fits'

    patargs=dict(visit=opt.visit, ccd=opt.ccd)

    conf = procCcd.ProcessCcdConfig()

    conf.load(os.path.join(os.environ.get('OBS_SUBARU_DIR'), 'config', 'processCcd.py'))
    conf.load(os.path.join(os.environ.get('OBS_SUBARU_DIR'), 'config', 'suprimecam', 'processCcd.py'))

    proc = procCcd.ProcessCcdTask

    pool = None
    if opt.threads:
        import multiprocessing
        pool = multiprocessing.Pool(opt.threads)

    t1091main(dr, opt, conf, proc, patargs=patargs, pool=pool)

if __name__ == '__main__':
    main()
    
