import matplotlib
matplotlib.use('Agg')
import pylab as plt
import numpy as np


import lsst.afw.table as afwTable
# Jim B says, to get FootprintPtr definition:
import lsst.afw.detection as afwDet
import lsst.afw.image as afwImage

srcs = afwTable.SourceCatalog.readFits('deblended.fits')
print len(srcs), 'sources'

cal = afwImage.ExposureF('calexp.fits')
print 'Read cal', cal

mim = cal.getMaskedImage()
img = mim.getImage()

var = mim.getVariance()
mvar = np.median(var.getArray()[::10, ::10].ravel())
sig1 = np.sqrt(mvar)

pidmap = {}
sibmap = {}
kids = []
for src in srcs:
    pid = src.getParent()
    if pid:
        kids.append(src)
        if not pid in sibmap:
            sibmap[pid] = []
        sibmap[pid].append(src)
    else:
        pidmap[src.getId()] = src

print len(kids), 'deblended children'

print len(sibmap), 'families'
print len(pidmap), 'top-level sources'

pids = sibmap.keys()
pids.sort()

for i,pid in enumerate(pids):
    parent = pidmap[pid]
    kids = sibmap[pid]
    print 'parent', parent
    print 'kids:', len(kids)
    for kid in kids:
        print '  ', kid

    pfp = parent.getFootprint()
    print 'Parent footprint:', pfp
    print ' is heavy?', pfp.isHeavy()

    pheavy = afwDet.makeHeavyFootprint(pfp, mim)
    pbb = pfp.getBBox()
    pim = afwImage.ImageF(pbb)
    pheavy.insert(pim)

    nk = len(kids)
    N = nk + 2
    C = int(np.ceil(np.sqrt(N)))
    R = int(np.ceil(N / float(C)))

    def bbExt(bbox):
        return [bbox.getMinX(), bbox.getMaxX(),
                bbox.getMinY(), bbox.getMaxY()]
    def imExt(img):
        bbox = img.getBBox(afwImage.PARENT)
        return bbExt(bbox)

    # globals used in 'myimshow'
    mx = pim.getArray().max()
    ax = imExt(pim)

    def myimshow(im):
        plt.imshow(im.getArray(),
                   extent=imExt(im),
                   interpolation='nearest', origin='lower',
                   vmin=-2.*sig1, vmax=5.*sig1,
                   cmap='gray')
        # vmin=0, vmax=mx, 
        plt.xticks([]); plt.yticks([])
        plt.axis(ax)
        

    Xpsf,Ypsf = [],[]
    Xext,Yext = [],[]
    X,Y = [],[]

    psfkey = srcs.getSchema().find('deblend.deblended-as-psf').key
    for kid in kids:
        pks = kid.getFootprint().getPeaks()
        xx = [pk.getIx() for pk in pks]
        yy = [pk.getIy() for pk in pks]
        if kid.get(psfkey):
            Xpsf.extend(xx)
            Ypsf.extend(yy)
        else:
            Xext.extend(xx)
            Yext.extend(yy)
        X.extend(xx)
        Y.extend(yy)

    esty = dict(linestyle='None', marker='o', mec='r', mfc='none')
    psty = dict(linestyle='None', marker='.', color='r')
    
    plt.clf()
    plt.subplot(R,C, 1)
    myimshow(pim)
    plt.plot(Xpsf, Ypsf, **psty)
    plt.plot(Xext, Yext,  **esty)
    plt.axis(ax)
    plt.title('pid %i' % pid)

    ksum = afwImage.ImageF(pbb)
    
    for j,kid in enumerate(kids):
        kheavy = kid.getFootprint()
        assert(kheavy.isHeavy())
        kheavy = afwDet.cast_HeavyFootprintF(kheavy)
        kbb = kheavy.getBBox()
        kim = afwImage.ImageF(kbb)
        kheavy.insert(kim)

        # add to accumulator
        kthis = afwImage.ImageF(pbb)
        kheavy.insert(kthis)
        ksum += kthis
        
        plt.subplot(R, C, j+3)
        myimshow(kim)
        if kid.get(psfkey):
            plt.plot(X[j], Y[j], **psty)
        else:
            plt.plot(X[j], Y[j], **esty)
        plt.axis(ax)
        plt.title('kid %i' % kid.getId())

    plt.subplot(R,C, 2)
    myimshow(ksum)
    plt.plot(Xpsf, Ypsf, **psty)
    plt.plot(Xext, Yext, **esty)

    plt.axis(ax)
    plt.title('sum of kids')

    plt.savefig('deb-%04i.png' % i)
    
    if i == 100:
        break
    
