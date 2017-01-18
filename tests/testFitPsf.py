#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2008-2016  AURA/LSST.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <https://www.lsstcorp.org/LegalNotices/>.
#
from __future__ import print_function
from builtins import zip
import unittest
import numpy as np

import lsst.utils.tests
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
from lsst.log import Log
import lsst.meas.algorithms as measAlg
from lsst.meas.deblender.plugins import _fitPsf
from lsst.meas.deblender.baseline import DeblendedPeak, CachingPsf

doPlot = False
if doPlot:
    import matplotlib
    matplotlib.use('Agg')
    import pylab as plt
    print("doPlot is set -- saving plots to current directory.")
else:
    print("doPlot is not set -- not saving plots.  To enable plots, edit", __file__)


class FitPsfTestCase(unittest.TestCase):

    def test1(self):

        # circle
        fp = afwDet.Footprint(afwGeom.Point2I(50, 50), 45.)

        #
        psfsig = 1.5
        psffwhm = psfsig * 2.35
        psf1 = measAlg.DoubleGaussianPsf(11, 11, psfsig)

        psf2 = measAlg.DoubleGaussianPsf(100, 100, psfsig)

        fbb = fp.getBBox()
        print('fbb', fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY())

        fmask = afwImage.MaskU(fbb)
        fmask.setXY0(fbb.getMinX(), fbb.getMinY())
        afwDet.setMaskFromFootprint(fmask, fp, 1)

        sig1 = 10.

        img = afwImage.ImageF(fbb)
        A = img.getArray()
        A += np.random.normal(0, sig1, size=(fbb.getHeight(), fbb.getWidth()))
        print('img x0,y0', img.getX0(), img.getY0())
        print('BBox', img.getBBox())

        peaks = afwDet.PeakCatalog(afwDet.PeakTable.makeMinimalSchema())

        def makePeak(x, y):
            p = peaks.addNew()
            p.setFx(x)
            p.setFy(y)
            p.setIx(int(x))
            p.setIy(int(y))
            return p
        pk1 = makePeak(20., 30.)
        pk2 = makePeak(23., 33.)
        pk3 = makePeak(92., 50.)

        ibb = img.getBBox()
        iext = [ibb.getMinX(), ibb.getMaxX(), ibb.getMinY(), ibb.getMaxY()]
        ix0, iy0 = iext[0], iext[2]

        pbbs = []
        pxys = []

        fluxes = [10000., 5000., 5000.]
        for pk, f in zip(peaks, fluxes):
            psfim = psf1.computeImage(afwGeom.Point2D(pk.getFx(), pk.getFy()))
            print('psfim x0,y0', psfim.getX0(), psfim.getY0())
            pbb = psfim.getBBox()
            print('pbb', pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY())
            pbb.clip(ibb)
            print('clipped pbb', pbb.getMinX(), pbb.getMaxX(), pbb.getMinY(), pbb.getMaxY())
            psfim = psfim.Factory(psfim, pbb)

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
            x0, x1, y0, y1 = fbb.getMinX(), fbb.getMaxX(), fbb.getMinY(), fbb.getMaxY()
            plt.plot([x0, x0, x1, x1, x0], [y0, y1, y1, y0, y0], 'k-')
            for x0, x1, y0, y1 in pbbs:
                plt.plot([x0, x0, x1, x1, x0], [y0, y1, y1, y0, y0], 'r-')
            for x, y in pxys:
                plt.plot(x, y, 'ro')
            plt.axis(ax)
            plt.savefig('img.png')

        varimg = afwImage.ImageF(fbb)
        varimg.set(sig1**2)

        psf_chisq_cut1 = psf_chisq_cut2 = psf_chisq_cut2b = 1.5

        #pkres = PerPeak()
        pkres = DeblendedPeak(pk1, 0, None)

        loglvl = Log.INFO
        # if verbose:
        #    loglvl = Log.DEBUG
        log = Log.getLogger('tests.fit_psf')
        log.setLevel(loglvl)

        cpsf = CachingPsf(psf1)

        peaksF = [pk.getF() for pk in peaks]
        pkF = pk1.getF()

        _fitPsf(fp, fmask, pk1, pkF, pkres, fbb, peaks, peaksF, log, cpsf, psffwhm,
                img, varimg,
                psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print('  ', k, getattr(pkres, k))

        cpsf = CachingPsf(psf2)
        _fitPsf(fp, fmask, pk1, pkF, pkres, fbb, peaks, peaksF, log, cpsf, psffwhm,
                img, varimg,
                psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print('  ', k, getattr(pkres, k))

        pkF = pk3.getF()
        _fitPsf(fp, fmask, pk3, pkF, pkres, fbb, peaks, peaksF, log, cpsf, psffwhm,
                img, varimg,
                psf_chisq_cut1, psf_chisq_cut2, psf_chisq_cut2b)
        for k in dir(pkres):
            if k.startswith('__'):
                continue
            print('  ', k, getattr(pkres, k))


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()

if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
