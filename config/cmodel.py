# Enable CModel mags (unsetup meas_multifit or use $MEAS_MULTIFIT_DIR/config/disable.py to disable)
# 'root' is a SourceMeasurementConfig.
import os
try:
    root.load(os.path.join(os.environ['MEAS_MULTIFIT_DIR'], 'config', 'enable.py'))
    root.algorithms['classification.extendedness'].fluxRatio = 0.985
except KeyError, ImportError:
    print "Cannot import lsst.meas.multifit: disabling CModel measurements"
