"""Set color terms for HSC"""

from lsst.pipe.tasks.colorterms import Colorterm, ColortermDict

config.data = {
    "hsc*": ColortermDict(data={
        # Names used by Exposure.getFilter() in Gen2.
        'g': Colorterm(primary="g", secondary="g"),
        'r': Colorterm(primary="r", secondary="r"),
        'i': Colorterm(primary="i", secondary="i"),
        'z': Colorterm(primary="z", secondary="z"),
        'y': Colorterm(primary="y", secondary="y"),
        # Names used by data IDs in both Gen2 and Gen3, and
        # Exposure.getFilter() in Gen3 (data is the same).
        'HSC-G': Colorterm(primary="g", secondary="g"),
        'HSC-R': Colorterm(primary="r", secondary="r"),
        'HSC-I': Colorterm(primary="i", secondary="i"),
        'HSC-Z': Colorterm(primary="z", secondary="z"),
        'HSC-Y': Colorterm(primary="y", secondary="y"),
    }),
    "sdss*": ColortermDict(data={
        # Names used by Exposure.getFilter() in Gen2.
        'g': Colorterm(primary="g", secondary="r", c0=-0.00816446, c1=-0.08366937, c2=-0.00726883),
        'r': Colorterm(primary="r", secondary="i", c0=0.00231810, c1=0.01284177, c2=-0.03068248),
        'r2': Colorterm(primary="r", secondary="i", c0=0.00074087, c1=-0.00830543, c2=-0.02848420),
        'i': Colorterm(primary="i", secondary="z", c0=0.00130204, c1=-0.16922042, c2=-0.01374245),
        'i2': Colorterm(primary="i", secondary="z", c0=0.00124676, c1=-0.20739606, c2=-0.01067212),
        'z': Colorterm(primary="z", secondary="i", c0=-0.00680620, c1=0.01353969, c2=0.01479369),
        'y': Colorterm(primary="z", secondary="i", c0=0.01739708, c1=0.35652971, c2=0.00574408),
        'N816': Colorterm(primary="i", secondary="z", c0=0.00927133, c1=-0.63558358, c2=-0.05474862),
        'N921': Colorterm(primary="z", secondary="i", c0=0.00752972, c1=0.09863530, c2=-0.05451118),
        # Names used by data IDs in both Gen2 and Gen3, and
        # Exposure.getFilter() in Gen3 (data is the same).
        'HSC-G': Colorterm(primary="g", secondary="r", c0=-0.00816446, c1=-0.08366937, c2=-0.00726883),
        'HSC-R': Colorterm(primary="r", secondary="i", c0=0.00231810, c1=0.01284177, c2=-0.03068248),
        'HSC-R2': Colorterm(primary="r", secondary="i", c0=0.00074087, c1=-0.00830543, c2=-0.02848420),
        'HSC-I': Colorterm(primary="i", secondary="z", c0=0.00130204, c1=-0.16922042, c2=-0.01374245),
        'HSC-I2': Colorterm(primary="i", secondary="z", c0=0.00124676, c1=-0.20739606, c2=-0.01067212),
        'HSC-Z': Colorterm(primary="z", secondary="i", c0=-0.00680620, c1=0.01353969, c2=0.01479369),
        'HSC-Y': Colorterm(primary="z", secondary="i", c0=0.01739708, c1=0.35652971, c2=0.00574408),
        'NB0816': Colorterm(primary="i", secondary="z", c0=0.00927133, c1=-0.63558358, c2=-0.05474862),
        'NB0921': Colorterm(primary="z", secondary="i", c0=0.00752972, c1=0.09863530, c2=-0.05451118),
    }),
    "ps1*": ColortermDict(data={
        # Names used by Exposure.getFilter() in Gen2.
        'g': Colorterm(primary="g", secondary="r", c0=0.005905, c1=0.063651, c2=-0.000716),
        'r': Colorterm(primary="r", secondary="i", c0=-0.000294, c1=-0.005458, c2=-0.009451),
        'r2': Colorterm(primary="r", secondary="i", c0=0.000118, c1=-0.002790, c2=-0.014363),
        'i': Colorterm(primary="i", secondary="z", c0=0.000979, c1=-0.154608, c2=-0.010429),
        'i2': Colorterm(primary="i", secondary="z", c0=0.001653, c1=-0.206313, c2=-0.016085),
        'z': Colorterm(primary="z", secondary="y", c0=-0.005585, c1=-0.220704, c2=-0.298211),
        'y': Colorterm(primary="y", secondary="z", c0=-0.001952, c1=0.199570, c2=0.216821),
        'I945': Colorterm(primary="y", secondary="z", c0=0.005275, c1=-0.194285, c2=-0.125424),
        'N387': Colorterm(primary="g", secondary="r", c0=0.427879, c1=1.869068, c2=0.540580),
        'N468': Colorterm(primary="g", secondary="r", c0=-0.042240, c1=0.121756, c2=0.027599),
        'N515': Colorterm(primary="g", secondary="r", c0=-0.021913, c1=-0.253159, c2=0.151553),
        'N527': Colorterm(primary="g", secondary="r", c0=-0.020641, c1=-0.366167, c2=0.038497),
        'N656': Colorterm(primary="r", secondary="i", c0=0.035658, c1=-0.512071, c2=0.042824),
        'N718': Colorterm(primary="i", secondary="r", c0=-0.016294, c1=-0.233139, c2=0.252505),
        'N816': Colorterm(primary="i", secondary="z", c0=0.013806, c1=-0.717681, c2=0.049289),
        'N921': Colorterm(primary="z", secondary="y", c0=0.002039, c1=-0.477412, c2=-0.492151),
        'N973': Colorterm(primary="y", secondary="z", c0=-0.007775, c1=-0.050972, c2=-0.197278),
        'N1010': Colorterm(primary="y", secondary="z", c0=0.003607, c1=0.865366, c2=1.271817),
        # Names used by data IDs in both Gen2 and Gen3, and
        # Exposure.getFilter() in Gen3 (data is the same).
        'HSC-G': Colorterm(primary="g", secondary="r", c0=0.005905, c1=0.063651, c2=-0.000716),
        'HSC-R': Colorterm(primary="r", secondary="i", c0=-0.000294, c1=-0.005458, c2=-0.009451),
        'HSC-R2': Colorterm(primary="r", secondary="i", c0=0.000118, c1=-0.002790, c2=-0.014363),
        'HSC-I': Colorterm(primary="i", secondary="z", c0=0.000979, c1=-0.154608, c2=-0.010429),
        'HSC-I2': Colorterm(primary="i", secondary="z", c0=0.001653, c1=-0.206313, c2=-0.016085),
        'HSC-Z': Colorterm(primary="z", secondary="y", c0=-0.005585, c1=-0.220704, c2=-0.298211),
        'HSC-Y': Colorterm(primary="y", secondary="z", c0=-0.001952, c1=0.199570, c2=0.216821),
        'IB0945': Colorterm(primary="y", secondary="z", c0=0.005275, c1=-0.194285, c2=-0.125424),
        'NB0387': Colorterm(primary="g", secondary="r", c0=0.427879, c1=1.869068, c2=0.540580),
        'NB0468': Colorterm(primary="g", secondary="r", c0=-0.042240, c1=0.121756, c2=0.027599),
        'NB0515': Colorterm(primary="g", secondary="r", c0=-0.021913, c1=-0.253159, c2=0.151553),
        'NB0527': Colorterm(primary="g", secondary="r", c0=-0.020641, c1=-0.366167, c2=0.038497),
        'NB0656': Colorterm(primary="r", secondary="i", c0=0.035658, c1=-0.512071, c2=0.042824),
        'NB0718': Colorterm(primary="i", secondary="r", c0=-0.016294, c1=-0.233139, c2=0.252505),
        'NB0816': Colorterm(primary="i", secondary="z", c0=0.013806, c1=-0.717681, c2=0.049289),
        'NB0921': Colorterm(primary="z", secondary="y", c0=0.002039, c1=-0.477412, c2=-0.492151),
        'NB0973': Colorterm(primary="y", secondary="z", c0=-0.007775, c1=-0.050972, c2=-0.197278),
        'NB1010': Colorterm(primary="y", secondary="z", c0=0.003607, c1=0.865366, c2=1.271817),
    }),
}
