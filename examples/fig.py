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
    for kid in kids:
        kfp = kid.getFootprint()
        print '  is kid heavy:', kfp.isHeavy()
        
    pheavy = afwDet.makeHeavyFootprint(pfp, mim)
    pbb = pfp.getBBox()
    pim = afwImage.ImageF(pbb)
    pheavy.insert(pim)

    nk = len(kids)
    N = nk + 1
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
                   vmin=0, vmax=mx, cmap='gray')
        plt.xticks([]); plt.yticks([])
        plt.axis(ax)
        
    plt.clf()
    plt.subplot(R,C, 1)
    myimshow(pim)
    plt.title('pid %i' % pid)

    for j,kid in enumerate(kids):
        kheavy = kid.getFootprint()
        assert(kheavy.isHeavy())
        kheavy = afwDet.cast_HeavyFootprintF(kheavy)
        kbb = kfp.getBBox()
        kim = afwImage.ImageF(kbb)
        print 'kid image bb:', kim.getBBox(afwImage.PARENT)
        print 'kheavy    bb:', kheavy.getBBox()
        kheavy.insert(kim)

        plt.subplot(R, C, j+2)
        myimshow(kim)
        plt.title('kid %i' % kid.getId())
        
    plt.savefig('deb-%04i.png' % i)
    
    break

    
