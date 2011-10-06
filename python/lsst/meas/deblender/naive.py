
import lsst.afw.image as afwImage
import lsst.afw.geom  as afwGeom

def deblend(footprints, maskedImage, psf):
    print 'Naive deblender starting'
    print 'footprints', footprints
    print 'maskedImage', maskedImage
    print 'psf', psf
    allt = []
    img = maskedImage.getImage()
    for fp in footprints:
        bb = fp.getBBox()
        W,H = bb.getWidth(), bb.getHeight()
        x0,y0 = bb.getMinX(), bb.getMinY()
        templs = []
        for pk in fp.getPeaks():
            template = afwImage.MaskedImage(W,H)
            template.setXY0(x0,y0)
            cx,cy = pk.getIx(), pk.getIy()
            timg = template.getImage()
            timg.set(cx,cy, img.get(cx,cy))
            #template.set(cx,cy, 
            for dy in range(H-cy):
                for dx in range(-cx, W-cx):
                    if dy == 0 and dx == 0:
                        continue
                    # twofold rotational symmetry
                    xa,ya = cx+dx, cy+dy
                    xb,yb = cx-dx, cy-dy
                    if not fp.contains(afwGeom.Point2I(xa, ya)):
                        continue
                    if not fp.contains(afwGeom.Point2I(xb, yb)):
                        continue
                    pa = img.get(xa, ya)
                    pb = img.get(xb, yb)
                    mn = min(pa,pb)
                    # Monotonic?
                    timg.set(xa, ya, mn)
                    timg.set(xb, yb, mn)

            templs.append(timg)
        allt.append(templs)
    return allt
