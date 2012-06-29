import os

import lsst.obs.suprimecam  as obsSc
import lsst.daf.persistence as dafPer

# needed in order to un-persist the PSF
import lsst.meas.algorithms as measAlg

import lsst.afw.detection   as afwDet
import lsst.afw.coord       as afwCoord
import lsst.afw.geom        as afwGeom


class DeblendSource(object):
    # Want some representation of an ellipse in RA,Dec...

    def __init__(self):
        pass




if __name__ == '__main__':

    basedir = '/home/dstn/lsst/ACT-data'
    caldir = os.path.join(basedir, 'CALIB')

    mapper = obsSc.SuprimecamMapper(root=basedir, calibRoot=caldir, rerun='dstn')
    bf = dafPer.ButlerFactory(mapper=mapper)
    but = bf.create()

    # One Exposure? including WCS, PSF
    #  1269685

    exps = []
    for did in [ {'visit': 126968, 'ccd': 5},
                 #{'visit': 126969, 'ccd': 9},
                 ]:
        print 'calexp map', mapper.map('calexp', dataId=did)
        exp = but.get('calexp', dataId=did)
        print 'psf map', mapper.map('psf', dataId=did)
        psf = but.get('psf', dataId=did)
        print 'psf', psf
        exp.setPsf(psf)
        print 'Exposure:', exp
        print 'PSF:', exp.getPsf()
        print 'WCS:', exp.getWcs()
        exps.append(exp)

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
    
    ralo,  rahi  = 5.60, 5.66
    declo, dechi = -0.76, -0.70

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
    # (nearly -- sort by Dec and cut on that)
    ss.sort(key=lambda x: x.getDec())
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
        j = i + 1
        while True:
            if j >= len(keep):
                break
            sj = ss[keep[j]]
            rdj = sj.getRaDec()
            sep = rdi.angularSeparation(rdj)
            if sep >= maxsep:
                # sources are sorted by Dec, so once we've found a j with Dec greater
                # than Dec_i + maxsep, we can skip all further j.
                if sj.getDec() > (si.getDec() + maxsep):
                    #for k in range(j, len(keep)):
                    #    assert(rdi.angularSeparation(ss[keep[k]].getRaDec()) >= maxsep)
                    break
                j += 1
                continue
            assert(sep.asArcseconds() < maxsep.asArcseconds())
            keep.pop(j)
            # j is not incremented (we've just removed an element)
        i += 1
    print 'After trimming nearby pairs:', len(keep)



    # We want to pass to the deblender/multimeas a list of sources with properties:
    #  -ra,dec extent
    #  -template (may be null)
    #  -boolean: currently being measured?
    #     (we want to know about nearby but not-currently-of-interest sources)






