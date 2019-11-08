from lsst.obs.subaru.ingest import HscParseTask
config.parse.retarget(HscParseTask)

config.parse.translation = {'proposal': 'PROP-ID',
                            'dataType': 'DATA-TYP',
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
config.parse.defaults = {'ccdTemp': "0", # Added in commissioning run 3
                         'autoguider': "0",
                         }
config.parse.translators = {'field': 'translate_field',
                            'visit': 'translate_visit',
                            'pointing': 'translate_pointing',
                            'filter': 'translate_filter',
                            }

config.register.columns = {'field': 'text',
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
config.register.unique = ['visit', 'ccd', ]
config.register.visit = ['visit', 'field', 'filter', 'dateObs', 'taiObs']
