#!/usr/bin/env python
import lsst.pex.policy as pexPolicy
import lsst.afw.table as afwTable
import lsst.afw.geom as afwGeom
import lsst.afw.cameraGeom as cameraGeom
from lsst.afw.cameraGeom import SCIENCE, FOCAL_PLANE, PUPIL, PIXELS, CameraConfig, DetectorConfig,\
                                makeCameraFromCatalogs
from lsst.obs.hsc.transformRegistry import xyTransformRegistry
from lsst.obs.hsc.hscMapper import HscMapper
from lsst.obs.suprimecam import SuprimecamMapper

import argparse
import eups
import os
import copy

FOCAL_PLANE_PIXELS = cameraGeom.CameraSys('Focal_Plane_Pixels')

PIXELSIZE = 0.015 #mm/pix
def makeCameraFromPolicy(filename, writeRepo=False, outputDir=None, doClobber=False, shortNameMethod=lambda x: x):
    policyFile = pexPolicy.DefaultPolicyFile("afw", "CameraGeomDictionary.paf", "policy")
    defPolicy = pexPolicy.Policy.createPolicy(policyFile, policyFile.getRepositoryPath(), True)

    polFile = pexPolicy.DefaultPolicyFile("obs_subaru", filename)
    geomPolicy = pexPolicy.Policy.createPolicy(polFile)
    geomPolicy.mergeDefaults(defPolicy.getDictionary())
    ampParams = makeAmpParams(geomPolicy)
    ccdParams = makeCcdParams(geomPolicy, ampParams)
    ccdToUse = None
    ccdInfoDict = parseCcds(geomPolicy, ccdParams, ccdToUse)
    camConfig = parseCamera(geomPolicy)
    camConfig.detectorList = dict([(i, ccdInfo) for i, ccdInfo in enumerate(ccdInfoDict['ccdInfo'])])
    if writeRepo:
        if outputDir is None:
            raise ValueError("Need output directory for writting")
        def makeDir(dirPath, doClobber=False):
            """Make a directory; if it exists then clobber or fail, depending on doClobber

            @param[in] dirPath: path of directory to create
            @param[in] doClobber: what to do if dirPath already exists:
                if True and dirPath is a dir, then delete it and recreate it, else raise an exception
            @throw RuntimeError if dirPath exists and doClobber False
            """
            if os.path.exists(dirPath):
                if doClobber and os.path.isdir(dirPath):
                    print "Clobbering directory %r" % (dirPath,)
                    shutil.rmtree(dirPath)
                else:
                    raise RuntimeError("Directory %r exists" % (dirPath,))
            print "Creating directory %r" % (dirPath,)
            os.makedirs(dirPath)

        # write data products
        makeDir(dirPath=outputDir, doClobber=doClobber)

        camConfigPath = os.path.join(outputDir, "camera.py")
        camConfig.save(camConfigPath)

        for detectorName, ampTable in ccdInfoDict['ampInfo'].iteritems():
            shortDetectorName = shortNameMethod(detectorName)
            ampInfoPath = os.path.join(outputDir, shortDetectorName + ".fits")
            ampTable.writeFits(ampInfoPath)

    return makeCameraFromCatalogs(camConfig, ccdInfoDict['ampInfo'])

def parseCamera(policy):
    camPolicy = policy.get('Camera')
    camConfig = CameraConfig()
    camConfig.name = camPolicy.get('name')
    # Assuming a 15m focal length
    # camConfig.plateScale = 13.751 #arcsec/mm
    camConfig.plateScale = 11. #arcsec/mm
    
    tConfig = afwGeom.TransformConfig()
    #Need to invert because this is stored with FOCAL_PLANE to PUPIL as the 
    #forward transform
    tConfig.transform.name = 'distEst'
    tConfig.transform.active.plateScale = camConfig.plateScale
    #This defaults to 45. anyway, but to show how it is set
    tConfig.transform.active.elevation = 45.
    """
    radialClass = afwGeom.xyTransformRegistry['radial']
    tConfig.transform.active.transform.retarget(radialClass)
    #The distorition is in pixels (I think), convert to radians
    conv = 1./afwGeom.arcsecToRad(camConfig.plateScale)
    coeffs = [conv*coeff for coeff in camPolicy.getArray('Distortion.RadialPolyDistortion.coeffs')]
    tConfig.transform.active.transform.coeffs = coeffs
    """
    tpConfig = afwGeom.TransformConfig()
    tpConfig.transform.name = 'affine'
    tpConfig.transform.active.linear = [1/PIXELSIZE, 0, 0, 1/PIXELSIZE]

    tmc = afwGeom.TransformMapConfig()
    tmc.nativeSys = FOCAL_PLANE.getSysName()
    tmc.transforms = {PUPIL.getSysName():tConfig, FOCAL_PLANE_PIXELS.getSysName():tpConfig}
    camConfig.transformDict = tmc
    return camConfig

