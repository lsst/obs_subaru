//--------------------------------------------------
//distest2.h
//
//Last Update 2013/03/15
//--------------------------------------------------
#ifndef LSST_OBS_HSC_DISTEST2_H
#define LSST_OBS_HSC_DISTEST2_H
#include<vector>
#include<iostream>

namespace lsst {
namespace obs {
namespace hsc {

std::vector< double > MAKE_Vdouble();
std::vector< std::vector< double > > MAKE_VVdouble();
std::vector< std::vector< double >  > CALC_RADEC(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION);
std::vector< std::vector< double >  > CALC_GLOBL(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION);
std::vector< std::vector< double >  > CALC_RADEC_SIM(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION);
std::vector< std::vector< double >  > CALC_GLOBL_SIM(std::vector< double > CRVAL,std::vector< double > CRPIX,std::vector< std::vector< double >  > POSITION);

}}} // namespace lsst::obs::hsc

#endif
