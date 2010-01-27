from pylab import *
import pyfits

if __name__ == '__main__':
	fn = 'mimage.fits'
	softbias = 1000

	I = pyfits.open(fn)[0].data
	print 'Image:', I.shape

	clf()
	imshow(I, interpolation='nearest', origin='lower',
		   vmin=I.min()-(I.max()-I.min()), vmax=I.max())
		   #cmap='gray',
	gray()
	(H,W) = I.shape

	xlo,xhi = 23,45
	ylo,yhi = 26,40

	for i in range(xlo, xhi+1):
		for j in range(ylo, yhi+1):
			pix = I[j,i]
			if pix < 0:
				s = '(%i)' % round(pix+softbias)
			else:
				s = '%i' % round(pix)

			text(i, j, s, color='k', fontsize='7',
				 horizontalalignment='center', verticalalignment='center')
	axis([xlo-0.5, xhi+0.5, ylo-0.5, yhi+0.5])
	savefig('test1.png')

