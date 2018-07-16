#
# LSST Data Management System
# Copyright 2017 LSST/AURA.
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <http://www.lsstcorp.org/LegalNotices/>.
#
import lsst.meas.base

__all__ = ["FilterFractionPlugin"]


class FilterFractionPluginConfig(lsst.meas.base.SingleFramePluginConfig):
    pass


@lsst.meas.base.register("subaru_FilterFraction")
class FilterFractionPlugin(lsst.meas.base.SingleFramePlugin):

    ConfigClass = FilterFractionPluginConfig

    @classmethod
    def getExecutionOrder(cls):
        return cls.FLUX_ORDER

    def __init__(self, config, name, schema, metadata):
        lsst.meas.base.SingleFramePlugin.__init__(self, config, name, schema, metadata)
        self.unweightedKey = schema.addField(
            schema.join(name, "unweighted"), type=float,
            doc=("Fraction of observations taken with the preferred version of this filter.  "
                 "For filters with a single version, this will always be 1; for HSC i or r, "
                 "the preferred filter is the replacement filter (i2 or r2)."),
            doReplace=True
        )
        self.weightedKey = schema.addField(
            schema.join(name, "weighted"), type=float,
            doc=("Contribution-weighted fraction of observations taken with the preferred "
                 "version of this filter.  "
                 "For filters with a single version, this will always be 1; for HSC i or r, "
                 "the preferred filter is the replacement filter (i2 or r2)."),
            doReplace=True
        )

    def measure(self, measRecord, exposure):
        try:
            ccds = exposure.getInfo().getCoaddInputs().ccds
        except AttributeError:
            raise lsst.meas.base.FatalAlgorithmError("FilterFraction can only be run on coadds.")
        overlapping = ccds.subsetContaining(measRecord.getCentroid(), exposure.getWcs(),
                                            includeValidPolygon=True)
        if not overlapping:
            measRecord.set(self.key, float("NaN"))  # no inputs in any filter
            return
        counts = {}
        weights = {}
        filterKey = overlapping.schema.find("filter").key
        try:
            weightKey = overlapping.schema.find("weight").key
        except KeyError:
            weightKey = None
        for ccd in overlapping:
            filterName = ccd.get(filterKey)
            counts[filterName] = counts.get(filterName, 0) + 1
            if weightKey is not None:
                weight = ccd.get(weightKey)
                weights[filterName] = weights.get(filterName, 0.0) + weight
        if "i" in counts:
            weird = set(counts.keys()) - set(["i", "i2"])
            if weird:
                raise lsst.meas.base.FatalAlgorithmError(
                    "Unexpected filter combination found in coadd: %s" % counts.keys()
                )
            measRecord.set(self.unweightedKey, counts.get("i2", 0.0)/len(overlapping))
            if weightKey is not None:
                measRecord.set(self.weightedKey, weights.get("i2", 0.0)/sum(weights.values()))
        elif "r" in counts:
            weird = set(counts.keys()) - set(["r", "r2"])
            if weird:
                raise lsst.meas.base.FatalAlgorithmError(
                    "Unexpected filter combination found in coadd: %s" % counts.keys()
                )
            measRecord.set(self.unweightedKey, counts.get("r2", 0.0)/len(overlapping))
            if weightKey is not None:
                measRecord.set(self.weightedKey, weights.get("r2", 0.0)/sum(weights.values()))
        elif len(counts) > 1:
            raise lsst.meas.base.FatalAlgorithmError(
                "Unexpected filter combination found in coadd: %s" % counts.keys()
            )
        else:
            measRecord.set(self.unweightedKey, 1.0)
            if weightKey is not None:
                measRecord.set(self.weightedKey, 1.0)

    def fail(self, measRecord, error=None):
        # this plugin can only raise fatal exceptions
        pass
