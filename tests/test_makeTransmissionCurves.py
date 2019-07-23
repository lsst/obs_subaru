#
# LSST Data Management System
#
# Copyright 2018 AURA/LSST.
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
import unittest
import numpy as np

import lsst.utils.tests
from lsst.geom import Point2D
from lsst.obs.hsc import makeTransmissionCurves


class MakeTransmissionCurvesTest(lsst.utils.tests.TestCase):

    def testOptics(self):
        wavelengths = np.linspace(4000, 10000, 100)
        point = Point2D(1000, -500)
        for curve in makeTransmissionCurves.getOpticsTransmission().values():
            if curve is None:
                continue
            throughputs = curve.sampleAt(point, wavelengths)
            self.assertTrue((throughputs > 0.70).all())

    def testAtmosphere(self):
        wavelengths = np.linspace(4000, 12000, 100)
        point = Point2D(1000, -500)
        for curve in makeTransmissionCurves.getAtmosphereTransmission().values():
            if curve is None:
                continue
            throughputs = curve.sampleAt(point, wavelengths)
            ohAbsorption = np.logical_and(wavelengths > 11315, wavelengths < 11397)
            midR = np.logical_and(wavelengths > 6000, wavelengths < 6300)
            self.assertTrue((throughputs[ohAbsorption] < throughputs[midR]).all())

    def testSensors(self):
        wavelengths = np.linspace(4000, 12000, 100)
        point = Point2D(200, 10)
        for sensors in makeTransmissionCurves.getSensorTransmission().values():
            for i in range(112):
                curve = sensors[i]
                throughputs = curve.sampleAt(point, wavelengths)
                siliconTransparent = wavelengths > 11000
                self.assertTrue((throughputs[siliconTransparent] < 0.01).all())
                midR = np.logical_and(wavelengths > 6000, wavelengths < 6300)
                self.assertTrue((throughputs[midR] > 0.9).all())

    def testFilters(self):
        wavelengths = np.linspace(3000, 12000, 1000)
        point = Point2D(1000, -500)

        def check(curve, central, w1, w2):
            # check that throughput within w1 of central is strictly greater
            # than throughput outside w2 of central
            throughput = curve.sampleAt(point, wavelengths)
            mid = np.logical_and(wavelengths > central - w1, wavelengths < central + w1)
            outer = np.logical_or(wavelengths < central - w2, wavelengths > central + w2)
            self.assertGreater(throughput[mid].min(), throughput[outer].max())

        for curves in makeTransmissionCurves.getFilterTransmission().values():
            check(curves["NB0387"], 3870, 50, 100)
            check(curves["NB0816"], 8160, 50, 100)
            check(curves["NB0921"], 9210, 50, 100)
            check(curves["HSC-G"], 4730, 500, 1500)
            check(curves["HSC-R"], 6230, 500, 1500)
            check(curves["HSC-R2"], 6230, 500, 1500)
            check(curves["HSC-I"], 7750, 500, 1500)
            check(curves["HSC-I2"], 7750, 500, 1500)
            check(curves["HSC-Z"], 8923, 500, 1500)
            check(curves["HSC-Y"], 10030, 500, 1500)

    def testRadialDependence(self):
        wavelengths = np.linspace(5000, 9000, 500)
        radii = np.linspace(0, 20000, 50)

        def computeMeanWavelengths(curve):
            w = []
            for radius in radii:
                throughput = curve.sampleAt(Point2D(radius, 0), wavelengths)
                w.append(np.dot(throughput, wavelengths)/throughput.sum())
            return np.array(w)

        for curves in makeTransmissionCurves.getFilterTransmission().values():
            r = computeMeanWavelengths(curves['HSC-R'])
            r2 = computeMeanWavelengths(curves['HSC-R2'])
            i = computeMeanWavelengths(curves['HSC-I'])
            i2 = computeMeanWavelengths(curves['HSC-I2'])
            # i2 and r2 should have lower variance as a function of radius
            self.assertLess(r2.std(), r.std())
            self.assertLess(i2.std(), i.std())
            # the mean wavelengths of the two r and two i filters should
            # neverthess be close (within 100 angstroms)
            self.assertLess(np.abs(r2.mean() - r.mean()), 100)
            self.assertLess(np.abs(i2.mean() - i.mean()), 100)


def setup_module(module):
    lsst.utils.tests.init()


if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
