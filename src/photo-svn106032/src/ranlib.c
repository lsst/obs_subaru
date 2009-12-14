/*
 * These routines are from ranlib and are in the public domain; the
 * URL used was http://www.netlib.no/netlib/random/, but there are
 * other equivalent sites.  The code has been repackaged and edited
 * to remove as much unused code as possible, and to avoid making
 * unnecessary external symbols visible to the linker.
 *
 * To quote the README in the original package (not all of this code
 * in included here):

                               SOURCES

The following routines,  which  were  written by others   and  lightly
modified for consistency in packaging, are included in RANLIB.

                        Bottom Level Routines

These routines are a transliteration of the Pascal in the reference to
Fortran.

L'Ecuyer, P. and  Cote, S. "Implementing  a Random Number Package with
Splitting  Facilities."  ACM  Transactions   on Mathematical Software,
17:98-111 (1991)


                             Exponential

This code was obtained from Netlib.

Ahrens,  J.H. and  Dieter, U.   Computer Methods for Sampling From the
Exponential and Normal  Distributions.  Comm. ACM,  15,10 (Oct. 1972),
873 - 882.

                                Gamma

(Case R >= 1.0)                                          

Ahrens, J.H. and Dieter, U.  Generating Gamma  Variates by  a Modified
Rejection Technique.  Comm. ACM, 25,1 (Jan. 1982), 47 - 54.
Algorithm GD                                                       

(Case 0.0 <= R <= 1.0)                                   

Ahrens, J.H. and Dieter, U.  Computer Methods for Sampling from Gamma,
Beta,  Poisson  and Binomial   Distributions.    Computing, 12 (1974),
223-246.  Adaptation of algorithm GS.

                                Normal

This code was obtained from netlib.

Ahrens, J.H.  and  Dieter, U.    Extensions of   Forsythe's Method for
Random Sampling  from  the Normal Distribution.  Math. Comput., 27,124
(Oct. 1973), 927 - 937.

                               Binomial

This code was kindly sent me by Dr. Kachitvichyanukul.

Kachitvichyanukul,  V. and Schmeiser, B.   W.  Binomial Random Variate
Generation.  Communications of the ACM, 31, 2 (February, 1988) 216.

                               Poisson

This code was obtained from netlib.

Ahrens,  J.H. and Dieter, U.   Computer Generation of Poisson Deviates
From Modified  Normal Distributions.  ACM Trans.  Math. Software, 8, 2
(June 1982),163-179


                                 Beta

This code was written by us following the recipe in the following.

R. C.  H.   Cheng Generating  Beta Variables  with  Nonintegral  Shape
Parameters. Communications of  the ACM,  21:317-322 (1978) (Algorithms
BB and BC)

                               Linpack

Routines SPOFA and SDOT are used to perform the Cholesky decomposition
of  the covariance  matrix  in  SETGMN  (used  for  the  generation of
multivariate normal deviates).

Dongarra, J.  J., Moler,   C.  B., Bunch, J.   R. and  Stewart, G.  W.
Linpack User's Guide.  SIAM Press, Philadelphia.  (1979)



                              LEGALITIES

Code that appeared  in an    ACM  publication  is subject  to    their
algorithms policy:

     Submittal of  an  algorithm    for publication  in   one of   the  ACM
     Transactions implies that unrestricted use  of the algorithm within  a
     computer is permissible.   General permission  to copy and  distribute
     the algorithm without fee is granted provided that the copies  are not
     made  or   distributed for  direct   commercial  advantage.    The ACM
     copyright notice and the title of the publication and its date appear,
     and  notice is given that copying  is by permission of the Association
     for Computing Machinery.  To copy otherwise, or to republish, requires
     a fee and/or specific permission.

     Krogh, F.  Algorithms  Policy.  ACM  Tran.   Math.  Softw.   13(1987),
     183-186.

We place the Ranlib code that we have written in the public domain.  

                                 NO WARRANTY
     
     WE PROVIDE ABSOLUTELY  NO WARRANTY  OF ANY  KIND  EITHER  EXPRESSED OR
     IMPLIED,  INCLUDING BUT   NOT LIMITED TO,  THE  IMPLIED  WARRANTIES OF
     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK
     AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS  WITH YOU.  SHOULD
     THIS PROGRAM PROVE  DEFECTIVE, YOU ASSUME  THE COST  OF  ALL NECESSARY
     SERVICING, REPAIR OR CORRECTION.
     
     IN NO  EVENT  SHALL THE UNIVERSITY  OF TEXAS OR  ANY  OF ITS COMPONENT
     INSTITUTIONS INCLUDING M. D.   ANDERSON HOSPITAL BE LIABLE  TO YOU FOR
     DAMAGES, INCLUDING ANY  LOST PROFITS, LOST MONIES,   OR OTHER SPECIAL,
     INCIDENTAL   OR  CONSEQUENTIAL DAMAGES   ARISING   OUT  OF  THE USE OR
     INABILITY TO USE (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA OR
     ITS ANALYSIS BEING  RENDERED INACCURATE OR  LOSSES SUSTAINED  BY THIRD
     PARTIES) THE PROGRAM.
     
     (Above NO WARRANTY modified from the GNU NO WARRANTY statement.)
 
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "phRanlib.h"


static void setall(long iseed1,long iseed2);
static void inrgcm(void);
static void gscgn(long getset,long *g);
static void gsrgs(long getset,long *qvalue);
static void gssst(long getset,long *qset);
static long mltmod(long a,long s,long m);
static float ranf(void);
static float sexpo(void);
static float snorm(void);

static long Xm1,Xm2,Xa1,Xa2,Xcg1[32],Xcg2[32],Xa1w,Xa2w,
   Xig1[32],Xig2[32],Xlg1[32], Xlg2[32],Xa1vw,Xa2vw;
static long Xqanti[32];
#define numg 32L

static long
ignlgi(void)
/*
**********************************************************************
     long ignlgi(void)
               GeNerate LarGe Integer
     Returns a random integer following a uniform distribution over
     (1, 2147483562) using the current generator.
     This is a transcription from Pascal to Fortran of routine
     Random from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)
**********************************************************************
*/
{
   static long ignlgi,curntg,k,s1,s2,z;
   static long qqssd,qrgnin;
/*
     IF THE RANDOM NUMBER PACKAGE HAS NOT BEEN INITIALIZED YET, DO SO.
     IT CAN BE INITIALIZED IN ONE OF TWO WAYS : 1) THE FIRST CALL TO
     THIS ROUTINE  2) A CALL TO SETALL.
*/
    gsrgs(0L,&qrgnin);
    if(!qrgnin) inrgcm();
    gssst(0,&qqssd);
    if(!qqssd) setall(1234567890L,123456789L);
/*
     Get Current Generator
*/
    gscgn(0L,&curntg);
    s1 = *(Xcg1+curntg-1);
    s2 = *(Xcg2+curntg-1);
    k = s1/53668L;
    s1 = Xa1*(s1-k*53668L)-k*12211;
    if(s1 < 0) s1 += Xm1;
    k = s2/52774L;
    s2 = Xa2*(s2-k*52774L)-k*3791;
    if(s2 < 0) s2 += Xm2;
    *(Xcg1+curntg-1) = s1;
    *(Xcg2+curntg-1) = s2;
    z = s1-s2;
    if(z < 1) z += (Xm1-1);
    if(*(Xqanti+curntg-1)) z = Xm1-z;
    ignlgi = z;
    return ignlgi;
}
static void
initgn(long isdtyp)
/*
**********************************************************************
     void initgn(long isdtyp)
          INIT-ialize current G-e-N-erator
     Reinitializes the state of the current generator
     This is a transcription from Pascal to Fortran of routine
     Init_Generator from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)
                              Arguments
     isdtyp -> The state to which the generator is to be set
          isdtyp = -1  => sets the seeds to their initial value
          isdtyp =  0  => sets the seeds to the first value of
                          the current block
          isdtyp =  1  => sets the seeds to the first value of
                          the next block
**********************************************************************
*/
{
static long g;
static long qrgnin;
/*
     Abort unless random number generator initialized
*/
    gsrgs(0L,&qrgnin);
    if(qrgnin) goto S10;
    fprintf(stderr,"%s\n",
      " INITGN called before random number generator  initialized -- abort!");
    exit(1);
S10:
    gscgn(0L,&g);
    if(-1 != isdtyp) goto S20;
    *(Xlg1+g-1) = *(Xig1+g-1);
    *(Xlg2+g-1) = *(Xig2+g-1);
    goto S50;
S20:
    if(0 != isdtyp) goto S30;
    goto S50;
S30:
/*
     do nothing
*/
    if(1 != isdtyp) goto S40;
    *(Xlg1+g-1) = mltmod(Xa1w,*(Xlg1+g-1),Xm1);
    *(Xlg2+g-1) = mltmod(Xa2w,*(Xlg2+g-1),Xm2);
    goto S50;
S40:
    fprintf(stderr,"%s\n","isdtyp not in range in INITGN");
    exit(1);
S50:
    *(Xcg1+g-1) = *(Xlg1+g-1);
    *(Xcg2+g-1) = *(Xlg2+g-1);
}

