#!/usr/bin/env python

import os

import unittest
import lsst.utils.tests as utilsTests

from lsst.pex.policy import Policy
import lsst.daf.persistence as dafPersist
from lsst.obs.suprime import SuprimeMapper

class GetFlatTestCase(unittest.TestCase):
	"""Testing butler flat image retrieval"""
	
	def setUp(self):
		self.bf = dafPersist.ButlerFactory(
			mapper=SuprimeMapper(root="./tests/science", calibRoot="./tests/calib"))
		self.butler = self.bf.create()
		
	def tearDown(self):
		del self.butler
		del self.bf

	def testFlat(self):
		"""Test retrieval of flat image"""
		raw = self.butler.get("flat", field="CFHQS", visit=101543, ccd=2)
		
		print "width: ",			  raw.getWidth()
		print "height: ",			  raw.getHeight()
		print "detector(amp) name: ", raw.getDetector().getId().getName()
		print "detector(ccd) name: ", raw.getDetector().getParent().getId().getName()
		
		self.assertEqual(raw.getWidth(), 2048)	# trimmed
		self.assertEqual(raw.getHeight(), 4177) # trimmed
		self.assertEqual(raw.getFilter().getFilterProperty().getName(), "i")
		self.assertEqual(raw.getDetector().getId().getName(), "ID0")
		self.assertEqual(raw.getDetector().getParent().getId().getName(), "Fio")
		
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
	"""Returns a suite containing all the test cases in this module."""

	utilsTests.init()

	suites = []
	suites += unittest.makeSuite(GetFlatTestCase)
	suites += unittest.makeSuite(utilsTests.MemoryTestCase)
	return unittest.TestSuite(suites)

def run(shouldExit = False):
	"""Run the tests"""
	utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
	run(True)
