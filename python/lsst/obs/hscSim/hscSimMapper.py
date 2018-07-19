from lsst.obs.hsc import HscMapper
from .deprecated import deprecated


class HscSimMapper(HscMapper):

    def __init__(self, *args, **kwargs):
        deprecated("HscSimMapper", "HscMapper")
        super(HscSimMapper, self).__init__(*args, **kwargs)
