# This file is part of ip_isr.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (https://www.lsst.org).
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# import os

import lsst.afw.image as afwImage
import lsst.geom as geom
import lsst.pex.config as pexConfig

from lsst.ip.isr.masking import MaskingTask, MaskingConfig


class SuprimeCamMaskingConfig(MaskingConfig):
    suprimeMaskLimitSetA = pexConfig.Field(
        dtype=int,
        doc="CCD numbers for devices that have the 'near' limit.",
        default=[1, 2, 7],
    )
    suprimeMaskLimitSetB = pexConfig.Field(
        dtype=int,
        doc="CCD numbers for devices that have the 'far' limit.",
        default=[0, 6],
    )


class SuprimeCamMaskingTask(MaskingTask):
    """Perform extra masking for detector issues such as ghosts and glints.
    """
    ConfigClass = SuprimeCamMaskingConfig

    def run(self, exposure):
        """Mask defects and trim guider shadow for SuprimeCam

        Parameters
        ----------
        exposure : `lsst.afw.image.Exposure`
            Exposure to construct detector-specific masks for.

        """
        assert exposure, "No exposure provided"

        ccd = exposure.getDetector()  # This is Suprime-Cam so we know the Detector is a Ccd
        ccdNum = ccd.getId().getSerial()
        if ccdNum not in (self.config.suprimeMaskLimitSetA + self.config.suprimeMaskLimitSetB):
            # No need to mask
            return

        md = exposure.getMetadata()
        if not md.exists("S_AG-X"):
            self.log.warn("No autoguider position in exposure metadata.")
            return

        xGuider = md.getScalar("S_AG-X")
        if ccdNum in self.config.suprimeMaskLimitSetA:
            maskLimit = int(60.0 * xGuider - 2300.0)  # From SDFRED
        elif ccdNum in self.config.suprimeMaskLimitSetB:
            maskLimit = int(60.0 * xGuider - 2000.0)  # From SDFRED

        mi = exposure.getMaskedImage()
        height = mi.getHeight()
        if height < maskLimit:
            # Nothing to mask!
            return

        # TODO DM-16806: Ensure GUIDER mask is respected downstream.
        if False:
            # XXX This mask plane isn't respected by background subtraction or source detection or measurement
            self.log.info("Masking autoguider shadow at y > %d" % maskLimit)
            mask = mi.getMask()
            bbox = geom.Box2I(geom.Point2I(0, maskLimit - 1),
                              geom.Point2I(mask.getWidth() - 1, height - 1))
            badMask = mask.Factory(mask, bbox, afwImage.LOCAL)

            mask.addMaskPlane("GUIDER")
            badBitmask = mask.getPlaneBitMask("GUIDER")

            badMask |= badBitmask
        else:
            # XXX Temporary solution until a mask plane is respected by downstream processes
            self.log.info("Removing pixels affected by autoguider shadow at y > %d" % maskLimit)
            bbox = geom.Box2I(geom.Point2I(0, 0), geom.Extent2I(mi.getWidth(), maskLimit))
            good = mi.Factory(mi, bbox, afwImage.LOCAL)
            exposure.setMaskedImage(good)
