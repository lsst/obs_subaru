#!/usr/bin/env python

import glob
import os
import re
import sqlite
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--create", action="store_true", help="Create new registry (clobber old)?")
parser.add_argument("--root", default=".", help="Root directory")
parser.add_argument("-v", "--verbose", action="store_true", help="Be chattier")
args = parser.parse_args()

registryName = os.path.join(args.root, "defectRegistry.sqlite3")
if args.create and os.path.exists(registryName):
    os.unlink(registryName)

makeTables = not os.path.exists(registryName)

conn = sqlite.connect(registryName)

if makeTables:
    cmd = "create table defect (id integer primary key autoincrement"
    cmd += ", path text, version text, ccdSerial int"
    cmd += ", validStart text, validEnd text)"
    conn.execute(cmd)
    conn.commit()

cmd = "INSERT INTO defect VALUES (NULL, ?, ?, ?, ?, ?)"

for f in glob.glob(os.path.join(args.root, "*", "defects*.fits")):
    m = re.search(r'(\S+)/defects_(\d+)\.fits', f)
    if not m:
        print >>sys.stderr, "Unrecognized file: %s" % (f,)
        continue
    #
    # Convert f to be relative to the location of the registry
    #
    f = os.path.relpath(f, args.root)

    if args.verbose:
        print "Registering %s" % f
    conn.execute(cmd, (f, m.group(1), int(m.group(2)), "1970-01-01", "2037-12-31"))
conn.commit()
conn.close()

