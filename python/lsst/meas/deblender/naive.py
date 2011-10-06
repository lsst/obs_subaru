
import lsst.afw.image as afwImage
import lsst.afw.geom  as afwGeom

def deblend(footprints, peaks, maskedImage, psf):
    print 'Naive deblender starting'
    print 'footprints', footprints
    print 'maskedImage', maskedImage
    print 'psf', psf
    allt = []
    allp = []
    img = maskedImage.getImage()
    for fp,pks in zip(footprints,peaks):
        bb = fp.getBBox()
        W,H = bb.getWidth(), bb.getHeight()
        x0,y0 = bb.getMinX(), bb.getMinY()

        templs = []
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

    return allt, allp
