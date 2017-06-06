"""Set color terms for HSC"""

from lsst.pipe.tasks.colorterms import Colorterm, ColortermDict

config.data = {
    "hsc*": ColortermDict(data={
        'g': Colorterm(primary="g", secondary="g"),
        'r': Colorterm(primary="r", secondary="r"),
        'i': Colorterm(primary="i", secondary="i"),
        'z': Colorterm(primary="z", secondary="z"),
        'y': Colorterm(primary="y", secondary="y"),
    }),
    "sdss*": ColortermDict(data={
        'g': Colorterm(primary="g", secondary="r", c0=-0.00816446, c1=-0.08366937, c2=-0.00726883),
        'r': Colorterm(primary="r", secondary="i", c0=0.00231810, c1=0.01284177, c2=-0.03068248),
        'r2': Colorterm(primary="r", secondary="i", c0=0.00074087, c1=-0.00830543, c2=-0.02848420),
        'i': Colorterm(primary="i", secondary="z", c0=0.00130204, c1=-0.16922042, c2=-0.01374245),
        'i2': Colorterm(primary="i", secondary="z", c0=0.00124676, c1=-0.20739606, c2=-0.01067212),
        'z': Colorterm(primary="z", secondary="i", c0=-0.00680620, c1=0.01353969, c2=0.01479369),
        'y': Colorterm(primary="z", secondary="i", c0=0.01739708, c1=0.35652971, c2=0.00574408),
        'N816': Colorterm(primary="i", secondary="z", c0=0.00927133, c1=-0.63558358, c2=-0.05474862),
        'N921': Colorterm(primary="z", secondary="i", c0=0.00752972, c1=0.09863530, c2=-0.05451118),
    }),
    "ps1*": ColortermDict(data={
        'g': Colorterm(primary="g", secondary="r", c0=0.00730066, c1=0.06508481, c2=-0.01510570),
        'r': Colorterm(primary="r", secondary="i", c0=0.00279757, c1=0.02093734, c2=-0.01877566),
        'r2': Colorterm(primary="r", secondary="i", c0=0.00117690, c1=0.00003996, c2=-0.01667794),
        'i': Colorterm(primary="i", secondary="z", c0=0.00166891, c1=-0.13944659, c2=-0.03034094),
        'i2': Colorterm(primary="i", secondary="z", c0=0.00180361, c1=-0.18483562, c2=-0.02675511),
        'z': Colorterm(primary="z", secondary="y", c0=-0.00907517, c1=-0.28840221, c2=-0.00316369),
        'y': Colorterm(primary="y", secondary="z", c0=-0.00156858, c1=0.14747401, c2=0.02880125),
        'N387': Colorterm(primary="g", secondary="r", c0=0.54993211, c1=1.93524093, c2=0.33520994),
        'N718': Colorterm(primary="i", secondary="r", c0=-0.03742158, c1=-0.29362076, c2=0.19006956),
        'N816': Colorterm(primary="i", secondary="z", c0=0.01191062, c1=-0.68757034, c2=-0.10781564),
        'N921': Colorterm(primary="z", secondary="y", c0=0.00142051, c1=-0.59278367, c2=-0.25059679),
        'N973': Colorterm(primary="y", secondary="z", c0=-0.00640165, c1=-0.03915281, c2=-0.24088565),
    }),
}