static void
inrgcm(void)
/*
**********************************************************************
     void inrgcm(void)
          INitialize Random number Generator CoMmon
                              Function
     Initializes common area  for random number  generator.  This saves
     the  nuisance  of  a  BLOCK DATA  routine  and the  difficulty  of
     assuring that the routine is loaded with the other routines.
**********************************************************************
*/
{
static long T1;
static long i;
/*
     V=20;                            W=30;
     A1W = MOD(A1**(2**W),M1)         A2W = MOD(A2**(2**W),M2)
     A1VW = MOD(A1**(2**(V+W)),M1)    A2VW = MOD(A2**(2**(V+W)),M2)
   If V or W is changed A1W, A2W, A1VW, and A2VW need to be recomputed.
    An efficient way to precompute a**(2*j) MOD m is to start with
    a and square it j times modulo m using the function MLTMOD.
*/
    Xm1 = 2147483563L;
    Xm2 = 2147483399L;
    Xa1 = 40014L;
    Xa2 = 40692L;
    Xa1w = 1033780774L;
    Xa2w = 1494757890L;
    Xa1vw = 2082007225L;
    Xa2vw = 784306273L;
    for(i=0; i<numg; i++) *(Xqanti+i) = 0;
    T1 = 1;
/*
     Tell the world that common has been initialized
*/
    gsrgs(1L,&T1);
}

