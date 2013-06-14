#!/usr/bin/env python
import matplotlib
matplotlib.use('Agg')
import pylab as plt
doPlot = True
import numpy as np

import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.pex.logging as pexLogging
from lsst.meas.deblender.baseline import *
from lsst.meas.deblender import BaselineUtilsF as butils

import lsst.meas.algorithms as measAlg

root = pexLogging.Log.getDefaultLog()
root.setThreshold(pexLogging.Log.DEBUG)
# Quiet some of the more chatty loggers
log = pexLogging.Log(root, 'lsst.meas.deblender.symmetrizeFootprint',
                     pexLogging.Log.INFO)
log = pexLogging.Log(root, 'lsst.meas.deblender.symmetricFootprint',
                     pexLogging.Log.INFO)
log = pexLogging.Log(root, 'lsst.meas.deblender.getSignificantEdgePixels',
                     pexLogging.Log.INFO)

def imExt(img):
    bbox = img.getBBox(afwImage.PARENT)
    return [bbox.getMinX(), bbox.getMaxX(),
            bbox.getMinY(), bbox.getMaxY()]

def doubleGaussianPsf(W, H, fwhm1, fwhm2, a2):
    return measAlg.DoubleGaussianPsf(W, H, fwhm1, fwhm2, a2)
    
def gaussianPsf(W, H, fwhm):
    return measAlg.DoubleGaussianPsf(W, H, fwhm)


