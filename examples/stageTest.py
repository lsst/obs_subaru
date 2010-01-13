#! /usr/bin/env python
"""
This example illustrates not only how to create a simple Stage, but also
how to use to test it using the SimpleStageTester class.  See in-lined
comments for details.

Multiple, chained stages can be tested in one SimpleStageTester; one can
add multiple stages via SimpleStageTester.addStage().
"""
import lsst.pex.harness as pexHarness
import lsst.pex.harness.stage as harnessStage
from lsst.pex.harness.simpleStageTester import SimpleStageTester
import lsst.pex.policy as pexPolicy
from lsst.pex.logging import Log, Debug, LogRec, Prop
from lsst.pex.exceptions import LsstCppException

def main():

    # First create a tester.  For convenience, we use our special AreaStage
    # factory class (which is defined below) to configure the tester.  
    # 
    file = pexPolicy.DefaultPolicyFile("pex_harness",
                                       "examples/simpleStageTest/AreaStagePolicy.paf")
    stagePolicy = pexPolicy.Policy.createPolicy(file)
    tester = SimpleStageTester( AreaStage(stagePolicy) )

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

    print "Area =", outWorker.get("area")

# Below is our example Stage implmentation

class AreaStageSerial(harnessStage.SerialProcessing):
    """
    a simple Stage implmentation that calculates the area of a rectangle
    given the length of the sides
    """

    def setup(self):
        """configure this stage with a policy"""

        # You should create a default policy file that is installed
        # with your Stage implmenetation's package and merge it with
        # that policy that wass handed to you by the framework (when
        # this instance was constructed).  
        #
        # Here's how you do it.  Note that the default policy file can
        # be a dictionary.  Here, we indicated "examples" as the so-called
        # default policy repository for this package; however normally,
        # this is "pipeline".  
        file = pexPolicy.DefaultPolicyFile("pex_harness",   # package name
                                      "AreaStagePolicy_dict.paf", # def. policy
                                           "examples/simpleStageTest" # dir containing policies
                                           )
        defpol = pexPolicy.Policy.createPolicy(file, file.getRepositoryPath())
        if self.policy is None:
            self.policy = defpol
        else:
            self.policy.mergeDefaults(defpol)

        # now we can configure our pipeline from the policy (which should
        # now be complete).  An exception will be thrown if the merged 
        # policy is incomplete.  
        self.inputScale = self.policy.get("inputScale")
        self.outputScale = self.policy.get("outputScale")

        # It is important to use the internal logger provided by the
        # framework at construction time; this is available as self.log.
        # You can feel free to create child logs from it in the usual
        # way:
        #   self.log = Log(self.log, "AreaStage")
        #
        # Use this for additional debugging API sugar:
        self.log = Debug(self.log, "AreaStage")
        if self.outputScale != 0:
            self.log.log(Log.INFO, "Area scaling factor: %i"% self.outputScale)

        self.log.debug(3, "Running with sliceID = %s" % self.getRank())

    # preprocess() and postprocess() provide the serial processing; the
    # former gets executed only on the master node prior to process, and 
    # the latter, afterward.  We provide a pre- and postprocess() here
    # mainly as an example; our excuse is to check that the clipboard has 
    # the inputs we need.  
    
    def preprocess(self, clipboard):

        # do our work
        if clipboard is not None:
            if not clipboard.contains("width"):
                raise RuntimeError("Missing width on clipboard")
            if not clipboard.contains("height"):
                raise RuntimeError("Missing width on clipboard")

        # if you are worried about overhead of formatting a debug message,
        # you can wrap it in an if block
        if self.log.sends(Log.DEBUG):
            # this attaches properties to our message
            LogRec(self.log, Log.DEBUG) << "all input data found."           \
                             << Prop("width", clipboard.get("width"))   \
                             << Prop("height", clipboard.get("height")) \
                             << LogRec.endr


    def postprocess(self, clipboard):
        # We didn't need to provide this; this is identical to the
        # inherited implementation
        pass 


class AreaStageParallel(harnessStage.ParallelProcessing):

    def setup(self):
        """configure this stage with a policy"""

        file = pexPolicy.DefaultPolicyFile("pex_harness",   # package name
                                      "AreaStagePolicy_dict.paf", # def. policy
                                           "examples/simpleStageTest" # dir containing policies
                                           )
        defpol = pexPolicy.Policy.createPolicy(file, file.getRepositoryPath())
        if self.policy is None:
            self.policy = defpol
        else:
            self.policy.mergeDefaults(defpol)

        self.inputScale = self.policy.get("inputScale")
        self.outputScale = self.policy.get("outputScale")

        self.log = Debug(self.log, "AreaStage")
        if self.outputScale != 0:
            self.log.log(Log.INFO, "Area scaling factor: %i"% self.outputScale)

        self.log.debug(3, "Running with sliceID = %s" % self.getRank())

    def process(self, clipboard):
        # 
        if clipboard is not None:

            # do our work
            area = clipboard.get("width") * clipboard.get("height")*\
                   (10.0**self.inputScale/10.0**self.outputScale)**2

            # maybe you want to write a debug message
            self.log.debug(3, "found area of %f" % area)

            # save the results to the clipboard
            clipboard.put("area", area)

# Add this for convenience of testing with SimpleStageTester; otherwise,
# this is not required.
#
class AreaStage(harnessStage.Stage):
    serialClass = AreaStageSerial
    parallelClass = AreaStageParallel


if __name__ == "__main__":
    main()
