#!/usr/bin/env python

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

import argparse
import re
import sys
import lsst.obs.hsc as obs_hsc
import lsst.afw.cameraGeom.utils as cameraGeomUtils
import lsst.afw.display as afwDisplay


def checkStr(strVal, level):
    """Check if a string is a valid identifier.

    Parameters
    ----------
    strVal : `str`
       String containing the identifier
    level : `str`
       Level of identifier: amp, ccd, raft.

    Raises
    ------
    ValueError
       If unknown combination of ``strVal`` and ``level`` are provided.

    Returns
    -------
    result : `bool`
       Returns `True` if valid.
    """
    if level == 'amp':
        matchStr = '^[01]_[0-9]* [0-3],0$'
        if not re.match(matchStr, strVal):
            raise ValueError("Specify ccd name and amp name: %s"%(strVal))
    elif level == 'ccd':
        matchStr = '^[01]_[0-9]*$'
        if not re.match(matchStr, strVal):
            raise ValueError("Specify the ccd name: e.g. 1_16: %s"%(strVal))
    elif level == 'raft':
        if not strVal.lower() in ('0', '1'):
            raise ValueError("Specify the raft name as '0' or '1': %s"%(strVal))
    else:
        raise ValueError("level must be one of: ('amp', 'ccd', 'raft')")
    return True


def displayCamera(args):
    """Display camera element according to command-line arguments.

    Parameters
    ----------
    args : `argparse.Namespace`
       Command-line arguments to parse.
    """
    mapper = obs_hsc.HscMapper(root=".")
    camera = mapper.camera
    frame = 0

    if args.showAmp:
        frame = 0
        for ampStr in args.showAmp:
            if checkStr(ampStr, 'amp'):
                ccd, amp = ampStr.split()
                detector = camera[ccd]
                amplifier = detector[amp]
                disp = afwDisplay.Display(frame=frame)
                cameraGeomUtils.showAmp(amplifier, display=disp)
                frame += 1

    if args.showCcd:
        frame = 0
        for ccdStr in args.showCcd:
            if checkStr(ccdStr, 'ccd'):
                detector = camera[ccdStr]
                disp = afwDisplay.Display(frame=frame)
                cameraGeomUtils.showCcd(detector, display=disp)
                frame += 1

    raftMap = {'0': [],
               '1': []}
    for det in camera:
        dName = det.getName()
        if dName.startswith('1'):
            raftMap['1'].append(dName)
        elif dName.startswith('0'):
            raftMap['0'].append(dName)
        else:
            raise RuntimeError("Did not recognize detector name")

    if args.showRaft:
        frame = 0
        for raftStr in args.showRaft:
            disp = afwDisplay.Display(frame)
            if checkStr(raftStr, 'raft'):
                detectorNameList = []
                for detector in camera:
                    detName = detector.getName()
                    if detName in raftMap[raftStr.lower()]:
                        detectorNameList.append(detName)
                cameraGeomUtils.showCamera(camera, detectorNameList=detectorNameList, display=disp, binSize=4)
                frame += 1

    if args.showCamera:
        disp = afwDisplay.Display(frame)
        cameraGeomUtils.showCamera(camera, display=disp, binSize=args.cameraBinSize)

    if args.plotFocalPlane:
        cameraGeomUtils.plotFocalPlane(camera, 2., 2.)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Display the Subaru cameras')
    parser.add_argument('--showAmp', help='Show an amplifier segment.  May have multiple arguments.  '
                                          'Format like ccd_name amp_name e.g. \"1_53 0,0\"',
                        type=str, nargs='+')
    parser.add_argument('--showCcd', help='Show a CCD from the mosaic.  May have multiple arguments.  '
                                          'Format like ccd_name e.g. \"0_17\"', type=str, nargs='+')
    parser.add_argument('--showRaft', help='Show a raft from the mosaic.  May have multiple arguments.  '
                                           'May be 0 or 1', type=str, nargs='+')
    parser.add_argument('--showCamera', help='Show the camera mosaic.', action='store_true')
    parser.add_argument('--cameraBinSize', type=int, default=20,
                        help='Size of binning when displaying the full camera mosaic')
    parser.add_argument('--plotFocalPlane', action='store_true',
                        help='Plot the focalplane in an interactive matplotlib window')
    args = parser.parse_args()

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    displayCamera(args)
