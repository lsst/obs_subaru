from builtins import zip
from builtins import str
from builtins import range
from builtins import object
#!/usr/bin/env python
#
# LSST Data Management System
# Copyright 2008-2016 AURA/LSST.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <https://www.lsstcorp.org/LegalNotices/>.
#
from collections import OrderedDict
import numpy as np

import lsst.pex.exceptions
import lsst.afw.image as afwImage
import lsst.afw.detection as afwDet
import lsst.afw.geom as afwGeom
import lsst.afw.math as afwMath

from . import plugins

DEFAULT_PLUGINS = [
    plugins.DeblenderPlugin(plugins.fitPsfs),
    plugins.DeblenderPlugin(plugins.buildSymmetricTemplates),
    plugins.DeblenderPlugin(plugins.medianSmoothTemplates),
    plugins.DeblenderPlugin(plugins.makeTemplatesMonotonic),
    plugins.DeblenderPlugin(plugins.clipFootprintsToNonzero),
    plugins.DeblenderPlugin(plugins.reconstructTemplates, onReset=5),
    plugins.DeblenderPlugin(plugins.apportionFlux),
]

class DeblenderResult(object):
    """Collection of objects in multiple bands for a single parent footprint
    """
    
    def __init__(self, footprints, maskedImages, psfs, psffwhms, log, filters=None,
            maxNumberOfPeaks=0, avgNoise=None):
        """ Initialize a DeblededParent
        
        Parameters
        ----------
        footprint: list of `afw.detection.Footprint`s
            Parent footprint to deblend in each band.
        maskedImages: list of `afw.image.MaskedImageF`s
            Masked image containing the ``footprint`` in each band.
        psf: list of `afw.detection.Psf`s
            Psf of the ``maskedImage`` for each band.
        psffwhm: list of `float`s
            FWHM of the ``maskedImage``'s ``psf`` in each band.
        filters: list of `string`, optional
            Names of the filters. This should be the same length as the other lists
        maxNumberOfPeaks: `int`, optional
            If positive, the maximum number of peaks to deblend.
            If the total number of peaks is greater than ``maxNumberOfPeaks``,
            then only the first ``maxNumberOfPeaks`` sources are deblended.
            The default is 0, which deblends all of the peaks.
        avgNoise: `float`or list of `float`s, optional
            Average noise level in each ``maskedImage``.
            The default is ``None``, which estimates the noise from the median value of the
            variance plane of ``maskedImage`` for each filter.
        Returns
        -------
        None
        """
        # Check if this is collection of footprints in multiple bands or a single footprint
        try:
            len(footprints)
        except TypeError:
            footprints = [footprints]
        try:
            len(maskedImages)
        except TypeError:
            maskedImages = [maskedImages]
        try:
            len(psfs)
        except TypeError:
            psfs = [psfs]
        try:
            len(psffwhms)
        except TypeError:
            psffwhms = [psffwhms]
        try:
            len(avgNoise)
        except TypeError:
            if avgNoise is None:
                avgNoise = [None]*len(psfs)
            else:
                avgNoise = [avgNoise]
        # Now check that all of the parameters have the same number of entries
        if any([len(footprints)!=len(p) for p in [maskedImages, psfs, psffwhms, avgNoise]]):
            raise ValueError("To use the multi-color deblender, "
                             "'footprint', 'maskedImage', 'psf', 'psffwhm', 'avgNoise'"
                             "must have the same length, but instead have lengths: "
                             "{0}".format([len(p) for p in [footprints,
                                                            maskedImages,
                                                            psfs,
                                                            psffwhms,
                                                            avgNoise]]))

        self.log = log
        self.filterCount = len(footprints)

        self.peakCount = len(footprints[0].getPeaks())
        if maxNumberOfPeaks>0 and maxNumberOfPeaks<self.peakCount:
            self.peakCount = maxNumberOfPeaks

        if filters is None:
            filters = range(len(footprints))
        self.filters = filters

        # Create a DeblendedParent for the Footprint in every filter
        self.deblendedParents = OrderedDict([])
        for n in range(self.filterCount):
            f = self.filters[n]
            dp = DeblendedParent(f, footprints[n], maskedImages[n], psfs[n],
                                 psffwhms[n], avgNoise[n], maxNumberOfPeaks, self)
            self.deblendedParents[self.filters[n]] = dp

        # Group the peaks in each color
        self.peaks = []
        for idx in range(self.peakCount):
            peakDict = {f: dp.peaks[idx] for f,dp in self.deblendedParents.items()}
            multiPeak = MultiColorPeak(peakDict, idx, self)
            self.peaks.append(multiPeak)

    def getParentProperty(self, propertyName):
        """Get the footprint in each filter"""
        return [getattr(fp, propertyName) for dp in self.deblendedParents]

    def setTemplateSums(self, templateSums, fidx=None):
        if fidx is not None:
            self.templateSums[fidx] = templateSums
        else:
            for f, templateSum in templateSums.items():
                self.deblendedParents[f].templateSum = templateSum

