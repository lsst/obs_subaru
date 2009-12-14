#if !defined(PHMEASUREOBJ_P_H)
#define PHMEASUREOBJ_P_H
/*
 * External symbols in measure objects that are not publically visible
 *
 * Note that this file is expected to be included by phMeasureObj.h
 * rather than by itself
 */
void p_phInitMeasureObj(int ncolor, RANDOM *rand);
void p_phFiniMeasureObj(void);
int p_phSetParamsRun(const FIELDPARAMS *fparams);
float p_phSetParamsFrame(int color,
			 FIELDPARAMS *fiparams,
			 FIELDSTAT *fieldstat);
void p_phUnsetParamsFrame(int color,
			  FIELDPARAMS *fiparams);

#endif
