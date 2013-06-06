import lsst.afw.table as afwTable

srcs = afwTable.SourceCatalog.readFits('deblended.fits')

fp = srcs[0].getFootprint()
print 'fp', fp
print 'heavy?', fp.isHeavy()


print len(srcs), 'sources'

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

for pid in pids:
    parent = pidmap[pid]
    kids = sibmap[pid]
    print 'parent', parent
    print 'kids:', len(kids)
    for kid in kids:
        print '  ', kid

    pfp = parent.getFootprint()
    print 'Parent footprint:', pfp
    print ' is heavy?', pfp.isHeavy()
