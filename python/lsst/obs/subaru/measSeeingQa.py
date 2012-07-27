#!/usr/bin/env python

"""
TODO:
 This module is to use the same algorithms to select stars & measure seeing
  as in the SC-RCM running for Suprime-Cam observation (publised in
  Furusawa+2011,PASJ,63,S581).
  Implemented for onsite QA output
"""

import os, os.path
import math
import lsst.pex.config as pexConfig
import lsst.pex.logging as pexLog
import lsst.afw.table as afwTable

from lsst.pipe.base import Task, Struct

import numpy

import matplotlib.pyplot as plt
plt.switch_backend('Agg')
from matplotlib import patches as patches


class MeasureSeeingConfig(pexConfig.Config):
    fracSrcIni = pexConfig.Field(
        dtype = float, 
        doc = 'What fraction of sources from the brightest is to be included for initial guess of seeing to avoid cosmic rays which dominate faint magnitudes', 
        default = 0.15, # is good for SC with 5-sigma detection
        )
    fwhmMin  = pexConfig.Field(
        dtype = float,
        doc = 'Minimum fwhm allowed in estimation of seeing (pix)',
        default = 1.5,
        )
    fwhmMax  = pexConfig.Field(
        dtype = float,
        doc = 'Maxmum fwhm allowed in estimation of seeing (pix)',
        default = 12.0,
        )
    nbinMagHist = pexConfig.Field(
        dtype = int,
        doc = 'Number of bins for number counting as a fn of instrumnetal mag',
        default = 80,
        )
    magMinHist = pexConfig.Field(
        dtype = float,
        doc = 'Brightest mag for number counting as a fn of instrumnetal mag',
        default = -20.0,
        )
    magMaxHist = pexConfig.Field(
        dtype = float,
        doc = 'Faintest mag for number counting as a fn of instrumnetal mag',
        default = 0.0,
        )
    nSampleRoughFwhm = pexConfig.Field(
        dtype = int,
        doc = 'Number of smallest objects which are used to determine rough-interim seeing',
        default = 30,
        )
    fwhmMarginFinal = pexConfig.Field(
        dtype = float,
        doc = 'How many pixels around the peak are used for final seeing estimation as mode',
        default = 1.5,
        ) 
    fwhmBinSize = pexConfig.Field(
        dtype = float,
        doc = "Bin size of FWHM histogram",
        default = 0.2,
        )
    doPlots = pexConfig.Field(
        dtype = bool,
        doc = "Make plots?",
        default = True
        )
    gridSize = pexConfig.Field(
        dtype = float,
        doc = "Size of grid (pixels)",
        default = 1024
        )


