import matplotlib
matplotlib.use('Agg')
import pylab as plt

import os
import numpy as np

import lsst.daf.persistence as dafPersist
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.table as afwTable
import lsst.pex.logging as pexLogging
from lsst.meas.deblender.baseline import *
from lsst.meas.deblender import BaselineUtilsF as butils
import lsst.meas.algorithms as measAlg

from astrometry.util.plotutils import *

import lsstDebug
lsstDebug.Info('lsst.meas.deblender.baseline').psf = True


root = pexLogging.Log.getDefaultLog()
root.setThreshold(pexLogging.Log.DEBUG)
# Quiet some of the more chatty loggers
pexLogging.Log(root, 'lsst.meas.deblender.symmetrizeFootprint',
               pexLogging.Log.INFO)
pexLogging.Log(root, 'lsst.meas.deblender.symmetricFootprint',
               pexLogging.Log.INFO)
pexLogging.Log(root, 'lsst.meas.deblender.getSignificantEdgePixels',
               pexLogging.Log.INFO)
pexLogging.Log(root, 'afw.Mask', pexLogging.Log.INFO)


def foot_to_img(foot, img=None):
    fimg = afwImage.ImageF(foot.getBBox())
    fimg.getArray()[:,:] = np.nan
    if foot.isHeavy():
        foot = afwDet.cast_HeavyFootprintF(foot)
        foot.insert(fimg)
        heavy = True
    else:
        if img is None:
            return None,False
        afwDet.copyWithinFootprintImage(foot, img, fimg)
        # ia = img.getArray()
        # fa = fimg.getArray()
        # fbb = fimg.getBBox(afwImage.PARENT)
        # fx0,fy0 = fbb.getMinX(), fbb.getMinY()
        # ibb = img.getBBox(afwImage.PARENT)
        # ix0,iy0 = ibb.getMinX(), ibb.getMinY()
        # for span in foot.getSpans():
        #     y,x0,x1 = span.getY(), span.getX0(), span.getX1()
        #     # print 'Span', y, x0, x1
        #     # print 'img', ix0, iy0
        #     # print 'shape', ia[y - iy0, x0 - ix0: x1+1 - ix0].shape
        #     # print 'fimg', fx0, fy0,
        #     # print 'shape', fa[y - fy0, x0 - fx0: x1+1 - fx0].shape
        #     fa[y - fy0, x0 - fx0: x1+1 - fx0] = ia[y - iy0, x0 - ix0: x1+1 - ix0]
        heavy = False
    return fimg, heavy

def img_to_rgb(im, mn, mx):
    rgbim = np.clip((im-mn)/(mx-mn), 0., 1.)[:,:,np.newaxis].repeat(3, axis=2)
    I = np.isnan(im)
    for i in range(3):
        rgbim[:,:,i][I] = (0.8, 0.8, 0.3)[i]
    I = (im == 0)
    for i in range(3):
        rgbim[:,:,i][I] = (0.5, 0.5, 0.8)[i]
    return rgbim

def bb_to_ext(bb):
    y0,y1,x0,x1 = bb.getMinY(), bb.getMaxY(), bb.getMinX(), bb.getMaxX()
    return [x0-0.5, x1+0.5, y0-0.5, y1+0.5]

def bb_to_xy(bb, margin=0):
    y0,y1,x0,x1 = bb.getMinY(), bb.getMaxY(), bb.getMinX(), bb.getMaxX()
    x0,x1,y0,y1 = x0-margin, x1+margin, y0-margin, y1+margin
    return [x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0]


