import pylab as plt
import numpy as np
from matplotlib.patches import Ellipse
from utils import *

def ahmplots((args, kwargs, fnpat)):
    ahmplot(fnpat, *args, **kwargs)


def save(fn):
    plt.gca().set_position([0.01, 0.01, 0.98, 0.98])
    plt.xticks([])
    plt.yticks([])
    plt.savefig(fn)
    print 'wrote', fn

def ahmplot(fnpat, parent, kids, dkids, sigma1,
            plotb=False, idmask=None, ellipses=True,
            arcsinh=True, maskbit=None, vmin=None, vmax=None):
    if idmask is None:
        idmask = ~0L
    pim = parent.im
    pext = parent.ext

    N = 1 + len(kids)
    S = math.ceil(math.sqrt(N))
    C = S
    R = math.ceil(float(N) / C)
    def nlmap(X):
        return np.arcsinh(X / (3.*sigma1))
    def myimshow(im, **kwargs):
        arcsinh = kwargs.pop('arcsinh', True)
        if arcsinh:
            kwargs = kwargs.copy()
            mn = kwargs.get('vmin', -5*sigma1)
            kwargs['vmin'] = nlmap(mn)
            mx = kwargs.get('vmax', 100*sigma1)
            kwargs['vmax'] = nlmap(mx)
            plt.imshow(nlmap(im), **kwargs)
        else:
            plt.imshow(im, **kwargs)

    if vmax is None:
        vmax=pim.max()
    imargs = dict(interpolation='nearest', origin='lower',
                  vmax=vmax, arcsinh=arcsinh)
    if maskbit:
        imargs.update(vmin=0)

    plt.figure(figsize=(4,4))
    plt.clf()
    myimshow(pim, extent=pext, **imargs)
    plt.gray()
    m = 0.25
    pax = [pext[0]-m, pext[1]+m, pext[2]-m, pext[3]+m]
    x,y = parent.pix[0], parent.piy[0]
    #tt = 'parent %i @ (%i,%i)' % (parent.sid & idmask,
    #                              x - parent.x0, y - parent.y0)
    #if len(parent.flags):
    #    tt += ', ' + ', '.join(parent.flags)
    #plt.title(tt)

    #sty = dict(color='r', ms=8, mew=1.5)
    sty = dict(color='r', ms=15, mew=3)

    for i,kid in enumerate(kids):
        # peak(s)
        plt.plot(kid.pfx, kid.pfy, '+', **sty)

    #plt.plot([parent.cx], [parent.cy], 'x', color='b')
    plt.axis(pax)
    save(fnpat % 'p')

    for i,kid in enumerate(kids):
        ext = kid.ext
        if plotb:
            ima = imargs.copy()
            ima.update(vmax=max(3.*sigma1, kid.im.max()))
        else:
            ima = imargs

        plt.clf()
        myimshow(kid.im, extent=ext, **ima)
        plt.gray()

        # peak(s)
        plt.plot(kid.pfx, kid.pfy, '+', **sty)

        if plotb:
            plt.axis(ext)
        else:
            plt.axis(pax)

        save(fnpat % ('c%i' % i))

        plt.clf()
        myimshow(kid.im, extent=ext, **ima)
        plt.gray()
        # centroid
        #plt.plot([kid.cx], [kid.cy], '+', **sty)
        # ellipse
        if ellipses and not kid.ispsf:
            drawEllipses(kid, ec='r', fc='none', lw=3)
            plt.plot([kid.getX()], [kid.getY()], '+', **sty)
        else:
            plt.plot([kid.cx], [kid.cy], '+', **sty)

        if plotb:
            plt.axis(ext)
        else:
            plt.axis(pax)

        save(fnpat % ('c%im' % i))
