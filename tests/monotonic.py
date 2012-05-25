import matplotlib
matplotlib.use('Agg')
import pylab as plt
import numpy as np

import lsst.afw.detection as afwDet
import lsst.afw.image as afwImg
import lsst.meas.deblender as measDeblend

def main000():
    butils = measDeblend.BaselineUtilsF
    mim = afwImg.MaskedImageF(10, 10)
    x0,y0 = 5,5
    im = mim.getImage().getArray()
    foot = afwDet.Footprint()
    peak = afwDet.Peak(x0, y0)

    mask = mim.getMask()
    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0-1, x0] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im2.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mim.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono2.png')

    """
    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0+1, x0+1] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im6.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mim.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono6.png')
    """

def randoms(S=10, N=1, GA=10, GS=10):
    butils = measDeblend.BaselineUtilsF
    mim = afwImg.MaskedImageF(S, S)
    x0,y0 = S/2, S/2
    mask = mim.getMask()
    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)
    foot = afwDet.Footprint()
    peak = afwDet.Peak(x0, y0)
    im = mim.getImage().getArray()

    for i in range(N):

        X,Y = np.meshgrid(np.arange(S), np.arange(S))
        R2 = (X-x0)**2 + (Y-y0)**2

        im[:,:] = np.random.normal(10, 1, size=im.shape) + GA * np.exp(-0.5 * R2 / GS**2)

        plt.clf()
        ima = dict(vmin=im.min(), vmax=im.max(), origin='lower', interpolation='nearest')
        plt.imshow(im, **ima)
        plt.gray()
        plt.savefig('Rim%i.png' % i)
        butils.makeMonotonic(mim, foot, peak, 1.)
        plt.clf()
        plt.imshow(mim.getImage().getArray(), **ima)
        plt.gray()
        plt.savefig('Rim%im.png' % i)
   

def main():
    butils = measDeblend.BaselineUtilsF

    mim = afwImg.MaskedImageF(10, 10)
    x0,y0 = 5,5
    #mim = afwImg.MaskedImageF(100, 100)
    #x0,y0 = 50,50
    im = mim.getImage().getArray()
    foot = afwDet.Footprint()
    peak = afwDet.Peak(x0, y0)
    
    im[:,:] = 5.
    im[y0,x0] = 10.

    mask = mim.getMask()
    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im1.png')

    butils.makeMonotonic(mim, foot, peak, 1.)
    mono = mim

    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono1.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0-1, x0] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im2.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono2.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0, x0-1] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im3.png')

    butils.makeMonotonic(mim, foot, peak, 1.)

    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono3.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0+1, x0] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im4.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono4.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0, x0+1] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im5.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono5.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0+1, x0+1] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im6.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono6.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0-1, x0+1] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im7.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono7.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0-1, x0-1] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im8.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono8.png')

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0+1, x0-1] = 1.

    plt.clf()
    plt.imshow(im, origin='lower', interpolation='nearest')
    plt.savefig('im9.png')
    butils.makeMonotonic(mim, foot, peak, 1.)
    plt.clf()
    plt.imshow(mono.getImage().getArray(), origin='lower', interpolation='nearest')
    plt.savefig('mono9.png')


if __name__ == '__main__':
    #main()
    randoms(S=100, N=10)
