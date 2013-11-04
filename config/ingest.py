from lsst.obs.subaru.ingest import HscParseTask
root.parse.retarget(HscParseTask)

root.parse.translation = {'proposal': 'PROP-ID',
                          'dataType': 'DATA-TYP',
                          'field': 'OBJECT',
                          'filter': 'FILTER01',
                          'expTime': 'EXPTIME',
                          'ccd': 'DET-ID',
                          'pa': 'INST-PA',
                          'autoguider': 'T_AG',
                          'ccdTemp': 'T_CCDTV',
                          'config': 'T_CFGFIL',
                          'frameId': 'FRAMEID',
                          'expId': 'EXP-ID',
                          'dateObs': 'DATE-OBS',
                          'taiObs': 'DATE-OBS',
}
root.parse.defaults = {'ccdTemp': "0", # Added in commissioning run 3
                       }
root.parse.translators = {'visit': 'translate_visit',
                          'pointing': 'translate_pointing',
}

root.register.columns = {'field': 'text',
                         'visit': 'int',
                         'ccd': 'int',
                         'pointing': 'int',
                         'filter': 'text',
                         'proposal': 'text',
                         'dateObs': 'text',
                         'taiObs': 'text',
                         'expTime': 'double',
                         'pa': 'double',
                         'autoguider': 'int',
                         'ccdTemp': 'double',
                         'config': 'text',
                         'frameId': 'text',
                         'expId': 'text',
                         'dataType': 'text',
                         }
root.register.unique = ['visit', 'ccd',]
root.register.visit = ['visit', 'field', 'filter', 'dateObs',]