class DeblendedParent(object):
    """Deblender result of a single parent footprint, in a single band
    
    Convenience class to store useful objects used by the deblender for a single band,
    such as the maskedImage, psf, etc as well as the results from the deblender.
    """
    def __init__(self, filterName, footprint, maskedImage, psf, psffwhm, avgNoise, 
                 maxNumberOfPeaks, debResult):
        """Create a DeblendedParent to store a deblender result
        
        Parameters
        ----------
        filterName: `str`
            Name of the filter used for `maskedImage`
        footprint: list of `afw.detection.Footprint`s
            Parent footprint to deblend in each band.
        maskedImages: list of `afw.image.MaskedImageF`s
            Masked image containing the ``footprint`` in each band.
        psf: list of `afw.detection.Psf`s
            Psf of the ``maskedImage`` for each band.
        psffwhm: list of `float`s
            FWHM of the ``maskedImage``'s ``psf`` in each band.
        avgNoise: `float`or list of `float`s, optional
            Average noise level in each ``maskedImage``.
            The default is ``None``, which estimates the noise from the median value of the
            variance plane of ``maskedImage`` for each filter.
        maxNumberOfPeaks: `int`, optional
            If positive, the maximum number of peaks to deblend.
            If the total number of peaks is greater than ``maxNumberOfPeaks``,
            then only the first ``maxNumberOfPeaks`` sources are deblended.
            The default is 0, which deblends all of the peaks.
        debResult: `DeblenderResult`
            The ``DeblenderResult`` that contains this ``DeblendedParent``.
        """
        self.filter = filterName
        self.fp = footprint
        self.fp.normalize()
        self.maskedImage = maskedImage
        self.psf = psf
        self.psffwhm = psffwhm
        self.img = maskedImage.getImage()
        self.imbb = self.img.getBBox()
        self.varimg = maskedImage.getVariance()
        self.mask = maskedImage.getMask()
        self.avgNoise = avgNoise
        self.updateFootprintBbox()
        self.debResult = debResult
        self.peakCount = debResult.peakCount
        self.templateSum = None
        
        # avgNoise is an estiamte of the average noise level for the image in this filter
        if avgNoise is None:
            stats = afwMath.makeStatistics(self.varimg, self.mask, afwMath.MEDIAN)
            avgNoise = np.sqrt(stats.getValue(afwMath.MEDIAN))
            debResult.log.trace('Estimated avgNoise for filter %s = %f', self.filter, avgNoise)
        self.avgNoise = avgNoise
        
        # Store all of the peak information in a single object
        self.peaks = []
        peaks = self.fp.getPeaks()
        for idx in range(self.peakCount):
            deblendedPeak = DeblendedPeak(peaks[idx], idx, self)
            self.peaks.append(deblendedPeak)

    def updateFootprintBbox(self):
        """Update the bounding box of the parent footprint
        
        If the parent Footprint is resized it will be necessary to update the bounding box of the footprint.
        """
        # Pull out the image bounds of the parent Footprint
        self.bb = self.fp.getBBox()
        if not self.imbb.contains(self.bb):
            raise ValueError(('Footprint bounding-box %s extends outside image bounding-box %s') %
                             (str(self.bb), str(self.imbb)))
        self.W, self.H = self.bb.getWidth(), self.bb.getHeight()
        self.x0, self.y0 = self.bb.getMinX(), self.bb.getMinY()
        self.x1, self.y1 = self.bb.getMaxX(), self.bb.getMaxY()

