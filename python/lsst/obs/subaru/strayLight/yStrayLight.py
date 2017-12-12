# Copyright (C) 2017  HSC Software Team
# Copyright (C) 2017  Sogo Mineo
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

from __future__ import absolute_import, division, print_function

__all__ = ["StrayLightTask"]

import os

import numpy
import astropy.io.fits as pyfits
import scipy.interpolate

from lsst.pex.config import Config, Field
from lsst.pipe.base import Task

from . import waveletCompression
from .rotatorAngle import inrStartEnd


BAD_THRESHOLD = 500  # Threshold for identifying bad pixels in the reconstructed dark image


class StrayLightConfig(Config):
    doRotatorAngleCorrection = Field(
        dtype=bool,
        doc="Calculate rotator start+end instead of using only the start value from the header?",
        default=True,
    )


class StrayLightTask(Task):
    """Remove stray light in the y-band

    LEDs in an encoder in HSC are producing stray light on the detectors,
    producing the "Eye of Y-band" feature. It can be removed by
    subtracting open-shutter darks. However, because the pattern of stray
    light varies with rotator angle, many dark exposures are required.
    To reduce the data volume for the darks, the images have been
    compressed using wavelets. The code used to construct these is at:

        https://hsc-gitlab.mtk.nao.ac.jp/sogo.mineo/ybackground/

    This Task retrieves the appropriate dark, uncompresses it and
    uses it to remove the stray light from an exposure.
    """
    ConfigClass = StrayLightConfig

    def run(self, sensorRef, exposure):
        """Subtract the y-band stray light

        This relies on knowing the instrument rotator angle during the
        exposure. The FITS headers provide only the instrument rotator
        angle at the start of the exposure (INR_STR), but better
        stray light removal is obtained when we calculate the start and
        end instrument rotator angles ourselves (config parameter
        ``doRotatorAngleCorrection=True``).

        Parameters
        ----------
        sensorRef : `lsst.daf.persistence.ButlerDataRef`
            Data reference for CCD exposure to correct.
        exposure : `lsst.afw.image.Exposure`
            Exposure to correct.
        """
        filterName = exposure.getFilter().getName()
        if filterName != 'y':
            # No correction to be made
            return

        # The LEDs that are causing the Y straylight have not been covered yet (on 2017-11-27),
        # but they will be covered in the near future.
        # Once the LEDs are covered, we will have to uncomment the following statement:
        #
        # if (ccdExposure is newer than a certain date):
        #     return

        header = sensorRef.get('raw_md')
        if self.config.doRotatorAngleCorrection:
            angleStart, angleEnd = inrStartEnd(header)
            self.log.debug(
                "(INR-STR, INR-END) = ({:g}, {:g}) (FITS header says ({:g}, {:g})).".format(
                    angleStart, angleEnd, header.getDouble('INR-STR'), header.getDouble('INR-END'))
            )
        else:
            angleStart = header.getDouble('INR-STR')
            angleEnd = None

        self.log.info("Correcting y-band background")
        filename = sensorRef.get("yBackground_filename")[0]
        model = get_ybackground(filename, angleStart, None if angleStart == angleEnd else angleEnd)

        # Some regions don't have useful model values because the amplifier is dead when the darks were taken
        # 
        badAmps = {9: [0, 1, 2, 3], 33: [0, 1], 43: [0]}  # Known bad amplifiers in the data: {ccdId: [ampId]}
        detId = exposure.getDetector().getId()
        if detId in badAmps:
            isBad = numpy.zeros_like(model, dtype=bool)
            for ii in badAmps[detId]:
                amp = exposure.getDetector()[ii]
                box = amp.getBBox()
                isBad[box.getBeginY():box.getEndY(), box.getBeginX():box.getEndX()] = True
            model[isBad] = numpy.median(model[~isBad])
            mask = exposure.getMaskedImage().getMask()
            mask.array[isBad] |= mask.getPlaneBitMask("BAD")

        model *= exposure.getInfo().getVisitInfo().getExposureTime()
        exposure.image.array -= model


def get_ybackground(filename, angle_start, angle_end=None):
    """Get y-band background image

    It is hypothesized that the instrument rotator rotates
    at a constant angular velocity. This is not strictly
    true, but should be a sufficient approximation for the
    relatively short exposure times typical for HSC.

    @param filename (str)
        Filename of the background data.
    @param angle_start (float)
        Instrument rotation angle in degrees at the start of exposure.
    @param angle_end (float)
        Instrument rotation angle in degrees at the end of exposure.

    @return (numpy.array[][])
    """
    hdulist = pyfits.open(filename)
    header = hdulist[0].header

    # full-size ccd height & channel width
    ccd_h, ch_w = header["F_NAXIS2"], header["F_NAXIS1"]
    # saved data is compressed to 1/2**scale_level of the original size
    image_scale_level = header["WTLEVEL2"], header["WTLEVEL1"]
    angle_scale_level = header["WTLEVEL3"]

    ccd_w = ch_w * len(hdulist)
    ccd_img = numpy.empty(shape=(ccd_h, ccd_w), dtype=numpy.float32)

    for ch, hdu in enumerate(hdulist):
        volume = _upscale_volume(hdu.data, angle_scale_level)

        if angle_end is None:
            img = volume(angle_start)
        else:
            img = volume.integrate(angle_start, angle_end) * (1.0 / (angle_end - angle_start))

        ccd_img[:, ch_w*ch:ch_w*(ch+1)] = _upscale_image(img, (ccd_h, ch_w), image_scale_level)

    # Some regions don't have useful values because the amplifier is dead when the darks were taken
#    is_bad = ccd_img > BAD_THRESHOLD
#    ccd_img[is_bad] = numpy.median(ccd_img[~is_bad])

    return ccd_img


def _upscale_image(img, target_shape, level):
    """
    Upscale the given image to `target_shape` .

    @param img (numpy.array[][])
        Compressed image. `img.shape` must agree
        with waveletCompression.scaled_size(target_shape, level)
    @param target_shape ((int, int))
        The shape of upscaled image, which is to be returned.
    @param level (int or tuple of int)
        Level of multiresolution analysis (or synthesis)

    @return (numpy.array[][])
    """
    h, w = waveletCompression.scaled_size(target_shape, level)

    large_img = numpy.zeros(shape=target_shape, dtype=float)
    large_img[:h, :w] = img

    return waveletCompression.icdf_9_7(large_img, level)


def _upscale_volume(volume, level):
    """
    Upscale the given volume (= sequence of images) along the 0-th axis,
    and return an instance of a interpolation object that interpolates
    the 0-th axis. The 0-th axis is the instrument rotation.

    @param volume (numpy.array[][][])
        Sequence of images.
    @param level (int)
        Level of multiresolution analysis along the 0-th axis.

    @return (scipy.interpolate.CubicSpline)
        You get a slice of the volume at a specific angle (in degrees)
        by calling the returned value as `ret_value(angle)` .
    """
    angles = 720
    _, h, w = volume.shape

    large_volume = numpy.zeros(shape=(angles+1, h, w), dtype=float)

    layers = waveletCompression.scaled_size(angles, level)
    large_volume[:layers] = volume

    large_volume[:-1] = waveletCompression.periodic_icdf_9_7_1d(large_volume[:-1], level, axis=0)
    large_volume[-1] = large_volume[0]

    x = numpy.arange(angles+1) / 2.0
    return scipy.interpolate.CubicSpline(x, large_volume, axis=0, bc_type="periodic")
