for source, target in [
        # Names used by Exposure.getFilter() in Gen2.
        # Wide bands
        ("r2", "r"),
        ("i2", "i"),
        # Narrow bands
        ('N387', 'g'),
        ('N468', 'g'),
        ('N515', 'g'),
        ('N527', 'g'),
        ('N656', 'r'),
        ('N718', 'i'),
        ('N816', 'i'),
        ('N921', 'z'),
        ('N926', 'z'),
        ('N973', 'y'),
        ('N1010', 'y'),
        # Intermediate bands
        ('I945', 'z'),

        # Names used by data IDs in both Gen2 and Gen3, and
        # Exposure.getFilter() in Gen3 (mappings are the same).
        # Wide bands
        ("HSC-G", "g"),
        ("HSC-R", "r"),
        ("HSC-R2", "r"),
        ("HSC-I", "i"),
        ("HSC-I2", "i"),
        ("HSC-Z", "z"),
        ("HSC-Y", "y"),
        # Narrow bands
        ('NB0387', 'g'),
        ('NB0391', 'g'),
        ('NB0395', 'g'),
        ('NB0430', 'g'),
        ('NB0468', 'g'),
        ('NB0497', 'g'),
        ('NB0506', 'g'),
        ('NB0515', 'g'),
        ('NB0527', 'g'),
        ('NB0656', 'r'),
        ('NB0718', 'i'),
        ('NB0816', 'i'),
        ('NB0871', 'i'),
        ('NB0921', 'z'),
        ('NB0926', 'z'),
        ('NB0973', 'y'),
        ('NB1010', 'y'),
        # Intermediate bands
        ('IB0945', 'z'),
        # Extended bands
        ('EB-GRI', 'r'),
    ]:
    config.filterMap[source] = target
