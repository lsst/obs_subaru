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
    FilterDefinition(band="unknown", physical_filter="unknown", lambdaEff=0,
                     alias=["UNKNOWN", "Unrecognised", "UNRECOGNISED",
                            "Unrecognized", "UNRECOGNIZED", "NOTSET"]),
    FilterDefinition(band="white", physical_filter="empty", lambdaEff=0,
                     alias=["NONE"]),
    FilterDefinition(physical_filter="HSC-G",
                     band="g",
                     lambdaEff=477, alias={'W-S-G+'}),
    FilterDefinition(physical_filter="HSC-R",
                     band="r",
                     doc="Original r-band filter, replaced in July 2016 with HSC-R2.",
                     lambdaEff=623, alias={'W-S-R+'}),
    FilterDefinition(physical_filter="ENG-R1",
                     band="r",
                     doc="A filter used during early camera construction;"
                         "very little data was taken with this filter.",
                     lambdaEff=623, alias={'109'}),
    FilterDefinition(physical_filter="HSC-I",
                     band="i",
                     doc="Original i-band filter, replaced in February 2016 with HSC-I2.",
                     lambdaEff=775, alias={'W-S-I+'}),
    FilterDefinition(physical_filter="HSC-Z",
                     band="z",
                     lambdaEff=925, alias={'W-S-Z+'}),
    FilterDefinition(physical_filter="HSC-Y",
                     band="y",
                     lambdaEff=990, alias={'W-S-ZR'}),
    FilterDefinition(physical_filter="NB0387",
                     band='N387', lambdaEff=387),
    FilterDefinition(physical_filter="NB0515",
                     band='N515', lambdaEff=515),
    FilterDefinition(physical_filter="NB0656",
                     band='N656', lambdaEff=656),
    FilterDefinition(physical_filter="NB0816",
                     band='N816', lambdaEff=816),
    FilterDefinition(physical_filter="NB0921",
                     band='N921', lambdaEff=921),
    FilterDefinition(physical_filter="NB1010",
                     band='N1010', lambdaEff=1010),
    FilterDefinition(physical_filter="SH",
                     band='SH',
                     doc="Shack-Hartman filter used for optical alignment.",
                     lambdaEff=0),
    FilterDefinition(physical_filter="PH",
                     band='PH', lambdaEff=0),
    FilterDefinition(physical_filter="NB0527",
                     band='N527', lambdaEff=527),
    FilterDefinition(physical_filter="NB0718",
                     band='N718', lambdaEff=718),
    FilterDefinition(physical_filter="IB0945",
                     band='I945', lambdaEff=945),
    FilterDefinition(physical_filter="NB0973",
                     band='N973', lambdaEff=973),
    FilterDefinition(physical_filter="HSC-I2",
                     band="i",
                     doc="A February 2016 replacement for HSC-I, with better uniformity.",
                     afw_name='i2', lambdaEff=775),
    FilterDefinition(physical_filter="HSC-R2",
                     band="r",
                     doc="A July 2016 replacement for HSC-R, with better uniformity.",
                     afw_name='r2', lambdaEff=623),
    FilterDefinition(physical_filter="NB0468",
                     band='N468', lambdaEff=468),
    FilterDefinition(physical_filter="NB0926",
                     band='N926', lambdaEff=926),
    FilterDefinition(physical_filter="NB0400",
                     band='N400', lambdaEff=400),
)
