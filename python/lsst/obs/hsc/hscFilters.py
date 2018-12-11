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


# SDSS g': http://www.naoj.org/Observing/Instruments/SCam/txt/g.txt
# SDSS r': http://www.naoj.org/Observing/Instruments/SCam/txt/r.txt
# SDSS i': http://www.naoj.org/Observing/Instruments/SCam/txt/i.txt
# SDSS z': http://www.naoj.org/Observing/Instruments/SCam/txt/z.txt
# y-band: Shimasaku et al., 2005, PASJ, 57, 447

# The order of these filters matters as their IDs are used to generate at
# least some object IDs (e.g. on coadds) and changing the order will
# invalidate old objIDs
HSC_FILTER_DEFINITIONS = [
    dict(name="UNRECOGNISED", lambdaEff=0,
         alias=["NONE", "None", "Unrecognised", "UNRECOGNISED",
                "Unrecognized", "UNRECOGNIZED", "NOTSET", ]),
    dict(name='g', lambdaEff=477, alias=['W-S-G+', 'HSC-G']),
    dict(name='r', lambdaEff=623, alias=['W-S-R+', 'HSC-R']),
    dict(name='r1', lambdaEff=623, alias=['109', 'ENG-R1']),
    dict(name='i', lambdaEff=775, alias=['W-S-I+', 'HSC-I']),
    dict(name='z', lambdaEff=925, alias=['W-S-Z+', 'HSC-Z']),
    dict(name='y', lambdaEff=990, alias=['W-S-ZR', 'HSC-Y']),
    dict(name='N387', lambdaEff=387, alias=['NB0387']),
    dict(name='N515', lambdaEff=515, alias=['NB0515']),
    dict(name='N656', lambdaEff=656, alias=['NB0656']),
    dict(name='N816', lambdaEff=816, alias=['NB0816']),
    dict(name='N921', lambdaEff=921, alias=['NB0921']),
    dict(name='N1010', lambdaEff=1010, alias=['NB1010']),
    dict(name='SH', lambdaEff=0, alias=['SH', ]),
    dict(name='PH', lambdaEff=0, alias=['PH', ]),
    dict(name='N527', lambdaEff=527, alias=['NB0527']),
    dict(name='N718', lambdaEff=718, alias=['NB0718']),
    dict(name='I945', lambdaEff=945, alias=['IB0945']),
    dict(name='N973', lambdaEff=973, alias=['NB0973']),
    dict(name='i2', lambdaEff=775, alias=['HSC-I2']),
    dict(name='r2', lambdaEff=623, alias=['HSC-R2']),
    dict(name='N468', lambdaEff=468, alias=['NB0468']),
    dict(name='N926', lambdaEff=926, alias=['NB0926']),
    dict(name='N400', lambdaEff=400, alias=['NB0400']),
]

HSC_FILTER_NAMES = [
    "HSC-G",
    "HSC-R",
    "HSC-R2",
    "HSC-I",
    "HSC-I2",
    "HSC-Z",
    "HSC-Y",
    "ENG-R1",
    "NB0387",
    "NB0400",
    "NB0468",
    "NB0515",
    "NB0527",
    "NB0656",
    "NB0718",
    "NB0816",
    "NB0921",
    "NB0926",
    "IB0945",
    "NB0973",
    "NB1010",
    "SH",
    "PH",
    "NONE",
    "UNRECOGNISED",
]
