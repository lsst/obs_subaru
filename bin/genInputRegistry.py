#!/usr/bin/env python

import glob
import math
import os
import re
import sqlite
import sys
import lsst.daf.base   as dafBase
import lsst.pex.policy as pexPolicy
import lsst.afw.image  as afwImage
import lsst.skypix     as skypix

import argparse

def fixSubaruHeader(md):
    """Fix a Subaru header with a weird mixture of CD and PC terms"""

    haveCD = False                      # have we seen a CD card?
    PC = []
    for i in (1, 2,):
        for j in (1, 2,):
            k = "CD%d_%d" % (i, j)
            if md.exists(k):
                haveCD = True

            for k in ("PC%03d%03d" % (i, j), "PC%d_%d" % (i, j),):
                if md.exists(k):
                    PC.append(k)

    if not haveCD:
        cdelt = {}
        for i in (1, 2,):
            k = "CDELT%d" % i
            cdelt[i] = md.get(k) if k in md.names() else 1.0
                
            for j in (1, 2,):
                md.set("CD%d_%d" % (i, j), cdelt[i]*md.get("PC%d_%d" % (i, j)))

    for k in PC:
        md.remove(k)

    return md

parser = argparse.ArgumentParser()
parser.add_argument("--create", action="store_true", help="Create new registry (clobber old)?")
parser.add_argument("--root", default=".", help="Root directory")
parser.add_argument("--camera", default="HSC", choices=["HSC", "SC"], help="Camera name")
parser.add_argument("--visits", type=int, nargs="*", help="Visits to process")
parser.add_argument("--fields", nargs="*", help="Fields to process")
parser.add_argument("-n", "--dryrun", action="store_true", help="Don't actually do anything", default=False)
parser.add_argument("-v", "--verbose", action="store_true", help="Be chattier")
args = parser.parse_args()

if args.camera.lower() in ("hsc",):
    mapperPolicy = "HscSimMapper.paf"
    reFilename = r'HSC-(\d{7})-(\d{3}).fits'
    globFilename = "HSC-%07d-???.fits"
    mapperName = "lsst.obs.hscSim.HscSimMapper"
elif args.camera.lower() in ("hscsim",):
    mapperPolicy = "HscSimMapper.paf"
    reFilename = r'HSCA(\d{5})(\d{3}).fits'
    globFilename = "HSCA%05d???.fits"
    mapperName = "lsst.obs.hscSim.HscSimMapper"
elif args.camera.lower() in ("sc", "suprimecam", "suprime-cam", "sc-mit", "suprimecam-mit"):
    mapperPolicy = "SuprimecamMapper.paf"
    reFilename = r'SUPA(\d{7})(\d).fits'
    globFilename = "SUPA%07d?.fits"
    mapperName = "lsst.obs.suprimecam.SuprimecamMapper"
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
if args.dryrun:
    conn = None
else:
    conn = sqlite.connect(registryName)
    
if makeTables:
    cmd = "create table raw (id integer primary key autoincrement"
    cmd += ", field text, visit int, filter text, ccd int"
    cmd += ", dateObs text, taiObs text, expTime double, pointing int"
    cmd += ", unique(visit, ccd))"
    if conn:
        conn.execute(cmd)
    else:
        print >> sys.stderr, cmd

    cmd = "create table raw_skyTile (id integer, skyTile integer"
    cmd += ", unique(id, skyTile), foreign key(id) references raw(id))"
    if conn:
        conn.execute(cmd)
    else:
        print >> sys.stderr, cmd

    cmd = "create table raw_visit (visit int, field text, filter text,"
    cmd += "dateObs text, taiObs text, expTime double, pointing int"
    cmd += ", unique(visit))"
    if conn:
        conn.execute(cmd)
        conn.commit()
    else:
        print >> sys.stderr, cmd
        print >> sys.stderr, "commit"

# Set up regex for parsing directory structure
reField = r'([\w .+-]+)'
reDate = r'(\d{4}-\d{2}-\d{2})'
rePointing = r'(\d+)'
reFilter = r'([\w\-\+]+)'
regex = re.compile('/'.join([reField, reDate, rePointing, reFilter, reFilename]))

qsp = skypix.createQuadSpherePixelization(skyPolicy)
if conn:
    cursor = conn.cursor()
else:
    cursor = None
    
nfile = len(files)
for fileNo, fits in enumerate(files):
    m = re.search(regex, fits)
    if not m:
        print "Warning: skipping unrecognized filename:", fits
        continue

    field, dateObs, pointing, filterId, visit, ccd = m.groups()
    visit = int(visit)
    ccd = int(ccd)
    #print "Processing %s" % fits

    if not args.create:
        cmd = "SELECT COUNT(*) FROM raw WHERE visit=? and ccd=?"

        if cursor:
            cursor.execute(cmd, (visit, ccd))
            if cursor.fetchone()[0] > 0:
                if args.verbose:
                    print "File %s is already in the registry --- skipping" % fits
                continue
        else:
            if args.verbose:
                print >> sys.stderr, cmd

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

    if md.get("CTYPE1").strip() == "CPX" and md.get("CTYPE2").strip() == "CPY": # HSC pixel WCS
        pix = []
    else:
        wcs = afwImage.makeWcs(fixSubaruHeader(md))

        try:
            poly = skypix.imageToPolygon(wcs, md.get("NAXIS1"), md.get("NAXIS2"),
                                         padRad=math.radians(15.0/3600))
        except ZeroDivisionError, e:
            print >> sys.stderr, "\nFailure finding polygon for %s: %s" % (fits, e)
            poly = None

        if poly:
            pix = qsp.intersect(poly)
        else:
            pix = []

    cmd = "INSERT INTO raw VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?)"
    if cursor:
        cursor.execute(cmd, (field, visit, filterId, ccd, dateObs, taiObs, expTime, pointing))
        ident = cursor.lastrowid
    else:
        if args.verbose:
            print >> sys.stderr, cmd

    cmd = "INSERT INTO raw_skyTile VALUES(?, ?)"
    if cursor:
        for skyTileId in pix:
            cursor.execute(cmd, (ident, skyTileId))
    else:
        if args.verbose:
            print >> sys.stderr, "for skyTileId in pix: %s" % (cmd)

    print "\rAdded %s [%d %5.1f%%]%20s" % (fits, fileNo + 1, 100*(fileNo + 1)/float(nfile), ""),
    sys.stdout.flush()

print ""
# Hmm. Needs better constraints. CPL
print "Adding raw_visits"

cmd = """
insert or ignore into raw_visit
select distinct visit, field, filter, dateObs, taiObs, expTime, pointing from raw"""
if conn:
    conn.execute(cmd)
    conn.commit()
    conn.close()
else:
    print >> sys.stderr, cmd
    

mapperFile = os.path.join(args.root, "_mapper")
if not os.path.exists(mapperFile):
    f = open(mapperFile, "w")
    f.write(mapperName + "\n")
    del f
