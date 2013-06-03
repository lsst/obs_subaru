#!/usr/bin/env python

try:
    import matplotlib
    matplotlib.use('Agg')
    import pylab as plt
    doPlot = True
except:
    doPlot = False

import unittest
import lsst.utils.tests         as utilsTests

import numpy as np

import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.pex.logging as pexLogging
from lsst.meas.deblender.baseline import *

root = pexLogging.Log.getDefaultLog()
root.setThreshold(pexLogging.Log.DEBUG)

class StrayFluxTestCase(unittest.TestCase):
    def test1(self):

        H,W = 100,100

        fpbb = afwGeom.Box2I(afwGeom.Point2I(0,0),
                             afwGeom.Point2I(W-1,H-1))
        fp = afwDet.Footprint(fpbb)

        afwimg = afwImage.MaskedImageF(fpbb)
        imgbb = afwimg.getBBox(afwImage.PARENT)
        print 'Img bb:', imgbb.getMinX(), imgbb.getMaxX(), imgbb.getMinY(), imgbb.getMaxY()
        img = afwimg.getImage().getArray()

        var = afwimg.getVariance().getArray()
        var[:,:] = 1.
        
        blob_fwhm = 10.
        blob_psf = afwDet.createPsf('DoubleGaussian', 99, 99, blob_fwhm)

        fakepsf_fwhm = 3.
        fakepsf = afwDet.createPsf('DoubleGaussian', 11, 11, fakepsf_fwhm)
        
        pks = []
        blobimgs = []
        x = 75.
        XY = [(x,40.), (x,60.), (40.,50.)]
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

            pk = afwDet.Peak(x,y)
            print 'Peak:', pk.getIx(), pk.getIy()
            pks.append(pk)

        # We omit the last peak, leaving it as "stray flux".
        pklist = fp.getPeaks()
        for pk in pks[:-1]:
            pklist.append(pk)
            
        ima = dict(interpolation='nearest', origin='lower', cmap='gray')
        plt.clf()
        plt.subplot(2,2,1)
        plt.imshow(img, **ima)
        ax = plt.axis()
        plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
        plt.axis(ax)
        for i,(b,(x,y)) in enumerate(zip(blobimgs, XY)):
            plt.subplot(2,2, 2+i)
            plt.imshow(b, **ima)
            ax = plt.axis()
            plt.plot(x, y, 'r.')
            plt.axis(ax)
        plt.savefig('stray1.png')

        print 'Deblending...'
        deb = deblend(fp, afwimg, fakepsf, fakepsf_fwhm,
                      verbose=True)
        print 'Result:', deb

        print len(deb.peaks), 'deblended peaks'

        plt.clf()
        for i,dpk in enumerate(deb.peaks):
            # dpk.symm, dpk.median
            # dpk.timg, dpk.portion
            # dpk.heavy
            print dpk
            print dir(dpk)
            plt.subplot(2, 3, i*3 + 1)
            plt.imshow(dpk.symm.getArray(), **ima)
            plt.title('symm')
            plt.subplot(2, 3, i*3 + 2)
            plt.imshow(dpk.portion.getArray(), **ima)
            plt.title('portion')

            himg = afwImage.ImageF(fpbb)
            dpk.heavy.insert(himg)
            
            plt.subplot(2, 3, i*3 + 3)
            plt.imshow(himg.getArray(), **ima)
            plt.title('heavy')
            ax = plt.axis()
            plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
            plt.axis(ax)
        plt.savefig('stray2.png')



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

