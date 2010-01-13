import lsst.pex.harness.stage as harnessStage
import lsst.pex.policy as pexPolicy
from lsst.pex.logging import Log, Debug, LogRec, Prop


class DeblenderStageSerial(harnessStage.SerialProcessing):
	'''
	'''

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
        file = pexPolicy.DefaultPolicyFile("meas_deblender",   # package name
										   "DeblenderStagePolicy_dict.paf", # def. policy
                                           "pipeline" # ? dir containing policies
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
        self.log = Debug(self.log, "DeblenderStage")
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


class DeblenderStageParallel(harnessStage.ParallelProcessing):

    def setup(self):
        """configure this stage with a policy"""

		# GAH I hate boilerplate
        pfile = pexPolicy.DefaultPolicyFile("meas_deblender",   # package name
											"DeblenderStagePolicy_dict.paf", # def. policy
											"pipeline" # ? dir containing policies
											)
        defpol = pexPolicy.Policy.createPolicy(pfile, pfile.getRepositoryPath())
        if self.policy is None:
            self.policy = defpol
        else:
            self.policy.mergeDefaults(defpol)

        self.inputScale = self.policy.get("inputScale")
        self.outputScale = self.policy.get("outputScale")

        self.log = Debug(self.log, "DeblenderStage")
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
