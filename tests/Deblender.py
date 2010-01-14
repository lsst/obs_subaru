from lsst.meas.deblender import deblender

#import lsst.meas.algorithms as measAlg
from lsst.meas.utils import sourceDetection
#import lsst.meas.utils.sourceDetection as sourceDetection

#   eups declare --current meas_utils svn -r ~/lsst/meas-utils/

import unittest

class TestDeblender(unittest.TestCase):

	def setUp(self):
		pass

	def testTwoStars(self):
		#self.assertEqual(self.seq, range(10))
		pass

if __name__ == '__main__':
	unittest.main()

