# This file is part of obs_subaru.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (http://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
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
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Gen3 Butler specializations for Hyper Suprime-Cam.
"""

import re

from ....hsc.makeHscRawVisitInfo import MakeHscRawVisitInfo
from ..ingest import SubaruRawIngestTask
from .rawFormatter import HyperSuprimeCamRawFormatter, HyperSuprimeCamCornerRawFormatter

__all__ = ("HyperSuprimeCamRawIngestTask", )


class HyperSuprimeCamRawIngestTask(SubaruRawIngestTask):
    """Hyper Suprime-Cam Gen3 raw data ingest specialization.
    """

    DAY0 = 55927  # Zero point for  2012-01-01  51544 -> 2000-01-01

    @staticmethod
    def getExposureId(header):
        expId = header.getScalar("EXP-ID").strip()
        m = re.search(r"^HSCE(\d{8})$", expId)  # 2016-06-14 and new scheme
        if m:
            return int(m.group(1))

        # Fallback to old scheme
        m = re.search(r"^HSC([A-Z])(\d{6})00$", expId)
        if not m:
            raise RuntimeError("Unable to interpret EXP-ID: %s" % expId)
        letter, exposure = m.groups()
        exposure = int(exposure)
        if exposure == 0:
            # Don't believe it
            frameId = header.getScalar("FRAMEID").strip()
            m = re.search(r"^HSC([A-Z])(\d{6})\d{2}$", frameId)
            if not m:
                raise RuntimeError("Unable to interpret FRAMEID: %s" % frameId)
            letter, exposure = m.groups()
            exposure = int(exposure)
            if exposure % 2:  # Odd?
                exposure -= 1
        return exposure + 1000000*(ord(letter) - ord("A"))

    # CCD index mapping for commissioning run 2
    CCD_MAP_COMMISSIONING_2 = {112: 106,
                               107: 105,
                               113: 107,
                               115: 109,
                               108: 110,
                               114: 108,
                               }

    @classmethod
    def getSensorId(cls, header):
        # Focus CCDs were numbered incorrectly in the readout software during
        # commissioning run 2.  We need to map to the correct ones.
        ccd = super(HyperSuprimeCamRawIngestTask, cls).getSensorId(header)
        try:
            tjd = cls.getTruncatedModifiedJulianDate(header)
        except Exception:
            return ccd

        if tjd > 390 and tjd < 405:
            ccd = cls.CCD_MAP_COMMISSIONING_2.get(ccd, ccd)

        return ccd

    def makeVisitInfo(self, headers, exposureId):
        maker = MakeHscRawVisitInfo(self.log)
        return maker(headers[0], exposureId)

    def getFormatter(self, file, headers, dataId):
        if dataId["sensor"] in (100, 101, 102, 103):
            return HyperSuprimeCamCornerRawFormatter()
        else:
            return HyperSuprimeCamRawFormatter()
