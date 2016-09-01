#!/usr/bin/env python

import sys
import os
import re

import numpy
import matplotlib.figure as figure
from matplotlib.backends.backend_agg import FigureCanvasAgg as FigCanvas
from matplotlib.patches import Rectangle

import lsst.pex.logging as pexLog
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.meas.algorithms as measAlg
import lsst.meas.deblender.baseline as measDeb

from lsst.meas.deblender import BaselineUtilsF as butils


def randomCoords(nSrc, grid=False, minSep=0.15, maxSep=0.25):

    if grid:
        # 3 in a row is a known failure, so ignore nSrc
        xy = (
            (0.33, 0.33),
            (0.33, 0.67),
            (0.67, 0.33),
            (0.67, 0.67),
        )
        nSrc = len(xy)
    else:
        i = 0
        x = numpy.array([numpy.random.uniform(maxSep, 1.0-maxSep)])
        y = numpy.array([numpy.random.uniform(maxSep, 1.0-maxSep)])
        nMax = 10

        # keep trying to add sources until we have nSrc
        while (i < nSrc - 1):
            smin = 0.0
            iTry = 0
            _x, _y = None, None
            while ((smin < minSep or smin > maxSep) and iTry < nMax):
                _x, _y = numpy.random.uniform(maxSep, 1.0-maxSep, 2)
                dx = x - _x
                dy = y - _y
                s = numpy.sqrt(dx*dx + dy*dy)
                smin = s.min()
                print smin, iTry
                iTry += 1
            if _x and _y:
                x = numpy.append(x, _x)
                y = numpy.append(y, _y)
            i += 1
        xy = zip(x, y)

    return xy


def makeFakeImage(nx, ny, xys, fluxes, fwhms):

    mimg = afwImage.MaskedImageF(nx, ny)
    img = mimg.getImage().getArray()
    var = mimg.getVariance().getArray()
    var[:, :] = 1.0

    nSrc = len(xys)

    n = min([nx, ny])

    for i in range(nSrc):
        x, y = nx*xys[i][0], ny*xys[i][1]
        src = measAlg.DoubleGaussianPsf(n, n, fwhms[i], fwhms[i], 0.03)
        pimg = src.computeImage(afwGeom.Point2D(x, y))
        pbox = pimg.getBBox(afwImage.PARENT)
        pbox.clip(mimg.getBBox())
        pimg = pimg.Factory(pimg, pbox, afwImage.PARENT)
        xlo, xhi = pbox.getMinX(), pbox.getMaxX() + 1
        ylo, yhi = pbox.getMinY(), pbox.getMaxY() + 1
        img[ylo:yhi, xlo:xhi] += fluxes[i]*pimg.getArray()

    return mimg


def detect(mimg):

    thresh = afwDet.createThreshold(10., 'value', True)
    fpSet = afwDet.FootprintSet(mimg, thresh, 'DETECTED', 1)
    fps = fpSet.getFootprints()
    print 'found', len(fps), 'footprints'
    for fp in fps:
        print 'peaks:', len(fp.getPeaks())
        for pk in fp.getPeaks():
            print '  ', pk.getIx(), pk.getIy()
    return fps[0] if fps else None


def makePortionFigure(deblend, origMimg, origMimgB, pedestal=0.0):

    portions = []
    centers = []
    boxes = []
    for i, peak in enumerate(deblend.peaks):
        # make an image matching the size of the original
        portionedImg = afwImage.ImageF(origMimg.getBBox())

        # get the heavy footprint for the flux aportioned to this peak
        heavyFoot = afwDet.cast_HeavyFootprintF(peak.getFluxPortion())
        footBox = heavyFoot.getBBox()
        pk = peak.peak
        centers.append((pk.getIx(), pk.getIy()))
        boxes.append(((footBox.getMinX(), footBox.getMinY()), footBox.getWidth(), footBox.getHeight()))
        print i, peak, pk.getIx(), pk.getIy(), footBox, "skip:", peak.skip

        # make a sub-image for this peak, and put the aportioned flux into it
        portionedSubImg = afwImage.ImageF(portionedImg, footBox)
        portionedSubImg += pedestal
        heavyFoot.insert(portionedSubImg)

        portions.append(portionedImg)

    fig = figure.Figure(figsize=(8, 10))
    canvas = FigCanvas(fig)

    g = int(numpy.ceil(numpy.sqrt(len(deblend.peaks))))
    gx, gy = g, g+1

    def makeAx(ax, im, title):
        im = im.getArray()
        a = ax.imshow(im, cmap='gray')
        cb = fig.colorbar(a)
        ax.set_title(title, size='small')
        ax.set_xlim((0, im.shape[0]))
        ax.set_ylim((0, im.shape[1]))
        for t in ax.get_xticklabels() + ax.get_yticklabels() + cb.ax.get_yticklabels():
            t.set_size('x-small')
        return ax

    # show the originals
    makeAx(fig.add_subplot(gy, gx, 1), origMimg.getImage(), "Orig")
    makeAx(fig.add_subplot(gy, gx, 2), origMimgB.getImage(), "Missing Src")

    # show each aportioned image
    i = gy
    for i_p in range(len(portions)):
        im = portions[i_p]
        ax = makeAx(fig.add_subplot(gy, gx, i), im, "")
        xy, w, h = boxes[i_p]
        ax.add_patch(Rectangle(xy, w, h, fill=False, edgecolor='#ff0000'))
        for x, y in centers:
            ax.plot(x, y, '+', color='#00ff00')
        i += 1

    return fig


def main():

    log = pexLog.Log(pexLog.Log.getDefaultLog(), 'foo', pexLog.Log.INFO)

    ny, nx = 256, 256

    fwhm0 = 5.0
    psf = measAlg.DoubleGaussianPsf(21, 21, fwhm0)
    flux = 1.0e6

    # make two sets of fake data, seconds set is missing a source
    nSrc = 4
    xy = randomCoords(nSrc)
    fluxs = [flux]*(nSrc-1) + [0.7*flux]
    mimg = makeFakeImage(nx, ny, xy, fluxs, [3.0*fwhm0]*nSrc)
    img = mimg.getImage().getArray()
    bbox = mimg.getBBox(afwImage.PARENT)
    mimg.writeFits("foo.fits")

    nSrcB = nSrc - 4
    mimgB = makeFakeImage(nx, ny, xy[0:nSrcB], fluxs[0:nSrcB], [3.0*fwhm0]*nSrcB)
    imgB = mimgB.getImage().getArray()
    bboxB = mimgB.getBBox(afwImage.PARENT)
    mimgB.writeFits("fooB.fits")

    # Run the detection
    fp = detect(mimg)
    fpB = detect(mimgB)

    # deblend mimgB (missing a peak) using the fp with the extra peak
    deb = measDeb.deblend(fp, mimgB, psf, fwhm0, verbose=True, rampFluxAtEdge=True, log=log)
    print "Deblended peaks: ", len(deb.peaks)

    fig = makePortionFigure(deb, mimg, mimgB)
    fig.savefig("test.png")


if __name__ == '__main__':
    main()
