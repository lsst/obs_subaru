#
# LSST Data Management System
# Copyright 2016 LSST Corporation.
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

import astropy.units

from lsst.afw.geom import degrees, SpherePoint
from lsst.afw.coord import Observatory, Weather
from lsst.afw.image import RotType
from lsst.obs.base import MakeRawVisitInfo

__all__ = ["MakeHscRawVisitInfo"]


class MakeHscRawVisitInfo(MakeRawVisitInfo):
    """Make a VisitInfo from the FITS header of a Subaru HSC image
    """
    observatory = Observatory(-155.476667*degrees, 19.825556*degrees, 4139)  # long, lat, elev

    def setArgDict(self, md, argDict):
        """Set an argument dict for VisitInfo and pop associated metadata

        @param[in,out] md  metadata, as an lsst.daf.base.PropertyList or PropertySet
        @param[in,out] argdict  a dict of arguments
        """
        MakeRawVisitInfo.setArgDict(self, md, argDict)
        argDict["boresightRaDec"] = SpherePoint(
            self.popAngle(md, "RA2000", units=astropy.units.h),
            self.popAngle(md, "DEC2000"),
        )
        altitude = self.popAngle(md, "ALTITUDE")
        if altitude > 90*degrees:  # Sometimes during day observations, when not tracking
            if self.log is not None:
                self.log.warn("Clipping altitude (%f) at 90 degrees", altitude)
            altitude = 90*degrees
        argDict["boresightAzAlt"] = SpherePoint(
            self.popAngle(md, "AZIMUTH"),
            altitude,
        )
        argDict["boresightAirmass"] = self.popFloat(md, "AIRMASS")
        argDict["observatory"] = self.observatory
        argDict["weather"] = Weather(
            self.centigradeFromKelvin(self.popFloat(md, "OUT-TMP")),
            self.pascalFromMmHg(self.popFloat(md, "OUT-PRS")),
            self.popFloat(md, "OUT-HUM"),
        )
        LST = self.popAngle(md, "LST-STR", units=astropy.units.h)
        argDict['era'] = self.eraFromLstAndLongitude(LST, self.observatory.getLongitude())
        argDict['darkTime'] = argDict['exposureTime']

        # Rotation angle formula determined empirically from visual inspection
        # of HSC images.  See DM-9111.
        rotAngle = (270.0*degrees - self.popAngle(md, "INST-PA")).wrap()
        argDict['boresightRotAngle'] = rotAngle
        argDict['rotType'] = RotType.SKY

    def getDateAvg(self, md, exposureTime):
        """Return date at the middle of the exposure

        @param[in,out] md  FITS metadata; changed in place
        @param[in] exposureTime  exposure time in sec
        """
        startDate = self.popMjdDate(md, "MJD-STR")
        return self.offsetDate(startDate, 0.5*exposureTime)
