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

from lsst.obs.base import FilterDefinition, FilterDefinitionCollection

# SDSS g': http://www.naoj.org/Observing/Instruments/SCam/txt/g.txt
# SDSS r': http://www.naoj.org/Observing/Instruments/SCam/txt/r.txt
# SDSS i': http://www.naoj.org/Observing/Instruments/SCam/txt/i.txt
# SDSS z': http://www.naoj.org/Observing/Instruments/SCam/txt/z.txt
# y-band: Shimasaku et al., 2005, PASJ, 57, 447

# The order of these filters matters as their IDs are used to generate at
# least some object IDs (e.g. on coadds) and changing the order will
# invalidate old objIDs
HSC_FILTER_DEFINITIONS = FilterDefinitionCollection(
    FilterDefinition(band="unknown", physical_filter="unknown",
                     alias=["UNKNOWN", "Unrecognised", "UNRECOGNISED",
                            "Unrecognized", "UNRECOGNIZED", "NOTSET"]),
    FilterDefinition(band="white", physical_filter="empty",
                     alias=["NONE"]),
    FilterDefinition(physical_filter="HSC-G",
                     band="g",
                     alias={'W-S-G+'}),
    FilterDefinition(physical_filter="HSC-R",
                     band="r",
                     doc="Original r-band filter, replaced in July 2016 with HSC-R2.",
                     alias={'W-S-R+'}),
    FilterDefinition(physical_filter="ENG-R1",
                     band="r",
                     doc="A filter used during early camera construction;"
                         "very little data was taken with this filter.",
                     alias={'109'}),
    FilterDefinition(physical_filter="HSC-I",
                     band="i",
                     doc="Original i-band filter, replaced in February 2016 with HSC-I2.",
                     alias={'W-S-I+'}),
    FilterDefinition(physical_filter="HSC-Z",
                     band="z",
                     alias={'W-S-Z+'}),
    FilterDefinition(physical_filter="HSC-Y",
                     band="y",
                     alias={'W-S-ZR'}),
    FilterDefinition(physical_filter="NB0387",
                     band='N387'),
    FilterDefinition(physical_filter="NB0515",
                     band='N515'),
    FilterDefinition(physical_filter="NB0656",
                     band='N656'),
    FilterDefinition(physical_filter="NB0816",
                     band='N816'),
    FilterDefinition(physical_filter="NB0921",
                     band='N921'),
    FilterDefinition(physical_filter="NB1010",
                     band='N1010'),
    FilterDefinition(physical_filter="SH",
                     band='SH',
                     doc="Shack-Hartman filter used for optical alignment."),
    FilterDefinition(physical_filter="PH",
                     band='PH'),
    FilterDefinition(physical_filter="NB0527",
                     band='N527'),
    FilterDefinition(physical_filter="NB0718",
                     band='N718'),
    FilterDefinition(physical_filter="IB0945",
                     band='I945'),
    FilterDefinition(physical_filter="NB0973",
                     band='N973'),
    FilterDefinition(physical_filter="HSC-I2",
                     band="i",
                     doc="A February 2016 replacement for HSC-I, with better uniformity.",
                     afw_name='i2'),
    FilterDefinition(physical_filter="HSC-R2",
                     band="r",
                     doc="A July 2016 replacement for HSC-R, with better uniformity.",
                     afw_name='r2'),
    FilterDefinition(physical_filter="NB0468",
                     band='N468'),
    FilterDefinition(physical_filter="NB0926",
                     band='N926'),
    FilterDefinition(physical_filter="NB0400",
                     band='N400'),
    FilterDefinition(physical_filter="NB0391", band="N391"),
    FilterDefinition(physical_filter="NB0395", band="N395"),
    FilterDefinition(physical_filter="NB0430", band="N430"),
    FilterDefinition(physical_filter="NB0497", band="N497"),
    FilterDefinition(physical_filter="NB0506", band="N506"),
    FilterDefinition(physical_filter="NB0872", band="N872"),
    FilterDefinition(physical_filter="EB-GRI", band="gri"),
)
