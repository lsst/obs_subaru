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

from abc import abstractmethod

from lsst.obs.base.gen3 import VisitInfoRawIngestTask


__all__ = ("SubaruRawIngestTask", )


class SubaruRawIngestTask(VisitInfoRawIngestTask):
    """Intermediate base class for Subaru Gen3 raw data ingest.
    """

    @classmethod
    def getTruncatedModifiedJulianDate(cls, header):
        return int(header.getScalar('MJD')) - cls.DAY0

    @staticmethod
    def getSensorId(header):
        return int(header.getScalar("DET-ID"))

    @staticmethod
    def getFilterName(header):
        # Want upper-case filter names
        try:
            return header.getScalar('FILTER01').strip().upper()
        except Exception:
            return None

    @staticmethod
    @abstractmethod
    def getExposureId(header):
        raise NotImplementedError("Must be implemented by subclasses.")

    def extractDataId(self, file, headers):
        exposureId = self.getExposureId(headers[0])
        return {
            "camera": "HSC",
            "exposure": exposureId,
            "visit": exposureId if headers[0].getScalar("DATA-TYP") == "OBJECT" else None,
            "sensor": self.getSensorId(headers[0]),
            "physical_filter": self.getFilterName(headers[0]),
        }
