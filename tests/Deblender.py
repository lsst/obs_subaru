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

class TestDeblender(unittest.TestCase):

    def setUp(self):
        pass

    def testTwoStars(self):
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
        #truepsf = measAlg.createPSF("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0)
        truepsf = afwDet.createPsf("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0)

        skypix = poisson(sky, (H,W))

        starpix = []
        for (x,y,flux) in stars:
            psfimg = truepsf.computeImage(afwGeom.makePointD(x, y))
            print 'psfimg has x0,y0', psfimg.getX0(), psfimg.getY0()
            #print 'psfimg has dir', dir(psfimg)
            X,Y = meshgrid(range(W), range(H))
            a = np.zeros((H,W))
            x0,y0 = psfimg.getY0(), psfimg.getX0()
            for y in range(psfimg.getHeight()):
                for x in range(psfimg.getWidth()):
                    a[y+y0, x+x0] = psfimg.get(x,y)
            starpix.append(a)
            #starpix.append(array([psfimg.get(xx,yy) for (xx,yy) in zip(X.ravel(), Y.ravel())]).reshape(X.shape))
        #starpix = [array([psfimg.get(Xi-x, Yi-y) for (Xi,Yi) in zip(X.ravel(), Y.ravel())]).reshape(X.shape)
        #           for (x,y,flux) in stars]
        # normalize by PSF sum (it's not 1!)
        starpix = [flux * s / s.sum()
                   for ((x,y,flux),s) in zip(stars, starpix)]

        maskedImage = afwImg.MaskedImageF(W, H)
        image = maskedImage.getImage()
        for y in range(H):
            for x in range(W):
                image.set(x, y, skypix[y,x] + sum([s[y,x] for s in starpix]))

        image.writeFits('test1.fits')

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
        
        #psf.convolve(convolvedImage.getImage(), 
        #             image,
        #             convolvedImage.getMask().getMaskPlane("EDGE")
        #            )
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

        #do positive detections        
        threshold = afwDet.createThreshold(thresholdValue)
        dsPositive = afwDet.makeFootprintSet(
            middle, threshold, "DETECTED", minPixels)

        #set detection region to be the entire region
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

        ## HACK -- plug in exact Peak locations.
        # UGH -- can't do that because _peaks is private to Footprint!
        #foots[0].getPeaks() = [afwDet.Peak(x,y) for (x,y,flux) in stars]
        peaks = [afwDet.Peak(x,y) for (x,y,flux) in stars]

        # DEBUG
        maskedImage.getImage().writeFits('mimage.fits')

        objs = deblender.deblend(foots, [peaks], maskedImage, psf)

        self.assertNotEqual(objs, None)


if __name__ == '__main__':
    unittest.main()

