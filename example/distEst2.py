#!/usr/bin/env python

import math
import hsc.meas.match.distest as distest

def main():
	CRPIX=distest.MAKE_Vdouble();
	CRVAL=distest.MAKE_Vdouble();
	XY=distest.MAKE_Vdouble();
	XY_GLOBL=distest.MAKE_VVdouble();
	XY_RADEC=distest.MAKE_VVdouble();
	CRPIX[0]=100.0
	CRPIX[1]= 10.0
	CRVAL[0]=  0.0
	CRVAL[1]=  0.0
	for x in range(-20,21):
		for y in range(-20,21):
			if(math.hypot(1000*x,1000*y)<17501):
				XY=[1000*x,1000*y]
				XY_GLOBL.append(XY)
	for x in range(-20,21):
		for y in range(-20,21):
			if(math.hypot(0.05*x,0.05*y)<0.751):
				XY=[0.05*x,0.05*y]
				XY_RADEC.append(XY)
	
	DIST_RADEC=distest.CALC_RADEC(CRVAL,CRPIX,XY_GLOBL)
	DIST_GLOBL=distest.CALC_GLOBL(CRVAL,CRPIX,XY_RADEC)
	DIST_RADEC_SIM=distest.CALC_RADEC_SIM(CRVAL,CRPIX,XY_GLOBL)
	DIST_GLOBL_SIM=distest.CALC_GLOBL_SIM(CRVAL,CRPIX,XY_RADEC)

	for i in range(len(XY_GLOBL)):
		print XY_GLOBL[i][0],XY_GLOBL[i][1],DIST_RADEC[i][0],DIST_RADEC[i][1],DIST_RADEC_SIM[i][0],DIST_RADEC_SIM[i][1]
#	for i in range(len(XY_RADEC)):
#		print XY_RADEC[i][0],XY_RADEC[i][1],DIST_GLOBL[i][0],DIST_GLOBL[i][1],DIST_GLOBL_SIM[i][0],DIST_GLOBL_SIM[i][1]

if __name__ == '__main__':
	main()
