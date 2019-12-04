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

from lsst.utils import getPackageDir
from lsst.afw.cameraGeom import makeCameraFromPath, CameraConfig
from lsst.daf.butler import DatasetType, DataCoordinate, FileDataset, DatasetRef
from lsst.obs.base import Instrument

from lsst.obs.hsc.hscPupil import HscPupilFactory
from lsst.obs.hsc.hscFilters import HSC_FILTER_DEFINITIONS
import lsst.obs.hsc.makeTransmissionCurves
from lsst.obs.subaru.strayLight.formatter import SubaruStrayLightDataFormatter

log = logging.getLogger(__name__)

# Regular expression that matches HSC PhysicalFilter names (the same as Gen2
# filternames), with a group that can be lowercased to yield the
# associated AbstractFilter.
FILTER_REGEX = re.compile(r"HSC\-([GRIZY])2?")

packageDir = getPackageDir("obs_subaru")


class HyperSuprimeCam(Instrument):
    """Gen3 Butler specialization class for Subaru's Hyper Suprime-Cam.
    """

    filterDefinitions = HSC_FILTER_DEFINITIONS
    configPaths = [os.path.join(packageDir, "config"),
                   os.path.join(packageDir, "config", "hsc")]
    dataPath = os.path.join(getPackageDir("obs_subaru_data"), "hsc")

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @classmethod
    def getName(cls):
        return "HSC"

    def register(self, registry):
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

    def _getBrighterFatterKernel(self):
        """Return the brighter-fatter kernel for HSC as a `numpy.ndarray`.
        """
        path = os.path.join(getPackageDir("obs_subaru"), "hsc", "brighter_fatter_kernel.pkl")
        with open(path, "rb") as fd:
            kernel = pickle.load(fd, encoding='latin1')  # encoding for pickle written with Python 2
        return kernel

    def _getDetectorTransmission(self, unboundedDataId):
        detectorTransmissions = lsst.obs.hsc.makeTransmissionCurves.getSensorTransmission()
        result = {}
        for entry in detectorTransmissions.values():
            for detector, transmissionCurve in entry.items():
                dataId = DataCoordinate.standardize(unboundedDataId, detector=detector)
                result[dataId] = transmissionCurve
        return result

    def _getOpticsTransmission(self, unboundedDataId):
        opticsTransmissions = lsst.obs.hsc.makeTransmissionCurves.getOpticsTransmission()
        result = {}
        for entry in opticsTransmissions.values():
            # The files can include dates with undefined transmissions: ignore them.
            if entry is None:
                continue
            result[unboundedDataId] = entry
        return result

    def _getFilterTransmission(self, unboundedDataId):
        filterTransmissions = lsst.obs.hsc.makeTransmissionCurves.getFilterTransmission()
        result = {}
        for entry in filterTransmissions.values():
            for filter, transmissionCurve in entry.items():
                dataId = DataCoordinate.standardize(unboundedDataId, physical_filter=filter)
                result[dataId] = transmissionCurve
        return result

    def _getAtmosphereTransmission(self, unboundedDataId):
        opticsTransmissions = lsst.obs.hsc.makeTransmissionCurves.getOpticsTransmission()
        result = {}
        for entry in opticsTransmissions.values():
            # # The files can include dates with undefined transmissions: ignore them.
            if entry is None:
                continue
            result[unboundedDataId] = entry
        return result

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
