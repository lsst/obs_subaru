#!/usr/bin/env python
from __future__ import absolute_import, division, print_function
from builtins import zip
from builtins import range
import argparse
import os
import re
import sys

import numpy as np
from matplotlib.mlab import griddata
import matplotlib.pyplot as plt

import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as cameraGeom
import lsst.daf.persistence as dafPersist
import lsst.obs.hscSim as hscSim


def getNumDataGrids(xArr, yArr, dataArr, xs, ys):
    """
    xs = [[0,0,..,0][1,1,..,1][2,2,..,2],..,[n,n,..,n]]
    ys = [[0,1,..,n][0,1,..,n][0,1,..,n],..,[0,1,..,n]]
    """

    gridWidth = xs[1][0] - xs[0][0]
    gridHeight = ys[0][1] - ys[0][0]
    nx = len(xs)
    ny = len(ys)

    nDataList = []
    for i in range(nx):
        nDataX = []
        for j in range(ny):
            xmin = xs[i][j] - int(gridWidth/2.0)
            xmax = xs[i][j] + int(gridWidth/2.0)
            ymin = ys[i][j] - int(gridHeight/2.0)
            ymax = ys[i][j] + int(gridHeight/2.0)

            ndata = 0
            for xc, yc, v in zip(xArr, yArr, dataArr):
                if xmin <= xc < xmax and ymin <= yc < ymax and (not np.isnan(v)) and v > -9999.0:
                    ndata += 1
            nDataX.append(ndata)

        nDataList.append(nDataX)

    return nDataList


