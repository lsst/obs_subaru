#!/usr/bin/env python
doPlot = False
if doPlot:
    import matplotlib
    matplotlib.use('Agg')
    import pylab as plt
    import os.path
    plotpat = os.path.join(os.path.dirname(__file__), 'edge%i.png')
    print 'Writing plots to', plotpat
else:
    print '"doPlot" not set -- not making plots.  To enable plots, edit', __file__

import unittest
import lsst.utils.tests         as utilsTests

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
pexLogging.Log(root, 'lsst.meas.deblender.symmetrizeFootprint',
               pexLogging.Log.INFO)
pexLogging.Log(root, 'lsst.meas.deblender.symmetricFootprint',
               pexLogging.Log.INFO)
pexLogging.Log(root, 'lsst.meas.deblender.getSignificantEdgePixels',
               pexLogging.Log.INFO)
pexLogging.Log(root, 'afw.Mask', pexLogging.Log.INFO)
               
def imExt(img):
    bbox = img.getBBox(afwImage.PARENT)
    return [bbox.getMinX(), bbox.getMaxX(),
            bbox.getMinY(), bbox.getMaxY()]

def doubleGaussianPsf(W, H, fwhm1, fwhm2, a2):
    return measAlg.DoubleGaussianPsf(W, H, fwhm1, fwhm2, a2)
    
def gaussianPsf(W, H, fwhm):
    return measAlg.DoubleGaussianPsf(W, H, fwhm)


