
from lsst.obs.base import MakeRawVisitInfoViaObsInfo
from lsst.obs.metadata import SuprimeCamTranslator

__all__ = ["MakeSuprimecamRawVisitInfo"]


class MakeSuprimecamRawVisitInfo(MakeRawVisitInfoViaObsInfo):
    """Make a VisitInfo from the FITS header of a Subaru SuprimeCam image
    """

    # Force HSC Translator.  Not required but being explicit does no harm.
    metadataTranslator = SuprimeCamTranslator
