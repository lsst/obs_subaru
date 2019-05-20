config.parse.translation = {'filter': 'FILTER',
                            'ccd': 'DETECTOR',
                            'calibDate': 'CALIBDATE',
                            }
config.register.columns = {'filter': 'text',
                           'ccd': 'int',
                           'calibDate': 'text',
                           'validStart': 'text',
                           'validEnd': 'text',
                           }
config.register.detector = ['filter', 'ccd']
config.register.unique = ['filter', 'ccd', 'calibDate']
config.register.tables = ['defects',]
config.register.visit = ['calibDate']
