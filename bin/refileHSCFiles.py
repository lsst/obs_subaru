#!/usr/bin/env python

""" Rename raw HSC files to the currently agreed on layout:

/data1/HSC/$PROGRAM_ID/$DATE/$VISIT_ID/$FILTER/$FRAMEID.fits

   $PROGRAM_ID    - a string, currently the 'OBJECT' FITS card.
   $VISIT_ID      - a monotonically increasing ID, %05d, currently the day number since 2008-07-21 UT.
                    For HSC, this is expected to be incremented each time the telescope is slewed
                    to a new field. Can also be set on the command line.
   $DATE          - an ISO date, currently from the 'DATE-OBS' FITS card. Should be UTC.
   $FILTER        - a string, currently from the 'FILTER01' FITS card.
   $FRAMEID       - currently the header's FRAMEID or per the command line

 - Bias and Dark files are saved with PROGRAM_ID of "BIAS" or "DARK", and no filter element.
 - All input files must come from SuprimeCam, as determined by the INSTRUME card.
  
"""

import os
import re
import sys
import pyfits
import shutil
import optparse

parser = optparse.OptionParser(usage="%prog [options] filenames")
parser.add_option("-x", "--execute", action="store_true", dest="execute",
                  default=False, help="actually execute the renames")
parser.add_option("-c", "--copy", action="store_true", dest="doCopy",
                  default=False, help="copy instead of move the files.")
parser.add_option("-r", "--root", action="store", dest="rootDir",
                  default=None, help="root directory for output files.")
parser.add_option("-v", "--visit", action="store", dest="visitID",
                  default=False, help="set the visit ID.")
parser.add_option("-f", "--frameID", action="store", dest="frameID",
                  default=False, help="set the frame number.")
parser.add_option("-d", "--date", action="store", dest="expDate",
                  default=False, help="set the exposure time.")

def getFrameInfo(filename):
    """ Read what we care about in the SuprimeCam headers. """

    d = {}
    h = pyfits.getheader(filename)

    # This program only deals with SUPA data.
    inst = h.get('INSTRUME', 'Unknown')
    if inst == 'Unknown':
        sys.stderr.write("%s: INSTRUME card is not defined\n" % (filename))

    if opts.frameID:
        d['frameID'] = "HSC%05d%03d" % (int(opts.frameID), h['DET-ID'])
    else:
        d['frameID'] = h['FRAMEID']
    if not re.search('^HSCA\d{8}', d['frameID']):
        raise SystemExit("frameID=%s for %s is invalid" % (d['frameID'], filename))
    
    d['progID'] = h['OBJECT'].upper()
    d['filterName'] = h['FILTER01'].upper()
    d['date'] = opts.expDate if opts.expDate else h['DATE-OBS']

    # For HSC data, the visit is encoded in the exposure ID.
    fieldID = d['frameID'][4:8]
    d['visitID'] = int(fieldID)

    return d

def getFinalFile(rootDir, filename, info):
    """ Return the directory and filename from the passed in info dictionary. """
    
    path = os.path.join(rootDir, 
                        info['progID'], 
                        info['date'],
                        '%05d' % info['visitID'])
    if info['progID'] not in ('BIAS', 'DARK'):
        path = os.path.join(path, info['filterName'])

    return path, info['frameID'] + '.fits'


def main():
    global opts, args

    (opts, args) = parser.parse_args()

    rootDir = opts.rootDir if opts.rootDir else os.environ.get('SUPRIME_DATA_DIR', None)
    if not rootDir:
        sys.stderr.write("Neither --root or $SUPRIME_DATA_DIR are set!\n")
        raise SystemExit()

    # We only import SuprimeCam data.
    rootDir = os.path.join(rootDir, 'HSC/SIMS')
    
    for infile in args:
        info = getFrameInfo(infile)
        outdir, outfile = getFinalFile(rootDir, infile, info)
        outpath = os.path.join(outdir, outfile)

        if opts.execute:
            # Actually do the moves. 
            try:
                if not os.path.isdir(outdir):
                    os.makedirs(outdir)
                if opts.doCopy:
                    shutil.copyfile(infile, outpath)
                    print "copied %s to %s" % (infile, outpath)
                else:
                    os.rename(infile, outpath)
                    print "moved %s to %s" % (infile, outpath)
            except Exception, e:
                sys.stderr.write("failed to move or copy %s to %s: %s\n" % (infile, outpath, e))
        else:        
            print infile, outpath

if __name__ == "__main__":
    main()
