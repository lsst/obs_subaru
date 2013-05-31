#!/usr/bin/env python
import argparse, os, re, sys

import numpy as np
from matplotlib.mlab import griddata
import matplotlib.pyplot as plt

import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as cameraGeom
import lsst.daf.persistence as dafPersist
import lsst.obs.hscSim as hscSim

def main(dataDir, visit, title="", outputTxtFileName=None,
         showFwhm=False, minFwhm=None, maxFwhm=None,
         correctDistortion=False,
         showEllipticity=False, ellipticityDirection=False,
         gridPoints=30, verbose=False):

    butler = dafPersist.ButlerFactory(mapper=hscSim.HscSimMapper(root=dataDir)).create()
    camera = butler.get("camera")

    if not (showFwhm or showEllipticity or outputTxtFileName):
        showFwhm = True
    #
    # Get a dict of cameraGeom::Ccd indexed by serial number
    #
    ccds = {}
    for raft in camera:
        raft = cameraGeom.cast_Raft(raft)
        for ccd in raft:
            ccd = cameraGeom.cast_Ccd(ccd)
            ccd.setTrimmed(True)
            ccds[ccd.getId().getSerial()] = ccd
    #
    # Read all the tableSeeingMap files, converting their (x, y) to focal plane coordinates
    #
    xArr = []; yArr = []; ellArr = []; fwhmArr = []
    paArr = []; aArr = []; bArr = []
    for tab in butler.subset("tableSeeingMap", visit=visit):
        # we could use tab.datasetExists() but it prints a rude message
        fileName = butler.get("tableSeeingMap_filename", **tab.dataId)[0]
        if not os.path.exists(fileName):
            continue

        with open(fileName) as fd:
            ccd = None
            for line in fd.readlines():
                if re.search(r"^\s*#", line):
                    continue
                fields = [float(_) for _ in line.split()]

                if ccd is None:
                    ccd = ccds[int(fields[0])]

                x, y, fwhm, ell, pa, a, b  = fields[1:]
                x, y = ccd.getPositionFromPixel(afwGeom.PointD(x, y)).getMm()
                xArr.append(x)
                yArr.append(y)
                ellArr.append(ell)
                fwhmArr.append(fwhm)
                paArr.append(pa)
                aArr.append(a)
                bArr.append(b)

    xArr = np.array(xArr)
    yArr = np.array(yArr)
    ellArr = np.array(ellArr)
    fwhmArr = np.array(fwhmArr)*0.168   # arcseconds
    paArr = np.array(paArr)
    aArr = np.array(aArr)
    bArr = np.array(bArr)

    if len(xArr) == 0:
        gridPoints = 0
        xs, ys = [], []
    else:
        N = gridPoints*1j
        extent = [min(xArr), max(xArr), min(yArr), max(yArr)]
        xs,ys = np.mgrid[extent[0]:extent[1]:N, extent[2]:extent[3]:N]

    if outputTxtFileName:
        pass

    title = [title,]

    title.append("\n")

    if showFwhm:
        title.append("FWHM (arcsec)")
        if len(xs) > 0:
            fwhmResampled = griddata(xArr, yArr, fwhmArr, xs, ys)
            
            plt.imshow(fwhmResampled.T, extent=extent, vmin=minFwhm, vmax=maxFwhm)
            plt.colorbar()
    elif showEllipticity:
        title.append("Ellipticity")
        scale = 4

        
        if ellipticityDirection:        # we don't care about the magnitude
            ellArr = 0.1

        u =  -ellArr*np.cos(np.radians(paArr))
        v =  -ellArr*np.sin(np.radians(paArr))
        if gridPoints > 0:
            u = griddata(xArr, yArr, u, xs, ys)
            v = griddata(xArr, yArr, v, xs, ys)
            x, y = xs, ys
        else:
            x, y = xArr, yArr

        Q = plt.quiver(x, y, u, v, scale=scale,
                       headwidth=0,
                       headlength=0,
                       headaxislength=0,
                       )
        keyLen = 0.10
        if not ellipticityDirection:    # we care about the magnitude
            plt.quiverkey(Q, 0.20, 0.95, keyLen, "e=%g" % keyLen, labelpos='W')
        
    #plt.plot(xArr, yArr, "r.")
    #plt.plot(xs, ys, "b.")
    plt.axes().set_aspect('equal')
    plt.axis([-20000, 20000, -20000, 20000])

    title.append(r'$\langle$FWHM$\rangle %4.2f$"' % np.median(fwhmArr))
    plt.title("%s visit=%s" % (" ".join(title), visit))

    return plt

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot contours of PSF quality')

    parser.add_argument('dataDir', type=str, nargs="?", help='Datadir to search for PSF information')
    parser.add_argument('--rerun', type=str, help="Rerun to examine", default=None)
    parser.add_argument('--visit', type=str, nargs="+", help='Name of desired visit')
    parser.add_argument('--inputFile', help="""File of desired visits and titles.
One page will be generated per line in the file.  A line
#dataDir: path
switches to that dataDir (the one on the command line is used previously)
""", default=None)
    parser.add_argument('--gridPoints', type=int,
                        help='Number of points in x and y when gridding (0: show raw data)', default=35)
    parser.add_argument('--showFwhm', action="store_true", help="Show the FWHM", default=False)
    parser.add_argument('--minFwhm', type=float, help="Minimum FWHM to plot", default=None)
    parser.add_argument('--maxFwhm', type=float, help="Maximum FWHM to plot", default=None)
    parser.add_argument('--showEllipticity', action="store_true", help="Show the stars' ellipticity",
                        default=False)
    parser.add_argument('--ellipticityDirection', action="store_true",
                        help="Show the ellipticity direction, not value", default=False)
    parser.add_argument('--correctDistortion', action="store_true",
                        help="Correct for the known optical distortion", default=False)
    parser.add_argument('--verbose', action="store_true", help="Be chatty", default=False)
    parser.add_argument('--outputPlotFile', help="File to save output to", default=None)
    parser.add_argument('--outputTxtFile', help="File to save output to", default=None)
    
    args = parser.parse_args()

    if not (args.dataDir or args.inputFile):
        args.dataDir = os.environ.get("SUPRIME_DATA_DIR")
        if not args.dataDir:
            print >> sys.stderr, "Please specify a dataDir (maybe in an inputFile) or set $SUPRIME_DATA_DIR"
            sys.exit(1)

    if args.rerun:
        args.dataDir = os.path.join(args.dataDir, "rerun", args.rerun)

    if args.outputPlotFile:
        from matplotlib.backends.backend_pdf import PdfPages
        pp = PdfPages(args.outputPlotFile)
    else:
        pp = None

    if args.inputFile:
        desires = []
        dd = args.dataDir
        with open(args.inputFile) as fd:
            for line in fd.readlines():
                line = line.rstrip()
                if re.search(r"^\s*(#|$)", line):
                    mat = re.search(r"^\s*#\s*dataDir:\s*(\S+)", line)
                    if mat:
                        dd = mat.group(1)
                    continue
                
                v, t = re.split(r"\s+", line, 1)
                t = t.replace("\t", " ")
                desires.append([dd, v, t])
    else:
        if not args.visit:
            print >> sys.stderr, "Please choose a visit"
            sys.exit(1)

        desires = [[args.dataDir, v, args.rerun if args.rerun else ""] for v in args.visit]

    for dataDir, visit, title in desires:
        if args.verbose:
            print "%-10s %s" % (visit, title)

        if args.correctDistortion:
            if title:
                title += " "
            title += "(corrected)"

        plt = main(dataDir, visit, title, args.outputTxtFile,
                   gridPoints=args.gridPoints,
                   showFwhm=args.showFwhm, minFwhm=args.minFwhm, maxFwhm=args.maxFwhm,
                   correctDistortion=args.correctDistortion,
                   showEllipticity=args.showEllipticity, ellipticityDirection=args.ellipticityDirection,
                   verbose=args.verbose)
                        
        if pp:
            try:
                pp.savefig()
            except ValueError:          # thrown if we failed to actually plot anything
                pass
            plt.clf()

    if pp:
        pp.close()
    else:
        if args.outputPlotFile:
            plt.savefig(args.outputPlotFile)
        else:
            plt.show()
