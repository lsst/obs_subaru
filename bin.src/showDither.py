#!/usr/bin/env python
from __future__ import absolute_import, division, print_function
from builtins import zip
from builtins import input
from builtins import range
import argparse
import fnmatch
import os
import re
import sys

import numpy as np
import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as afwCG
import lsst.daf.persistence as dafPersist
try:
    plt
except NameError:
    import matplotlib.pyplot as plt
    from matplotlib.patches import Circle, PathPatch
    from matplotlib.path import Path

    plt.interactive(1)


def main(butler, visits, fields, fieldRadius, showCCDs=False, aitoff=False, alpha=0.2,
         byFilter=False, byVisit=False, title="", verbose=False):
    ra, dec = [], []
    filters = {}
    _visits, visits = visits, []
    for v in _visits:
        try:
            exp = butler.get("raw", visit=v, ccd=49)
        except RuntimeError as e:
            if verbose:
                print(e, file=sys.stderr)
            continue

        ccd = exp.getDetector()

        xy = ccd.getPixelFromPosition(afwCG.FpPoint(0, 0))
        sky = exp.getWcs().pixelToSky(xy)
        visits.append(v)
        ra.append(sky[0].asDegrees())
        dec.append(sky[1].asDegrees())
        filters[v] = exp.getFilter().getName()

    plt.clf()
    if aitoff:
        axes = plt.gcf().add_axes((0.1, 0.1, 0.85, 0.80), projection="aitoff")
        axes.grid(1)
    else:
        axes = plt.gca()
        axes.set_aspect('equal')

    ctypes = dict(BIAS="orange",
                  DARK="cyan",
                  )
    if byFilter:
        ctypeFilters = dict(g="green",
                            r="red",
                            r1="orange",
                            i="magenta",
                            z="brown",
                            y="black",
                            nb0921='darkgray',
                            )
    else:
        ctypeKeys = {}
        colors = list("rgbcmyk") + ["orange", "brown", "orchid"]  # colours for ctypeKeys

    if aitoff:
        fieldRadius = np.radians(fieldRadius)
        dec = np.radians(dec)
        ra = np.array(ra)
        ra = np.radians(np.where(ra > 180, ra - 360, ra))

    plots, labels = [], []
    for v, r, d in zip(visits, ra, dec):
        field = fields.get(v)
        if verbose:
            print("Drawing %s %s         \r" % (v, field), end=' ', flush=True)

        if byFilter:
            facecolor = ctypes.get(field, ctypeFilters.get(filters[v], "gray"))
        else:
            key = v if byVisit else field
            if key not in ctypeKeys:
                ctypeKeys[key] = colors[len(ctypeKeys)%len(colors)]
            facecolor = ctypeKeys[key]

        circ = Circle(xy=(r, d), radius=fieldRadius, fill=False if showCCDs else True,
                      facecolor=facecolor, alpha=alpha)
        axes.add_artist(circ)

        if showCCDs:
            pathCodes = [Path.MOVETO, Path.LINETO, Path.LINETO, Path.LINETO, Path.CLOSEPOLY, ]

            for ccd in butler.queryMetadata("raw", "visit", ["ccd"], visit=v):
                try:
                    md = butler.get("raw_md", visit=v, ccd=ccd)
                except RuntimeError as e:
                    if verbose:
                        print(e, file=sys.stderr)
                    continue

                width, height = md.get("NAXIS1"), md.get("NAXIS2")
                wcs = afwGeom.makeSkyWcs(md)

                verts = []
                for p in [(0, 0), (width, 0), (width, height), (0, height)]:
                    sky = wcs.pixelToSky(afwGeom.PointD(*p))
                    verts.append([sky[0].asDegrees(), sky[1].asDegrees()])
                verts.append((0, 0))    # dummy

                axes.add_patch(PathPatch(Path(verts, pathCodes), alpha=alpha, facecolor=facecolor))

        if byFilter:
            key = filters[v]
        else:
            key = v if byVisit else field

        if not labels.count(key):
            plots.append(Circle((0, 0), facecolor=facecolor))
            labels.append(key)

    plt.legend(plots, labels,
               loc='best', bbox_to_anchor=(0, 1.02, 1, 0.102)).draggable()

    if not aitoff:
        raRange = np.max(ra) - np.min(ra) + 1.2*fieldRadius
        decRange = np.max(dec) - np.min(dec) + 1.2*fieldRadius
        raRange *= np.cos(np.radians(np.mean(dec)))

        plt.xlim(0.5*(np.max(ra) + np.min(ra)) + raRange*np.array((1, -1)))
        plt.ylim(0.5*(np.max(dec) + np.min(dec)) + decRange*np.array((-1, 1)))

    plt.xlabel("ra")
    plt.ylabel("dec")

    return plt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""Plot dither pattern from a set of visits;
