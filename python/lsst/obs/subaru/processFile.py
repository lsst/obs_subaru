#!/usr/bin/env python
#
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.
#
import argparse
import os
import sys
from lsst.pipe.tasks.processImage import ProcessImageTask
import lsst.pipe.base.argumentParser as pipeAP
import lsst.afw.image as afwImage
import lsst.afw.table as afwTable
import lsst.pex.config as pexConfig
import lsst.pex.logging as pexLog
import lsst.pipe.base as pipeBase
import lsst.daf.persistence as dafPersist

class ProcessFileConfig(ProcessImageTask.ConfigClass):
    """Config for ProcessFile"""
    pass

class ProcessFileTask(ProcessImageTask):
    """Process a CCD
    
    Available steps include:
    - calibrate
    - detect sources
    - measure sources
    """
    ConfigClass = ProcessFileConfig
    _DefaultName = "processFile"

    def __init__(self, **kwargs):
        ProcessImageTask.__init__(self, **kwargs)

    def makeIdFactory(self, sensorRef):
        expBits = sensorRef.get("ccdExposureId_bits")
        expId = long(sensorRef.get("ccdExposureId"))
        return afwTable.IdFactory.makeSource(expId, 64 - expBits)        

    @classmethod
    def _makeArgumentParser(cls):
        """Create an argument parser
        """
        return FileArgumentParser(name=cls._DefaultName)

    @pipeBase.timeMethod
    def run(self, sensorRef):
        """Process one CCD
        
        @param sensorRef: sensor-level butler data reference
        @return pipe_base Struct containing these fields:
        - exposure: calibrated exposure (calexp): as computed if config.doCalibrate,
            else as upersisted and updated if config.doDetection, else None
        - calib: object returned by calibration process if config.doCalibrate, else None
        - sources: detected source if config.doPhotometry, else None
        """
        self.log.info("Processing %s" % (sensorRef.dataId))

        # initialize outputs
        postIsrExposure = sensorRef.get("calexp")
        
        # delegate the work to ProcessImageTask
        result = self.process(sensorRef, postIsrExposure)
        return result

class FileArgumentParser(pipeAP.ArgumentParser):
    def __init__(self, *args, **kwargs):
        pipeAP.ArgumentParser.__init__(self, *args, **kwargs)

    def parse_args(self, config, args=None, log=None, override=None):
        """Parse arguments for a pipeline task

        @param config: config for the task being run
        @param args: argument list; if None use sys.argv[1:]
        @param log: log (instance pex_logging Log); if None use the default log
        @param override: a config override callable, to be applied after camera-specific overrides
            files but before any command-line config overrides.  It should take the root config
            object as its only argument.

        @return namespace: a struct containing many useful fields including:
        - camera: camera name
        - config: the supplied config with all overrides applied, validated and frozen
        - butler: a butler for the data
        - datasetType: dataset type
        - dataIdList: a list of data ID dicts
        - dataRefList: a list of butler data references; each data reference is guaranteed to contain
            data for the specified datasetType (though perhaps at a lower level than the specified level,
            and if so, valid data may not exist for all valid sub-dataIDs)
        - log: a pex_logging log
        - an entry for each command-line argument, with the following exceptions:
          - config is Config, not an override
          - configfile, id, logdest, loglevel are all missing
        - obsPkg: name of obs_ package for this camera
        """
        if args == None:
            args = sys.argv[1:]

        if len(args) < 1 or args[0].startswith("-") or args[0].startswith("@"):
            self.print_help()
            self.exit("%s: error: Must specify input as first argument" % self.prog)

        # note: don't set namespace.input until after running parse_args, else it will get overwritten
        inputRoot = pipeAP._fixPath(pipeAP.DEFAULT_INPUT_NAME, args[0])
        if not os.path.exists(inputRoot):
            self.error("Error: input=%r not found" % (inputRoot,))
        elif os.path.isdir(inputRoot):
            self.error("Error: input=%r is a directory" % (inputRoot,))
        
        namespace = argparse.Namespace()
        namespace.config = config
        namespace.log = log if log is not None else pexLog.Log.getDefaultLog()
        namespace.dataIdList = []
        namespace.datasetType = self._datasetType

        self.handleCamera(namespace)

        if False:
            namespace.obsPkg = "file"       # used to find initial overrides
            self._applyInitialOverrides(namespace)
        if override is not None:
            override(namespace.config)
        
        namespace = argparse.ArgumentParser.parse_args(self, args=args, namespace=namespace)
        namespace.input = inputRoot
        del namespace.configfile
        del namespace.id
        
        namespace.calib  = pipeAP._fixPath(pipeAP.DEFAULT_CALIB_NAME,  namespace.calib)
        namespace.output = pipeAP._fixPath(pipeAP.DEFAULT_OUTPUT_NAME, namespace.output)
        
        namespace.log.info("input=%s"  % (namespace.input,))
        namespace.log.info("calib=%s"  % (namespace.calib,))
        namespace.log.info("output=%s" % (namespace.output,))
        
        if "config" in namespace.show:
            namespace.config.saveToStream(sys.stdout, "config")

        namespace.butler = FileButler(
            root = namespace.input,
            calibRoot = namespace.calib,
            outputRoot = namespace.output,
        )

        self._castDataIds(namespace)

        namespace.dataRefList = [FileDataRef(namespace.input)]
        
        if "data" in namespace.show:
            for dataRef in namespace.dataRefList:
                print "dataRef.dataId =", dataRef.dataId
        
        if "exit" in namespace.show:
            sys.exit(0)

        if namespace.debug:
            try:
                import debug
            except ImportError:
                sys.stderr.write("Warning: no 'debug' module found\n")
                namespace.debug = False

        if namespace.logdest:
            namespace.log.addDestination(namespace.logdest)
        del namespace.logdest
        
        if namespace.loglevel:
            permitted = ('DEBUG', 'INFO', 'WARN', 'FATAL')
            if namespace.loglevel.upper() in permitted:
                value = getattr(pexLog.Log, namespace.loglevel.upper())
            else:
                try:
                    value = int(namespace.loglevel)
                except ValueError:
                    self.error("log-level=%s not int or one of %s" % (namespace.loglevel, permitted))
            namespace.log.setThreshold(value)
        del namespace.loglevel
        
        namespace.config.validate()
        namespace.config.freeze()

        return namespace

