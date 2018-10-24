#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2008-2016 AURA/LSST.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
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
from astropy.io import fits
import sys
import re
import os

# http://www.cfht.hawaii.edu/Instruments/Imaging/Megacam/generalinformation.html
cfhtPixelScale = 13.5 * 1.e-3  # millimeters

detsizeX = 23219             # pixels
detsizeY = 19354             # pixels
detCenterX = 0.5 * detsizeX  # pixels
detCenterY = 0.5 * detsizeY  # pixels

hack = True


def getSecCenter(sec):
    sec = re.sub('\'', '', sec)
    sec = re.sub(r'\[|\:|\,|\]', ' ', sec)
    fields = list(map(int, sec.split()))
    # these indices run from 1..N
    # to run from 0..N-1 subtract off 2 from each sum
    return 0.5 * (fields[0] + fields[1] - 2), 0.5 * (fields[2] + fields[3] - 2)


def printCameraGeom(filename, buff):
    buff.write('Camera: { \n')
    buff.write('    name: "CFHT MegaCam %s" \n' % (os.path.basename(filename)))
    buff.write('    nCol: 1 \n')
    buff.write('    nRow: 1 \n')
    buff.write('    Raft: { \n')
    buff.write('        name: "R:0,0" \n')
    if hack:
        buff.write('        serial: -1 \n')
    buff.write('        index: 0 0 \n')
    buff.write('        offset: 0.0 0.0 \n')
    buff.write('    } \n')
    buff.write('} \n')


def printAmpDefaults(buff):
    # http://cfht.hawaii.edu/Instruments/Imaging/MegaPrime/rawdata.html
    #
    # amp image comes out 4612 + 32 high by 1024 + 32 wide
    buff.write('Amp: { \n')
    buff.write('    height: 4612 \n')
    buff.write('    width: 1024 \n')
    buff.write('    extended: 0 \n')
    buff.write('    preRows: 0 \n')
    buff.write('    overclockH: 32 \n')
    buff.write('    overclockV: 32 \n')
    buff.write('} \n')


def printCcdAmpDefaults(buff):
    # Depending on how much we need to undo the imsplice operation
    # here are the defaults from straight off the camera
    #
    # http://www.cfht.hawaii.edu/Instruments/Imaging/Megacam/specsinformation.html
    # if ccdId < 18:
    #     first  = 'B'
    #     second = 'A'
    #     freadOut = 'ULC'
    #     sreadOut = 'URC'
    # else:
    #     first  = 'A'
    #     second = 'B'
    #     freadOut = 'LLC'
    #     sreadOut = 'LRC'

    buff.write('Ccd: { \n')
    buff.write('    pixelSize: %.6f \n' % (cfhtPixelScale))
    buff.write('    nCol: 2 \n')
    buff.write('    nRow: 1 \n')

    buff.write('    Amp: { \n')
    buff.write('        index: 0 0 \n')
    buff.write('        readoutCorner: LLC \n')
    buff.write('    } \n')

    buff.write('    Amp: { \n')
    buff.write('        index: 1 0 \n')
    buff.write('        readoutCorner: LRC \n')
    buff.write('    } \n')
    buff.write('} \n')


def printCcdDiskLayout(buff):
    buff.write('CcdDiskLayout: { \n')
    buff.write('    HduPerAmp: true \n')
    buff.write('    Amp: { \n')
    if hack:
        buff.write('        serial: 0 \n')
    else:
        buff.write('        index: 0 0 \n')
    buff.write('        flipLR: false \n')
    buff.write('        flipTB: false \n')
    buff.write('        hdu: 0 \n')
    buff.write('    } \n')
    buff.write('    Amp: { \n')
    if hack:
        buff.write('        serial: 1 \n')
    else:
        buff.write('        index: 1 0 \n')
    buff.write('        flipLR: false \n')
    buff.write('        flipTB: false \n')
    buff.write('        hdu: 0 \n')
    buff.write('    } \n')
    buff.write('} \n')


def printRaftCcdGeom(buff, ccdId, ccdname, xidx, yidx, xpos, ypos):
    if (ccdId == 0):
        # header
        buff.write('Raft: { \n')
        buff.write('    nCol: 9 \n')
        buff.write('    nRow: 4 \n')
        buff.write('    name: "R:0,0" \n')
        if hack:
            buff.write('    serial: -1 \n')

    buff.write('    Ccd: { \n')
    buff.write('        name: "CFHT %d" \n' % (ccdId))
    buff.write('        serial: %s \n' % (re.sub('-', '', ccdname)))
    buff.write('        index: %d %d \n' % (xidx, yidx))
    buff.write('        offset: %.6f %.6f \n' % (xpos, ypos))
    buff.write('        nQuarter: 0 \n')
    buff.write('        orientation: 0.000000 0.000000 0.000000 \n')
    buff.write('    } \n')


