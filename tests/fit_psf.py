#!/usr/bin/env python

try:
    import matplotlib
    matplotlib.use('Agg')
    import pylab as plt
    doPlot = True
except:
    doPlot = False


import unittest
import lsst.utils.tests         as utilsTests

import numpy as np

import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.pex.logging as pexLogging
import lsst.meas.algorithms as measAlg
from lsst.meas.deblender.baseline import _fitPsf, CachingPsf, PerPeak

class FitPsfTestCase(unittest.TestCase):
    def test1(self):

        # circle
        fp = afwDet.Footprint(afwGeom.Point2I(50,50), 45.)

        #
        psfsig = 1.5
        psffwhm = psfsig * 2.35
        psf1 = measAlg.DoubleGaussianPsf(11, 11, psfsig)

        psf2 = measAlg.DoubleGaussianPsf(100, 100, psfsig)


        fbb = fp.getBBox()
        print 'fbb', fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY()
        
        fmask = afwImage.MaskU(fbb)
        fmask.setXY0(fbb.getMinX(), fbb.getMinY())
        afwDet.setMaskFromFootprint(fmask, fp, 1)

        sig1 = 10.

        img = afwImage.ImageF(fbb)
        A = img.getArray()
        A += np.random.normal(0, sig1, size=(fbb.getHeight(), fbb.getWidth()))
        print 'img x0,y0', img.getX0(), img.getY0()
        print 'BBox', img.getBBox(afwImage.PARENT)

        pk1 = afwDet.Peak(20., 30.)
        pk2 = afwDet.Peak(23., 33.)
        pk3 = afwDet.Peak(92., 50.)
        peaks = []
        peaks.append(pk1)
        peaks.append(pk2)
        peaks.append(pk3)

        ibb = img.getBBox(afwImage.PARENT)
        iext = [ibb.getMinX(), ibb.getMaxX(), ibb.getMinY(), ibb.getMaxY()]
        ix0,iy0 = iext[0], iext[2]

        pbbs = []
        pxys = []
        
        fluxes = [10000., 5000., 5000.]
        for pk,f in zip(peaks, fluxes):
            psfim = psf1.computeImage(afwGeom.Point2D(pk.getFx(), pk.getFy()))
            print 'psfim x0,y0', psfim.getX0(), psfim.getY0()
            pbb = psfim.getBBox(afwImage.PARENT)
            print 'pbb', pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY()
            pbb.clip(ibb)
            print 'clipped pbb', pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY()
            psfim = psfim.Factory(psfim, pbb, afwImage.PARENT)

            psfa = psfim.getArray()
            psfa /= psfa.sum()
            img.getArray()[pbb.getMinY() - iy0: pbb.getMaxY()+1 - iy0,
                           pbb.getMinX() - ix0: pbb.getMaxX()+1 - ix0] += f * psfa

            pbbs.append((pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY()))
            pxys.append((pk.getFx(), pk.getFy()))


        if doPlot:
            plt.clf()
            plt.imshow(img.getArray(), extent=iext, interpolation='nearest', origin='lower')
            ax = plt.axis()
            x0,x1,y0,y1 = fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY()
            plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'k-')
            for x0,x1,y0,y1 in pbbs:
                plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'r-')
            for x,y in pxys:
                plt.plot(x, y, 'ro')
            plt.axis(ax)
            plt.savefig('img.png')

        varimg = afwImage.ImageF(fbb)
        varimg.set(sig1**2)

        psf_chisq_cut1 = psf_chisq_cut2 = psf_chisq_cut2b = 1.5

        pkres = PerPeak()

        loglvl = pexLogging.Log.INFO
        #if verbose:
        #    loglvl = pexLogging.Log.DEBUG
        log = pexLogging.Log(pexLogging.Log.getDefaultLog(), 'tests.fit_psf', loglvl)

        cpsf = CachingPsf(psf1)

        peaksF = [pk.getF() for pk in peaks]
        pkF = pk1.getF()

        _fitPsf(fp, fmask, pk1, pkF, pkres, fbb, peaks, peaksF, log, cpsf, psffwhm,
                 img, varimg,
                 psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print '  ', k, getattr(pkres, k)

        cpsf = CachingPsf(psf2)
        _fitPsf(fp, fmask, pk1, pkF, pkres, fbb, peaks, peaksF, log, cpsf, psffwhm,
                 img, varimg,
                 psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print '  ', k, getattr(pkres, k)


        pkF = pk3.getF()
        _fitPsf(fp, fmask, pk3, pkF, pkres, fbb, peaks, peaksF, log, cpsf, psffwhm,
                 img, varimg,
                 psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print '  ', k, getattr(pkres, k)


#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    utilsTests.init()
    suites = []
    suites += unittest.makeSuite(FitPsfTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""
    utilsTests.run(suite(), exit)
 
if __name__ == "__main__":
    run(True)







