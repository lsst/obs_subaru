import numpy as np
import matplotlib.pyplot as pyplot
import matplotlib.cm as cm

from mpl_toolkits.axes_grid1 import ImageGrid

import lsst.afw.image as afwImage

def setupPyplot():
    global clippedCm

    pyplot.ion()
    clippedCm = cm.gray
    clippedCm.set_over(color='red')
    clippedCm.set_under(color='blue')

def imgScale(img):
    flatImg = img.flatten()
    s = np.argsort(flatImg, axis=None)
    ns = len(s)
    return flatImg[s[0.05*ns]], flatImg[s[0.99*ns]]

def plotFootprint(exposure, psf, bb, bootPrint):

    mi = exposure.getMaskedImage()
    imgbb = mi.getBBox()

    footImg = afwImage.ImageF(mi.getImage(), bb).getArray()
    residImg = np.array(footImg, copy=True)
    bbExtent = (bb.getMinX(), bb.getMaxX(),
                bb.getMinY(), bb.getMaxY())
    footxy0 = (bb.getMinX(), bb.getMinY())
    bootPeaks = bootPrint.peaks
    vmin, vmax = imgScale(footImg)

    if bb.getWidth() >= bb.getHeight():
        nRow = 2
        nCol = 1
    else:
        nRow = 1
        nCol = 2

    pyplot.figure(1); pyplot.clf()
    footprintGrid = ImageGrid(pyplot.figure(1), 111,
                              nrows_ncols=(nRow, nCol),
                              share_all=True,
                              cbar_mode='each',
                              axes_pad=0.1)


    footprintGrid[0].imshow(footImg,
                            interpolation='nearest',
                            origin='lower',
                            cmap=clippedCm,
                            vmin=vmin, vmax=vmax,
                            extent=bbExtent)
    ax = footprintGrid[0].get_axes()

    nPlot = len(bootPeaks)+1            # input image and residuals

    nRow = int(round(np.sqrt(nPlot)+0.5))
    nCol = int(round(float(nPlot)/nRow + 0.5))

    pyplot.figure(2); pyplot.clf()
    deblendGrid = ImageGrid(pyplot.figure(2), 111,
                            nrows_ncols=(nRow, nCol),
                            share_all=True,
                            axes_pad=0.1)

    deblendGrid[0].imshow(footImg,
                          interpolation='nearest',
                          origin='lower',
                          cmap=clippedCm,
                          aspect='equal',
                          extent=bbExtent)

    for i in range(len(bootPeaks)):
        pk = bootPeaks[i]
        if pk.out_of_bounds:
            thisImg = residImg*0 - 1
            deblendGrid[i+1].imshow(thisImg,
                                    interpolation='nearest',
                                    origin='lower',
                                    cmap=clippedCm,
                                    aspect='equal',
                                    extent=bbExtent)
            
            continue

        stampImg = pk.portion.getArray()
        stampBBox = pk.portion.getBBox(afwImage.PARENT)
        stampxy0 = (stampBBox.getMinX() - footxy0[0],
                    stampBBox.getMinY() - footxy0[1])
        stampxy1 = (stampBBox.getMaxX() - footxy0[0],
                    stampBBox.getMaxY() - footxy0[1])
        #stampxy0 = (pk.stampxy0[0] - footxy0[0],
        #            pk.stampxy0[1] - footxy0[1])
        #stampxy1 = (pk.stampxy1[0] - footxy0[0],
        #            pk.stampxy1[1] - footxy0[1])
        residImg[stampxy0[1]:stampxy1[1]+1,
                 stampxy0[0]:stampxy1[0]+1] -= stampImg

        thisImg = residImg * 0 + np.median(stampImg)
        thisImg[stampxy0[1]:stampxy1[1]+1,
                stampxy0[0]:stampxy1[0]+1] = stampImg

        extent = (pk.stampxy0[0],
                  pk.stampxy1[0],
                  pk.stampxy0[1],
                  pk.stampxy1[1])

        deblendGrid[i+1].imshow(thisImg,
                                interpolation='nearest',
                                origin='lower',
                                cmap=clippedCm,
                                aspect='equal',
                                extent=bbExtent)

        deblendGrid[i+1].plot(pk.center[0], pk.center[1], '+',
                              color='red' if pk.deblend_as_psf else 'blue',
                              scalex=False, scaley=False)

        deblendGrid[0].plot(pk.center[0], pk.center[1], '+',
                            color='red' if pk.deblend_as_psf else 'blue',
                            scalex=False, scaley=False)
        footprintGrid[0].plot(pk.center[0], pk.center[1], '+',
                              color='red' if pk.deblend_as_psf else 'blue',
                              scalex=False, scaley=False)
        footprintGrid[1].plot(pk.center[0], pk.center[1], '+',
                              color='red' if pk.deblend_as_psf else 'blue',
                              scalex=False, scaley=False)


    footprintGrid[1].imshow(residImg,
                            interpolation='nearest',
                            origin='lower',
                            cmap=clippedCm,
                            aspect='equal',
                            extent=bbExtent)

    pyplot.show()
    debug_here()


setupPyplot()
from IPython.core.debugger import Tracer
debug_here = Tracer()