def makeAmpParams(policy):
    retParams = {}
    for amp in policy.getArray('Amp'):
        retParams[amp.get('ptype')] = {}
        retParams[amp.get('ptype')]['datasec'] = amp.getArray('datasec')
        retParams[amp.get('ptype')]['biassec'] = amp.getArray('biassec')
        retParams[amp.get('ptype')]['ewidth'] = amp.get('ewidth')
        retParams[amp.get('ptype')]['eheight'] = amp.get('eheight')
    return retParams

def makeCcdParams(policy, ampParms):
    retParams = {}
    for ccd in policy.getArray('Ccd'):
        ptype = ccd.get('ptype')
        tdict = {}
        #The policy file has units of pixels, but to get to
        #pupil coords we need to work in units of mm
        tdict['pixelSize'] = PIXELSIZE
        tdict['offsetUnit'] = ccd.get('offsetUnit')
        tdict['ampArr'] = []
        xsize = 0
        ysize = 0
        for amp in ccd.getArray('Amp'):
            parms = copy.copy(ampParms[amp.get('ptype')])
            xsize += parms['datasec'][2] - parms['datasec'][0] + 1
            #I think the subaru chips only have a single row of amps
            ysize = parms['datasec'][3] - parms['datasec'][1] + 1
            parms['id'] = amp.get('serial')
            parms['flipX'] = amp.get('flipLR')
            #As far as I know there is no bilateral symmetry in subaru cameras
            parms['flipY'] = False
            tdict['ampArr'].append(parms)
        tdict['xsize'] = xsize
        tdict['ysize'] = ysize
        retParams[ptype] = tdict
    return retParams

def makeEparams(policy): 
    rafts = policy.getArray('Electronic.Raft')
    if len(rafts) > 1:
        raise ValueError("These cameras should only have one raft")
    eparms = {}
    for ccd in rafts[0].getArray('Ccd'):
        eparms[ccd.get('name')] = []
        for amp in ccd.getArray('Amp'):
            eparm = {}
            eparm['index'] = amp.getArray('index')
            eparm['gain'] = amp.get('gain')
            eparm['readNoise'] = amp.get('readNoise')
            eparm['saturation'] = amp.get('saturationLevel')
            eparms[ccd.get('name')].append(eparm)
    return eparms

def addAmp(ampCatalog, amp, eparams):
    """ Add an amplifier to an AmpInfoCatalog
    
    @param ampCatalog: An instance of an AmpInfoCatalog object to fill with amp properties
    @param amp: Dictionary of amp geometric properties
    @param eparams: Dictionary of amp electronic properties for this amp
    """
    record = ampCatalog.addNew()
 
    xtot = amp['ewidth']
    ytot = amp['eheight']

    allPixels = afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(xtot, ytot))
    biassl = amp['biassec']
    biasSec = afwGeom.Box2I(afwGeom.Point2I(biassl[0], biassl[1]), afwGeom.Point2I(biassl[2], biassl[3]))
    datasl = amp['datasec']
    dataSec = afwGeom.Box2I(afwGeom.Point2I(datasl[0], datasl[1]), afwGeom.Point2I(datasl[2], datasl[3]))

    extended = dataSec.getMin().getX()
    voverscan = ytot - dataSec.getMaxY()
    pscan = dataSec.getMin().getY()

    voscanSec = afwGeom.Box2I(afwGeom.Point2I(extended, dataSec.getMax().getY()+1),
                              afwGeom.Point2I(dataSec.getMax().getX(), allPixels.getMax().getY()))

    pscanSec = afwGeom.Box2I(afwGeom.Point2I(extended, 0),
                             afwGeom.Point2I(dataSec.getMax().getX(),pscan-1))

    if amp['flipX']:
        #No need to flip bbox or allPixels since they
        #are at the origin and span the full pixel grid
        biasSec.flipLR(xtot)
        dataSec.flipLR(xtot)
        voscanSec.flipLR(xtot)
        pscanSec.flipLR(xtot) 

    bbox = afwGeom.BoxI(afwGeom.PointI(0, 0), dataSec.getDimensions())
    bbox.shift(afwGeom.Extent2I(dataSec.getDimensions().getX()*eparams['index'][0], 0))

    shiftp = afwGeom.Extent2I(xtot*eparams['index'][0], 0)
    allPixels.shift(shiftp)
    biasSec.shift(shiftp)
    dataSec.shift(shiftp)
    voscanSec.shift(shiftp)
    pscanSec.shift(shiftp)
   

    record.setBBox(bbox)
    record.setRawXYOffset(afwGeom.ExtentI(0,0))
    record.setName("%i,%i"%(eparams['index'][0], eparams['index'][1]))
    record.setReadoutCorner(afwTable.LR if amp['flipX'] else afwTable.LL)    
    record.setGain(eparams['gain'])
    record.setReadNoise(eparams['readNoise'])
    record.setSaturation(int(eparams['saturation']))
    #The files do not have any linearity information
    record.setLinearityType('Proportional')
    record.setLinearityCoeffs([1.,])
    record.setHasRawInfo(True)
    record.setRawFlipX(False)
    record.setRawFlipY(False)
    record.setRawBBox(allPixels)
    record.setRawDataBBox(dataSec)
    record.setRawHorizontalOverscanBBox(biasSec)
    record.setRawVerticalOverscanBBox(voscanSec)
    record.setRawPrescanBBox(pscanSec)

