from lsst.pex.config import Config, ConfigurableField
from lsst.pipe.base import CmdLineTask
from lsst.meas.deblender import SourceDeblendTask
import lsst.afw.table as afwTable

'''
This script allows one to quickly load an image and run just the
deblender on it, without any of the calibration steps before that.

That is, it reads the calexp and pre-deblending sources, then runs the
deblender and writes the outputs to self-contained FITS files.
'''

class MyConfig(Config):
    deblend = ConfigurableField(target=SourceDeblendTask, doc="Deblender")

class MyTask(CmdLineTask):
    _DefaultName = "my"
    ConfigClass = MyConfig

    def _getConfigName(self):
        return None

    def __init__(self, *args, **kwargs):
        super(MyTask, self).__init__(*args, **kwargs)
        #self.makeSubtask("deblend", schema=self.schema)

    def run(self, dataRef):
        calexp = dataRef.get("calexp")
        psf = calexp.getPsf()
        sources = dataRef.get("src")

        mapper = afwTable.SchemaMapper(sources.getSchema())
        # map all the existing fields
        mapper.addMinimalSchema(sources.getSchema(), True)
        schema = mapper.getOutputSchema()

        # It would be better for the schema-populating code to not be in
        # the SourceDeblendTask constructor!
        self.makeSubtask("deblend", schema=schema)

        outsources = afwTable.SourceCatalog(schema)
        outsources.reserve(2 * len(sources))
        outsources.extend(sources, mapper=mapper)
        sources = outsources
        print len(sources), 'sources before deblending'

        self.deblend.run(calexp, sources, psf)
        print len(sources), 'sources after deblending'

        fn = 'deblended.fits'
        print 'Writing sources...'
        sources.writeFits(fn)
        print 'Wrote sources to', fn

        fn = 'calexp.fits'
        calexp.writeFits(fn)
        print 'Wrote calexp to', fn

        fn = 'psf.fits'
        psf.writeFits(fn)
        print 'Wrote PSF to', fn

if __name__ == "__main__":
    MyTask.parseAndRun()
