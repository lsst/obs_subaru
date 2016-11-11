"""Set color terms for Suprime-Cam with MITLL detectors"""

from lsst.pipe.tasks.colorterms import Colorterm, ColortermDict

# From the last page of http://www.naoj.org/staff/nakata/suprime/illustration/colorterm_report_ver3.pdf
# Transformation for griz band between SDSS and SC (estimated with GS83 SEDs)
config.data = {
    "sdss-*": ColortermDict(data={
        'g': Colorterm(primary="g", secondary="r", c0=-0.00569, c1=-0.0427),
        'r': Colorterm(primary="r", secondary="g", c0=0.00261, c1=0.0304),
        'i': Colorterm(primary="i", secondary="r", c0=0.00586, c1=0.0827, c2=-0.0118),
        'z': Colorterm(primary="z", secondary="i", c0=0.000329, c1=0.0608, c2=0.0219),
    }),
}
