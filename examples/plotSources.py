

class Source(object):
    pass

def plotSources(butler=None, dataId=None, exposure=None, image=None,
                sources=None, fn=None, bboxes=None, exposureDatatype='visitim',
                datarange=None, roi=None):
    import pylab as plt
    import numpy as np
    from matplotlib.patches import Ellipse

    if image is None:
        if exposure is None:
            assert(butler is not None and dataId is not None)
            print 'Reading exposure'
            exposure = butler.get(exposureDatatype, dataId)
        assert(exposure is not None)
        print 'exposure is', exposure
        print 'size', exposure.getWidth(), 'x', exposure.getHeight()
        mi = exposure.getMaskedImage()
        image = mi.getImage()
    assert(image is not None)

    if sources is None:
        sources = butler.get('src', dataId)
        print 'got sources', sources
        sources = sources.getSources()

    #print 'got sources', sources
    #for s in sources:
    #    print '  ', s, 'x,y', s.getXAstrom(), s.getYAstrom(), 'psfFlux', s.getPsfFlux(), 'Ixx,Iyy,Ixy', s.getIxx(), s.getIyy(), s.getIxy()

    # old-school numpy has no rad2deg
    if not hasattr(np, 'rad2deg'):
        np.rad2deg = np.degrees
    if not hasattr(np, 'deg2rad'):
        np.deg2rad = np.radians

    # this is what we are reduced to?
    if roi is None:
        x0,x1,y0,y1 = 0,image.getWidth(),0,image.getHeight()
    else:
        x0,x1,y0,y1 = roi
    img = np.empty((y1-y0, x1-x0))
    for r,row in enumerate(range(y0, y1)):
        for c,col in enumerate(range(x0, x1)):
            img[r,c] = image.get(col, row)

    plt.clf()
    imargs = dict(origin='lower', interpolation='nearest')
    if datarange is not None:
        imargs.update(vmin=datarange[0], vmax=datarange[1])
    if roi is not None:
        imargs.update(extent=roi)

    plt.imshow(img, **imargs)
    plt.gray()
    ax = plt.axis()
    plt.plot([s.getXAstrom() for s in sources],
             [s.getYAstrom() for s in sources], 'r.', alpha=0.3)

    x2 = np.array([s.getIxx() for s in sources])
    y2 = np.array([s.getIyy() for s in sources])
    xy = np.array([s.getIxy() for s in sources])
    # SExtractor manual pg 29.
    a2 = (x2 + y2)/2. + np.sqrt(((x2 - y2)/2.)**2 + xy**2)
    b2 = (x2 + y2)/2. - np.sqrt(((x2 - y2)/2.)**2 + xy**2)
    theta = np.rad2deg(np.arctan2(2.*xy, (x2 - y2)) / 2.)
    a = np.sqrt(a2)
    b = np.sqrt(b2)
    #print 'a,b', a, b
    #print 'theta', theta

    x = np.array([s.getXAstrom() for s in sources])
    y = np.array([s.getYAstrom() for s in sources])

    ca = plt.gca()
    #print 'x, y, a, b, theta:'

    # Number of radii outside image bounds at which to trim sources
    nr = 8
    srcs = []
    for xi,yi,ai,bi,ti,si in zip(x, y, a, b, theta, sources):
        margin = nr * max(ai,bi)
        if (xi < x0 - margin or xi > x1 + margin or
            yi < y0 - margin or yi > y1 + margin):
            print 'skipping source at xi,yi = ', (xi,yi)
            continue
        #print '  x,y', (xi, yi), 'a,b', (ai,bi), 'theta', ti
        # alpha=0.25 doesn't seem to work, but lw=0.5 does.
        el = Ellipse([xi,yi], 2.*ai, 2.*bi, angle=ti,
                     ec='r', fc='none',
                     lw=0.5)
                     #alpha=0.25,
        ca.add_artist(el)

        tirad = np.deg2rad(ti)
        plt.plot([xi, xi + ai * np.cos(tirad)],
                 [yi, yi + ai * np.sin(tirad)], 'r-', alpha=0.5)

        s = Source()
        s.ra, s.dec = si.getRa().asDegrees(), si.getDec().asDegrees()
        s.x, s.y = xi, yi
        s.a, s.b, s.theta = ai, bi, ti
        s.flux = si.getModelFlux()
        #print 'psf flux', s.flux
        #print 'model flux', si.getModelFlux()
        srcs.append(s)

    if bboxes is not None:
        for i,(x0,y0,x1,y1) in enumerate(bboxes):
            plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'b-', alpha=0.5)
            plt.text(x0, y0, '%i' % i, color='b')

    plt.axis(ax)
    if fn is not None:
        plt.savefig(fn)

    return srcs
