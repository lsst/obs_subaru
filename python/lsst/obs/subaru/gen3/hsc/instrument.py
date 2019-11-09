# This file is part of obs_subaru.
#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (http://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
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
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Gen3 Butler registry declarations for Hyper Suprime-Cam.
"""

__all__ = ("HyperSuprimeCam",)

import re
import os
import pickle
import logging
import datetime
from dateutil import parser

from lsst.utils import getPackageDir
from lsst.afw.cameraGeom import makeCameraFromPath, CameraConfig
from lsst.daf.butler import DatasetType, DataCoordinate, FileDataset, DatasetRef
from lsst.obs.base import Instrument, addUnboundedCalibrationLabel
from lsst.pipe.tasks.read_defects import read_all_defects

from lsst.obs.hsc.hscPupil import HscPupilFactory
from lsst.obs.hsc.hscFilters import HSC_FILTER_DEFINITIONS
from lsst.obs.hsc.makeTransmissionCurves import (getSensorTransmission, getOpticsTransmission,
                                                 getFilterTransmission, getAtmosphereTransmission)
from lsst.obs.subaru.strayLight.formatter import SubaruStrayLightDataFormatter

log = logging.getLogger(__name__)

# Regular expression that matches HSC PhysicalFilter names (the same as Gen2
# filternames), with a group that can be lowercased to yield the
# associated AbstractFilter.
FILTER_REGEX = re.compile(r"HSC\-([GRIZY])2?")


class HyperSuprimeCam(Instrument):
    """Gen3 Butler specialization class for Subaru's Hyper Suprime-Cam.
    """

    filterDefinitions = HSC_FILTER_DEFINITIONS

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        packageDir = getPackageDir("obs_subaru")
        self.configPaths = [os.path.join(packageDir, "config"),
                            os.path.join(packageDir, "config", "hsc")]

    @classmethod
    def getName(cls):
        # Docstring inherited from Instrument.getName
        return "HSC"

    def register(self, registry):
        # Docstring inherited from Instrument.register
        camera = self.getCamera()
        # The maximum values below make Gen3's ObservationDataIdPacker produce
        # outputs that match Gen2's ccdExposureId.
        obsMax = 21474800
        registry.insertDimensionData(
            "instrument",
            {
                "name": self.getName(),
                "detector_max": 200,
                "visit_max": obsMax,
                "exposure_max": obsMax
            }
        )
        registry.insertDimensionData(
            "detector",
            *[
                {
                    "instrument": self.getName(),
                    "id": detector.getId(),
                    "full_name": detector.getName(),
                    # TODO: make sure these definitions are consistent with those
                    # extracted by astro_metadata_translator, and test that they
                    # remain consistent somehow.
                    "name_in_raft": detector.getName().split("_")[1],
                    "raft": detector.getName().split("_")[0],
                    "purpose": str(detector.getType()).split(".")[-1],
                }
                for detector in camera
            ]
        )
        self._registerFilters(registry)

    def getRawFormatter(self, dataId):
        # Docstring inherited from Instrument.getRawFormatter
        # Import the formatter here to prevent a circular dependency.
        from .rawFormatter import HyperSuprimeCamRawFormatter, HyperSuprimeCamCornerRawFormatter
        if dataId["detector"] in (100, 101, 102, 103):
            return HyperSuprimeCamCornerRawFormatter
        else:
            return HyperSuprimeCamRawFormatter

    def getCamera(self):
        """Retrieve the cameraGeom representation of HSC.

        This is a temporary API that should go away once obs_ packages have
        a standardized approach to writing versioned cameras to a Gen3 repo.
        """
        path = os.path.join(getPackageDir("obs_subaru"), "hsc", "camera")
        config = CameraConfig()
        config.load(os.path.join(path, "camera.py"))
        return makeCameraFromPath(
            cameraConfig=config,
            ampInfoPath=path,
            shortNameFunc=lambda name: name.replace(" ", "_"),
            pupilFactoryClass=HscPupilFactory
        )

    def getBrighterFatterKernel(self):
        """Return the brighter-fatter kernel for HSC as a `numpy.ndarray`.

        This is a temporary API that should go away once obs_ packages have
        a standardized approach to writing versioned kernels to a Gen3 repo.
        """
        path = os.path.join(getPackageDir("obs_subaru"), "hsc", "brighter_fatter_kernel.pkl")
        with open(path, "rb") as fd:
            kernel = pickle.load(fd, encoding='latin1')  # encoding for pickle written with Python 2
        return kernel

    def writeCuratedCalibrations(self, butler):
        """Write human-curated calibration Datasets to the given Butler with
        the appropriate validity ranges.

        This is a temporary API that should go away once obs_ packages have
        a standardized approach to this problem.
        """

        # Write cameraGeom.Camera, with an infinite validity range.
        datasetType = DatasetType("camera", ("instrument", "calibration_label"), "Camera",
                                  universe=butler.registry.dimensions)
        butler.registry.registerDatasetType(datasetType)
        unboundedDataId = addUnboundedCalibrationLabel(butler.registry, self.getName())
        camera = self.getCamera()
        butler.put(camera, datasetType, unboundedDataId)

        # Write brighter-fatter kernel, with an infinite validity range.
        datasetType = DatasetType("bfKernel", ("instrument", "calibration_label"), "NumpyArray",
                                  universe=butler.registry.dimensions)
        butler.registry.registerDatasetType(datasetType)
        # Load and then put instead of just moving the file in part to ensure
        # the version in-repo is written with Python 3 and does not need
        # `encoding='latin1'` to be read.
        bfKernel = self.getBrighterFatterKernel()
        butler.put(bfKernel, datasetType, unboundedDataId)

        # The following iterate over the values of the dictionaries returned by the transmission functions
        # and ignore the date that is supplied. This is due to the dates not being ranges but single dates,
        # which do not give the proper notion of validity. As such unbounded calibration labels are used
        # when inserting into the database. In the future these could and probably should be updated to
        # properly account for what ranges are considered valid.

        # Write optical transmissions
        opticsTransmissions = getOpticsTransmission()
        datasetType = DatasetType("transmission_optics",
                                  ("instrument", "calibration_label"),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions)
        butler.registry.registerDatasetType(datasetType)
        for entry in opticsTransmissions.values():
            if entry is None:
                continue
            butler.put(entry, datasetType, unboundedDataId)

        # Write transmission sensor
        sensorTransmissions = getSensorTransmission()
        datasetType = DatasetType("transmission_sensor",
                                  ("instrument", "detector", "calibration_label"),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions)
        butler.registry.registerDatasetType(datasetType)
        for entry in sensorTransmissions.values():
            if entry is None:
                continue
            for sensor, curve in entry.items():
                dataId = DataCoordinate.standardize(unboundedDataId, detector=sensor)
                butler.put(curve, datasetType, dataId)

        # Write filter transmissions
        filterTransmissions = getFilterTransmission()
        datasetType = DatasetType("transmission_filter",
                                  ("instrument", "physical_filter", "calibration_label"),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions)
        butler.registry.registerDatasetType(datasetType)
        for entry in filterTransmissions.values():
            if entry is None:
                continue
            for band, curve in entry.items():
                dataId = DataCoordinate.standardize(unboundedDataId, physical_filter=band)
                butler.put(curve, datasetType, dataId)

        # Write atmospheric transmissions, this only as dimension of instrument as other areas will only
        # look up along this dimension (ISR)
        atmosphericTransmissions = getAtmosphereTransmission()
        datasetType = DatasetType("transmission_atmosphere", ("instrument",),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions)
        butler.registry.registerDatasetType(datasetType)
        for entry in atmosphericTransmissions.values():
            if entry is None:
                continue
            butler.put(entry, datasetType, {"instrument": self.getName()})

        # Write defects with validity ranges taken from obs_subaru_data/hsc/defects
        # (along with the defects themselves).
        datasetType = DatasetType("defects", ("instrument", "detector", "calibration_label"), "DefectsList",
                                  universe=butler.registry.dimensions)
        butler.registry.registerDatasetType(datasetType)
        defectPath = os.path.join(getPackageDir("obs_subaru_data"), "hsc", "defects")
        camera = self.getCamera()
        defectsDict = read_all_defects(defectPath, camera)
        endOfTime = '20380119T031407'
        dimensionRecords = []
        datasetRecords = []
        # First loop just gathers up the things we want to insert, so we
        # can do some bulk inserts and minimize the time spent in transaction.
        for det in defectsDict:
            detector = camera[det]
            times = sorted([k for k in defectsDict[det]])
            defects = [defectsDict[det][time] for time in times]
            times = times + [parser.parse(endOfTime), ]
            for defect, beginTime, endTime in zip(defects, times[:-1], times[1:]):
                md = defect.getMetadata()
                calibrationLabel = f"defect/{md['CALIBDATE']}/{md['DETECTOR']}"
                dataId = DataCoordinate.standardize(
                    universe=butler.registry.dimensions,
                    instrument=self.getName(),
                    calibration_label=calibrationLabel,
                    detector=detector.getId(),
                )
                datasetRecords.append((defect, dataId))
                dimensionRecords.append({
                    "instrument": self.getName(),
                    "name": calibrationLabel,
                    "datetime_begin": beginTime,
                    "datetime_end": endTime,
                })
        # Second loop actually does the inserts and filesystem writes.
        with butler.transaction():
            butler.registry.insertDimensionData("calibration_label", *dimensionRecords)
            # TODO: vectorize these puts, once butler APIs for that become
            # available.
            for defect, dataId in datasetRecords:
                butler.put(defect, datasetType, dataId)

    def ingestStrayLightData(self, butler, directory, *, transfer=None):
        """Ingest externally-produced y-band stray light data files into
        a data repository.

        Parameters
        ----------
        butler : `lsst.daf.butler.Butler`
            Butler initialized with the collection to ingest into.
        directory : `str`
            Directory containing yBackground-*.fits files.
        transfer : `str`, optional
            If not `None`, must be one of 'move', 'copy', 'hardlink', or
            'symlink', indicating how to transfer the files.
        """
        calibrationLabel = "y-LED-encoder-on"
        # LEDs covered up around 2018-01-01, no need for correctin after that
        # date.
        datetime_end = datetime.datetime(2018, 1, 1)
        datasets = []
        # TODO: should we use a more generic name for the dataset type?
        # This is just the (rather HSC-specific) name used in Gen2, and while
        # the instances of this dataset are camera-specific, the datasetType
        # (which is used in the generic IsrTask) should not be.
        datasetType = DatasetType("yBackground",
                                  dimensions=("physical_filter", "detector", "calibration_label"),
                                  storageClass="StrayLightData",
                                  universe=butler.registry.dimensions)
        for detector in self.getCamera():
            path = os.path.join(directory, f"yBackground-{detector.getId():03d}.fits")
            if not os.path.exists(path):
                log.warn(f"No stray light data found for detector {detector.getId()}.")
                continue
            ref = DatasetRef(datasetType, dataId={"instrument": self.getName(),
                                                  "detector": detector.getId(),
                                                  "physical_filter": "HSC-Y",
                                                  "calibration_label": calibrationLabel})
            datasets.append(FileDataset(ref=ref, path=path, formatter=SubaruStrayLightDataFormatter))
        with butler.transaction():
            butler.registry.registerDatasetType(datasetType)
            butler.registry.insertDimensionData("calibration_label", {"instrument": self.getName(),
                                                                      "name": calibrationLabel,
                                                                      "datetime_begin": datetime.min,
                                                                      "datetime_end": datetime_end})
            butler.ingest(datasets, transfer=transfer)
