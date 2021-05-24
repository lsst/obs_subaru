# Enable HSM shapes (unsetup meas_extensions_shapeHSM to disable)
# 'config' is a SourceMeasurementConfig.
import os.path
from lsst.utils import getPackageDir
from lsst.pex.exceptions.wrappers import NotFoundError

try:
    config.load(os.path.join(getPackageDir("meas_extensions_shapeHSM"), "config", "enable.py"))
    config.plugins["ext_shapeHSM_HsmShapeRegauss"].deblendNChild = "deblend_nChild"
    # Enable debiased moments
    config.plugins.names |= ["ext_shapeHSM_HsmPsfMomentsDebiased"]
except NotFoundError as e:
    print("Cannot enable shapeHSM (%s): disabling HSM shape measurements" % (e,))
