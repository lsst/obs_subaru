#!/usr/bin/env python

import glob
import os
import re
import sqlite3
import sys
import lsst.daf.base   as dafBase
import lsst.afw.image  as afwImage
import lsst.skypix     as skypix

if os.path.exists("registry.sqlite3"):
    os.unlink("registry.sqlite3")
conn = sqlite3.connect("registry.sqlite3")

cmd = "create table raw (id integer primary key autoincrement"
cmd += ", field text, visit int, filter text, ccd int"
cmd += ", dateObs text, taiObs text, expTime double, mystery text"
cmd += ", unique(visit, ccd))"
conn.execute(cmd)

cmd = "create table raw_skyTile (id integer, skyTile integer"
cmd += ", unique(id, skyTile), foreign key(id) references raw(id))"
conn.execute(cmd)

cmd = "create table raw_visit (visit int, field text, filter text,"
cmd += "dateObs text, taiObs text, expTime double, mystery text"
cmd += ", unique(visit))"
conn.execute(cmd)

conn.commit()

root = sys.argv[1]
for fits in glob.glob(os.path.join(root, "*", "20*-*-*", "*", "W-S-*", "SUPA*.fits*")):
    print fits
    m = re.search(r'(\w+)/(\d{4}-\d{2}-\d{2})/(\d+)/(W-S-.{1,3})/SUPA(\d{7})(\d).fits', fits)
    if not m:
        print >>sys.stderr, "Warning: Unrecognized file:", fits
        continue

    field, dateObs, mystery, filter, visit, ccd = m.groups()

    md = afwImage.readMetadata(fits)
    expTime = md.get("EXPTIME")
    mjdObs = md.get("MJD")
    taiObs = dafBase.DateTime(mjdObs, dafBase.DateTime.MJD,
            dafBase.DateTime.UTC).toString()[:-1]
    conn.execute("INSERT INTO raw VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?)",
            (field, visit, filter, ccd, dateObs, taiObs, expTime, mystery))

    for row in conn.execute("SELECT last_insert_rowid()"):
        id = row[0]
        break

    wcs = afwImage.makeWcs(md)
    poly = skypix.imageToPolygon(wcs, md.get("NAXIS1"), md.get("NAXIS2"),
            padRad=0.000075) # about 15 arcsec
    qsp = skypix.createQuadSpherePixelization()
    pix = qsp.intersect(poly)
    for skyTileId in pix:
        conn.execute("INSERT INTO raw_skyTile VALUES(?, ?)",
                (id, skyTileId))

conn.execute("""insert into raw_visit
        select distinct visit, field, filter, dateObs, taiObs, expTime, mystery from raw""")
conn.commit()
conn.close()