def parseCcds(policy, ccdParams, ccdToUse=None):
    # The pafs I have now in the hsc dir include the focus sensors (but not the guiders)
    specialChipMap = {'hsc108':cameraGeom.FOCUS, 'hsc110':cameraGeom.FOCUS, 'hsc111':cameraGeom.FOCUS,
                      'hsc107':cameraGeom.FOCUS, 'hsc105':cameraGeom.FOCUS, 'hsc104':cameraGeom.FOCUS,
                      'hsc109':cameraGeom.FOCUS, 'hsc106':cameraGeom.FOCUS}
    eParams = makeEparams(policy)
    ampInfoDict ={}
    ccdInfoList = []
    rafts = policy.getArray('Raft')
    if len(rafts) > 1:
        raise ValueError("Expecting only one raft")
    for ccd in rafts[0].getArray('Ccd'):
        detConfig = DetectorConfig()
        schema = afwTable.AmpInfoTable.makeMinimalSchema()
        ampCatalog = afwTable.AmpInfoCatalog(schema)
        if ccdToUse is not None:
            ccdParam = ccdParams[ccdToUse]
        else:
            ccdParam = ccdParams[ccd.get('ptype')]
        detConfig.name = ccd.get('name')
        #This should be the serial number on the device, but for now is an integer id
        detConfig.serial = str(ccd.get('serial'))
        detConfig.id = ccd.get('serial')
        offset = ccd.getArray('offset')
        if ccdParam['offsetUnit'] == 'pixels':
            offset[0] *= ccdParam['pixelSize']
            offset[1] *= ccdParam['pixelSize']
        detConfig.offset_x = offset[0]
        detConfig.offset_y = offset[1]
        if detConfig.name in specialChipMap:
            detConfig.detectorType = specialChipMap[detConfig.name]
        else:
            detConfig.detectorType = cameraGeom.SCIENCE
        detConfig.pixelSize_x = ccdParam['pixelSize']
        detConfig.pixelSize_y = ccdParam['pixelSize']
        detConfig.refpos_x = (ccdParam['xsize'] - 1)/2.
        detConfig.refpos_y = (ccdParam['ysize'] - 1)/2.
        detConfig.bbox_x0 = 0
        detConfig.bbox_y0 = 0
        detConfig.bbox_x1 = ccdParam['xsize'] - 1
        detConfig.bbox_y1 = ccdParam['ysize'] - 1
        detConfig.rollDeg = 0.
        detConfig.pitchDeg = 0.
        detConfig.yawDeg = 90.*ccd.get('nQuarter') + ccd.getArray('orientation')[2]
        for amp in ccdParam['ampArr']:
            eparms = None
            for ep in eParams[ccd.get('name')]:
                if amp['id']-1 == ep['index'][0]:
                    eparms = ep
            if eparms is None:
                raise ValueError("Could not find electronic params.")
            addAmp(ampCatalog, amp, eparms)
        ampInfoDict[ccd.get('name')] = ampCatalog
        ccdInfoList.append(detConfig)
    return {"ccdInfo":ccdInfoList, "ampInfo":ampInfoDict}

if __name__ == "__main__":
    mapperDict = {'hsc':HscMapper, 'suprimecam':SuprimecamMapper}
    baseDir = eups.productDir("obs_subaru")

    parser = argparse.ArgumentParser()
    parser.add_argument("LayoutPolicy", help="Policy file to parse for camera information")
    parser.add_argument("InstrumentName", help="Name of the instrument (hsc, suprimecam)")
    parser.add_argument("OutputDir", help="Location for the persisted camerea") 
    parser.add_argument("--clobber", action="store_true", dest="clobber", default=False,
        help="remove and re-create the output directory if it exists")
    args = parser.parse_args()
   
    camera = makeCameraFromPolicy(args.LayoutPolicy, writeRepo=True, outputDir=args.OutputDir, doClobber=args.clobber, 
        shortNameMethod=mapperDict[args.InstrumentName].getShortCcdName)
