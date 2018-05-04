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

"""Gen3 Butler specializations for Hyper Suprime-Cam.
"""

import re

from lsst.daf.butler.instrument import Instrument
from lsst.daf.butler.gen2convert import Translator, KeyHandler, ConstantKeyHandler, CopyKeyHandler

__all__ = ("HyperSuprimeCam",)


# Regular expression that matches HSC PhysicalFilter names (the same as Gen2
# filternames), with a group that can be lowercased to yield the
# associated AbstractFilter.
FILTER_REGEX = re.compile(r"HSC\-([GRIZY])2?")


class HyperSuprimeCam(Instrument):
    """Gen3 Butler specialization class for Subaru's Hyper Suprime-Cam.

    The current implementation simply retrieves the information it needs
    from a Gen2 HscMapper instance (the only constructor argument).  This
    will obviously need to be changed before Gen2 is retired, but it avoids
    duplication for now.
    """

    camera = "HSC"

    def __init__(self, mapper):
        self.sensors = [
            dict(
                sensor=sensor.getId(),
                name=sensor.getName(),
                # getType() returns a pybind11-wrapped enum, which
                # unfortunately has no way to extract the name of just
                # the value (it's always prefixed by the enum type name).
                purpose=str(sensor.getType()).split(".")[-1],
                # The most useful grouping of sensors in HSC is by their
                # orientation w.r.t. the focal plane, so that's what
                # we put in the 'group' field.
                group="NQUARTER{:d}".format(sensor.getOrientation().getNQuarter() % 4)
            )
            for sensor in mapper.camera
        ]
        self.physicalFilters = []
        for name in mapper.filters:
            # We use one of grizy for the abstract filter, when appropriate,
            # which we identify as when the physical filter starts with
            # "HSC-[GRIZY]".  Note that this means that e.g. "HSC-I" and
            # "HSC-I2" are both mapped to abstract filter "i".
            m = FILTER_REGEX.match(name)
            self.physicalFilters.append(
                dict(
                    physical_filter=name,
                    abstract_filter=m.group(1).lower() if m is not None else None
                )
            )


class HscAbstractFilterKeyHandler(KeyHandler):
    """KeyHandler for HSC filter keys that should be mapped to AbstractFilters.
    """

    __slots__ = ()

    def __init__(self):
        super().__init__("abstract_filter", "AbstractFilter")

    def extract(self, gen2id, skyMap, skyMapName):
        physical = gen2id["filter"]
        m = FILTER_REGEX.match(physical)
        if m:
            return m.group(1).lower()
        return physical


class HscPhysicalFilterKeyHandler(KeyHandler):
    """KeyHandler for HSC filter keys that should be mapped to PhysicalFilters.
    """

    __slots__ = ()

    def __init__(self):
        super().__init__("physical_filter", "PhysicalFilter")

    def translate(self, gen2id, gen3id, skyMap, skyMapName):
        super().translate(gen2id, gen3id, skyMap, skyMapName)
        # also add camera
        gen3id["camera"] = "HSC"

    def extract(self, gen2id, skyMap, skyMapName):
        return gen2id["filter"]


# Add camera to Gen3 data ID if Gen2 contains "visit" or "ccd".
# (Both rules will match, so we'll actually set camera in the same dict twice).
Translator.addRule(ConstantKeyHandler("camera", "Camera", "HSC"),
                   camera="HSC", gen2keys=("visit",), consume=False)
Translator.addRule(ConstantKeyHandler("camera", "Camera", "HSC"),
                   camera="HSC", gen2keys=("ccd",), consume=False)

# Copy Gen2 'visit' to Gen3 'exposure' for raw only.
Translator.addRule(CopyKeyHandler("exposure", "Exposure", "visit"),
                   camera="HSC", datasetTypeName="raw", gen2keys=("visit",))

# Copy Gen2 'visit' to Gen3 'visit' otherwise
Translator.addRule(CopyKeyHandler("visit", "Visit"), camera="HSC", gen2keys=("visit",))

# Copy Gen2 'ccd' to Gen3 'sensor;
Translator.addRule(CopyKeyHandler("sensor", "Sensor", "ccd"), camera="HSC", gen2keys=("ccd",))

# Translate Gen2 `filter` to AbstractFilter iff Gen2 data ID contains "tract".
Translator.addRule(HscAbstractFilterKeyHandler(), camera="HSC", gen2keys=("tract", "filter"),
                   consume=("filter",))
Translator.addRule(HscPhysicalFilterKeyHandler(), camera="HSC", gen2keys=("filter",))
