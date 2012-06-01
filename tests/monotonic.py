import matplotlib
matplotlib.use('Agg')
import pylab as plt
import numpy as np

import lsst.afw.detection as afwDet
import lsst.afw.image as afwImg
import lsst.meas.deblender as measDeblend

def main000():
    butils = measDeblend.BaselineUtilsF
    S = 20
    mim = afwImg.MaskedImageF(S,S)
    x0,y0 = S/2, S/2
    im = mim.getImage().getArray()
    foot = afwDet.Footprint()
    peak = afwDet.Peak(x0, y0)

    mask = mim.getMask()
    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0, x0 + 2] = 1.

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
   

def cardinal():
    butils = measDeblend.BaselineUtilsF

    S = 20
    mim = afwImg.MaskedImageF(S,S)
    x0,y0 = S/2, S/2

    im = mim.getImage().getArray()
    foot = afwDet.Footprint()
    peak = afwDet.Peak(x0, y0)
    
    mask = mim.getMask()
    for nm in ['SYMM_1SIG', 'SYMM_3SIG', 'MONOTONIC_1SIG']:
        bit = mask.addMaskPlane(nm)
        val = mask.getPlaneBitMask(nm)

    R = 2
    xx,yy = [],[]
    x,y = R,-R
    for dx,dy in [(0,1), (-1,0), (0,-1), (1,0)]:
        xx.extend(x + dx * np.arange(2*R))
        yy.extend(y + dy * np.arange(2*R))
        x += dx*2*R
        y += dy*2*R

    for i,(dx,dy) in enumerate(zip(xx,yy)):
        im[:,:] = 5.
        im[y0,x0] = 10.
        im[y0 + dy, x0 + dx] = 1.
        mn,mx = im.min(), im.max()
        plota = dict(origin='lower', interpolation='nearest', vmin=mn, vmax=mx)

        plt.clf()
        plt.imshow(im, **plota)
        plt.savefig('im%i.png' % i)
        butils.makeMonotonic(mim, foot, peak, 1.)
        plt.clf()
        plt.imshow(mim.getImage().getArray(), **plota)
        plt.savefig('im%im.png' % i)

if __name__ == '__main__':
    cardinal()
    #main000()
    #randoms(S=100, N=10)
    randoms(S=100, N=1)
