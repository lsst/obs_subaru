import matplotlib
matplotlib.use('Agg')
from matplotlib.patches import Ellipse
import pylab as plt
import numpy as np
import math
import lsst.pipe.base as pipeBase
import lsst.pipe.tasks.processCcd as procCcd
import lsst.pex.logging as pexLog
import lsst.daf.base as dafBase
import lsst.afw.table as afwTable
import lsst.afw.math  as afwMath
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet

from utils import *

def main():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('-H', '--heavy', dest='heavypat', default='heavy-%(visit)i-%(ccd)i-%(id)04i.fits',
                      help='Output filename pattern for heavy footprints (with %i pattern); FITS')
    #parser.add_option('--drill', '-D', dest='drill', action='append', type=int, default=[],
    #                  help='Drill down on individual source IDs')
    parser.add_option('--drill', '-D', dest='drill', action='append', type=str, default=[],
                      help='Drill down on individual source IDs')
    parser.add_option('--visit', dest='visit', type=int, default=108792, help='Suprimecam visit id')
    parser.add_option('--ccd', dest='ccd', type=int, default=5, help='Suprimecam CCD number')
    parser.add_option('--prefix', dest='prefix', default='design-', help='plot filename prefix')
    parser.add_option('-v', dest='verbose', action='store_true')
    opt,args = parser.parse_args()

    dr = getDataref(opt.visit, opt.ccd)

    keepids = None
    if len(opt.drill):
        keepids = []
        for d in opt.drill:
            for dd in d.split(','):
                keepids.append(int(dd))
        print 'Keeping parent ids', keepids
        
    cat = readCatalog(None, opt.heavypat, dataref=dr, keepids=keepids,
                      patargs=dict(visit=opt.visit, ccd=opt.ccd))
    print 'Got', len(cat), 'sources'
    
    exposure = dr.get('calexp')
    print 'Exposure', exposure
    mi = exposure.getMaskedImage()

    sigma1 = get_sigma1(mi)

    fams = getFamilies(cat)
    for j,(parent,children) in enumerate(fams):
        print 'parent', parent
        print 'children', children

        plotDeblendFamily(mi, parent, children, cat, sigma1, ellipses=False)
        fn = opt.prefix + '%04i.png' % parent.getId()
        plt.savefig(fn)
        print 'wrote', fn

if __name__ == '__main__':
    main()

