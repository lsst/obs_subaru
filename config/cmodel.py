# Enable CModel mags (unsetup meas_multifit or use $MEAS_MULTIFIT_DIR/config/disable.py to disable)
# 'config' is a SourceMeasurementConfig.
import os
try:
    import lsst.meas.modelfit
    config.algorithms.names |= ["modelfit_ShapeletPsfApprox", "modelfit_CModel"]
    config.algorithms['base_ClassificationExtendedness'].fluxRatio = 0.985
except KeyError, ImportError:
    print "Cannot import lsst.meas.modelfit: disabling CModel measurements"
