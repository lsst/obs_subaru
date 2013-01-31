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

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--create", action="store_true", help="Create new registry (clobber old)?")
parser.add_argument("--root", default=".", help="Root directory")
parser.add_argument("--camera", default="HSC", choices=["HSC", "SC"], help="Camera name")
parser.add_argument("--visits", type=int, nargs="*", help="Visits to process")
parser.add_argument("--fields", nargs="*", help="Fields to process")
args = parser.parse_args()

if args.camera.lower() in ("hsc",):
    mapperPolicy = "HscSimMapper.paf"
    reFilename = r'HSC-(\d{7})-(\d{3}).fits'
    globFilename = "HSC-%07d-???.fits"
elif args.camera.lower() in ("hscsim",):
    mapperPolicy = "HscSimMapper.paf"
    reFilename = r'HSCA(\d{5})(\d{3}).fits'
    globFilename = "HSCA%05d???.fits"
elif args.camera.lower() in ("sc", "suprimecam", "suprime-cam", "sc-mit", "suprimecam-mit"):
    mapperPolicy = "SuprimecamMapper.paf"
    reFilename = r'SUPA(\d{7})(\d).fits'
    globFilename = "SUPA%07d?.fits"
else:
    raise KeyError("Unrecognised camera name: %s" % args.camera)

mapperPolicy = pexPolicy.Policy(os.path.join(os.getenv("OBS_SUBARU_DIR"), "policy", mapperPolicy))
if mapperPolicy.exists("skytiles"):
    skyPolicy = mapperPolicy.getPolicy("skytiles")
else:
    skyPolicy = None


# Gather list of files
def globber(field="*", filename="*.fits"):
    return glob.glob(os.path.join(args.root, field, "*-*-*", "*", "*", filename))
if not args.visits and not args.fields:
    files = globber()
else:
    files = []
    if args.visits:
        for v in args.visits:
            files += globber(filename=globFilename % v)
    if args.fields:
        for f in args.fields:
            files += globber(field=f)
print 'Processing %d files...' % len(files)

registryName = os.path.join(args.root, "registry.sqlite3")
if args.create and os.path.exists(registryName):
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

# Set up regex for parsing directory structure
reField = r'([\w .+-]+)'
reDate = r'(\d{4}-\d{2}-\d{2})'
rePointing = r'(\d+)'
reFilter = r'([\w\-\+]+)'
regex = re.compile('/'.join([reField, reDate, rePointing, reFilter, reFilename]))

qsp = skypix.createQuadSpherePixelization(skyPolicy)
cursor = conn.cursor()
for fits in files:
    m = re.search(regex, fits)
    if not m:
        print "Warning: skipping unrecognized filename:", fits
        continue

    field, dateObs, pointing, filterId, visit, ccd = m.groups()
    visit = int(visit)
    ccd = int(ccd)
    #print "Processing %s" % fits

    try:
        if not args.create:
            cursor.execute("SELECT COUNT(*) FROM raw WHERE visit=? and ccd=?", (visit, ccd))
            if cursor.fetchone()[0] > 0:
                print "File %s is already in the registry --- skipping" % fits
                continue

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

        cursor.execute("INSERT INTO raw VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?)",
                       (field, visit, filterId, ccd, dateObs, taiObs, expTime, pointing))
        ident = cursor.lastrowid

        for skyTileId in pix:
            cursor.execute("INSERT INTO raw_skyTile VALUES(?, ?)", (ident, skyTileId))

        print "Added %s" % fits
    except Exception, e:
        print "Skipping botched %s: %s" % (fits, e)
        continue

# Hmm. Needs better constraints. CPL
print "Adding raw_visits"
conn.execute("""insert or ignore into raw_visit
        select distinct visit, field, filter, dateObs, taiObs, expTime, pointing from raw""")
conn.commit()
conn.close()
