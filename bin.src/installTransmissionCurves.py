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
import argparse


from lsst.obs.hsc import makeTransmissionCurves, HscMapper
from lsst.daf.persistence import Butler


def main(butler):
    for start, nested in makeTransmissionCurves.getFilterTransmission().items():
        for name, curve in nested.items():
            if curve is not None:
                butler.put(curve, "transmission_filter", filter=name)
    for start, nested in makeTransmissionCurves.getSensorTransmission().items():
        for ccd, curve in nested.items():
            if curve is not None:
                butler.put(curve, "transmission_sensor", ccd=ccd)
    for start, curve in makeTransmissionCurves.getOpticsTransmission().items():
        if curve is not None:
            butler.put(curve, "transmission_optics")
    for start, curve in makeTransmissionCurves.getAtmosphereTransmission().items():
        if curve is not None:
            butler.put(curve, "transmission_atmosphere")


if __name__ == "__main__":
    parser = argparse.ArgumentParser("Install HSC transmission curve datasets into a data repository.")
    parser.add_argument('root', type=str, help="Root directory of the repository to write to.")
    args = parser.parse_args()
    butler = Butler(outputs={'root': args.root, 'mode': 'rw', 'mapper': HscMapper})
    main(butler)
