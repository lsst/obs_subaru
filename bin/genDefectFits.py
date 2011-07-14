#!/usr/bin/env python

import os.path
import re
import numpy
import pyfits

import collections

Defect = collections.namedtuple('Defect', ['x0', 'y0', 'width', 'height'])

def genDefectFits(mapperPolicy, source, targetDir):
    if not isinstance(mapperPolicy, pexPolicy.Policy):
        mapperPolicy = pexPolicy.Policy(mapperPolicy)
    cameraPolicy = mapperPolicy.get("camera")
    if not isinstance(cameraPolicy, pexPolicy.Policy):
        cameraPolicy = pexPolicy.Policy(cameraPolicy)
    geomPolicy = afwCGU.getGeomPolicy(cameraPolicy)
    camera = afwCGU.makeCamera(geomPolicy)

    ccds = dict()
    for raft in camera:
        for ccd in raft:
            ccdNum = ccd.getId().getSerial()
            ccds[ccdNum] = ccd.getId().getName()

    print ccds

    defects = dict()

    f = open(source, "r")
    for line in f:
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
            col = numpy.array([d[colName] for d in defects[ccd]])
            columns.append(col)
        
        cols = pyfits.ColDefs(columns)
        table = pyfits.new_table(cols)

        table.header['SERIAL'] = ccd
        table.header['NAME'] = ccds[ccd]

        table.writeto(os.path.join(targetDir, "defects_" + ccd + ".fits"))
