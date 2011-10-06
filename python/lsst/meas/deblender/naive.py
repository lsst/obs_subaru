import math

import lsst.afw.image as afwImage
import lsst.afw.geom  as afwGeom
import lsst.afw.math  as afwMath

def deblend(footprints, peaks, maskedImage, psf):
    print 'Naive deblender starting'
    print 'footprints', footprints
    print 'maskedImage', maskedImage
    print 'psf', psf
    allt = []
    allp = []
    allbgsub = []
    allmod = []
    
    img = maskedImage.getImage()
    for fp,pks in zip(footprints,peaks):
        bb = fp.getBBox()
        W,H = bb.getWidth(), bb.getHeight()
        x0,y0 = bb.getMinX(), bb.getMinY()

        templs = []
        bgsubs = []
        models = []
        #for pk in fp.getPeaks():
        print 'Footprint x0,y0', x0,y0
        print 'W,H', W,H
        for pk in pks:
            template = afwImage.MaskedImageF(W,H)
            template.setXY0(x0,y0)
            cx,cy = pk.getIx(), pk.getIy()
            timg = template.getImage()
            p = img.get(cx,cy)
            timg.set(cx - x0, cy - y0, p)
            #template.set(cx,cy, 
            for dy in range(H-(cy-y0)):
                #print 'dy', dy
                for dx in range(-(cx-x0), W-(cx-x0)):
                    #print 'dx', dx
                    if dy == 0 and dx == 0:
                        continue
                    # twofold rotational symmetry
                    xa,ya = cx+dx, cy+dy
                    xb,yb = cx-dx, cy-dy
                    #print 'xa,ya', xa,ya
                    if not fp.contains(afwGeom.Point2I(xa, ya)):
                        #print ' (oob)'
                        continue
                    #print 'xb,yb', xb,yb
                    if not fp.contains(afwGeom.Point2I(xb, yb)):
                        #print ' (oob)'
                        continue
                    pa = img.get(xa, ya)
                    pb = img.get(xb, yb)
                    #print 'pa', pa, 'pb', pb
                    mn = min(pa,pb)
                    # Monotonic?
                    timg.set(xa - x0, ya - y0, mn)
                    timg.set(xb - x0, yb - y0, mn)
            templs.append(timg)

        # Now try fitting a PSF + smooth background model to the template.
        # Smooth background = ....?
        # ... median filter?
        # ... from our friends down at math::makeBackground?

        #for timg in templs:
        for timg in []:
            # declare a background control object for a natural spline
            bgctrl = afwMath.BackgroundControl(afwMath.Interpolate.NATURAL_SPLINE)
            gridsz = 32.
            bgctrl.setNxSample(int(math.ceil(W / gridsz)))
            bgctrl.setNySample(int(math.ceil(H / gridsz)))
            # bgctrl.getStatisticsControl().setNumIter(3)
            # bgctrl.getStatisticsControl().setNumSigmaClip(2.5)
            back = afwMath.makeBackground(timg, bgctrl)
            bgsub = afwImage.ImageF(W,H)
            bgsub += timg
            bgsub -= back.getImageF()
            bgsubs.append(bgsub)

        for pk,timg in zip(pks, templs):
            from scipy.ndimage.filters import median_filter
            import numpy as np
            npimg = np.zeros((H,W))
            for y in range(H):
                for x in range(W):
                    npimg[y,x] = timg.get(x,y)
            # median filter patch size
            mfsize = 10
            mf = median_filter(npimg, mfsize) #mode='constant', cval=0.)

            bgsub = afwImage.ImageF(W,H)
            bgsub += timg
            for y in range(H):
                for x in range(W):
                    bgsub.set(x,y, bgsub.get(x,y) - mf[y,x])
                    #bgsub = timg.get(x,y) - mf[y,x]
            bgsubs.append(bgsub)

            psfimg = psf.computeImage(afwGeom.Point2D(pk.getFx(), pk.getFy()))
            bbox = psfimg.getBBox(afwImage.PARENT)
            print 'Peak pos', pk.getFx(), pk.getFy()
            print 'PSF bbox X:', bbox.getMinX(), bbox.getMaxX()
            print 'PSF bbox Y:', bbox.getMinY(), bbox.getMaxY()
            bbox.clip(fp.getBBox())
            print 'clipped bbox X:', bbox.getMinX(), bbox.getMaxX()
            print 'clipped bbox Y:', bbox.getMinY(), bbox.getMaxY()
            px0,py0 = psfimg.getX0(), psfimg.getY0()

            numer = 0.
            denom = 0.
            for y in range(bbox.getMinY(), bbox.getMaxY()+1):
                for x in range(bbox.getMinX(), bbox.getMaxX()+1):
                    p = psfimg.get(x-px0,y-py0)
                    numer += p * bgsub.get(x-x0, y-y0)
                    denom += p * p
            amp = numer / denom
            print 'PSF amplitude:', amp

            model = afwImage.ImageF(W,H)
            for y in range(H):
                for x in range(W):
                    #m = bgsub.get(x,y)
                    m = 0.
                    px,py = x+x0,y+y0
                    if bbox.contains(afwGeom.Point2I(px,py)):
                        m += amp * psfimg.get(px-px0, py-py0)
                    model.set(x,y, m)
            models.append(model)

        # Now apportion flux according to the templates ?
        portions = [afwImage.ImageF(W,H) for t in templs]
        for y in range(H):
            for x in range(W):
                tvals = [t.get(x,y) for t in templs]
                #S = sum([max(0, t) for t in tvals])
                S = sum([abs(t) for t in tvals])
                if S == 0:
                    continue
                for t,p in zip(tvals,portions):
                    #p.set(x,y, img.get(x0+x, y0+y) * max(0,tvals[i])/S)
                    p.set(x,y, img.get(x0+x, y0+y) * abs(t)/S)

        allp.append(portions)
        allt.append(templs)
        allbgsub.append(bgsubs)
        allmod.append(models)
    return allt, allp, allbgsub, allmod
