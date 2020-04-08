# This file is part of obs_subaru.
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

import numpy as np

import lsst.afw.image as afwImage
import lsst.pipe.base as pipeBase
import lsst.pex.config as pexConfig
import lsst.afw.table as afwTable
from lsst.meas.algorithms import (SourceDetectionTask, SubtractBackgroundTask,)
from lsst.ip.isr.ampOffset import (AmpOffsetConfig, AmpOffsetTask)


class SubaruAmpOffsetConfig(AmpOffsetConfig):
    """Configuration parameters for SubaruAmpOffsetTask.
    """
    ignoredPixelMask = pexConfig.ListField(
        default=["BAD", "SAT", "INTRP", "CR", "EDGE", "DETECTED",
                 "DETECTED_NEGATIVE", "SUSPECT", "NO_DATA"],
        dtype=str,
        doc="Names of mask planes to ignore while estimating the background.",
        itemCheck=lambda x: x in afwImage.Mask().getMaskPlaneDict().keys(),
    )
    doDetection = pexConfig.Field(
        default=True,
        dtype=bool,
        doc="Detect sources prior to pattern continuity estimation?",
    )
    ampEdgeInset = pexConfig.Field(
        default=5,
        dtype=int,
        doc="Number of pixels amp edge strip inset from amp edge, used to calculate amp edge flux values.",
    )
    ampEdgeWidth = pexConfig.Field(
        default=64,
        dtype=int,
        doc="Width of amp edge strip, used to calculate amp edge flux values.",
    )


class SubaruAmpOffsetTask(AmpOffsetTask):
    """Calculate and apply pattern continuity corrections to an exposure,
    correcting for any amp-to-amp offsets.
    """
    ConfigClass = SubaruAmpOffsetConfig
    _DefaultName = "subaruIsrAmpOffset"

    def run(self, exposure):
        """Calculate amp-edge pattern continuity corrections and update the
        input exposure in-place.

        Parameters
        ----------
        exposure: `lsst.afw.image.Exposure`
            Exposure to be corrected for any amp-to-amp offsets.
        """

        # clone input data
        exp = exposure.clone()
        maskedImage = exp.getMaskedImage()

        # establish bit mask
        bitMask = maskedImage.getMask().getPlaneBitMask(self.config.ignoredPixelMask)
        self.log.debug(f"Ignoring mask planes: {', '.join(self.config.ignoredPixelMask)}")
        if (maskedImage.getMask().getArray() & bitMask).all():
            raise pipeBase.TaskError("All pixels masked: cannot calculate any amp offset corrections.")

        # add a detection mask plane?
        if self.config.doDetection and ("DETECTED" in self.config.ignoredPixelMask or
                                        "DETECTED_NEGATIVE" in self.config.ignoredPixelMask):

            # create subtract background task
            backgroundConfig = SubtractBackgroundTask.ConfigClass()
            backgroundTask = SubtractBackgroundTask(config=backgroundConfig)

            # fit and subtract temp. background (to enable source detection)
            fittedBg = backgroundTask.fitBackground(maskedImage)
            maskedImage -= fittedBg.getImageF(backgroundConfig.algorithm, backgroundConfig.undersampleStyle)

            # create source detection task
            detectionConfig = SourceDetectionTask.ConfigClass()
            detectionConfig.reEstimateBackground = False
            detectionSchema = afwTable.SourceTable.makeMinimalSchema()
            detectionTask = SourceDetectionTask(config=detectionConfig, schema=detectionSchema)
            detectionTable = afwTable.SourceTable.make(detectionSchema)

            # detect sources and update mask planes in place on cloned exposure
            _ = detectionTask.run(detectionTable, exp, sigma=2).sources

        # get copy of original image and set masked pixels to NaN
        im = exposure.getImage().clone()
        im.getArray()[(maskedImage.getMask().getArray() & bitMask) > 0] = np.nan

        metadata = exposure.getMetadata()
        amps = exposure.getDetector().getAmplifiers()
        ampEdgeOuter = self.config.ampEdgeInset + self.config.ampEdgeWidth

        # extract amp-edge pixel step values
        deltas = []
        for ii in range(1, len(amps)):
            ampA = im[amps[ii - 1].getBBox()].getArray()
            ampB = im[amps[ii].getBBox()].getArray()
            stripA = ampA[:, -ampEdgeOuter:][:, :self.config.ampEdgeWidth]
            stripB = ampB[:, :ampEdgeOuter][:, -self.config.ampEdgeWidth:]
            arrayA = np.ma.median(np.ma.array(stripA, mask=np.isnan(stripA)), axis=1)
            arrayB = np.ma.median(np.ma.array(stripB, mask=np.isnan(stripB)), axis=1)
            ampDiff = arrayA.data[~arrayA.mask & ~arrayB.mask] - arrayB.data[~arrayA.mask & ~arrayB.mask]
            ampStep = np.median(ampDiff) if len(ampDiff) > 0 else 0
            deltas.append(ampStep)
            # metadata.set(f"AMP{ii}{ii + 1}OFF", float(ampStep))
        # commented out, as redundant information with piston values below
        # self.log.info(f"amp-to-amp delta steps: "
        #               f"{', '.join([f'{x:.2f}' for x in deltas])}")

        # solve for piston values and update original exposure in-place
        A = np.array([[1.0, -1.0, 0.0, 0.0],
                      [-1.0, 2.0, -1.0, 0.0],
                      [0.0, -1.0, 2.0, -1.0],
                      [0.0, 0.0, -1.0, 1.0]])
        B = np.array([deltas[0],
                      deltas[1] - deltas[0],
                      deltas[2] - deltas[1],
                      -deltas[2]])
        pistons = np.nan_to_num(np.linalg.lstsq(A, B, rcond=None)[0])
        for ii, (amp, piston) in enumerate(zip(amps, pistons)):
            ampIm = exposure.getImage()[amp.getBBox()].getArray()
            ampIm -= piston
            metadata.set(f"PISTON{ii + 1}", float(piston), f"Piston level subtracted from amp {ii + 1}")
        self.log.info(f"amp piston values: {', '.join([f'{x:.2f}' for x in pistons])}")
