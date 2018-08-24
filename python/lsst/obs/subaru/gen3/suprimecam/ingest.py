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

from ....suprimecam.makeSuprimecamRawVisitInfo import MakeSuprimecamRawVisitInfo
from ..ingest import SubaruRawIngestTask
from .rawFormatter import SuprimeCamRawFormatter

__all__ = ("SuprimeCamRawIngestTask", )


class SuprimeCamRawIngestTask(SubaruRawIngestTask):
    """Suprime-Cam Gen3 raw data ingest specialization.
    """

    DAY0 = 53005  # 2004-01-01

    @staticmethod
    def getExposureId(header):
        expId = header.getScalar("EXP-ID").strip()
        m = re.search(r"^SUP[A-Z](\d{7})0$", expId)
        if not m:
            raise RuntimeError("Unable to interpret EXP-ID: %s" % expId)
        exposure = int(m.group(1))
        if int(exposure) == 0:
            # Don't believe it
            frameId = header.getScalar("FRAMEID").strip()
            m = re.search(r"^SUP[A-Z](\d{7})\d{1}$", frameId)
            if not m:
                raise RuntimeError("Unable to interpret FRAMEID: %s" % frameId)
            exposure = int(m.group(1))
        return exposure

    def makeVisitInfo(self, headers, exposureId):
        maker = MakeSuprimecamRawVisitInfo(self.log)
        return maker(headers[0], exposureId)

    def getFormatter(self, file, headers):
        return SuprimeCamRawFormatter()
