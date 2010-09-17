#!/usr/bin/env python
#

import sys, os, re
import lsst.afw.image        as afwImage

# HSC does not use bias frames, but the LSST pipe does.
# Use this to write a fits file with an appropriate name (first arg you pass in)
# look in policy/SuprimeMapperDictionary.paf for the mapping of 'bias' 
def main(zerofile):

    #nx = 2272 # raw
    #ny = 4273 # raw
    nx = 2048 # trim
    ny = 4177 # trim
    
    img = afwImage.MaskedImageF(nx, ny)
    img.set(0, 0x0, 0)
    img.writeFits(zerofile)
    
    print "Writing ", zerofile

if __name__ == '__main__':
    main(sys.argv[1])