static void
setall(long iseed1,long iseed2)
/*
**********************************************************************
     void setall(long iseed1,long iseed2)
               SET ALL random number generators
     Sets the initial seed of generator 1 to ISEED1 and ISEED2. The
     initial seeds of the other generators are set accordingly, and
     all generators states are set to these seeds.
     This is a transcription from Pascal to Fortran of routine
     Set_Initial_Seed from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)
                              Arguments
     iseed1 -> First of two integer seeds
     iseed2 -> Second of two integer seeds
**********************************************************************
*/
{
static long T1;
static long g,ocgn;
static long qrgnin;
    T1 = 1;
/*
     TELL IGNLGI, THE ACTUAL NUMBER GENERATOR, THAT THIS ROUTINE
      HAS BEEN CALLED.
*/
    gssst(1,&T1);
    gscgn(0L,&ocgn);
/*
     Initialize Common Block if Necessary
*/
    gsrgs(0L,&qrgnin);
    if(!qrgnin) inrgcm();
    *Xig1 = iseed1;
    *Xig2 = iseed2;
    initgn(-1L);
    for(g=2; g<=numg; g++) {
        *(Xig1+g-1) = mltmod(Xa1vw,*(Xig1+g-2),Xm1);
        *(Xig2+g-1) = mltmod(Xa2vw,*(Xig2+g-2),Xm2);
        gscgn(1L,&g);
        initgn(-1L);
    }
    gscgn(1L,&ocgn);
}

/*****************************************************************************/

#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))

static void
gscgn(long getset,long *g)
/*
**********************************************************************
     void gscgn(long getset,long *g)
                         Get/Set GeNerator
     Gets or returns in G the number of the current generator
                              Arguments
     getset --> 0 Get
                1 Set
     g <-- Number of the current random number generator (1..32)
**********************************************************************
*/
{
static long curntg = 1;
    if(getset == 0) *g = curntg;
    else  {
        if(*g < 0 || *g > numg) {
            fputs(" Generator number out of range in GSCGN",stderr);
            exit(0);
        }
        curntg = *g;
    }
}
static void
gsrgs(long getset,long *qvalue)
/*
**********************************************************************
     void gsrgs(long getset,long *qvalue)
               Get/Set Random Generators Set
     Gets or sets whether random generators set (initialized).
     Initially (data statement) state is not set
     If getset is 1 state is set to qvalue
     If getset is 0 state returned in qvalue
**********************************************************************
*/
{
static long qinit = 0;

    if(getset == 0) *qvalue = qinit;
    else qinit = *qvalue;
}
static void
gssst(long getset,long *qset)
/*
**********************************************************************
     void gssst(long getset,long *qset)
          Get or Set whether Seed is Set
     Initialize to Seed not Set
     If getset is 1 sets state to Seed Set
     If getset is 0 returns T in qset if Seed Set
     Else returns F in qset
**********************************************************************
*/
{
static long qstate = 0;
    if(getset != 0) qstate = 1;
    else  *qset = qstate;
}

