from lsst.meas.deblender import deblender

#from lsst.meas.utils import sourceDetection
#import lsst.meas.utils.sourceDetection as sourceDetection

import lsst.meas.algorithms as measAlg
import lsst.afw.math as afwMath
import lsst.afw.detection as afwDet
import lsst.afw.image as afwImg

from numpy.random import seed,poisson
from numpy import meshgrid, array

#	eups declare --current meas_utils svn -r ~/lsst/meas-utils/

import unittest

class TestDeblender(unittest.TestCase):

	def setUp(self):
		pass

	def testTwoStars(self):
		#self.assertEqual(self.seq, range(10))
		# seed numpy random number generator.
		seed(42)
		# Read/create a test image....
		W = H = 64
		sky = 100
		# (x, y, flux)
		#stars = [(26.4, 32.8, 1000),
		#		 (40.1, 32.5, 2000)]
		stars = [(28.4, 32.8, 2000),
				 (38.1, 32.5, 4000)]
		# Single-Gaussian PSF
		truepsfsigma = 2.

		#image = afwImg.ImageF.Factory(W, H, 0)
		maskedImage = afwImg.MaskedImageF(W, H)
		#afwImg.ImageF.Factory(fn, hdu)

		truepsf = measAlg.createPSF("DoubleGaussian", 10, 10, truepsfsigma, 0, 0.0)
		skypix = poisson(sky, (H,W))
		X,Y = meshgrid(range(W), range(H))
		starpix = [array([truepsf.getValue(Xi-x, Yi-y) for (Xi,Yi) in zip(X.ravel(), Y.ravel())]).reshape(X.shape)
				   for (x,y,flux) in stars]
		# normalize by PSF sum (it's not 1!)
		starpix = [flux * s / s.sum()
				   for ((x,y,flux),s) in zip(stars, starpix)]
		#print [s.sum() for s in starpix]

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

		#background, backgroundSubtracted = sourceDetection.estimateBackground(
		#	exposure, self.policy.get("backgroundPolicy"), True)

		bctrl = afwMath.BackgroundControl(afwMath.Interpolate.AKIMA_SPLINE)
		bctrl.setNxSample(5)
		bctrl.setNySample(5)
		bg = afwMath.makeBackground(image, bctrl)

		bgimg = bg.getImageF()
		image.scaledMinus(1, bgimg)
		image.writeFits('bgsub1.fits')

		#psf = measAlg.createPSF("DoubleGaussian", 10, 10, 2, 4, 0.5)
		psf = truepsf
		psfsigma = truepsfsigma
		# "DoubleGaussian", w, h, sigma1, sigma2, b (rel.ampl. of gaussian #2)

		#dsPositive, dsNegative = sourceDetection.detectSources(
		#	backgroundSubtracted, psf, self.policy.get("detectionPolicy"))		  

		minPixels = 5
		# n sigma
		thresholdValue = 10
		thresholdType = afwDet.Threshold.VARIANCE
		thresholdPolarity = "positive"
		nGrow = int(psfsigma)

		# 
		# Smooth the Image
		#
		convolvedImage = maskedImage.Factory(maskedImage.getDimensions())
		convolvedImage.setXY0(maskedImage.getXY0())
		psf.convolve(convolvedImage.getImage(), 
					 image,
					 convolvedImage.getMask().getMaskPlane("EDGE")
					)
		convolvedImage.getImage().writeFits('conv1.fits')

		#
		# Only search psf-smoothed part of frame
		#
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

		print dsPositive.toString()

		footImg = afwImg.ImageU(W, H, 0)

		foots = dsPositive.getFootprints()
		#print foots
		for i,f in enumerate(foots):
			#print 'footprint:', f
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

		objs = deblender.deblend(foots, [peaks], maskedImage, psf)

		self.assertNotEqual(objs, None)



if __name__ == '__main__':
	unittest.main()

