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

from lsst.afw.detection import Psf
from lsst.meas.algorithms import * #pcaPsf

def main():
    '''
    Runs the deblender and creates plots for the "design document",
    doc/design.tex.  See the file NOTES for how to get set up to the
    point where you can actually run this on data.
    '''

    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('--root', dest='root', help='Root directory for Subaru data')
    parser.add_option('--outroot', '-o', dest='outroot', help='Output root directory for Subaru data')
    parser.add_option('--sources', help='Read a FITS table of sources')
    parser.add_option('--calexp', help='Read a FITS calexp')
    parser.add_option('--psf', help='Read a FITS PSF')

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
    parser.add_option('--ramp', dest='sec', action='store_const', const='ramp',
                      help='Produce plots for the "ramp edges" section.')
    parser.add_option('--ramp2', dest='sec', action='store_const', const='ramp2',
                      help='Produce plots for the "ramp edges + stray flux" section.')
    parser.add_option('--patch', dest='sec', action='store_const', const='patch',
                      help='Produce plots for the "patch edges" section.')

    opt,args = parser.parse_args()

    # Logging
    root = pexLog.Log.getDefaultLog()
    if opt.verbose:
        root.setThreshold(pexLog.Log.DEBUG)
    else:
        root.setThreshold(pexLog.Log.INFO)
    # Quiet some of the more chatty loggers
    pexLog.Log(root, 'lsst.meas.deblender.symmetrizeFootprint',
                   pexLog.Log.INFO)
    #pexLog.Log(root, 'lsst.meas.deblender.symmetricFootprint',
    #               pexLog.Log.INFO)
    pexLog.Log(root, 'lsst.meas.deblender.getSignificantEdgePixels',
                   pexLog.Log.INFO)
    pexLog.Log(root, 'afw.Mask', pexLog.Log.INFO)


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

    # Load data using the butler, if desired
    dr = None
    if opt.sources is None or opt.calexp is None:
        print 'Creating DataRef...'
        dr = getSuprimeDataref(opt.visit, opt.ccd, rootdir=opt.root,
                               outrootdir=opt.outroot)
        print 'Got', dr

    # Which parent ids / deblend families are we going to plot?
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
        
    # Read from butler or local file
    cat = readCatalog(opt.sources, None, dataref=dr, keepids=keepids,
                      keepxys=keepxys, patargs=dict(visit=opt.visit, ccd=opt.ccd))
    print 'Got', len(cat), 'sources'

    # Load data from butler or local files
    if opt.calexp is not None:
        print 'Reading exposure from', opt.calexp
        exposure = afwImage.ExposureF(opt.calexp)
    else:
        exposure = dr.get('calexp')
    print 'Exposure', exposure
    mi = exposure.getMaskedImage()

    if opt.psf is not None:
        print 'Reading PSF from', opt.psf
        psf = afwDet.Psf.readFits(opt.psf)
        print 'Got', psf
    elif dr:
        psf = dr.get('psf')
    else:
        psf = exposure.getPsf()
        

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
    plt.subplots_adjust(left=0.01, right=0.99, bottom=0.01, top=0.99,
                        wspace=0.05, hspace=0.1)

    # Make plots for each deblend family.

    for j,(parent,children) in enumerate(fams):
        print 'parent', parent.getId()
        print 'children', [ch.getId() for ch in children]
        print 'parent x,y', parent.getX(), parent.getY()

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
        
        # Parent footprint
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

        xc = int((bb.getMinX() + bb.getMaxX()) / 2.)
        yc = int((bb.getMinY() + bb.getMaxY()) / 2.)
        if hasattr(psf, 'getFwhm'):
            psf_fwhm = psf.getFwhm(xc, yc)
        else:
            psf_fwhm = psf.computeShape().getDeterminantRadius() * 2.35
            
        # Each section of the design doc runs the deblender with different args.
            
        kwargs = dict(sigma1=sigma1, verbose=opt.verbose,
                      getTemplateSum=True)

        basic = kwargs.copy()
        basic.update(fit_psfs=False,
                     median_smooth_template=False,
                     monotonic_template=False,
                     lstsq_weight_templates=False,
                     findStrayFlux=False,
                     rampFluxAtEdge=False,
                     patchEdges=False)

        if opt.sec == 'sdss':
            # SDSS intro
            kwargs = basic
            kwargs.update(lstsq_weight_templates=True)
                          
        elif opt.sec == 'mono':
            kwargs = basic
            kwargs.update(lstsq_weight_templates=True,
                          monotonic_template=True)
        elif opt.sec == 'median':
            kwargs = basic
            kwargs.update(lstsq_weight_templates=True,
                          median_smooth_template=True,
                          monotonic_template=True)
        elif opt.sec == 'ramp':
            kwargs = basic
            kwargs.update(median_smooth_template=True,
                          monotonic_template=True,
                          rampFluxAtEdge=True)

        elif opt.sec == 'ramp2':
            kwargs = basic
            kwargs.update(median_smooth_template=True,
                          monotonic_template=True,
                          rampFluxAtEdge=True,
                          findStrayFlux=True,
                          assignStrayFlux=True)

        elif opt.sec == 'patch':
            kwargs = basic
            kwargs.update(median_smooth_template=True,
                          monotonic_template=True,
                          patchEdges=True)

        else:
            raise 'Unknown section: "%s"' % opt.sec

        print 'Running deblender with kwargs:', kwargs
        res = deblend(fp, mi, psf, psf_fwhm, **kwargs)
        #print 'got result with', [x for x in dir(res) if not x.startswith('__')]
        #for pk in res.peaks:
        #    print 'got peak with', [x for x in dir(pk) if not x.startswith('__')]
        #    print '  deblend as psf?', pk.deblend_as_psf

        # Find bounding-box of all templates.
        tbb = fp.getBBox()
        for pkres,pk in zip(res.peaks, pks):
            tbb.include(pkres.template_foot.getBBox())
        print 'Bounding-box of all templates:', tbb

        # Sum-of-templates plot
        tsum = np.zeros((tbb.getHeight(), tbb.getWidth()))
        tx0,ty0 = tbb.getMinX(), tbb.getMinY()

        # Sum-of-deblended children plot(s)
        # "heavy" bbox == template bbox.
        hsum = np.zeros((tbb.getHeight(), tbb.getWidth()))
        hsum2 = np.zeros((tbb.getHeight(), tbb.getWidth()))

        # Sum of templates from the deblender itself
        plt.clf()
        t = res.templateSum
        myimshow(t.getArray(), extent=getExtent(t.getBBox(afwImage.PARENT)), **imargs)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        savefig(pid, 'tsum1')

        # Make plots for each deblended child (peak)
        
        k = 0
        for pkres,pk in zip(res.peaks, pks):

            heavy = pkres.get_flux_portion()
            if heavy is None:
                print 'Child has no HeavyFootprint -- skipping'
                continue

            kk = mapchild(k)

            w = pkres.template_weight

            cfp = pkres.template_foot
            cbb = cfp.getBBox()
            cext = getExtent(cbb)

            # Template image
            tim = pkres.template_mimg.getImage()
            timext = cext
            tim = tim.getArray()

            (x0,x1,y0,y1) = timext
            print 'tim ext', timext
            tsum[y0-ty0:y1-ty0, x0-tx0:x1-tx0] += tim

            # "Heavy" image -- flux assigned to child
            him = footprintToImage(heavy).getArray()
            hext = getExtent(heavy.getBBox())

            (x0,x1,y0,y1) = hext
            hsum[y0-ty0:y1-ty0, x0-tx0:x1-tx0] += him

            # "Heavy" without stray flux
            h2 = pkres.get_flux_portion(strayFlux=False)
            him2 = footprintToImage(h2).getArray()
            hext2 = getExtent(h2.getBBox())
            (x0,x1,y0,y1) = hext2
            hsum2[y0-ty0:y1-ty0, x0-tx0:x1-tx0] += him2
            
            if opt.sec == 'median':
                try:
                    med = pkres.median_filtered_template
                except:
                    med = pkres.orig_template

                for im,nm in [(pkres.orig_template, 'symm'), (med, 'med')]:
                    #print 'im:', im
                    plt.clf()
                    myimshow(im.getArray(), extent=cext, **imargs)
                    plt.gray()
                    plt.xticks([])
                    plt.yticks([])
                    plt.plot([pk.getIx()], [pk.getIy()], **pksty)
                    plt.axis(pext)
                    savefig(pid, nm + '%i' % (kk))

            # Template
            plt.clf()
            myimshow(pkres.template_mimg.getImage().getArray() / w, extent=cext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.axis(pext)
            savefig(pid, 't%i' % (kk))

            # Weighted template
            plt.clf()
            myimshow(tim, extent=cext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.axis(pext)
            savefig(pid, 'tw%i' % (kk))

            # "Heavy"
            plt.clf()
            myimshow(him, extent=hext, **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.axis(pext)
            savefig(pid, 'h%i' % (kk))

            # Original symmetric template
            plt.clf()
            t = pkres.orig_template
            foot = pkres.orig_foot
            myimshow(t.getArray(), extent=getExtent(foot.getBBox()), **imargs)
            plt.gray()
            plt.xticks([])
            plt.yticks([])
            plt.plot([pk.getIx()], [pk.getIy()], **pksty)
            plt.axis(pext)
            savefig(pid, 'o%i' % (kk))

            if opt.sec == 'patch' and pkres.patched:
                pass
            
            if opt.sec in ['ramp','ramp2'] and pkres.has_ramped_template:

                # Ramped template
                plt.clf()
                t = pkres.ramped_template
                myimshow(t.getArray(), extent=getExtent(t.getBBox(afwImage.PARENT)),
                         **imargs)
                plt.gray()
                plt.xticks([])
                plt.yticks([])
                plt.plot([pk.getIx()], [pk.getIy()], **pksty)
                plt.axis(pext)
                savefig(pid, 'r%i' % (kk))

                # Median-filtered template
                plt.clf()
                t = pkres.median_filtered_template
                myimshow(t.getArray(), extent=getExtent(t.getBBox(afwImage.PARENT)),
                         **imargs)
                plt.gray()
                plt.xticks([])
                plt.yticks([])
                plt.plot([pk.getIx()], [pk.getIy()], **pksty)
                plt.axis(pext)
                savefig(pid, 'med%i' % (kk))

                # Assigned flux
                plt.clf()
                t = pkres.portion_mimg.getImage()
                myimshow(t.getArray(), extent=getExtent(t.getBBox(afwImage.PARENT)),
                         **imargs)
                plt.gray()
                plt.xticks([])
                plt.yticks([])
                plt.plot([pk.getIx()], [pk.getIy()], **pksty)
                plt.axis(pext)
                savefig(pid, 'p%i' % (kk))

            if opt.sec == 'ramp2':
                # stray flux
                if pkres.stray_flux is not None:
                    s = pkres.stray_flux
                    strayim = footprintToImage(s).getArray()
                    strayext = getExtent(s.getBBox())

                    plt.clf()
                    myimshow(strayim, extent=strayext, **imargs)
                    plt.gray()
                    plt.xticks([])
                    plt.yticks([])
                    plt.plot([pk.getIx()], [pk.getIy()], **pksty)
                    plt.axis(pext)
                    savefig(pid, 's%i' % (kk))

                    # Assigned flux, omitting stray flux.
                    plt.clf()
                    myimshow(him2, extent=hext2, **imargs)
                    plt.gray()
                    plt.xticks([])
                    plt.yticks([])
                    plt.plot([pk.getIx()], [pk.getIy()], **pksty)
                    plt.axis(pext)
                    savefig(pid, 'hb%i' % (kk))


            k += 1

        # sum of templates
        plt.clf()
        myimshow(tsum, extent=getExtent(tbb), **imargs)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], **pksty)
        plt.axis(pext)
        savefig(pid, 'tsum')

        # sum of assigned flux
        plt.clf()
        myimshow(hsum, extent=getExtent(tbb), **imargs)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], **pksty)
        plt.axis(pext)
        savefig(pid, 'hsum')

        plt.clf()
        myimshow(hsum2, extent=getExtent(tbb), **imargs)
        plt.gray()
        plt.xticks([])
        plt.yticks([])
        plt.plot([pk.getIx() for pk in pks], [pk.getIy() for pk in pks], **pksty)
        plt.axis(pext)
        savefig(pid, 'hsum2')

        k = 0
        for pkres,pk in zip(res.peaks, pks):
            heavy = pkres.get_flux_portion()
            if heavy is None:
                continue

            print 'Template footprint:', pkres.template_foot.getBBox()
            print 'Template img:', pkres.template_mimg.getBBox(afwImage.PARENT)
            print 'Heavy footprint:', heavy.getBBox()

            cfp = pkres.template_foot
            cbb = cfp.getBBox()
            cext = getExtent(cbb)
            tim = pkres.template_mimg.getImage().getArray()
            (x0,x1,y0,y1) = cext

            frac = tim / tsum[y0-ty0:y1-ty0, x0-tx0:x1-tx0]

            msk = afwImage.ImageF(cbb.getWidth(), cbb.getHeight())
            msk.setXY0(cbb.getMinX(), cbb.getMinY())
            afwDet.setImageFromFootprint(msk, cfp, 1.)
            msk = msk.getArray()
            frac[msk == 0.] = np.nan

            # Fraction of flux assigned to this child.
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

