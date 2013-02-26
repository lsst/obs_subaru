#!/usr/bin/env python
import argparse
import os
import sys
import sqlite

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""
Dump the contents of a registry

If no registry is provided, try $SUPRIME_DATA_DIR
""")

    parser.add_argument('registryFile', type=str, nargs="?", help="The registry or directory in question")
    parser.add_argument('--field', type=str, help="Just tell me about this field (may be a glob with *)")
    parser.add_argument('--visit', type=int, help="Just tell me about this visit")
    parser.add_argument('-s', '--summary', action="store_true", help="Print summary (grouped by field)")
    
    args = parser.parse_args()

    if not args.registryFile:
        args.registryFile = os.environ.get("SUPRIME_DATA_DIR", "")

    if os.path.exists(args.registryFile) and not os.path.isdir(args.registryFile):
        registryFile = args.registryFile
    else:
        registryFile = os.path.join(args.registryFile, "registry.sqlite3")
        if not os.path.exists(registryFile):
            print >> sys.stderr, "Unable to open %s" % args.registryFile
            sys.exit(1)

    conn = sqlite.connect(registryFile)
    cursor = conn.cursor()
    if args.summary:
        print "%-7s %-20s %7s" % ("filter", "field", "expTime")
    else:
        print "%-7s %-20s %6s %7s %3s" % ("filter", "field", "visit", "expTime", "nCCD")

    where = []; vals = []
    if args.field:
        where.append('field like ?')
        vals.append(args.field.replace("*", "%"))
    if args.visit:
        where.append("visit = ?")
        vals.append(args.visit)
    if where:
        where = "WHERE " + " AND ".join(where) if where else ""

    query = """
SELECT field, visit, filter, expTime, count(ccd)
FROM raw
%s
GROUP BY visit
ORDER BY filter
""" % (where)

    n = {}
    expTimes = {}
    for line in cursor.execute(query, vals):
        field, visit, filter, expTime, nCCD = line

        if args.summary:
            k = (filter, field)
            if not n.get(k):
                n[k] = 0
                expTimes[k] = 0

            n[k] += 1
            expTimes[k] += expTime
        else:
            print "%-7s %-20s %6d %7.1f %3d" % (filter, field, visit, expTime, nCCD)

    conn.close()

    if args.summary:
        for k in sorted(n.keys()):
            filter, field = k
            print "%-7s %-20s %7.1f" % (filter, field, expTimes[k])