class MultiColorPeak(object):
    """Collection of single peak deblender results in multiple bands.
    
    There is one of these objects for each Peak in the footprint.
    """
    
    def __init__(self, peaks, pki, parent):
        """Create a collection for deblender results in each band.
        
        Parameters
        ----------
        peaks: `dict` of `afw.detection.PeakRecord`
            Each entry in ``peaks`` is a peak record for the same peak in a different band
        pki: int
            Index of the peak in `parent.peaks`
        parent: `DeblendedParent`
            DeblendedParent object that contains the peak as a child.
        
        Returns
        -------
        None
        
        """
        self.filters = list(peaks.keys())
        self.deblendedPeaks = peaks
        self.parent = parent
        for pki, peak in self.deblendedPeaks.items():
            peak.multiColorPeak = self

        # Fields common to the peak in all bands that will be set by the deblender
        # In the future this is likely to contain information about the probability of the peak
        # being a point source, color-information about templateFootprints, etc.
        self.pki = pki
        self.skip = False
        self.deblendedAsPsf = False

class DeblendedPeak(object):
    """Result of deblending a single Peak within a parent Footprint.

    There is one of these objects for each Peak in the Footprint.
    """

    def __init__(self, peak, pki, parent, multiColorPeak=None):
        """Initialize a new deblended peak in a single filter band
        
        Parameters
        ----------
        peak: `afw.detection.PeakRecord`
            Peak object in a single band from a peak record
        pki: `int`
            Index of the peak in `multiColorPeak.parent.peaks`
        parent: `DeblendedParent`
            Parent in the same filter that contains the peak
        multiColorPeak: `MultiColorPeak`
            Object containing the same peak in multiple bands
        
        Returns
        -------
        None
        """
        # Peak object
        self.peak = peak
        # int, peak index number
        self.pki = pki
        self.parent = parent
        self.multiColorPeak = multiColorPeak
        # union of all the ways of failing...
        self.skip = False

        self.outOfBounds = False
        self.tinyFootprint = False
        self.noValidPixels = False
        self.deblendedAsPsf = False
        self.degenerate = False

        # Field set during _fitPsf:
        self.psfFitFailed = False
        self.psfFitBadDof = False
        # (chisq, dof) for PSF fit without decenter
        self.psfFit1 = None
        # (chisq, dof) for PSF fit with decenter
        self.psfFit2 = None
        # (chisq, dof) for PSF fit after applying decenter
        self.psfFit3 = None
        # decentered PSF fit wanted to move the center too much
        self.psfFitBigDecenter = False
        # was the fit with decenter better?
        self.psfFitWithDecenter = False
        #
        self.psfFitR0 = None
        self.psfFitR1 = None
        self.psfFitStampExtent = None
        self.psfFitCenter = None
        self.psfFitBest = None
        self.psfFitParams = None
        self.psfFitFlux = None
        self.psfFitNOthers = None

        # Things only set in _fitPsf when debugging is turned on:
        self.psfFitDebugPsf0Img = None
        self.psfFitDebugPsfImg = None
        self.psfFitDebugPsfDerivImg = None
        self.psfFitDebugPsfModel = None

        self.failedSymmetricTemplate = False

        # The actual template Image and Footprint
        self.templateImage = None
        self.templateFootprint = None

        # The flux assigned to this template -- a MaskedImage
        self.fluxPortion = None

        # The stray flux assigned to this template (may be None), a HeavyFootprint
        self.strayFlux = None

        self.hasRampedTemplate = False

        self.patched = False

        # debug -- a copy of the original symmetric template
        self.origTemplate = None
        self.origFootprint = None
        # MaskedImage
        self.rampedTemplate = None
        # MaskedImage
        self.medianFilteredTemplate = None

        # when least-squares fitting templates, the template weight.
        self.templateWeight = 1.0

    def __str__(self):
        return (('deblend result: outOfBounds: %s, deblendedAsPsf: %s') %
                (self.outOfBounds, self.deblendedAsPsf))

    @property
    def psfFitChisq(self):
        chisq, dof = self.psfFitBest
        return chisq

    @property
    def psfFitDof(self):
        chisq, dof = self.psfFitBest
        return dof

    def getFluxPortion(self, strayFlux=True):
        """!
        Return a HeavyFootprint containing the flux apportioned to this peak.

        @param[in]     strayFlux   include stray flux also?
        """
        if self.templateFootprint is None or self.fluxPortion is None:
            return None
        heavy = afwDet.makeHeavyFootprint(self.templateFootprint, self.fluxPortion)
        if strayFlux:
            if self.strayFlux is not None:
                heavy.normalize()
                self.strayFlux.normalize()
                heavy = afwDet.mergeHeavyFootprints(heavy, self.strayFlux)

        return heavy

    def setStrayFlux(self, stray):
        self.strayFlux = stray

    def setFluxPortion(self, mimg):
        self.fluxPortion = mimg

    def setTemplateWeight(self, w):
        self.templateWeight = w

    def setPatched(self):
        self.patched = True

    # DEBUG
    def setOrigTemplate(self, t, tfoot):
        self.origTemplate = t.Factory(t, True)
        self.origFootprint = tfoot

    def setRampedTemplate(self, t, tfoot):
        self.hasRampedTemplate = True
        self.rampedTemplate = t.Factory(t, True)

    def setMedianFilteredTemplate(self, t, tfoot):
        self.medianFilteredTemplate = t.Factory(t, True)

    def setPsfTemplate(self, tim, tfoot):
        self.psfFootprint = afwDet.Footprint(tfoot)
        self.psfTemplate = tim.Factory(tim, True)

    def setOutOfBounds(self):
        self.outOfBounds = True
        self.skip = True

    def setTinyFootprint(self):
        self.tinyFootprint = True
        self.skip = True

    def setNoValidPixels(self):
        self.noValidPixels = True
        self.skip = True

    def setPsfFitFailed(self):
        self.psfFitFailed = True

    def setBadPsfDof(self):
        self.psfFitBadDof = True

    def setDeblendedAsPsf(self):
        self.deblendedAsPsf = True

    def setFailedSymmetricTemplate(self):
        self.failedSymmetricTemplate = True
        self.skip = True

    def setTemplate(self, image, footprint):
        self.templateImage = image
        self.templateFootprint = footprint

