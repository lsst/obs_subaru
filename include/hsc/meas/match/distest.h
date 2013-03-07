#ifndef DISTEST_H
#define DISTEST_H

const int NSAMPLE_EL = 12;              // original 12

//extern "C" {

namespace hsc {
 namespace meas {
  namespace match {
	void getDistortedPosition(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation);
	void getDistortedPositionIterative(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation);
	void getUndistortedPosition(float x_dist, float y_dist, float* x_undist, float* y_undist, float elevation);
	void getDistortedPosition_HSCSIM(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation);
	void getDistortedPositionIterative_HSCSIM(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation);
	void getUndistortedPosition_HSCSIM(float x_dist, float y_dist, float* x_undist, float* y_undist, float elevation);
  }
 }
}

//}
#endif
