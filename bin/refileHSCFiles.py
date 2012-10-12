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
import datetime

parser = optparse.OptionParser(usage="%prog [options] filenames")
parser.add_option("-x", "--execute", action="store_true", dest="execute",
                  default=False, help="actually execute the renames")
parser.add_option("-c", "--copy", action="store_true", dest="doCopy",
                  default=False, help="copy instead of move the files.")
parser.add_option("-l", "--link", action="store_true", dest="doLink",
                  default=False, help="link instead of move the files.")
parser.add_option("-r", "--root", action="store", dest="rootDir",
                  default=None, help="root directory for output files.")
parser.add_option("-v", "--visit", action="store", dest="visitID",
                  default=False, help="set the visit ID.")
parser.add_option("-f", "--frameID", action="store", dest="frameID",
                  default=False, help="set the frame number.")
parser.add_option("-d", "--date", action="store", dest="expDate",
                  default=False, help="set the exposure time.")

def datetime2mjd(date_time):

    YY = date_time.year
    MO = date_time.month
    DD = date_time.day
    HH = date_time.hour
    MI = date_time.minute
    SS = date_time.second

    if MO == 1 or MO == 2:
        mm = MO + 12
        yy = YY - 1
    else:
        mm = MO
        yy = YY

    dd = DD + (HH/24.0 + MI/24.0/60.0 + SS/24.0/3600.0)

    A = int(365.25*yy);
    B = int(yy/400.0);
    C = int(yy/100.0);
    D = int(30.59*(mm-2));

    mjd = A + B -C + D  + dd - 678912;

    return mjd

def getFrameInfo(filename):
    """ Read what we care about in the SuprimeCam headers. """

    d = {}
    h = pyfits.getheader(filename)

    # This program only deals with SUPA data.
    inst = h.get('INSTRUME', 'Unknown')
    if inst == 'Unknown':
        sys.stderr.write("%s: INSTRUME card is not defined\n" % (filename))

    if opts.frameID:
        d['frameID'] = "HSCA%05d%03d" % (int(opts.frameID), h['DET-ID']) # FH. this may not match real frameId assign, so to be written if necessary
    else:
        d['frameID'] = h['FRAMEID']
    if not re.search('^HSC[A-Z]\d{8}', d['frameID']):
        raise SystemExit("frameID=%s for %s is invalid" % (d['frameID'], filename))

    d['progID'] = h['OBJECT'].upper()
    d['filterName'] = h['FILTER01'].upper()
    d['date'] = opts.expDate if opts.expDate else h.get('DATE-OBS')

    # visit_Id (i.e., pointing in registry) is derived from MJD
    mjd =  h.get('MJD')
    dateobs =  d['date']
    if not dateobs:
        sys.stderr.write("%s: DATE-OBS is not defined.\n" % (filename))
        sys.exit(-1)

    day0 = 55927  # 2012-01-01  51544 -> 2000-01-01
    if mjd and int(mjd) > day0:  # this handling should be consistent with hscDb.regist_raw_Hsc
        d['visitID'] = int(mjd) - day0
    elif dateobs: # if MJD is unavailable in simulation data, convert from date-obs
        sys.stderr.write("%s: MJD card is not defined. DATE-OBS is converted to MJD.\n" % (filename))
        m = re.search(r'(\d{4})-(\d{2})-(\d{2})', dateobs)
        year, month, day = m.groups()
        obsday = datetime.datetime(int(year), int(month), int(day), 0, 0, 0)
        mjd = datetime2mjd(obsday)
        d['visitID'] = int(mjd) - day0
    else: # if neither mjd and dateobs available (for old HscSim data)
        sys.stderr.write("%s: MJD card is not defined. FRAMEID is used to derive visitID.\n" % (filename))
        fieldID = d['frameID'][4:8]
        d['visitID'] = int(fieldID)

    # extracting visit, ccd from frameId

    m = re.search(r'HSCA(\d{6})(\d{2})', d['frameID'])
    cycle_num = 0 ## 1st cycle of frame number
    if not m:
         m = re.search(r'HSCB(\d{6})(\d{2})', d['frameID'])
         cycle_num = 1 ## 2nd cycle of frame number
         if not m:
             m = re.search(r'HSCC(\d{6})(\d{2})', d['frameID'])
             cycle_num = 2 ## 3rd cycle of frame number
             if not m:
                 sys.stderr.write("Error: Unrecognized Frame ID in FITS Header...:", filename)
                 sys.exit(-1)

    start6, last2 = m.groups() # HSCA[start6][last2]
    ccd_int = int(h.get('DET-ID'))
    if ccd_int >= 100:
        int_visit_base = int(start6) - 1
    else:
        int_visit_base = int(start6)

    frame_num = int(start6)*100 + int(last2) + 100000000*cycle_num   ### Integer corresponding to FRAME-ID + 'A','B','C' etc..  ###
    visit = "%07d" % (int_visit_base + cycle_num*1000000)

    d['visit'] = int(visit)
    d['ccd'] = ccd_int
    d['id'] = frame_num

    return d

def getFinalFile(rootDir, filename, info):
    """ Return the directory and filename from the passed in info dictionary. """
    
    path = os.path.join(rootDir, 
                        info['progID'], 
                        info['date'],
                        '%05d' % info['visitID'])
    if info['progID'] not in ('BIAS', 'DARK'):
        path = os.path.join(path, info['filterName'])

    root_new = "HSC-%07d-%03d" % (int(info['visit']), int(info['ccd']))
    return path, root_new + '.fits'


def main():
    global opts, args

    (opts, args) = parser.parse_args()

    if opts.doCopy and opts.doLink:
        sys.stderr.write("May only set one of --copy and --link")
        raise SystemExit()

    rootDir = opts.rootDir if opts.rootDir else os.environ.get('SUPRIME_DATA_DIR', None)
    if not rootDir:
        sys.stderr.write("Neither --root or $SUPRIME_DATA_DIR are set!\n")
        raise SystemExit()

    # We only import SuprimeCam data.
    rootDir = os.path.join(rootDir, 'HSC')
    
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
