from lsst.pipe.base import PostprocessTask
from lsst.pex.config import ConfigurableField, Config, Field

from .qa import QaTask

class SubaruPostprocessConfig(PostprocessTask.ConfigClass):
    doQa = Field(dtype = bool, doc = "Whether to run postprocessing QA")
    qa = ConfigurableField(target = QaTask, doc = "Postprocessing QA analysis")
    doWriteUnpackedMatches = pexConfig.Field(
        dtype=bool, default=True,
        doc="Write the denormalized match table as well as the normalized match table"
    )
    

class SubaruPostprocessTask(PostprocessTask):
    
    def __init__(self, **kwargs):
        super(SubaruPostprocessTask, self).__init__(**kwargs)
        self.makeSubtask("qa")

    def run(self, dataRef, results):
        """
        Run additional code after ProcessCcdTask.
        
        @param dataRef    Butler data reference passed to ProcessCcdTask,run
        @param results    Results Struct produced by ProcessCcdTask.run
        """
        if self.config.doQa:
            self.qa.run(dataRef, results.exposure, results.sources)
        if self.config.doWriteUnpackedMatches:
            self.writeUnpackedMatches(dataRef, results.calibrate.matches, results.calibrate.matchMeta)

    def writeUnpackedMatches(self, dataRef, matches, matchMeta):

        # Now write unpacked matches
        refSchema = matchlist[0].first.getSchema()
        srcSchema = matchlist[0].second.getSchema()

        mergedSchema = afwTable.Schema()
        def merge(schema, key, merged, name):
            field = schema.find(key).field
            typeStr = field.getTypeString()
            fieldDoc = field.getDoc()
            fieldUnits = field.getUnits()
            mergedSchema.addField(name + '.' + key, type=typeStr, doc=fieldDoc, units=fieldUnits)

        for keyName in refSchema.getNames():
            merge(refSchema, keyName, mergedSchema, "ref")

        for keyName in srcSchema.getNames():
            merge(srcSchema, keyName, mergedSchema, "src")

        mergedCatalog = afwTable.BaseCatalog(mergedSchema)

        refKeys = []
        for keyName in refSchema.getNames():
            refKeys.append((refSchema.find(keyName).key, mergedSchema.find('ref.' + keyName).key))
        srcKeys = []
        for keyName in srcSchema.getNames():
            srcKeys.append((srcSchema.find(keyName).key, mergedSchema.find('src.' + keyName).key))

        # obtaining reference catalog name
        catalogName = os.path.basename(os.getenv("ASTROMETRY_NET_DATA_DIR").rstrip('/'))
        matchMeta.add('REFCAT', catalogName)
        mergedCatalog.getTable().setMetadata(matchMeta)

        dataRef.put(mergedCatalog, "matchedList")
