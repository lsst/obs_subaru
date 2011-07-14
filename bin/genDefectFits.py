#!/usr/bin/env python

import sys
import os.path
import re
import numpy
import pyfits
import collections

import lsst.pex.policy as pexPolicy
import lsst.afw.cameraGeom as afwCG
import lsst.afw.cameraGeom.utils as afwCGU

Defect = collections.namedtuple('Defect', ['x0', 'y0', 'width', 'height'])

def genDefectFits(cameraPolicy, source, targetDir):
#    if not isinstance(mapperPolicy, pexPolicy.Policy):
#        mapperPolicy = pexPolicy.Policy(mapperPolicy)
#    cameraPolicy = mapperPolicy.get("camera")
    if not isinstance(cameraPolicy, pexPolicy.Policy):
        cameraPolicy = pexPolicy.Policy(cameraPolicy)
    geomPolicy = afwCGU.getGeomPolicy(cameraPolicy)
    camera = afwCGU.makeCamera(geomPolicy)

    ccds = dict()
    for raft in camera:
        for ccd in afwCG.cast_Raft(raft):
            ccdNum = ccd.getId().getSerial()
            ccds[ccdNum] = ccd.getId().getName()

    print ccds

    defects = dict()

    f = open(source, "r")
    for line in f:
        line = re.sub("\#.*", "", line).strip()
        if len(line) == 0:
            continue
        ccd, x0, y0, width, height = re.split("\s+", line)
        ccd = int(ccd)
        if not ccds.has_key(ccd):
            raise RuntimeError("Unrecognised ccd: %d" % ccd)
        if not defects.has_key(ccd):
            defects[ccd] = list()
        defects[ccd].append(Defect(x0=int(x0), y0=int(y0), width=int(width), height=int(height)))
    f.close()
    
    for ccd in defects.keys():
        columns = list()
        for colName in Defect._fields:
            colData = numpy.array([d._asdict()[colName] for d in defects[ccd]])
            col = pyfits.Column(name=colName, format="I", array=colData)
            columns.append(col)
        
        cols = pyfits.ColDefs(columns)
        table = pyfits.new_table(cols)

        table.header.update('SERIAL', ccd)
        table.header.update('NAME', ccds[ccd])

        table.writeto(os.path.join(targetDir, "defects_%d.fits" % ccd))

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print "USAGE: %s CAMERA_POLICY.paf DEFECTS.dat TARGET_DIR" % sys.argv[0]
        sys.exit(1)
    genDefectFits(sys.argv[1], sys.argv[2], sys.argv[3])
