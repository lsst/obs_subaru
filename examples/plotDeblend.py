import numpy as np
import matplotlib.pyplot as pyplot
import matplotlib.cm as cm

from mpl_toolkits.axes_grid1 import ImageGrid

import lsst.afw.image as afwImage

clippedCm = None

def setupPyplot():
    global clippedCm

    pyplot.ion()
    clippedCm = cm.gray
    clippedCm.set_over(color='red')
    clippedCm.set_under(color='blue')

def getImgScale(img, pmin=0.0, pmax=1.0):
    ss = np.sort(img.ravel())
    mn,mx = [ss[int(p*(len(ss)-1))] for p in [pmin, pmax]]
    #print 'mn,mx', mn,mx

    q1,q2,q3 = [ss[int(p*len(ss))] for p in [0.25, 0.5, 0.75]]

    return mn, q1, q2, q3, mx

def myimshow(imshow, x, qscales=None, *args, **kwargs):
    mykwargs = kwargs.copy()

    if qscales == None:
        qscales = getImgScale(x, **mykwargs)
    mn, q1, q2, q3, mx = qscales

    def nlmap(X, q1, q2, q3):
        Y = (X - q2) / ((q3-q1)/2.)
        return np.arcsinh(Y * 10.)/10.

    mykwargs['vmin'] = nlmap(mn, q1, q2, q3)
    mykwargs['vmax'] = nlmap(mx, q1, q2, q3)

    return imshow(nlmap(x, q1, q2, q3), *args, **mykwargs)

def imgScale(img):
    flatImg = img.flatten()
    s = np.argsort(flatImg, axis=None)
    ns = len(s)
    return flatImg[s[0.05*ns]], flatImg[s[0.99*ns]]

def plotFootprint(exposure, psf, bootPrint):

    mi = exposure.getMaskedImage()
    imgbb = mi.getBBox()
    bb = bootPrint.getBBox()
    
    footImg = afwImage.ImageF(mi.getImage(), imgbb).getArray()
    residImg = np.array(footImg, copy=True)
    bbExtent = (bb.getMinX(), bb.getMaxX(),
                bb.getMinY(), bb.getMaxY())
    footxy0 = (bb.getMinX(), bb.getMinY())
    bootPeaks = bootPrint.peaks

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

    qscales = getImgScale(footImg)
    myimshow(footprintGrid[0].imshow, footImg,
             qscales=qscales,
             interpolation='nearest',
             origin='lower',
             cmap=clippedCm,
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

    myimshow(deblendGrid[0].imshow, footImg,
             qscales=qscales,
             interpolation='nearest',
             origin='lower',
             cmap=clippedCm,
             aspect='equal',
             extent=bbExtent)

    for i in range(len(bootPeaks)):
        pk = bootPeaks[i]
        if pk.out_of_bounds:
            thisImg = residImg*0 - 1
            myimshow(deblendGrid[i+1].imshow, thisImg,
                     qscales=qscales,
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
        thisImg = residImg * 0 + np.median(stampImg)
        try:
            residImg[stampxy0[1]:stampxy1[1]+1,
                     stampxy0[0]:stampxy1[0]+1] -= stampImg

            thisImg[stampxy0[1]:stampxy1[1]+1,
                    stampxy0[0]:stampxy1[0]+1] = stampImg
        except Exception, e:
            print "subframe botch"
            debug_here()

        myimshow(deblendGrid[i+1].imshow, thisImg,
                 qscales=qscales,
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


    myimshow(footprintGrid[1].imshow, residImg,
             qscales=qscales,
             interpolation='nearest',
             origin='lower',
             cmap=clippedCm,
             aspect='equal',
             extent=bbExtent)

    pyplot.show()
    # debug_here()


setupPyplot()
from IPython.core.debugger import Tracer
debug_here = Tracer()
