#!/usr/bin/env python
#
# LSST Data Management System
#
# Copyright 2018 AURA/LSST.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
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
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <https://www.lsstcorp.org/LegalNotices/>.
#
import os
import glob
import numpy as np

from lsst.afw.image import TransmissionCurve
from lsst.utils import getPackageDir
from . import hscFilters

__all__ = ("getOpticsTransmission", "getSensorTransmission", "getAtmosphereTransmission",
           "getFilterTransmission",)

DATA_DIR = os.path.join(getPackageDir("obs_subaru"), "hsc", "transmission")

HSC_BEGIN = "2012-12-18"  # initial date for curves valid for entire lifetime of HSC


def getLongFilterName(short):
    """Return a long HSC filter name (e.g. 'HSC-R') that's usable as a data ID
    value from the short one (e.g. 'r') declared canonical in afw.image.Filter.
    """
    if short.startswith("HSC"):
        return short
    if short.startswith("NB") or short.startswith("IB"):
        num = int(short[2:].lstrip("0"))
        return "%s%04d" % (short[:2], num)
    for filter in hscFilters.HSC_FILTER_DEFINITIONS:
        if short == filter.afw_name or short == filter.band:
            return filter.physical_filter
    return short


def readTransmissionCurveFromFile(filename, unit="angstrom", atMin=None, atMax=None):
    """Load a spatial TransmissionCurve from a text file with wavelengths and
    throughputs in columns.

    Parameters
    ----------
    filename : `str`
        Name of the file to read.
    unit : `str`
        Wavelength unit; one of "nm" or "angstrom".
    atMin : `float`
        Throughput to use at wavelengths below the tabulated minimum.  If
        ``None``, the tabulated throughput at the mininum will be used.
    atMax : `float`
        Throughput to use at wavelengths above the tabulated maximum.  If
        ``None``, the tabulated throughput at the maximum will be used.
    """
    wavelengths, throughput = np.loadtxt(os.path.join(DATA_DIR, filename), usecols=[0, 1], unpack=True)
    i = np.argsort(wavelengths)
    wavelengths = wavelengths[i]
    throughput = throughput[i]
    if unit == "nm":
        wavelengths *= 10
    elif unit != "angstrom":
        raise ValueError("Invalid wavelength unit")
    if atMin is None:
        atMin = throughput[0]
    if atMax is None:
        atMax = throughput[-1]
    return TransmissionCurve.makeSpatiallyConstant(throughput=throughput, wavelengths=wavelengths,
                                                   throughputAtMin=atMin, throughputAtMax=atMax)


def getOpticsTransmission():
    """Return a dictionary of TransmissionCurves describing the combined
    throughput of HSC and the Subaru primary mirror.

    Dictionary keys are string dates (YYYY-MM-DD) indicating the beginning of
    the validity period for the curve stored as the associated dictionary
    value.  If the curve is spatially varying, it will be defined in focal
    plane coordinates.

    Dictionary values may be None to indicate that no TransmissionCurve is
    valid after the date provided in the key.
    """
    mirror2010 = readTransmissionCurveFromFile("M1-2010s.txt", unit="nm")
    camera = readTransmissionCurveFromFile("throughput_popt2.txt")
    camera *= readTransmissionCurveFromFile("throughput_win.txt")
    return {
        HSC_BEGIN: mirror2010*camera,
        "2017-10-01": None   # mirror recoating begins, approximately
    }


def getSensorTransmission():
    """Return a nested dictionary of TransmissionCurves describing the
    throughput of each sensor.

    Outer directionary keys are string dates (YYYY-MM-DD), with values
    a dictionary mapping CCD ID to TransmissionCurve.  If the curve
    is spatially varying, it will be defined in pixel coordinates.

    Outer dictionary values may be None to indicate that no TransmissionCurve
    is valid after the date provided in the key.
    """
    qe = readTransmissionCurveFromFile("qe_ccd_HSC.txt", atMin=0.0, atMax=0.0)
    return {HSC_BEGIN: {n: qe for n in range(112)}}


def getAtmosphereTransmission():
    """Return a dictionary of TransmissionCurves describing the atmospheric
    throughput at Mauna Kea.

    Dictionary keys are string dates (YYYY-MM-DD) indicating the beginning of
    the validity period for the curve stored as the associated dictionary
    value.  The curve is guaranteed not to be spatially-varying.

    Dictionary values may be None to indicate that no TransmissionCurve is
    valid after the date provided in the key.
    """
    average = readTransmissionCurveFromFile("modtran_maunakea_am12_pwv15_binned10ang.dat")
    return {HSC_BEGIN: average}


def getFilterTransmission():
    """Return a nested dictionary of TransmissionCurves describing the
    throughput of each HSC filter.

    Outer directionary keys are string dates (YYYY-MM-DD), with values
    a dictionary mapping filter name to TransmissionCurve.  If the curve
    is spatially varying, it will be defined in pixel coordinates.

    Filter curve names are in the long form used as data ID values (e.g.
    'HSC-I').

    Outer dictionary values may be None to indicate that no TransmissionCurve
    is valid after the date provided in the key.
    """
    module = {}
    filename = os.path.join(DATA_DIR, "filterTraces.py")
    with open(filename) as file:
        exec(compile(file.read(), filename, mode='exec'), module)
    result = {}
    for band, data in module["FILTER_DATA"].items():
        result[getLongFilterName(band)] = TransmissionCurve.makeRadial(
            throughput=data["T"], wavelengths=data["lam"]*10,
            radii=data['radius']/module["PIXEL_SIZE"],
            throughputAtMin=0.0, throughputAtMax=0.0
        )
    for filename in glob.glob(os.path.join(DATA_DIR, "wHSC-*.txt")):
        band = getLongFilterName(os.path.split(filename)[1][len("wHSC-"): -len(".txt")])
        if band not in result:
            result[band] = readTransmissionCurveFromFile(filename, atMin=0.0, atMax=0.0)
    return {HSC_BEGIN: result}
