#ifndef ATCONVERSIONS_H
#define ATCONVERSIONS_H

#ifdef __cplusplus
extern "C" {
#endif
//dstn
//#include <smaValues.h>   /* This is where we get the value M_PI */
#include <math.h>

  /* NOTE:  these values are SET in atExternalSet.c */

  /*To convert from a number, multiply by one of the following constants.
    For example:   3.14159 * rad2deg yields 180.0
    
    rad - radians
    
    hrs - hours
    tmin - time minutes
    tsec - time seconds
    
    deg - degrees
    amin - arc minutes
    asec - arc seconds
    
    */
  
  /* Hours <--> Radians */
  extern const double at_hrs2Rad;
  extern const double at_rad2Hrs;
  
  /* Time minutes <--> Radians */
  extern const double at_tmin2Rad;
  extern const double at_rad2Tmin;
  
  /* Time seconds <--> Radians */
  extern const double at_tsec2Rad;
  extern const double at_rad2Tsec;
  
  /* Degrees <--> Radians */
  extern const double at_deg2Rad;
  extern const double at_rad2Deg;
    
  /* Arc Minutes <--> Radians */
  extern const double at_amin2Rad;
  extern const double at_rad2Amin;
	
  /* Arc Seconds <--> Radians */
  extern const double at_asec2Rad;
  extern const double at_rad2Asec;
	    


  /* Hours <--> Degrees */
  extern const double at_hrs2Deg;
  extern const double at_deg2Hrs;
  
  /* Time minutes <--> Degrees */
  extern const double at_tmin2Deg;
  extern const double at_deg2Tmin;
  
  /* Time seconds <--> Degrees */
  extern const double at_tsec2Deg;
  extern const double at_deg2Tsec;
  
  /* Arc Minutes <--> Degrees */
  extern const double at_amin2Deg;
  extern const double at_deg2Amin;
	
  /* Arc Seconds <--> Degrees */
  extern const double at_asec2Deg;
  extern const double at_deg2Asec;
	    
#ifdef __cplusplus
}
#endif

#endif

