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

import numpy
import matplotlib.pyplot as plt

import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.geom as afwGeom


def main(camera, sample=20, names=False, showDistortion=True, plot=True, outputFile=None):
    if plot:
        fig = plt.figure(1)
        fig.clf()
        ax = fig.add_axes((0.1, 0.1, 0.8, 0.8))
        ax.set_title('%s  Distorted CCDs' % camera.getId().getName())
    else:
        fig = None

    dist = camera.getDistortion()

    for raft in camera:
        raft = cameraGeom.cast_Raft(raft)
        for ccd in raft:
            ccd.setTrimmed(True)

            width, height = ccd.getAllPixels(True).getDimensions()

            corners = ((0.0, 0.0), (0.0, height), (width, height), (width, 0.0), (0.0, 0.0))
            for (x0, y0), (x1, y1) in zip(corners[0:4], corners[1:5]):
                if x0 == x1 and y0 != y1:
                    yList = numpy.linspace(y0, y1, num=sample)
                    xList = [x0] * len(yList)
                elif y0 == y1 and x0 != x1:
                    xList = numpy.linspace(x0, x1, num=sample)
                    yList = [y0] * len(xList)
                else:
                    raise RuntimeError("Should never get here")

                xOriginal = []
                yOriginal = []
                xDistort = []
                yDistort = []
                for x, y in zip(xList, yList):
                    position = ccd.getPositionFromPixel(afwGeom.Point2D(x, y)) # focal plane position

                    xOriginal.append(position.getMm().getX())
                    yOriginal.append(position.getMm().getY())

                    if not showDistortion:
                        continue
                    # Calculate offset (in CCD pixels) due to distortion
                    distortion = dist.distort(afwGeom.Point2D(x, y), ccd) - afwGeom.Extent2D(x, y)

                    # Calculate the distorted position
                    distorted = position + cameraGeom.FpPoint(distortion)*ccd.getPixelSize()

                    xDistort.append(distorted.getMm().getX())
                    yDistort.append(distorted.getMm().getY())

                if fig:
                    ax.plot(xOriginal, yOriginal, 'k-')
                    ax.plot(xDistort, yDistort, 'r-')

            if fig:
                x, y = ccd.getPositionFromPixel(afwGeom.Point2D(width/2, height/2)).getMm()
                cid = ccd.getId()
                if names:
                    ax.text(x, y, cid.getName(), ha='center', rotation=90 if height > width else 0,
                            fontsize="smaller")
                else:
                    ax.text(x, y, cid.getSerial(), ha='center', va='center')

    if fig:
        if camera.getId().getName() == "HSC":
            from matplotlib.patches import Circle
            cen = (0, 0)
            ax.add_patch(Circle(cen, radius=18100, color='black', alpha=0.2))
            if showDistortion:
                ax.add_patch(Circle(cen, radius=19000, color='red', alpha=0.2))

            for x, y, t in ([-1, 0, "N"], [0, 1, "W"], [1, 0, "S"], [0, -1, "E"]):
                ax.text(19500*x, 19500*y, t, ha="center", va="center")

            plt.axis([-21000, 21000, -21000, 21000])

        ax.set_aspect('equal')
        if outputFile:
            plt.savefig(outputFile)
        else:
            plt.show()

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Plot contours of PSF quality')

    parser.add_argument('--suprimeCam', action="store_true", help="Show SuprimeCam", default=False)
    parser.add_argument('--outputFile', type=str, help="File to write plot to", default=None)
    parser.add_argument('--noDistortion', action="store_true", help="Include distortion")
    parser.add_argument('--names', action="store_true", help="Use CCD's names, not serials")

    args = parser.parse_args()

    if args.suprimeCam:
        from lsst.obs.suprimecam import SuprimecamMapper as mapper
    else:
        from lsst.obs.hsc import HscMapper as mapper

    camera = mapper().camera

    main(camera, names=args.names, sample=2, outputFile=args.outputFile,
         showDistortion=not args.noDistortion)
    if not args.outputFile:
        print "Hit any key to exit",
        raw_input()