def deblend(footprint, maskedImage, psf, psffwhm, filters=None,
            psfChisqCut1=1.5, psfChisqCut2=1.5, psfChisqCut2b=1.5, fitPsfs=True,
            medianSmoothTemplate=True, medianFilterHalfsize=2,
            monotonicTemplate=True, weightTemplates=False,
            log=None, verbose=False, sigma1=None, maxNumberOfPeaks=0,
            assignStrayFlux=True, strayFluxToPointSources='necessary', strayFluxAssignment='r-to-peak',
            rampFluxAtEdge=False, patchEdges=False, tinyFootprintSize=2,
            getTemplateSum=False, clipStrayFluxFraction=0.001, clipFootprintToNonzero=True,
            removeDegenerateTemplates=False, maxTempDotProd=0.5
            ):
    """Deblend a parent ``Footprint`` in a ``MaskedImageF``.
    
    Deblending assumes that ``footprint`` has multiple peaks, as it will still create a
    `PerFootprint` object with a list of peaks even if there is only one peak in the list.
    It is recommended to first check that ``footprint`` has more than one peak, similar to the
    execution of `lsst.meas.deblender.deblend.SourceDeblendTask`.

    .. note::
        This is the API for the old deblender, however the function builds the plugins necessary
        to use the new deblender to perform identically to the old deblender.
        To test out newer functionality use ``newDeblend`` instead.

    Deblending involves several mandatory and optional steps:
    # Optional: If ``fitPsfs`` is True, find all peaks that are well-fit by a PSF + background model
        * Peaks that pass the cuts have their footprints modified to the PSF + background model
          and their ``deblendedAsPsf`` property set to ``True``.
        * Relevant parameters: ``psfChisqCut1``, ``psfChisqCut2``, ``psfChisqCut2b``,
          ``tinyFootprintSize``.
        * See the parameter descriptions for more.
    # Build a symmetric template for each peak not well-fit by the PSF model
        * Given ``maskedImageF``, ``footprint``, and a ``DeblendedPeak``, creates a symmetric
          template (``templateImage`` and ``templateFootprint``) around the peak
          for all peaks not flagged as ``skip`` or ``deblendedAsPsf``.
        * If ``patchEdges=True`` and if ``footprint`` touches pixels with the
          ``EDGE`` bit set, then ``footprint`` is grown to include spans whose
          symmetric mirror is outside of the image.
        * Relevant parameters: ``sigma1`` and ``patchEdges``.
    # Optional: If ``rampFluxAtEdge`` is True, adjust flux on the edges of the template footprints
        * Using the PSF, a peak ``Footprint`` with pixels on the edge of of ``footprint``
          is grown by the psffwhm*1.5 and filled in with zeros.
        * The result is a new symmetric footprint template for the peaks near the edge.
        * Relevant parameter: ``patchEdges``.
    # Optionally (``medianSmoothTemplate=True``) filter the template images
        * Apply a median smoothing filter to all of the template images.
        * Relevant parameters: ``medianFilterHalfSize``
    # Optional: If ``monotonicTemplate`` is True, make the templates monotonic.
        * The pixels in the templates are modified such that pixels
          further from the peak will have values smaller than those closer to the peak.
    # Optional: If ``clipFootprintToNonzero`` is True, clip non-zero spans in the template footprints
        * Peak ``Footprint``s are clipped to the region in the image containing non-zero values
          by dropping spans that are completely zero and moving endpoints to non-zero pixels
          (but does not split spans that have internal zeros).
    # Optional: If ``weightTemplates`` is True,  weight the templates to best fit the observed image
        * Re-weight the templates so that their linear combination
          best represents the observed ``maskedImage``
    # Optional: If ``removeDegenerateTempaltes`` is True, reconstruct shredded galaxies
        * If galaxies have substructure, such as face-on spirals, the process of identifying peaks can
          "shred" the galaxy into many pieces. The templates of shredded galaxies are typically quite
          similar because they represent the same galaxy, so we try to identify these "degenerate" peaks
          by looking at the inner product (in pixel space) of pairs of templates.
        * If they are nearly parallel, we only keep one of the peaks and reject the other.
        * If only one of the peaks is a PSF template, the other template is used,
          otherwise the one with the maximum template value is kept.
        * Relevant parameters: ``maxTempDotProduct``
    # Apportion flux to all of the peak templates
        * Divide the ``maskedImage`` flux amongst all of the templates based on the fraction of 
          flux assigned to each ``tempalteFootprint``.
        * Leftover "stray flux" is assigned to peaks based on the other parameters.
        * Relevant parameters: ``clipStrayFluxFraction``, ``strayFluxAssignment``,
          ``strayFluxToPointSources``, ``assignStrayFlux``
    
    Parameters
    ----------
    footprint: `afw.detection.Footprint`
        Parent footprint to deblend
    maskedImage: `afw.image.MaskedImageF`
        Masked image containing the ``footprint``
    psf: `afw.detection.Psf`
        Psf of the ``maskedImage``
    psffwhm: `float`
        FWHM of the ``maskedImage``'s ``psf``
    psfChisqCut*: `float`, optional
        If ``fitPsfs==True``, all of the peaks are fit to the image PSF.
        ``psfChisqCut1`` is the maximum chi-squared-per-degree-of-freedom allowed for a peak to
        be considered a PSF match without recentering.
        A fit is also made that includes terms to recenter the PSF.
        ``psfChisqCut2`` is the same as ``psfChisqCut1`` except it determines the restriction on the
        fit that includes recentering terms.
        If the peak is a match for a re-centered PSF, the PSF is repositioned at the new center and
        the peak footprint is fit again, this time to the new PSF.
        If the resulting chi-squared-per-degree-of-freedom is less than ``psfChisqCut2b`` then it
        passes the re-centering algorithm.
        If the peak passes both the re-centered and fixed position cuts, the better of the two is accepted,
        but parameters for all three psf fits are stored in the ``DeblendedPeak``.
        The default for ``psfChisqCut1``, ``psfChisqCut2``, and ``psfChisqCut2b`` is ``1.5``.
    fitPsfs: `bool`, optional
        If True then all of the peaks will be compared to the image PSF to
        distinguish stars from galaxies.
    medianSmoothTemplate: ``bool``, optional
        If ``medianSmoothTemplate==True`` it a median smoothing filter is applied to the ``maskedImage``.
        The default is ``True``.
    medianFilterHalfSize: `int`, optional
        Half the box size of the median filter, ie a ``medianFilterHalfSize`` of 50 means that
        each output pixel will be the median of  the pixels in a 101 x 101-pixel box in the input image.
        This parameter is only used when ``medianSmoothTemplate==True``, otherwise it is ignored.
        The default value is 2.
    monotonicTempalte: `bool`, optional
        If True then make the template monotonic.
        The default is True.
    weightTemplates: `bool`, optional
        If True, re-weight the templates so that their linear combination best represents
        the observed ``maskedImage``.
        The default is False.
    log: `log.Log`, optional
        LSST logger for logging purposes.
        The default is ``None`` (no logging).
    verbose: `bool`, optional
        Whether or not to show a more verbose output.
        The default is ``False``.
    sigma1: `float`, optional
        Average noise level in ``maskedImage``.
        The default is ``None``, which estimates the noise from the median value of ``maskedImage``.
    maxNumberOfPeaks: `int`, optional
        If nonzero, the maximum number of peaks to deblend.
        If the total number of peaks is greater than ``maxNumberOfPeaks``,
        then only the first ``maxNumberOfPeaks`` sources are deblended.
        The default is 0, which deblends all of the peaks.
    assignStrayFlux: `bool`, optional
        If True then flux in the parent footprint that is not covered by any of the
        template footprints is assigned to templates based on their 1/(1+r^2) distance.
        How the flux is apportioned is determined by ``strayFluxAssignment``.
        The default is True.
    strayFluxToPointSources: `string`
        Determines how stray flux is apportioned to point sources
        * ``never``: never apportion stray flux to point sources
        * ``necessary`` (default): point sources are included only if there are no extended sources nearby
        * ``always``: point sources are always included in the 1/(1+r^2) splitting
    strayFluxAssignment: `string`, optional
        Determines how stray flux is apportioned.
        * ``trim``: Trim stray flux and do not include in any footprints
        * ``r-to-peak`` (default): Stray flux is assigned based on (1/(1+r^2) from the peaks
        * ``r-to-footprint``: Stray flux is distributed to the footprints based on 1/(1+r^2) of the
          minimum distance from the stray flux to footprint
        * ``nearest-footprint``: Stray flux is assigned to the footprint with lowest L-1 (Manhattan)
          distance to the stray flux
    rampFluxAtEdge: `bool`, optional
        If True then extend footprints with excessive flux on the edges as described above.
        The default is False.
    patchEdges: `bool`, optional
        If True and if the footprint touches pixels with the ``EDGE`` bit set,
        then grow the footprint to include all symmetric templates.
        The default is ``False``.
    tinyFootprintSize: `float`, optional
        The PSF model is shrunk to the size that contains the original footprint.
        If the bbox of the clipped PSF model for a peak is smaller than ``max(tinyFootprintSize,2)``
        then ``tinyFootprint`` for the peak is set to ``True`` and the peak is not fit.
        The default is 2.
    getTemplateSum: `bool`, optional
        As part of the flux calculation, the sum of the templates is calculated.
        If ``getTemplateSum==True`` then the sum of the templates is stored in the result (a `PerFootprint`).
        The default is False.
    clipStrayFluxFraction: `float`, optional
        Minimum stray-flux portion.
        Any stray-flux portion less than ``clipStrayFluxFraction`` is clipped to zero.
        The default is 0.001.
    clipFootprintToNonzero: `bool`, optional
        If True then clip non-zero spans in the template footprints. See above for more.
        The default is True.
    removeDegenerateTemplates: `bool`, optional
        If True then we try to identify "degenerate" peaks by looking at the inner product
        (in pixel space) of pairs of templates.
        The default is False.
    maxTempDotProduct: `float`, optional
        All dot products between templates greater than ``maxTempDotProduct`` will result in one
        of the templates removed. This parameter is only used when ``removeDegenerateTempaltes==True``.
        The default is 0.5.
    
    Returns
    -------
    res: `PerFootprint`
        Deblender result that contains a list of ``DeblendedPeak``s for each peak and (optionally)
        the template sum.
    """
    avgNoise = sigma1

    debPlugins = []

    # Add activated deblender plugins
    if fitPsfs:
        debPlugins.append(plugins.DeblenderPlugin(plugins.fitPsfs, 
                                                  psfChisqCut1=psfChisqCut1,
                                                  psfChisqCut2=psfChisqCut2,
                                                  psfChisqCut2b=psfChisqCut2b,
                                                  tinyFootprintSize=tinyFootprintSize))
    debPlugins.append(plugins.DeblenderPlugin(plugins.buildSymmetricTemplates, patchEdges=patchEdges))
    if rampFluxAtEdge:
        debPlugins.append(plugins.DeblenderPlugin(plugins.rampFluxAtEdge, patchEdges=patchEdges))
    if medianSmoothTemplate:
        debPlugins.append(plugins.DeblenderPlugin(plugins.medianSmoothTemplates,
                                                  medianFilterHalfsize=medianFilterHalfsize))
    if monotonicTemplate:
        debPlugins.append(plugins.DeblenderPlugin(plugins.makeTemplatesMonotonic))
    if clipFootprintToNonzero:
        debPlugins.append(plugins.DeblenderPlugin(plugins.clipFootprintsToNonzero))
    if weightTemplates:
        debPlugins.append(plugins.DeblenderPlugin(plugins.weightTemplates))
    if removeDegenerateTemplates:
        if weightTemplates:
            onReset = len(debPlugins)-1
        else:
            onReset = len(debPlugins)
        debPlugins.append(plugins.DeblenderPlugin(plugins.reconstructTemplates,
                                                  onReset=onReset,
                                                  maxTempDotProd=maxTempDotProd))
    debPlugins.append(plugins.DeblenderPlugin(plugins.apportionFlux,
                                              clipStrayFluxFraction=clipStrayFluxFraction,
                                              assignStrayFlux=assignStrayFlux,
                                              strayFluxAssignment=strayFluxAssignment,
                                              strayFluxToPointSources=strayFluxToPointSources,
                                              getTemplateSum=getTemplateSum))

    debResult = newDeblend(debPlugins, footprint, maskedImage, psf, psffwhm, filters, log, verbose, avgNoise)

    return debResult