static long
lennob( char *str )
/* 
Returns the length of str ignoring trailing blanks but not 
other white space.
*/
{
   long i, i_nb;

   for (i=0, i_nb= -1L; *(str+i); i++)
     if ( *(str+i) != ' ' ) i_nb = i;
   return (i_nb+1);
}

static long
mltmod(long a,long s,long m)
/*
**********************************************************************
     long mltmod(long a,long s,long m)
                    Returns (A*S) MOD M
     This is a transcription from Pascal to Fortran of routine
     MULtMod_Decompos from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)
                              Arguments
     a, s, m  -->
**********************************************************************
*/
{
#define h 32768L
static long mltmod,a0,a1,k,p,q,qh,rh;
/*
     H = 2**((b-2)/2) where b = 32 because we are using a 32 bit
      machine. On a different machine recompute H
*/
    if(!(a <= 0 || a >= m || s <= 0 || s >= m)) goto S10;
    fputs(" a, m, s out of order in mltmod - ABORT!",stderr);
    fprintf(stderr," a = %12ld s = %12ld m = %12ld\n",a,s,m);
    fputs(" mltmod requires: 0 < a < m; 0 < s < m",stderr);
    exit(1);
S10:
    if(!(a < h)) goto S20;
    a0 = a;
    p = 0;
    goto S120;
S20:
    a1 = a/h;
    a0 = a-h*a1;
    qh = m/h;
    rh = m-h*qh;
    if(!(a1 >= h)) goto S50;
    a1 -= h;
    k = s/qh;
    p = h*(s-k*qh)-k*rh;
S30:
    if(!(p < 0)) goto S40;
    p += m;
    goto S30;
S40:
    goto S60;
S50:
    p = 0;
S60:
/*
     P = (A2*S*H)MOD M
*/
    if(!(a1 != 0)) goto S90;
    q = m/a1;
    k = s/q;
    p -= (k*(m-a1*q));
    if(p > 0) p -= m;
    p += (a1*(s-k*q));
S70:
    if(!(p < 0)) goto S80;
    p += m;
    goto S70;
S90:
S80:
    k = p/qh;
/*
     P = ((A2*H + A1)*S)MOD M
*/
    p = h*(p-k*qh)-k*rh;
S100:
    if(!(p < 0)) goto S110;
    p += m;
    goto S100;
S120:
S110:
    if(!(a0 != 0)) goto S150;
/*
     P = ((A2*H + A1)*H*S)MOD M
*/
    q = m/a0;
    k = s/q;
    p -= (k*(m-a0*q));
    if(p > 0) p -= m;
    p += (a0*(s-k*q));
S130:
    if(!(p < 0)) goto S140;
    p += m;
    goto S130;
S150:
S140:
    mltmod = p;
    return mltmod;
}
#undef h

static float
ranf(void)
/*
**********************************************************************
     float ranf(void)
                RANDom number generator as a Function
     Returns a random floating point number from a uniform distribution
     over 0 - 1 (endpoints of this interval are not returned) using the
     current generator
     This is a transcription from Pascal to Fortran of routine
     Uniform_01 from the paper
     L'Ecuyer, P. and Cote, S. "Implementing a Random Number Package
     with Splitting Facilities." ACM Transactions on Mathematical
     Software, 17:98-111 (1991)
**********************************************************************
*/
{
static float ranf;
/*
     4.656613057E-10 is 1/M1  M1 is set in a data statement in IGNLGI
      and is currently 2147483563. If M1 changes, change this also.
*/
    ranf = ignlgi()*4.656613057E-10;
    return ranf;
}

static float
sexpo(void)
/*
**********************************************************************
                                                                      
                                                                      
     (STANDARD-)  E X P O N E N T I A L   DISTRIBUTION                
                                                                      
                                                                      
**********************************************************************
**********************************************************************
                                                                      
     FOR DETAILS SEE:                                                 
                                                                      
               AHRENS, J.H. AND DIETER, U.                            
               COMPUTER METHODS FOR SAMPLING FROM THE                 
               EXPONENTIAL AND NORMAL DISTRIBUTIONS.                  
               COMM. ACM, 15,10 (OCT. 1972), 873 - 882.               
                                                                      
     ALL STATEMENT NUMBERS CORRESPOND TO THE STEPS OF ALGORITHM       
     'SA' IN THE ABOVE PAPER (SLIGHTLY MODIFIED IMPLEMENTATION)       
                                                                      
     Modified by Barry W. Brown, Feb 3, 1988 to use RANF instead of   
     SUNIF.  The argument IR thus goes away.                          
                                                                      
**********************************************************************
     Q(N) = SUM(ALOG(2.0)**K/K!)    K=1,..,N ,      THE HIGHEST N
     (HERE 8) IS DETERMINED BY Q(N)=1.0 WITHIN STANDARD PRECISION
*/
{
static float q[8] = {
    0.6931472,0.9333737,0.9888778,0.9984959,0.9998293,0.9999833,0.9999986,1.0
};
static long i;
static float sexpo,a,u,ustar,umin;
static float *q1 = q;
    a = 0.0;
    u = ranf();
    goto S30;
S20:
    a += *q1;
S30:
    u += u;
    if(u <= 1.0) goto S20;
    u -= 1.0;
    if(u > *q1) goto S60;
    sexpo = a+u;
    return sexpo;
S60:
    i = 1;
    ustar = ranf();
    umin = ustar;
S70:
    ustar = ranf();
    if(ustar < umin) umin = ustar;
    i += 1;
    if(u > *(q+i-1)) goto S70;
    sexpo = a+umin**q1;
    return sexpo;
}

