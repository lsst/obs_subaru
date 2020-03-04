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
import lsst.afw.math as afwMath
import numpy as np

__all__ = ["PureNoiseMakeCoaddTempExpTask"]


class PureNoiseMakeCoaddTempExpTask(MakeCoaddTempExpTask):
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

        maskPlanes = ["DETECTED", "DETECTED_NEGATIVE", "BAD", "SAT", "NO_DATA", "INTRP"]
        maskVal = exposure.maskedImage.mask.getPlaneBitMask(maskPlanes)
        isGood = ((exposure.maskedImage.mask.array & maskVal == 0) & (exposure.maskedImage.variance.array >
                0))
        q1, q3 = np.percentile(exposure.maskedImage.image.array[isGood], (16,84))
        noise_rms = 0.5*(q3-q1)
        noise_median = np.percentile(exposure.maskedImage.image.array[isGood], 50)
        
        rand = afwMath.Random("MT19937", 1234)
        afwMath.randomGaussianImage(exposure.maskedImage.image, rand)
        ## JUST REMOVE THIS LINE? exposure.maskedImage.mask.set(0)
        exposure.maskedImage.variance.set(1.0)
        exposure.maskedImage *= noise_rms

        # TODO: The images will have a calibration of 1.0 everywhere once RFC-545 is implemented.
        # exposure.setCalib(afwImage.Calib(1.0))
        return exposure
