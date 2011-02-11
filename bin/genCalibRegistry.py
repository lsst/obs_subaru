#!/usr/bin/env python

import glob
import os
import re
import sqlite
import sys
import math

import lsst.daf.base   as dafBase
import lsst.afw.image  as afwImage

root = sys.argv[1]
registry = os.path.join(root, "calibRegistry.sqlite3")

if os.path.exists(registry):
    os.unlink(registry)
conn = sqlite.connect(registry)


for calib in ('bias', 'dark', 'flat', 'fringe'):

    # Flats
    cmd = "create table " + calib.lower() + " (id integer primary key autoincrement"
    cmd += ", validStart text, validEnd text"
    cmd += ", taiObs text, dateobs text, filter text, mystery text, num int, ccd int"
    cmd += ")"
    conn.execute(cmd)
    conn.commit()

    # ?? This can't match anything in the public directories... CPL
    # Also SUPA vs. HSCA w.r.t. CCD ids
    for fits in glob.glob(os.path.join(root, calib.upper(), "20*-*-*", "?-?-*", "*",
                                       calib.upper() + "-*.fits*")):
        print fits
        m = re.search(r'\w+/(\d{4}-\d{2}-\d{2})/(.-.-.{1,3})/(\d+)/\w+-(\d{7})(\d).fits', fits)
        if not m:
            print >>sys.stderr, "Warning: Unrecognized file:", fits
            continue
        dateObs, filterName, mystery, num, ccd = m.groups()

        if False:
            # Convert to local time to get meaningful validStart, validEnd
            md = afwImage.readMetadata(fits)
            mjdObs = md.get("MJD")
            taiObs = dafBase.DateTime(mjdObs, dafBase.DateTime.MJD,
                                      dafBase.DateTime.UTC).toString()[:-1]
            localObs = mjdObs - 10.0/24.0
            validStart = math.floor(localObs - 0.5) + 0.5 + 10.0/24.0 # Local midday, in UTC
            validEnd = validStart + 1.0
            validStart = dafBase.DateTime(validStart, dafBase.DateTime.MJD,
                                          dafBase.DateTime.UTC).toString()[:-1]
            validEnd = dafBase.DateTime(validEnd, dafBase.DateTime.MJD,
                                        dafBase.DateTime.UTC).toString()[:-1]
        else:
            # Make data valid for (almost) all time
            validStart = "2000-01-01"
            validEnd = "2099-12-31"
            taiObs = dateObs

        # print "%f --> %f --> %f %f" % (mjdObs, localObs, validStart, validEnd)

        conn.execute("INSERT INTO " + calib.lower() + " VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?)",
            (validStart, validEnd, taiObs, dateObs, filterName, mystery, num, ccd))

conn.commit()
conn.close()
