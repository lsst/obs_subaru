# Enable CModel mags (unsetup meas_multifit or use $MEAS_MULTIFIT_DIR/config/disable.py to disable)
# 'root' is a SourceMeasurementConfig.
import os
try:
    import lsst.meas.modelfit
    root.algorithms.names |= ["modelfit_ShapeletPsfApprox", "modelfit_CModel"]
    root.algorithms['base_ClassificationExtendedness'].fluxRatio = 0.985
except KeyError, ImportError:
    print "Cannot import lsst.meas.modelfit: disabling CModel measurements"
