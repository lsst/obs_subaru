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
        # Don't really care about schema because the sources we read already have that,
        # but the SourceDeblendTask constructor requires it.
        #self.schema = kwargs.pop("schema", None)
        #self.makeSubtask("deblend", schema=self.schema)

    def run(self, dataRef):
        calexp = dataRef.get("calexp")
        psf = calexp.getPsf()
        sources = dataRef.get("src")

        #print 'rerun: schema is', self.schema
        #if self.schema is None:
        #    #self.schema = afwTable.SourceTable.makeMinimalSchema()


        # inschema = sources.getSchema()
        # 
        # mapper = afwTable.SchemaMapper(inschema)
        # print 'Mapper:', mapper
        # print 'inschema:', dir(inschema)
        # inames = []
        # for name in inschema.getNames():
        #     inames.append(name)
        #     sitem = inschema.find(name)
        #     print 'sitem', name, 'is', sitem
        #     print '  key', sitem.key
        #     print '  field', sitem.field
        #     mapper.addMapping(sitem.key)
        # 
        # # copy-on-write
        # outschema = inschema
        # self.makeSubtask("deblend", schema=outschema)
        # #print 'After makeSubtask, schema:'
        # #print self.schema
        # #print 'source schema:'
        # #print sources.getSchema()        
        # 
        # for name in outschema.getNames():
        #     print 'Output schema name:', name
        #     if name in inames:
        #         continue
        #     mapper.addOutputField(outschema.find(name).field)
        #     
        # print 'Mapper input schema:'
        # print mapper.getInputSchema()
        # 
        # print 'Mapper output schema:'
        # print mapper.getOutputSchema()
        # 
        # osources = afwTable.SourceCatalog(mapper.getOutputSchema())
        # osources.extend(sources, mapper=mapper)
        # 
        # sources = osources



        mapper = afwTable.SchemaMapper(sources.getSchema())
        # map all the existing fields
        mapper.addMinimalSchema(sources.getSchema(), True)
        schema = mapper.getOutputSchema()

        self.makeSubtask("deblend", schema=schema)

        output = afwTable.SourceCatalog(schema)
        output.extend(sources, mapper=mapper)

        sources = output
        
        print len(sources), 'sources before deblending'

        self.deblend.run(calexp, sources, psf)
        print len(sources), 'sources after deblending'

        sources.setWriteHeavyFootprints(True)
        sources.writeFits('deblended.fits')

        calexp.writeFits('calexp.fits')
        
if __name__ == "__main__":
    MyTask.parseAndRun()
