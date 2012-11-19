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
from suprime import *

def main():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--root', dest='root', help='Root directory for Subaru data')
    parser.add_option('--outroot', '-o', dest='outroot', help='Output root directory for Subaru data')
    parser.add_option('-H', '--heavy', dest='heavypat', #default='heavy-%(visit)i-%(ccd)i-%(id)04i.fits',
                      help='Input filename pattern for heavy footprints (with %i pattern); FITS')
    parser.add_option('--drill', '-D', dest='drill', action='append', type=str, default=[],
                      help='Drill down on individual source IDs')
    parser.add_option('--drillxy', dest='drillxy', action='append', type=str, default=[],
                      help='Drill down on individual source positions, eg 132,46;54,67')
    parser.add_option('--visit', dest='visit', type=int, default=108792, help='Suprimecam visit id')
    parser.add_option('--ccd', dest='ccd', type=int, default=5, help='Suprimecam CCD number')
    parser.add_option('--prefix', dest='prefix', default='design-', help='plot filename prefix')
    parser.add_option('--suffix', dest='suffix', default=None, help='plot filename suffix (default: ".png")')
    parser.add_option('--pat', dest='pat', help='Plot filename pattern: eg, "design-%(pid)04i-%(name).png"; overrides --prefix and --suffix')
    parser.add_option('--pdf', dest='pdf', action='store_true', default=False, help='save in PDF format?')
    parser.add_option('-v', dest='verbose', action='store_true')
    parser.add_option('--figw', dest='figw', type=float, help='Figure window width (inches)',
                      default=4.)
    parser.add_option('--figh', dest='figh', type=float, help='Figure window height (inches)',
                      default=4.)
    parser.add_option('--order', dest='order', type=str, help='Child order: eg 3,0,1,2')

    parser.add_option('--sdss', dest='sec', action='store_const', const='sdss',
                      help='Produce plots for the SDSS section.')
    parser.add_option('--mono', dest='sec', action='store_const', const='mono',
                      help='Produce plots for the "monotonic" section.')
    parser.add_option('--median', dest='sec', action='store_const', const='median',
                      help='Produce plots for the "median filter" section.')

    opt,args = parser.parse_args()

    if opt.sec is None:
        opt.sec = 'sdss'
    if opt.pdf:
        if opt.suffix is None:
            opt.suffix = ''
        opt.suffix += '.pdf'
    if not opt.suffix:
        opt.suffix = '.png'

    if opt.pat:
        plotpattern = opt.pat
    else:
        plotpattern = opt.prefix + '%(pid)04i-%(name)s' + opt.suffix

    if opt.order is not None:
        opt.order = [int(x) for x in opt.order.split(',')]
        invorder = np.zeros(len(opt.order))
        invorder[opt.order] = np.arange(len(opt.order))

    def mapchild(i):
        if opt.order is None:
            return i
        return invorder[i]

    def savefig(pid, figname):
        fn = plotpattern % dict(pid=pid, name=figname)
        plt.savefig(fn)

    dr = getSuprimeDataref(opt.visit, opt.ccd, rootdir=opt.root, outrootdir=opt.outroot)

    keepids = None
    if len(opt.drill):
        keepids = []
        for d in opt.drill:
            for dd in d.split(','):
                keepids.append(int(dd))
        print 'Keeping parent ids', keepids

    keepxys = None
    if len(opt.drillxy):
        keepxys = []
        for d in opt.drillxy:
            for dd in d.split(';'):
                xy = dd.split(',')
                assert(len(xy) == 2)
                keepxys.append((int(xy[0]),int(xy[1])))
        print 'Keeping parents at xy', keepxys
        
    cat = readCatalog(None, opt.heavypat, dataref=dr, keepids=keepids,
                      keepxys=keepxys,
                      patargs=dict(visit=opt.visit, ccd=opt.ccd))
    print 'Got', len(cat), 'sources'
    
    exposure = dr.get('calexp')
    print 'Exposure', exposure
    mi = exposure.getMaskedImage()

    sigma1 = get_sigma1(mi)

    fams = getFamilies(cat)
    print len(fams), 'deblend families'
    
    if False:
        for j,(parent,children) in enumerate(fams):
            print 'parent', parent
            print 'children', children
            plotDeblendFamily(mi, parent, children, cat, sigma1, ellipses=False)
            fn = '%04i.png' % parent.getId()
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
    plt.figure(figsize=(opt.figw, opt.figh))
    plt.subplot(1,1,1)
    #plt.subplots_adjust(left=0.05, right=0.95, bottom=0.05, top=0.9,
    #                    wspace=0.05, hspace=0.1)
    plt.subplots_adjust(left=0.01, right=0.99, bottom=0.01, top=0.99,
                        wspace=0.05, hspace=0.1)
    for j,(parent,children) in enumerate(fams):
        print 'parent', parent.getId()
        print 'children', [ch.getId() for ch in children]
        pid = parent.getId()
        fp = parent.getFootprint()
        bb = fp.getBBox()
        pim = footprintToImage(parent.getFootprint(), mi).getArray()
        pext = getExtent(bb)
        px0,py0 = pext[0],pext[2]
        imargs = dict(interpolation='nearest', origin='lower', vmax=pim.max() * 0.95, vmin=-3.*sigma1)
        pksty = dict(linestyle='None', marker='+', color='r', mew=3, ms=20, alpha=0.6)

        plt.clf()
        myimshow(afwImage.ImageF(mi.getImage(), bb, afwImage.PARENT).getArray(), **imargs)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        savefig(pid, 'image')
        
        plt.clf()
        myimshow(pim, extent=pext, **imargs)
        plt.gray()
        pks = fp.getPeaks()
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], **pksty)
        plt.xticks([])
        plt.yticks([])
        plt.axis(pext)
        savefig(pid, 'parent')

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

        # SDSS intro
        kwargs = dict(sigma1=sigma1)

        if opt.sec == 'sdss':
            kwargs.update(fit_psfs=False,
                          median_smooth_template=False, monotonic_template=False,
                          lstsq_weight_templates=True)
        elif opt.sec == 'mono':
            kwargs.update(fit_psfs=False,
                          median_smooth_template=False, monotonic_template=True,
                          lstsq_weight_templates=True)
        elif opt.sec == 'median':
            kwargs.update(fit_psfs=False,
                          median_smooth_template=True, monotonic_template=True,
                          lstsq_weight_templates=True)
        else:
            raise 'Unknown section: "%s"' % opt.sec

        print 'Running deblender...'
        res = deblend(fp, mi, psf, psf_fwhm, **kwargs)
        print 'got result with', [x for x in dir(res) if not x.startswith('__')]
        for pk in res.peaks:
            print 'got peak with', [x for x in dir(pk) if not x.startswith('__')]
            print '  deblend as psf?', pk.deblend_as_psf
        tsum = np.zeros((bb.getHeight(), bb.getWidth()))
        k = 0
        for pkres,pk in zip(res.peaks, pks):
            if not hasattr(pkres, 'timg'):
                continue
            if not hasattr(pkres, 'heavy'):
                continue

            kk = mapchild(k)

            w = pkres.tweight
            cfp = pkres.heavy
            cbb = cfp.getBBox()
            msk = afwImage.ImageF(cbb.getWidth(), cbb.getHeight())
            msk.setXY0(cbb.getMinX(), cbb.getMinY())
            afwDet.setImageFromFootprint(msk, cfp, 1.)
            cext = getExtent(cbb)
            tim = pkres.timg
            tim *= msk
            tim = tim.getArray()
            (x0,x1,y0,y1) = cext
            tsum[y0-py0:y1-py0, x0-px0:x1-px0] += tim
            cim = footprintToImage(cfp).getArray()

            if opt.sec == 'median':
                for im,nm in [(pkres.symm, 'symm'), (pkres.median, 'med')]:
                    #print 'im:', im
                    plt.clf()
                    myimshow(im.getArray(), extent=cext, **imargs)
                    plt.gray()
                    plt.xticks([])
                    plt.yticks([])
                    plt.plot([pk.getIx()], [pk.getIy()], **pksty)
                    plt.axis(pext)
                    savefig(pid, nm + '%i' % (kk))

            plt.clf()
            myimshow(pkres.timg.getArray() / w, extent=cext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.axis(pext)
            savefig(pid, 't%i' % (kk))

            plt.clf()
            myimshow(tim, extent=cext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.axis(pext)
            savefig(pid, 'tw%i' % (kk))

            plt.clf()
            myimshow(cim, extent=cext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.axis(pext)
            savefig(pid, 'h%i' % (kk))

            k += 1

        plt.clf()
        myimshow(tsum, extent=pext, **imargs)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], **pksty)
        plt.axis(pext)
        savefig(pid, 'tsum')

        k = 0
        for pkres,pk in zip(res.peaks, pks):
            if not hasattr(pkres, 'timg'):
                continue
            if not hasattr(pkres, 'heavy'):
                continue
            cfp = pkres.heavy
            cbb = cfp.getBBox()
            cext = getExtent(cbb)
            tim = pkres.timg.getArray()
            (x0,x1,y0,y1) = cext
            frac = tim / tsum[y0-py0:y1-py0, x0-px0:x1-px0]

            msk = afwImage.ImageF(cbb.getWidth(), cbb.getHeight())
            msk.setXY0(cbb.getMinX(), cbb.getMinY())
            afwDet.setImageFromFootprint(msk, cfp, 1.)
            msk = msk.getArray()
            frac[msk == 0.] = np.nan
            
            plt.clf()
            plt.imshow(frac, extent=cext, interpolation='nearest', origin='lower', vmin=0, vmax=1)
            #plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'k-')
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.gca().set_axis_bgcolor((0.9,0.9,0.5))
            plt.axis(pext)
            savefig(pid, 'f%i' % (mapchild(k)))

            k += 1

if __name__ == '__main__':
    main()

