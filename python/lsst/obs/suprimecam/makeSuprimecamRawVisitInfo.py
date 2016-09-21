from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from ..hsc.makeHscRawVisitInfo import MakeHscRawVisitInfo


class MakeSuprimecamRawVisitInfo(MakeHscRawVisitInfo):
    """Suprimecam has the same FITS metadata as HSC, at least for VisitInfo fields
    """
    pass
