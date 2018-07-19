#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2008-2016 AURA/LSST.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <https://www.lsstcorp.org/LegalNotices/>.
#
import argparse
import os
import sys

try:
    import sqlite3 as sqlite
except ImportError:
    import sqlite


def formatVisits(visits):
    """Format a set of visits into the format used for an --id argument"""
    visits = sorted(visits)

    visitSummary = []
    i = 0
    while i < len(visits):
        v0 = -1

        while i < len(visits):
            v = visits[i]
            i += 1
            if v0 < 0:
                v0 = v
                dv = -1                 # visit stride
                continue

            if dv < 0:
                dv = v - v0

            if visits[i - 2] + dv != v:
                i -= 1                  # process this visit again later
                v = visits[i - 1]       # previous value of v
                break

        if v0 == v:
            vstr = "%d" % v
        else:
            if v == v0 + dv:
                vstr = "%d^%d" % (v0, v)
            else:
                vstr = "%d..%d" % (v0, v)
                if dv > 1:
                    vstr += ":%d" % dv

        visitSummary.append(vstr)

    return "^".join(visitSummary)


def queryRegistry(field=None, visit=None, filterName=None, summary=False):
    """Query an input registry"""
    where = []
    vals = []
    if field:
        where.append('field like ?')
        vals.append(field.replace("*", "%"))
    if filterName:
        where.append('filter like ?')
        vals.append(filterName.replace("*", "%"))
    if visit:
        where.append("visit = ?")
        vals.append(visit)
    where = "WHERE " + " AND ".join(where) if where else ""

    query = """
SELECT max(field), visit, max(filter), max(expTime), max(dateObs), max(pointing), count(ccd)
FROM raw
%s
GROUP BY visit
ORDER BY max(filter), visit
""" % (where)

    n = {}
    expTimes = {}
    visits = {}

    conn = sqlite.connect(registryFile)

    cursor = conn.cursor()

    if args.summary:
        print("%-7s %-20s %7s %s" % ("filter", "field", "expTime", "visit"))
    else:
        print("%-7s %-20s %10s %7s %8s %6s %4s" % ("filter", "field", "dataObs", "expTime",
                                                   "pointing", "visit", "nCCD"))

    ret = cursor.execute(query, vals)

    for line in ret:
        field, visit, filter, expTime, dateObs, pointing, nCCD = line

        if summary:
            k = (filter, field)
            if not n.get(k):
                n[k] = 0
                expTimes[k] = 0
                visits[k] = []

            n[k] += 1
            expTimes[k] += expTime
            visits[k].append(visit)
        else:
            print("%-7s %-20s %10s %7.1f %8d %6d %4d" % (filter, field, dateObs, expTime,
                                                         pointing, visit, nCCD))

    conn.close()

    if summary:
        for k in sorted(n.keys()):
            filter, field = k

            print("%-7s %-20s %7.1f %s" % (filter, field, expTimes[k], formatVisits(visits[k])))


def queryCalibRegistry(what, filterName=None, summary=False):
    """Query a calib registry"""
    where = []
    vals = []

    if filterName:
        where.append('filter like ?')
        vals.append(filterName.replace("*", "%"))
    where = "WHERE " + " AND ".join(where) if where else ""

    query = """
SELECT
    validStart, validEnd, calibDate, filter, calibVersion, count(ccd)
FROM %s
%s
GROUP BY filter, calibDate
ORDER BY filter, calibDate
""" % (what, where)

    conn = sqlite.connect(registryFile)

    cursor = conn.cursor()

    if summary:
        print("No summary is available for calib data", file=sys.stderr)
        return
    else:
        print("%-10s--%-10s  %-10s  %-7s %-24s %4s" % (
            "validStart", "validEnd", "calibDate", "filter", "calibVersion", "nCCD"))

    ret = cursor.execute(query, vals)

    for line in ret:
        validStart, validEnd, calibDate, filter, calibVersion, nCCD = line

        print("%-10s--%-10s  %-10s  %-7s %-24s %4d" % (
            validStart, validEnd, calibDate, filter, calibVersion, nCCD))

    conn.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""
Dump the contents of a registry

If no registry is provided, try $SUPRIME_DATA_DIR
""")

    parser.add_argument('registryFile', type=str, nargs="?", help="The registry or directory in question")
    parser.add_argument('--calib', type=str, help="The registration is a Calib; value is desired product")
    parser.add_argument('--field', type=str, help="Just tell me about this field (may be a glob with *)")
    parser.add_argument('--filter', dest="filterName", type=str,
                        help="Just tell me about this filter (may be a glob with *)")
    parser.add_argument('--verbose', action="store_true", help="How chatty should I be?", default=0)
    parser.add_argument('--visit', type=int, help="Just tell me about this visit")
    parser.add_argument('-s', '--summary', action="store_true", help="Print summary (grouped by field)")

    args = parser.parse_args()

    if not args.registryFile:
        args.registryFile = os.environ.get("SUPRIME_DATA_DIR", "")
        if args.calib:
            args.registryFile = os.path.join(args.registryFile, "CALIB")

    if os.path.exists(args.registryFile) and not os.path.isdir(args.registryFile):
        registryFile = args.registryFile
    else:
        registryFile = os.path.join(args.registryFile,
                                    "calibRegistry.sqlite3" if args.calib else "registry.sqlite3")
        if not os.path.exists(registryFile):
            print("Unable to open %s" % registryFile, file=sys.stderr)
            sys.exit(1)

    if args.verbose:
        print("Reading %s" % registryFile)

    if args.calib:
        queryCalibRegistry(args.calib, args.filterName, args.summary)
    else:
        queryRegistry(args.field, args.visit, args.filterName, args.summary)
