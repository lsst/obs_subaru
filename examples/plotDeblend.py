import numpy as np
import matplotlib.pyplot as pyplot
import matplotlib.cm as cm

import lsst.afw.image as afwImage

def setupPyplot():
    pyplot.ion()
    
def plotFootprint(exposure, psf, bb, bootPrint):

    mi = exposure.getMaskedImage()
    imgbb = mi.getBBox()
    
    setupPyplot()
    bootPeaks = bootPrint.peaks
    pyplot.figure(1); pyplot.clf()
    pyplot.imshow(afwImage.ImageF(mi.getImage(), bb).getArray(),
                  cmap=cm.gray, extent=(bb.getMinX(), bb.getMaxX(),
                                        bb.getMinY(), bb.getMaxY()))
    
    nPlot = len(bootPeaks)+2            # input image and residuals
    
    nRow = int(round(np.sqrt(nPlot)+0.5))
    nCol = int(round(float(nPlot)/nRow + 0.5))
                
    pyplot.figure(2); pyplot.clf()
    pyplot.subplots(nRow, nCol, num=2)
    pyplot.subplot(nRow, nCol, 1)
    pyplot.imshow(afwImage.ImageF(mi.getImage(), bb).getArray(),
                  cmap=cm.gray, extent=(bb.getMinX(), bb.getMaxX(),
                                        bb.getMinY(), bb.getMaxY()))
    ax = pyplot.figure(2).get_axes()
    for i in range(len(bootPeaks)):
        pk = bootPeaks[i]
        if pk.out_of_bounds:
            # Get more info here....
            continue
        
        pyplot.figure(2)
        pyplot.subplot(nRow, nCol, i+2)
        pyplot.imshow(bootPeaks[i].stamp.getArray(), axes=ax,
                      cmap=cm.gray, extent=pk.stampxy0 + pk.stampxy1)


        pyplot.figure(1)
