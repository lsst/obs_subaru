#ifndef LSST_OBS_HSC_LEAST_SQUARE_H
#define LSST_OBS_HSC_LEAST_SQUARE_H

namespace lsst {
namespace obs {
namespace hsc {

    void    F_InvM(int MNUM,double **Min,double **Mout);
    void    F_LS1(int dataNUM,int Order,double **data,double *Coef);
    void    F_LS2(int dataNUM,int Order,double **data,double *Coef);
    void    F_LS3(int dataNUM,int Order,double **data,double *Coef);

}}} // namespace lsst::obs::hsc

#endif


