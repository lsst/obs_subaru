#!/usr/bin/env python
import argparse, fnmatch, os, re, sys

import numpy as np
import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as afwCG
import lsst.daf.persistence as dafPersist
import lsst.obs.hscSim as hscSim
try:
    plt
except NameError:
    import matplotlib.pyplot as plt
    from matplotlib.patches import Circle

    plt.interactive(1)

def main(butler, visits, fields, title="", aitoff=False, verbose=False):
    camera = butler.get("camera")

    ra, dec = [], []
    filters = {}
    _visits = visits; visits = []
    for v in _visits:
        try:
            exp = butler.get("raw", visit=v, ccd=49)
        except RuntimeError, e:
            if verbose:
                print >> sys.stderr, e
            continue

        ccd = afwCG.cast_Ccd(exp.getDetector())

        xy = ccd.getPixelFromPosition(afwCG.FpPoint(0,0))
        sky = exp.getWcs().pixelToSky(xy)
        visits.append(v)
        ra.append( sky[0].asDegrees())
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
    ctypeFilters = dict(g="green",
                        r="red",
                        i="magenta",
                        z="brown",
                        y="black",
                    )

    fieldRadius = 0.9                   # degrees
    if aitoff:
        fieldRadius = np.radians(fieldRadius)
        dec = np.radians(dec)
        ra = np.array(ra)
        ra  = np.radians(np.where(ra > 180, ra - 360, ra))

    plots, labels = [], []
    for v, r, d in zip(visits, ra, dec):
        field = fields.get(v)

        circ = Circle(xy=(r, d), radius=fieldRadius)
        axes.add_artist(circ)
        circ.set_alpha(0.2)
        circ.set_facecolor(ctypes.get(field, ctypeFilters.get(filters[v], "gray")))

        if not labels.count(field):
            plots.append(circ); labels.append(field)

    plt.legend(plots, labels,
               loc='lower left', bbox_to_anchor=(0, 1.02, 1, 0.102)).draggable()

    if not aitoff:
        raRange = np.max(ra)   - np.min(ra)  + 1.2*fieldRadius
        decRange = np.max(dec) - np.min(dec) + 1.2*fieldRadius
        raRange *= np.cos(np.radians(np.mean(dec)))
        
        plt.xlim(0.5*(np.max(ra)  + np.min(ra))  +  raRange*np.array((1, -1)))
        plt.ylim(0.5*(np.max(dec) + np.min(dec)) + decRange*np.array((-1, 1)))

    plt.xlabel("ra")
    plt.ylabel("dec")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""Plot dither pattern from a set of visits;
The intersection of the desired visit/field/dateObs is used.

E.g.
   showDither.py /data/data1/Subaru/HSC --field STRIPE82L
   showDither.py --dateObs=2013-11-02 --exclude BIAS *FOCUS* --visits 904086..904999:2

""")

    parser.add_argument('dataDir', type=str, nargs="?", help='Directory to search for data')
    parser.add_argument('--visits', type=str, help='Desired visits (you may use .. and ^ as in pipe_base --id)')
    parser.add_argument('--field', type=str, help='Desired field (e.g. STRIPE82L)')
    parser.add_argument('--exclude', type=str, nargs="*",
                        help='Exclude these types of  field (e.g. BIAS DARK *FOCUS*)')
    parser.add_argument('--dateObs', type=str, nargs="*", help='Desired date[s] (e.g. 2013-06-15 2013-06-16)')
    parser.add_argument('--aitoff', action="store_true", help="Use an Aitoff projection", default=False)
    parser.add_argument('--verbose', action="store_true", help="Be chatty", default=False)
    
    args = parser.parse_args()

    if not args.dataDir:
        args.dataDir = os.environ.get("SUPRIME_DATA_DIR")
        if not args.dataDir:
            print >> sys.stderr, "Please specify a dataDir (maybe in an inputFile) or set $SUPRIME_DATA_DIR"
            sys.exit(1)

    butler = dafPersist.Butler(args.dataDir)

    if not (args.visits or args.field or args.dateObs):
        print >> sys.stderr, "Please choose a visit, field, and/or dateObs"
        sys.exit(1)

    visits = []
    if args.visits:
        for v in args.visits.split("^"):
            mat = re.search(r"^(\d+)\.\.(\d+)(?::(\d+))?$", v)
            if mat:
                v1 = int(mat.group(1))
                v2 = int(mat.group(2))
                v3 = mat.group(3); v3 = int(v3) if v3 else 1
                for v in range(v1, v2 + 1, v3):
                    visits.append(v)
            else:
                visits.append(int(v))
        #
        # check that they really exist
        #
        visits = list(set(visits) & set(butler.queryMetadata("raw", "visit", ccd=0)))
 
    if args.dateObs or args.field:
        query = {}
        if args.field:
            query["field"] = args.field

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
        _visits = visits; visits = []
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
        print >> sys.stderr, "You haven't provided any visits for me to process"
        sys.exit(1)

    fields = dict([(v, f) for v, f in fields.items() if v in visits])

    if args.verbose:
        for v in sorted(fields.keys()):
            print v, fields[v]

    main(butler, visits, fields, aitoff=args.aitoff, title="", verbose=args.verbose)

    raw_input("Exit? ")
