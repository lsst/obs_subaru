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
import getpass
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
from lsst.obs.hscSim.fileMapper import FileMapper

class ProcessFileConfig(ProcessImageTask.ConfigClass):
    """Config for ProcessFile"""
    doCalibrate = pexConfig.Field(dtype=bool, default=False, doc = "Perform calibration?")

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
        parser = FileArgumentParser(name=cls._DefaultName)
        parser.add_id_argument(name="--id", datasetType="calexp",
                               help="data ID, e.g. --id calexp=XXX")

        return parser

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
        postIsrExposure.getMaskedImage().getMask()[:] &= \
            afwImage.MaskU.getPlaneBitMask(["SAT", "INTRP", "BAD", "EDGE"])
        
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

        if len(args) > 0 and not (args[0].startswith("-") or args[0].startswith("@")):
            # note: don't set namespace.input until after running parse_args, else it will get overwritten
            inputRoot = pipeAP._fixPath(pipeAP.DEFAULT_INPUT_NAME, args[0])
            if not os.path.exists(inputRoot):
                self.error("Error: input=%r not found" % (inputRoot,))
            if not os.path.isdir(inputRoot):
                inputRoot, fileName = os.path.split(inputRoot)
                args[0:1] = [inputRoot, "--id", "calexp=%s" % fileName]

            if not os.path.isdir(inputRoot):
                self.error("Error: input=%r is not a directory" % (inputRoot,))
        
        namespace = argparse.Namespace()
        namespace.input = pipeAP._fixPath(pipeAP.DEFAULT_INPUT_NAME, args[0])
        if not os.path.isdir(namespace.input):
            self.error("Error: input=%r not found" % (namespace.input,))
        namespace.config = config
        namespace.log = log if log is not None else pexLog.Log.getDefaultLog()
        namespace.dataIdList = []
        namespace.datasetType = "calexp"

        self.handleCamera(namespace)

        if False:
            namespace.obsPkg = "file"       # used to find initial overrides
            self._applyInitialOverrides(namespace)
        if override is not None:
            override(namespace.config)

        # Add data ID containers to namespace
        for dataIdArgument in self._dataIdArgDict.itervalues():
            setattr(namespace, dataIdArgument.name, dataIdArgument.ContainerClass(level=dataIdArgument.level))

        namespace = argparse.ArgumentParser.parse_args(self, args=args, namespace=namespace)
        del namespace.configfile
                
        namespace.calib = pipeAP._fixPath(pipeAP.DEFAULT_CALIB_NAME,  namespace.rawCalib)
        if namespace.rawOutput:
            namespace.output = pipeAP._fixPath(pipeAP.DEFAULT_OUTPUT_NAME, namespace.rawOutput)
        else:
            namespace.output = None
            if namespace.rawRerun is None:
                namespace.rawRerun = getpass.getuser()

        if namespace.rawRerun:
            if namespace.output:
                self.error("Error: cannot specify both --output and --rerun")
            namespace.rerun = tuple(os.path.join(namespace.input, "rerun", v)
                                    for v in namespace.rawRerun.split(":"))
            modifiedInput = False
            if len(namespace.rerun) == 2:
                namespace.input, namespace.output = namespace.rerun
                modifiedInput = True
            elif len(namespace.rerun) == 1:
                if os.path.exists(namespace.rerun[0]):
                    namespace.output = namespace.rerun[0]
                    namespace.input = os.path.realpath(os.path.join(namespace.rerun[0], "_parent"))
                    modifiedInput = True
                else:
                    namespace.output = namespace.rerun[0]
            else:
                self.error("Error: invalid argument for --rerun: %s" % namespace.rerun)
            if modifiedInput:
                try:
                    inputMapper = dafPersist.Butler.getMapperClass(namespace.input)
                except RuntimeError:
                    inputMapper = None
                if inputMapper and inputMapper != mapperClass:
                    self.error("Error: input directory specified by --rerun must have the same mapper as INPUT")
        else:
            namespace.rerun = None
        del namespace.rawInput
        del namespace.rawCalib
        del namespace.rawOutput
        del namespace.rawRerun
        
        if namespace.clobberOutput:
            if namespace.output is None:
                self.error("--clobber-output is only valid with --output or --rerun")
            elif namespace.output == namespace.input:
                self.error("--clobber-output is not valid when the output and input repos are the same")
            if os.path.exists(namespace.output):
                namespace.log.info("Removing output repo %s for --clobber-output" % namespace.output)
                shutil.rmtree(namespace.output)

        namespace.log.info("input=%s"  % (namespace.input,))
        if namespace.calib:
            namespace.log.info("calib=%s"  % (namespace.calib,))
        namespace.log.info("output=%s" % (namespace.output,))
        
        if "config" in namespace.show:
            namespace.config.saveToStream(sys.stdout, "config")

        mapper = FileMapper(root=namespace.input,
                            calibRoot=namespace.calib, outputRoot=namespace.output)
        namespace.butler = dafPersist.Butler(root=namespace.input, mapper=mapper)

        # convert data in each of the identifier lists to proper types
        # this is done after constructing the butler, hence after parsing the command line,
        # because it takes a long time to construct a butler
        self._processDataIds(namespace)
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

    def _processDataIds(self, namespace):
        """See lsst.pipe.tasks.ArgumentParser._processDataIds"""

        # Strip the .fits (or .fit or .fts) extension from the name of the calexp
        # as we'll be using the basename to specify the output directory
        for d in namespace.id.idList:
            calexp = d.get("calexp", None)
            if calexp:
                b, e = os.path.splitext(calexp)
                if e in (".fits", ".fit", ".fts"):
                    d["calexp"] = b
                    
        return pipeAP.ArgumentParser._processDataIds(self, namespace)
