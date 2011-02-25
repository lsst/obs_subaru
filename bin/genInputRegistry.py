#!/usr/bin/env python

import glob
import os
import re
import sqlite
import sys
import lsst.daf.base   as dafBase
import lsst.afw.image  as afwImage
import lsst.skypix     as skypix

# Needs optparse w/ --create, etc. CPL
root = sys.argv[1]
if root == '-':
    files = [l.strip() for l in sys.stdin.readlines()]
else:
    files = glob.glob(os.path.join(root, "*", "*-*-*", "*", "*", "*.fits"))
sys.stderr.write('processing %d files...\n' % (len(files)))

makeTables = not os.path.exists("registry.sqlite3")
conn = sqlite.connect("registry.sqlite3")
if makeTables:
    cmd = "create table raw (id integer primary key autoincrement"
    cmd += ", field text, visit int, filter text, ccd int"
    cmd += ", dateObs text, taiObs text, expTime double, pointing int"
    cmd += ", unique(visit, ccd))"
    conn.execute(cmd)

    cmd = "create table raw_skyTile (id integer, skyTile integer"
    cmd += ", unique(id, skyTile), foreign key(id) references raw(id))"
    conn.execute(cmd)

    cmd = "create table raw_visit (visit int, field text, filter text,"
    cmd += "dateObs text, taiObs text, expTime double, pointing int"
    cmd += ", unique(visit))"
    conn.execute(cmd)
    conn.commit()

for fits in files:
    m = re.search(r'([\w+-]+)/(\d{4}-\d{2}-\d{2})/(\d+)/([\w\-\+]+)/SUPA(\d{7})(\d).fits', fits)
    if not m:
        m = re.search(r'([\w+-]+)/(\d{4}-\d{2}-\d{2})/(\d+)/([\w\-\+]+)/HSCA(\d{5})(\d{3}).fits', fits)
    if not m:
        print >>sys.stderr, "Warning: skipping unrecognized filename:", fits
        continue

    field, dateObs, pointing, filterId, visit, ccd = m.groups()
    sys.stderr.write("Processing %s\n" % (fits))

    try:
        md = afwImage.readMetadata(fits)
        expTime = md.get("EXPTIME")

        # Mmmph. Need to make sure that MJD/DATE-OBS agree with the file $dateObs
        # For now use the filename, damnit. CPL
        if True:
            # .toString? Really? CPL
            taiObs = dafBase.DateTime(dateObs + "T00:00:00Z").toString()[:-1]
        else:
            try:
                mjdObs = md.get("MJD")
                taiObs = dafBase.DateTime(mjdObs, dafBase.DateTime.MJD,
                                          dafBase.DateTime.UTC).toString()[:-1]
            except:
                dateObsCard = md.get("DATE-OBS")
                if dateObsCard.find('T') == -1:
                    dateObsCard += "T00:00:00Z"
                taiObs = dafBase.DateTime(dateObsCard).toString()[:-1]
        
        wcs = afwImage.makeWcs(md)
        poly = skypix.imageToPolygon(wcs, md.get("NAXIS1"), md.get("NAXIS2"),
                                     padRad=0.000075) # about 15 arcsec
        qsp = skypix.createQuadSpherePixelization()
        pix = qsp.intersect(poly)

        conn.execute("INSERT INTO raw VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?)",
                     (field, visit, filterId, ccd, dateObs, taiObs, expTime, pointing))

        for row in conn.execute("SELECT last_insert_rowid()"):
            id = row[0]
            break

        for skyTileId in pix:
            conn.execute("INSERT INTO raw_skyTile VALUES(?, ?)",
                         (id, skyTileId))
    except Exception, e:
        print "skipping botched %s: %s" % (fits, e)
        continue

# Hmm. Needs better constraints. CPL
sys.stderr.write("Adding raw_visits\n")
conn.execute("""insert or ignore into raw_visit
        select distinct visit, field, filter, dateObs, taiObs, expTime, pointing from raw""")
conn.commit()
conn.close()
