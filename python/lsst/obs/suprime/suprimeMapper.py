import os
import re

import lsst.daf.persistence as dafPersist
import lsst.afw.image as afwImage

class SuprimeMapper(dafPersist.Mapper):
	def __init__(self, policy=None, root=".", registry=None, calibRoot=None):
		dafPersist.Mapper.__init__(self, policy="SuprimeMapper.paf", module="obs_suprime",
								   policyDir="policy", root=root, registry=registry, calibRoot=calibRoot)
		
		self.keys = ["field", "visit", "ccd", "amp", "filter", "mystery", "taiObs" ]
		self.keys += ["filter", "expTime"]
		
		self.filterMap = {
			"W-S-I+"  : "i",
			"W-S-ZR"  : "z2",
			"W-S-Z+"  : "z"
			}
		
		self.filterIdMap = { 'u': 0, 'g': 1, 'r': 2, 'i': 3, 'z': 4, 'y': 5, 'z2': 5}
		
	def _extractAmpId(self, dataId):
		return (self._extractDetectorName(dataId), 0, 0)

	def _extractDetectorName(self, dataId):
		miyazakiNames = ["Nausicaa", "Kiki", "Fio", "Sophie", "Sheeta",
						 "Satsuki", "Chihiro", "Clarisse", "Ponyo", "San"]
		#return "Suprime %(ccd)d" % dataId
		ccdTmp = int("%(ccd)d" % dataId)
		return miyazakiNames[ccdTmp]
	
	def _transformId(self, dataId):
		return dataId.copy()

	def standardize_raw(self, mapping, item, dataId):
		dataId = self._transformId(dataId)
		exposure = afwImage.makeExposure(
			afwImage.makeMaskedImage(item.getImage()))
		md = item.getMetadata()
		exposure.setMetadata(md)
		exposure.setWcs(afwImage.makeWcs(md))
		wcsMetadata = exposure.getWcs().getFitsMetadata()
		for kw in wcsMetadata.paramNames():
			md.remove(kw)
		return self._standardize(mapping, exposure, dataId)