def makeplots(butler, dataId, ps, sources=None):
    calexp = butler.get("calexp", **dataId)
    if sources is None:
        ss = butler.get('src', **dataId)
    else:
        ss = sources

    #print 'Sources', ss
    #print 'Calexp', calexp
    #print dir(ss)
    
    srcs = {}
    families = {}
    for src in ss:
        sid = src.getId()
        srcs[sid] = src
        parent = src.getParent()
        if parent == 0:
            continue
        if not parent in families:
            families[parent] = []
        families[parent].append(src)
        # print 'Source', src
        # print '  ', dir(src)
        # print '  parent', src.getParent()
        # print '  footprint', src.getFootprint()
    
    print
    lsstimg = calexp.getMaskedImage().getImage()
    img = lsstimg.getArray()
    print 'percentiles', np.percentile(img, 25), np.percentile(img, 95)
    schema = ss.getSchema()
    psfkey = schema.find("deblend.deblended-as-psf").key
    nchildkey = schema.find("deblend.nchild").key
    toomanykey = schema.find("deblend.too-many-peaks").key
    failedkey = schema.find("deblend.failed").key
    
    def getFlagString(src):
        ss = ['Nchild: %i' % src.get(nchildkey)]
        for key,s in [(psfkey, 'PSF'),
                      (toomanykey, 'TooMany'),
                      (failedkey, 'Failed')]:
            if src.get(key):
                ss.append(s)
        return ', '.join(ss)
    
    
    for ifam,(p,kids) in enumerate(families.items()):
    
        if len(kids) < 5:
            print 'Skipping family with', len(kids)
            continue

        print 'ifam', ifam
    
        if ifam != 18:
            print 'skipping'
            continue

        parent = srcs[p]
    
        print 'Parent', parent
        print 'Kids', kids
    
        print 'Parent', parent.getId()
        print 'Kids', [k.getId() for k in kids]
    
        pfoot = parent.getFootprint()
        bb = pfoot.getBBox()
    
        y0,y1,x0,x1 = bb.getMinY(), bb.getMaxY(), bb.getMinX(), bb.getMaxX()
        slc = slice(y0, y1+1), slice(x0, x1+1)
    
        ima = dict(interpolation='nearest', origin='lower', cmap='gray',
                   vmin=-5, vmax=20.)
        mn,mx = ima['vmin'], ima['vmax']
    
        if False:
            plt.clf()
            plt.imshow(img[slc], extent=bb_to_ext(bb), **ima)
            plt.title('Parent %i, %s' % (parent.getId(), getFlagString(parent)))
            ax = plt.axis()
            x,y = bb_to_xy(bb)
            plt.plot(x, y, 'r-', lw=2)
            for i,kid in enumerate(kids):
                kfoot = kid.getFootprint()
                kbb = kfoot.getBBox()
                kx,ky = bb_to_xy(kbb, margin=0.4)
                plt.plot(kx, ky, 'm-')
            for pk in pfoot.getPeaks():
                plt.plot(pk.getIx(), pk.getIy(), 'r+', ms=10, mew=3)
            plt.axis(ax)
            ps.savefig()
    
        print 'parent footprint:', pfoot
        print 'heavy?', pfoot.isHeavy()
        plt.clf()
        pimg,h = foot_to_img(pfoot, lsstimg)

        plt.imshow(img_to_rgb(pimg.getArray(), mn,mx), extent=bb_to_ext(bb), **ima)
        tt = 'Parent %i' % parent.getId()
        if not h:
            tt += ', no HFoot'
        tt += ', ' + getFlagString(parent)
        plt.title(tt)
        ax = plt.axis()
        plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'r-', lw=2)
        for i,kid in enumerate(kids):
            kfoot = kid.getFootprint()
            kbb = kfoot.getBBox()
            kx,ky = bb_to_xy(kbb, margin=-0.1)
            plt.plot(kx, ky, 'm-', lw=1.5)
        for pk in pfoot.getPeaks():
            plt.plot(pk.getIx(), pk.getIy(), 'r+', ms=10, mew=3)
        plt.axis(ax)
        ps.savefig()
    
    
        cols = int(np.ceil(np.sqrt(len(kids))))
        rows = int(np.ceil(len(kids) / float(cols)))
    
        if False:
            plt.clf()
            for i,kid in enumerate(kids):
                plt.subplot(rows, cols, 1+i)
                kfoot = kid.getFootprint()
                print 'kfoot:', kfoot
                print 'heavy?', kfoot.isHeavy()
                #print dir(kid)
                kbb = kfoot.getBBox()
                ky0,ky1,kx0,kx1 = kbb.getMinY(), kbb.getMaxY(), kbb.getMinX(), kbb.getMaxX()
                kslc = slice(ky0, ky1+1), slice(kx0, kx1+1)
                plt.imshow(img[kslc], extent=bb_to_ext(kbb), **ima)
                plt.title('Child %i' % kid.getId())
                plt.axis(ax)
            ps.savefig()
    
        plt.clf()
        for i,kid in enumerate(kids):
            plt.subplot(rows, cols, 1+i)
            kfoot = kid.getFootprint()
            kbb = kfoot.getBBox()
            kimg,h = foot_to_img(kfoot, lsstimg)
            tt = getFlagString(kid)
            if not h:
                tt += ', no HFoot'
            plt.title('%s' % tt)
            if kimg is None:
                plt.axis(ax)
                continue
            plt.imshow(img_to_rgb(kimg.getArray(),mn,mx), extent=bb_to_ext(kbb), **ima)
            for pk in kfoot.getPeaks():
                plt.plot(pk.getIx(), pk.getIy(), 'g+', ms=10, mew=3)
            plt.axis(ax)
        plt.suptitle('Child HeavyFootprints')
        ps.savefig()
    
    
    
        print
        print 'Re-running deblender...'
        psf = calexp.getPsf()
        psf_fwhm = psf.computeShape().getDeterminantRadius() * 2.35
        deb = deblend(pfoot, calexp.getMaskedImage(), psf, psf_fwhm, verbose=True,
                      maxNumberOfPeaks=10,
                      rampFluxAtEdge=True,
                      clipStrayFluxFraction=0.01,
                      )
        print 'Got', deb
    
        def getDebFlagString(kid):
            ss = []
            for k in ['skip', 'outOfBounds', 'tinyFootprint', 'noValidPixels',
                      ('deblendedAsPsf','PSF'), 'psfFitFailed', 'psfFitBadDof',
                      'psfFitBigDecenter', 'psfFitWithDecenter',
                      'failedSymmetricTemplate', 'hasRampedTemplate', 'patched']:
                if len(k) == 2:
                    k,s = k
                else:
                    s = k
                if getattr(kid, k):
                    ss.append(s)
            return ', '.join(ss)
    
        N = len(deb.peaks)
        cols = int(np.ceil(np.sqrt(N)))
        rows = int(np.ceil(N / float(cols)))
    
        for plotnum in range(4):
            plt.clf()
            for i,kid in enumerate(deb.peaks):
                print 'child', kid
                print '  flags:', getDebFlagString(kid)

                kfoot = None
                if plotnum == 0:
                    kfoot = kid.getFluxPortion(strayFlux=False)
                    supt = 'flux portion'
                elif plotnum == 1:
                    kfoot = kid.getFluxPortion(strayFlux=True)
                    supt = 'flux portion + stray'
                elif plotnum == 2:
                    kfoot = afwDet.makeHeavyFootprint(kid.templateFootprint,
                                                      kid.templateMaskedImage)
                    supt = 'template'
                elif plotnum == 3:
                    if kid.deblendedAsPsf:
                        kfoot = afwDet.makeHeavyFootprint(kid.psfFootprint,
                                                          kid.psfTemplate)
                        kfoot.normalize()
                        kfoot.clipToNonzeroF(kid.psfTemplate.getImage())
                        print 'kfoot BB:', kfoot.getBBox()
                        print 'Img bb:', kid.psfTemplate.getImage().getBBox(afwImage.PARENT)
                        for sp in kfoot.getSpans():
                            print '  span', sp
                    else:
                        kfoot = afwDet.makeHeavyFootprint(kid.templateFootprint,
                                                          kid.templateMaskedImage)
                    supt = 'psf template'

                kimg,h = foot_to_img(kfoot, None)
                tt = 'kid %i: %s' % (i, getDebFlagString(kid))
                if not h:
                    tt += ', no HFoot'
                plt.subplot(rows, cols, 1+i)
                plt.title('%s' % tt, fontsize=8)
                if kimg is None:
                    plt.axis(ax)
                    continue
                kbb = kfoot.getBBox()

                plt.imshow(img_to_rgb(kimg.getArray(), mn,mx), extent=bb_to_ext(kbb), **ima)
    
                #plt.imshow(kimg.getArray(), extent=bb_to_ext(kbb), **ima)
    
                plt.axis(ax)
    
            plt.suptitle(supt)
            ps.savefig()


        for i,kid in enumerate(deb.peaks):
            if not kid.deblendedAsPsf:
                continue
            plt.clf()

            ima = dict(interpolation='nearest', origin='lower', cmap='gray')
            #vmin=0, vmax=kid.psfFitFlux)

            plt.subplot(2,3,1)
            #plt.title('fit psf 0')
            #plt.imshow(kid.psfFitDebugPsf0Img.getArray(), **ima)
            #plt.colorbar()
            plt.title('valid pixels')
            plt.imshow(kid.psfFitDebugValidPix, vmin=0, vmax=1, **ima)
            plt.colorbar()

            plt.subplot(2,3,2)
            #plt.title('fit psf')
            #plt.imshow(kid.psfFitDebugPsfImg.getArray(), **ima)
            #plt.colorbar()
            plt.title('variance')
            plt.imshow(kid.psfFitDebugVar.getArray(), vmin=0, **ima)
            plt.colorbar()

            mx = kid.psfFitDebugPsfModel.getArray().max()

            plt.subplot(2,3,3)
            plt.title('fit psf model')
            plt.imshow(kid.psfFitDebugPsfModel.getArray(), vmin=0, vmax=mx, **ima)
            plt.colorbar()

            plt.subplot(2,3,4)
            plt.title('fit psf image')
            plt.imshow(kid.psfFitDebugStamp.getArray(), vmin=0, vmax=mx, **ima)
            plt.colorbar()

            chi = (kid.psfFitDebugValidPix *
                   (kid.psfFitDebugStamp.getArray() -
                    kid.psfFitDebugPsfModel.getArray()) /
                   np.sqrt(kid.psfFitDebugVar.getArray()))

            plt.subplot(2,3,5)
            plt.title('fit psf chi')
            plt.imshow(-chi, vmin=-3, vmax=3, interpolation='nearest', origin='lower', cmap='RdBu')
            plt.colorbar()

            chi,dof = kid.psfFitBest
            plt.suptitle('kid %i: chisq %g, dof %i' % (i, chi, dof))

            ps.savefig()

    
        #if ifam == 5:
        #    break


