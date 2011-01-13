from math import sqrt
from lsst.meas.deblender import deblender

#from lsst.meas.utils import sourceDetection
#import lsst.meas.utils.sourceDetection as sourceDetection

import lsst.meas.algorithms as measAlg
import lsst.afw.math as afwMath
import lsst.afw.geom as afwGeom
import lsst.afw.detection as afwDet
import lsst.afw.image as afwImg

from numpy.random import seed,poisson
from numpy import meshgrid, array
import numpy as np

#   eups declare --current meas_utils svn -r ~/lsst/meas-utils/

import unittest

def lsstimagetonumpy(img):
    x0,y0 = img.getX0(), img.getY0()
    W,H = img.getWidth(), img.getHeight()
    print 'LSST image: x0,y0', (x0,y0), ', size %i x %i' % (W,H)
    a = np.zeros((H+y0,W+x0))
    for i in range(H):
        for j in range(W):
            a[i+y0, j+x0] = img.get(j,i)
    return a

def renderImage(stars, sky, W, H, truepsfsigma):
    # "DoubleGaussian", w, h, sigma1, sigma2, b (rel.ampl. of gaussian #2)
    truepsf = afwDet.createPsf("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0)
    # Sample sky
    skypix = poisson(sky, (H,W))

    # Create images of each star
    starpix = []
    for (x,y,flux) in stars:
        psfimg = truepsf.computeImage(afwGeom.makePointD(x, y))
        s = np.zeros((H,W))
        si = lsstimagetonumpy(psfimg)
        (sh,sw) = si.shape
        s[:sh,:sw] = si
        # normalize then scale PSFs
        s /= s.sum()
        s *= flux
        starpix.append(s)

    maskedImage = afwImg.MaskedImageF(W, H)
    image = maskedImage.getImage()
    for y in range(H):
        for x in range(W):
            image.set(x, y, skypix[y,x] + sum([s[y,x] for s in starpix]))

    # Give it ~correct variance image
    varimg = maskedImage.getVariance()
    for y in range(H):
        for x in range(W):
            varimg.set(x, y, sky)

    return maskedImage, truepsf, skypix, starpix

def doDetection(maskedImage, psf, psfsigma, bginterp=afwMath.Interpolate.AKIMA_SPLINE, bgn=5):
    image = maskedImage.getImage()

    bctrl = afwMath.BackgroundControl(bginterp)
    bctrl.setNxSample(bgn)
    bctrl.setNySample(bgn)
    bg = afwMath.makeBackground(image, bctrl)

    bgimg = bg.getImageF()
    image.scaledMinus(1, bgimg)

    minPixels = 5
    # n sigma
    thresholdValue = 3.
    thresholdType = afwDet.Threshold.STDEV
    thresholdPolarity = "positive"
    nGrow = int(psfsigma)

    # Smooth the Image
    convolvedImage = maskedImage.Factory(maskedImage.getDimensions())
    convolvedImage.setXY0(maskedImage.getXY0())

    afwMath.convolve(convolvedImage,
                     maskedImage,
                     psf.getKernel())

    # Only search psf-smoothed part of frame
    llc = afwImg.PointI(psf.getKernel().getWidth()/2, psf.getKernel().getHeight()/2)
    urc = afwImg.PointI(convolvedImage.getWidth() - 1,
                        convolvedImage.getHeight() - 1)
    urc -= llc
    bbox = afwImg.BBox(llc, urc)    
    middle = convolvedImage.Factory(convolvedImage, bbox)

    # do positive detections        
    threshold = afwDet.createThreshold(thresholdValue)
    dsPositive = afwDet.makeFootprintSet(
        middle, threshold, "DETECTED", minPixels)

    # set detection region to be the entire region
    region = afwImg.BBox(
        afwImg.PointI(maskedImage.getX0(), maskedImage.getY0()),
        maskedImage.getWidth(), maskedImage.getHeight()
        )
    dsPositive.setRegion(region)

    # We want to grow the detections into the edge by at least one pixel so 
    # that it sees the EDGE bit
    if nGrow > 0:
        dsPositive = afwDet.FootprintSetF(dsPositive, nGrow, False)
    dsPositive.setMask(maskedImage.getMask(), "DETECTED")
    foots = dsPositive.getFootprints()

    return bgimg, convolvedImage, dsPositive

class TestDeblender(unittest.TestCase):

    def setUp(self):
        pass

    def tstTwoStars(self):
        # seed numpy random number generator.
        seed(42)
        # Read/create a test image....
        W = H = 64
        sky = 100
        # (x, y, flux)
        stars = [(28.4, 32.8, 2000),
                 (38.1, 32.5, 4000)]
        # Single-Gaussian PSF
        truepsfsigma = 2.
        # "DoubleGaussian", w, h, sigma1, sigma2, b (rel.ampl. of gaussian #2)
        truepsf = afwDet.createPsf("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0)

        # Sample sky
        skypix = poisson(sky, (H,W))

        # Create images of each star
        starpix = []
        for (x,y,flux) in stars:
            psfimg = truepsf.computeImage(afwGeom.makePointD(x, y))
            s = np.zeros((H,W))
            si = lsstimagetonumpy(psfimg)
            (sh,sw) = si.shape
            s[:sh,:sw] = si
            # normalize then scale PSFs
            s /= s.sum()
            s *= flux
            starpix.append(s)

        maskedImage = afwImg.MaskedImageF(W, H)
        image = maskedImage.getImage()
        for y in range(H):
            for x in range(W):
                image.set(x, y, skypix[y,x] + sum([s[y,x] for s in starpix]))

        image.writeFits('test1.fits')

        if True:
            import matplotlib
            matplotlib.use('Agg')
            import pylab as plt
            plt.clf()
            im = np.zeros_like(starpix[0])
            for s in starpix:
                im += s
            im += skypix
            plt.imshow(im, interpolation='nearest', origin='lower')
            ax = plt.axis()
            for x,y,flux in stars:
                plt.plot([x],[y], 'r+', ms=10)
            plt.axis(ax)
            plt.savefig('image.png')

        # Give it ~correct variance image
        varimg = maskedImage.getVariance()
        for y in range(H):
            for x in range(W):
                varimg.set(x, y, sky)

        bctrl = afwMath.BackgroundControl(afwMath.Interpolate.AKIMA_SPLINE)
        bctrl.setNxSample(5)
        bctrl.setNySample(5)
        bg = afwMath.makeBackground(image, bctrl)

        bgimg = bg.getImageF()
        image.scaledMinus(1, bgimg)
        image.writeFits('bgsub1.fits')

        psf = truepsf
        psfsigma = truepsfsigma

        minPixels = 5
        # n sigma
        thresholdValue = 10
        thresholdType = afwDet.Threshold.VARIANCE
        thresholdPolarity = "positive"
        nGrow = int(psfsigma)

        # Smooth the Image
        convolvedImage = maskedImage.Factory(maskedImage.getDimensions())
        convolvedImage.setXY0(maskedImage.getXY0())

        afwMath.convolve(convolvedImage,
                         maskedImage,
                         psf.getKernel())
        
        convolvedImage.getImage().writeFits('conv1.fits')

        # Only search psf-smoothed part of frame
        llc = afwImg.PointI(
            psf.getKernel().getWidth()/2, 
            psf.getKernel().getHeight()/2
        )
        urc = afwImg.PointI(
            convolvedImage.getWidth() - 1,
            convolvedImage.getHeight() - 1
        )
        urc -= llc
        bbox = afwImg.BBox(llc, urc)    
        middle = convolvedImage.Factory(convolvedImage, bbox)

        # do positive detections        
        threshold = afwDet.createThreshold(thresholdValue)
        dsPositive = afwDet.makeFootprintSet(
            middle, threshold, "DETECTED", minPixels)

        # set detection region to be the entire region
        region = afwImg.BBox(
            afwImg.PointI(maskedImage.getX0(), maskedImage.getY0()),
            maskedImage.getWidth(), maskedImage.getHeight()
            )
        dsPositive.setRegion(region)

        # We want to grow the detections into the edge by at least one pixel so 
        # that it sees the EDGE bit
        if nGrow > 0:
            dsPositive = afwDet.FootprintSetF(dsPositive, nGrow, False)
            dsPositive.setMask(maskedImage.getMask(), "DETECTED")
        foots = dsPositive.getFootprints()

        # DEBUG
        footImg = afwImg.ImageU(W, H, 0)
        for i,f in enumerate(foots):
            print 'footprint:', f.getNpix(), 'pix, bb is', f.getBBox()
            print 'peaks:', len(f.getPeaks())
            f.insertIntoImage(footImg, 65535-i)
        footImg.writeFits('foot1.fits')

        # one blob.
        self.assertEqual(len(foots), 1)

        # The LSST Footprint/Peak stuff isn't done, so fake it.
        if False:
            print '%i footprints' % len(foots)
            for fi,f in enumerate(foots):
                print 'footprint', fi, 'has', len(f.getPeaks()), 'peaks'
                for i,p in enumerate(f.getPeaks()):
                    print 'footprint', fi, 'peak', i, 'is', p
            allpeaks = [f.getPeaks() for f in foots]

        else:
            ## HACK -- plug in exact Peak locations.
            peaks = [afwDet.Peak(x,y) for (x,y,flux) in stars]
            allpeaks = [peaks]

        # DEBUG
        maskedImage.getImage().writeFits('mimage.fits')

        objs = deblender.deblend(foots, allpeaks, maskedImage, psf)

        self.assertNotEqual(objs, None)

        self.assertEqual(2, len(objs))

        print 'Got deblended objects:', objs
        for i,o in enumerate(objs):
            print o
            img = afwImg.ImageF(W, H)
            o.addToImage(img, 0)
            img.writeFits('child-%02i.fits' % i)

            img = o.images[0]
            img.writeFits('child-%02i-b.fits' % i)

            simg = afwImg.ImageF(W, H)
            for y in range(H):
                for x in range(W):
                    simg.set(x, y, starpix[i][y, x])
            simg.writeFits('star-%02i.fits' % i)

        if True:
            import pylab as plt
            plt.clf()

            N = len(objs)

            children = []
            for o in objs:
                img = afwImg.ImageF(W, H)
                o.addToImage(img, 0)
                a = lsstimagetonumpy(img)
                children.append(a)

            mn,mx = min([s.min() for s in starpix]), max([s.max() for s in starpix])

            C = 4
            ploti = 1
            diff = 5 * sqrt(sky)

            for i,(s,c) in enumerate(zip(starpix, children)):

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s+skypix-sky, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(c, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s - c, interpolation='nearest', origin='lower',
                           vmin=-diff, vmax=diff)
                plt.colorbar()

            plt.savefig('child-stars.png')

    def testStarGal(self):
        if True:
            import matplotlib
            matplotlib.use('Agg')

        import pyfits
        sources = [(50,50,10), (68,50,10)]
        P = pyfits.open('tests/test1.fits')
        # peaks have height ~ 10 counts.
        imgs = [P[0].data, P[1].data]
        H,W = imgs[0].shape
        scale = 1000.
        imgs = [im * scale for im in imgs]
        print 'read test image: shape', W,H
        # Single-Gaussian PSF
        truepsfsigma = 2.
        # variance
        var = (0.5 * scale) ** 2

        sky = 0
        skypix = 0
        starpix = imgs

        maskedImage = afwImg.MaskedImageF(W, H)

        sumimg = np.dstack(imgs).sum(axis=2)
        image = maskedImage.getImage()
        for y in range(H):
            for x in range(W):
                image.set(x, y, sumimg[y,x])
        image.writeFits('test5.fits')

        # Give it ~correct variance image
        varimg = maskedImage.getVariance()
        for y in range(H):
            for x in range(W):
                varimg.set(x, y, var)

        # "DoubleGaussian", w, h, sigma1, sigma2, b (rel.ampl. of gaussian #2)
        truepsf = afwDet.createPsf("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0)

        psf = truepsf
        psfsigma = truepsfsigma

        (bgimg, convolvedImage, detections) = doDetection(maskedImage, psf, psfsigma, bginterp=afwMath.Interpolate.CONSTANT, bgn=1)
        foots = detections.getFootprints()
        print 'Got footprints:', len(foots)

        if True:
            import pylab as plt

            im1 = np.zeros_like(starpix[0])
            for s in starpix:
                im1 += s
            im1 += skypix

            im2 = lsstimagetonumpy(convolvedImage.getImage())

            for img,fn in [(im1, 'image5a.png'), (im2, 'image5b.png')]:
                plt.clf()
                plt.imshow(img, interpolation='nearest', origin='lower')
                ax = plt.axis()
                for x,y,flux in sources:
                    plt.plot([x],[y], 'r+', ms=10)
                for f in foots:
                    bb = f.getBBox()
                    x0,y0,x1,y1 = bb.getX0(), bb.getY0(), bb.getX1(), bb.getY1()
                    plt.plot([x0,x1,x1,x0,x0], [y0,y0,y1,y1,y0], 'r-', lw=2)
                plt.axis(ax)
                plt.savefig(fn)


        if True:
            ## HACK -- plug in exact Peak locations.
            peaks = [afwDet.Peak(x,y) for (x,y,flux) in sources]
            allpeaks = [peaks]

        objs = deblender.deblend(foots, allpeaks, maskedImage, psf)

        self.assertNotEqual(objs, None)

        print 'Got', len(objs), 'deblended objects:', objs
        for i,o in enumerate(objs):
            print o
            img = afwImg.ImageF(W, H)
            o.addToImage(img, 0)
            img.writeFits('child5-%02i.fits' % i)

            img = o.images[0]
            img.writeFits('child5-%02i-b.fits' % i)

            simg = afwImg.ImageF(W, H)
            for y in range(H):
                for x in range(W):
                    simg.set(x, y, starpix[i][y, x])
            simg.writeFits('star5-%02i.fits' % i)

        if True:
            import pylab as plt
            N = len(objs)
            children = []
            for o in objs:
                img = afwImg.ImageF(W, H)
                o.addToImage(img, 0)
                a = lsstimagetonumpy(img)
                children.append(a)
            mn,mx = min([s.min() for s in starpix]), max([s.max() for s in starpix])

            C = 4
            ploti = 1
            diff = 5 * sqrt(var)

            plt.clf()
            for i,c in enumerate(children):
                # which starpix is closest?
                diffs = array([sqrt(np.mean((c - s)**2)) for s in starpix])
                I = np.argmin(diffs)
                s = starpix[I]

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s+skypix-sky, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(c, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s - c, interpolation='nearest', origin='lower',
                           vmin=-diff, vmax=diff)
                plt.colorbar()

            plt.savefig('child-stars5.png')


    def tstFourStars(self):
        # seed numpy random number generator.
        seed(42)

        # (x, y, flux)
        stars = [(20.4, 32.8, 2000),
                 (28.1, 32.5, 3000),
                 (36.2, 32.2, 4000),
                 (44.3, 32.8, 5000)]

        stars = [(19.4, 32.8, 2000),
                 (28.1, 32.5, 3000),
                 (37.2, 35.2, 4000),
                 (46.3, 32.8, 5000)]

        sky = 100
        W = H = 64
        # Single-Gaussian PSF
        truepsfsigma = 2.

        (maskedImage, truepsf, skypix, starpix) = renderImage(stars, sky, W, H, truepsfsigma)

        image = maskedImage.getImage()
        image.writeFits('test4.fits')

        psf = truepsf
        psfsigma = truepsfsigma

        (bgimg, convolvedImage, detections) = doDetection(maskedImage, psf, psfsigma)
        foots = detections.getFootprints()

        image.writeFits('bgsub1.fits')
        convolvedImage.getImage().writeFits('conv1.fits')

        # DEBUG
        footImg = afwImg.ImageU(W, H, 0)
        for i,f in enumerate(foots):
            print 'footprint:', f.getNpix(), 'pix, bb is', f.getBBox()
            print 'peaks:', len(f.getPeaks())
            f.insertIntoImage(footImg, 65535-i)
        footImg.writeFits('foot4.fits')

        if True:
            import matplotlib
            matplotlib.use('Agg')
            import pylab as plt
            plt.clf()
            im = np.zeros_like(starpix[0])
            for s in starpix:
                im += s
            im += skypix
            plt.imshow(im, interpolation='nearest', origin='lower')
            ax = plt.axis()
            for x,y,flux in stars:
                plt.plot([x],[y], 'r+', ms=10)
            for f in foots:
                bb = f.getBBox()
                x0,y0,x1,y1 = bb.getX0(), bb.getY0(), bb.getX1(), bb.getY1()
                plt.plot([x0,x1,x1,x0,x0], [y0,y0,y1,y1,y0], 'r-', lw=3)
            plt.axis(ax)
            plt.savefig('image4.png')

        # one blob.
        self.assertEqual(len(foots), 1)

        # The LSST Footprint/Peak stuff isn't done, so fake it.
        if False:
            print '%i footprints' % len(foots)
            for fi,f in enumerate(foots):
                print 'footprint', fi, 'has', len(f.getPeaks()), 'peaks'
                for i,p in enumerate(f.getPeaks()):
                    print 'footprint', fi, 'peak', i, 'is', p
            allpeaks = [f.getPeaks() for f in foots]

        else:
            ## HACK -- plug in exact Peak locations.
            peaks = [afwDet.Peak(x,y) for (x,y,flux) in stars]
            allpeaks = [peaks]

        # DEBUG
        maskedImage.getImage().writeFits('mimage4.fits')

        objs = deblender.deblend(foots, allpeaks, maskedImage, psf)

        self.assertNotEqual(objs, None)

        print 'Got deblended objects:', objs
        for i,o in enumerate(objs):
            print o
            img = afwImg.ImageF(W, H)
            o.addToImage(img, 0)
            img.writeFits('child-%02i.fits' % i)

            img = o.images[0]
            img.writeFits('child-%02i-b.fits' % i)

            simg = afwImg.ImageF(W, H)
            for y in range(H):
                for x in range(W):
                    simg.set(x, y, starpix[i][y, x])
            simg.writeFits('star-%02i.fits' % i)

        if True:
            import pylab as plt
            N = len(objs)
            children = []
            for o in objs:
                img = afwImg.ImageF(W, H)
                o.addToImage(img, 0)
                a = lsstimagetonumpy(img)
                children.append(a)
            mn,mx = min([s.min() for s in starpix]), max([s.max() for s in starpix])

            C = 4
            ploti = 1
            diff = 5 * sqrt(sky)

            plt.clf()
            #for i,(s,c) in enumerate(zip(starpix, children)):
            for i,c in enumerate(children):
                # which starpix is closest?
                diffs = array([sqrt(np.mean((c - s)**2)) for s in starpix])
                I = np.argmin(diffs)
                s = starpix[I]

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s+skypix-sky, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(c, interpolation='nearest', origin='lower',
                           vmin=mn, vmax=mx)

                plt.subplot(N,C, ploti)
                ploti += 1
                plt.imshow(s - c, interpolation='nearest', origin='lower',
                           vmin=-diff, vmax=diff)
                plt.colorbar()

            plt.savefig('child-stars4.png')

        #self.assertEqual(4, len(objs))




if __name__ == '__main__':
    unittest.main()

