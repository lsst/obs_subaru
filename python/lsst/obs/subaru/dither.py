import lsst.geom as geom

try:
    pyplot
except NameError:
    import matplotlib.pyplot as pyplot
    pyplot.interactive(1)


def ditherDES(camera, scale=4.5, names=False):
    positions = [
        (0.0, 0.0),
        (65.0, 100.0),
        (57.0, -99.0),
        (-230.0, -193.0),
        (-136.0, 267.0),
        (-430.0, 417.0),
        (553.0, 233.0),
        (628.0, 940.0),
        (621.0, -946.0),
        (-857.0, 1425.0),
        (10.0, 5.0),
        (1647.0, 231.0),
        (1305.0, -1764.0),
        (-2134.0, -511.0),
        (2700.0, 370.0),
        (-714.0, -2630.0),
        (2339.0, -2267.0),
        (-3001.0, -1267.0),
        (-152.0, -3785.0),
        (-3425.0, 1619.0),
        (1862.0, 3038.0),
        (5.0, 10.0),
    ]

    xdither, ydither = [x*scale for x, y in positions], [y*scale for x, y in positions]
    x0, y0 = xdither[0], ydither[0]
    for x, y in zip(xdither, ydither):
        print("%8.1f %8.1f   %8.1f %8.1f" % (x, y, x - x0, y - y0))
        x0, y0 = x, y

    pyplot.clf()
    pyplot.plot(xdither, ydither, "o")
    pyplot.axes().set_aspect('equal')

    for raft in camera:
        for ccd in raft:
            ccd.setTrimmed(True)

            width, height = ccd.getAllPixels(True).getDimensions()

            corners = ((0.0, 0.0), (0.0, height), (width, height), (width, 0.0), (0.0, 0.0))
            for (x0, y0), (x1, y1) in zip(corners[0:4], corners[1:5]):
                if x0 == x1 and y0 != y1:
                    yList = [y0, y1]
                    xList = [x0]*len(yList)
                elif y0 == y1 and x0 != x1:
                    xList = [x0, x1]
                    yList = [y0]*len(xList)
                else:
                    raise RuntimeError("Should never get here")

                xOriginal = []
                yOriginal = []
                for x, y in zip(xList, yList):
                    position = ccd.getPositionFromPixel(geom.Point2D(x, y))  # focal plane position

                    xOriginal.append(position.getMm().getX())
                    yOriginal.append(position.getMm().getY())

                pyplot.plot(xOriginal, yOriginal, 'k-')

            x, y = ccd.getPositionFromPixel(geom.Point2D(width/2, height/2)).getMm()
            cid = ccd.getId()
            if names:
                pyplot.text(x, y, cid.getName(), ha='center', rotation=90 if height > width else 0,
                            fontsize="smaller")
            else:
                pyplot.text(x, y, cid.getSerial(), ha='center', va='center')


if __name__ == "__main__":
    ditherDES()
    input("Exit? ")