def newDeblend(debPlugins, footprint, maskedImage, psf, psffwhm, filters=None,
               log=None, verbose=False, avgNoise=None, maxNumberOfPeaks=0):
    """Deblend a parent ``Footprint`` in a ``MaskedImageF``.
    
    Deblending assumes that ``footprint`` has multiple peaks, as it will still create a
    `PerFootprint` object with a list of peaks even if there is only one peak in the list.
    It is recommended to first check that ``footprint`` has more than one peak, similar to the
    execution of `lsst.meas.deblender.deblend.SourceDeblendTask`.

    This version of the deblender accepts a list of plugins to execute, with the option to re-run parts
    of the deblender if templates are changed during any of the steps.

    Parameters
    ----------
    debPlugins: list of `meas.deblender.plugins.DeblenderPlugins`
        Plugins to execute (in order of execution) for the deblender.
    footprint: `afw.detection.Footprint` or list of Footprints
        Parent footprint to deblend.
    maskedImage: `afw.image.MaskedImageF` or list of MaskedImages
        Masked image containing the ``footprint``.
    psf: `afw.detection.Psf` or list of Psfs
        Psf of the ``maskedImage``.
    psffwhm: `float` or list of floats
        FWHM of the ``maskedImage``'s ``psf``.
    filters: list of `string`s, optional
        Names of the filters when ``footprint`` is a list instead of a single ``footprint``.
        This is used to index the deblender results by filter.
        The default is None, which will numerically index lists of objects.
    log: `log.Log`, optional
        LSST logger for logging purposes.
        The default is ``None`` (no logging).
    verbose: `bool`, optional
        Whether or not to show a more verbose output.
        The default is ``False``.
    avgNoise: `float`or list of `float`s, optional
        Average noise level in each ``maskedImage``.
        The default is ``None``, which estimates the noise from the median value of the
        variance plane of ``maskedImage`` for each filter.
    maxNumberOfPeaks: `int`, optional
        If nonzero, the maximum number of peaks to deblend.
        If the total number of peaks is greater than ``maxNumberOfPeaks``,
        then only the first ``maxNumberOfPeaks`` sources are deblended.

    Returns
    -------
    debResult: `DeblendedParent`
        Deblender result that contains a list of ``MultiColorPeak``s for each peak and
        information that describes the footprint in all filters.
    """
    # Import C++ routines
    import lsst.meas.deblender as deb
    butils = deb.BaselineUtilsF
    
    if log is None:
        import lsst.log as lsstLog

        component = 'meas_deblender.baseline'
        log = lsstLog.Log.getLogger(component)

        if verbose:
            log.setLevel(lsstLog.Log.TRACE)

    # get object that will hold our results
    debResult = DeblenderResult(footprint, maskedImage, psf, psffwhm, log, filters=filters,
                                maxNumberOfPeaks=maxNumberOfPeaks, avgNoise=avgNoise)

    step = 0
    while step < len(debPlugins):
        reset = debPlugins[step].run(debResult, log)
        if reset:
            step = debPlugins[step].onReset
        else:
            step+=1

    return debResult


class CachingPsf(object):
    """Cache the PSF models

    In the PSF fitting code, we request PSF models for all peaks near
    the one being fit.  This was turning out to be quite expensive in
    some cases.  Here, we cache the PSF models to bring the cost down
    closer to O(N) rather than O(N^2).
    """

    def __init__(self, psf):
        self.cache = {}
        self.psf = psf

    def computeImage(self, cx, cy):
        im = self.cache.get((cx, cy), None)
        if im is not None:
            return im
        try:
            im = self.psf.computeImage(afwGeom.Point2D(cx, cy))
        except lsst.pex.exceptions.Exception:
            im = self.psf.computeImage()
        self.cache[(cx, cy)] = im
        return im