class FileButler(object):
    def __init__(self, root, calibRoot, outputRoot):
        self.root = root
        self.calibRoot = calibRoot
        self.outputRoot = outputRoot

    def __get(self, *args, **kwargs):
        print "RHL get", args, kwargs
        import pdb; pdb.set_trace()

    def getKeys(self, *args, **kwargs):
        return {}

class FileDataRef(object):
    """
    A FileDataRef is like a ButlerDataRef, but only handles files

    Public methods:

    get(self, datasetType=None, **rest)

    put(self, obj, datasetType=None, **rest)

    datasetExists(self, datasetType=None, **rest)
    """

    def __init__(self, fileName):
        self.fileName = fileName
        self.dataId = fileName

    def get(self, datasetType=None, **rest):
        """
        Retrieve a dataset of the given type (or the type used when creating
        the ButlerSubset, if None) as specified by the ButlerDataRef.

        @param datasetType (str)  dataset type to retrieve.
        @param **rest             keyword arguments with data identifiers
        @returns object corresponding to the given dataset type.
        """

        if datasetType == "calexp":
            return afwImage.ExposureF(self.fileName)
        elif datasetType == "ccdExposureId_bits":
            return 32
        elif datasetType == "ccdExposureId":
            return 0

        print "RHL", datasetType
        import pdb; pdb.set_trace() 

    def put(self, obj, datasetType=None, **rest):
        """
        Persist a dataset of the given type (or the type used when creating
        the ButlerSubset, if None) as specified by the ButlerDataRef.

        @param obj                object to persist.
        @param datasetType (str)  dataset type to persist.
        @param **rest             keyword arguments with data identifiers
        """

        if datasetType == "processFile_config":
            return
        elif datasetType == "processFile_metadata":
            return

        print "RHL datasetType", datasetType
        return

        import pdb; pdb.set_trace() 
        if datasetType is None:
            datasetType = self.butlerSubset.datasetType
        self.butlerSubset.butler.put(obj, datasetType, self.dataId, **rest)

    def datasetExists(self, datasetType=None, **rest):
        """
        Determine if a dataset exists of the given type (or the type used when
        creating the ButlerSubset, if None) as specified by the ButlerDataRef.

        @param datasetType (str) dataset type to check.
        @param **rest            keywords arguments with data identifiers
        @returns bool
        """
        return os.path.isfile(self.fileName)
