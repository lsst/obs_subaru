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
        g = Colorterm("g", "r", -0.00956847, -0.10352234, -0.00502665),
        r = Colorterm("r", "i",  0.00124709, -0.00313555, -0.02849828),
        i = Colorterm("i", "z",  0.00133279, -0.17328299, -0.01424202),
        z = Colorterm("z", "i", -0.00683705,  0.01458871,  0.01447283),
        y = Colorterm("z", "i",  0.01698942,  0.34691808,  0.00271266),
        ),

    # SDSS -> HSC
    HSC_from_SDSS = dict(
        g = Colorterm("g", "r", -0.00816446, -0.08366937, -0.00726883),
        r = Colorterm("r", "i",  0.00231810,  0.01284177, -0.03068248),
        i = Colorterm("i", "z",  0.00130204, -0.16922042, -0.01374245),
        z = Colorterm("z", "i", -0.00680620,  0.01353969,  0.01479369),
        y = Colorterm("z", "i",  0.01739708,  0.35652971,  0.00574408),
        ),

    # PS1 -> HSC
    HSC_from_PS1 = dict(
        g = Colorterm("g", "r",  0.00730066,  0.06508481, -0.01510570),
        r = Colorterm("r", "i",  0.00279757,  0.02093734, -0.01877566),
        i = Colorterm("i", "z",  0.00166891, -0.13944659, -0.03034094),
        z = Colorterm("z", "y", -0.00907517, -0.28840221, -0.00316369),
        y = Colorterm("y", "z", -0.00156858,  0.14747401,  0.02880125),
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
