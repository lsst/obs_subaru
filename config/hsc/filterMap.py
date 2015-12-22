config.refObjLoader.filterMap = {
    'B': 'g',
    'V': 'r',
    'R': 'r',
    'I': 'i',
    'y': 'z',
}

from lsst.pipe.tasks.setConfigFromEups import setAstrometryConfigFromEups
menu = { "ps1*": {}, # Defaults are fine
         "sdss*": {"refObjLoader.filterMap": {"y": "z"}}, # No y-band, use z instead
         "2mass*": {"refObjLoader.filterMap": {ff: "J" for ff in "grizy"}}, # No optical bands, use J instead
        }
setAstrometryConfigFromEups(config, menu)
