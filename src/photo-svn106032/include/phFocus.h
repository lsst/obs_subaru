#if !defined(PHFOCUS_H)
#define PHFOCUS_H
 
struct gausmom_P {
   float g_xmom;			/* normalized moment of 2(x/sig)^2-1 */
   float g_ymom;			/* normalized moment of 2(y/sig)^2-1 */
   float g_filval;			/* value of filter */
   float g_xf;				/* float coordinates; column */
   float g_yf;				/* float coordinates; row */
};
 
double phGaussianWidthEstimate(const REGION *reg, int rowc, int colc,
			       int sky, struct gausmom_P *ps);
 
#endif


