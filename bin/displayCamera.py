#!/usr/bin/env python
import argparse, re, sys
import lsst.obs.hsc as obs_hsc
import lsst.afw.cameraGeom.utils as cameraGeomUtils
from lsst.afw.cameraGeom import Camera

def checkStr(strVal, level):
    """Check if a string is a valid identifier
    @param[in] strVal: String containing the identifier
    @param[in] level: level of identifier: amp, ccd, raft
    return True if valid
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
    """Display camera element according to command-line arguments
    @param[in] args: argparse.Namespace list of command-line arguments
    """
    mapper = obs_hsc.HscMapper()
    camera = mapper.camera
    frame = 0

    if args.showAmp:
        frame = 0
        for ampStr in args.showAmp:
            if checkStr(ampStr, 'amp'):
                ccd, amp = ampStr.split()
                detector = camera[ccd]
                amplifier = detector[amp]
                cameraGeomUtils.showAmp(amplifier, frame=frame)
                frame += 1

    if args.showCcd:
        frame = 0
        for ccdStr in args.showCcd:
            if checkStr(ccdStr, 'ccd'):
                detector = camera[ccdStr]
                cameraGeomUtils.showCcd(detector, frame=frame)
                frame += 1

    raftMap = {'0':[],
               '1':[]}
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
            if checkStr(raftStr, 'raft'):
                detectorList = []
                for detector in camera:
                    detName = detector.getName()
                    if detName in raftMap[raftStr.lower()]:
                        detectorList.append(detector)
                tmpCamera = Camera(raftStr, detectorList, camera._transformMap)
                cameraGeomUtils.showCamera(tmpCamera, frame=frame, binSize=1)
                frame += 1

    if args.showCamera:
        cameraGeomUtils.showCamera(camera, frame=frame, binSize=args.cameraBinSize)

    if args.plotFocalPlane:
        cameraGeomUtils.plotFocalPlane(camera, 2., 2.)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Display the Subaru cameras')
    parser.add_argument('--showAmp', help='Show an amplifier segment in ds9  May have multiple arguments. '\
                                          'Format like ccd_name amp_name e.g. '\
                                          '\"1_53 0,0\"', type=str, nargs='+')
    parser.add_argument('--showCcd', help='Show a CCD from the mosaic in ds9.  May have multiple arguments. '\
                                          'Format like ccd_name e.g. \"0_17\"', type=str,
                                          nargs='+')
    parser.add_argument('--showRaft', help='Show a raft from the mosaic in ds9.  May have multiple arguments. '\
                                          'May be 0 or 1', type=str)
    parser.add_argument('--showCamera', help='Show the camera mosaic in ds9.', action='store_true')
    parser.add_argument('--cameraBinSize', type= int, default=20,
                        help='Size of binning when displaying the full camera mosaic')
    parser.add_argument('--plotFocalPlane', action='store_true',
                        help='Plot the focalplane in an interactive matplotlib window')
    args = parser.parse_args()

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    displayCamera(args)
