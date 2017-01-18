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
import unittest
import numpy as np

import lsst.utils.tests
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
from lsst.meas.deblender.baseline import deblend
import lsst.meas.algorithms as measAlg

def imExt(img):
    bbox = img.getBBox()
    return [bbox.getMinX(), bbox.getMaxX(), bbox.getMinY(), bbox.getMaxY()]

def doubleGaussianPsf(W, H, fwhm1, fwhm2, a2):
    return measAlg.DoubleGaussianPsf(W, H, fwhm1, fwhm2, a2)

def gaussianPsf(W, H, fwhm):
    return measAlg.DoubleGaussianPsf(W, H, fwhm)

class DegenerateTemplateTestCase(lsst.utils.tests.TestCase):

    def testPeakRemoval(self):
        '''
        A simple example: three overlapping blobs (detected as 1
        footprint with three peaks).  Additional peaks are added near
        the blob peaks that should be identified as degenerate.
        '''
        H, W = 100, 100

        fpbb = afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Point2I(W - 1, H - 1))

        afwimg = afwImage.MaskedImageF(fpbb)
        imgbb = afwimg.getBBox()
        img = afwimg.getImage().getArray()

        var = afwimg.getVariance().getArray()
        var[:, :] = 1.

        blob_fwhm = 10.
        blob_psf = doubleGaussianPsf(99, 99, blob_fwhm, 2.*blob_fwhm, 0.03)

        fakepsf_fwhm = 3.
        fakepsf = gaussianPsf(11, 11, fakepsf_fwhm)

        blobimgs = []
        x = 75.
        XY = [(x, 35.), (x, 65.), (50., 50.)]
        flux = 1e6
        for x, y in XY:
            bim = blob_psf.computeImage(afwGeom.Point2D(x, y))
            bbb = bim.getBBox()
            bbb.clip(imgbb)

            bim = bim.Factory(bim, bbb)
            bim2 = bim.getArray()

            blobimg = np.zeros_like(img)
            blobimg[bbb.getMinY():bbb.getMaxY()+1, bbb.getMinX():bbb.getMaxX()+1] += flux*bim2
            blobimgs.append(blobimg)

            img[bbb.getMinY():bbb.getMaxY()+1,
                bbb.getMinX():bbb.getMaxX()+1] += flux * bim2

        # Run the detection code to get a ~ realistic footprint
        thresh = afwDet.createThreshold(5., 'value', True)
        fpSet = afwDet.FootprintSet(afwimg, thresh, 'DETECTED', 1)
        fps = fpSet.getFootprints()

        self.assertTrue(len(fps) == 1)

        # Add new peaks near to the first peaks that will be degenerate
        fp0 = fps[0]
        for x, y in XY:
            fp0.addPeak(x - 10, y + 6, 10)

        deb = deblend(fp0, afwimg, fakepsf, fakepsf_fwhm, verbose=True, removeDegenerateTemplates=True)

        self.assertTrue(deb.deblendedParents[0].peaks[3].degenerate)
        self.assertTrue(deb.deblendedParents[0].peaks[4].degenerate)
        self.assertTrue(deb.deblendedParents[0].peaks[5].degenerate)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


class TestMemory(lsst.utils.tests.MemoryTestCase):
    pass


def setup_module(module):
    lsst.utils.tests.init()

if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
