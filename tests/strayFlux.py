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
        # XY = [(x,40.), (x,60.), (50.,50.)]
        XY = [(x,35.), (x,65.), (50.,50.)]
        # XY = [(x,30.), (x,70.), (40.,50.)]
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


        # middle = convolvedImage = afwimg.Factory(afwimg)
        # fpSetsPos =  thresholdImage(middle, "positive")
        thresh = afwDet.createThreshold(5., 'stdev', True)
        fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
        print 'got', fpSet
        fps = fpSet.getFootprints()
        print 'found', len(fps), 'footprints'
        pks2 = []
        for fp in fps:
            print 'footprint', fp
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
                   vmin=0, vmax=1e6)

        plt.figure(figsize=(12,6))

        plt.clf()
        plt.subplot(2,2,1)
        plt.imshow(img, **ima)
        ax = plt.axis()
        plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')

        #for (x,y),c in zip(pks2, 'mgb'):
        #plt.plot(fp.

        plt.axis(ax)
        for i,(b,(x,y)) in enumerate(zip(blobimgs, XY)):
            plt.subplot(2,2, 2+i)
            plt.imshow(b, **ima)
            ax = plt.axis()
            plt.plot(x, y, 'r.')
            plt.axis(ax)
        plt.savefig('stray1.png')

        print 'Deblending...'
        #deb = deblend(fp, afwimg, fakepsf, fakepsf_fwhm, verbose=True)
        deb = deblend(fakefp, afwimg, fakepsf, fakepsf_fwhm, verbose=True)

        print 'Result:', deb

        print len(deb.peaks), 'deblended peaks'

        def imExt(img):
            bbox = img.getBBox(afwImage.PARENT)
            return [bbox.getMinX(), bbox.getMaxX(),
                    bbox.getMinY(), bbox.getMaxY()]

        def myimshow(*args, **kwargs):
            plt.imshow(*args, **kwargs)
            plt.xticks([]); plt.yticks([])
            plt.axis(imExt(afwimg))

        plt.clf()

        R,C = 3,5
        plt.subplot(R, C, (2*C) + 1)
        myimshow(img, **ima)
        ax = plt.axis()
        plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
        plt.axis(ax)
        plt.title('Image')

        hfp = afwDet.makeHeavyFootprint(fakefp, afwimg)
        himg = afwImage.ImageF(fpbb)
        hfp.insert(himg)
        
        plt.subplot(R, C, (2*C) + 2)
        myimshow(himg.getArray(), **ima)
        ax = plt.axis()
        plt.plot([pk.getIx() for pk in fakefp.getPeaks()],
                 [pk.getIy() for pk in fakefp.getPeaks()], 'r.')
        plt.axis(ax)
        plt.title('Footprint')
        
        sumimg = None
            
        for i,dpk in enumerate(deb.peaks):
            # dpk.symm, dpk.median
            # dpk.timg, dpk.portion
            # dpk.heavy
            print dpk
            print dir(dpk)

            plt.subplot(R, C, i*C + 1)
            myimshow(dpk.symm.getArray(), extent=imExt(dpk.symm), **ima)
            plt.title('symm')

            plt.subplot(R, C, i*C + 2)
            myimshow(dpk.portion.getArray(), extent=imExt(dpk.portion), **ima)
            plt.title('portion')

            himg = afwImage.ImageF(fpbb)
            dpk.heavy.insert(himg)
            
            plt.subplot(R, C, i*C + 3)
            myimshow(himg.getArray(), **ima)
            plt.title('heavy')
            ax = plt.axis()
            plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
            plt.axis(ax)

            simg = afwImage.ImageF(fpbb)
            dpk.stray.insert(simg)
            
            plt.subplot(R, C, i*C + 4)
            myimshow(simg.getArray(), **ima)
            plt.title('stray')
            ax = plt.axis()
            plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
            plt.axis(ax)

            himg2 = afwImage.ImageF(fpbb)
            dpk.heavy2.insert(himg2)

            if sumimg is None:
                sumimg = himg2.getArray().copy()
            else:
                sumimg += himg2.getArray()
                
            plt.subplot(R, C, i*C + 5)
            myimshow(himg2.getArray(), **ima)
            plt.title('heavy+stray')
            ax = plt.axis()
            plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
            plt.axis(ax)

        plt.subplot(R, C, (2*C) + C)
        myimshow(sumimg, **ima)
        ax = plt.axis()
        plt.plot([x for x,y in XY], [y for x,y in XY], 'r.')
        plt.axis(ax)
        plt.title('Sum of deblends')
            
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

