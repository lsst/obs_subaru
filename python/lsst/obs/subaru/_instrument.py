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

import os
import pickle
import logging

from functools import lru_cache

import astropy.time
from lsst.utils import getPackageDir
from lsst.afw.cameraGeom import makeCameraFromPath, CameraConfig
from lsst.daf.butler import (DatasetType, DataCoordinate, FileDataset, DatasetRef,
                             CollectionType, Timespan)
from lsst.daf.butler.core.utils import getFullTypeName
from lsst.obs.base import Instrument
from lsst.obs.base.gen2to3 import TranslatorFactory, PhysicalFilterToBandKeyHandler

from ..hsc.hscPupil import HscPupilFactory
from ..hsc.hscFilters import HSC_FILTER_DEFINITIONS
from ..hsc.makeTransmissionCurves import (getSensorTransmission, getOpticsTransmission,
                                          getFilterTransmission, getAtmosphereTransmission)
from .strayLight.formatter import SubaruStrayLightDataFormatter

log = logging.getLogger(__name__)


class HyperSuprimeCam(Instrument):
    """Gen3 Butler specialization class for Subaru's Hyper Suprime-Cam.
    """

    policyName = "hsc"
    obsDataPackage = "obs_subaru_data"
    filterDefinitions = HSC_FILTER_DEFINITIONS
    additionalCuratedDatasetTypes = ("bfKernel", "transmission_optics", "transmission_sensor",
                                     "transmission_filter", "transmission_atmosphere", "yBackground")

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        packageDir = getPackageDir("obs_subaru")
        self.configPaths = [os.path.join(packageDir, "config"),
                            os.path.join(packageDir, "config", self.policyName)]

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
        with registry.transaction():
            registry.syncDimensionData(
                "instrument",
                {
                    "name": self.getName(),
                    "detector_max": 200,
                    "visit_max": obsMax,
                    "exposure_max": obsMax,
                    "class_name": getFullTypeName(self),
                }
            )
            for detector in camera:
                registry.syncDimensionData(
                    "detector",
                    {
                        "instrument": self.getName(),
                        "id": detector.getId(),
                        "full_name": detector.getName(),
                        # TODO: make sure these definitions are consistent with
                        # those extracted by astro_metadata_translator, and
                        # test that they remain consistent somehow.
                        "name_in_raft": detector.getName().split("_")[1],
                        "raft": detector.getName().split("_")[0],
                        "purpose": str(detector.getType()).split(".")[-1],
                    }
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
        path = os.path.join(getPackageDir("obs_subaru"), self.policyName, "camera")
        return self._getCameraFromPath(path)

    @staticmethod
    @lru_cache()
    def _getCameraFromPath(path):
        """Return the camera geometry given solely the path to the location
        of that definition."""
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
        path = os.path.join(getPackageDir("obs_subaru"), self.policyName, "brighter_fatter_kernel.pkl")
        with open(path, "rb") as fd:
            kernel = pickle.load(fd, encoding='latin1')  # encoding for pickle written with Python 2
        return kernel

    def writeAdditionalCuratedCalibrations(self, butler, collection=None, labels=()):
        # Register the CALIBRATION collection that adds validity ranges.
        # This does nothing if it is already registered.
        if collection is None:
            collection = self.makeCalibrationCollectionName(*labels)
        butler.registry.registerCollection(collection, type=CollectionType.CALIBRATION)

        # Register the RUN collection that holds these datasets directly.  We
        # only need one because all of these datasets have the same (unbounded)
        # validity range right now.
        run = self.makeUnboundedCalibrationRunName(*labels)
        butler.registry.registerRun(run)
        baseDataId = butler.registry.expandDataId(instrument=self.getName())
        refs = []

        # Write brighter-fatter kernel, with an infinite validity range.
        datasetType = DatasetType("bfKernel", ("instrument",), "NumpyArray",
                                  universe=butler.registry.dimensions,
                                  isCalibration=True)
        butler.registry.registerDatasetType(datasetType)

        # Load and then put instead of just moving the file in part to ensure
        # the version in-repo is written with Python 3 and does not need
        # `encoding='latin1'` to be read.
        bfKernel = self.getBrighterFatterKernel()
        refs.append(butler.put(bfKernel, datasetType, baseDataId, run=run))

        # The following iterate over the values of the dictionaries returned
        # by the transmission functions and ignore the date that is supplied.
        # This is due to the dates not being ranges but single dates,
        # which do not give the proper notion of validity. As such unbounded
        # calibration labels are used when inserting into the database.
        # In the future these could and probably should be updated to
        # properly account for what ranges are considered valid.

        # Write optical transmissions
        opticsTransmissions = getOpticsTransmission()
        datasetType = DatasetType("transmission_optics",
                                  ("instrument",),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions,
                                  isCalibration=True)
        butler.registry.registerDatasetType(datasetType)
        for entry in opticsTransmissions.values():
            if entry is None:
                continue
            refs.append(butler.put(entry, datasetType, baseDataId, run=run))

        # Write transmission sensor
        sensorTransmissions = getSensorTransmission()
        datasetType = DatasetType("transmission_sensor",
                                  ("instrument", "detector",),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions,
                                  isCalibration=True)
        butler.registry.registerDatasetType(datasetType)
        for entry in sensorTransmissions.values():
            if entry is None:
                continue
            for sensor, curve in entry.items():
                dataId = DataCoordinate.standardize(baseDataId, detector=sensor)
                refs.append(butler.put(curve, datasetType, dataId, run=run))

        # Write filter transmissions
        filterTransmissions = getFilterTransmission()
        datasetType = DatasetType("transmission_filter",
                                  ("instrument", "physical_filter",),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions,
                                  isCalibration=True)
        butler.registry.registerDatasetType(datasetType)
        for entry in filterTransmissions.values():
            if entry is None:
                continue
            for band, curve in entry.items():
                dataId = DataCoordinate.standardize(baseDataId, physical_filter=band)
                refs.append(butler.put(curve, datasetType, dataId, run=run))

        # Write atmospheric transmissions
        atmosphericTransmissions = getAtmosphereTransmission()
        datasetType = DatasetType("transmission_atmosphere", ("instrument",),
                                  "TransmissionCurve",
                                  universe=butler.registry.dimensions,
                                  isCalibration=True)
        butler.registry.registerDatasetType(datasetType)
        for entry in atmosphericTransmissions.values():
            if entry is None:
                continue
            refs.append(butler.put(entry, datasetType, {"instrument": self.getName()}, run=run))

        # Associate all datasets with the unbounded validity range.
        butler.registry.certify(collection, refs, Timespan(begin=None, end=None))

    def ingestStrayLightData(self, butler, directory, *, transfer=None, collection=None, labels=()):
        """Ingest externally-produced y-band stray light data files into
        a data repository.

        Parameters
        ----------
        butler : `lsst.daf.butler.Butler`
            Butler to write with.  Any collections associated with it are
            ignored in favor of ``collection`` and/or ``labels``.
        directory : `str`
            Directory containing yBackground-*.fits files.
        transfer : `str`, optional
            If not `None`, must be one of 'move', 'copy', 'hardlink', or
            'symlink', indicating how to transfer the files.
        collection : `str`, optional
            Name to use for the calibration collection that associates all
            datasets with a validity range.  If this collection already exists,
            it must be a `~CollectionType.CALIBRATION` collection, and it must
            not have any datasets that would conflict with those inserted by
            this method.  If `None`, a collection name is worked out
            automatically from the instrument name and other metadata by
            calling ``makeCuratedCalibrationCollectionName``, but this
            default name may not work well for long-lived repositories unless
            ``labels`` is also provided (and changed every time curated
            calibrations are ingested).
        labels : `Sequence` [ `str` ], optional
            Extra strings to include in collection names, after concatenating
            them with the standard collection name delimeter.  If provided,
            these are inserted into to the names of the `~CollectionType.RUN`
            collections that datasets are inserted directly into, as well the
            `~CollectionType.CALIBRATION` collection if it is generated
            automatically (i.e. if ``collection is None``).  Usually this is
            just the name of the ticket on which the calibration collection is
            being created.
        """
        # Register the CALIBRATION collection that adds validity ranges.
        # This does nothing if it is already registered.
        if collection is None:
            collection = self.makeCalibrationCollectionName(*labels)
        butler.registry.registerCollection(collection, type=CollectionType.CALIBRATION)

        # Register the RUN collection that holds these datasets directly.  We
        # only need one because there is only one validity range and hence no
        # data ID conflicts even when there are no validity ranges.
        run = self.makeUnboundedCalibrationRunName(*labels)
        butler.registry.registerRun(run)

        # LEDs covered up around 2018-01-01, no need for correctin after that
        # date.
        timespan = Timespan(begin=None, end=astropy.time.Time("2018-01-01", format="iso", scale="tai"))
        datasets = []
        # TODO: should we use a more generic name for the dataset type?
        # This is just the (rather HSC-specific) name used in Gen2, and while
        # the instances of this dataset are camera-specific, the datasetType
        # (which is used in the generic IsrTask) should not be.
        datasetType = DatasetType("yBackground",
                                  dimensions=("physical_filter", "detector",),
                                  storageClass="StrayLightData",
                                  universe=butler.registry.dimensions,
                                  isCalibration=True)
        for detector in self.getCamera():
            path = os.path.join(directory, f"ybackground-{detector.getId():03d}.fits")
            if not os.path.exists(path):
                log.warning("No stray light data found for detector %s @ %s.", detector.getId(), path)
                continue
            ref = DatasetRef(datasetType, dataId={"instrument": self.getName(),
                                                  "detector": detector.getId(),
                                                  "physical_filter": "HSC-Y"})
            datasets.append(FileDataset(refs=ref, path=path, formatter=SubaruStrayLightDataFormatter))
        butler.registry.registerDatasetType(datasetType)
        with butler.transaction():
            butler.ingest(*datasets, transfer=transfer, run=run)
            refs = []
            for dataset in datasets:
                refs.extend(dataset.refs)
            butler.registry.certify(collection, refs, timespan)

    def makeDataIdTranslatorFactory(self) -> TranslatorFactory:
        # Docstring inherited from lsst.obs.base.Instrument.
        factory = TranslatorFactory()
        factory.addGenericInstrumentRules(self.getName())
        # Translate Gen2 `filter` to band if it hasn't been consumed
        # yet and gen2keys includes tract.
        factory.addRule(PhysicalFilterToBandKeyHandler(self.filterDefinitions),
                        instrument=self.getName(), gen2keys=("filter", "tract"), consume=("filter",))
        return factory