if __name__ == '__main__':
    H,W = 100,100
    fpbb = afwGeom.Box2I(afwGeom.Point2I(0,0),
                         afwGeom.Point2I(W-1,H-1))

    afwimg = afwImage.MaskedImageF(fpbb)
    imgbb = afwimg.getBBox(afwImage.PARENT)
    img = afwimg.getImage().getArray()

    var = afwimg.getVariance().getArray()
    var[:,:] = 1.
    
    blob_fwhm = 15.
    blob_psf = doubleGaussianPsf(201, 201, blob_fwhm, 3.*blob_fwhm, 0.03)

    #fakepsf_fwhm = 3.
    fakepsf_fwhm = 5.
    S = int(np.ceil(fakepsf_fwhm * 2.)) * 2 + 1
    print 'S', S
    fakepsf = gaussianPsf(S, S, fakepsf_fwhm)

    blobimgs = []
    XY = [(50.,50.), (90.,50.)]
    flux = 1e6
    for x,y in XY:
        bim = blob_psf.computeImage(afwGeom.Point2D(x, y))
        bbb = bim.getBBox(afwImage.PARENT)
        bbb.clip(imgbb)

        bim = bim.Factory(bim, bbb, afwImage.PARENT)
        bim2 = bim.getArray()

        blobimg = np.zeros_like(img)
        blobimg[bbb.getMinY():bbb.getMaxY()+1,
                bbb.getMinX():bbb.getMaxX()+1] += flux * bim2
        blobimgs.append(blobimg)

        img[bbb.getMinY():bbb.getMaxY()+1,
            bbb.getMinX():bbb.getMaxX()+1] += flux * bim2

    # Run the detection code to get a ~ realistic footprint
    #thresh = afwDet.createThreshold(40., 'value', True)
    thresh = afwDet.createThreshold(10., 'value', True)
    fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
    fps = fpSet.getFootprints()
    print 'found', len(fps), 'footprints'

    # set EDGE bit.
    margin = 5
    lo = imgbb.getMin()
    lo.shift(afwGeom.Extent2I(margin, margin))
    hi = imgbb.getMax()
    hi.shift(afwGeom.Extent2I(-margin, -margin))
    goodbbox = afwGeom.Box2I(lo, hi)
    print 'Good bbox for setting EDGE pixels:', goodbbox
    print 'image bbox:', imgbb
    edgebit = afwimg.getMask().getPlaneBitMask("EDGE")
    print 'edgebit:', edgebit
    measAlg.SourceDetectionTask.setEdgeBits(afwimg, goodbbox, edgebit)

    if False:
        plt.clf()
        plt.imshow(afwimg.getMask().getArray(),
                   interpolation='nearest', origin='lower')
        plt.colorbar()
        plt.title('Mask')
        plt.savefig('mask.png')

        M = afwimg.getMask().getArray()
        for bit in range(32):
            mbit = (1 << bit)
            if not np.any(M & mbit):
                continue
            plt.clf()
            plt.imshow(M & mbit,
                       interpolation='nearest', origin='lower')
            plt.colorbar()
            plt.title('Mask bit %i (0x%x)' % (bit, mbit))
            plt.savefig('mask-%02i.png' % bit)

    for fp in fps:
        print 'peaks:', len(fp.getPeaks())
        for pk in fp.getPeaks():
            print '  ', pk.getIx(), pk.getIy()
    assert(len(fps) == 1)
    fp = fps[0]
    assert(len(fp.getPeaks()) == 2)
    
    ima = dict(interpolation='nearest', origin='lower', #cmap='gray',
               cmap='jet',
               vmin=0, vmax=400)
    
    for j,(tt,kwa) in enumerate([
            ('No edge treatment', dict()),
            ('Ramp by PSF', dict(rampFluxAtEdge=True)),
            ('No clip at edge', dict(patchEdges=True)),
        ]):
        print 'Deblending...'
        deb = deblend(fp, afwimg, fakepsf, fakepsf_fwhm, verbose=True,
                      **kwa)
        print 'Result:', deb
        print len(deb.peaks), 'deblended peaks'

        parent_img = afwImage.ImageF(fpbb)
        butils.copyWithinFootprint(fp, afwimg.getImage(), parent_img)

        X = [x for x,y in XY]
        Y = [y for x,y in XY]
        PX = [pk.getIx() for pk in fp.getPeaks()]
        PY = [pk.getIy() for pk in fp.getPeaks()]

        def myimshow(*args, **kwargs):
            x0,x1,y0,y1 = imExt(afwimg)
            plt.fill([x0,x0,x1,x1,x0],[y0,y1,y1,y0,y0], color=(1,1,0.8),
                     zorder=20)
            plt.imshow(*args, zorder=25, **kwargs)
            plt.xticks([]); plt.yticks([])
            plt.axis(imExt(afwimg))

        plt.clf()

        pa = dict(color='m', marker='.', linestyle='None', zorder=30)
        
        R,C = 3,6
        plt.subplot(R, C, (2*C) + 1)
        myimshow(img, **ima)
        ax = plt.axis()
        plt.plot(X, Y, **pa)
        plt.axis(ax)
        plt.title('Image')

        plt.subplot(R, C, (2*C) + 2)
        myimshow(parent_img.getArray(), **ima)
        ax = plt.axis()
        plt.plot(PX, PY, **pa)
        plt.axis(ax)
        plt.title('Footprint')

        sumimg = None
        for i,dpk in enumerate(deb.peaks):

            plt.subplot(R, C, i*C + 1)
            myimshow(blobimgs[i], **ima)
            ax = plt.axis()
            plt.plot(PX[i], PY[i], **pa)
            plt.axis(ax)
            plt.title('true')

            plt.subplot(R, C, i*C + 2)
            myimshow(dpk.symm.getArray(), extent=imExt(dpk.symm), **ima)
            ax = plt.axis()
            plt.plot(PX[i], PY[i], **pa)
            plt.axis(ax)
            plt.title('symm')

            # monotonic template
            mimg = afwImage.ImageF(fpbb)
            butils.copyWithinFootprint(dpk.tfoot, dpk.timg, mimg)

            plt.subplot(R, C, i*C + 3)
            myimshow(mimg.getArray(), extent=imExt(mimg), **ima)
            ax = plt.axis()
            plt.plot(PX[i], PY[i], **pa)
            plt.axis(ax)
            plt.title('monotonic')

            plt.subplot(R, C, i*C + 4)
            myimshow(dpk.portion.getArray(), extent=imExt(dpk.portion), **ima)
            plt.title('portion')
            ax = plt.axis()
            plt.plot(PX[i], PY[i], **pa)
            plt.axis(ax)

            if dpk.stray is not None:
                simg = afwImage.ImageF(fpbb)
                dpk.stray.insert(simg)
            
                plt.subplot(R, C, i*C + 5)
                myimshow(simg.getArray(), **ima)
                plt.title('stray')
                ax = plt.axis()
                plt.plot(PX, PY, **pa)
                plt.axis(ax)

            himg2 = afwImage.ImageF(fpbb)
            dpk.heavy2.insert(himg2)

            if sumimg is None:
                sumimg = himg2.getArray().copy()
            else:
                sumimg += himg2.getArray()
                
            plt.subplot(R, C, i*C + 6)
            myimshow(himg2.getArray(), **ima)
            plt.title('portion+stray')
            ax = plt.axis()
            plt.plot(PX, PY, **pa)
            plt.axis(ax)

        plt.subplot(R, C, (2*C) + C)
        myimshow(sumimg, **ima)
        ax = plt.axis()
        plt.plot(X, Y, **pa)
        plt.axis(ax)
        plt.title('Sum of deblends')

        plt.suptitle(tt)
        
        plt.savefig('edge%i.png' % j)
