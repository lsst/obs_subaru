# -*- python -*-
#
# Setup our environment
#
import glob, os.path, sys, traceback
import lsst.SConsUtils as scons

env = scons.makeEnv(
    "hscdistest",
    r"$HeadURL: FILL IN PATH /distEst/trunk/SConstruct $",
    [
        ["python", "Python.h"],
    ],
)

#env.libs["hscdistest"] += env.getlibs("")
#env.libs["hscdistest"] += env.getlibs("gsl gslcblas afw daf_base")
#env.libs["hscdistest"] += env.getlibs("pex_exceptions afw boost utils daf_base daf_data daf_persistence pex_logging pex_policy security")
#

for d in (
    "lib",
    "python/hsc/meas/match"
):
    SConscript(os.path.join(d, "SConscript"))

scons.CleanTree(r"*~ core *.so *.os *.o *.pyc config.log")

env.Declare()
env.Help("""
LSST Application Framework packages
""")
