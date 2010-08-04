#!/usr/bin/env python

import hsc.meas.match.distest as distest

elevation = 30.0 

x_undist = 10000.0 # pix from the fov center
y_undist = 10000.0 # pix from the fov center

x_dist, y_dist = distest.getDistortedPosition(x_undist, y_undist, elevation)

print "undistored position: ( %lf , %lf )" % (x_undist, y_undist)
print "==>"
print "distored position:   ( %lf , %lf )" % (x_dist, y_dist)

print " ---- "

x_undist, y_undist = distest.getUndistortedPosition(x_dist, y_dist, elevation)

print "distored position:   ( %lf , %lf )" % (x_dist, y_dist)
print "==>"
print "undistored position: ( %lf , %lf )" % (x_undist, y_undist)

