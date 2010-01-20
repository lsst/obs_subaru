import lsst.meas.deblender.deblenderLib as lib

def deblend(footprints, peaks, maskedImage, psf):
	print 'type of footprints:', type(footprints)
	print 'footprints:', footprints
	print 'type of peaks:', type(peaks)
	print 'peaks:', peaks
	print 'type of maskedImage:', type(maskedImage)
	print 'maskedImage:', maskedImage
	print 'type of psf:', type(psf)
	print 'psf:', psf
	# convert 2d list "peaks" into 2d vector.
	pks = lib.PeakContainerListT()
	for ll in peaks:
		cll = lib.PeakContainerT()
		for x in ll:
			cll.push_back(x)
		pks.push_back(cll)
	print 'peaks converted to C++:', pks

	#objs = lib.deblend(footprints, pks, maskedImage, psf)
	db = lib.SDSSDeblenderF()
	print db.deblend.__doc__
	objs = db.deblend(footprints, pks, maskedImage, psf)

	return None
