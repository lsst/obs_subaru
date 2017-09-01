#
# LSST Data Management System
# Copyright 2017 LSST Corporation.
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
# see <http://www.lsstcorp.org/LegalNotices/>.
#

from __future__ import absolute_import, division, print_function

from lsst.afw.cameraGeom import PupilFactory
from lsst.afw.geom import Angle, degrees
import numpy as np


class HscPupilFactory(PupilFactory):
    """!Pupil obscuration function factory for HSC.
    """
    def __init__(self, visitInfo, pupilSize, npix):
        """!Construct a PupilFactory.

        @param[in] visitInfo  VisitInfo object for a particular exposure.
        @param[in] pupilSize  Size in meters of constructed Pupils.
        @param[in] npix       Constructed Pupils will be npix x npix.
        """
        PupilFactory.__init__(self, visitInfo, pupilSize, npix)

        hra = self._horizonRotAngle()
        hraRad = hra.asRadians()
        rot = np.array([[ np.cos(hraRad), np.sin(hraRad)],
                        [-np.sin(hraRad), np.cos(hraRad)]])

        # Compute spider shadow parameters accounting for rotation angle.
        # Location where pairs of struts meet near prime focus.
        unrotStartPos = [np.array([0.43, 0.43]),
                         np.array([0.43, 0.43]),
                         np.array([-0.43, -0.43]),
                         np.array([-0.43, -0.43])]
        # Half angle between pair of struts that meet at Subaru prime focus
        # ring.
        strutAngle = 51.75*degrees
        alpha = strutAngle - 45.0*degrees
        unrotAngles = [90*degrees + alpha,
                       -alpha,
                       180*degrees - alpha,
                       270*degrees + alpha]
        # Apply rotation and save the results
        self._spiderStartPos = []
        self._spiderAngles = []
        for pos, angle in zip(unrotStartPos, unrotAngles):
            self._spiderStartPos.append(np.dot(rot, pos))
            self._spiderAngles.append(angle - hra)

    telescopeDiameter = 8.2  # meters

    def _horizonRotAngle(self):
        """!Compute rotation angle of camera with respect to horizontal
        coordinates from self.visitInfo.

        @returns horizon rotation angle.
        """
        observatory = self.visitInfo.getObservatory()
        lat = observatory.getLatitude()
        lon = observatory.getLongitude()
        radec = self.visitInfo.getBoresightRaDec()
        ra = radec.getRa()
        dec = radec.getDec()
        era = self.visitInfo.getEra()
        ha = (era + lon - ra).wrap()
        alt = self.visitInfo.getBoresightAzAlt().getLatitude()

        # parallactic angle
        sinParAng = (np.cos(lat.asRadians()) * np.sin(ha.asRadians())
                / np.cos(alt.asRadians()))
        cosParAng = np.sqrt(1 - sinParAng*sinParAng)
        if dec > lat:
            cosParAng = -cosParAng
        parAng = Angle(np.arctan2(sinParAng, cosParAng))

        bra = self.visitInfo.getBoresightRotAngle()
        return (bra - parAng).wrap()

    def getPupil(self, point):
        """!Calculate a Pupil at a given point in the focal plane.

        @param point  Point2D indicating focal plane coordinates.
        @returns      Pupil
        """
        subaruRadius = self.telescopeDiameter/2

        hscFrac = 0.231  # linear fraction
        # radius of HSC camera shadow in meters
        hscRadius = hscFrac * subaruRadius

        subaruStrutThick = 0.22 # meters

        # See DM-8589 for more detailed description of following parameters
        # d(lensCenter)/d(theta) in meters per degree
        lensRate = 0.0276 * 3600 / 128.9 * subaruRadius
        # d(cameraCenter)/d(theta) in meters per degree
        hscRate = 0.00558 * 3600 / 128.9 * subaruRadius
        # Projected radius of lens obstruction in meters
        lensRadius = subaruRadius * 138./128.98

        # Focal plane location in degrees
        hscPlateScale = 0.168  # arcsec/pixel
        thetaX = point.getX() * hscPlateScale / 3600
        thetaY = point.getY() * hscPlateScale / 3600

        pupil = self._fullPupil()
        # Cut out primary mirror exterior
        self._cutCircleExterior(pupil, (0.0, 0.0), subaruRadius)
        # Cut out camera shadow
        camX = thetaX * hscRate
        camY = thetaY * hscRate
        self._cutCircleInterior(pupil, (camX, camY), hscRadius)
        # Cut outer edge where L1 is too small
        lensX = thetaX * lensRate
        lensY = thetaY * lensRate
        self._cutCircleExterior(pupil, (lensX, lensY), lensRadius)
        # Cut out spider shadow
        for pos, angle in zip(self._spiderStartPos, self._spiderAngles):
            x = pos[0] + camX
            y = pos[1] + camY
            self._cutRay(pupil, (x, y), angle, subaruStrutThick)
        return pupil
