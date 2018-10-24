#!/usr/bin/env python

"""
Refile focus chips, which had incorrect DET-ID values for the Jan/Feb 2013 run
    Old (DET-ID)	Future (Furusawa's note)
    112		106
    104		104
    107		105
    113		107
    115		109
    111		111
    108		110
    114		108
"""
import glob
import os
import re
import shutil

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("root", nargs="?", help="Root directory")
parser.add_argument("--visits", type=int, nargs="*", help="Visits to process")
parser.add_argument("--fields", nargs="*", help="Fields to process")
parser.add_argument("-v", "--verbose", action="store_true", help="Be chattier")
args = parser.parse_args()

if not args.root:
    args.root = os.environ.get("SUPRIME_DATA_DIR", ".")

reFilename = r'HSC-(\d{7})-(\d{3}).fits'
globFilename = "HSC-%07d-???.fits"
#
# Desired remapping
#
remap = {112: 106,
         # 104: 104
         107: 105,
         113: 107,
         115: 109,
         # 111: 111,
         108: 110,
         114: 108,
         }

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
print('Processing %d files...' % len(files))

# Set up regex for parsing directory structure
reField = r'([\w .+-]+)'
reDate = r'(\d{4}-\d{2}-\d{2})'
rePointing = r'(\d+)'
reFilter = r'([\w\-\+]+)'
regex = re.compile('/'.join([reField, reDate, rePointing, reFilter, reFilename]))

visits = {}
fileNames = {}
for fileNo, fits in enumerate(files):
    m = re.search(regex, fits)
    if not m:
        print("Warning: skipping unrecognized filename:", fits)
        continue

    field, dateObs, pointing, filterId, visit, ccd = m.groups()
    visit = int(visit)
    ccd = int(ccd)
    if visit not in visits:
        visits[visit] = set()
    visits[visit].add(ccd)
    # print "Processing %s" % fits
    fileNames[(visit, ccd)] = fits

# Ignore visits that have already been fixed
for visit, ccds in visits.items():
    if 105 in ccds:                     # already refiled
        print("%d is already refiled" % visit)
        del visits[visit]
        continue

    if set(remap.keys()) != set([_ for _ in ccds if remap.get(_)]):  # no focus chips are available
        continue

    print(visit)
    try:
        tmpDir = None
        for old, new in remap.items():
            oldFile = fileNames[(visit, old)]
            newFile = re.sub(r"%03d\.fits" % old, "%03d.fits" % new, oldFile)
            if not tmpDir:
                tmpDir = os.path.join(os.path.split(oldFile)[0], "tmp")
                if not os.path.isdir(tmpDir):
                    os.makedirs(tmpDir)

            shutil.copyfile(oldFile, os.path.join(tmpDir, os.path.split(newFile)[1]))

        for f in glob.glob(os.path.join(tmpDir, "*.fits")):
            os.rename(f, os.path.join(os.path.split(tmpDir)[0], os.path.split(f)[1]))
    except Exception:
        raise
    finally:
        if tmpDir:
            os.rmdir(tmpDir)
#
#
