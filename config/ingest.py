config.parse.translation = {'proposal': 'PROP-ID',
                            'dataType': 'DATA-TYP',
                            'expTime': 'EXPTIME',
                            'ccd': 'DET-ID',
                            'pa': 'INST-PA',
                            'ccdTemp': 'T_CCDTV',
                            'frameId': 'FRAMEID',
                            'expId': 'EXP-ID',
                            'dateObs': 'DATE-OBS',
                            'taiObs': 'DATE-OBS',
                            }
config.parse.defaults = {'ccdTemp': "0", # Added in HSC commissioning run 3
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
                           'ccdTemp': 'double',
                           'frameId': 'text',
                           'expId': 'text',
                           'dataType': 'text',
                           }
config.register.unique = ['visit', 'ccd', ]
config.register.visit = ['visit', 'field', 'filter', 'dateObs', 'taiObs']
