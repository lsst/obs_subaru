from lsst.pipe.tasks.colorterms import Colorterm

# From the last page of http://www.naoj.org/staff/nakata/suprime/illustration/colorterm_report_ver3.pdf
# Transformation for griz band between SDSS and SC (estimated with GS83 SEDs)
colortermsData = dict(
    # Old (MIT-Lincoln Labs) chips
    MIT = dict(
        g = Colorterm("g", "r", -0.00569, -0.0427),
        r = Colorterm("r", "g", 0.00261, 0.0304),
        i = Colorterm("i", "r", 0.00586, 0.0827, -0.0118),
        z = Colorterm("z", "i", 0.000329, 0.0608, 0.0219),
        y = Colorterm("z", "i", 0.000329, 0.0608, 0.0219), # Same as Suprime-Cam z for now
        ),
    # New (Hamamatsu) chips
    Hamamatsu = dict(
        g = Colorterm("g", "r", -0.00928, -0.0824),
        #r = Colorterm("r", "g",  0.00235,  0.0283),
        r = Colorterm("r", "i", -0.00282, -0.0498, -0.0149),
        #i = Colorterm("i", "r", 0.00582, 0.0807, -0.0129),
        i = Colorterm("i", "z", 0.00186, -0.140, -0.0196),
        z = Colorterm("z", "i", -4.03e-4, 0.0967, 0.0210),
        B = Colorterm("g", "r",  0.02461907,  0.20098328, 0.00858468),
        V = Colorterm("g", "r", -0.03117934, -0.63134136, 0.05056544),
        R = Colorterm("r", "i", -0.01179613, -0.25403307, 0.00696479),
        I = Colorterm("i", "r",  0.01078282,  0.26727768, 0.00747123),
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
