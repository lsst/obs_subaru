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

from typing import Any
from lsst.resources import ResourcePath
from lsst.daf.butler import FormatterV2, FormatterNotImplementedError

from .yStrayLight import SubaruStrayLightData

__all__ = ("SubaruStrayLightDataFormatter",)


class SubaruStrayLightDataFormatter(FormatterV2):
    """Gen3 Butler Formatters for HSC y-band stray light correction data.
    """

    can_read_from_local_file = True
    default_extension = ".fits"

    def read_from_local_file(
        self, path: str, component: str | None = None, expected_size: int = -1
    ) -> Any:
        # Docstring inherited from FitsGenericFormatter._readFile.
        return SubaruStrayLightData.readFits(path)

    def write_local_file(self, in_memory_dataset: Any, uri: ResourcePath) -> None:
        # Docstring inherited from FitsGenericFormatter._writeFile.
        raise FormatterNotImplementedError("SubaruStrayLightData cannot be written directly.")
