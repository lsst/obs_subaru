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
import sqlite3
from datetime import datetime

from lsst.utils import getPackageDir
from lsst.afw.cameraGeom import makeCameraFromPath, CameraConfig
from lsst.daf.butler.instrument import Instrument, addUnboundedCalibrationLabel
from lsst.daf.butler import DatasetType, DataId

from lsst.obs.hsc.hscPupil import HscPupilFactory
from lsst.obs.hsc.hscFilters import HSC_FILTER_NAMES
from lsst.obs.hsc.makeTransmissionCurves import (getSensorTransmission, getOpticsTransmission,
                                                 getFilterTransmission, getAtmosphereTransmission)
from .rawFormatter import HyperSuprimeCamRawFormatter, HyperSuprimeCamCornerRawFormatter

# Regular expression that matches HSC PhysicalFilter names (the same as Gen2
# filternames), with a group that can be lowercased to yield the
# associated AbstractFilter.
FILTER_REGEX = re.compile(r"HSC\-([GRIZY])2?")


def readDateTime(value):
    """Read datetime strings stored either with or without a time.

    Values must start with YYYY-MM-DD, which may or may not be followed
    by THH:MM:SS.

    Both formats appear in the defectRegistry.sqlite3 file.
    """
    try:
        return datetime.strptime(value, "%Y-%m-%d")
    except ValueError:
        pass
    return datetime.strptime(value, "%Y-%m-%dT%H:%M:%S")


class HyperSuprimeCam(Instrument):
    """Gen3 Butler specialization class for Subaru's Hyper Suprime-Cam.
    """

    @classmethod
    def getName(cls):
        # Docstring inherited from Instrument.getName
        return "HSC"

    def register(self, registry):
        # Docstring inherited from Instrument.register
        camera = self.getCamera()
        dataId = {"instrument": self.getName()}
        # The maximum values below make Gen3's ObservationDataIdPacker produce
        # outputs that match Gen2's ccdExposureId.
        obsMax = 21474800
        registry.addDimensionEntry("instrument", dataId,
                                   entries={"detector_max": 200,
                                            "visit_max": obsMax,
                                            "exposure_max": obsMax})
        for detector in camera:
            registry.addDimensionEntry(
                "detector", dataId,
                detector=detector.getId(),
                name=detector.getName(),
                # getType() returns a pybind11-wrapped enum, which
                # unfortunately has no way to extract the name of just
                # the value (it's always prefixed by the enum type name).
                purpose=str(detector.getType()).split(".")[-1],
                # HSC doesn't have rafts
                raft=None
            )
        for physical in HSC_FILTER_NAMES:
            # We use one of grizy for the abstract filter, when appropriate,
            # which we identify as when the physical filter starts with
            # "HSC-[GRIZY]".  Note that this means that e.g. "HSC-I" and
            # "HSC-I2" are both mapped to abstract filter "i".
            m = FILTER_REGEX.match(physical)
            registry.addDimensionEntry(
                "physical_filter",
                dataId,
                physical_filter=physical,
                abstract_filter=m.group(1).lower() if m is not None else None
            )

    def getRawFormatter(self, dataId):
        # Docstring inherited from Instrument.getRawFormatter
        if dataId["detector"] in (100, 101, 102, 103):
            return HyperSuprimeCamCornerRawFormatter()
        else:
            return HyperSuprimeCamRawFormatter()

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
        datasetType = DatasetType("camera", ("instrument", "calibration_label"), "TablePersistableCamera")
        butler.registry.registerDatasetType(datasetType)
        unboundedDataId = addUnboundedCalibrationLabel(butler.registry, self.getName())
        camera = self.getCamera()
        butler.put(camera, datasetType, unboundedDataId)

        # Write brighter-fatter kernel, with an infinite validity range.
        datasetType = DatasetType("bfKernel", ("instrument", "calibration_label"), "NumpyArray")
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
                                  "TablePersistableTransmissionCurve")
        butler.registry.registerDatasetType(datasetType)
        for entry in opticsTransmissions.values():
            if entry is None:
                continue
            butler.put(entry, datasetType, unboundedDataId)

        # Write transmission sensor
        sensorTransmissions = getSensorTransmission()
        datasetType = DatasetType("transmission_sensor",
                                  ("instrument", "detector", "calibration_label"),
                                  "TablePersistableTransmissionCurve")
        butler.registry.registerDatasetType(datasetType)
        for entry in sensorTransmissions.values():
            if entry is None:
                continue
            for sensor, curve in entry.items():
                dataId = DataId(unboundedDataId, detector=sensor)
                butler.put(curve, datasetType, dataId)

        # Write filter transmissions
        filterTransmissions = getFilterTransmission()
        datasetType = DatasetType("transmission_filter",
                                  ("instrument", "physical_filter", "calibration_label"),
                                  "TablePersistableTransmissionCurve")
        butler.registry.registerDatasetType(datasetType)
        for entry in filterTransmissions.values():
            if entry is None:
                continue
            for band, curve in entry.items():
                dataId = DataId(unboundedDataId, physical_filter=band)
                butler.put(curve, datasetType, dataId)

        # Write atmospheric transmissions, this only as dimension of instrument as other areas will only
        # look up along this dimension (ISR)
        atmosphericTransmissions = getAtmosphereTransmission()
        datasetType = DatasetType("transmission_atmosphere", ("instrument",),
                                  "TablePersistableTransmissionCurve")
        butler.registry.registerDatasetType(datasetType)
        for entry in atmosphericTransmissions.values():
            if entry is None:
                continue
            butler.put(entry, datasetType, {"instrument": self.getName()})

        # Write defects with validity ranges taken from obs_subaru/hsc/defects
        # (along with the defects themselves).
        datasetType = DatasetType("defects", ("instrument", "detector", "calibration_label"), "DefectsList")
        butler.registry.registerDatasetType(datasetType)
        defectPath = os.path.join(getPackageDir("obs_subaru"), "hsc", "defects")
        dbPath = os.path.join(defectPath, "defectRegistry.sqlite3")
        db = sqlite3.connect(dbPath)
        db.row_factory = sqlite3.Row
        sql = "SELECT path, ccd, validStart, validEnd FROM defect"
        with butler.transaction():
            for row in db.execute(sql):
                dataId = DataId(universe=butler.registry.dimensions,
                                instrument=self.getName(),
                                calibration_label=f"defect/{row['path']}/{row['ccd']}")
                dataId.entries["calibration_label"]["valid_first"] = readDateTime(row["validStart"])
                dataId.entries["calibration_label"]["valid_last"] = readDateTime(row["validEnd"])
                butler.registry.addDimensionEntry("calibration_label", dataId)
                ref = butler.registry.addDataset(datasetType, dataId, run=butler.run, recursive=True,
                                                 detector=row['ccd'])
                butler.datastore.ingest(os.path.join(defectPath, row["path"]), ref, transfer="copy")
