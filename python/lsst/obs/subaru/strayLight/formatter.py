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

from lsst.obs.base.formatters.fitsGeneric import FitsGenericFormatter

from .yStrayLight import SubaruStrayLightData

__all__ = ("SubaruStrayLightDataFormatter",)


class SubaruStrayLightDataFormatter(FitsGenericFormatter):
    """Gen3 Butler Formatters for HSC y-band stray light correction data.
    """

    extension = ".fits"

    def _readFile(self, path, pytype=None):
        # Docstring inherited from FitsGenericFormatter._readFile.
        return SubaruStrayLightData.readFits(path)

    def _writeFile(self, inMemoryDataset):
        # Docstring inherited from FitsGenericFormatter._writeFile.
        raise NotImplementedError("SubaruStrayLightData cannot be written directly.")
