#!/usr/bin/env python
#
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
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
# see <http://www.lsstcorp.org/LegalNotices/>.
#

import os
import numpy as np
import matplotlib.pyplot as plt

import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.geom as afwGeom
import lsst.geom as geom

debug = False


def main(camera, sample=20, names=False, showDistortion=True, plot=True, outputFile=None):
    baseFileName = os.path.splitext(os.path.basename(outputFile))[0] if outputFile else None
    if plot:
        fig = plt.figure(1)
        fig.clf()
        ax = fig.add_axes((0.1, 0.1, 0.8, 0.8))
        if showDistortion:
            if baseFileName:
                ax.set_title('%s Distorted CCDs: %s' % (camera.getName(), baseFileName))
            else:
                ax.set_title('%s Distorted CCDs' % camera.getName())
        else:
            if baseFileName:
                ax.set_title('%s Non-Distorted CCDs: %s' % (camera.getName(), baseFileName))
            else:
                ax.set_title('%s Non-Distorted CCDs' % camera.getName())
    else:
        fig = None

    focalPlaneToFieldAngle = camera.getTransformMap().getTransform(cameraGeom.FOCAL_PLANE,
                                                                   cameraGeom.FIELD_ANGLE)
    if debug:
        pixelsOriginal = []
        pixelsDistorted = []
        fieldAngles = []
        ccdIds = []

    for ccd in camera:
        if ccd.getType() == cameraGeom.SCIENCE:
            pixelToFocalPlane = ccd.getTransform(cameraGeom.PIXELS, cameraGeom.FOCAL_PLANE)

            width, height = ccd.getBBox().getWidth(), ccd.getBBox().getHeight()
            corners = ((0.0, 0.0), (0.0, height), (width, height), (width, 0.0), (0.0, 0.0))
            for (x0, y0), (x1, y1) in zip(corners[0:4], corners[1:5]):
                if x0 == x1 and y0 != y1:
                    yList = np.linspace(y0, y1, num=sample)
                    xList = [x0] * len(yList)
                elif y0 == y1 and x0 != x1:
                    xList = np.linspace(x0, x1, num=sample)
                    yList = [y0] * len(xList)
                else:
                    raise RuntimeError("Should never get here")

                xFpOriginal = []
                yFpOriginal = []
                xFpDistort = []
                yFpDistort = []

                for x, y in zip(xList, yList):
                    fpPosition = pixelToFocalPlane.applyForward(geom.Point2D(x, y))  # focal plane position
                    xFpOriginal.append(fpPosition.getX())  # focal plane x position in mm
                    yFpOriginal.append(fpPosition.getY())  # focal plane y position in mm

                    if not showDistortion:
                        continue
                    # Calculate offset (in CCD pixels) due to distortion
                    pixelToDistortedPixel = afwGeom.wcsUtils.computePixelToDistortedPixel(
                        pixelToFocalPlane=pixelToFocalPlane,
                        focalPlaneToFieldAngle=focalPlaneToFieldAngle)
                    # Compute distorted pixel position
                    distortedPixelPosition = pixelToDistortedPixel.applyForward(geom.Point2D(x, y))
                    # Comput distorted focal plane position from distorted pixel position
                    distortedFpPosition = pixelToFocalPlane.applyForward(
                        geom.Point2D(distortedPixelPosition))
                    xFpDistort.append(distortedFpPosition.getX())
                    yFpDistort.append(distortedFpPosition.getY())
                    if debug:
                        # The following is just for recording and comparing with old values
                        pixPosition = pixelToFocalPlane.applyInverse(fpPosition)
                        fieldAngle = focalPlaneToFieldAngle.applyForward(fpPosition)
                        if fieldAngle not in fieldAngles:
                            fieldAngles.append(fieldAngle)
                            pixelsOriginal.append(pixPosition)
                            pixelsDistorted.append(distortedPixelPosition)
                            ccdIds.append(ccd.getId())
                if fig:
                    ax.plot(xFpOriginal, yFpOriginal, 'k-')
                    ax.plot(xFpDistort, yFpDistort, 'r-')

            if fig:
                fpPosition = pixelToFocalPlane.applyForward(geom.Point2D(width/2, height/2))
                x, y = fpPosition.getX(), fpPosition.getY()
                if names:
                    ax.text(x, y, ccd.getName(), ha='center', rotation=90 if height > width else 0,
                            fontsize="smaller")
                else:
                    ax.text(x, y, ccd.getSerial(), ha='center', va='center')

    if fig:
        if camera.getName() == "HSC":
            from matplotlib.patches import Circle
            cen = (0, 0)
            pixelSize = 0.015  # in mm
            vignetteCen = (-1.5, 1.5)  # in mm
            vignetteRadius = 17500*pixelSize  # in mm
            ax.add_patch(Circle(cen, radius=pixelSize*19000, color='black', alpha=0.2, label="focal plane"))
            ax.add_patch(Circle(vignetteCen, radius=vignetteRadius, color='blue', alpha=0.2,
                                label="vignetting"))
            if showDistortion:
                ax.add_patch(Circle(cen, radius=pixelSize*18100, color='red', alpha=0.2, label="distorted"))

            for x, y, t in ([-1, 0, "N"], [0, 1, "W"], [1, 0, "S"], [0, -1, "E"]):
                ax.text(pixelSize*19500*x, pixelSize*19500*y, t, ha="center", va="center")

            plt.axis([pixelSize*-21000, pixelSize*21000, pixelSize*-21000, pixelSize*21000])
            ax.set_xlabel("x Focal Plane (mm)")
            ax.set_ylabel("y Focal Plane (mm)")

        plt.legend(bbox_to_anchor=(1.01, 1), loc='upper left', fontsize=8)
        ax.set_aspect('equal')
        if outputFile:
            plt.savefig(outputFile)
        else:
            plt.show()

    if debug:
        for ccdId, pixelDistorted in zip(ccdIds, pixelsDistorted):
            print(pixelDistorted[0], pixelDistorted[1], ccdId)


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Plot contours of PSF quality')

    parser.add_argument('--suprimeCam', action="store_true", help="Show SuprimeCam", default=False)
    parser.add_argument('--outputFile', type=str, help="File to write plot to", default=None)
    parser.add_argument('--noDistortion', action="store_true", help="Include distortion")
    parser.add_argument('--names', action="store_true", help="Use CCD's names, not serials")

    args = parser.parse_args()

    if args.suprimeCam:
        from lsst.obs.suprimecam import SuprimecamMapper as mapper  # noqa N813
    else:
        from lsst.obs.hsc import HscMapper as mapper  # noqa N813

    camera = mapper(root="/datasets/hsc/repo").camera

    main(camera, names=args.names, sample=2, outputFile=args.outputFile, showDistortion=not args.noDistortion)
    if not args.outputFile:
        print("Hit any key to exit", end=' ')
        input()
