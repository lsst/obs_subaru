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

"""Gen2->Gen3 Data repo conversion specializations for Hyper Suprime-Cam
"""

from lsst.obs.base.gen2to3 import Translator, KeyHandler, ConstantKeyHandler, CopyKeyHandler

from .instrument import FILTER_REGEX


__all__ = ()


class HscAbstractFilterKeyHandler(KeyHandler):
    """KeyHandler for HSC filter keys that should be mapped to AbstractFilters.
    """

    __slots__ = ()

    def __init__(self):
        super().__init__("abstract_filter")

    def extract(self, gen2id, skyMap, skyMapName, datasetTypeName):
        physical = gen2id["filter"]
        m = FILTER_REGEX.match(physical)
        if m:
            return m.group(1).lower()
        return physical


# Add instrument to Gen3 data ID if Gen2 contains "visit" or "ccd".
# (Both rules will match, so we'll actually set instrument in the same dict twice).
Translator.addRule(ConstantKeyHandler("instrument", "HSC"),
                   instrument="HSC", gen2keys=("visit",), consume=False)
Translator.addRule(ConstantKeyHandler("instrument", "HSC"),
                   instrument="HSC", gen2keys=("ccd",), consume=False)

# Copy Gen2 'visit' to Gen3 'exposure' for raw only.  Also consume filter,
# since that's implied by 'exposure' in Gen3.
Translator.addRule(CopyKeyHandler("exposure", "visit"),
                   instrument="HSC", datasetTypeName="raw", gen2keys=("visit",),
                   consume=("visit", "filter"))

# Copy Gen2 'visit' to Gen3 'visit' otherwise.  Also consume filter.
Translator.addRule(CopyKeyHandler("visit"), instrument="HSC", gen2keys=("visit",),
                   consume=("visit", "filter"))

# Copy Gen2 'ccd' to Gen3 'detector;
Translator.addRule(CopyKeyHandler("detector", "ccd"), instrument="HSC", gen2keys=("ccd",))

# Translate Gen2 `filter` to AbstractFilter if it hasn't been consumed yet and gen2keys includes tract.
Translator.addRule(HscAbstractFilterKeyHandler(), instrument="HSC", gen2keys=("filter", "tract"),
                   consume=("filter",))

# Add instrument for HSC transmission curve datasets (transmission_sensor is
# already handled by the above translators).
Translator.addRule(ConstantKeyHandler("instrument", "HSC"),
                   instrument="HSC", datasetTypeName="transmission_optics")
Translator.addRule(ConstantKeyHandler("instrument", "HSC"),
                   instrument="HSC", datasetTypeName="transmission_atmosphere")
Translator.addRule(ConstantKeyHandler("instrument", "HSC"),
                   instrument="HSC", datasetTypeName="transmission_filter")
Translator.addRule(CopyKeyHandler("physical_filter", "filter"),
                   instrument="HSC", datasetTypeName="transmission_filter")

# Add calibration mapping for filter for filter dependent types
for calibType in ('flat', 'sky', 'fringe'):
    Translator.addRule(CopyKeyHandler("physical_filter", "filter"),
                       instrument="HSC", datasetTypeName=calibType)
