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
import warnings

import lsst.afw.table as afwTable
import lsst.afw.math as afwMath
from lsst.ip.isr.ampOffset import (AmpOffsetConfig, AmpOffsetTask)


class SubaruAmpOffsetConfig(AmpOffsetConfig):
    """Configuration parameters for SubaruAmpOffsetTask.
    """

    def setDefaults(self):
        # the binSize should be as large as possible, preventing background
        # subtraction from inadvertently removing the amp offset signature.
        # Here it's set to the width of an HSC amp, which seems reasonable.
        self.background.binSize = 512
        self.background.algorithm = "AKIMA_SPLINE"
        self.background.useApprox = False
        self.background.ignoredPixelMask = ["BAD", "SAT", "INTRP", "CR", "EDGE", "DETECTED",
                                            "DETECTED_NEGATIVE", "SUSPECT", "NO_DATA"]
        self.detection.reEstimateBackground = False


class SubaruAmpOffsetTask(AmpOffsetTask):
    """Calculate and apply amp offset corrections to an exposure.
    """
    ConfigClass = SubaruAmpOffsetConfig
    _DefaultName = "subaruIsrAmpOffset"

    def __init__(self, *args, **kwargs):
        AmpOffsetTask.__init__(self, *args, **kwargs)

    def run(self, exposure):
        """Calculate amp offset values, determine corrective pedestals for each
        amp, and update the input exposure in-place.

        Parameters
        ----------
        exposure: `lsst.afw.image.Exposure`
            Exposure to be corrected for amp offsets.
        """

        # generate a clone to work on and establish the bit mask
        exp = exposure.clone()
        bitMask = exp.mask.getPlaneBitMask(self.background.config.ignoredPixelMask)
        self.log.debug(f"Ignoring mask planes: {', '.join(self.background.config.ignoredPixelMask)}")

        # fit and subtract background
        if self.config.doBackground:
            maskedImage = exp.getMaskedImage()
            bg = self.background.fitBackground(maskedImage)
            bgImage = bg.getImageF(self.background.config.algorithm, self.background.config.undersampleStyle)
            maskedImage -= bgImage

        # detect sources and update cloned exposure mask planes in-place
        if self.config.doDetection:
            schema = afwTable.SourceTable.makeMinimalSchema()
            table = afwTable.SourceTable.make(schema)
            # detection sigma, used for smoothing and to grow detections, is
            # normally measured from the PSF of the exposure. As the PSF hasn't
            # been measured at this stage of processing, sigma is instead
            # set to an approximate value here which should be sufficient.
            _ = self.detection.run(table=table, exposure=exp, sigma=2)

        # safety check: do any pixels remain for amp offset estimation?
        if (exp.mask.array & bitMask).all():
            self.log.warning("All pixels masked: cannot calculate any amp offset corrections.")
        else:
            # set up amp offset inputs
            im = exp.image
            im.array[(exp.mask.array & bitMask) > 0] = np.nan
            amps = exp.getDetector().getAmplifiers()
            ampEdgeOuter = self.config.ampEdgeInset + self.config.ampEdgeWidth
            sctrl = afwMath.StatisticsControl()

            # loop over each amp edge boundary to extract amp offset values
            ampOffsets = []
            for ii in range(1, len(amps)):
                ampA = im[amps[ii - 1].getBBox()].array
                ampB = im[amps[ii].getBBox()].array
                stripA = ampA[:, -ampEdgeOuter:][:, :self.config.ampEdgeWidth]
                stripB = ampB[:, :ampEdgeOuter][:, -self.config.ampEdgeWidth:]
                # catch warnings to prevent all-NaN slice RuntimeWarning
                with warnings.catch_warnings():
                    warnings.filterwarnings('ignore', r'All-NaN (slice|axis) encountered')
                    edgeA = np.nanmedian(stripA, axis=1)
                    edgeB = np.nanmedian(stripB, axis=1)
                edgeDiff = edgeB - edgeA
                # compute rolling averages
                edgeDiffSum = np.convolve(np.nan_to_num(edgeDiff), np.ones(self.config.ampEdgeWindow), 'same')
                edgeDiffNum = np.convolve(~np.isnan(edgeDiff), np.ones(self.config.ampEdgeWindow), 'same')
                edgeDiffAvg = edgeDiffSum / np.clip(edgeDiffNum, 1, None)
                edgeDiffAvg[np.isnan(edgeDiff)] = np.nan
                # take clipped mean of rolling average data as amp offset value
                ampOffset = afwMath.makeStatistics(edgeDiffAvg, afwMath.MEANCLIP, sctrl).getValue()
                # perform a couple of do-no-harm safety checks:
                # a) the fraction of unmasked pixel rows is > ampEdgeMinFrac,
                # b) the absolute offset ADU value is < ampEdgeMaxOffset
                ampEdgeGoodFrac = 1 - (np.sum(np.isnan(edgeDiffAvg))/len(edgeDiffAvg))
                minFracFail = ampEdgeGoodFrac < self.config.ampEdgeMinFrac
                maxOffsetFail = np.abs(ampOffset) > self.config.ampEdgeMaxOffset
                if minFracFail or maxOffsetFail:
                    ampOffset = 0
                ampOffsets.append(ampOffset)
                self.log.debug(f"amp edge {ii}{ii+1} : "
                               f"viable edge frac = {ampEdgeGoodFrac}, "
                               f"edge offset = {ampOffset:.3f}")

            # solve for pedestal values and update original exposure in-place
            A = np.array([[-1.0, 1.0, 0.0, 0.0],
                          [1.0, -2.0, 1.0, 0.0],
                          [0.0, 1.0, -2.0, 1.0],
                          [0.0, 0.0, 1.0, -1.0]])
            B = np.array([ampOffsets[0],
                          ampOffsets[1] - ampOffsets[0],
                          ampOffsets[2] - ampOffsets[1],
                          -ampOffsets[2]])
            # if least-squares minimization fails, convert NaNs to zeroes,
            # ensuring that no values are erroneously added/subtracted
            pedestals = np.nan_to_num(np.linalg.lstsq(A, B, rcond=None)[0])
            metadata = exposure.getMetadata()
            for ii, (amp, pedestal) in enumerate(zip(amps, pedestals)):
                ampIm = exposure.image[amp.getBBox()].array
                ampIm -= pedestal
                metadata.set(f"PEDESTAL{ii + 1}",
                             float(pedestal),
                             f"Pedestal level subtracted from amp {ii + 1}")
            self.log.info(f"amp pedestal values: {', '.join([f'{x:.4f}' for x in pedestals])}")
