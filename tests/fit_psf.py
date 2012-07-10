#!/usr/bin/env python

import matplotlib
matplotlib.use('Agg')
import pylab as plt


import unittest
import lsst.utils.tests         as utilsTests

import numpy as np

import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.pex.logging as pexLogging
from lsst.meas.deblender.baseline import _fit_psf, CachingPsf, PerPeak

class FitPsfTestCase(unittest.TestCase):
    def test1(self):

        # circle
        fp = afwDet.Footprint(afwGeom.Point2I(50,50), 45.)

        #
        psfsig = 1.5
        psffwhm = psfsig * 2.35
        psf1 = afwDet.createPsf('DoubleGaussian', 11, 11, psfsig)

        psf2 = afwDet.createPsf('DoubleGaussian', 100, 100, psfsig)


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

        pk = afwDet.Peak(20., 30.)
        pk2 = afwDet.Peak(23., 33.)
        pk3 = afwDet.Peak(92., 50.)
        peaks = []
        peaks.append(pk)
        peaks.append(pk2)
        peaks.append(pk3)

        ibb = img.getBBox(afwImage.PARENT)
        iext = [ibb.getMinX(), ibb.getMaxX(), ibb.getMinY(), ibb.getMaxY()]
        ix0,iy0 = iext[0], iext[2]

        f1 = 10000.
        psfim = psf1.computeImage(afwGeom.Point2D(pk.getFx(), pk.getFy()))
        print 'psfim x0,y0', psfim.getX0(), psfim.getY0()
        pbb = psfim.getBBox(afwImage.PARENT)
        print 'pbb', pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY()
        psfa = psfim.getArray()
        psfa /= psfa.sum()
        img.getArray()[pbb.getMinY() - iy0: pbb.getMaxY()+1 - iy0,
                       pbb.getMinX() - ix0: pbb.getMaxX()+1 - ix0] += f1 * psfa


        plt.clf()
        plt.imshow(img.getArray(), extent=iext, interpolation='nearest', origin='lower')
        ax = plt.axis()
        x0,x1,y0,y1 = fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY()
        plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'k-')
        x0,x1,y0,y1 = pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY()
        plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'r-')
        plt.plot(pk.getFx(), pk.getFy(), 'ro')
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
        _fit_psf(fp, fmask, pk, pkres, fbb, peaks, log, cpsf, psffwhm, img, varimg,
                 psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print '  ', k, getattr(pkres, k)


        cpsf = CachingPsf(psf2)
        _fit_psf(fp, fmask, pk, pkres, fbb, peaks, log, cpsf, psffwhm, img, varimg,
                 psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print '  ', k, getattr(pkres, k)


        _fit_psf(fp, fmask, pk3, pkres, fbb, peaks, log, cpsf, psffwhm, img, varimg,
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







