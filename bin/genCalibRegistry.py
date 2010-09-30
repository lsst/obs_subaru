#!/usr/bin/env python

import glob
import os
import re
import sqlite3
import sys
import math

import lsst.daf.base   as dafBase
import lsst.afw.image  as afwImage



root = sys.argv[1]

if os.path.exists("calibRegistry.sqlite3"):
    os.unlink("calibRegistry.sqlite3")
conn = sqlite3.connect("calibRegistry.sqlite3")


# Flats
cmd = "create table flat (id integer primary key autoincrement"
cmd += ", derivedRunId text, runId text, version int"
cmd += ", validStart text, validEnd text"
cmd += ", taiObs text, dateobs text, filter text, mystery text, flatnum int, ccd int"
cmd += ")"
conn.execute(cmd)
conn.commit()

# Not sure what these are for, or if they are strictly necessary yet
derivedRunId = 0
runId = 0
version = 0

for fits in glob.glob(os.path.join(root, "FLAT", "20*-*-*", "W-S-*", "*", "FLAT-*.fits*")):
    print fits
    m = re.search(r'\w+/(\d{4}-\d{2}-\d{2})/(W-S-.{1,3})/(\d+)/\w+-(\d{7})(\d).fits', fits)
    if not m:
        print >>sys.stderr, "Warning: Unrecognized file:", fits
        continue

    dateObs, filter, mystery, flatnum, ccd = m.groups()

    md = afwImage.readMetadata(fits)
    expTime = md.get("EXPTIME")
    mjdObs = md.get("MJD")

    # Convert to local time to get meaningful validStart, validEnd
    localObs = mjdObs - 10.0/24.0
    validStart = math.floor(localObs - 0.5) + 0.5 + 10.0/24.0 # Local midday, in UTC
    validEnd = validStart + 1.0

    # print "%f --> %f --> %f %f" % (mjdObs, localObs, validStart, validEnd)

    taiObs = dafBase.DateTime(mjdObs, dafBase.DateTime.MJD,
            dafBase.DateTime.UTC).toString()[:-1]
    validStart = dafBase.DateTime(validStart, dafBase.DateTime.MJD,
                                  dafBase.DateTime.UTC).toString()[:-1]
    validEnd = dafBase.DateTime(validEnd, dafBase.DateTime.MJD,
                                dafBase.DateTime.UTC).toString()[:-1]

    derivedRunId = runId

    conn.execute("INSERT INTO flat VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (derivedRunId, runId, version, validStart, validEnd,
             taiObs, dateObs, filter, mystery, flatnum, ccd))

conn.commit()
conn.close()


# It seems we don't use other detrend types for Subaru.....