static float
snorm(void)
/*
**********************************************************************
                                                                      
                                                                      
     (STANDARD-)  N O R M A L  DISTRIBUTION                           
                                                                      
                                                                      
**********************************************************************
**********************************************************************
                                                                      
     FOR DETAILS SEE:                                                 
                                                                      
               AHRENS, J.H. AND DIETER, U.                            
               EXTENSIONS OF FORSYTHE'S METHOD FOR RANDOM             
               SAMPLING FROM THE NORMAL DISTRIBUTION.                 
               MATH. COMPUT., 27,124 (OCT. 1973), 927 - 937.          
                                                                      
     ALL STATEMENT NUMBERS CORRESPOND TO THE STEPS OF ALGORITHM 'FL'  
     (M=5) IN THE ABOVE PAPER     (SLIGHTLY MODIFIED IMPLEMENTATION)  
                                                                      
     Modified by Barry W. Brown, Feb 3, 1988 to use RANF instead of   
     SUNIF.  The argument IR thus goes away.                          
                                                                      
**********************************************************************
     THE DEFINITIONS OF THE CONSTANTS A(K), D(K), T(K) AND
     H(K) ARE ACCORDING TO THE ABOVEMENTIONED ARTICLE
*/
{
static float a[32] = {
    0.0,3.917609E-2,7.841241E-2,0.11777,0.1573107,0.1970991,0.2372021,0.2776904,
    0.3186394,0.36013,0.4022501,0.4450965,0.4887764,0.5334097,0.5791322,
    0.626099,0.6744898,0.7245144,0.7764218,0.8305109,0.8871466,0.9467818,
    1.00999,1.077516,1.150349,1.229859,1.318011,1.417797,1.534121,1.67594,
    1.862732,2.153875
};
static float d[31] = {
    0.0,0.0,0.0,0.0,0.0,0.2636843,0.2425085,0.2255674,0.2116342,0.1999243,
    0.1899108,0.1812252,0.1736014,0.1668419,0.1607967,0.1553497,0.1504094,
    0.1459026,0.14177,0.1379632,0.1344418,0.1311722,0.128126,0.1252791,
    0.1226109,0.1201036,0.1177417,0.1155119,0.1134023,0.1114027,0.1095039
};
static float t[31] = {
    7.673828E-4,2.30687E-3,3.860618E-3,5.438454E-3,7.0507E-3,8.708396E-3,
    1.042357E-2,1.220953E-2,1.408125E-2,1.605579E-2,1.81529E-2,2.039573E-2,
    2.281177E-2,2.543407E-2,2.830296E-2,3.146822E-2,3.499233E-2,3.895483E-2,
    4.345878E-2,4.864035E-2,5.468334E-2,6.184222E-2,7.047983E-2,8.113195E-2,
    9.462444E-2,0.1123001,0.136498,0.1716886,0.2276241,0.330498,0.5847031
};
static float h[31] = {
    3.920617E-2,3.932705E-2,3.951E-2,3.975703E-2,4.007093E-2,4.045533E-2,
    4.091481E-2,4.145507E-2,4.208311E-2,4.280748E-2,4.363863E-2,4.458932E-2,
    4.567523E-2,4.691571E-2,4.833487E-2,4.996298E-2,5.183859E-2,5.401138E-2,
    5.654656E-2,5.95313E-2,6.308489E-2,6.737503E-2,7.264544E-2,7.926471E-2,
    8.781922E-2,9.930398E-2,0.11556,0.1404344,0.1836142,0.2790016,0.7010474
};
static long i;
static float snorm,u,s,ustar,aa,w,y,tt;
    u = ranf();
    s = 0.0;
    if(u > 0.5) s = 1.0;
    u += (u-s);
    u = 32.0*u;
    i = (long) (u);
    if(i == 32) i = 31;
    if(i == 0) goto S100;
/*
                                START CENTER
*/
    ustar = u-(float)i;
    aa = *(a+i-1);
S40:
    if(ustar <= *(t+i-1)) goto S60;
    w = (ustar-*(t+i-1))**(h+i-1);
S50:
/*
                                EXIT   (BOTH CASES)
*/
    y = aa+w;
    snorm = y;
    if(s == 1.0) snorm = -y;
    return snorm;
S60:
/*
                                CENTER CONTINUED
*/
    u = ranf();
    w = u*(*(a+i)-aa);
    tt = (0.5*w+aa)*w;
    goto S80;
S70:
    tt = u;
    ustar = ranf();
S80:
    if(ustar > tt) goto S50;
    u = ranf();
    if(ustar >= u) goto S70;
    ustar = ranf();
    goto S40;
S100:
/*
                                START TAIL
*/
    i = 6;
    aa = *(a+31);
    goto S120;
S110:
    aa += *(d+i-1);
    i += 1;
S120:
    u += u;
    if(u < 1.0) goto S110;
    u -= 1.0;
S140:
    w = u**(d+i-1);
    tt = (0.5*w+aa)*w;
    goto S160;
S150:
    tt = u;
S160:
    ustar = ranf();
    if(ustar > tt) goto S50;
    u = ranf();
    if(ustar >= u) goto S150;
    u = ranf();
    goto S140;
}
static float
fsign( float num, float sign )
/* Transfers sign of argument sign to argument num */
{
if ( ( sign>0.0f && num<0.0f ) || ( sign<0.0f && num>0.0f ) )
    return -num;
else return num;
}