def main(dataDir, visit, title="", outputTxtFileName=None,
         showFwhm=False, minFwhm=None, maxFwhm=None,
         correctDistortion=False,
         showEllipticity=False, ellipticityDirection=False,
         showNdataFwhm=False, showNdataEll=False,
         minNdata=None, maxNdata=None,
         gridPoints=30, verbose=False):

    butler = dafPersist.ButlerFactory(mapper=hscSim.HscSimMapper(root=dataDir)).create()
    camera = butler.get("camera")

    if not (showFwhm or showEllipticity or showNdataFwhm or showNdataEll or outputTxtFileName):
        showFwhm = True
    #
    # Get a dict of cameraGeom::Ccd indexed by serial number
    #
    ccds = {}
    for raft in camera:
        for ccd in raft:
            ccd.setTrimmed(True)
            ccds[ccd.getId().getSerial()] = ccd
    #
    # Read all the tableSeeingMap files, converting their (x, y) to focal plane coordinates
    #
    xArr = []
    yArr = []
    ellArr = []
    fwhmArr = []
    paArr = []
    aArr = []
    bArr = []
    e1Arr = []
    e2Arr = []
    elle1e2Arr = []
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

                x, y, fwhm, ell, pa, a, b = fields[1:8]
                x, y = ccd.getPositionFromPixel(afwGeom.PointD(x, y)).getMm()
                xArr.append(x)
                yArr.append(y)
                ellArr.append(ell)
                fwhmArr.append(fwhm)
                paArr.append(pa)
                aArr.append(a)
                bArr.append(b)
                if len(fields) == 11:
                    e1 = fields[8]
                    e2 = fields[9]
                    elle1e2 = fields[10]
                else:
                    e1 = -9999.
                    e2 = -9999.
                    elle1e2 = -9999.
                e1Arr.append(e1)
                e2Arr.append(e2)
                elle1e2Arr.append(elle1e2)

    xArr = np.array(xArr)
    yArr = np.array(yArr)
    ellArr = np.array(ellArr)
    fwhmArr = np.array(fwhmArr)*0.168   # arcseconds
    paArr = np.radians(np.array(paArr))
    aArr = np.array(aArr)
    bArr = np.array(bArr)

    e1Arr = np.array(e1Arr)
    e2Arr = np.array(e2Arr)
    elle1e2Arr = np.array(elle1e2Arr)

    if correctDistortion:
        import lsst.afw.geom.ellipses as afwEllipses

        dist = camera.getDistortion()
        for i in range(len(aArr)):
            axes = afwEllipses.Axes(aArr[i], bArr[i], paArr[i])
            if False:                                                       # testing only!
                axes = afwEllipses.Axes(1.0, 1.0, np.arctan2(yArr[i], xArr[i]))
            quad = afwGeom.Quadrupole(axes)
            quad = quad.transform(dist.computeQuadrupoleTransform(afwGeom.PointD(xArr[i], yArr[i]), False))
            axes = afwEllipses.Axes(quad)
            aArr[i], bArr[i], paArr[i] = axes.getA(), axes.getB(), axes.getTheta()

        ellArr = 1 - bArr/aArr

    if len(xArr) == 0:
        gridPoints = 0
        xs, ys = [], []
    else:
        N = gridPoints*1j
        extent = [min(xArr), max(xArr), min(yArr), max(yArr)]
        xs, ys = np.mgrid[extent[0]:extent[1]:N, extent[2]:extent[3]:N]

    title = [title, ]

    title.append("\n#")

    if outputTxtFileName:
        f = open(outputTxtFileName, 'w')
        f.write("# %s visit %s\n" % (" ".join(title), visit))
        for x, y, ell, fwhm, pa, a, b, e1, e2, elle1e2 in zip(xArr, yArr, ellArr, fwhmArr, paArr, aArr, bArr, e1Arr, e2Arr, elle1e2Arr):
            f.write('%f %f %f %f %f %f %f %f %f %f\n' % (x, y, ell, fwhm, pa, a, b, e1, e2, elle1e2))

    if showFwhm:
        title.append("FWHM (arcsec)")
        if len(xs) > 0:
            fwhmResampled = griddata(xArr, yArr, fwhmArr, xs, ys)
            plt.imshow(fwhmResampled.T, extent=extent, vmin=minFwhm, vmax=maxFwhm, origin='lower')
            plt.colorbar()

        if outputTxtFileName:

            ndataGrids = getNumDataGrids(xArr, yArr, fwhmArr, xs, ys)

            f = open(outputTxtFileName+'-fwhm-grid.txt', 'w')
            f.write("# %s visit %s\n" % (" ".join(title), visit))
            for xline, yline, fwhmline, ndataline in zip(xs.tolist(), ys.tolist(), fwhmResampled.tolist(), ndataGrids):
                for xx, yy, fwhm, ndata in zip(xline, yline, fwhmline, ndataline):
                    if fwhm is None:
                        fwhm = -9999
                    f.write('%f %f %f %d\n' % (xx, yy, fwhm, ndata))

    elif showEllipticity:
        title.append("Ellipticity")
        scale = 4

        if ellipticityDirection:        # we don't care about the magnitude
            ellArr = 0.1

        u = -ellArr*np.cos(paArr)
        v = -ellArr*np.sin(paArr)
        if gridPoints > 0:
            u = griddata(xArr, yArr, u, xs, ys)
            v = griddata(xArr, yArr, v, xs, ys)
            x, y = xs, ys
        else:
            x, y = xArr, yArr

        Q = plt.quiver(x, y, u, v, scale=scale,
                       pivot="middle",
                       headwidth=0,
                       headlength=0,
                       headaxislength=0,
                       )
        keyLen = 0.10
        if not ellipticityDirection:    # we care about the magnitude
            plt.quiverkey(Q, 0.20, 0.95, keyLen, "e=%g" % keyLen, labelpos='W')

        if outputTxtFileName:
            ndataGrids = getNumDataGrids(xArr, yArr, ellArr, xs, ys)

            f = open(outputTxtFileName+'-ell-grid.txt', 'w')
            f.write("# %s visit %s\n" % (" ".join(title), visit))
            #f.write('# %f %f %f %f %f %f %f\n' % (x, y, ell, fwhm, pa, a, b))
            for xline, yline, uline, vline, ndataline in zip(x.tolist(), y.tolist(), u.tolist(), v.tolist(), ndataGrids):
                for xx, yy, uu, vv, ndata in zip(xline, yline, uline, vline, ndataline):
                    if uu is None:
                        uu = -9999
                    if vv is None:
                        vv = -9999
                    f.write('%f %f %f %f %d\n' % (xx, yy, uu, vv, ndata))

    elif showNdataFwhm:
        title.append("N per fwhm grid")
        if len(xs) > 0:
            ndataGrids = getNumDataGrids(xArr, yArr, fwhmArr, xs, ys)
            plt.imshow(ndataGrids, interpolation='nearest', extent=extent,
                       vmin=minNdata, vmax=maxNdata, origin='lower')
            plt.colorbar()
        else:
            pass

    elif showNdataEll:
        title.append("N per ell grid")
        if len(xs) > 0:
            ndataGrids = getNumDataGrids(xArr, yArr, ellArr, xs, ys)
            plt.imshow(ndataGrids, interpolation='nearest', extent=extent,
                       vmin=minNdata, vmax=maxNdata, origin='lower')
            plt.colorbar()
        else:
            pass

    #plt.plot(xArr, yArr, "r.")
    #plt.plot(xs, ys, "b.")
    plt.axes().set_aspect('equal')
    plt.axis([-20000, 20000, -20000, 20000])

    def frameInfoFrom(filepath):
        import pyfits
        with pyfits.open(filepath) as hdul:
            h = hdul[0].header
            'object=ABELL2163 filter=HSC-I exptime=360.0 alt=62.11143274 azm=202.32265181 hst=(23:40:08.363-23:40:48.546)'
            return 'object=%s filter=%s exptime=%.1f azm=%.2f hst=%s' % (h['OBJECT'], h['FILTER01'], h['EXPTIME'], h['AZIMUTH'], h['HST'])

    title.insert(0, frameInfoFrom(butler.get('raw_filename', {'visit': visit, 'ccd': 0})[0]))
    title.append(r'$\langle$FWHM$\rangle %4.2f$"' % np.median(fwhmArr))
    plt.title("%s visit=%s" % (" ".join(title), visit), fontsize=9)

    return plt

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot contours of PSF quality')

    parser.add_argument('dataDir', type=str, nargs="?", help='Datadir to search for PSF information')
    parser.add_argument('--rerun', type=str, help="Rerun to examine", default=None)
    parser.add_argument('--visit', type=str,
                        help='Name of desired visit (you may use .. and ^ as in pipe_base --id)')
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
    parser.add_argument('--showNdataFwhm', action="store_true",
                        help="Show the num of sources used to make Fwhm grids", default=False)
    parser.add_argument('--showNdataEll', action="store_true",
                        help="Show the num of sources used to make ellipticity grids", default=False)
    parser.add_argument('--minNdata', type=int, help="Minimum N sources to plot", default=None)
    parser.add_argument('--maxNdata', type=int, help="Maximum N sources to plot", default=None)

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
            print("Please specify a dataDir (maybe in an inputFile) or set $SUPRIME_DATA_DIR", file=sys.stderr)
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
            print("Please choose a visit", file=sys.stderr)
            sys.exit(1)

        visits = []
        for v in args.visit.split("^"):
            mat = re.search(r"^(\d+)\.\.(\d+)(?::(\d+))?$", v)
            if mat:
                v1 = int(mat.group(1))
                v2 = int(mat.group(2))
                v3 = mat.group(3)
                v3 = int(v3) if v3 else 1
                for v in range(v1, v2 + 1, v3):
                    visits.append(v)
            else:
                visits.append(int(v))

        desires = [[args.dataDir, v, args.rerun if args.rerun else ""] for v in visits]

    for dataDir, visit, title in desires:
        if args.verbose:
            print("%-10s %s" % (visit, title))

        if args.correctDistortion:
            if title:
                title += " "
            title += "(corrected)"

        plt = main(dataDir, visit, title, args.outputTxtFile,
                   gridPoints=args.gridPoints,
                   showFwhm=args.showFwhm, minFwhm=args.minFwhm, maxFwhm=args.maxFwhm,
                   correctDistortion=args.correctDistortion,
                   showEllipticity=args.showEllipticity, ellipticityDirection=args.ellipticityDirection,
                   showNdataFwhm=args.showNdataFwhm, showNdataEll=args.showNdataEll,
                   minNdata=args.minNdata, maxNdata=args.maxNdata,
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
