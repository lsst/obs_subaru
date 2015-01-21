#ifndef LEAST_SQUARE_H
#define LEAST_SQUARE_H

//#if __cplusplus
//extern "C" {
//#endif
    void    F_InvM(int MNUM,double **Min,double **Mout);
    void    F_LS1(int dataNUM,int Order,double **data,double *Coef);
    void    F_LS2(int dataNUM,int Order,double **data,double *Coef);
    void    F_LS3(int dataNUM,int Order,double **data,double *Coef);

//#if __cplusplus
//}
//#endif

#endif


