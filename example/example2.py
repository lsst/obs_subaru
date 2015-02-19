#!/usr/bin/env python

import hsc.meas.match.distest as distest

elevation = 30.0 

x_dist = 8000.0 # pix from the fov center
y_dist = 15000.0 # pix from the fov center

x_undist, y_undist = distest.getUndistortedPosition(x_dist, y_dist, elevation)

print "distorted position: ( %lf , %lf )" % (x_dist, y_dist)
print "==>"
print "undistorted position:   ( %lf , %lf )" % (x_undist, y_undist)
print ""

print " ---- "

x_dist, y_dist = distest.getDistortedPosition(x_undist, y_undist, elevation)
x_dist_iter, y_dist_iter = distest.getDistortedPositionIterative(x_undist, y_undist, elevation)
print ""
print "undistorted position: ( %lf , %lf )" % (x_undist, y_undist)
print "==>"
print "distorted position:   ( %lf , %lf )" % (x_dist, y_dist)
print "distorted(iter) pos:  ( %lf , %lf )" % (x_dist_iter, y_dist_iter)

