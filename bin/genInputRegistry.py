#!/usr/bin/env python

import glob
import os
import re
import sqlite
import sys
import lsst.daf.base   as dafBase
import lsst.pex.policy as pexPolicy
import lsst.afw.image  as afwImage
import lsst.skypix     as skypix

import optparse

# Needs optparse w/ --create, etc. CPL
parser = optparse.OptionParser()
parser.add_option("--create", dest="create", default=False, action="store_true", 
                  help="Create new registry (clobber old)?")
parser.add_option("--root", dest="root", default=".", help="Root directory")
parser.add_option("--camera", dest="camera", default="hsc", help="Camera name: HSC|SC")
opts, args = parser.parse_args()

if len(args) > 0 or len(sys.argv) == 1:
    print "Unrecognised arguments:", sys.argv
    parser.print_help()
    sys.exit(1)

if opts.camera.lower() in ("hsc"):
    mapperPolicy = "HscSimMapper.paf"
elif opts.camera.lower() in ("sc", "suprimecam", "suprime-cam"):
    mapperPolicy = "SuprimecamMapper.paf"
else:
    raise KeyError("Unrecognised camera name: %s" % opts.camera)

mapperPolicy = pexPolicy.Policy(os.path.join(os.getenv("OBS_SUBARU_DIR"), "policy", mapperPolicy))
if mapperPolicy.exists("skytiles"):
    skyPolicy = mapperPolicy.getPolicy("skytiles")
else:
    skyPolicy = None

root = opts.root
if root == '-':
    files = [l.strip() for l in sys.stdin.readlines()]
else:
    files = glob.glob(os.path.join(root, "*", "*-*-*", "*", "*", "*.fits"))
sys.stderr.write('processing %d files...\n' % (len(files)))

registryName = "registry.sqlite3"
if opts.create and os.path.exists(registryName):
    os.unlink(registryName)

makeTables = not os.path.exists(registryName)
conn = sqlite.connect(registryName)
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

qsp = skypix.createQuadSpherePixelization(skyPolicy)
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