The intersection of the desired visit/field/dateObs is used.

E.g.
   showDither.py /data/data1/Subaru/HSC --field STRIPE82L
   showDither.py --dateObs=2013-11-02 --exclude BIAS *FOCUS* --visits 904086..904999:2

""")

    parser.add_argument('dataDir', type=str, nargs="?", help='Directory to search for data')
    parser.add_argument('--visits', type=str,
                        help='Desired visits (you may use .. and ^ as in pipe_base --id)')
    parser.add_argument('--field', type=str, help='Desired field (e.g. STRIPE82L)')
    parser.add_argument('--filter', type=str, help='Desired filter (e.g. g or HSC-R)')
    parser.add_argument('--exclude', type=str, nargs="*",
                        help='Exclude these types of  field (e.g. BIAS DARK *FOCUS*)')
    parser.add_argument('--dateObs', type=str, nargs="*", help='Desired date[s] (e.g. 2013-06-15 2013-06-16)')
    parser.add_argument('--fileName', type=str, help='File to save plot to')
    parser.add_argument('--alpha', type=float, help='Transparency of observed regions', default=0.2)
    parser.add_argument('--fieldRadius', type=float, help='Radius of usable field (degrees)', default=0.75)
    parser.add_argument('--showCCDs', action="store_true",
                        help="Show the individual CCDs (quite slow)", default=False)
    parser.add_argument('--byFilter', action="store_true",
                        help="Colour pointings by their filter", default=False)
    parser.add_argument('--byVisit', action="store_true",
                        help="Colour pointings by their visit", default=False)
    parser.add_argument('--aitoff', action="store_true", help="Use an Aitoff projection", default=False)
    parser.add_argument('--verbose', action="store_true", help="Be chatty", default=False)

    args = parser.parse_args()

    if not args.dataDir:
        args.dataDir = os.environ.get("SUPRIME_DATA_DIR")
        if not args.dataDir:
            print("Please specify a dataDir (maybe in an inputFile) or set $SUPRIME_DATA_DIR",
                  file=sys.stderr)
            sys.exit(1)

    try:
        butler = dafPersist.Butler(args.dataDir)
    except RuntimeError as e:
        print("I couldn't instantiate a butler with root %s: %s" % (args.dataDir, e), file=sys.stderr)
        sys.exit(1)

    if not (args.visits or args.field or args.dateObs):
        print("Please choose a visit, field, and/or dateObs (and maybe a filter)", file=sys.stderr)
        sys.exit(1)

    visits = []
    if args.visits:
        for v in args.visits.split("^"):
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
        #
        # check that they really exist
        #
        visits = list(set(visits) & set(butler.queryMetadata("raw", "visit", ccd=0)))

    if args.dateObs or args.field or args.filter:
        query = {}
        if args.field:
            query["field"] = args.field

        if args.filter:
            if args.filter in "grizy":
                args.filter = "HSC-%s" % args.filter.upper()

            query["filter"] = args.filter

        _visits = []
        if args.dateObs:
            for d in args.dateObs:
                query["dateObs"] = d
                _visits += butler.queryMetadata("raw", "visit", **query)
        else:
            _visits = butler.queryMetadata("raw", "visit", **query)

        if visits:
            visits = list(set(visits) & set(_visits))
        else:
            visits = _visits

    fields = dict(butler.queryMetadata("raw", "visit", ["visit", "field"]))
    if args.exclude:
        _visits, visits = visits, []
        for v in _visits:
            keep = True
            field = fields.get(v)
            for i in args.exclude:
                if fnmatch.fnmatch(fields[v], i):
                    keep = False
                    break
            if keep:
                visits.append(v)

    if not visits:
        print("You haven't provided any visits for me to process", file=sys.stderr)
        sys.exit(1)

    fields = dict([(v, f) for v, f in fields.items() if v in visits])
    visits = sorted(fields.keys())

    if args.verbose:
        for v in visits:
            print(v, fields[v])

    fig = main(butler, visits, fields, args.fieldRadius, byFilter=args.byFilter, byVisit=args.byVisit,
               showCCDs=args.showCCDs, aitoff=args.aitoff, alpha=args.alpha, title="",
               verbose=args.verbose)

    if args.verbose:
        print("                          \r", end=' ', flush=True)

    if args.fileName:
        plt.savefig(args.fileName)
    else:
        plt.interactive(1)
        input("Exit? ")
