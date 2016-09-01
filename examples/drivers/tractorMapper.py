from __future__ import print_function
#
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
# Copyright 2011 Dustin Lang.
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
# see <http://www.lsstcorp.org/LegalNotices/>.
#

import os
import re
import time
import lsst.daf.base as dafBase
from lsst.daf.persistence import Mapper, ButlerLocation, LogicalLocation
import lsst.daf.butlerUtils as butlerUtils
import lsst.afw.image as afwImage
import lsst.afw.coord as afwCoord
import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as afwCameraGeom
import lsst.afw.cameraGeom.utils as cameraGeomUtils
import lsst.afw.image.utils as imageUtils
import lsst.pex.logging as pexLog
import lsst.pex.policy as pexPolicy

# Solely to get boost serialization registrations for Measurement subclasses
import lsst.meas.algorithms as measAlgo
import lsst.meas.multifit as measMultifit
import lsst.afw.detection


class TractorMapper(Mapper):

    def __init__(self, rerun=0, basedir='.', **kwargs):
        Mapper.__init__(self)

        print('TractorMapper(): ignoring kwargs', kwargs)

        self.basedir = basedir
        self.rerun = rerun
        self.log = pexLog.Log(pexLog.getDefaultLog(), 'TractorMapper')

        indir = os.path.join(self.basedir, 't%(visit)04i')
        outdir = os.path.join(indir, 'rr%(rerun)04i')
        self.filenames = {'outdir': (outdir, None, None),
                          'visitim': (os.path.join(indir, 't.fits'), #'t_img.fits'), #img.fits'),
                                      'lsst.afw.image.ExposureF', 'ExposureF'),
                          'psf': (os.path.join(outdir, 'psf.boost'),
                                  'lsst.afw.detection.Psf', 'Psf'),
                          'src': (os.path.join(outdir, 'src.boost'),
                                  # dare to dream / keep dreaming
                                  #os.path.join(outdir, 'src.fits'),
                                  # htf did this work before?
                                  #'lsst.afw.detection.Source', 'Source'),
                                  'lsst.afw.detection.PersistableSourceVector',
                                  'PersistableSourceVector'),
                          'bb': (os.path.join(outdir, 'bb.pickle'),
                                 None, None),
                          'pyfoots': (os.path.join(outdir, 'foots.pickle'),
                                      None, None),
                          'footprints': (os.path.join(outdir, 'foots.boost'),
                                         'lsst.afw.detection.FootprintList',
                                         'FootprintList'),
                          'truesrc': (os.path.join(indir, 'srcs.fits'),
                                      None, None),
                          }
        '''
        for datasetType in ["raw", "bias", "dark", "flat", "fringe",
            "postISR", "postISRCCD", "sdqaAmp", "sdqaCcd",
            "icSrc", "icMatch", "visitim", "psf", "apCorr", "calexp", "src",
            "sourceHist", "badSourceHist", "source", "badSource",
            "invalidSource", "object", "badObject"]:
            '''
        self.keys = ['visit', 'filter']

    def getKeys(self):
        return self.keys

    def getPath(self, datasetType, dataId):
        print('Mapping', datasetType, 'with keys', dataId)
        if not 'rerun' in dataId:
            dataId['rerun'] = self.rerun
        (pattern, cname, pyname) = self.filenames[datasetType]
        path = pattern % dataId
        print('->', path)
        return path

    def map(self, datasetType, dataId):
        print('Mapping', datasetType, 'with keys', dataId)
        path = self.getPath(datasetType, dataId)
        (pattern, cname, pyname) = self.filenames[datasetType]
        # wow, this is rocket science
        storagetype = None
        if path.endswith('.fits'):
            storagetype = 'FitsStorage'
        elif path.endswith('.boost'):
            storagetype = 'BoostStorage'
        elif path.endswith('.pickle'):
            storagetype = 'PickleStorage'
        return ButlerLocation(cname, pyname, storagetype, path, dataId)
