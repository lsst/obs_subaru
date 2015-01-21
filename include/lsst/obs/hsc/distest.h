#ifndef LSST_OBS_HSC_DISTEST_H
#define LSST_OBS_HSC_DISTEST_H

namespace lsst {
namespace obs {
namespace hsc {

const int NSAMPLE_EL = 12;              // original 12

	void getDistortedPosition(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation);
	void getDistortedPositionIterative(float x_undist, float y_undist, float* x_dist, float* y_dist, float elevation);
	void getUndistortedPosition(float x_dist, float y_dist, float* x_undist, float* y_undist, float elevation);

}}} // namespace lsst::obs::hsc

#endif