class MeasureSeeingTask(Task):
    """
    This Task measures seeing (mode) and ellipticity (median) representative of the input frame
    by using the input source set.

    Requirements:
       This function has optimized for reasonably deep detection. Please use 2 sigma detection for
       the input sourceset. 

       exposure is used for metadata filling
       
    """

    ConfigClass = MeasureSeeingConfig

    def __init__(self, *args, **kwargs):
        super(MeasureSeeingTask, self).__init__(*args, **kwargs)
        self.debugFlag = False

    def run(self, dataRef, sources, exposure):
        data = self.convert(sources)
        magLim = self.magLimit(dataRef, data)
        if magLim is None:
            return
        fwhmRough = self.getFwhmRough(dataRef, data, magLim)
        data = self.getFwhmRobust(dataRef, data, fwhmRough)

        self.setMetadata(exposure)

        if self.config.doPlots:
            self.plotSeeingMap(dataRef, data, exposure)
            self.plotEllipseMap(dataRef, data, exposure)
            self.plotEllipticityMap(dataRef, data, exposure)
            self.plotFwhmGrid(dataRef, data, exposure)
            self.plotEllipseGrid(dataRef, data, exposure)
            self.plotEllipticityGrid(dataRef, data, exposure)

    def setMetadata(self, exposure):
        """Put processing results in header of exposure"""
        metadata = exposure.getMetadata()
        metadata.set('SEEING_MODE', self.metadata.get("fwhmRobust"))
        metadata.set('ELL_MED', self.metadata.get("ellRobust"))
        metadata.set('ELL_PA_MED', self.metadata.get("ellPaRobust"))


    def convert(self, sources):
        """Convert input data to numpy arrays for processing"""

        # -- Filtering sources with rough min-max fwhm
        xListAll = []
        yListAll = []
        magListAll = []
        fwhmListAll = []
        ellListAll = []
        ellPaListAll = []
        IxxListAll = []
        IyyListAll = []
        IxyListAll = []
        AEllListAll = []
        BEllListAll = []        
        objFlagListAll = []
        indicesSourcesFwhmRange = [] # indices of sources in acceptable fwhm range 

        catalogSchema = sources.table.getSchema()
        nameSaturationFlag = 'flags.pixel.saturated.any'
        nameSaturationCenterFlag = 'flags.pixel.saturated.center'
        keySaturationFlag = catalogSchema.find(nameSaturationFlag).key
        keySaturationCenterFlag = catalogSchema.find(nameSaturationCenterFlag).key    

        iGoodId = 0
        for iseq, source in enumerate(sources):
            xc = source.getX()   
            yc = source.getY()   
            Ixx = source.getIxx() # luminosity-weighted 2nd moment of pixels
            Iyy = source.getIyy() 
            Ixy = source.getIxy()
            sigma = math.sqrt(0.5*(Ixx+Iyy))
            fwhm = 2.*math.sqrt(2.*math.log(2.)) * sigma # (pix) assuming Gaussian

            saturationFlag = source.get(keySaturationFlag)
            saturationCenterFlag = source.get(keySaturationCenterFlag)        
            isSaturated = (saturationFlag | saturationCenterFlag)        
            if self.debugFlag:
                print ('objFlag SatAny %x  SatCenter %x  isSaturated %x' %
                       (saturationFlag, saturationCenterFlag, isSaturated))

            fluxAper = source.getApFlux()
            fluxErrAper =  source.getApFluxErr() 
            fluxGauss = source.getModelFlux()
            fluxErrGauss =  source.getModelFluxErr()
            fluxPsf = source.getPsfFlux()
            fluxErrPsf = source.getPsfFluxErr()

            # which flux do you use:
            fluxForSeeing = fluxAper 

            # validity check
            if math.isnan(fwhm) or math.isnan(Ixx) or math.isnan(fluxForSeeing) or fwhm <= 0 or \
                    fluxForSeeing <= 0 or (Ixx == 0 and Iyy == 0 and Ixy == 0) or isSaturated:
                continue

            mag = -2.5*numpy.log10(fluxForSeeing)
            magListAll.append(mag)
            fwhmListAll.append(fwhm)
            xListAll.append(xc)
            yListAll.append(yc)
            IxxListAll.append(Ixx)
            IyyListAll.append(Iyy)
            IxyListAll.append(Ixy)

            if True: # definition by SExtractor
                Val1 = 0.5*(Ixx+Iyy)
                Ixx_Iyy = Ixx-Iyy
                Val2 = 0.25*Ixx_Iyy*Ixx_Iyy + Ixy*Ixy
                if Val2 >= 0 and (Val1-math.sqrt(Val2)) > 0:
                    aa = math.sqrt( Val1 + math.sqrt(Val2) )
                    bb = math.sqrt( Val1 - math.sqrt(Val2) )
                    ell =  1. - bb/aa
                    if math.fabs(Ixx_Iyy) > 1.0e-10:
                        ellPa = 0.5 * math.degrees(math.atan(2*Ixy / math.fabs(Ixx_Iyy)))
                    else:
                        ellPa = 0.0
                else:
                    ell = None
                    ellPa = None
                    aa = None
                    bb = None
            else: # definition by Kaiser
                # e=sqrt(e1^2+e2^2) where e1=(Ixx-Iyy)/(Ixx+Iyy), e2=2Ixy/(Ixx+Iy)
                # SExtractor's B/A=sqrt((1-e)/(1+e)), ell=1-B/A
                e1 = (Ixx-Iyy)/(Ixx+Iyy)
                if e1 > 0: 
                    e2 = 2.0*Ixy/(Ixx+Iyy)
                    ell = math.sqrt(e1*e1 + e2*e2)
                    fabs_Ixx_Iyy = math.fabs(Ixx-Iyy)
                    if fabs_Ixx_Iyy > 1.0e-10:
                        ellPa = 0.5 * math.degrees(math.atan(2*Ixy / fabs_Ixx_Iyy))
                    else:
                        ellPa = 0.0
                else:
                    ell = None
                    ellPa = None

            if ellPa is not None:
                ellPa = 90. - ellPa ## definition of PA to be confirmed

            if self.debugFlag:
                print ('*** %d : Ixx: %f Iyy: %f Ixy: %f fwhm: %f flux: %f isSatur: %x' % 
                       (iseq, Ixx, Iyy, Ixy, fwhm, fluxAper, isSaturated))

            ellListAll.append( ell )
            ellPaListAll.append( ellPa )
            AEllListAll.append( aa ) # sigma in long axis
            BEllListAll.append( bb ) # sigma in short axis
            #objFlagListAll.append(objFlag)

            if fwhm > self.config.fwhmMin and fwhm < self.config.fwhmMax:
                indicesSourcesFwhmRange.append(iGoodId)

            # this sample is added to the good sample
            iGoodId += 1

        return Struct(magListAll = numpy.array(magListAll),
                      fwhmListAll = numpy.array(fwhmListAll),
                      ellListAll = numpy.array(ellListAll),
                      ellPaListAll = numpy.array(ellPaListAll),
                      AEllListAll = numpy.array(AEllListAll),
                      BEllListAll = numpy.array(BEllListAll),
                      xListAll = numpy.array(xListAll),
                      yListAll = numpy.array(yListAll),
                      IxxListAll = numpy.array(IxxListAll),
                      IyyListAll = numpy.array(IyyListAll),
                      IxyListAll = numpy.array(IxyListAll),
                      indicesSourcesFwhmRange = indicesSourcesFwhmRange,
                      )

    def magLimit(self, dataRef, data):
        """Derive the normalized cumulative magnitude histogram"""
        #indicesSourcesFwhmRange = numpy.array(indicesSourcesFwhmRange)
        magListFwhmRange = data.magListAll[data.indicesSourcesFwhmRange]
        # n.b. magHist[1] is sorted so index=0 is brightest
        magHist = numpy.histogram(magListFwhmRange, range=(self.config.magMinHist, self.config.magMaxHist),
                                  bins=self.config.nbinMagHist)

        sumAll = magHist[0].sum()
        #print '*** sumAll: ', sumAll
        self.log.info("QaSeeing: total number of objects in the first dataset (sumAll): %d" % sumAll)

        magCumHist = list(magHist)
        magCumHist[0] = (magCumHist[0].cumsum())
        #print '------ mag cum hist ------'
        #print magCumHist
        magCumHist[0] = magCumHist[0] / float(sumAll)
        #print '------ normalized mag cum hist ------'
        #print magCumHist

        # -- Estimating mag limit based no the cumlative mag histogram
        magLim = None
        for i, cumFraction in enumerate(magCumHist[0]):
            if cumFraction >= self.config.fracSrcIni:
                magLim = magCumHist[1][i] # magLim is the mag which exceeds the cumulative n(m) of 0.15
                break
        if not magLim:
            self.log.log(self.log.WARN, "Error: cumulative magnitude histogram does not exceed 0.15.")
            return None

        #print '*** magLim: ', magLim
        self.log.info("Mag limit auto-determined: %5.2f or %f (ADU)" %
                 (magLim, numpy.power(10, -0.4*magLim)))
        self.metadata.add("magLim", numpy.power(10, -0.4*magLim))

        if self.debugFlag:
            self.log.logdebug("QaSeeing: magHist: %s" % magHist)
            self.log.logdebug("QaSeeing: cummurative magHist: %s" % magCumHist)

        if self.config.doPlots:
            fig = plt.figure()
            pltMagHist = fig.add_subplot(2,1,1)
            pltMagHist.hist(magListFwhmRange, bins=magHist[1], orientation='vertical')
            pltMagHist.set_title('histogram of magnitudes')
            #        pltMagHist.set_xlabel('magnitude instrumental')
            pltMagHist.set_ylabel('number of samples')
            pltMagHist.legend()

            pltCumHist = fig.add_subplot(2,1,2)
            # histtype=bar,barstacked,step,stepfilled
            pltCumHist.hist(magListFwhmRange, bins=magCumHist[1], normed=True, cumulative=True,
                            orientation='vertical', histtype='step')
            xx = [magLim, magLim]; yy = [0, 1]
            pltCumHist.plot(xx, yy, linestyle='dashed', label='mag limit') # solid,dashed,dashdot,dotted
            pltCumHist.set_title('cumulative histogram of magnitudes')
            pltCumHist.set_xlabel('magnitude instrumental')
            pltCumHist.set_ylabel('Nsample scaled to unity')
            pltCumHist.legend()

            fname = getFilename(dataRef, "plotMagHist")
            plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait', papertype=None,
                        format='png', transparent=False, bbox_inches=None, pad_inches=0.1)
            del fig
            del pltMagHist
            del pltCumHist

        return magLim

    def getFwhmRough(self, dataRef, data, magLim):
        """Estimating roughly-estimated FWHM for sources with mag < magLim"""
        indicesSourcesPsfLike = [i for i in data.indicesSourcesFwhmRange if data.magListAll[i] < magLim ]

        magListPsfLike = data.magListAll[indicesSourcesPsfLike]
        fwhmListPsfLike = data.fwhmListAll[indicesSourcesPsfLike]
        ellListPsfLike = data.ellListAll[indicesSourcesPsfLike]
        ellPaListPsfLike = data.ellPaListAll[indicesSourcesPsfLike]
        AEllListPsfLike = data.AEllListAll[indicesSourcesPsfLike]
        BEllListPsfLike = data.BEllListAll[indicesSourcesPsfLike]
        xListPsfLike = data.xListAll[indicesSourcesPsfLike]
        yListPsfLike = data.yListAll[indicesSourcesPsfLike]
        IxxListPsfLike = data.IxxListAll[indicesSourcesPsfLike]
        IyyListPsfLike = data.IyyListAll[indicesSourcesPsfLike]
        IxyListPsfLike = data.IxyListAll[indicesSourcesPsfLike]

        data.magListPsfLike = magListPsfLike
        data.fwhmListPsfLike = fwhmListPsfLike

        self.log.logdebug("nSampleRoughFwhm: %d" % self.config.nSampleRoughFwhm)

        fwhmListForRoughFwhm = numpy.sort(fwhmListPsfLike)[:self.config.nSampleRoughFwhm]
        fwhmRough = numpy.median(fwhmListForRoughFwhm)

        if self.debugFlag:
            print '*** fwhmListPsfLike:', fwhmListPsfLike
            print '*** fwhmListForRoughFwhm:', fwhmListForRoughFwhm
            print '*** fwhmRough:', fwhmRough
        self.log.info("fwhmRough: %f" % fwhmRough)
        self.metadata.add("fwhmRough", fwhmRough)

        if self.config.doPlots:
            fig = plt.figure()
            pltMagFwhm = fig.add_subplot(1,1,1)
            pltMagFwhm.set_xlim(-20,-5)
            pltMagFwhm.set_ylim(0,20)
            pltMagFwhm.plot(magListPsfLike, fwhmListPsfLike, 'bx', label='coarse PSF-like sample')
            xx = [-20,-5]; yy = [fwhmRough, fwhmRough]
            pltMagFwhm.plot(xx, yy, linestyle='dashed', label='tentative FWHM')
            pltMagFwhm.set_title('FWHM vs magnitudes')
            pltMagFwhm.set_xlabel('magnitude instrumental')
            pltMagFwhm.set_ylabel('FWHM (pix)')

            pltMagFwhm.legend()
            fname = getFilename(dataRef, "plotSeeingRough")
            plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait',
                        papertype=None, format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

            del fig
            del pltMagFwhm
        
        return fwhmRough


    def getFwhmRobust(self, dataRef, data, fwhmRough):
        """Good Estimation of Final PSF"""
        fwhmMin = fwhmRough - self.config.fwhmMarginFinal
        fwhmMax = fwhmRough + self.config.fwhmMarginFinal

        indicesSourcesPsfLikeRobust = [ i for i in data.indicesSourcesFwhmRange if 
                                        data.fwhmListAll[i] > fwhmMin and data.fwhmListAll[i] < fwhmMax
                                        ] 
        
        if len(indicesSourcesPsfLikeRobust) < 1:
            self.log.warn("No sources selected in robust seeing estimation")
            return None

        magListPsfLikeRobust = data.magListAll[indicesSourcesPsfLikeRobust]
        fwhmListPsfLikeRobust = data.fwhmListAll[indicesSourcesPsfLikeRobust]
        ellListPsfLikeRobust = data.ellListAll[indicesSourcesPsfLikeRobust]
        ellPaListPsfLikeRobust = data.ellPaListAll[indicesSourcesPsfLikeRobust]
        AEllListPsfLikeRobust = data.AEllListAll[indicesSourcesPsfLikeRobust]
        BEllListPsfLikeRobust = data.BEllListAll[indicesSourcesPsfLikeRobust]
        xListPsfLikeRobust = data.xListAll[indicesSourcesPsfLikeRobust]
        yListPsfLikeRobust = data.yListAll[indicesSourcesPsfLikeRobust]
        IxxListPsfLikeRobust = data.IxxListAll[indicesSourcesPsfLikeRobust]
        IyyListPsfLikeRobust = data.IyyListAll[indicesSourcesPsfLikeRobust]
        IxyListPsfLikeRobust = data.IxyListAll[indicesSourcesPsfLikeRobust]
        
        nbin = (fwhmMax-fwhmMin) / self.config.fwhmBinSize
        # finding the mode
        histFwhm = numpy.histogram(fwhmListPsfLikeRobust, bins=nbin)
        minval=0
        for i, num in enumerate(histFwhm[0]):
            if num > minval:
                icand = i
                minval = num

        fwhmRobust = histFwhm[1][icand]
        ellRobust = numpy.median(ellListPsfLikeRobust)
        ellPaRobust = numpy.median(ellPaListPsfLikeRobust) 

        self.log.info("Robust quantities: %f %f %f" % (fwhmRobust, ellRobust, ellPaRobust))
        self.metadata.add("fwhmRobust", fwhmRobust)
        self.metadata.add("ellRobust", ellRobust)
        self.metadata.add("ellPaRobust", ellPaRobust)

        if self.config.doPlots:
            fig = plt.figure()
            pltMagFwhm = fig.add_subplot(1,2,1)
            pltMagFwhm.set_xlim(-20,-5)
            pltMagFwhm.set_ylim(0,20)
            pltMagFwhm.plot(data.magListAll, data.fwhmListAll, '+', label='all sample')
            pltMagFwhm.plot(data.magListPsfLike, data.fwhmListPsfLike, 'bx', label='coarse PSF-like sample')
            pltMagFwhm.plot(magListPsfLikeRobust, fwhmListPsfLikeRobust, 'mo',
                            label='best-effort PSF-like sample')        
            xx = [-20,-5]; yy = [fwhmRobust, fwhmRobust]
            pltMagFwhm.plot(xx, yy, linestyle='dashed', label='resultant FWHM')
            pltMagFwhm.set_title('FWHM vs magnitudes')
            pltMagFwhm.set_xlabel('magnitude instrumental')
            pltMagFwhm.set_ylabel('FWHM (pix)')
            pltMagFwhm.legend()

            pltHistFwhm = fig.add_subplot(1,2,2)
            pltHistFwhm.hist(fwhmListPsfLikeRobust, bins=nbin, orientation='horizontal', histtype='bar') 
            pltHistFwhm.set_title('histogram of FWHM')
            pltHistFwhm.set_ylabel('FWHM of best-effort PSF-like sources')
            pltHistFwhm.set_xlabel('number of sources')
            pltHistFwhm.legend()

            fname = getFilename(dataRef, "plotSeeingRobust")
            plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait',
                        papertype=None, format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

            del fig
            del pltHistFwhm

        return Struct(
            magListPsfLikeRobust = magListPsfLikeRobust,
            fwhmListPsfLikeRobust = fwhmListPsfLikeRobust, 
            ellListPsfLikeRobust = ellListPsfLikeRobust,
            ellPaListPsfLikeRobust = ellPaListPsfLikeRobust,
            AEllListPsfLikeRobust = AEllListPsfLikeRobust,
            BEllListPsfLikeRobust = BEllListPsfLikeRobust,
            xListPsfLikeRobust = xListPsfLikeRobust,
            yListPsfLikeRobust = yListPsfLikeRobust,
            IxxListPsfLikeRobust = IxxListPsfLikeRobust,
            IyyListPsfLikeRobust = IyyListPsfLikeRobust,
            IxyListPsfLikeRobust = IxyListPsfLikeRobust,
            fwhmRobust = fwhmRobust,
            ellRobust = ellRobust,
            ellPaRobust = ellPaRobust,
            )

    def plotSeeingMap(self, dataRef, data, exposure):
        """fwhm-map plots"""

        xSize, ySize = exposure.getWidth(), exposure.getHeight()
        facSize = 10.0 / max(xSize,ySize)  # targeting 10 inch in size
        wFig = xSize * facSize * 1.3
        hFig = ySize * facSize
        fig = plt.figure(figsize=(wFig,hFig))
        pltFwhmMap = plt.axes([0.2, 0.1, 0.7, 0.8]) # left,bottom,width,height
        pltFwhmMap.set_xlim(0, xSize)
        pltFwhmMap.set_ylim(0, ySize)
        pointSize = math.pi*(10*data.fwhmListPsfLikeRobust/2)**2. # 10pix=2arcsec fwhm = 50 point radius
        pltFwhmMap.scatter(data.xListPsfLikeRobust, data.yListPsfLikeRobust, s=pointSize, marker='o',
                           color=None, facecolor=(1,1,1,0), label='PSF sample')
        #pltFwhmMap.legend()

        # reference sample point
        fwhmPix = numpy.array([5.0]) # pixel in fwhm
        pointSize = math.pi*(10*fwhmPix/2)**2.
        pltFwhmMap.scatter([0.1*xSize], [0.9*ySize], s=pointSize, marker='o', color='magenta',
                           facecolor=(1,1,1,0), label='PSF sample')        
        plt.text(0.1 * xSize, 0.9 * ySize, 'fwhm=%4.1f pix' % fwhmPix, ha='center', va='top')

        pltFwhmMap.set_title('FWHM of PSF sources')
        pltFwhmMap.set_xlabel('X (pix)')
        pltFwhmMap.set_ylabel('Y (pix)')

        fname = getFilename(dataRef, "plotSeeingMap")
        plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait', papertype=None,
                    format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

        del fig
        del pltFwhmMap

    def plotEllipseMap(self, dataRef, data, exposure):
        """psfellipse-map plots"""
        xSize, ySize = exposure.getWidth(), exposure.getHeight()

        facSize = 10.0 / max(xSize,ySize)  # targeting 10 inch in size
        wFig = xSize * facSize * 1.2
        hFig = ySize * facSize
        fig = plt.figure(figsize=(wFig,hFig))
        pltEllipseMap = plt.axes([0.2, 0.1, 0.7, 0.8]) # left,bottom,width,height
        
        pltEllipseMap.set_xlim(0, xSize)
        pltEllipseMap.set_ylim(0, ySize)
        scaleFactor = 100.  # 10pix~2arcsec (width) -> 1000pix
        for xEllipse, yEllipse, aa, bb, ellPa in zip(data.xListPsfLikeRobust, data.yListPsfLikeRobust,
                                                     data.AEllListPsfLikeRobust, data.BEllListPsfLikeRobust,
                                                     data.ellPaListPsfLikeRobust):
            if all([aa, bb, ellPa]):
                ell = patches.Ellipse((xEllipse, yEllipse), 2.*aa*scaleFactor, 2.*bb*scaleFactor,
                                      angle=ellPa, linewidth=2., fill=False, zorder=2)
            pltEllipseMap.add_patch(ell)

        # reference sample point
        fwhmPix = aa = bb = 2.5 # pixel in A, B (in half width)
        ell = patches.Ellipse((0.1*xSize, 0.9*ySize), 2.*aa*scaleFactor, 2.*bb*scaleFactor, angle=0.,
                              linewidth=4., color='magenta', fill=False, zorder=2)
        pltEllipseMap.add_patch(ell)
        plt.text(0.1 * xSize, 0.9 * ySize, 'fwhm=%4.1f pix' % fwhmPix, ha='center', va='top')

        pltEllipseMap.set_title('Size and Ellongation of PSF sources')
        pltEllipseMap.set_xlabel('X (pix)')
        pltEllipseMap.set_ylabel('Y (pix)')

        fname = getFilename(dataRef, "plotEllipseMap")
        plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait', papertype=None,
                    format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

        del fig
        del pltEllipseMap

    def plotEllipticityMap(self, dataRef, data, exposure):
        """ellipticity-map plots"""

        xSize, ySize = exposure.getWidth(), exposure.getHeight()
        facSize = 10.0 / max(xSize,ySize)  # targeting 10 inch in size
        wFig = xSize * facSize * 1.3
        hFig = ySize * facSize
        fig = plt.figure(figsize=(wFig,hFig))
        
        pltEllMap = plt.axes([0.2, 0.1, 0.7, 0.8]) # left,bottom,width,height
        pltEllMap.set_xlim(0, xSize)
        pltEllMap.set_ylim(0, ySize)

        ellX = numpy.array([ell*numpy.cos(numpy.radians(ellPa)) for ell, ellPa in 
                            zip(data.ellListPsfLikeRobust, data.ellPaListPsfLikeRobust) if all([ell, ellPa])])
        ellY = numpy.array([ell*numpy.sin(numpy.radians(ellPa)) for ell, ellPa in
                            zip(data.ellListPsfLikeRobust, data.ellPaListPsfLikeRobust) if all([ell, ellPa])])
        
        Q = pltEllMap.quiver(
            data.xListPsfLikeRobust, data.yListPsfLikeRobust,
            #data.ellListPsfLikeRobust*numpy.cos(ellPaRadianListPsfLikeRobust),
            #data.ellListPsfLikeRobust*numpy.sin(ellPaRadianListPsfLikeRobust),
            ellX, ellY,
            units = 'x', # scale is based on multiplication of 'x (pixel)'
            angles = 'uv',
            #angles = data.ellPaRobust,
            scale = 0.0005,   # (ell/pix)
            # 1pix corresponds to this value of ellipticity --> 1000pix = ellipticity 0.02 for full CCD size.
            # and scaled by using (GridSize(in shorter axis) / CcdSize)
            scale_units = 'x',
            headwidth=0,
            headlength=0,
            headaxislength=0,
            label='PSF sample'
            )

        plt.quiverkey(Q, 0.05, 1.05, 0.05, 'e=0.05', labelpos='W')

        pltEllMap.set_title('Ellipticity of PSF sources')
        pltEllMap.set_xlabel('X (pix)')
        pltEllMap.set_ylabel('Y (pix)')
        pltEllMap.legend()

        fname = getFilename(dataRef, "plotEllipticityMap")
        plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait', papertype=None,
                    format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

        del fig
        del pltEllMap




    def plotFwhmGrid(self, dataRef, data, exposure):
        """fwhm-in-grids plots"""
        xSize, ySize = exposure.getWidth(), exposure.getHeight()
        facSize = 10.0 / max(xSize,ySize)  # targeting 10 inch in size
        wFig = xSize * facSize * 1.3
        hFig = ySize * facSize
        fig = plt.figure(figsize=(wFig,hFig))
        pltFwhm = plt.axes([0.2, 0.1, 0.7, 0.8]) # left,bottom,width,height

        pltFwhm.set_xlim(0, xSize)
        pltFwhm.set_ylim(0, ySize)

        xGridSize = self.config.gridSize
        yGridSize = self.config.gridSize
        
        # making grids
        nx = int(numpy.floor(xSize/xGridSize))
        ny = int(numpy.floor(ySize/yGridSize))

        xGrids = numpy.array([ (i+0.5)*xGridSize for i in range(nx) ])
        yGrids = numpy.array([ (i+0.5)*yGridSize for i in range(ny) ])
        xMeshGrid, yMeshGrid = numpy.meshgrid(xGrids, yGrids)
        xGridList = numpy.reshape(xMeshGrid, (-1,))
        yGridList = numpy.reshape(yMeshGrid, (-1,))    

        # walking through all grid meshes
        fwhmList = numpy.array([])
        for xGridCenter, yGridCenter in zip(xGridList, yGridList):
            xGridMin = xGridCenter - xGridSize/2.
            xGridMax = xGridCenter + xGridSize/2.
            yGridMin = yGridCenter - yGridSize/2.
            yGridMax = yGridCenter + yGridSize/2.

            fwhmsInGrid = numpy.array([])
            for xFwhm, yFwhm, fwhm in zip(data.xListPsfLikeRobust, data.yListPsfLikeRobust,
                                          data.fwhmListPsfLikeRobust):
                if xGridMin <= xFwhm and xFwhm < xGridMax and yGridMin <= yFwhm and yFwhm < yGridMax:
                    if fwhm is not None:
                        fwhmsInGrid = numpy.append(fwhmsInGrid, fwhm)
            # taking median to represent the value in a grid mesh
            fwhmList = numpy.append(fwhmList, numpy.median(fwhmsInGrid))

        # 10pix=2arcsec(fwhm)=500 point(radius) (to be ~0.6*min(xGridSize, yGridSize)?)
        pointRadius = 100*fwhmList/2.
        scaleFactor = min(xGridSize/xSize, yGridSize/ySize)
        pointRadius *= scaleFactor    
        pointArea = math.pi*(pointRadius)**2.

        pltFwhm.scatter(xGridList, yGridList, s=pointArea, marker='o', color=None, facecolor=(1,1,1,0),
                        linewidth=5.0, label='PSF sample')

        # reference sample symbol
        fwhmPix = 5.0 # 5pix in fwhm
        pointRadius = 100*numpy.array([fwhmPix])/2. 
        scaleFactor = min(xGridSize/xSize, yGridSize/ySize)
        pointRadius *= scaleFactor    
        pointArea = math.pi*(pointRadius)**2.
        pltFwhm.scatter([0.1 * xSize], [0.9 * ySize], s=pointArea, marker='o', color='magenta',
                        facecolor=(1,1,1,0), linewidth=8.0, label='PSF sample')
        plt.text(0.1 * xSize, 0.9 * ySize, 'fwhm=%4.1f pix' % fwhmPix, ha='center', va='top')

        pltFwhm.set_title('FWHM of PSF sources')
        pltFwhm.set_xlabel('X (pix)')
        pltFwhm.set_ylabel('Y (pix)')

        plt.xticks([ xc+xGridSize/2. for xc in xGridList ])
        plt.yticks([ yc+yGridSize/2. for yc in yGridList ])
        pltFwhm.grid()

        fname = getFilename(dataRef, "plotFwhmGrid")
        plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait',
                    papertype=None, format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

        del fig
        del pltFwhm

    def plotEllipseGrid(self, dataRef, data, exposure):
        xSize, ySize = exposure.getWidth(), exposure.getHeight()

        facSize = 10.0 / max(xSize,ySize)  # targeting 10 inch in size
        wFig = xSize * facSize * 1.2
        hFig = ySize * facSize
        fig = plt.figure(figsize=(wFig,hFig))
        pltEllipse = plt.axes([0.2, 0.1, 0.7, 0.8]) # left,bottom,width,height

        pltEllipse.set_xlim(0, xSize)
        pltEllipse.set_ylim(0, ySize)

        xGridSize = self.config.gridSize
        yGridSize = self.config.gridSize

        # making grids
        nx = int(numpy.floor(xSize/xGridSize))
        ny = int(numpy.floor(ySize/yGridSize))

        xGrids = numpy.array([ (i+0.5)*xGridSize for i in range(nx) ])
        yGrids = numpy.array([ (i+0.5)*yGridSize for i in range(ny) ])
        xMeshGrid, yMeshGrid = numpy.meshgrid(xGrids, yGrids)
        xGridList = numpy.reshape(xMeshGrid, (-1,))
        yGridList = numpy.reshape(yMeshGrid, (-1,))    

        # walking through all grid meshes
        AEllList = numpy.array([])
        BEllList = numpy.array([])
        ellPaList = numpy.array([])        
        for xGridCenter, yGridCenter in zip(xGridList, yGridList):
            xGridMin = xGridCenter - xGridSize/2.
            xGridMax = xGridCenter + xGridSize/2.
            yGridMin = yGridCenter - yGridSize/2.
            yGridMax = yGridCenter + yGridSize/2.

            AEllsInGrid = numpy.array([])
            BEllsInGrid = numpy.array([])        
            ellPasInGrid = numpy.array([])
            for xEllipse, yEllipse, aa, bb, ellPa in zip(data.xListPsfLikeRobust, data.yListPsfLikeRobust,
                                                         data.AEllListPsfLikeRobust,
                                                         data.BEllListPsfLikeRobust,
                                                         data.ellPaListPsfLikeRobust):
                if xGridMin <= xEllipse and xEllipse < xGridMax and \
                        yGridMin <= yEllipse and yEllipse < yGridMax:
                    if all([aa, bb, ellPa]):
                        AEllsInGrid = numpy.append(AEllsInGrid, aa)
                        BEllsInGrid = numpy.append(BEllsInGrid, bb)
                        ellPasInGrid = numpy.append(ellPasInGrid, ellPa)
            # taking median to represent the value in a grid mesh
            AEllList = numpy.append(AEllList, numpy.median(AEllsInGrid))
            BEllList = numpy.append(BEllList, numpy.median(BEllsInGrid))        
            ellPaList = numpy.append(ellPaList, numpy.median(ellPasInGrid))        

        # 10pix=2arcsec(fwhm)==>A=0.6*gridSize~1200pix(for whole_CCD)
        scaleFactor = (1/10.)*0.8*min(xGridSize, yGridSize)
        for xEllipse, yEllipse, aa, bb, ellPa in zip(xGridList, yGridList, AEllList, BEllList, ellPaList):
            if all([aa, bb, ellPa]):
                ell = patches.Ellipse((xEllipse, yEllipse), 2.*aa*scaleFactor, 2.*bb*scaleFactor,
                                      angle=ellPa, linewidth=2., fill=False, zorder=2)
            pltEllipse.add_patch(ell)
        pltEllipse.set_title('Size and Ellongation of PSF sources')
        pltEllipse.set_xlabel('X (pix)')
        pltEllipse.set_ylabel('Y (pix)')

        # reference sample point
        fwhmPix = 5 # pixel in A, B (in half width)
        aa = bb = fwhmPix/2.
        ell = patches.Ellipse((0.1*xSize, 0.9*ySize), 2.*aa*scaleFactor, 2.*bb*scaleFactor, angle=0.,
                              linewidth=4., color='magenta', fill=False, zorder=2)
        pltEllipse.add_patch(ell)
        plt.text(0.1 * xSize, 0.9 * ySize, 'fwhm=%4.1f pix' % fwhmPix, ha='center', va='top')

        plt.xticks([ xc+xGridSize/2. for xc in xGridList ])
        plt.yticks([ yc+yGridSize/2. for yc in yGridList ])
        pltEllipse.grid()

        fname = getFilename(dataRef, "plotEllipseGrid")
        plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait', papertype=None,
                    format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

        del fig
        del pltEllipse

    def plotEllipticityGrid(self, dataRef, data, exposure):
        """ellipticity-in-grids plots"""
        # http://stackoverflow.com/questions/9382664/python-matplotlib-imshow-custom-tickmarks
    
        xSize, ySize = exposure.getWidth(), exposure.getHeight()
        facSize = 10.0 / max(xSize,ySize)  # targeting 10 inch in size
        wFig = xSize * facSize * 1.2
        hFig = ySize * facSize
        fig = plt.figure(figsize=(wFig,hFig))

        pltEll = plt.axes([0.2, 0.1, 0.7, 0.8]) # left,bottom,width,height
        pltEll.set_xlim(0, xSize)
        pltEll.set_ylim(0, ySize)

        xGridSize = self.config.gridSize
        yGridSize = self.config.gridSize
        nx = int(numpy.floor(xSize/xGridSize))
        ny = int(numpy.floor(ySize/yGridSize))

        xGrids = numpy.array([ (i+0.5)*xGridSize for i in range(nx) ])
        yGrids = numpy.array([ (i+0.5)*yGridSize for i in range(ny) ])
        xMeshGrid, yMeshGrid = numpy.meshgrid(xGrids, yGrids)
        xGridList = numpy.reshape(xMeshGrid, (-1,))
        yGridList = numpy.reshape(yMeshGrid, (-1,))            

        ellXList = numpy.array([])
        ellYList = numpy.array([])
        ellMed = numpy.array([])
        ellPaMed = numpy.array([])
        # walking through all grid meshes
        for xGridCenter, yGridCenter in zip(xGridList, yGridList):
            xGridMin = xGridCenter - xGridSize/2.
            xGridMax = xGridCenter + xGridSize/2.
            yGridMin = yGridCenter - yGridSize/2.
            yGridMax = yGridCenter + yGridSize/2.

            ellsInGrid = numpy.array([])
            ellPasInGrid = numpy.array([])
            for xEll, yEll, ell, ellPa in zip(data.xListPsfLikeRobust, data.yListPsfLikeRobust,
                                              data.ellListPsfLikeRobust, data.ellPaListPsfLikeRobust):
                if (xGridMin <= xEll) and (xEll < xGridMax) and (yGridMin <= yEll) and (yEll < yGridMax):
                    if all([ell, ellPa]):
                        ellsInGrid = numpy.append(ellsInGrid, ell)
                        ellPasInGrid = numpy.append(ellPasInGrid, ellPa)
            # taking median to represent the value in a grid mesh
            ellPerGrid = numpy.median(ellsInGrid)
            ellPaPerGrid = numpy.median(ellPasInGrid)

            ellXList = numpy.append(ellXList, ellPerGrid*numpy.cos(numpy.radians(ellPaPerGrid)))
            ellYList = numpy.append(ellYList, ellPerGrid*numpy.sin(numpy.radians(ellPaPerGrid)))

            ellMed = numpy.append(ellMed, ellPerGrid)
            ellPaMed = numpy.append(ellPaMed, ellPaPerGrid)                

        scaleFactor = min(xGridSize/xSize, yGridSize/ySize)

        Q = pltEll.quiver(
            xGridList, yGridList,
            ellXList, ellYList,    # ellipticity 1 ~ 1000?
            units = 'x', # scale is based on multiplication of 'x (pixel)'
            angles = 'uv',
            #angles = ellPaMed,
            scale = (2e-5 / scaleFactor),   # (ell/pix)
            # 1pix corresponds to this value of ellipticity --> 1000pix = ellipticity 0.02 for full CCD size.
            # and scaled by using (GridSize(in shorter axis) / CcdSize)
            scale_units = 'x',
            headwidth=0,
            headlength=0,
            headaxislength=0,
            width = 50,
            #label='PSF sample'
            #angles = 'xy',
            pivot = 'middle',
            )
        plt.quiverkey(Q, 0.05, 1.05, 0.05, 'e=0.05', labelpos='W')

        plt.xticks([ xc+xGridSize/2. for xc in xGridList ])
        plt.yticks([ yc+yGridSize/2. for yc in yGridList ])
        pltEll.grid()

        pltEll.set_title('Ellipticity of PSF sources')
        pltEll.set_xlabel('X (pix)')
        pltEll.set_ylabel('Y (pix)')


        fname = getFilename(dataRef, "plotEllipticityGrid")
        plt.savefig(fname, dpi=None, facecolor='w', edgecolor='w', orientation='portrait', papertype=None,
                    format='png', transparent=False, bbox_inches=None, pad_inches=0.1)

        del fig
        del pltEll


def getFilename(dataRef, dataset):
    fname = dataRef.get(dataset + "_filename")[0]
    directory = os.path.dirname(fname)
    if not os.path.exists(directory):
        os.makedirs(directory)
    return fname
            
