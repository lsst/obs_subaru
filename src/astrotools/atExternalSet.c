#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "atConversions.h"

/* atExternalSet.c 
   set the VALUES of all the external variables that are in
   atSurveyGeometry.h and atConversions.h */


/* To add a NEW variable, you have to put it in three places:
   
   1 -- a "const double at_variableName = 1.2345" statement in the 
              first half of this file;

   2 -- a "SETVAR(at_variableName)" statement near the bottom of this file;

   3 -- a "extern const double at_variableName" statement in a .h file
              which is included in one of the next lines.
*/

/* 
 * Hours <--> Radians 
 */
const double at_hrs2Rad = M_PI/12.0; 
const double at_rad2Hrs = 12.0/M_PI; 

/* 
 * Time minutes <--> Radians
 */
const double at_tmin2Rad = M_PI/(12.0 * 60.0); 
const double at_rad2Tmin = (12.0 * 60.0)/M_PI;

/*
 * Time seconds <--> Radians 
 */
const double at_tsec2Rad = M_PI/(12.0 * 3600.0);	
const double at_rad2Tsec = (12.0 * 3600.0)/M_PI;

/* 
 * Degrees <--> Radians 
 */
const double at_deg2Rad = M_PI/180.0; 
const double at_rad2Deg = 180.0/M_PI;

/* 
 * Arc Minutes <--> Radians 
 */
const double at_amin2Rad = M_PI/(180.0 * 60.0); 
const double at_rad2Amin = (180.0 * 60.0)/M_PI;

/* 
 * Arc Seconds <--> Radians 
 */
const double at_asec2Rad = M_PI/(180.0 * 3600.0); 
const double at_rad2Asec = (180.0 * 3600.0)/M_PI;

/* 
 * Hours <--> Degrees 
 */
const double at_hrs2Deg = 180.0 / 12.0; 
const double at_deg2Hrs = 12.0/ 180.0; 

/* 
 * Time minutes <--> Degrees 
 */
const double at_tmin2Deg = 180.0 /(12.0 * 60.0); 
const double at_deg2Tmin = (12.0 * 60.0)/ 180.0;

/* 
 * Time seconds <--> Degrees 
 */
const double at_tsec2Deg = 180.0 / (12.0 * 3600.0);	
const double at_deg2Tsec = (12.0 * 3600.0)/ 180.0;

/* 
 * Arc Minutes <--> Degrees 
 */
const double at_amin2Deg = 1.0 / 60.0;
const double at_deg2Amin = 60.0;

/* 
 * Arc Seconds <--> Degrees 
 */
const double at_asec2Deg = 1.0 / 3600.0; 
const double at_deg2Asec = 3600.0;

/* </PRE>
  </HTML>*/

