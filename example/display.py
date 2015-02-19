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

import math
import numpy
import matplotlib.pyplot as plt

import lsst.afw.cameraGeom as cameraGeom
import lsst.afw.geom as afwGeom
import lsst.obs.hscSim as hscSim
import lsst.pipette.config as pipConfig
import lsst.pipette.distortion as pipDist

SAMPLE = 100


def main(camera, distortionConfig):
    fig = plt.figure(1)
    fig.clf()
    ax = fig.add_axes((0.1, 0.1, 0.8, 0.8))
#    ax.set_autoscale_on(False)
#    ax.set_ybound(lower=-0.2, upper=0.2)
#    ax.set_xbound(lower=-17, upper=-7)
    ax.set_title('Distorted CCDs')
    
    for raft in camera:
        raft = cameraGeom.cast_Raft(raft)
        for ccd in raft:
            ccd = cameraGeom.cast_Ccd(ccd)
            size = ccd.getSize()
            width, height = 2048, 4096
            dist = pipDist.createDistortion(ccd, distortionConfig)
            corners = ((0.0,0.0), (0.0, height), (width, height), (width, 0.0), (0.0, 0.0))
            for (x0, y0), (x1, y1) in zip(corners[0:4],corners[1:5]):
                if x0 == x1 and y0 != y1:
                    yList = numpy.linspace(y0, y1, num=SAMPLE)
                    xList = [x0] * len(yList)
                elif y0 == y1 and x0 != x1:
                    xList = numpy.linspace(x0, x1, num=SAMPLE)
                    yList = [y0] * len(xList)
                else:
                    raise RuntimeError("Should never get here")

                xDistort = []; yDistort = []
                for x, y in zip(xList, yList):
                    distorted = dist.actualToIdeal(afwGeom.Point2D(x, y))
                    xDistort.append(distorted.getX())
                    yDistort.append(distorted.getY())

                ax.plot(xDistort, yDistort, 'k-')

    plt.show()

if __name__ == '__main__':
    camera = hscSim.HscSimMapper().camera
    config = pipConfig.Config()
    config['class'] = "hsc.meas.match.hscDistortion.HscDistortion"
    main(camera, config)
