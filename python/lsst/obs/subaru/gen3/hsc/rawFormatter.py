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

from lsst.afw.image import ImageU, bboxFromMetadata
from lsst.afw.geom import makeSkyWcs, Point2D, makeFlippedWcs
from lsst.afw.math import flipImage
from lsst.daf.butler.formatters.fitsRawFormatterBase import FitsRawFormatterBase

from ....hsc.makeHscRawVisitInfo import MakeHscRawVisitInfo

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
