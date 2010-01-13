#! /usr/bin/env python
"""
Multiple, chained stages can be tested in one SimpleStageTester; one can
add multiple stages via SimpleStageTester.addStage().
"""
import lsst.pex.harness as pexHarness
import lsst.pex.harness.stage as harnessStage
from lsst.pex.harness.simpleStageTester import SimpleStageTester
import lsst.pex.policy as pexPolicy
from lsst.pex.logging import Log, Debug, LogRec, Prop
from lsst.pex.exceptions import LsstCppException

from lsst.meas.deblender import deblenderStage

def main():
    # First create a tester.  For convenience, we use our special AreaStage
    # factory class (which is defined below) to configure the tester.  
    # 
    pfile = pexPolicy.DefaultPolicyFile("meas_deblender",
										"examples/DeblenderPolicy.paf")
    stagePolicy = pexPolicy.Policy.createPolicy(pfile)
    tester = SimpleStageTester( DeblenderStage(stagePolicy) )

    # set the verbosity of the logger.  If the level is at least 5, you
    # will see debugging messages from the SimpleStageTester wrapper.
    tester.setDebugVerbosity(5)

    # if you want to see all log message properties (including the DATE)
    # uncomment this line:
    # tester.showAllLogProperties(True)

    # create a simple dictionary with the data expected to be on the
    # stage's input clipboard.  If this includes images, you will need to 
    # read in and create the image objects yourself.
    clipboard = dict( width=1.0, height=2.0 )

    # you can either test the stage as part of a Master slice (which runs
    # its preprocess() and postprocess() functions)...
    outMaster = tester.runMaster(clipboard)

    # ...or you can test it as part of a Worker.  Note that in the current
    # implementation, the output clipboard is the same instance as the input
    # clipboard.  
    clipboard = dict( width=1.0, height=2.0 )
    outWorker = tester.runWorker(clipboard)

    print 'Got', outWorker


# Add this for convenience of testing with SimpleStageTester; otherwise,
# this is not required.
#
class DeblenderStage(harnessStage.Stage):
    serialClass = deblenderStage.DeblenderStageSerial
    parallelClass = deblenderStage.DeblenderStageParallel

if __name__ == "__main__":
    main()
