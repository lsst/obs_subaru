#!/usr/bin/env python

""" Rename raw SuprimeCam files to the currently agreed on layout:

/data1/SUPA/$PROGRAM_ID/$DATE/$VISIT_ID/$FILTER/$FRAMEID.fits

   $PROGRAM_ID    - a string, currently the 'OBJECT' FITS card.
   $VISIT_ID      - a monotonically increasing ID, %05d, currently the day number since 2008-07-21 UT.
                    For HSC, this is expected to be incremented each time the telescope is slewed
                    to a new field.
   $DATE          - an ISO date, currently from the 'DATE-OBS' FITS card. Should be UTC.
   $FILTER        - a string, currently from the 'FILTER01' FITS card.
   $FRAMEID       - currently the header's FRAMEID

 - Bias and Dark files are saved with PROGRAM_ID of "BIAS" or "DARK", and no filter element.
 - All input files must come from SuprimeCam, as determined by the INSTRUME card.
  
"""

import os
import sys
import pyfits
import shutil
import optparse

parser = optparse.OptionParser(usage="%prog [options] filenames")
parser.add_option("-x", "--execute", action="store_true", dest="execute",
                  default=False, help="actually execute the renames")
parser.add_option("-r", "--root", action="store", dest="rootDir",
                  default=None, help="root directory for output files.")
parser.add_option("-c", "--copy", action="store_true", dest="doCopy",
                  default=False, help="copy instead of move the files.")
parser.add_option("-l", "--link", action="store_true", dest="doLink",
                  default=False, help="link instead of move the files.")

def getFrameInfo(filename):
    """ Read what we care about in the SuprimeCam headers. """

    d = {}
    h = pyfits.getheader(filename)

    # This program only deals with SUPA data.
    inst = h.get('INSTRUME', 'Unknown')
    if inst != 'SuprimeCam':
        sys.stderr.write("%s: INSTRUME card is not 'SuprimeCam', but %s\n" % (filename, inst))
        raise SystemExit

    d['frameID'] = h['FRAMEID']
    d['progID'] = re.sub(r'\W', '_', h['OBJECT']).upper()
    d['filterName'] = h['FILTER01'].upper()
    d['date'] = h['DATE-OBS']

    # For Suprime-cam data, set visitID to the # of days since 2008-07-21 UT. This should
    # keep it positive, smallish, and monotonically increasing with time.
    #day0 = 54668.0
    day0 = 51544 # 2000-01-01
    d['visitID'] = h['MJD'] - day0
#    if d['visitID'] <= 0 or d['visitID'] > 2000:
#        sys.stderr.write("%s: MJD and visitID are odd (%s and %s). We only handle new SuprimeCam detector images, since MJD %d\n" % \
#                             (filename, h['MJD'], d['visitID'], day0))
#        raise SystemExit

    return d

def getFinalFile(rootDir, filename, info):
    """ Return the directory and filename from the passed in info dictionary. """
    
    path = os.path.join(rootDir, 
                        info['progID'], 
                        info['date'],
                        '%05d' % info['visitID'])
    if info['progID'] in ('BIAS', 'DARK'):
        info['filterName'] = 'NONE'

    path = os.path.join(path, info['filterName'])
    return path, info['frameID'] + '.fits'

def main():
    (opts, args) = parser.parse_args()

    if opts.doCopy and opts.doLink:
        sys.stderr.write("May only set one of --copy and --link")
        raise SystemExit()

    rootDir = opts.rootDir if opts.rootDir else os.environ.get('SUPRIME_DATA_DIR', None)
    if not rootDir:
        sys.stderr.write("Neither --root or $SUPRIME_DATA_DIR are set!\n")
        raise SystemExit()

    # We only import SuprimeCam data.
    rootDir = os.path.join(rootDir, 'SUPA')
    
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
                elif opts.doLink:
                    os.symlink(infile, outpath)
                    print "linked %s to %s" % (infile, outpath)
                else:
                    os.rename(infile, outpath)
                    print "moved %s to %s" % (infile, outpath)
            except Exception, e:
                sys.stderr.write("failed to move or copy %s to %s: %s\n" % (infile, outpath, e))
        else:        
            print infile, outpath

if __name__ == "__main__":
    main()
