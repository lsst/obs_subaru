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

"""Gen3 Butler Formatters for HSC raw data.
"""

from lsst.afw.image import ImageU, bboxFromMetadata, Filter
from lsst.afw.geom import makeSkyWcs, Point2D, makeFlippedWcs
from lsst.afw.math import flipImage
from lsst.obs.base.fitsRawFormatterBase import FitsRawFormatterBase
from astro_metadata_translator import HscTranslator, ObservationInfo

from ....hsc.makeHscRawVisitInfo import MakeHscRawVisitInfo
from ....hsc.hscFilters import HSC_FILTER_DEFINITIONS

__all__ = ("HyperSuprimeCamRawFormatter", "HyperSuprimeCamCornerRawFormatter")


class HyperSuprimeCamRawFormatter(FitsRawFormatterBase):

    FLIP_LR = True
    FLIP_TB = False

    def makeVisitInfo(self, metadata):
        maker = MakeHscRawVisitInfo()
        return maker(metadata)

    def makeWcs(self, metadata):
        wcs = makeSkyWcs(metadata, strip=True)
        dimensions = bboxFromMetadata(metadata).getDimensions()
        center = Point2D(dimensions/2.0)
        return makeFlippedWcs(wcs, self.FLIP_LR, self.FLIP_TB, center)

    def makeFilter(self, metadata):
        obsInfo = ObservationInfo(metadata, translator_class=HscTranslator)
        # For historical reasons we need to return a short, lowercase filter
        # name that is neither a physical_filter nor an abstract_filter in Gen3
        # or a filter data ID value in Gen2.
        # We'll suck that out of the definitions used to construct filters
        # for HSC in Gen2.  This should all get cleaned up in RFC-541.
        for d in HSC_FILTER_DEFINITIONS:
            if obsInfo.physical_filter == d["name"] or obsInfo.physical_filter in d["alias"]:
                return Filter(d["name"], force=True)
        return Filter(obsInfo.physical_filter, force=True)

    def readImage(self, fileDescriptor):
        if fileDescriptor.parameters:
            # It looks like the Gen2 std_raw code wouldn't have handled
            # flipping vs. subimages correctly, so we won't bother to either.
            # But we'll make sure no one tries to get a subimage, rather than
            # doing something confusing.
            raise NotImplementedError("Formatter does not support subimages.")
        image = ImageU(fileDescriptor.location.path)
        return flipImage(image, self.FLIP_LR, self.FLIP_TB)


class HyperSuprimeCamCornerRawFormatter(HyperSuprimeCamRawFormatter):

    FLIP_LR = False
    FLIP_TB = True
