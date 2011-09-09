import os

import lsst.obs.suprimecam  as obsSc
import lsst.daf.persistence as dafPer

# needed in order to un-persist the PSF
import lsst.meas.algorithms as measAlg

import lsst.afw.detection   as afwDet
import lsst.afw.coord       as afwCoord
import lsst.afw.geom        as afwGeom

if __name__ == '__main__':

    basedir = '/home/dstn/lsst/ACT-data'
    caldir = os.path.join(basedir, 'CALIB')

    mapper = obsSc.SuprimecamMapper(root=basedir, calibRoot=caldir, rerun='dstn')
    bf = dafPer.ButlerFactory(mapper=mapper)
    but = bf.create()

    # One Exposure? including WCS, PSF
    #  1269685
    did = {'visit': 126968, 'ccd': 5}
    print 'calexp map', mapper.map('calexp', dataId=did)
    exp = but.get('calexp', dataId=did)

    print 'psf map', mapper.map('psf', dataId=did)
    psf = but.get('psf', dataId=did)
    print 'psf', psf
    exp.setPsf(psf)

    print 'Exposure:', exp
    print 'PSF:', exp.getPsf()
    print 'WCS:', exp.getWcs()

    # List of sources from union of three ACT-cluster SuprimeCam exposures:
    #  1269685
    #  1269699
    #  1269709
    # In the region RA in [5.60, 5.66], Dec in [-0.76, -0.70]

    s1 = but.get('src', dataId=did)
    print 'Sources', s1
    s1 = s1.getSources()
    print 'Sources', s1
    print 'Got', len(s1), 'sources'

    s2 = but.get('src', dataId={'visit': 126969, 'ccd': 9})
    s3 = but.get('src', dataId={'visit': 126970, 'ccd': 9})
    s2 = s2.getSources()
    s3 = s3.getSources()
    print 'Got', len(s2), 'and', len(s3), 'other sources'
    
    #s1.remove(0)
    #print len(s1)

    ralo,  rahi  = 5.60, 5.66
    declo, dechi = -0.76, -0.70

    #ss = afwDet.SourceSet()
    keep1 = []
    for s in s1:
        ra = s.getRa().asDegrees()
        if ra < ralo or ra > rahi:
            continue
        dec = s.getDec().asDegrees()
        if dec < declo or dec > dechi:
            continue
        keep1.append(s)

    keep2 = []
    for ss in [s2,s3]:
        for s in ss:
            ra = s.getRa().asDegrees()
            if ra < ralo or ra > rahi:
                continue
            dec = s.getDec().asDegrees()
            if dec < declo or dec > dechi:
                continue
            keep2.append(s)

    print 'Kept', len(keep1), 'and', len(keep2)
    s1 = keep1
    s2 = keep2

    # Any reason to keep s1, s2 separate?
    ss = s1 + s2

    # trim nearby pairs -- by brute-force
    maxsep = 0.5 * afwGeom.arcseconds
    print 'maxsep', maxsep
    print maxsep.asArcseconds(), 'arcsec'
    keep = range(len(ss))
    i = 0
    while True:
        if i >= len(keep):
            break
        si = ss[keep[i]]
        rdi = si.getRaDec()
        # we're going to keep si; remove any sj that is too close.
        print 'rdi', rdi
        j = i + 1
        while True:
            if j >= len(keep):
                break
            rdj = ss[keep[j]].getRaDec()
            print '  i', i, 'j', j
            print '  rdj', rdj
            sep = rdi.angularSeparation(rdj)
            print '    ', sep
            print '    ', sep.asArcseconds(), 'arcsec'
            if sep >= maxsep:
                print '    keeping j', j, 'keep[j]', keep[j]
                j += 1
                continue
            print '  removing j', j, 'keep[j]', keep[j]
            print '  sep', sep, '  <  maxsep', maxsep
            assert(sep.asArcseconds() < maxsep.asArcseconds())
            keep.pop(j)
            # j is not incremented (we've just removed an element)
        i += 1
                
    print 'After trimming nearby pairs:', len(keep)


