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

import lsst.meas.algorithms as measAlg

root = pexLogging.Log.getDefaultLog()
root.setThreshold(pexLogging.Log.DEBUG)

# Quiet some of the more chatty loggers
log = pexLogging.Log(root, 'lsst.meas.deblender.symmetrizeFootprint',
                     pexLogging.Log.INFO)

def imExt(img):
    bbox = img.getBBox(afwImage.PARENT)
    return [bbox.getMinX(), bbox.getMaxX(),
            bbox.getMinY(), bbox.getMaxY()]

def doubleGaussianPsf(W, H, fwhm1, fwhm2, a2):
    #return afwDet.createPsf('DoubleGaussian', W, H, fwhm1, fwhm2, a2)
    #return afwDet.DoubleGaussianPsf(W, H, fwhm1, fwhm2, a2)
    return measAlg.DoubleGaussianPsf(W, H, fwhm1, fwhm2, a2)
    
def gaussianPsf(W, H, fwhm):
    #return afwDet.createPsf('DoubleGaussian', W, H, fwhm)
    #return afwDet.DoubleGaussianPsf(W, H, fwhm1)
    return measAlg.DoubleGaussianPsf(W, H, fwhm)


class StrayFluxTestCase(unittest.TestCase):

    def testClippedAtEdge(self):
        plots = True and doPlot
        
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
        self.assertEqual(len(fps), 1)
        fp = fps[0]
        self.assertEqual(len(fp.getPeaks()), 2)
        
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

            if not plots:
                continue

            hfp = afwDet.makeHeavyFootprint(fp, afwimg)
            parent_img = afwImage.ImageF(fpbb)
            hfp.insert(parent_img)

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

            #plt.subplot(R, C, (2*C) + 3)
            #myimshow(img, **ima)
            #plt.colorbar()
            #plt.title('Image')
            
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
                mimg = afwImage.MaskedImageF(fpbb)
                hm = afwDet.makeHeavyFootprint(dpk.tfoot, dpk.tmimg)
                hm.insert(mimg)

                plt.subplot(R, C, i*C + 3)
                myimshow(mimg.getImage().getArray(), extent=imExt(mimg), **ima)
                ax = plt.axis()
                plt.plot(PX[i], PY[i], **pa)
                plt.axis(ax)
                plt.title('monotonic')

                plt.subplot(R, C, i*C + 4)
                myimshow(dpk.portion.getArray(), extent=imExt(dpk.portion), **ima)
                plt.title('portion')
                # himg = afwImage.ImageF(fpbb)
                # dpk.heavy1.insert(himg)
                # plt.subplot(R, C, i*C + 3)
                # myimshow(himg.getArray(), **ima)
                # plt.title('heavy')
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


    def tst1(self):
        '''
        A simple example: three overlapping blobs (detected as 1
        footprint with three peaks).  We artificially omit one of the
        peaks, meaning that its flux is "stray".  Assert that the
        stray flux assigned to the other two peaks accounts for all
        the flux in the parent.
        '''
        plots = True and doPlot
        
        H,W = 100,100

        fpbb = afwGeom.Box2I(afwGeom.Point2I(0,0),
                             afwGeom.Point2I(W-1,H-1))

        afwimg = afwImage.MaskedImageF(fpbb)
        imgbb = afwimg.getBBox(afwImage.PARENT)
        img = afwimg.getImage().getArray()

        var = afwimg.getVariance().getArray()
        var[:,:] = 1.
        
        blob_fwhm = 10.
        #blob_psf = afwDet.createPsf('DoubleGaussian', 99, 99, blob_fwhm)
        blob_psf = doubleGaussianPsf(99, 99, blob_fwhm, 3.*blob_fwhm, 0.03)

        fakepsf_fwhm = 3.
        fakepsf = gaussianPsf(11, 11, fakepsf_fwhm)
        
        blobimgs = []
        x = 75.
        # XY = [(x,40.), (x,60.), (50.,50.)]
        XY = [(x,35.), (x,65.), (50.,50.)]
        # XY = [(x,30.), (x,70.), (40.,50.)]
        flux = 1e6
        #pks = []
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

            # pk = afwDet.Peak(x,y)
            # print 'Peak:', pk.getIx(), pk.getIy()
            # pks.append(pk)

        # We omit the last peak, leaving it as "stray flux".
        # pklist = fp.getPeaks()
        # for pk in pks[:-1]:
        #     pklist.append(pk)

        # Run the detection code to get a ~ realistic footprint
        thresh = afwDet.createThreshold(5., 'value', True)
        fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
        fps = fpSet.getFootprints()
        print 'found', len(fps), 'footprints'
        pks2 = []
        for fp in fps:
            #print 'footprint', fp
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

        if plots:
            plt.figure(figsize=(12,6))

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
        deb = deblend(fakefp, afwimg, fakepsf, fakepsf_fwhm, verbose=True)
        print 'Result:', deb
        print len(deb.peaks), 'deblended peaks'

        hfp = afwDet.makeHeavyFootprint(fakefp, afwimg)
        parent_img = afwImage.ImageF(fpbb)
        hfp.insert(parent_img)

        if plots:
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
    
            plt.subplot(R, C, (2*C) + 2)
            myimshow(parent_img.getArray(), **ima)
            ax = plt.axis()
            plt.plot([pk.getIx() for pk in fakefp.getPeaks()],
                     [pk.getIy() for pk in fakefp.getPeaks()], 'r.')
            plt.axis(ax)
            plt.title('Footprint')
            
            sumimg = None
            for i,dpk in enumerate(deb.peaks):
                plt.subplot(R, C, i*C + 1)
                myimshow(dpk.symm.getArray(), extent=imExt(dpk.symm), **ima)
                plt.title('symm')
    
                plt.subplot(R, C, i*C + 2)
                myimshow(dpk.portion.getArray(), extent=imExt(dpk.portion), **ima)
                plt.title('portion')
    
                himg = afwImage.ImageF(fpbb)
                dpk.heavy1.insert(himg)
                
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

        # Compute the sum-of-children image
        sumimg = None
        for i,dpk in enumerate(deb.peaks):
            himg2 = afwImage.ImageF(fpbb)
            dpk.heavy2.insert(himg2)
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


    def tst2(self):
        '''
        A 1-d example, to test the stray-flux assignment.
        '''
        #H,W = 3,100
        H,W = 1,100

        fpbb = afwGeom.Box2I(afwGeom.Point2I(0,0),
                             afwGeom.Point2I(W-1,H-1))
        afwimg = afwImage.MaskedImageF(fpbb)
        imgbb = afwimg.getBBox(afwImage.PARENT)
        img = afwimg.getImage().getArray()
        
        var = afwimg.getVariance().getArray()
        var[:,:] = 1.

        #y = 1
        y = 0
        
        #img[y, 2:-2] = 10.
        img[y, 1:-1] = 10.

        img[0,  1] = 20.
        img[0, -2] = 20.
        
        #print 'Image:', afwimg.getImage().getArray()

        fakepsf_fwhm = 1.
        fakepsf = gaussianPsf(1, 1, fakepsf_fwhm)
        #fakepsf = afwDet.createPsf('DoubleGaussian', 1, 3, 1.)
        #fakepsf = afwDet.createPsf('DoubleGaussian', 3, 3, 1.)

        # Run the detection code to get a ~ realistic footprint
        thresh = afwDet.createThreshold(5., 'value', True)
        fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
        fps = fpSet.getFootprints()
        print 'found', len(fps), 'footprints'
        self.assertEqual(len(fps), 1)
        fp = fps[0]
        print 'peaks:', len(fp.getPeaks())
        # WORKAROUND: the detection alg produces ONE peak, at (1,0),
        # rather than two.
        self.assertEqual(len(fp.getPeaks()), 1)
        fp.getPeaks().append(afwDet.Peak(W-2, y))
        print 'Added peak; peaks:', len(fp.getPeaks())

        for pk in fp.getPeaks():
            print '  ', pk.getFx(), pk.getFy()
        
        print 'Deblending...'
        deb = deblend(fp, afwimg, fakepsf, fakepsf_fwhm, verbose=True,
                      fit_psfs=False, )
        print 'Result:', deb
        print len(deb.peaks), 'deblended peaks'

        plots = True and doPlot
        if plots:
            XX = np.arange(W+1).repeat(2)[1:-1]
            plt.clf()
            plt.plot(XX, img[y,:].repeat(2), 'g-', lw=3, alpha=0.3)

            for i,dpk in enumerate(deb.peaks):
                print dpk
                bb = dpk.portion.getBBox(afwImage.PARENT)
                YY = np.zeros(XX.shape)
                YY[bb.getMinX()*2 : (bb.getMaxX()+1)*2] = dpk.portion.getArray()[0,:].repeat(2)
                plt.plot(XX, YY, 'r-')

                simg = afwImage.ImageF(fpbb)
                dpk.stray.insert(simg)
                plt.plot(XX, simg.getArray()[y,:].repeat(2), 'b-')

            plt.savefig('stray3.png')

        strays = []
        for i,dpk in enumerate(deb.peaks):
            simg = afwImage.ImageF(fpbb)
            dpk.stray.insert(simg)
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
        
        if plots:
            plt.plot(XX, s1.repeat(2), 'm-')
            plt.plot(XX, s2.repeat(2), 'm-')
            plt.savefig('stray4.png')

        # print strays[0]
        # print s1
        # print strays[1]
        # print s2

        d = np.max(np.abs(s1 - strays[0]))
        print 'diff', d
        self.assertTrue(d < 1e-6)
        d = np.max(np.abs(s2 - strays[1]))
        print 'diff', d
        self.assertTrue(d < 1e-6)
        
        
        
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

