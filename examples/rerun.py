from lsst.pex.config import Config, ConfigurableField
from lsst.pipe.base import CmdLineTask
from lsst.meas.algorithms import SourceDeblendTask
import lsst.afw.table as afwTable

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
        outsources.extend(sources, mapper=mapper)
        sources = outsources
        
        print len(sources), 'sources before deblending'

        self.deblend.run(calexp, sources, psf)
        print len(sources), 'sources after deblending'

        sources.setWriteHeavyFootprints(True)
        sources.writeFits('deblended.fits')

        calexp.writeFits('calexp.fits')
        
if __name__ == "__main__":
    MyTask.parseAndRun()
