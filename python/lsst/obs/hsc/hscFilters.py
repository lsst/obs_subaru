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
    FilterDefinition(abstract_filter="UNRECOGNISED", physical_filter="NONE", lambdaEff=0,
                     alias=["NONE", "None", "Unrecognised", "UNRECOGNISED",
                            "Unrecognized", "UNRECOGNIZED", "NOTSET"]),
    FilterDefinition(physical_filter="HSC-G",
                     abstract_filter="g",
                     lambdaEff=477, alias={'W-S-G+'}),
    FilterDefinition(physical_filter="HSC-R",
                     abstract_filter="r",
                     lambdaMin=562, lambdaMax=725,
                     lambdaEff=623, alias={'W-S-R+'}),
    FilterDefinition(physical_filter="ENG-R1",
                     abstract_filter="r1",
                     lambdaMin=562, lambdaMax=725,
                     lambdaEff=623, alias={'109'}),
    FilterDefinition(physical_filter="HSC-I",
                     abstract_filter="i",
                     lambdaEff=775, alias={'W-S-I+'}),
    FilterDefinition(physical_filter="HSC-Z",
                     abstract_filter="z",
                     lambdaEff=925, alias={'W-S-Z+'}),
    FilterDefinition(physical_filter="HSC-Y",
                     abstract_filter="y",
                     lambdaEff=990, alias={'W-S-ZR'}),
    FilterDefinition(physical_filter="NB0387",
                     abstract_filter='N387', lambdaEff=387),
    FilterDefinition(physical_filter="NB0515",
                     abstract_filter='N515', lambdaEff=515),
    FilterDefinition(physical_filter="NB0656",
                     abstract_filter='N656', lambdaEff=656),
    FilterDefinition(physical_filter="NB0816",
                     abstract_filter='N816', lambdaEff=816),
    FilterDefinition(physical_filter="NB0921",
                     abstract_filter='N921', lambdaEff=921),
    FilterDefinition(physical_filter="NB1010",
                     abstract_filter='N1010', lambdaEff=1010),
    FilterDefinition(physical_filter="SH",
                     abstract_filter='SH', lambdaEff=0),
    FilterDefinition(physical_filter="PH",
                     abstract_filter='PH', lambdaEff=0),
    FilterDefinition(physical_filter="NB0527",
                     abstract_filter='N527', lambdaEff=527),
    FilterDefinition(physical_filter="NB0718",
                     abstract_filter='N718', lambdaEff=718),
    FilterDefinition(physical_filter="IB0945",
                     abstract_filter='I945', lambdaEff=945),
    FilterDefinition(physical_filter="NB0973",
                     abstract_filter='N973', lambdaEff=973),
    FilterDefinition(physical_filter="HSC-I2",
                     abstract_filter="i",
                     afw_name='i2', lambdaEff=775),
    FilterDefinition(physical_filter="HSC-R2",
                     abstract_filter="r",
                     afw_name='r2', lambdaEff=623),
    FilterDefinition(physical_filter="NB0468",
                     abstract_filter='N468', lambdaEff=468),
    FilterDefinition(physical_filter="NB0926",
                     abstract_filter='N926', lambdaEff=926),
    FilterDefinition(physical_filter="NB0400",
                     abstract_filter='N400', lambdaEff=400),
)
