from __future__ import absolute_import, division, print_function

import re
import datetime

from lsst.pipe.tasks.ingest import IngestTask, ParseTask, IngestArgumentParser
from lsst.pipe.tasks.ingestCalibs import CalibsParseTask


class HscIngestArgumentParser(IngestArgumentParser):

    def _parseDirectories(self, namespace):
        """Don't do any 'rerun' hacking: we want the raw data to end up in the root directory"""
        namespace.input = namespace.rawInput
        namespace.output = namespace.rawOutput
        namespace.calib = None
        del namespace.rawInput
        del namespace.rawCalib
        del namespace.rawOutput
        del namespace.rawRerun


class HscIngestTask(IngestTask):
    ArgumentParser = HscIngestArgumentParser


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

    A = int(365.25*yy)
    B = int(yy/400.0)
    C = int(yy/100.0)
    D = int(30.59*(mm-2))

    mjd = A + B - C + D + dd - 678912

    return mjd


class HscParseTask(ParseTask):
    DAY0 = 55927  # Zero point for  2012-01-01  51544 -> 2000-01-01

    def translate_field(self, md):
        field = md.get("OBJECT").strip()
        if field == "#":
            field = "UNKNOWN"
        # replacing inappropriate characters for file path and upper()
        field = re.sub(r'\W', '_', field).upper()

        return field

    def translate_visit(self, md):
        expId = md.get("EXP-ID").strip()
        m = re.search("^HSCE(\d{8})$", expId)  # 2016-06-14 and new scheme
        if m:
            return int(m.group(1))

        # Fallback to old scheme
        m = re.search("^HSC([A-Z])(\d{6})00$", expId)
        if not m:
            raise RuntimeError("Unable to interpret EXP-ID: %s" % expId)
        letter, visit = m.groups()
        visit = int(visit)
        if visit == 0:
            # Don't believe it
            frameId = md.get("FRAMEID").strip()
            m = re.search("^HSC([A-Z])(\d{6})\d{2}$", frameId)
            if not m:
                raise RuntimeError("Unable to interpret FRAMEID: %s" % frameId)
            letter, visit = m.groups()
            visit = int(visit)
            if visit % 2:  # Odd?
                visit -= 1
        return visit + 1000000*(ord(letter) - ord("A"))

    def getTjd(self, md):
        """Return truncated (modified) Julian Date"""
        return int(md.get('MJD')) - self.DAY0

    def translate_pointing(self, md):
        """This value was originally called 'pointing', and intended to be used
        to identify a logical group of exposures.  It has evolved to simply be
        a form of truncated Modified Julian Date, and is called 'visitID' in
        some versions of the code.  However, we retain the name 'pointing' for
        backward compatibility.
        """
        try:
            return self.getTjd(md)
        except:
            pass

        try:
            dateobs = md.get("DATE-OBS")
            m = re.search(r'(\d{4})-(\d{2})-(\d{2})', dateobs)
            year, month, day = m.groups()
            obsday = datetime.datetime(int(year), int(month), int(day), 0, 0, 0)
            mjd = datetime2mjd(obsday)
            return int(mjd) - self.DAY0
        except:
            pass

        self.log.warn("Unable to determine suitable 'pointing' value; using 0")
        return 0

    # CCD index mapping for commissioning run 2
    CCD_MAP_COMMISSIONING_2 = {112: 106,
                               107: 105,
                               113: 107,
                               115: 109,
                               108: 110,
                               114: 108,
                               }

    def translate_ccd(self, md):
        """Focus CCDs were numbered incorrectly in the readout software during
        commissioning run 2.  We need to map to the correct ones.
        """
        ccd = int(md.get("DET-ID"))
        try:
            tjd = self.getTjd(md)
        except:
            return ccd

        if tjd > 390 and tjd < 405:
            ccd = self.CCD_MAP_COMMISSIONING_2.get(ccd, ccd)

        return ccd

    def translate_filter(self, md):
        """Want upper-case filter names"""
        try:
            return md.get('FILTER01').strip().upper()
        except:
            return "Unrecognized"


class HscCalibsParseTask(CalibsParseTask):

    def _translateFromCalibId(self, field, md):
        data = md.get("CALIB_ID")
        match = re.search(".*%s=(\S+)" % field, data)
        return match.groups()[0]

    def translate_ccd(self, md):
        return int(self._translateFromCalibId("ccd", md))

    def translate_filter(self, md):
        return self._translateFromCalibId("filter", md)

    def translate_calibDate(self, md):
        return self._translateFromCalibId("calibDate", md)

    def translate_calibVersion(self, md):
        return self._translateFromCalibId("calibVersion", md)


class SuprimecamParseTask(HscParseTask):
    DAY0 = 53005  # 2004-01-01

    def translate_visit(self, md):
        expId = md.get("EXP-ID").strip()
        m = re.search("^SUP[A-Z](\d{7})0$", expId)
        if not m:
            raise RuntimeError("Unable to interpret EXP-ID: %s" % expId)
        visit = int(m.group(1))
        if int(visit) == 0:
            # Don't believe it
            frameId = md.get("FRAMEID").strip()
            m = re.search("^SUP[A-Z](\d{7})\d{1}$", frameId)
            if not m:
                raise RuntimeError("Unable to interpret FRAMEID: %s" % frameId)
            visit = int(m.group(1))
        return visit
