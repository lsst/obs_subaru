from __future__ import print_function
# Enable CModel mags (unsetup meas_modelfit to disable)
# 'config' is a SourceMeasurementConfig.
import os
try:
    import lsst.meas.modelfit
    config.measurement.plugins.names |= ["modelfit_DoubleShapeletPsfApprox", "modelfit_CModel"]
    config.measurement.slots.modelFlux = 'modelfit_CModel'
    config.catalogCalculation.plugins['base_ClassificationExtendedness'].fluxRatio = 0.985

    # DM-9795 fixes a bug in the relative weight of likelihood vs. prior, but
    # before it's fully vetted we explicitly scale the likelihood below to
    # retain approximately the same behavior.
    # The median per-pixel variance in HSC Wide coadds is 0.004, and the
    # weights are the inverse of the sqrt of the per-pixel variance, so
    # multiplying the likelihood by the sqrt of the variance should
    # approximately cancel out the weights on average (and unit weight was the
    # old buggy behavior).
    avgPixelSigma = 0.004**0.5
    config.measurement.plugins["modelfit_CModel"].initial.weightsMultiplier = avgPixelSigma
    config.measurement.plugins["modelfit_CModel"].exp.weightsMultiplier = avgPixelSigma
    config.measurement.plugins["modelfit_CModel"].dev.weightsMultiplier = avgPixelSigma

except (KeyError, ImportError):
    print("Cannot import lsst.meas.modelfit: disabling CModel measurements")
