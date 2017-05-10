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
from __future__ import absolute_import, division, print_function
import unittest

import lsst.utils.tests
from lsst.obs.hsc import HscMapper
from lsst.afw.cameraGeom import PIXELS, FOCAL_PLANE, PUPIL, TAN_PIXELS
from lsst.afw.geom import Point2D


class HscDistortionTestCase(unittest.TestCase):
    """Testing HscDistortion implementation

    HscDistortion is based on the HSC package "distEst".  We
    test that it produces the same results.
    """

    @lsst.utils.tests.debugger(Exception)
    def test(self):
        verification = getVerificationData()
        camera = HscMapper(root=".").camera
        for ccd in camera:
            ccdVerification = verification[ccd.getName()]
            self.assertEqual(ccd.getName(), ccdVerification.name)
            self.assertEqual(int(ccd.getSerial()), int(ccdVerification.id))
            for xy in ccd.getBBox().getCorners():
                pixels = ccd.makeCameraPoint(Point2D(xy), PIXELS)
                fp = ccd.transform(pixels, FOCAL_PLANE)
                tp = camera.transform(pixels, TAN_PIXELS)
                camera.transform(fp, PUPIL)

                cornerVerification = ccdVerification.corners[tuple(xy)]
                fpDiff = fp.getPoint() - cornerVerification.focalPlane
                tp.getPoint() - cornerVerification.distEst

                print(ccd.getSerial(), ccd.getName(), xy, fp, fpDiff)


#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(HscDistortionTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)


def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

################################################################################

from collections import namedtuple
CcdData = namedtuple("CcdData", ['name', 'id', 'corners'])
CornerData = namedtuple("CornerData", ['focalPlane', 'distEst'])


