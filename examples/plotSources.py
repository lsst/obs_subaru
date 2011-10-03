

def plotSources(butler=None, dataId=None, exposure=None, image=None,
                sources=None):
    import pylab as plt
    import numpy as np
    from matplotlib.patches import Ellipse

    if image is None:
        if exposure is None:
            assert(butler is not None and dataId is not None)
            print 'Reading exposure'
            exposure = butler.get('visitim', dataId)
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

    print 'got sources', sources
    for s in sources:
        print '  ', s, 'x,y', s.getXAstrom(), s.getYAstrom(), 'psfFlux', s.getPsfFlux(), 'Ixx,Iyy,Ixy', s.getIxx(), s.getIyy(), s.getIxy()

    # old-school numpy has no rad2deg
    if not hasattr(np, 'rad2deg'):
        np.rad2deg = np.degrees
    if not hasattr(np, 'deg2rad'):
        np.deg2rad = np.radians

    # this is what we are reduced to?
    img = np.empty((image.getHeight(), image.getWidth()))
    for row in range(image.getHeight()):
        for col in range(image.getWidth()):
            img[row,col] = image.get(col, row)

    plt.clf()
    plt.imshow(img)
    plt.gray()
    ax = plt.axis()
    plt.plot([s.getXAstrom() for s in sources],
             [s.getYAstrom() for s in sources], 'r.')

    x2 = np.array([s.getIxx() for s in sources])
    y2 = np.array([s.getIyy() for s in sources])
    xy = np.array([s.getIxy() for s in sources])
    # SExtractor manual pg 29.
    a2 = (x2 + y2)/2. + np.sqrt(((x2 - y2)/2.)**2 + xy**2)
    b2 = (x2 + y2)/2. - np.sqrt(((x2 - y2)/2.)**2 + xy**2)
    theta = np.rad2deg(np.arctan2(2.*xy, (x2 - y2)) / 2.)
    a = np.sqrt(a2)
    b = np.sqrt(b2)
    print 'a,b', a, b
    print 'theta', theta

    x = np.array([s.getXAstrom() for s in sources])
    y = np.array([s.getYAstrom() for s in sources])

    ca = plt.gca()
    print 'x, y, a, b, theta:'
    for xi,yi,ai,bi,ti in zip(x, y, a, b, theta):
        print '  x,y', (xi, yi), 'a,b', (ai,bi), 'theta', ti
        ca.add_artist(Ellipse([xi,yi], 2.*ai, 2.*bi, angle=ti,
                              ec='r', fc='none'))
        tirad = np.deg2rad(ti)
        plt.plot([xi, xi + ai * np.cos(tirad)],
                 [yi, yi + ai * np.sin(tirad)], 'r-')

    for (x0,y0,x1,y1) in bb:
        plt.plot([x0,x0,x1,x1,x0], [y0,y1,y1,y0,y0], 'b-')

    plt.axis(ax)
    if fn is not None:
        plt.savefig(fn)
