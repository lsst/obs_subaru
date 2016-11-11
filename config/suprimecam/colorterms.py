"""Set color terms for Suprime-Cam with Hamamatsu detectors"""

from lsst.pipe.tasks.colorterms import Colorterm, ColortermDict

config.data = {
    "sdss-*": ColortermDict(data={
        'g': Colorterm(primary="g", secondary="r", c0=-0.00928, c1=-0.0824),
        'r': Colorterm(primary="r", secondary="i", c0=-0.00282, c1=-0.0498, c2=-0.0149),
        'i': Colorterm(primary="i", secondary="z", c0=0.00186, c1=-0.140, c2=-0.0196),
        'z': Colorterm(primary="z", secondary="i", c0=-4.03e-4, c1=0.0967, c2=0.0210),
        'B': Colorterm(primary="g", secondary="r", c0=0.02461907, c1=0.20098328, c2=0.00858468),
        'V': Colorterm(primary="g", secondary="r", c0=-0.03117934, c1=-0.63134136, c2=0.05056544),
        'R': Colorterm(primary="r", secondary="i", c0=-0.01179613, c1=-0.25403307, c2=0.00696479),
        'I': Colorterm(primary="i", secondary="r", c0=0.01078282, c1=0.26727768, c2=0.00747123),
    }),
}
