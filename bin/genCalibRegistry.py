#!/usr/bin/env python

import glob
import os
import re
import sqlite
import sys
import math
import datetime

import lsst.daf.base   as dafBase
import lsst.afw.image  as afwImage

import optparse

# Needs optparse w/ --create, etc. CPL
parser = optparse.OptionParser()
parser.add_option("--create", dest="create", default=False, action="store_true", 
                  help="Create new registry (clobber old)?")
parser.add_option("--root", dest="root", default=".", help="Root directory")
parser.add_option("--camera", dest="camera", default="hsc", help="Camera name: HSC|SC")
parser.add_option("--validity", type="int", dest="validity", default=30, help="Calibration validity (days)")
opts, args = parser.parse_args()

if len(args) > 0 or len(sys.argv) == 1:
    print "Unrecognised arguments:", sys.argv[1:]
    parser.print_help()
    sys.exit(1)

if opts.camera.lower() not in ("suprime-cam", "suprimecam", "sc", "hsc"):
    raise RuntimeError("Camera not recognised: %s" % camera)

registry = os.path.join(opts.root, "calibRegistry.sqlite3")

if os.path.exists(registry):
    os.unlink(registry)
conn = sqlite.connect(registry)


for calib in ('bias', 'dark', 'flat', 'fringe'):
    cmd = "create table " + calib.lower() + " (id integer primary key autoincrement"
    cmd += ", validStart text, validEnd text"
    cmd += ", taiObs text, dateobs text, filter text, mystery text, num int, ccd int"
    cmd += ")"
    conn.execute(cmd)
    conn.commit()

    for fits in glob.glob(os.path.join(opts.root, calib.upper(), "20*-*-*", "?-?-*", "*",
                                       calib.upper() + "-*.fits*")):
        print fits
        if opts.camera.lower() in ("suprime-cam", "suprimecam", "sc"):
            m = re.search(r'\w+/(\d{4})-(\d{2})-(\d{2})/(.-.-.{1,3})/(\d+)/\w+-(\d{7})(\d).fits', fits)
        elif opts.camera.lower() in ("hsc"):
            m = re.search(r'\w+/(\d{4})-(\d{2})-(\d{2})/(.-.-.{1,3})/(\d+)/\w+-(\d{5})(\d{3}).fits', fits)
        if not m:
            print >>sys.stderr, "Warning: Unrecognized file:", fits
            continue
        year, month, day, filterName, mystery, num, ccd = m.groups()

        dateObs = '%s-%s-%s' % (year, month, day)
        taiObs = dateObs

        date = datetime.date(int(year), int(month), int(day))
        validStart = (date - datetime.timedelta(opts.validity)).isoformat()
        validEnd = (date + datetime.timedelta(opts.validity)).isoformat()

        # print "%f --> %f %f" % (dateObs, validStart, validEnd)

        conn.execute("INSERT INTO " + calib.lower() + " VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?)",
            (validStart, validEnd, taiObs, dateObs, filterName, mystery, num, ccd))

conn.commit()
conn.close()
