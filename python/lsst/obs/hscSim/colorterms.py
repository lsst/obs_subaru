from lsst.meas.photocal.colorterms import Colorterm

colortermsData = dict(
    # Dummy null color terms for development phase
    Dummy = dict(
        g = Colorterm("g", "r", 0.0, 0.0),
        r = Colorterm("r", "i", 0.0, 0.0),
        i = Colorterm("i", "z", 0.0, 0.0),
        z = Colorterm("z", "i", 0.0, 0.0),
        ),
    # Hamamatsu chips
    Hamamatsu = dict(
        g = Colorterm("g", "r", -0.00928, -0.0824),
        #r = Colorterm("r", "g",  0.00235,  0.0283),
        r = Colorterm("r", "i", -0.00282, -0.0498, -0.0149),
        #i = Colorterm("i", "r", 0.00582, 0.0807, -0.0129),
        i = Colorterm("i", "z", 0.00186, -0.140, -0.0196),
        z = Colorterm("z", "i", -4.03e-4, 0.0967, 0.0210),
        ),
    )

if __name__ == "__main__":
    class Source(object):
        def __init__(self, **kwargs):
            self._fluxes = kwargs

        def X__init__(self, g, r):
            self._fluxes = dict(g = g, r = r)

        def get(self, band):
            return self._fluxes[band]

    sources = dict(g=0.0, r=0.0), dict(g=0.0, r=-1.0)

    print [Colorterm.transform("g", s, colorterms=colortermsData["Hamamatsu"]) for s in sources]
    Colorterm.setColorterms(colortermsData)
    Colorterm.setActiveDevice("Hamamatsu")
    print [Colorterm.transform("g", s) for s in sources]
