import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom
from lsst.pipe.base import Struct
from lsst.pipe.tasks.selectImages import BaseSelectImagesTask

class ButlerSelectImagesTask(BaseSelectImagesTask):
    def runDataRef(self, dataRef, coordList, makeDataRefList=True, inputRefList=[]):

        # XXX Dirty hack
#        searchId = {'field': "ACTJ0022M0036", 'filter': "W-S-R+"}
        searchId = {'field': "M31", }

        butler = dataRef.getButler()
        inputRefList = [r for r in butler.subset("raw", "Ccd", searchId)]

        dataRefList = []
        exposureInfoList = []
        for inputRef in inputRefList:
            md = inputRef.get("calexp_md")
            try:
                wcs = afwImage.makeWcs(md)
                nx, ny = md.get("NAXIS1"), md.get("NAXIS2")
                bbox = afwGeom.Box2D(afwGeom.Point2D(0,0), afwGeom.Extent2D(nx, ny))
                bounds = afwGeom.Box2D()
                for coord in coordList:
                    pix = wcs.skyToPixel(coord) # May throw() if wcslib barfs
                    bounds.include(pix)
                if bbox.overlaps(bounds):
                    self.log.info("Selecting calexp %s" % inputRef.dataId)
                    dataRefList.append(inputRef)
                    corners = [wcs.pixelToSky(x,y) for x in (0, nx) for y in (0, ny)]
                    exposureInfoList.append(inputRef.dataId, corners)
                else:
                    self.log.info("De-selecting calexp %s" % inputRef.dataId)
            except:
                self.log.info("Error in testing calexp %s: deselecting" % inputRef.dataId)

        self.log.info("Selected %d inputs" % len(dataRefList))
        return Struct(
            dataRefList = dataRefList if makeDataRefList else None,
            exposureInfoList = exposureInfoList,
        )