class RampEdgeTestCase(unittest.TestCase):

    def test1(self):
        '''
        In this test, we create a test image containing two blobs, one 
        of which is truncated by the edge of the image.

        We run the detection code to get realistic peaks and
        footprints.
        
        We then test out the different edge treatments and assert that
        they do what they claim.  We also make plots, tests/edge*.png
        '''


        # Create fake image...
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
        fakepsf_fwhm = 5.
        S = int(np.ceil(fakepsf_fwhm * 2.)) * 2 + 1
        print 'S', S
        fakepsf = gaussianPsf(S, S, fakepsf_fwhm)
    
        # Create and save blob images, and add to image to deblend.
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
        thresh = afwDet.createThreshold(10., 'value', True)
        fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
        fps = fpSet.getFootprints()
        print 'found', len(fps), 'footprints'
    
        # set EDGE bit on edge pixels.
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
            #print 'Deblending...'
            deb = deblend(fp, afwimg, fakepsf, fakepsf_fwhm, verbose=True,
                          **kwa)
            #print 'Result:', deb
            #print len(deb.peaks), 'deblended peaks'
    
            parent_img = afwImage.ImageF(fpbb)
            afwDet.copyWithinFootprintImage(fp, afwimg.getImage(), parent_img)
    
            X = [x for x,y in XY]
            Y = [y for x,y in XY]
            PX = [pk.getIx() for pk in fp.getPeaks()]
            PY = [pk.getIy() for pk in fp.getPeaks()]

            # Grab 1-d slices to make assertion about.
            symms = []
            monos = []
            symm1ds = []
            mono1ds = []
            yslice = H/2
            parent1d = img[yslice, :]
            for i,dpk in enumerate(deb.peaks):
                symm = dpk.origTemplate
                symms.append(symm)

                bbox = symm.getBBox(afwImage.PARENT)
                x0,y0 = bbox.getMinX(), bbox.getMinY()
                im = symm.getArray()
                h,w = im.shape
                oned = np.zeros(W)
                oned[x0: x0+w] = im[yslice-y0, :]
                symm1ds.append(oned)
    
                mono = afwImage.ImageF(fpbb)
                afwDet.copyWithinFootprintImage(dpk.templateFootprint,
                                                dpk.templateMaskedImage.getImage(), mono)
                monos.append(mono)

                im = mono.getArray()
                bbox = mono.getBBox(afwImage.PARENT)
                x0,y0 = bbox.getMinX(), bbox.getMinY()
                h,w = im.shape
                oned = np.zeros(W)
                oned[x0: x0+w] = im[yslice-y0, :]
                mono1ds.append(oned)


            for i,(symm,mono) in enumerate(zip(symm1ds, mono1ds)):
                # for the first two cases, the basic symmetric
                # template for the second source drops to zero at <
                # ~75 where the symmetric part is outside the
                # footprint.
                if i == 1 and j in [0,1]:
                    self.assertTrue(np.all(symm[:74] == 0))
                if i == 1 and j == 2:
                    # For the third case, the 'symm' template gets
                    # "patched" with the parent's value
                    self.assertTrue(np.all((symm == parent1d)[:74]))
    
                if i == 1 and j == 0:
                    # No edge handling: mono template == 0
                    self.assertTrue(np.all(mono[:74] == 0))
                if i == 1 and j == 1:
                    # ramp by psf: zero up to ~65, ramps up
                    self.assertTrue(np.all(mono[:64] == 0))
                    self.assertTrue(np.any(mono[65:74] > 0))
                    self.assertTrue(np.all(np.diff(mono)[60:80] >= 0.))
                if i == 1 and j == 2:
                    # no edge clipping: profile is monotonic and positive.
                    self.assertTrue(np.all(np.diff(mono)[:85] >= 0.))
                    self.assertTrue(np.all(mono[:85] > 0.))


            if not doPlot:
                continue

            plt.clf()
            p1 = plt.plot(parent1d, 'b-', lw=3, alpha=0.5)
            for i,(symm,mono) in enumerate(zip(symm1ds, mono1ds)):
                p2 = plt.plot(symm, 'r-', lw=2, alpha=0.7)
                p3 = plt.plot(mono, 'g-')
            plt.legend((p1[0],p2[0],p3[0]), ('Parent','Symm template', 'Mono template'),
                       loc='upper left')
            plt.title('1-d slice: %s' % tt)
            fn = plotpat % (2*j+0)
            plt.savefig(fn)
            print 'Wrote', fn
            
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
                t = dpk.origTemplate
                myimshow(t.getArray(), extent=imExt(t), **ima)
                ax = plt.axis()
                plt.plot(PX[i], PY[i], **pa)
                plt.axis(ax)
                plt.title('symm')
    
                # monotonic template
                mimg = afwImage.ImageF(fpbb)
                afwDet.copyWithinFootprintImage(dpk.templateFootprint,
                                                dpk.templateMaskedImage.getImage(), mimg)
    
                plt.subplot(R, C, i*C + 3)
                myimshow(mimg.getArray(), extent=imExt(mimg), **ima)
                ax = plt.axis()
                plt.plot(PX[i], PY[i], **pa)
                plt.axis(ax)
                plt.title('monotonic')
    
                plt.subplot(R, C, i*C + 4)
                port = dpk.fluxPortion.getImage()
                myimshow(port.getArray(), extent=imExt(port), **ima)
                plt.title('portion')
                ax = plt.axis()
                plt.plot(PX[i], PY[i], **pa)
                plt.axis(ax)
    
                if dpk.strayFlux is not None:
                    simg = afwImage.ImageF(fpbb)
                    dpk.strayFlux.insert(simg)
                
                    plt.subplot(R, C, i*C + 5)
                    myimshow(simg.getArray(), **ima)
                    plt.title('stray')
                    ax = plt.axis()
                    plt.plot(PX, PY, **pa)
                    plt.axis(ax)
    
                himg2 = afwImage.ImageF(fpbb)
                portion = dpk.getFluxPortion()
                portion.insert(himg2)
    
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
            fn = plotpat % (2*j + 1)
            plt.savefig(fn)
            print 'Wrote', fn



#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    utilsTests.init()
    suites = []
    suites += unittest.makeSuite(RampEdgeTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""
    utilsTests.run(suite(), exit)
 
if __name__ == "__main__":
    run(True)