if __name__ == '__main__':
    import optparse
    import sys

    parser = optparse.OptionParser()
    parser.add_option('--data', help='Data dir, default $SUPRIME_DATA_DIR/rerun/RERUN')
    parser.add_option('--rerun', help='Rerun name, default %default', default='dstn/deb')
    parser.add_option('--visit', help='Visit number, default %default', default=905516, type=int)
    parser.add_option('--ccd', help='CCD number, default %default', default=22, type=int)
    parser.add_option('--sources', help='Read sources file', type=str)
    parsre.add_option('--hdu', help='With --sources, HDU to read; default %default',
                      type=int, default=2)
    opt,args = parser.parse_args()

    if len(args):
        parser.print_help()
        sys.exit(-1)

    if not opt.data:
        opt.data = os.path.join(os.environ['SUPRIME_DATA_DIR'],
                                'rerun', opt.rerun)

    butler = dafPersist.Butler(opt.data)
    dataId = dict(visit=opt.visit, ccd=opt.ccd)
    ps = PlotSequence('deb')

    sources = None
    if opt.sources:
        flags = 0
        srcs = afwTable.SourceCatalog.readFits(opt.sources, opt.hdu, flags)
        print 'Read sources from', opt.sources, ':', srcs

    makeplots(butler, dataId, ps, sources=sources)
