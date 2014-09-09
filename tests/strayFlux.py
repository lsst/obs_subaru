#!/usr/bin/env python

doPlot = False
if doPlot:
    import matplotlib
    matplotlib.use('Agg')
    import pylab as plt
    import os.path
    plotpat = os.path.join(os.path.dirname(__file__), 'stray%i.png')
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
pexLogging.Log(root, 'afw.Mask',
               pexLogging.Log.INFO)

def imExt(img):
    bbox = img.getBBox(afwImage.PARENT)
    return [bbox.getMinX(), bbox.getMaxX(),
            bbox.getMinY(), bbox.getMaxY()]

def doubleGaussianPsf(W, H, fwhm1, fwhm2, a2):
    return measAlg.DoubleGaussianPsf(W, H, fwhm1, fwhm2, a2)
    
def gaussianPsf(W, H, fwhm):
    return measAlg.DoubleGaussianPsf(W, H, fwhm)


class StrayFluxTestCase(unittest.TestCase):

    def test1(self):
        '''
        A simple example: three overlapping blobs (detected as 1
        footprint with three peaks).  We artificially omit one of the
        peaks, meaning that its flux is "stray".  Assert that the
        stray flux assigned to the other two peaks accounts for all
        the flux in the parent.
        '''
        H,W = 100,100

        fpbb = afwGeom.Box2I(afwGeom.Point2I(0,0),
                             afwGeom.Point2I(W-1,H-1))

        afwimg = afwImage.MaskedImageF(fpbb)
        imgbb = afwimg.getBBox(afwImage.PARENT)
        img = afwimg.getImage().getArray()

        var = afwimg.getVariance().getArray()
        var[:,:] = 1.
        
        blob_fwhm = 10.
        blob_psf = doubleGaussianPsf(99, 99, blob_fwhm, 3.*blob_fwhm, 0.03)

        fakepsf_fwhm = 3.
        fakepsf = gaussianPsf(11, 11, fakepsf_fwhm)
        
        blobimgs = []
        x = 75.
        XY = [(x,35.), (x,65.), (50.,50.)]
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
        thresh = afwDet.createThreshold(5., 'value', True)
        fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
        fps = fpSet.getFootprints()
        print 'found', len(fps), 'footprints'
        pks2 = []
        for fp in fps:
            print 'peaks:', len(fp.getPeaks())
            for pk in fp.getPeaks():
                print '  ', pk.getIx(), pk.getIy()
                pks2.append((pk.getIx(), pk.getIy()))

        # The first peak in this list is the one we want to omit.
        fp0 = fps[0]
        fakefp = afwDet.Footprint(fp0.getSpans(), fp0.getBBox())
        for pk in fp0.getPeaks()[1:]:
            fakefp.getPeaks().append(pk)
        
        ima = dict(interpolation='nearest', origin='lower', cmap='gray',
                   vmin=0, vmax=1e3)

        if doPlot:
            plt.figure(figsize=(12,6))

            plt.clf()
            plt.suptitle('strayFlux.py: test1 input')
            plt.subplot(2,2,1)
            plt.title('Image')
            plt.imshow(img, **ima)
            ax = plt.axis()
            plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
            plt.axis(ax)
            for i,(b,(x,y)) in enumerate(zip(blobimgs, XY)):
                plt.subplot(2,2, 2+i)
                plt.title('Blob %i' % i)
                plt.imshow(b, **ima)
                ax = plt.axis()
                plt.plot(x, y, 'r.')
                plt.axis(ax)
            plt.savefig(plotpat % 1)

        deb = deblend(fakefp, afwimg, fakepsf, fakepsf_fwhm, verbose=True)
        parent_img = afwImage.ImageF(fpbb)
        afwDet.copyWithinFootprintImage(fakefp, afwimg.getImage(), parent_img)

        if doPlot:
            def myimshow(*args, **kwargs):
                plt.imshow(*args, **kwargs)
                plt.xticks([]); plt.yticks([])
                plt.axis(imExt(afwimg))
    
            plt.clf()
            plt.suptitle('strayFlux.py: test1 results')
            #R,C = 3,5
            R,C = 3,4
            plt.subplot(R, C, (2*C) + 1)
            plt.title('Image')
            myimshow(img, **ima)
            ax = plt.axis()
            plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
            plt.axis(ax)
    
            plt.subplot(R, C, (2*C) + 2)
            plt.title('Parent footprint')
            myimshow(parent_img.getArray(), **ima)
            ax = plt.axis()
            plt.plot([pk.getIx() for pk in fakefp.getPeaks()],
                     [pk.getIy() for pk in fakefp.getPeaks()], 'r.')
            plt.axis(ax)
            
            sumimg = None
            for i,dpk in enumerate(deb.peaks):
                plt.subplot(R, C, i*C + 1)
                plt.title('ch%i symm' % i)
                symm = dpk.templateMaskedImage.getImage()
                myimshow(symm.getArray(), extent=imExt(symm), **ima)
    
                plt.subplot(R, C, i*C + 2)
                plt.title('ch%i portion' % i)
                port = dpk.fluxPortion.getImage()
                myimshow(port.getArray(), extent=imExt(port), **ima)
    
                himg = afwImage.ImageF(fpbb)
                heavy = dpk.getFluxPortion(strayFlux=False)
                heavy.insert(himg)
                
                # plt.subplot(R, C, i*C + 3)
                # plt.title('ch%i heavy' % i)
                # myimshow(himg.getArray(), **ima)
                # ax = plt.axis()
                # plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
                # plt.axis(ax)
    
                simg = afwImage.ImageF(fpbb)
                dpk.strayFlux.insert(simg)
                
                plt.subplot(R, C, i*C + 3)
                plt.title('ch%i stray' % i)
                myimshow(simg.getArray(), **ima)
                ax = plt.axis()
                plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
                plt.axis(ax)

                himg2 = afwImage.ImageF(fpbb)
                heavy = dpk.getFluxPortion(strayFlux=True)
                heavy.insert(himg2)
    
                if sumimg is None:
                    sumimg = himg2.getArray().copy()
                else:
                    sumimg += himg2.getArray()
                    
                plt.subplot(R, C, i*C + 4)
                myimshow(himg2.getArray(), **ima)
                plt.title('ch%i total' % i)
                ax = plt.axis()
                plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
                plt.axis(ax)
    
            plt.subplot(R, C, (2*C) + C)
            myimshow(sumimg, **ima)
            ax = plt.axis()
            plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
            plt.axis(ax)
            plt.title('Sum of deblends')
                
            plt.savefig(plotpat % 2)

        # Compute the sum-of-children image
        sumimg = None
        for i,dpk in enumerate(deb.peaks):
            himg2 = afwImage.ImageF(fpbb)
            dpk.getFluxPortion().insert(himg2)
            if sumimg is None:
                sumimg = himg2.getArray().copy()
            else:
                sumimg += himg2.getArray()
        
        # Sum of children ~= Original image inside footprint (parent_img)

        absdiff = np.max(np.abs(sumimg - parent_img.getArray()))
        print 'Max abs diff:', absdiff
        imgmax = parent_img.getArray().max()
        print 'Img max:', imgmax
        self.assertTrue(absdiff < imgmax * 1e-6)


    def test2(self):
        '''
        A 1-d example, to test the stray-flux assignment.
        '''
        H,W = 1,100

        fpbb = afwGeom.Box2I(afwGeom.Point2I(0,0),
                             afwGeom.Point2I(W-1,H-1))
        afwimg = afwImage.MaskedImageF(fpbb)
        imgbb = afwimg.getBBox(afwImage.PARENT)
        img = afwimg.getImage().getArray()
        
        var = afwimg.getVariance().getArray()
        var[:,:] = 1.

        y = 0
        img[y, 1:-1] = 10.

        img[0,  1] = 20.
        img[0, -2] = 20.
        
        fakepsf_fwhm = 1.
        fakepsf = gaussianPsf(1, 1, fakepsf_fwhm)

        # Run the detection code to get a ~ realistic footprint
        thresh = afwDet.createThreshold(5., 'value', True)
        fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
        fps = fpSet.getFootprints()
        self.assertEqual(len(fps), 1)
        fp = fps[0]

        # WORKAROUND: the detection alg produces ONE peak, at (1,0),
        # rather than two.
        self.assertEqual(len(fp.getPeaks()), 1)
        fp.getPeaks().append(afwDet.Peak(W-2, y))
        #print 'Added peak; peaks:', len(fp.getPeaks())
        #for pk in fp.getPeaks():
        #    print '  ', pk.getFx(), pk.getFy()
        
        deb = deblend(fp, afwimg, fakepsf, fakepsf_fwhm, verbose=True,
                      fitPsfs=False, )

        if doPlot:
            XX = np.arange(W+1).repeat(2)[1:-1]

            plt.clf()
            p1 = plt.plot(XX, img[y,:].repeat(2), 'g-', lw=3, alpha=0.3)

            for i,dpk in enumerate(deb.peaks):
                print dpk
                port = dpk.fluxPortion.getImage()
                bb = port.getBBox(afwImage.PARENT)
                YY = np.zeros(XX.shape)
                YY[bb.getMinX()*2 : (bb.getMaxX()+1)*2] = port.getArray()[0,:].repeat(2)
                p2 = plt.plot(XX, YY, 'r-')

                simg = afwImage.ImageF(fpbb)
                dpk.strayFlux.insert(simg)
                p3 = plt.plot(XX, simg.getArray()[y,:].repeat(2), 'b-')

            plt.legend((p1[0],p2[0],p3[0]),
                       ('Parent Flux', 'Child portion', 'Child stray flux'))
            plt.ylim(-2, 22)
            plt.savefig(plotpat % 3)

        strays = []
        for i,dpk in enumerate(deb.peaks):
            simg = afwImage.ImageF(fpbb)
            dpk.strayFlux.insert(simg)
            strays.append(simg.getArray())

        ssum = reduce(np.add, strays)

        starget = np.zeros(W)
        starget[2:-2] = 10.
        
        self.assertTrue(np.all(ssum == starget))

        X = np.arange(W)
        dx1 = X - 1.
        dx2 = X - (W-2)
        f1 = (1. / (1. + dx1**2))
        f2 = (1. / (1. + dx2**2))
        strayclip = 0.001
        fsum = f1 + f2
        f1[f1 < strayclip * fsum] = 0.
        f2[f2 < strayclip * fsum] = 0.

        s1 = f1 / (f1+f2) * 10.
        s2 = f2 / (f1+f2) * 10.

        s1[:2] = 0.
        s2[-2:] = 0.
        
        if doPlot:
            p4 = plt.plot(XX, s1.repeat(2), 'm-')
            plt.plot(XX, s2.repeat(2), 'm-')

            plt.legend((p1[0],p2[0],p3[0],p4[0]),
                       ('Parent Flux', 'Child portion', 'Child stray flux',
                        'Expected stray flux'))
            plt.ylim(-2, 22)
            plt.savefig(plotpat % 4)

        # test abs diff
        d = np.max(np.abs(s1 - strays[0]))
        self.assertTrue(d < 1e-6)
        d = np.max(np.abs(s2 - strays[1]))
        self.assertTrue(d < 1e-6)

        # test relative diff
        self.assertTrue(np.max(np.abs(s1 - strays[0]) / np.maximum(1e-3, s1)) < 1e-6)
        self.assertTrue(np.max(np.abs(s2 - strays[1]) / np.maximum(1e-3, s2)) < 1e-6)
        
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    utilsTests.init()
    suites = []
    suites += unittest.makeSuite(StrayFluxTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""
    utilsTests.run(suite(), exit)
 
if __name__ == "__main__":
    run(True)