/*
**********************************************************************
     void phrtsd(char* phrase,long *seed1,long *seed2)
               PHRase To SeeDs

                              Function

     Uses a phrase (character string) to generate two seeds for the RGN
     random number generator.
                              Arguments
     phrase --> Phrase to be used for random number generation
      
     seed1 <-- First seed for generator
                        
     seed2 <-- Second seed for generator
                        
                              Note

     Trailing blanks are eliminated before the seeds are generated.
     Generated seed values will fall in the range 1..2^30
     (1..1,073,741,824)

     
**********************************************************************
*/

static void
phrtsd(char* phrase,			/* seed phrase; if NULL seed[12]
					   are _input_ not output */
       long *seed1,			/* The two seeds utilised;
					   _input_ iff phrase == NULL */
       long *seed2)
{
   static char table[] =
     "abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\
0123456789\
!@#$%^&*()_+[];:'\\\"<>?,./";
   long ix;
   
   static long twop30 = 1073741824L;
   static long shift[5] = {
      1L,64L,4096L,262144L,16777216L
   };
   static long i,ichr,j,lphr,values[5];

   if(phrase == NULL) {
      assert(*seed1 != 0 && *seed2 != 0);
   } else {
      *seed1 = 1234567890L;
      *seed2 = 123456789L;
      lphr = lennob(phrase); 
      if(lphr < 1) return;
      for(i=0; i<=(lphr-1); i++) {
	 for (ix=0; table[ix]; ix++) if (phrase[i] == table[ix]) break; 
	 if (!table[ix]) ix = 0;
	 ichr = ix % 64;
	 if(ichr == 0) ichr = 63;
	 for(j=1; j<=5; j++) {
            values[j-1] = ichr-j;
            if(values[j-1] < 1) values[j-1] += 63;
	 }
	 for(j=1; j<=5; j++) {
            *seed1 = ( *seed1+shift[j-1]*values[j-1] ) % twop30;
            *seed2 = ( *seed2+shift[j-1]*values[6-j-1] )  % twop30;
	 }
      }
   }

   setall(*seed1,*seed2);
}

