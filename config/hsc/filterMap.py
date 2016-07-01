from lsst.pipe.tasks.setConfigFromEups import setAstrometryConfigFromEups
menu = { "ps1*": {}, # Defaults are fine
         "sdss*": {"filterMap": {"y": "z"}}, # No y-band, use z instead
         "2mass*": {"filterMap": {ff: "J" for ff in "grizy"}}, # No optical bands, use J instead
        }
setAstrometryConfigFromEups(config, menu)

for source, target in [("N921", 'z'), ("N816", 'i'), ("N515", 'g'), ("N387", 'g'), ("N1010", 'z')]:
    config.filterMap[source] = target
