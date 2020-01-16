#!/usr/bin/env python
# This file is part of obs_subaru.
#
# LSST Data Management System
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
# See COPYRIGHT file at the top of the source tree.
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

from lsst.pipe.tasks.makeCoaddTempExp import MakeCoaddTempExpTask, MissingExposureError
import lsst.daf.persistence as dafPersist
import numpy as np

__all__ = ["SubaruMakeCoaddTempExpTask"]


class SubaruMakeCoaddTempExpTask(MakeCoaddTempExpTask):
    """Subaru override for use specifically in the S19A processing.

    The defect-mask for CCD=33 was underestimated.
    Because reading in calexps are the next time that calexps are
    touched by the pipeline, it is most cost-effective to update the
    masks here. This is a hack.

    TODO: DM-20074 Remove this class and module after S19A complete.

    """

    def getCalibratedExposure(self, dataRef, bgSubtracted):
        """Return one calibrated Exposure, possibly with an updated SkyWcs.

        @param[in] dataRef        a sensor-level data reference
        @param[in] bgSubtracted   return calexp with background subtracted? If False get the
                                  calexp's background background model and add it to the calexp.
        @return calibrated exposure

        @raises MissingExposureError If data for the exposure is not available.

        This was copy-pasted from MakeCoaddTempExpTask, and extra step added to check
        whether an additional mask is needed.

        If config.doApplyExternalPhotoCalib is `True`, the photometric calibration
        (`photoCalib`) is taken from `config.externalPhotoCalibName` via the
        `name_photoCalib` dataset.  Otherwise, the photometric calibration is
        retrieved from the processed exposure.  When
        `config.doApplyExternalSkyWcs` is `True`,
        the astrometric calibration is taken from `config.externalSkyWcsName`
        with the `name_wcs` dataset.  Otherwise, the astrometric calibration
        is taken from the processed exposure.
        """
        try:
            exposure = dataRef.get(self.calexpType, immediate=True)
        except dafPersist.NoResults as e:
            raise MissingExposureError('Exposure not found: %s ' % str(e)) from e

        if dataRef.dataId['ccd'] == 33:
            self.maskMidCcdAmpBoundary(exposure)
        elif dataRef.dataId['ccd'] == 9:
            self.maskWholeCcd(exposure)

        if not bgSubtracted:
            background = dataRef.get("calexpBackground", immediate=True)
            mi = exposure.getMaskedImage()
            mi += background.getImage()
            del mi

        if self.config.doApplyExternalPhotoCalib:
            photoCalib = dataRef.get(f"{self.config.externalPhotoCalibName}_photoCalib")
            exposure.setPhotoCalib(photoCalib)
        else:
            photoCalib = exposure.getPhotoCalib()

        if self.config.doApplyExternalSkyWcs:
            skyWcs = dataRef.get(f"{self.config.externalSkyWcsName}_wcs")
            exposure.setWcs(skyWcs)

        exposure.maskedImage = photoCalib.calibrateImage(exposure.maskedImage,
                                                         includeScaleUncertainty=self.config.includeCalibVar)
        exposure.maskedImage /= photoCalib.getCalibrationMean()
        # TODO: The images will have a calibration of 1.0 everywhere once RFC-545 is implemented.
        # exposure.setCalib(afwImage.Calib(1.0))
        return exposure

    def maskMidCcdAmpBoundary(self, exposure):
        """Modify two defect masks that were insuffient to cover bad amp in ccd=33
        """
        mask = exposure.mask
        badPlane = mask.getPlaneBitMask('BAD')

        # expand mask by a further 3 pixels
        mask[1023:1030, 0:4176] |= badPlane
        mask[0:2048, 4151:4176] |= badPlane

        # mask another long edge
        mask[2022:2030, 0:4176] |= badPlane

        # mask around mid bottom
        mask[1049:1057, 0:25] |= badPlane
        mask[1025:1057, 21:29] |= badPlane

        # mask around right corner
        mask[1993:2001, 4035:4157] |= badPlane
        mask[1993:2027, 4030:4038] |= badPlane

    def maskWholeCcd(self, exposure):
        """Mask entire CCD (intended for use with ccd==9)
        """
        exposure.image.set(np.nan)
        badPlane = exposure.mask.getPlaneBitMask('BAD')
        exposure.mask |= badPlane