def printElectronics(buff, ccdId, ccdname, xidx, yidx, infoA, infoB):
    # http://www.cfht.hawaii.edu/Instruments/Imaging/Megacam/specsinformation.html

    if (ccdId == 0):
        # header
        buff.write('Electronic: { \n')
        buff.write('    Raft: { \n')
        buff.write('        name: "R:0,0" \n')
        if hack:
            buff.write('        serial: -1 \n')

    buff.write('        Ccd: { \n')
    buff.write('            name: "CFHT %d" \n' % (ccdId))
    buff.write('            serial: %s \n' % (re.sub('-', '', ccdname)))

    buff.write('            Amp: { \n')
    if not hack:
        buff.write('                serial: 0 \n')
    else:
        buff.write('                index: 0 0 \n')
    buff.write('                gain: %.3f \n' % (infoA[0]))
    buff.write('                readNoise: %.3f \n' % (infoA[1]))
    buff.write('                saturationLevel: %.3f \n' % (infoA[2]))
    buff.write('            } \n')
    buff.write('            Amp: { \n')
    if not hack:
        buff.write('                serial: 1 \n')
    else:
        buff.write('                index: 1 0 \n')
    buff.write('                gain: %.3f \n' % (infoB[0]))
    buff.write('                readNoise: %.3f \n' % (infoB[1]))
    buff.write('                saturationLevel: %.3f \n' % (infoB[2]))
    buff.write('            } \n')
    buff.write('        } \n')


if __name__ == '__main__':

    # NOTES : the full focal plane size is reflected in the image headers by
    #
    #  DETSIZE = '[1:23219,1:19354]'
    #
    # and the information on each CCD as
    #
    #  cal-53535-i-797722_1_img.fits: DETSIZE = '[4160:2113,19351:14740]'
    #  cal-53535-i-797722_2_img.fits: DETSIZE = '[6278:4231,19352:14741]'
    #  cal-53535-i-797722_5_img.fits: DETSIZE = '[12630:10583,19354:14743]'
    #
    # Note that the boundary of these CCDs differs by 4231-4160 = 71
    # pixels so we can infer the chip gaps this way.  There also is
    # information on the misalignment of the CCDs in the other dimension;
    # in these cases they are offset by 1 pixel or 13.5 microns.
    #
    # Finally the amps are described within a CCD by
    #
    #  cal-53535-i-797722_5_img.fits: DETSECA = '[12630:11607,19354:14743]'
    #  cal-53535-i-797722_5_img.fits: DETSECB = '[10583:11606,19354:14743]'
    #
    # so there are no boundaries between amps.
    #
    #
    # Another convention is that everything is w.r.t. the center of
    # everything else.  Raft w.r.t. boresight; CCD w.r.t. Raft center; Amp
    # w.r.t CCD center.

    buffCamera = open('Camera.paf', 'w')
    buffElectro = open('Electronics.paf', 'w')

    infile = sys.argv[1]           # full MEF file; e.g. 871034p.fits
    ptr = fits.open(infile)

    printCameraGeom(infile, buffCamera)
    printAmpDefaults(buffCamera)
    printCcdAmpDefaults(buffCamera)
    printCcdDiskLayout(buffCamera)

    # 0th layer is pure metadata
    for ccd in range(1, len(ptr)):
        ccdId = ptr[ccd].header['EXTVER']
        ccdBoundary = ptr[ccd].header['DETSEC']
        ccdName = ptr[ccd].header['CCDNAME']
        amp1Boundary = ptr[ccd].header['DETSECA']
        amp2Boundary = ptr[ccd].header['DETSECB']

        ccdxc, ccdyc = getSecCenter(ccdBoundary)
        xpos = (ccdxc - detCenterX) * cfhtPixelScale
        ypos = (ccdyc - detCenterY) * cfhtPixelScale

        gain1 = ptr[ccd].header['GAINA']
        gain2 = ptr[ccd].header['GAINB']

        rdnoise1 = ptr[ccd].header['RDNOISEA']
        rdnoise2 = ptr[ccd].header['RDNOISEB']

        # assume non-linear = saturation since we have no non-linearity curves
        saturate1 = ptr[ccd].header['MAXLINA']
        saturate2 = ptr[ccd].header['MAXLINB']

        print(ccdId, ccdBoundary, amp1Boundary, amp2Boundary, gain1, gain2,
              rdnoise1, rdnoise2, saturate1, saturate2)

        # flip y
        xidx = ccdId % 9
        yidx = abs(ccdId / 9 - 3)

        printRaftCcdGeom(buffCamera, ccdId, ccdName, xidx, yidx, xpos, ypos)
        printElectronics(buffElectro, ccdId, ccdName, xidx, yidx,
                         [gain1, rdnoise1, saturate1],
                         [gain2, rdnoise2, saturate2])

    buffCamera.write('} \n')
    buffCamera.close()

    buffElectro.write('    } \n')
    buffElectro.write('} \n')
    buffElectro.close()
