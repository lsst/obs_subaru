#if !defined(PHCATOBJ_H)
#define PHCATOBJ_H

   /*
    * this structure is used to store information on objects
    * in ASCII catalogs, such as "ppmcat" or "starqsocat".
    * We create CHAINs of these structures by reading the
    * catalog files, and can use them to build decision trees
    * for later use in classification
    */

#define CATTYPE_STARORQSO     0   /* can't tell from some catalogs */
#define CATTYPE_ELLIP         1   /* elliptical galaxy */
#define CATTYPE_S0            2   /* S-zero galaxy */
#define CATTYPE_SA            3   /* Sa galaxy */
#define CATTYPE_SB            4   /* Sb galaxy */
#define CATTYPE_SC            5   /* Sc galaxy */
#define CATTYPE_SD            6   /* Sd galaxy */
#define CATTYPE_IRREG         7   /* Irregular galaxy */
#define CATTYPE_STAR         10   /* star (and not QSO) */
#define CATTYPE_QSO          11   /* QSO (and not star) */

   /* this macro yields 1 if object is any type of galaxy */
#define CATISGALAXY(x)    (((x) >= CATTYPE_ELLIP) && ((x) <= CATTYPE_IRREG))

typedef struct {
   int id;                /* unique ID number */
   float lambda;          /* lambda coord, in degrees */
   float eta;             /* eta coord, in degrees */
   float redshift;        /* object's cosmological redshift */
   float pa;              /* position angle, in degrees */
   float mag;             /* fiducial mag, same as either g or r */
   int junk;              /* =1 indicates "junk" object */
   int type;              /* type of object (see codes above) */
   float axratio;         /* axis ratio: 1.0 is roune, 0.1 elongated */
   float reff;            /* effective radius, in pixels */
   float umag;            /* total magnitude in SDSS u' band */
   float gmag;            /* total magnitude in SDSS g' band */
   float rmag;            /* total magnitude in SDSS r' band */
   float imag;            /* total magnitude in SDSS i' band */
   float zmag;            /* total magnitude in SDSS z' band */
} CATOBJ;

CATOBJ *phCatobjNew(void);
void phCatobjDel(CATOBJ *catobj);
CHAIN *phCatobjSelectByPosition(CHAIN *chain, float minlambda, float maxlambda,
                              float mineta, float maxeta);


#endif
