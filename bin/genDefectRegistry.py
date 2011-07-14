#!/usr/bin/env python

import glob
import os
import re
import sqlite
import sys

if os.path.exists("defectRegistry.sqlite3"):
    os.unlink("defectRegistry.sqlite3")
conn = sqlite.connect("defectRegistry.sqlite3")

cmd = "create table defect (id integer primary key autoincrement"
cmd += ", path text, version text, ccdSerial int"
cmd += ", validStart text, validEnd text)"
conn.execute(cmd)
conn.commit()

cmd = "INSERT INTO defect VALUES (NULL, ?, ?, ?, ?, ?)"

for f in glob.glob("*/defects*.fits"):
    m = re.search(r'(\S+)/defects_(\d+)\.fits', f)
    if not m:
        print >>sys.stderr, "Unrecognized file: %s" % (f,)
        continue
    print f
    conn.execute(cmd, (f, m.group(1), int(m.group(2)), "1970-01-01", "2037-12-31"))
conn.commit()
conn.close()

