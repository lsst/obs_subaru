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
    """Record the fraction of the input images from two physical_filters
    at the positions of objects on the coadd.
    """

    ConfigClass = FilterFractionPluginConfig

    @classmethod
    def getExecutionOrder(cls):
        return cls.FLUX_ORDER

    def __init__(self, config, name, schema, metadata):
        lsst.meas.base.SingleFramePlugin.__init__(self, config, name, schema, metadata)
        self.unweightedKey = schema.addField(
            schema.join(name, "unweighted"), type=float,
            doc=("Fraction of observations taken with the preferred version of this filter.  "
                 "For filters with a single version, this will always be 1; for HSC-I or HSC-R, "
                 "the preferred filter is the replacement filter (HSC-I2 or HSC-R2)."),
            doReplace=True
        )
        self.weightedKey = schema.addField(
            schema.join(name, "weighted"), type=float,
            doc=("Contribution-weighted fraction of observations taken with the preferred "
                 "version of this filter.  "
                 "For filters with a single version, this will always be 1; for HSC HSC-I or HSC-R, "
                 "the preferred filter is the replacement filter (HSC-I2 or HSC-R2)."),
            doReplace=True
        )

    def measure(self, measRecord, exposure):
        """Count the number of filters used to create the coadd.

        Parameters
        ----------
        measRecord : `lsst.afw.table.SourceRecord`
            Record describing the object being measured. Previously-measured
            quantities may be retrieved from here, and it will be updated
            in-place with the outputs of this plugin.
        exposure : `lsst.afw.image.ExposureF`
            The Coadd exposure, which must contain a CoaddInput object
            describing the inputs to this Coadd, and a valid WCS.
        """

        try:
            ccds = exposure.getInfo().getCoaddInputs().ccds
        except AttributeError:
            raise lsst.meas.base.FatalAlgorithmError("FilterFraction can only be run on coadds.")
        overlapping = ccds.subsetContaining(measRecord.getCentroid(), exposure.getWcs(),
                                            includeValidPolygon=True)
        if not overlapping:
            # set both keys to nan if there are no inputs in any filter
            measRecord.set(self.unweightedKey, float("NaN"))
            measRecord.set(self.weightedKey, float("NaN"))
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
        if "HSC-I" in counts:
            weird = set(counts.keys()) - set(["HSC-I", "HSC-I2"])
            if weird:
                raise lsst.meas.base.FatalAlgorithmError(
                    "Unexpected filter combination found in coadd: %s" % counts.keys()
                )
            measRecord.set(self.unweightedKey, counts.get("HSC-I2", 0.0)/len(overlapping))
            if weightKey is not None:
                measRecord.set(self.weightedKey, weights.get("HSC-I2", 0.0)/sum(weights.values()))
        elif "HSC-R" in counts:
            weird = set(counts.keys()) - set(["HSC-R", "HSC-R2"])
            if weird:
                raise lsst.meas.base.FatalAlgorithmError(
                    "Unexpected filter combination found in coadd: %s" % counts.keys()
                )
            measRecord.set(self.unweightedKey, counts.get("HSC-R2", 0.0)/len(overlapping))
            if weightKey is not None:
                measRecord.set(self.weightedKey, weights.get("HSC-R2", 0.0)/sum(weights.values()))
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
