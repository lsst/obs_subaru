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

    if False:
        for j,(parent,children) in enumerate(fams):
            print 'parent', parent
            print 'children', children
            plotDeblendFamily(mi, parent, children, cat, sigma1, ellipses=False)
            fn = opt.prefix + '%04i.png' % parent.getId()
            plt.savefig(fn)
            print 'wrote', fn



    def nlmap(X):
        return np.arcsinh(X / (3.*sigma1))
    def myimshow(im, **kwargs):
        kwargs = kwargs.copy()
        mn = kwargs.get('vmin', -5*sigma1)
        kwargs['vmin'] = nlmap(mn)
        mx = kwargs.get('vmax', 100*sigma1)
        kwargs['vmax'] = nlmap(mx)
        plt.imshow(nlmap(im), **kwargs)
    plt.figure(figsize=(4,4))
    plt.subplot(1,1,1)
    plt.subplots_adjust(left=0.05, right=0.95, bottom=0.05, top=0.9,
                        wspace=0.05, hspace=0.1)
    # SDSS intro
    for j,(parent,children) in enumerate(fams):
        print 'parent', parent
        print 'children', children
        pid = parent.getId()
        fp = parent.getFootprint()
        bb = fp.getBBox()
        pim = footprintToImage(parent.getFootprint(), mi).getArray()
        pext = getExtent(bb)
        imargs = dict(interpolation='nearest', origin='lower',
                      vmax=pim.max())

        plt.clf()
        myimshow(afwImage.ImageF(mi.getImage(), bb, afwImage.PARENT).getArray(), **imargs)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        plt.savefig(opt.prefix + 'image-%04i.png' % pid)
        
        plt.clf()
        myimshow(pim, extent=pext, **imargs)
        plt.gray()
        pks = fp.getPeaks()
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], 'r+', mew=2, ms=10, alpha=0.6)
        plt.xticks([])
        plt.yticks([])
        plt.axis(pext)
        plt.savefig(opt.prefix + 'parent-%04i.png' % pid)

        from lsst.meas.deblender.baseline import deblend

        psf = dr.get('psf')
        xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
        yc = int((bb.getMinY() + bb.getMaxY()) / 2.)
        if hasattr(psf, 'getFwhm'):
            psf_fwhm = psf.getFwhm(xc, yc)
        else:
            pa = measAlg.PsfAttributes(psf, xc, yc)
            psfw = pa.computeGaussianWidth(measAlg.PsfAttributes.ADAPTIVE_MOMENT)
            psf_fwhm = 2.35 * psfw

        X = deblend([fp], mi, psf, psf_fwhm, sigma1=sigma1, fit_psfs=False,
                    median_smooth_template=False, monotonic_template=False)
        res = X[0]
        print res

        k = 0
        for pkres,pk in zip(res.peaks, pks):
            #print dir(pkres)
            if not hasattr(pkres, 'timg'):
                continue
            if not hasattr(pkres, 'heavy'):
                continue
            cfp = pkres.heavy
            cbb = cfp.getBBox()
            cext = getExtent(cbb)
            plt.clf()
            myimshow(pkres.timg.getArray(), extent=cext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], 'r+', mew=2, ms=10, alpha=0.6)
            plt.axis(pext)
            plt.savefig(opt.prefix + '%04i-t%i.png' % (pid,k))

            cim = footprintToImage(cfp).getArray()
            plt.clf()
            myimshow(cim, extent=cext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], 'r+', mew=2, ms=10, alpha=0.6)
            plt.axis(pext)
            plt.savefig(opt.prefix + '%04i-h%i.png' % (pid,k))

            k += 1






if __name__ == '__main__':
    main()

