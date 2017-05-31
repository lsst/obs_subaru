for source, target in [
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
    ]:
    config.filterMap[source] = target
