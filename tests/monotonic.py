#!/usr/bin/env python
try:
    import matplotlib
    matplotlib.use('Agg')
    import pylab as plt
except ImportError:
    plt = None

import numpy as np

import lsst.afw.detection as afwDet
import lsst.afw.image as afwImg
import lsst.meas.deblender as measDeblend

def makePeak(x, y):
    peakTable = afwDet.PeakTable.make(afwDet.PeakTable.makeMinimalSchema())
    peak = peakTable.makeRecord()
    peak.setFx(x)
    peak.setFy(y)
    peak.setIx(x)
    peak.setIy(y)
    return peak

def main000():
    butils = measDeblend.BaselineUtilsF
    S = 20
    mim = afwImg.MaskedImageF(S,S)
    x0,y0 = S/2, S/2
    im = mim.getImage().getArray()
    peakTable = afwDet.PeakTable.make(afwDet.PeakTable.makeMinimalSchema)
    peak = makePeak(x0, y0)

    im[:,:] = 5.
    im[y0,x0] = 10.
    im[y0, x0 + 2] = 1.

    if plt:
        plt.clf()
        plt.imshow(im, origin='lower', interpolation='nearest')
        plt.savefig('im2.png')
        butils.makeMonotonic(mim, peak)
        plt.clf()
        plt.imshow(mim.getImage().getArray(), origin='lower', interpolation='nearest')
        plt.savefig('mono2.png')

def randoms(S=10, N=1, GA=10, GS=10):
    butils = measDeblend.BaselineUtilsF
    mim = afwImg.MaskedImageF(S, S)
    x0,y0 = S/2, S/2
    peak = makePeak(x0, y0)
    im = mim.getImage().getArray()

    for i in range(N):

        X,Y = np.meshgrid(np.arange(S), np.arange(S))
        R2 = (X-x0)**2 + (Y-y0)**2

        im[:,:] = np.random.normal(10, 1, size=im.shape) + GA * np.exp(-0.5 * R2 / GS**2)

        if plt:
            plt.clf()
            ima = dict(vmin=im.min(), vmax=im.max(), origin='lower', interpolation='nearest')
            plt.imshow(im, **ima)
            plt.gray()
            plt.savefig('Rim%i.png' % i)
            butils.makeMonotonic(mim, peak)
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
    peak = makePeak(x0, y0)
    
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

        if plt:
            plt.clf()
            plt.imshow(im, **plota)
            plt.savefig('im%i.png' % i)
            butils.makeMonotonic(mim, peak)
            plt.clf()
            plt.imshow(mim.getImage().getArray(), **plota)
            plt.savefig('im%im.png' % i)

if __name__ == '__main__':
    cardinal()
    #main000()
    #randoms(S=100, N=10)
    randoms(S=100, N=1)