/*
**********************************************************************
     long ignpoi(float mu)
                    GENerate POIsson random deviate
                              Function
     Generates a single random deviate from a Poisson
     distribution with mean AV.
                              Arguments
     av --> The mean of the Poisson distribution from which
            a random deviate is to be generated.
     genexp <-- The random deviate.
                              Method
     Renames KPOIS from TOMS as slightly modified by BWB to use RANF
     instead of SUNIF.
     For details see:
               Ahrens, J.H. and Dieter, U.
               Computer Generation of Poisson Deviates
               From Modified Normal Distributions.
               ACM Trans. Math. Software, 8, 2
               (June 1982),163-179
**********************************************************************
**********************************************************************
                                                                      
                                                                      
     P O I S S O N  DISTRIBUTION                                      
                                                                      
                                                                      
**********************************************************************
**********************************************************************
                                                                      
     FOR DETAILS SEE:                                                 
                                                                      
               AHRENS, J.H. AND DIETER, U.                            
               COMPUTER GENERATION OF POISSON DEVIATES                
               FROM MODIFIED NORMAL DISTRIBUTIONS.                    
               ACM TRANS. MATH. SOFTWARE, 8,2 (JUNE 1982), 163 - 179. 
                                                                      
     (SLIGHTLY MODIFIED VERSION OF THE PROGRAM IN THE ABOVE ARTICLE)  
                                                                      
**********************************************************************
      INTEGER FUNCTION IGNPOI(IR,MU)
     INPUT:  IR=CURRENT STATE OF BASIC RANDOM NUMBER GENERATOR
             MU=MEAN MU OF THE POISSON DISTRIBUTION
     OUTPUT: IGNPOI=SAMPLE FROM THE POISSON-(MU)-DISTRIBUTION
     MUPREV=PREVIOUS MU, MUOLD=MU AT LAST EXECUTION OF STEP P OR B.
     TABLES: COEFFICIENTS A0-A7 FOR STEP F. FACTORIALS FACT
     COEFFICIENTS A(K) - FOR PX = FK*V*V*SUM(A(K)*V**K)-DEL
     SEPARATION OF CASES A AND B
*/
static long
ignpoi(float mu)
{
   static float a0 = -0.5;
   static float a1 = 0.3333333;
   static float a2 = -0.2500068;
   static float a3 = 0.2000118;
   static float a4 = -0.1661269;
   static float a5 = 0.1421878;
   static float a6 = -0.1384794;
   static float a7 = 0.125006;
   static float muold = 0.0;
   static float muprev = 0.0;
   static float fact[10] = {
      1.0,1.0,2.0,6.0,24.0,120.0,720.0,5040.0,40320.0,362880.0
};
static long ignpoi,j,k,kflag,l,m;
static float b1,b2,c,c0,c1,c2,c3,d,del,difmuk,e,fk,fx,fy,g,omega,p,p0,px,py,q,s,
    t,u,v,x,xx,pp[35];

    if(mu == muprev) goto S10;
    if(mu < 10.0) goto S120;
/*
     C A S E  A. (RECALCULATION OF S,D,L IF MU HAS CHANGED)
*/
    muprev = mu;
    s = sqrt(mu);
    d = 6.0*mu*mu;
/*
             THE POISSON PROBABILITIES PK EXCEED THE DISCRETE NORMAL
             PROBABILITIES FK WHENEVER K >= M(MU). L=IFIX(MU-1.1484)
             IS AN UPPER BOUND TO M(MU) FOR ALL MU >= 10 .
*/
    l = (long) (mu-1.1484);
S10:
/*
     STEP N. NORMAL SAMPLE - SNORM(IR) FOR STANDARD NORMAL DEVIATE
*/
    g = mu+s*snorm();
    if(g < 0.0) goto S20;
    ignpoi = (long) (g);
/*
     STEP I. IMMEDIATE ACCEPTANCE IF IGNPOI IS LARGE ENOUGH
*/
    if(ignpoi >= l) return ignpoi;
/*
     STEP S. SQUEEZE ACCEPTANCE - SUNIF(IR) FOR (0,1)-SAMPLE U
*/
    fk = (float)ignpoi;
    difmuk = mu-fk;
    u = ranf();
    if(d*u >= difmuk*difmuk*difmuk) return ignpoi;
S20:
/*
     STEP P. PREPARATIONS FOR STEPS Q AND H.
             (RECALCULATIONS OF PARAMETERS IF NECESSARY)
             .3989423=(2*PI)**(-.5)  .416667E-1=1./24.  .1428571=1./7.
             THE QUANTITIES B1, B2, C3, C2, C1, C0 ARE FOR THE HERMITE
             APPROXIMATIONS TO THE DISCRETE NORMAL PROBABILITIES FK.
             C=.1069/MU GUARANTEES MAJORIZATION BY THE 'HAT'-FUNCTION.
*/
    if(mu == muold) goto S30;
    muold = mu;
    omega = 0.3989423/s;
    b1 = 4.166667E-2/mu;
    b2 = 0.3*b1*b1;
    c3 = 0.1428571*b1*b2;
    c2 = b2-15.0*c3;
    c1 = b1-6.0*b2+45.0*c3;
    c0 = 1.0-b1+3.0*b2-15.0*c3;
    c = 0.1069/mu;
S30:
    if(g < 0.0) goto S50;
/*
             'SUBROUTINE' F IS CALLED (KFLAG=0 FOR CORRECT RETURN)
*/
    kflag = 0;
    goto S70;
S40:
/*
     STEP Q. QUOTIENT ACCEPTANCE (RARE CASE)
*/
    if(fy-u*fy <= py*exp(px-fx)) return ignpoi;
S50:
/*
     STEP E. EXPONENTIAL SAMPLE - SEXPO(IR) FOR STANDARD EXPONENTIAL
             DEVIATE E AND SAMPLE T FROM THE LAPLACE 'HAT'
             (IF T <= -.6744 THEN PK < FK FOR ALL MU >= 10.)
*/
    e = sexpo();
    u = ranf();
    u += (u-1.0);
    t = 1.8+fsign(e,u);
    if(t <= -0.6744) goto S50;
    ignpoi = (long) (mu+s*t);
    fk = (float)ignpoi;
    difmuk = mu-fk;
/*
             'SUBROUTINE' F IS CALLED (KFLAG=1 FOR CORRECT RETURN)
*/
    kflag = 1;
    goto S70;
S60:
/*
     STEP H. HAT ACCEPTANCE (E IS REPEATED ON REJECTION)
*/
    if(c*fabs(u) > py*exp(px+e)-fy*exp(fx+e)) goto S50;
    return ignpoi;
S70:
/*
     STEP F. 'SUBROUTINE' F. CALCULATION OF PX,PY,FX,FY.
             CASE IGNPOI .LT. 10 USES FACTORIALS FROM TABLE FACT
*/
    if(ignpoi >= 10) goto S80;
    px = -mu;
    py = pow(mu,(double)ignpoi)/ *(fact+ignpoi);
    goto S110;
S80:
/*
             CASE IGNPOI .GE. 10 USES POLYNOMIAL APPROXIMATION
             A0-A7 FOR ACCURACY WHEN ADVISABLE
             .8333333E-1=1./12.  .3989423=(2*PI)**(-.5)
*/
    del = 8.333333E-2/fk;
    del -= (4.8*del*del*del);
    v = difmuk/fk;
    if(fabs(v) <= 0.25) goto S90;
    px = fk*log(1.0+v)-difmuk-del;
    goto S100;
S90:
    px = fk*v*v*(((((((a7*v+a6)*v+a5)*v+a4)*v+a3)*v+a2)*v+a1)*v+a0)-del;
S100:
    py = 0.3989423/sqrt(fk);
S110:
    x = (0.5-difmuk)/s;
    xx = x*x;
    fx = -0.5*xx;
    fy = omega*(((c3*xx+c2)*xx+c1)*xx+c0);
    if(kflag <= 0) goto S40;
    goto S60;
S120:
/*
     C A S E  B. (START NEW TABLE AND CALCULATE P0 IF NECESSARY)
*/
    muprev = 0.0;
    if(mu == muold) goto S130;
    muold = mu;
    m = max(1L,(long) (mu));
    l = 0;
    p = exp(-mu);
    q = p0 = p;
S130:
/*
     STEP U. UNIFORM SAMPLE FOR INVERSION METHOD
*/
    u = ranf();
    ignpoi = 0;
    if(u <= p0) return ignpoi;
/*
     STEP T. TABLE COMPARISON UNTIL THE END PP(L) OF THE
             PP-TABLE OF CUMULATIVE POISSON PROBABILITIES
             (0.458=PP(9) FOR MU=10)
*/
    if(l == 0) goto S150;
    j = 1;
    if(u > 0.458) j = min(l,m);
    for(k=j; k<=l; k++) {
        if(u <= *(pp+k-1)) goto S180;
    }
    if(l == 35) goto S130;
S150:
/*
     STEP C. CREATION OF NEW POISSON PROBABILITIES P
             AND THEIR CUMULATIVES Q=PP(K)
*/
    l += 1;
    for(k=l; k<=35; k++) {
        p = p*mu/(float)k;
        q += p;
        *(pp+k-1) = q;
        if(u <= q) goto S170;
    }
    l = 35;
    goto S130;
S170:
    l = k;
S180:
    ignpoi = k;
    return ignpoi;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*
 * Here beginneth the externally visible parts of this file.
 */
void
phPoissonSeedSet(long seed1,
		 long seed2)
{
   phrtsd(NULL, &seed1, &seed2);
}

long
phPoissonDevGet(float mu)
{
   return(ignpoi(mu));
}