def getVerificationData():
    """Retrieve verification data

    The verification data consists of the focal plane and distorted positions of
    each corner of each CCD in HSC.

    The verification data has been pickled for inclusion in this file.  Pickling
    it is the most convenient means of recording it for posterity, as it is mostly
    self-contained (apart from the definition of the namedtuple classes, which we
    prefer to a dict because of their convenience).

    It was constructed with the following code, using hscPipe 3.5.1:

        import pickle
        import itertools
        from collections import namedtuple
        import lsst.afw.geom as afwGeom
        import lsst.afw.cameraGeom as afwCG
        from lsst.obs.hsc import HscMapper

        camera = HscMapper(root=".").camera
        CcdData = namedtuple("CcdData", ['name', 'id', 'corners'])
        CornerData = namedtuple("CornerData", ['focalPlane', 'distEst'])
        data = {}
        for raft in map(afwCG.cast_Raft, camera):  # now use: `for raft in camera:`
            for ccd in map(afwCG.cast_Ccd, raft):  # now use: `for ccd in raft:`
                ccd.setTrimmed(True)
                distorter = ccd.getDistortion()
                cornerData = {}
                data[ccd.getId().getName()] = CcdData(ccd.getId().getName(), ccd.getId().getSerial(),
                                                      cornerData)
                # Detector.getPositionFromPixel takes pixel positions AFTER the CCD has been transformed into
                # camera coordinates, and I can't see any way to convert positions from BEFORE the
                # transformation.  So we have to transform them ourselves.
                offset = afwGeom.Extent2D(ccd.getCenterPixel())
                orientation = ccd.getOrientation()
                transform = (afwGeom.AffineTransform.makeTranslation(offset)*
                             afwGeom.AffineTransform.makeRotation(90.0*orientation.getNQuarter()*
                                                                  afwGeom.degrees + orientation.getYaw())*
                             afwGeom.AffineTransform.makeTranslation(offset*-1))
                for x, y in itertools.product([0, 2047], [0, 4175]):
                    point = transform(afwGeom.Point2D(x,y))
                    print (ccd.getId().getSerial(), ccd.getId().getName(), x, y, point,
                           ccd.getPositionFromPixel(point).getMm())
                    cornerData[(x,y)] = CornerData(ccd.getPositionFromPixel(point).getMm(),
                                                   distorter.undistort(point, ccd))

        print pickle.dumps(data)
    """
    return {'1_44': CcdData(name='1_44', id=6,
                            corners={(0, 0): CornerData(focalPlane=Point2D(13764.18245, -65.60289259),
                                                        distEst=Point2D(1757.325262, 4175.07894)),
                                     (2047, 0): CornerData(focalPlane=Point2D(11717.18344, -63.58738901),
                                                           distEst=Point2D(-176.1960137, 4176.163218)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(11713.07268, -4238.585365),
                                                              distEst=Point2D(-202.083226, 72.99491644)),
                                     (0, 4175): CornerData(focalPlane=Point2D(13760.07169, -4240.600869),
                                                           distEst=Point2D(1726.140418, 97.2513258))}
                            ),
            '1_45': CcdData(name='1_45', id=7,
                            corners={(0, 0): CornerData(focalPlane=Point2D(13759.95779, 4412.536539),
                                                        distEst=Point2D(1723.26049, 4071.55042)),
                                     (2047, 0): CornerData(focalPlane=Point2D(11712.95971, 4409.737902),
                                                           distEst=Point2D(-204.5651278, 4097.305289)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(11718.66772, 234.7418043),
                                                              distEst=Point2D(-175.9354135, -4.373290838)),
                                     (0, 4175): CornerData(focalPlane=Point2D(13765.66581, 237.5404407),
                                                           distEst=Point2D(1757.54326, -4.803878365))}
                            ),
            '1_46': CcdData(name='1_46', id=8,
                            corners={(0, 0): CornerData(focalPlane=Point2D(13764.61796, 8888.296166),
                                                        distEst=Point2D(1624.844697, 3900.07959)),
                                     (2047, 0): CornerData(focalPlane=Point2D(11717.61873, 8890.0733),
                                                           distEst=Point2D(-284.3944961, 3958.007664)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(11713.99414, 4715.074873),
                                                              distEst=Point2D(-207.7650881, -83.62156227)),
                                     (0, 4175): CornerData(focalPlane=Point2D(13760.99337, 4713.29774),
                                                           distEst=Point2D(1719.300832, -113.5804695))}
                            ),
            '1_47': CcdData(name='1_47', id=9,
                            corners={(0, 0): CornerData(focalPlane=Point2D(13766.12638, 13362.90815),
                                                        distEst=Point2D(1446.924006, 3588.231029)),
                                     (2047, 0): CornerData(focalPlane=Point2D(11719.12862, 13365.93348),
                                                           distEst=Point2D(-429.8667239, 3682.300223)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(11712.95825, 9190.938042),
                                                              distEst=Point2D(-294.1826555, -230.0019463)),
                                     (0, 4175): CornerData(focalPlane=Point2D(13759.95601, 9187.912712),
                                                           distEst=Point2D(1613.460372, -290.6948165))}
                            ),
            '1_40': CcdData(name='1_40', id=14,
                            corners={(0, 0): CornerData(focalPlane=Point2D(11637.75535, 8889.539668),
                                                        distEst=Point2D(1764.817979, 3960.471013)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9590.756747, 8887.151076),
                                                           distEst=Point2D(-183.731535, 4005.05612)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9595.628448, 4712.153918),
                                                              distEst=Point2D(-119.4880777, -60.14123393)),
                                     (0, 4175): CornerData(focalPlane=Point2D(11642.62705, 4714.54251),
                                                           distEst=Point2D(1844.761026, -82.33657042))}
                            ),
            '1_41': CcdData(name='1_41', id=15,
                            corners={(0, 0): CornerData(focalPlane=Point2D(11640.66934, 13364.0567),
                                                        distEst=Point2D(1621.677387, 3684.815988)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9593.669659, 13365.1939),
                                                           distEst=Point2D(-297.1902242, 3759.906032)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9591.350272, 9190.19454),
                                                              distEst=Point2D(-189.0662993, -180.8375241)),
                                     (0, 4175): CornerData(focalPlane=Point2D(11638.34996, 9189.057346),
                                                           distEst=Point2D(1758.025246, -228.8826181))}
                            ),
            '1_42': CcdData(name='1_42', id=4,
                            corners={(0, 0): CornerData(focalPlane=Point2D(13759.36893, -9016.390851),
                                                        distEst=Point2D(1620.790607, 4453.130396)),
                                     (2047, 0): CornerData(focalPlane=Point2D(11712.36897, -9016.816458),
                                                           distEst=Point2D(-287.8364565, 4395.8044)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(11713.23703, -13191.81637),
                                                              distEst=Point2D(-422.5770263, 475.3935816)),
                                     (0, 4175): CornerData(focalPlane=Point2D(13760.23699, -13191.39076),
                                                           distEst=Point2D(1456.06039, 566.5498437))}
                            ),
            '1_43': CcdData(name='1_43', id=5,
                            corners={(0, 0): CornerData(focalPlane=Point2D(13762.74199, -4541.450976),
                                                        distEst=Point2D(1723.373182, 4280.890177)),
                                     (2047, 0): CornerData(focalPlane=Point2D(11715.74247, -4540.050489),
                                                           distEst=Point2D(-204.0133879, 4254.43251)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(11712.88608, -8715.049512),
                                                              distEst=Point2D(-280.9769009, 208.1964729)),
                                     (0, 4175): CornerData(focalPlane=Point2D(13759.8856, -8716.449999),
                                                           distEst=Point2D(1629.268319, 262.4641959))}
                            ),
            '1_48': CcdData(name='1_48', id=104,
                            corners={(0, 0): CornerData(focalPlane=Point2D(18007.86, -4399.42),
                                                        distEst=Point2D(1317.5775, 4351.171465)),
                                     (2047, 0): CornerData(focalPlane=Point2D(15960.86, -4399.42),
                                                           distEst=Point2D(-501.4752344, 4312.044512)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(15960.86, -8574.42),
                                                              distEst=Point2D(-612.3590234, 328.0362109)),
                                     (0, 4175): CornerData(focalPlane=Point2D(18007.86, -8574.42),
                                                           distEst=Point2D(1186.919297, 410.3653125))}
                            ),
            '0_29': CcdData(name='0_29', id=17,
                            corners={(0, 0): CornerData(focalPlane=Point2D(7465.933568, -8489.063695),
                                                        distEst=Point2D(-105.8856082, 119.8073714)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9512.933016, -8490.567225),
                                                           distEst=Point2D(1874.926772, 152.405925)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9515.999572, -4315.568352),
                                                              distEst=Point2D(1933.577053, 4226.466802)),
                                     (0, 4175): CornerData(focalPlane=Point2D(7469.000124, -4314.064821),
                                                           distEst=Point2D(-60.4778549, 4210.74571))}
                            ),
            '0_28': CcdData(name='0_28', id=18,
                            corners={(0, 0): CornerData(focalPlane=Point2D(7467.417867, -4014.87778),
                                                        distEst=Point2D(-59.57998091, 32.04431031)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9514.417707, -4015.686172),
                                                           distEst=Point2D(1935.100221, 46.94794144)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9516.06648, 159.3135023),
                                                              distEst=Point2D(1952.876272, 4173.24183)),
                                     (0, 4175): CornerData(focalPlane=Point2D(7469.066639, 160.1218946),
                                                           distEst=Point2D(-45.60200494, 4174.309862))}
                            ),
            '0_21': CcdData(name='0_21', id=32,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3220.080235, -8490.742227),
                                                        distEst=Point2D(-29.0393586, 76.61770969)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5267.080156, -8490.173076),
                                                           distEst=Point2D(1989.38752, 93.15351296)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5265.919333, -4315.173238),
                                                              distEst=Point2D(2019.373609, 4197.296845)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3218.919412, -4315.742389),
                                                           distEst=Point2D(-11.06337425, 4188.863297))}
                            ),
            '0_20': CcdData(name='0_20', id=33,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3225.1677, -4014.127361),
                                                        distEst=Point2D(-8.846307159, 11.43464494)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5272.166609, -4012.01458),
                                                           distEst=Point2D(2022.126686, 20.07915357)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5267.857445, 162.9831957),
                                                              distEst=Point2D(2029.257269, 4175.065514)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3220.858535, 160.870415),
                                                           distEst=Point2D(-5.332532073, 4174.253363))}
                            ),
            '0_23': CcdData(name='0_23', id=30,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3223.143276, -17441.66905),
                                                        distEst=Point2D(-116.5999908, 635.9896586)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5270.142771, -17440.23132),
                                                           distEst=Point2D(1843.762325, 676.6938003)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5267.210423, -13265.23235),
                                                              distEst=Point2D(1926.682581, 4477.605344)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3220.210928, -13266.67008),
                                                           distEst=Point2D(-67.16058517, 4449.723508))}
                            ),
            '0_22': CcdData(name='0_22', id=31,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3219.299287, -12966.86782),
                                                        distEst=Point2D(-64.607016, 257.4145671)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5266.29849, -12968.67411),
                                                           distEst=Point2D(1931.112336, 283.0509473)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5269.982539, -8793.675731),
                                                              distEst=Point2D(1986.895085, 4275.971455)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3222.983336, -8791.869444),
                                                           distEst=Point2D(-30.29774643, 4259.941813))}
                            ),
            '0_25': CcdData(name='0_25', id=24,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5347.661555, -8490.243894),
                                                        distEst=Point2D(-58.13902044, 93.2973653)),
                                     (2047, 0): CornerData(focalPlane=Point2D(7394.660395, -8488.064769),
                                                           distEst=Point2D(1944.986778, 119.0319403)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(7390.215915, -4313.067134),
                                                              distEst=Point2D(1986.263143, 4210.403616)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5343.217075, -4315.24626),
                                                           distEst=Point2D(-29.30628397, 4197.008674))}
                            ),
            '0_24': CcdData(name='0_24', id=25,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5346.209007, -4012.402506),
                                                        distEst=Point2D(-26.0890698, 19.56139523)),
                                     (2047, 0): CornerData(focalPlane=Point2D(7393.208574, -4011.071158),
                                                           distEst=Point2D(1990.019957, 31.67749585)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(7390.493196, 163.9279592),
                                                              distEst=Point2D(2001.717756, 4174.460349)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5343.493629, 162.5966111),
                                                           distEst=Point2D(-18.03927402, 4174.191627))}
                            ),
            '0_27': CcdData(name='0_27', id=22,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5342.082586, -17440.8728),
                                                        distEst=Point2D(-207.7530571, 678.1726161)),
                                     (2047, 0): CornerData(focalPlane=Point2D(7389.082563, -17441.18154),
                                                           distEst=Point2D(1734.405428, 739.2487662)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(7389.712254, -13266.18158),
                                                              distEst=Point2D(1855.304893, 4519.88973)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5342.712278, -13265.87285),
                                                           distEst=Point2D(-121.7920951, 4478.57657))}
                            ),
            '0_26': CcdData(name='0_26', id=23,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5347.030908, -12966.61211),
                                                        distEst=Point2D(-116.200517, 284.391544)),
                                     (2047, 0): CornerData(focalPlane=Point2D(7394.030306, -12965.04182),
                                                           distEst=Point2D(1862.974531, 325.3843667)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(7390.827576, -8790.043045),
                                                              distEst=Point2D(1938.87379, 4303.159258)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5343.828178, -8791.613342),
                                                           distEst=Point2D(-63.03344506, 4276.788437))}
                            ),
            '1_31': CcdData(name='1_31', id=103,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-8563.673972, 12603.11061),
                                                        distEst=Point2D(3334.862875, -1394.280747)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-8564.981276, 14650.11019),
                                                           distEst=Point2D(3393.709122, 497.4027068)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-12739.98042, 14647.44385),
                                                              distEst=Point2D(-496.0199849, 324.9113754)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-12738.67312, 12600.44427),
                                                           distEst=Point2D(-588.7921283, -1536.5464))}
                            ),
            '1_30': CcdData(name='1_30', id=83,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-7468.25958, 13366.06643),
                                                        distEst=Point2D(2243.388786, 3820.867)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9515.259505, 13365.51106),
                                                           distEst=Point2D(291.3199314, 3763.359344)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9514.126794, 9190.511216),
                                                              distEst=Point2D(183.9023436, -178.835105)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-7467.126869, 9191.066583),
                                                           distEst=Point2D(2161.771616, -142.0057542))}
                            ),
            '1_33': CcdData(name='1_33', id=20,
                            corners={(0, 0): CornerData(focalPlane=Point2D(9515.188, 8888.673919),
                                                        distEst=Point2D(1867.875848, 4006.824237)),
                                     (2047, 0): CornerData(focalPlane=Point2D(7468.188071, 8889.215239),
                                                           distEst=Point2D(-111.0273269, 4042.525472)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(7467.084012, 4714.215385),
                                                              distEst=Point2D(-64.46353199, -40.50209811)),
                                     (0, 4175): CornerData(focalPlane=Point2D(9514.08394, 4713.674065),
                                                           distEst=Point2D(1928.713243, -58.96707078))}
                            ),
            '1_32': CcdData(name='1_32', id=19,
                            corners={(0, 0): CornerData(focalPlane=Point2D(9514.654572, 4412.01794),
                                                        distEst=Point2D(1931.382492, 4121.566013)),
                                     (2047, 0): CornerData(focalPlane=Point2D(7467.654803, 4411.046962),
                                                           distEst=Point2D(-62.43688632, 4138.0926)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(7469.635181, 236.0474316),
                                                              distEst=Point2D(-45.55444015, -1.586889332)),
                                     (0, 4175): CornerData(focalPlane=Point2D(9516.634951, 237.0184098),
                                                           distEst=Point2D(1952.908396, -2.051574261))}
                            ),
            '1_35': CcdData(name='1_35', id=101,
                            corners={(0, 0): CornerData(focalPlane=Point2D(8568.290019, 16777.76953),
                                                        distEst=Point2D(704.3847846, 2407.335789)),
                                     (2047, 0): CornerData(focalPlane=Point2D(8568.242752, 14730.76953),
                                                           distEst=Point2D(776.5163522, 569.7096683)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(12743.24275, 14730.67313),
                                                              distEst=Point2D(4662.430463, 396.4156654)),
                                     (0, 4175): CornerData(focalPlane=Point2D(12743.29002, 16777.67313),
                                                           distEst=Point2D(4551.175221, 2203.807936))}
                            ),
            '1_34': CcdData(name='1_34', id=21,
                            corners={(0, 0): CornerData(focalPlane=Point2D(9512.143055, 13365.44516),
                                                        distEst=Point2D(1750.793741, 3763.733305)),
                                     (2047, 0): CornerData(focalPlane=Point2D(7465.147845, 13361.01681),
                                                           distEst=Point2D(-200.7961656, 3819.352125)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(7474.179778, 9186.026576),
                                                              distEst=Point2D(-113.9060195, -143.6543363)),
                                     (0, 4175): CornerData(focalPlane=Point2D(9521.174988, 9190.454928),
                                                           distEst=Point2D(1863.380382, -178.3312126))}
                            ),
            '1_37': CcdData(name='1_37', id=11,
                            corners={(0, 0): CornerData(focalPlane=Point2D(11635.12336, -4539.477965),
                                                        distEst=Point2D(1844.796124, 4254.045191)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9588.125414, -4542.377502),
                                                           distEst=Point2D(-120.0793311, 4230.658469)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9594.039222, -8717.373313),
                                                              distEst=Point2D(-177.1075813, 161.7705925)),
                                     (0, 4175): CornerData(focalPlane=Point2D(11641.03717, -8714.473777),
                                                           distEst=Point2D(1772.501195, 207.1153978))}
                            ),
            '1_36': CcdData(name='1_36', id=10,
                            corners={(0, 0): CornerData(focalPlane=Point2D(11638.13077, -9016.136908),
                                                        distEst=Point2D(1763.728823, 4393.924897)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9591.130787, -9015.901054),
                                                           distEst=Point2D(-184.3046912, 4348.410327)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9590.649745, -13190.90103),
                                                              distEst=Point2D(-291.0244977, 400.1375707)),
                                     (0, 4175): CornerData(focalPlane=Point2D(11637.64973, -13191.13688),
                                                           distEst=Point2D(1629.424472, 472.3224263))}
                            ),
            '1_39': CcdData(name='1_39', id=13,
                            corners={(0, 0): CornerData(focalPlane=Point2D(11640.03761, 4410.8137),
                                                        distEst=Point2D(1848.461555, 4098.129329)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9593.038547, 4412.767182),
                                                           distEst=Point2D(-116.6325767, 4120.973411)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9589.054284, 237.7690833),
                                                              distEst=Point2D(-97.75417147, -1.923430834)),
                                     (0, 4175): CornerData(focalPlane=Point2D(11636.05335, 235.8156016),
                                                           distEst=Point2D(1872.425906, -4.316688823))}
                            ),
            '1_38': CcdData(name='1_38', id=12,
                            corners={(0, 0): CornerData(focalPlane=Point2D(11641.33927, -61.93617774),
                                                        distEst=Point2D(1874.474769, 4175.018966)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9594.340681, -59.53034382),
                                                           distEst=Point2D(-95.63564116, 4176.202058)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9589.433814, -4234.52746),
                                                              distEst=Point2D(-117.0566584, 51.77801761)),
                                     (0, 4175): CornerData(focalPlane=Point2D(11636.4324, -4236.933294),
                                                           distEst=Point2D(1848.422627, 70.98772003))}
                            ),
            '1_57': CcdData(name='1_57', id=107,
                            corners={(0, 0): CornerData(focalPlane=Point2D(15884.71, 13359.95),
                                                        distEst=Point2D(1214.967734, 3474.880078)),
                                     (2047, 0): CornerData(focalPlane=Point2D(13837.71, 13359.95),
                                                           distEst=Point2D(-608.7207422, 3586.138867)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(13837.71, 9184.95),
                                                              distEst=Point2D(-438.3017969, -291.9490234)),
                                     (0, 4175): CornerData(focalPlane=Point2D(15884.71, 9184.95),
                                                           distEst=Point2D(1418.225547, -363.9392578))}
                            ),
            '1_56': CcdData(name='1_56', id=3,
                            corners={(0, 0): CornerData(focalPlane=Point2D(15885.1169, 8888.808422),
                                                        distEst=Point2D(1429.753482, 3828.840617)),
                                     (2047, 0): CornerData(focalPlane=Point2D(13838.11695, 8889.277102),
                                                           distEst=Point2D(-428.7232505, 3898.46282)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(13837.16104, 4714.277211),
                                                              distEst=Point2D(-332.3674044, -114.1384797)),
                                     (0, 4175): CornerData(focalPlane=Point2D(15884.16099, 4713.808531),
                                                           distEst=Point2D(1546.522433, -149.801315))}
                            ),
            '1_55': CcdData(name='1_55', id=2,
                            corners={(0, 0): CornerData(focalPlane=Point2D(15884.02337, 4412.162579),
                                                        distEst=Point2D(1551.918513, 4036.649141)),
                                     (2047, 0): CornerData(focalPlane=Point2D(13837.02349, 4411.452264),
                                                           distEst=Point2D(-327.9176282, 4069.410842)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(13838.47223, 236.4525155),
                                                              distEst=Point2D(-295.3200089, -5.87958867)),
                                     (0, 4175): CornerData(focalPlane=Point2D(15885.47211, 237.1628304),
                                                           distEst=Point2D(1591.291986, -7.826669563))}
                            ),
            '1_54': CcdData(name='1_54', id=1,
                            corners={(0, 0): CornerData(focalPlane=Point2D(15884.85432, -63.26114806),
                                                        distEst=Point2D(1591.129665, 4175.527059)),
                                     (2047, 0): CornerData(focalPlane=Point2D(13837.85432, -63.19295258),
                                                           distEst=Point2D(-295.5246343, 4175.684032)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(13837.71523, -4238.19295),
                                                              distEst=Point2D(-325.121765, 98.91942807)),
                                     (0, 4175): CornerData(focalPlane=Point2D(15884.71523, -4238.261146),
                                                           distEst=Point2D(1555.510531, 129.9370827))}
                            ),
            '1_53': CcdData(name='1_53', id=0,
                            corners={(0, 0): CornerData(focalPlane=Point2D(15884.11223, -4541.560689),
                                                        distEst=Point2D(1550.14862, 4315.968663)),
                                     (2047, 0): CornerData(focalPlane=Point2D(13837.11225, -4541.866476),
                                                           distEst=Point2D(-329.599986, 4282.404962)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(13837.73593, -8716.866429),
                                                              distEst=Point2D(-422.8394855, 265.2590832)),
                                     (0, 4175): CornerData(focalPlane=Point2D(15884.7359, -8716.560643),
                                                           distEst=Point2D(1437.697853, 333.6279298))}
                            ),
            '1_52': CcdData(name='1_52', id=106,
                            corners={(0, 0): CornerData(focalPlane=Point2D(15884.71, -8865.59),
                                                        distEst=Point2D(1432.075156, 4517.293125)),
                                     (2047, 0): CornerData(focalPlane=Point2D(13837.71, -8865.59),
                                                           distEst=Point2D(-427.5322656, 4447.728672)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(13837.71, -13040.59),
                                                              distEst=Point2D(-591.2119531, 556.6808203)),
                                     (0, 4175): CornerData(focalPlane=Point2D(15884.71, -13040.59),
                                                           distEst=Point2D(1237.459922, 668.1691016))}
                            ),
            '1_51': CcdData(name='1_51', id=105,
                            corners={(0, 0): CornerData(focalPlane=Point2D(18007.86, 8894.86),
                                                        distEst=Point2D(1166.284531, 3742.893906)),
                                     (2047, 0): CornerData(focalPlane=Point2D(15960.86, 8894.86),
                                                           distEst=Point2D(-625.844375, 3825.85582)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(15960.86, 4719.86),
                                                              distEst=Point2D(-507.7447656, -151.3121484)),
                                     (0, 4175): CornerData(focalPlane=Point2D(18007.86, 4719.86),
                                                           distEst=Point2D(1307.936875, -194.4181055))}
                            ),
            '0_54': CcdData(name='0_54', id=98,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-15888.02926, 466.2268113),
                                                        distEst=Point2D(452.851398, -13.16440691)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-13841.03149, 463.2063826),
                                                           distEst=Point2D(2340.253778, -11.19411806)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-13834.87111, 4638.201838),
                                                              distEst=Point2D(2377.815307, 4063.064195)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-15881.86888, 4641.222266),
                                                           distEst=Point2D(497.7416331, 4029.863648))}
                            ),
            '0_55': CcdData(name='0_55', id=97,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-15885.18352, -4010.822787),
                                                        distEst=Point2D(485.3694319, 121.8494959)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-13838.18352, -4010.731633),
                                                           distEst=Point2D(2367.468388, 92.48923131)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-13838.36944, 164.2683626),
                                                              distEst=Point2D(2341.253527, 4170.757425)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-15885.36943, 164.177209),
                                                           distEst=Point2D(453.7007618, 4169.384778))}
                            ),
            '0_56': CcdData(name='0_56', id=96,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-15885.04328, -8488.887806),
                                                        distEst=Point2D(599.8186276, 318.9755393)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-13838.04329, -8488.776574),
                                                           distEst=Point2D(2461.889122, 253.7774867)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-13838.27015, -4313.77658),
                                                              distEst=Point2D(2371.522813, 4275.817541)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-15885.27015, -4313.887812),
                                                           distEst=Point2D(490.3098053, 4307.458497))}
                            ),
            '0_57': CcdData(name='0_57', id=108,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-15885.76, -13040.59),
                                                        distEst=Point2D(807.8498438, 662.4435156)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-13838.76, -13040.59),
                                                           distEst=Point2D(2636.621328, 555.0450781)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-13838.76, -8865.59),
                                                              distEst=Point2D(2473.212148, 4447.266758)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-15885.76, -8865.59),
                                                           distEst=Point2D(613.5051172, 4515.789219))}
                            ),
            '0_51': CcdData(name='0_51', id=110,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-18010.05, -8574.42),
                                                        distEst=Point2D(862.2160156, 403.9590625)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-15963.05, -8574.42),
                                                           distEst=Point2D(2658.005078, 326.6065234)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-15963.05, -4399.42),
                                                              distEst=Point2D(2546.236523, 4311.889238)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-18010.05, -4399.42),
                                                           distEst=Point2D(727.5949219, 4350.195879))}
                            ),
            '0_52': CcdData(name='0_52', id=109,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-15885.76, 9184.95),
                                                        distEst=Point2D(624.4396875, -362.8142578)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-13838.76, 9184.95),
                                                           distEst=Point2D(2482.756094, -290.7205078)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-13838.76, 13359.95),
                                                              distEst=Point2D(2651.620352, 3588.419141)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-15885.76, 13359.95),
                                                           distEst=Point2D(823.6975, 3477.191602))}
                            ),
            '0_53': CcdData(name='0_53', id=99,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-15887.84211, 4943.897804),
                                                        distEst=Point2D(500.9638282, -156.4826744)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-13840.84452, 4940.753974),
                                                           distEst=Point2D(2379.876568, -120.9743953)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-13834.43246, 9115.74905),
                                                              distEst=Point2D(2482.036649, 3887.598889)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-15881.43005, 9118.89288),
                                                           distEst=Point2D(623.3476186, 3817.66277))}
                            ),
            '0_48': CcdData(name='0_48', id=111,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-18010.05, 4719.86),
                                                        distEst=Point2D(733.1496094, -192.410293)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-15963.05, 4719.86),
                                                           distEst=Point2D(2552.292187, -150.6900781)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-15963.05, 8894.86),
                                                              distEst=Point2D(2668.684766, 3826.864609)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-18010.05, 8894.86),
                                                           distEst=Point2D(870.3039062, 3744.948594))}
                            ),
            '0_47': CcdData(name='0_47', id=90,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-13761.22584, -12963.08046),
                                                        distEst=Point2D(578.9826165, 544.4212708)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-11714.22587, -12962.75853),
                                                           distEst=Point2D(2460.622142, 455.8524111)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-11714.88246, -8787.758586),
                                                              distEst=Point2D(2328.202612, 4385.272016)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-13761.88244, -8788.080514),
                                                           distEst=Point2D(417.7001489, 4440.951543))}
                            ),
            '0_46': CcdData(name='0_46', id=91,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-13760.7931, -8489.075327),
                                                        distEst=Point2D(410.2430648, 250.6980263)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-11713.79449, -8486.689765),
                                                           distEst=Point2D(2322.335836, 198.1824788)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-11718.66001, -4311.6926),
                                                              distEst=Point2D(2247.020499, 4249.097658)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-13765.65862, -4314.078162),
                                                           distEst=Point2D(318.4536276, 4274.000242))}
                            ),
            '0_45': CcdData(name='0_45', id=92,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-13761.21438, -4010.847464),
                                                        distEst=Point2D(315.2222636, 91.44717963)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-11714.21438, -4010.777604),
                                                           distEst=Point2D(2244.665211, 67.26288826)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-11714.35686, 164.2223938),
                                                              distEst=Point2D(2223.195212, 4171.842586)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-13761.35686, 164.1525334),
                                                           distEst=Point2D(289.2098422, 4170.76516))}
                            ),
            '0_44': CcdData(name='0_44', id=93,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-13764.25409, 464.5587639),
                                                        distEst=Point2D(288.8147303, -9.979419954)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-11717.25491, 462.7262176),
                                                           distEst=Point2D(2222.670516, -8.072593153)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-11713.51731, 4637.724545),
                                                              distEst=Point2D(2252.527287, 4092.781239)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-13760.51649, 4639.557091),
                                                           distEst=Point2D(325.0647927, 4065.284682))}
                            ),
            '0_43': CcdData(name='0_43', id=94,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-13760.85042, 4942.303208),
                                                        distEst=Point2D(328.6520019, -119.0418703)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-11713.85046, 4941.890505),
                                                           distEst=Point2D(2255.259325, -88.90283092)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-11713.00872, 9116.89042),
                                                              distEst=Point2D(2336.030028, 3948.845078)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-13760.00868, 9117.303123),
                                                           distEst=Point2D(427.6276987, 3890.476917))}
                            ),
            '0_42': CcdData(name='0_42', id=95,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-13760.44201, 9416.868897),
                                                        distEst=Point2D(436.8173864, -300.4283846)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-11713.44206, 9416.433451),
                                                           distEst=Point2D(2343.445723, -239.8828178)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-11712.55394, 13591.43336),
                                                              distEst=Point2D(2485.130986, 3664.929149)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-13759.55389, 13591.8688),
                                                           distEst=Point2D(608.8665588, 3570.931734))}
                            ),
            '0_41': CcdData(name='0_41', id=84,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-11636.5098, -12965.76477),
                                                        distEst=Point2D(409.8088641, 452.0563329)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9589.511643, -12963.02006),
                                                           distEst=Point2D(2332.644419, 382.3785326)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9595.109672, -8788.023813),
                                                              distEst=Point2D(2225.473116, 4339.493264)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-11642.10783, -8790.768523),
                                                           distEst=Point2D(276.063161, 4382.689851))}
                            ),
            '0_40': CcdData(name='0_40', id=85,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-11637.7243, -8488.718558),
                                                        distEst=Point2D(270.2895332, 195.7771586)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9590.724473, -8487.883806),
                                                           distEst=Point2D(2221.312181, 153.3871639)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9592.427007, -4312.884153),
                                                              distEst=Point2D(2162.608604, 4227.044238)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-11639.42684, -4313.718905),
                                                           distEst=Point2D(196.998477, 4247.506542))}
                            ),
            '1_08': CcdData(name='1_08', id=42,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3147.122857, 4412.436325),
                                                        distEst=Point2D(2035.727824, 4160.592926)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1100.123168, 4411.307363),
                                                           distEst=Point2D(-3.345716203, 4164.221628)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1102.425766, 236.3079975),
                                                              distEst=Point2D(0.1856659058, -0.3651824388)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3149.425455, 237.43696),
                                                           distEst=Point2D(2043.555589, -0.02398432877))}
                            ),
            '1_09': CcdData(name='1_09', id=43,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3144.285627, 8890.427727),
                                                        distEst=Point2D(2014.860519, 4088.356268)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1097.286501, 8888.535939),
                                                           distEst=Point2D(-11.02788908, 4095.999439)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1101.144937, 4713.537722),
                                                              distEst=Point2D(-2.146009479, -13.08908788)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3148.144062, 4715.42951),
                                                           distEst=Point2D(2036.310334, -16.63455034))}
                            ),
            '1_00': CcdData(name='1_00', id=26,
                            corners={(0, 0): CornerData(focalPlane=Point2D(7394.209937, 4410.526155),
                                                        distEst=Point2D(1986.95048, 4138.653619)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5347.21008, 4411.291039),
                                                           distEst=Point2D(-28.34689056, 4151.570558)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5345.650045, 236.2913303),
                                                              distEst=Point2D(-17.78745933, -0.5329497143)),
                                     (0, 4175): CornerData(focalPlane=Point2D(7392.649902, 235.5264462),
                                                           distEst=Point2D(2001.945671, -1.523018334))}
                            ),
            '1_01': CcdData(name='1_01', id=27,
                            corners={(0, 0): CornerData(focalPlane=Point2D(7392.19439, 8889.614339),
                                                        distEst=Point2D(1937.08627, 4043.861947)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5345.194746, 8888.407949),
                                                           distEst=Point2D(-64.23982968, 4069.257349)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5347.655262, 4713.408674),
                                                              distEst=Point2D(-29.74667024, -26.94000517)),
                                     (0, 4175): CornerData(focalPlane=Point2D(7394.654907, 4714.615064),
                                                           distEst=Point2D(1984.892707, -39.7568094))}
                            ),
            '1_02': CcdData(name='1_02', id=28,
                            corners={(0, 0): CornerData(focalPlane=Point2D(7391.436746, 13366.03577),
                                                        distEst=Point2D(1849.605099, 3822.709281)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5344.439658, 13362.58313),
                                                           distEst=Point2D(-126.5660536, 3863.191316)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5351.481569, 9187.589066),
                                                              distEst=Point2D(-65.25337758, -115.3969632)),
                                     (0, 4175): CornerData(focalPlane=Point2D(7398.478657, 9191.041711),
                                                           distEst=Point2D(1934.664461, -140.7115186))}
                            ),
            '1_03': CcdData(name='1_03', id=29,
                            corners={(0, 0): CornerData(focalPlane=Point2D(7392.905759, 17840.92502),
                                                        distEst=Point2D(1719.116873, 3383.830314)),
                                     (2047, 0): CornerData(focalPlane=Point2D(5345.90583, 17841.46487),
                                                           distEst=Point2D(-218.6461984, 3445.801354)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(5344.804759, 13666.46502),
                                                              distEst=Point2D(-130.0343445, -331.4262514)),
                                     (0, 4175): CornerData(focalPlane=Point2D(7391.804688, 13665.92516),
                                                           distEst=Point2D(1844.097876, -374.9423003))}
                            ),
            '1_04': CcdData(name='1_04', id=34,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5270.478272, 4410.689821),
                                                        distEst=Point2D(2019.447836, 4151.622841)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3223.478364, 4411.303237),
                                                           distEst=Point2D(-10.79786222, 4160.235148)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3222.227258, 236.3034244),
                                                              distEst=Point2D(-4.58367664, -0.164068746)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5269.227166, 235.690008),
                                                           distEst=Point2D(2029.992866, -0.8569789537))}
                            ),
            '1_05': CcdData(name='1_05', id=35,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5270.418189, 8887.644177),
                                                        distEst=Point2D(1985.44105, 4070.009133)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3223.418585, 8888.917934),
                                                           distEst=Point2D(-31.40000863, 4087.724413)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3220.82067, 4713.918742),
                                                              distEst=Point2D(-12.71079822, -17.02241254)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5267.820273, 4712.644986),
                                                           distEst=Point2D(2016.937767, -26.53351139))}
                            ),
            '1_06': CcdData(name='1_06', id=36,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5270.637876, 13363.44575),
                                                        distEst=Point2D(1924.631565, 3865.147042)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3223.637876, 13363.42475),
                                                           distEst=Point2D(-68.53723955, 3892.658242)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3223.680708, 9188.42475),
                                                              distEst=Point2D(-33.99495539, -96.01185687)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5270.680708, 9188.44575),
                                                           distEst=Point2D(1981.587717, -113.7331334))}
                            ),
            '1_07': CcdData(name='1_07', id=37,
                            corners={(0, 0): CornerData(focalPlane=Point2D(5270.505554, 17841.03831),
                                                        distEst=Point2D(1832.151177, 3447.401001)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3223.505696, 17841.80032),
                                                           distEst=Point2D(-124.293013, 3488.409894)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3221.951505, 13666.80061),
                                                              distEst=Point2D(-71.93642506, -301.7454651)),
                                     (0, 4175): CornerData(focalPlane=Point2D(5268.951364, 13666.03859),
                                                           distEst=Point2D(1919.218114, -330.4070773))}
                            ),
            '1_19': CcdData(name='1_19', id=61,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1099.239192, 17842.20504),
                                                        distEst=Point2D(2086.055144, 3509.988238)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3146.239094, 17841.57149),
                                                           distEst=Point2D(118.618122, 3489.566139)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3144.94692, 13666.57169),
                                                              distEst=Point2D(68.39923072, -300.8138307)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1097.947018, 13667.20524),
                                                           distEst=Point2D(2069.388737, -286.7445801))}
                            ),
            '1_18': CcdData(name='1_18', id=60,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1096.59387, 13366.28427),
                                                        distEst=Point2D(2067.968771, 3907.161913)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3143.593862, 13366.10523),
                                                           distEst=Point2D(65.0137124, 3893.689934)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3143.228709, 9191.105247),
                                                              distEst=Point2D(32.30411441, -95.28422528)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1096.228716, 9191.284282),
                                                           distEst=Point2D(2057.09881, -86.5475762))}
                            ),
            '1_13': CcdData(name='1_13', id=51,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1019.947169, 8891.04399),
                                                        distEst=Point2D(2035.64726, 4097.529405)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1027.049258, 8887.219264),
                                                           distEst=Point2D(6.73753602, 4095.803774)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1019.248462, 4712.226552),
                                                              distEst=Point2D(4.617896771, -13.40697627)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1027.747965, 4716.051278),
                                                           distEst=Point2D(2046.049191, -11.59489843))}
                            ),
            '1_12': CcdData(name='1_12', id=50,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1018.568013, 4415.736128),
                                                        distEst=Point2D(2042.560502, 4165.480853)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1028.428678, 4412.055667),
                                                           distEst=Point2D(0.5229921662, 4163.713107)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1020.922121, 237.0624158),
                                                              distEst=Point2D(2.197015628, -0.9715842508)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1026.074571, 240.7428765),
                                                           distEst=Point2D(2048.530467, 0.8419675765))}
                            ),
            '1_11': CcdData(name='1_11', id=45,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3146.311058, 17841.50181),
                                                        distEst=Point2D(1925.011025, 3489.57029)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1099.311121, 17840.99262),
                                                           distEst=Point2D(-42.45279821, 3509.678276)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1100.349658, 13665.99275),
                                                              distEst=Point2D(-23.60386155, -287.1297618)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3147.349595, 13666.50194),
                                                           distEst=Point2D(1977.435811, -300.9968066))}
                            ),
            '1_10': CcdData(name='1_10', id=44,
                            corners={(0, 0): CornerData(focalPlane=Point2D(3146.477171, 13366.55809),
                                                        distEst=Point2D(1979.737237, 3893.472499)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1099.477359, 13365.68032),
                                                           distEst=Point2D(-23.25007085, 3906.765231)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1101.267641, 9190.6807),
                                                              distEst=Point2D(-10.31398005, -86.92904063)),
                                     (0, 4175): CornerData(focalPlane=Point2D(3148.267453, 9191.558474),
                                                           distEst=Point2D(2014.353932, -95.38267094))}
                            ),
            '1_17': CcdData(name='1_17', id=59,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1097.890016, 8889.981748),
                                                        distEst=Point2D(2056.470698, 4096.525238)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3144.890012, 8890.099411),
                                                           distEst=Point2D(30.46324513, 4088.228729)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3145.129994, 4715.099418),
                                                              distEst=Point2D(11.25456805, -16.91049686)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1098.129998, 4714.981755),
                                                           distEst=Point2D(2049.85442, -12.581399))}
                            ),
            '1_16': CcdData(name='1_16', id=58,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1098.976974, 4412.437538),
                                                        distEst=Point2D(2049.39266, 4164.662861)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3145.976935, 4412.040755),
                                                           distEst=Point2D(10.18263768, 4160.354638)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3145.167669, 237.0408337),
                                                              distEst=Point2D(4.073818173, -0.4051653366)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1098.167707, 237.4376165),
                                                           distEst=Point2D(2047.565082, 0.04331345836))}
                            ),
            '1_15': CcdData(name='1_15', id=53,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1023.652848, 17841.04794),
                                                        distEst=Point2D(2007.244192, 3510.355533)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1023.347031, 17840.34382),
                                                           distEst=Point2D(36.1607701, 3510.24727)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1021.910929, 13665.34406),
                                                              distEst=Point2D(20.80491856, -286.7071762)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1025.08895, 13666.04818),
                                                           distEst=Point2D(2025.072794, -286.5675117))}
                            ),
            '1_14': CcdData(name='1_14', id=52,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1025.256261, 13366.29476),
                                                        distEst=Point2D(2026.468778, 3906.874405)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1021.74336, 13367.5411),
                                                           distEst=Point2D(20.27986559, 3907.598621)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1024.285357, 9192.541875),
                                                              distEst=Point2D(8.709311801, -86.16192292)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1022.714264, 9191.295535),
                                                           distEst=Point2D(2036.407559, -86.8717776))}
                            ),
            '0_10': CcdData(name='0_10', id=55,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3149.666409, -12964.52674),
                                                        distEst=Point2D(61.2528274, 256.5729098)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1102.667644, -12966.77599),
                                                           distEst=Point2D(2066.70427, 242.633687)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1098.080155, -8791.778508),
                                                              distEst=Point2D(2057.743429, 4250.290502)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3145.07892, -8789.529265),
                                                           distEst=Point2D(31.40602552, 4259.733874))}
                            ),
            '0_11': CcdData(name='0_11', id=54,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3148.296619, -17442.15835),
                                                        distEst=Point2D(114.5797059, 635.1874351)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1101.297129, -17443.60338),
                                                           distEst=Point2D(2085.505708, 615.1955981)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1098.349887, -13268.60442),
                                                              distEst=Point2D(2069.563133, 4435.58767)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3145.349377, -13267.15939),
                                                           distEst=Point2D(66.00511517, 4449.512325))}
                            ),
            '0_12': CcdData(name='0_12', id=49,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1022.952934, -4013.390879),
                                                        distEst=Point2D(2.43866821, 7.948652945)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1024.046999, -4012.867158),
                                                           distEst=Point2D(2045.159407, 8.09432255)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1022.978835, 162.132705),
                                                              distEst=Point2D(2046.393452, 4175.073187)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1024.021098, 161.6089845),
                                                           distEst=Point2D(0.05276763412, 4174.837924))}
                            ),
            '0_13': CcdData(name='0_13', id=48,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1024.288128, -8490.087164),
                                                        distEst=Point2D(8.263104113, 68.35460889)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1022.711857, -8490.33846),
                                                           distEst=Point2D(2038.630433, 68.10521242)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1023.224392, -4315.338492),
                                                              distEst=Point2D(2044.741522, 4184.66956)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1023.775593, -4315.087196),
                                                           distEst=Point2D(2.580491795, 4184.91628))}
                            ),
            '0_14': CcdData(name='0_14', id=47,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1026.219755, -12965.74754),
                                                        distEst=Point2D(19.81775632, 242.7552268)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1020.780181, -12965.23719),
                                                           distEst=Point2D(2028.422415, 242.9833565)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1019.739298, -8790.237323),
                                                              distEst=Point2D(2037.975526, 4250.657978)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1027.260638, -8790.747667),
                                                           distEst=Point2D(8.753755222, 4250.544723))}
                            ),
            '0_15': CcdData(name='0_15', id=46,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-1025.375797, -17441.77452),
                                                        distEst=Point2D(36.40631446, 615.105638)),
                                     (2047, 0): CornerData(focalPlane=Point2D(1021.624142, -17442.27624),
                                                           distEst=Point2D(2010.889869, 614.9491701)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(1022.64742, -13267.27636),
                                                              distEst=Point2D(2027.39774, 4435.506886)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-1024.352518, -13266.77465),
                                                           distEst=Point2D(20.66100509, 4435.718972))}
                            ),
            '0_16': CcdData(name='0_16', id=41,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1099.809485, -4011.58186),
                                                        distEst=Point2D(-1.994859453, 7.880462232)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3146.809406, -4011.014181),
                                                           distEst=Point2D(2037.777807, 11.86471484)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3145.651585, 163.9856588),
                                                              distEst=Point2D(2042.713461, 4174.925502)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1098.651664, 163.41798),
                                                           distEst=Point2D(-0.6729293873, 4174.796243))}
                            ),
            '0_17': CcdData(name='0_17', id=40,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1097.949369, -8489.307795),
                                                        distEst=Point2D(-9.207590783, 68.44901109)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3144.949267, -8489.956302),
                                                           distEst=Point2D(2018.169148, 76.1256252)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3146.271943, -4314.956511),
                                                              distEst=Point2D(2036.953503, 4188.618887)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1099.272046, -4314.308004),
                                                           distEst=Point2D(-2.255171536, 4184.958953))}
                            ),
            '0_18': CcdData(name='0_18', id=39,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1095.158516, -12964.40792),
                                                        distEst=Point2D(-21.37623946, 243.513762)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3142.15761, -12966.33347),
                                                           distEst=Point2D(1984.037803, 255.7079559)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3146.084905, -8791.335317),
                                                              distEst=Point2D(2017.675896, 4258.522071)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1099.085811, -8789.409767),
                                                           distEst=Point2D(-8.535334679, 4251.141339))}
                            ),
            '0_19': CcdData(name='0_19', id=38,
                            corners={(0, 0): CornerData(focalPlane=Point2D(1098.519416, -17441.37481),
                                                        distEst=Point2D(-38.53434064, 615.3505078)),
                                     (2047, 0): CornerData(focalPlane=Point2D(3145.519416, -17441.39557),
                                                           distEst=Point2D(1932.472395, 635.1378449)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(3145.561752, -13266.39557),
                                                              distEst=Point2D(1982.210137, 4449.297277)),
                                     (0, 4175): CornerData(focalPlane=Point2D(1098.561752, -13266.37481),
                                                           distEst=Point2D(-21.32763697, 4435.833993))}
                            ),
            '0_03': CcdData(name='0_03', id=70,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-7398.978874, -17439.35792),
                                                        distEst=Point2D(310.4384228, 737.2031088)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5351.983675, -17443.79155),
                                                           distEst=Point2D(2253.02624, 676.1017381)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5342.940972, -13268.80135),
                                                              distEst=Point2D(2171.478874, 4476.952178)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-7389.936171, -13264.36771),
                                                           distEst=Point2D(194.2296794, 4519.77479))}
                            ),
            '0_02': CcdData(name='0_02', id=71,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-7393.45656, -12965.40659),
                                                        distEst=Point2D(185.4588996, 323.5491966)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5346.45656, -12965.38023),
                                                           distEst=Point2D(2164.611476, 283.9075475)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5346.510321, -8790.380231),
                                                              distEst=Point2D(2109.2158, 4277.030951)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-7393.510321, -8790.40659),
                                                           distEst=Point2D(107.2535586, 4302.116106))}
                            ),
            '0_01': CcdData(name='0_01', id=72,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-7391.575544, -8488.531402),
                                                        distEst=Point2D(102.9826353, 117.7573181)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5344.575551, -8488.365772),
                                                           distEst=Point2D(2106.210667, 93.68408655)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5344.913364, -4313.365786),
                                                              distEst=Point2D(2074.899648, 4197.61065)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-7391.913358, -4313.531416),
                                                           distEst=Point2D(59.19992941, 4209.446593))}
                            ),
            '0_00': CcdData(name='0_00', id=73,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-7394.217511, -4014.197611),
                                                        distEst=Point2D(57.67714761, 30.81272062)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5347.217634, -4013.486789),
                                                           distEst=Point2D(2073.921411, 20.09326457)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5348.667406, 161.5129593),
                                                              distEst=Point2D(2063.725239, 4174.493932)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-7395.667282, 160.8021371),
                                                           distEst=Point2D(43.8261057, 4173.472659))}
                            ),
            '0_07': CcdData(name='0_07', id=62,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5268.081889, -17442.24336),
                                                        distEst=Point2D(205.8491036, 674.4618744)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3221.082182, -17441.14901),
                                                           distEst=Point2D(2166.106457, 635.9476503)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3223.314186, -13266.14961),
                                                              distEst=Point2D(2113.669208, 4450.080944)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5270.313893, -13267.24396),
                                                           distEst=Point2D(119.9010067, 4476.329711))}
                            ),
            '0_06': CcdData(name='0_06', id=63,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5271.210247, -12965.71517),
                                                        distEst=Point2D(115.4215087, 282.7385084)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3224.210248, -12965.77449),
                                                           distEst=Point2D(2111.168468, 256.7400825)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3224.089259, -8790.774492),
                                                              distEst=Point2D(2078.273552, 4259.655622)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5271.089258, -8790.715171),
                                                           distEst=Point2D(60.92467845, 4276.284594))}
                            ),
            '0_05': CcdData(name='0_05', id=64,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5271.15002, -8487.594968),
                                                        distEst=Point2D(57.29438632, 93.18830412)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3224.150294, -8488.654256),
                                                           distEst=Point2D(2075.849994, 76.68422062)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3221.989802, -4313.654815),
                                                              distEst=Point2D(2058.265465, 4188.974242)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5268.989528, -4312.595527),
                                                           distEst=Point2D(27.66749644, 4197.466909))}
                            ),
            '0_04': CcdData(name='0_04', id=65,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5269.796706, -4012.090085),
                                                        distEst=Point2D(25.33668774, 19.74442241)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-3222.796832, -4012.80936),
                                                           distEst=Point2D(2056.48652, 11.98506423)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-3221.32982, 162.1903825),
                                                              distEst=Point2D(2051.48688, 4174.596631)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5268.329694, 162.9096577),
                                                           distEst=Point2D(16.72343407, 4174.519524))}
                            ),
            '0_09': CcdData(name='0_09', id=56,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3146.128749, -8488.500224),
                                                        distEst=Point2D(28.57654018, 76.45892751)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1099.128751, -8488.419324),
                                                           distEst=Point2D(2056.060146, 68.42276943)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1099.293752, -4313.419327),
                                                              distEst=Point2D(2049.605969, 4184.941452)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3146.29375, -4313.500227),
                                                           distEst=Point2D(10.27865598, 4188.992005))}
                            ),
            '0_08': CcdData(name='0_08', id=57,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3148.590604, -4013.670509),
                                                        distEst=Point2D(8.208625777, 12.58988342)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-1101.592099, -4016.143902),
                                                           distEst=Point2D(2048.089481, 7.554700273)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-1096.547439, 158.85305),
                                                              distEst=Point2D(2048.620693, 4174.348964)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3143.545945, 161.3264434),
                                                           distEst=Point2D(5.119679773, 4175.408723))}
                            ),
            '0_38': CcdData(name='0_38', id=87,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-11642.82051, 462.88393),
                                                        distEst=Point2D(171.8752557, -6.839517614)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9595.82218, 460.2733025),
                                                           distEst=Point2D(2142.101573, -5.827025796)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9590.497622, 4635.269907),
                                                              distEst=Point2D(2166.957346, 4116.367488)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-11637.49596, 4637.880535),
                                                           distEst=Point2D(202.2499515, 4094.870993))}
                            ),
            '0_39': CcdData(name='0_39', id=86,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-11639.2297, -4009.970728),
                                                        distEst=Point2D(193.9815758, 66.47112353)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9592.229699, -4010.087805),
                                                           distEst=Point2D(2160.267289, 47.03663117)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9591.990912, 164.912188),
                                                              distEst=Point2D(2143.194227, 4172.728054)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-11638.99091, 165.029265),
                                                           distEst=Point2D(172.851477, 4171.885194))}
                            ),
            '0_36': CcdData(name='0_36', id=89,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-11638.49884, 9416.504803),
                                                        distEst=Point2D(291.8448591, -237.5027538)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9591.499074, 9415.526307),
                                                           distEst=Point2D(2237.978935, -189.4389797)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9589.503364, 13590.52583),
                                                              distEst=Point2D(2350.568194, 3743.66812)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-11636.50313, 13591.50433),
                                                           distEst=Point2D(432.957204, 3668.479327))}
                            ),
            '0_37': CcdData(name='0_37', id=88,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-11640.55, 4941.09659),
                                                        distEst=Point2D(203.951273, -87.26860698)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9593.55085, 4939.232826),
                                                           distEst=Point2D(2167.853904, -63.8550205)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9589.749572, 9114.231095),
                                                              distEst=Point2D(2233.192563, 3997.37827)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-11636.74872, 9116.094859),
                                                           distEst=Point2D(285.4573852, 3951.504268))}
                            ),
            '0_34': CcdData(name='0_34', id=78,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-9512.728285, -12968.33478),
                                                        distEst=Point2D(281.8830556, 379.0058211)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-7465.73029, -12965.46954),
                                                           distEst=Point2D(2236.907332, 325.7986917)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-7471.574145, -8790.473627),
                                                              distEst=Point2D(2154.994166, 4303.891174)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-9518.572139, -8793.338866),
                                                           distEst=Point2D(175.4859606, 4336.813676))}
                            ),
            '0_35': CcdData(name='0_35', id=102,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-8566.708974, -16375.56816),
                                                        distEst=Point2D(3455.53323, -410.4006279)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-8568.828858, -14328.56926),
                                                           distEst=Point2D(3385.294667, 1439.214115)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-12743.82662, -14332.89291),
                                                              distEst=Point2D(-510.4335401, 1603.012605)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-12741.70674, -16379.89181),
                                                           distEst=Point2D(-404.7767614, -215.9593408))}
                            ),
            '0_32': CcdData(name='0_32', id=80,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-9515.339295, -4011.46753),
                                                        distEst=Point2D(110.6566432, 46.56578884)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-7468.339345, -4011.91726),
                                                           distEst=Point2D(2105.522097, 31.33839355)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-7467.422089, 163.0826395),
                                                              distEst=Point2D(2092.703306, 4173.510304)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-9514.422039, 163.5323695),
                                                           distEst=Point2D(94.04124907, 4172.923699))}
                            ),
            '0_33': CcdData(name='0_33', id=79,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-9514.177418, -8487.788926),
                                                        distEst=Point2D(171.6399183, 151.3931518)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-7467.177894, -8486.391762),
                                                           distEst=Point2D(2152.599717, 119.101871)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-7470.027509, -4311.392735),
                                                              distEst=Point2D(2107.129232, 4210.326123)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-9517.027032, -4312.789899),
                                                           distEst=Point2D(112.9034362, 4225.790387))}
                            ),
            '0_30': CcdData(name='0_30', id=16,
                            corners={(0, 0): CornerData(focalPlane=Point2D(7465.218048, -12968.74997),
                                                        distEst=Point2D(-188.5258925, 327.0726469)),
                                     (2047, 0): CornerData(focalPlane=Point2D(9512.217648, -12970.02887),
                                                           distEst=Point2D(1766.344963, 381.2848829)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(9514.826057, -8795.029685),
                                                              distEst=Point2D(1870.551245, 4338.291261)),
                                     (0, 4175): CornerData(focalPlane=Point2D(7467.826456, -8793.750784),
                                                           distEst=Point2D(-108.889393, 4304.259827))}
                            ),
            '0_31': CcdData(name='0_31', id=100,
                            corners={(0, 0): CornerData(focalPlane=Point2D(8565.781569, -12201.66814),
                                                        distEst=Point2D(850.5081763, 3416.166062)),
                                     (2047, 0): CornerData(focalPlane=Point2D(8564.977078, -14248.66798),
                                                           distEst=Point2D(793.0516825, 1515.015087)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(12739.97676, -14250.3088),
                                                              distEst=Point2D(4688.262654, 1679.2229)),
                                     (0, 4175): CornerData(focalPlane=Point2D(12740.78125, -12203.30896),
                                                           distEst=Point2D(4779.192911, 3549.894125))}
                            ),
            '1_26': CcdData(name='1_26', id=76,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5342.664597, 13366.15742),
                                                        distEst=Point2D(2171.041875, 3864.032943)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-7389.663827, 13367.93288),
                                                           distEst=Point2D(194.6947183, 3823.045965)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-7393.285006, 9192.934454),
                                                              distEst=Point2D(111.6479997, -140.5617116)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5346.285776, 9191.158992),
                                                           distEst=Point2D(2111.969932, -114.5887524))}
                            ),
            '1_27': CcdData(name='1_27', id=77,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5344.118928, 17842.34694),
                                                        distEst=Point2D(2263.081072, 3445.555576)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-7391.118921, 17842.17767),
                                                           distEst=Point2D(324.6365729, 3382.598612)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-7390.773689, 13667.17769),
                                                              distEst=Point2D(201.0574307, -373.9285084)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5343.773696, 13667.34696),
                                                           distEst=Point2D(2175.29066, -330.8314223))}
                            ),
            '1_24': CcdData(name='1_24', id=74,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5344.973785, 4414.07769),
                                                        distEst=Point2D(2074.866056, 4151.636117)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-7391.973694, 4413.467282),
                                                           distEst=Point2D(59.27231394, 4138.740349)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-7390.728723, 238.4674676),
                                                              distEst=Point2D(44.43374997, -1.968482926)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5343.728814, 239.0778758),
                                                           distEst=Point2D(2064.368032, -0.7809125484))}
                            ),
            '1_25': CcdData(name='1_25', id=75,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-5344.753101, 8891.179182),
                                                        distEst=Point2D(2109.538594, 4069.965537)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-7391.753095, 8891.020262),
                                                           distEst=Point2D(107.8491098, 4044.034449)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-7391.428965, 4716.020275),
                                                              distEst=Point2D(61.87172876, -39.99550723)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-5344.428972, 4716.179195),
                                                           distEst=Point2D(2076.841494, -26.46679045))}
                            ),
            '1_22': CcdData(name='1_22', id=68,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3221.964693, 13365.24263),
                                                        distEst=Point2D(2113.568208, 3893.165552)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5268.964591, 13364.59895),
                                                           distEst=Point2D(120.4012493, 3865.584399)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5267.651752, 9189.599156),
                                                              distEst=Point2D(64.6672424, -113.4449735)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3220.651853, 9190.242841),
                                                           distEst=Point2D(2080.496008, -95.5519535))}
                            ),
            '1_23': CcdData(name='1_23', id=69,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3218.029913, 17840.69773),
                                                        distEst=Point2D(2170.058768, 3488.427914)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5265.029286, 17842.29969),
                                                           distEst=Point2D(213.5009469, 3448.011931)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5268.296591, 13667.30097),
                                                              distEst=Point2D(125.2015617, -329.1098236)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3221.297218, 13665.69901),
                                                           distEst=Point2D(2116.274255, -301.7369766))}
                            ),
            '1_20': CcdData(name='1_20', id=66,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3221.454857, 4412.866117),
                                                        distEst=Point2D(2057.755778, 4160.241627)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5268.454855, 4412.777958),
                                                           distEst=Point2D(27.2717105, 4151.868359)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5268.275047, 237.7779615),
                                                              distEst=Point2D(16.41470671, -0.9219355037)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3221.275049, 237.8661212),
                                                           distEst=Point2D(2051.173021, -0.2999802449))}
                            ),
            '1_21': CcdData(name='1_21', id=67,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-3220.142285, 8890.754812),
                                                        distEst=Point2D(2078.618648, 4087.573025)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-5267.142199, 8891.34825),
                                                           distEst=Point2D(61.55805697, 4070.890997)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-5268.352557, 4716.348425),
                                                              distEst=Point2D(28.58891656, -25.93950856)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-3221.352643, 4715.754987),
                                                           distEst=Point2D(2058.454879, -17.33960908))}
                            ),
            '1_28': CcdData(name='1_28', id=81,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-7469.062762, 4415.846645),
                                                        distEst=Point2D(2108.16038, 4138.312255)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9516.062762, 4415.864451),
                                                           distEst=Point2D(114.0964889, 4121.336785)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9516.099079, 240.8644516),
                                                              distEst=Point2D(93.88102851, -2.963229373)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-7469.099079, 240.8466453),
                                                           distEst=Point2D(2092.516278, -1.866691739))}
                            ),
            '1_29': CcdData(name='1_29', id=82,
                            corners={(0, 0): CornerData(focalPlane=Point2D(-7470.436585, 8890.447852),
                                                        distEst=Point2D(2156.331508, 4043.223084)),
                                     (2047, 0): CornerData(focalPlane=Point2D(-9517.436153, 8889.116647),
                                                           distEst=Point2D(177.0672917, 4007.33248)),
                                     (2047, 4175): CornerData(focalPlane=Point2D(-9514.721066, 4714.11753),
                                                              distEst=Point2D(117.5605012, -59.02485019)),
                                     (0, 4175): CornerData(focalPlane=Point2D(-7467.721499, 4715.448735),
                                                           distEst=Point2D(2111.006174, -40.2208897))}
                            ),
            }

################################################################################


class TestMemory(lsst.utils.tests.MemoryTestCase):
    def setUp(self):
        HscMapper.clearCache()
        lsst.utils.tests.MemoryTestCase.setUp(self)


def setup_module(module):
    lsst.utils.tests.init()

if __name__ == "__main__":
    lsst.utils.tests.init()
    unittest.main()
