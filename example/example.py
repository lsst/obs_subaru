#!/usr/bin/env python

import hsc.meas.match.distest as distest

elevation = 30.0 

x_dist = 10000.0 # pix from the fov center
y_dist = 10000.0 # pix from the fov center

x_undist, y_undist = distest.getDistortedPosition(x_dist, y_dist, elevation)

print "distored position:   ( %lf , %lf )" % (x_dist, y_dist)
print "==>"
print "undistored position: ( %lf , %lf )" % (x_undist, y_undist)

print " ---- "

x_dist, y_dist = distest.getUndistortedPosition(x_undist, y_undist, elevation)

print "undistored position: ( %lf , %lf )" % (x_undist, y_undist)
print "==>"
print "distored position:   ( %lf , %lf )" % (x_dist, y_dist)

                                        
