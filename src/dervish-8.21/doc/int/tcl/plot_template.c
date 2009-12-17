/* pplot_template.c -
 *
 *	Template routines for calling histogramming/contouring routines 
 *	using region data.
 *
 *  12-Feb-1993	Penelope Constanta-Fanourakis
 *
 */

#include "plot_template.h"

/***************************  Supporting routines *****************************/
/*
 * Returns a pointer to a character string or NULL.
 * If NULL, it sets interp->result appropriately.
 * It concatenates arguments passed from tcl interpreter to form
 * one string suitable for parsing by the ftclParse routines.
 * The caller is responsible for freeing the allocated space with free();
 */
char *concatTclArgs (int argc, char **argv){

   char *string;
   int  len, i;

/* 
 *	Find how much space to allocate.
 */
   len = 0;
   for (i=1; i<argc; i++)
      {
      len += strlen(argv[i]) + 1;
      }
/*
 *	Allocate space for the string to be constructed. This space needs to
 *	be freed by the calling routine.
 */
   string = (char *)malloc(len*sizeof(char));
   if (string == (char *)NULL) return (string);

/*
 *	Construct a string by concatenating all input arguments starting from
 *	argv[1]
 */
   string[0] = NULL;
   for (i=1; i< argc; i++)
      {
      strcat(string, argv[i]);
      strcat(string, " ");
      }
   return(string);
}

/*
 * Returns a pointer to an array of nfloat floating point numbers, or NULL.
 * If NULL, set interp->result appropriately. 
 * It creates an array of floats from the tcl list.
 * The caller is responsible for freeing the allocated space with free();
 */
float *getFloatArray (Tcl_Interp *interp, char *list, int *nfloat) 
{
   int  i, status;
   char **largv;
   double tdouble;
   float  *fvals;

/*
 *	Split the tcl list
 */
   if (Tcl_SplitList(interp, list, nfloat, &largv) != TCL_OK) return((float *)0);
/*
 *	Allocate space for the resulting floating array
 */
   fvals = (float *)malloc (*nfloat*sizeof(float));
   if (fvals == (float *)0)
      {
      Tcl_SetResult(interp, "getFloatArray: Internal allocation error",TCL_VOLATILE);
      return (fvals);
      }
/*
 *	Fill the array of floats
 */
   i = 0;
   status = TCL_OK;
   if (*nfloat > 0)
      {
      while ((i < *nfloat) && 
             (status = Tcl_GetDouble(interp, largv[i], &tdouble)) == TCL_OK )
         {
         fvals[i++] = (float)tdouble;
         }
      }
/*
 *	Free local space and return appropriate values
 */
   free (largv);
   if (status != TCL_OK) 
      {
      free (fvals);
      return ((float *)0);
      }

   return (fvals);
}


/***********************  Template plotting routines  *************************/
/*
 *		--------  dervishHist  ------
 *	It accepts input of the form:
 *  reg -rmin=a -rmax=b -cmin=c -cmax=d -nbin=e -dmin=f -dmax=g -flag=NEW/CURRENT
 *	were:
 *		reg = region name
 *         and optionally:
 *		rmin = minimum row value, default = 0
 *		rmax = maximum row value, default = maximum row in region
 *		cmin = minimum column value, default = 0
 *		cmax = maximum column value, default = maximum column in region
 *		dmin = minimum data value (x-axis minimum), default = min (data)
 *		dmax = maximum data value (x-axis maximum), default = max (data)
 *		flag = CURRENT = use current page to draw histogram
 *		       any other value create a new page.
 */

int dervishHist (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   char    *fullCmd = "reg -rmin -rmax -cmin -cmax -nbin -dmin -dmax -flag";
   char    *userCmd;
   char    *region;
   int     rmin, rmax, cmin, cmax, nbin, flag;
   int     nr, nc, ndata;
   float   dmin, dmax, xmin, xmax;
   char    *error_msg;
   REGION  *regPtr;
   IMDATA  **rows;

   int        ir;
   float      *data;
   Tcl_Interp *new_interp;

/*
 *	Check if required arguments are present
 */
   if (argc < 2)
      {
      Tcl_SetResult(interp,
            "Usage: dervishHist reg -rmin -rmax -cmin -cmax -dmin -dmax -nbin -flag", 
            TCL_VOLATILE);
      return (TCL_ERROR);
      }
/*
 *  Parse input using ftclParse routines. Create a new interpreter that will
 *  be used by the parsing routines. The interpreter should be deleted
 *  before exiting from this routine.
 */

   new_interp = Tcl_CreateExtendedInterp();
   ftclParseInit(new_interp);
/*
 *	Construct a string that will contain the input arguments. 
 *	Space will be allocated by the concatenating routine and it should
 *	be freed before exiting from this routine.
 */
   userCmd = concatTclArgs(argc, argv);
   if (userCmd == (char *)NULL)
      {
      Tcl_SetResult(interp, "dervishHist: Internal allocation error",TCL_VOLATILE);
      goto error_exit;
      }
/*
 *	Start the parsing of the user entered command
 */
   if (!ftclFullParse (fullCmd, userCmd))
      {
      Tcl_SetResult(interp,
            "Usage: dervishHist reg -rmin -rmax -cmin -cmax -dmin -dmax -nbin -flag", 
            TCL_VOLATILE);
      goto error_exit;
      }

/*
 *	Initialize values
 */

   region = ftclGetStr("reg");
/*
 *	Get a pointer to the pixel values and the number of rows and columns
 */

   if (shRegionPixels(&error_msg, region, &rows, &nr, &nc)) 
      {
      Tcl_SetResult(interp, error_msg, TCL_VOLATILE);
      goto error_exit;
      }
/*
 *	Make the returned values for number of rows and columns to zero
 *	based array indeces
 */
   nr--;
   nc--;

   rmin = 0;
   cmin = 0;
   nbin = 200;
   flag = 0;

/*
 *	Get optional data from input
 */

   if (ftclPresent("rmin")) rmin = ftclGetInt("rmin");
   if (rmin < 0) rmin = 0;

   if (ftclPresent("rmax")) 
      {
      rmax = ftclGetInt("rmax");
      if (rmax > nr) rmax = nr;
      }
   else
      {
      rmax = nr;
      }

   if (ftclPresent("cmin")) cmin = ftclGetInt("cmin");
   if (cmin < 0) cmin = 0;

   if (ftclPresent("cmax")) 
      {
      cmax = ftclGetInt("cmax");
      if (cmax > nc) cmax = nc;
      }
   else
      {
      cmax = nc;
      }
/*
 *	Validate given values
 */
   if ((rmin > rmax) || (cmin > cmax))
      {
      Tcl_SetResult(interp, "dervishHist: Invalid pixel selection", TCL_VOLATILE);
      goto error_exit;
      }

   if (ftclPresent("nbin")) nbin = ftclGetInt("nbin");
   if (ftclPresent("flag")) 
      {if (strcasecmp(ftclGetStr("flag"), "CURRENT")  == 0) flag = 1;}

   dmin = -1.0;
   dmax = 1.0;
   if (ftclPresent("dmin")) dmin = ftclGetDouble("dmin");
   if (ftclPresent("dmax")) dmax = ftclGetDouble("dmax");
   if (dmin > dmax)
      {
      Tcl_SetResult(interp, "dervishHist: Invalid data range", TCL_VOLATILE);
      goto error_exit;
      }

/*
 *	Get space for the data array and fill it with the pixel values.
 *	The space should be freed before exiting this routine.
 */
   ndata = (rmax - rmin +1)*(cmax - cmin +1);
   data = (float *)malloc (ndata*sizeof(float));
   if (data == (float *)NULL)
      {
      Tcl_SetResult(interp, "dervishHist: Internal allocation error", TCL_VOLATILE);
      goto error_exit;
      }

/*
 *	Since the size of an region is in general ~4Mbytes, try to optimize
 *	the code as much as possible.
 */
   xmin = 0;
   xmax = 0;
   {
      register float *dataptr;
      register IMDATA *pxl;

      dataptr = data;
      for (ir = rmin; ir <= rmax; ir++)
         {
         pxl = rows[ir] + cmin;
         while (pxl < rows[ir]+cmax )
            {
            *dataptr = *pxl++;
            if (*dataptr < xmin) xmin = *dataptr;
            if (*dataptr > xmax) xmax = *dataptr;
            dataptr++;
            }
         }
   }
/*
 *	Check which value of dmin, dmax to set
 */

   if (!ftclPresent("dmin")) dmin = xmin;
   if (!ftclPresent("dmax")) dmax = xmax;
/*
 *	Call the histogramming routine
 */

   pgplot_pghist(&ndata, data, &dmin, &dmax, &nbin, &flag);

/*
 *	Free allocated space and delete the interpreter before returning
 */

   free (userCmd);
   free (data);
   Tcl_DeleteInterp(new_interp);
   return (TCL_OK);

error_exit:
   if (userCmd !=  (char *)NULL) free (userCmd);
   if (data    != (float *)NULL) free (data);
   Tcl_DeleteInterp(new_interp);
   return (TCL_ERROR);
}


/*
 *		--------  dervishCont  ------
 *	It accepts input of the form:
 *reg levels -flag=AUTO/CURRENT -rmin=a -rmax=b -cmin=c -cmax=d -tr={e,f,g,i,j,k}
 *	were:
 *		reg    = region name
 *		levels = contour levels array
 *         and optionally:
 *		flag = CURRENT = use current line style
 *		       any other value use solid lines for positive contours and
 *		       dashed for negative.
 *		rmin = minimum row value, default = 0
 *		rmax = maximum row value, default = maximum row in region
 *		cmin = minimum column value, default = 0
 *		cmax = maximum column value, default = maximum column in region
 *		tr   = 6-element array with transformation of I,J to world coord
 *		       default :{0 1 0 0 0 1}
 */

int dervishCont (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) {

   char    *fullCmd = "reg levels -flag -rmin -rmax -cmin -cmax -tr";
   char    *userCmd;
   char    *region;
   int     rmin, rmax, cmin, cmax, flag;
   int     nr, nc, idim, jdim;
   float   *levels;
   int     nlevel;
   char    *error_msg;
   char    *string;
   REGION  *regPtr;
   IMDATA  **rows;

   int        ir;
   float      *data;
   float      *tr;
   int        ntr;
   Tcl_Interp *new_interp;

/*
 *	Check if required arguments are present
 */
   if (argc < 3)
      {
      Tcl_SetResult(interp,
            "Usage: dervishCont reg levels -flag -rmin -rmax -cmin -cmax -tr",
            TCL_VOLATILE);
      return (TCL_ERROR);
      }
/*
 *  Parse input using ftclParse routines. Create a new interpreter that will
 *  be used by the parsing routines. The interpreter should be deleted
 *  before exiting from this routine.
 */

   new_interp = Tcl_CreateExtendedInterp();
   ftclParseInit(new_interp);
/*
 *	Construct a string that will contain the input arguments. 
 *	Space will be allocated by the concatenating routine and it should
 *	be freed before exiting from this routine.
 */
   userCmd = concatTclArgs(argc, argv);
   if (userCmd == (char *)NULL)
      {
      Tcl_SetResult(interp, "dervishCont: Internal allocation error",TCL_VOLATILE);
      goto error_exit;
      }
/*
 *	Start the parsing of the user entered command
 */
   if (!ftclFullParse (fullCmd, userCmd))
      {
      Tcl_SetResult(interp,
            "Usage: dervishCont reg levels -flag -rmin -rmax -cmin -cmax -tr",
            TCL_VOLATILE);
      goto error_exit;
      }

/*
 *	Initialize values
 */

   region = ftclGetStr("reg");

/*
 *	Get a pointer to the pixel values and the number of rows and columns
 */

   if (shRegionPixels(&error_msg, region, &rows, &nr, &nc)) 
      {
      Tcl_SetResult(interp, error_msg, TCL_VOLATILE);
      goto error_exit;
      }
/*
 *	Make the returned values for number of rows and columns to zero
 *	based array indeces
 */
   nr--;
   nc--;

/*
 *	Get the contour levels. Do not use the ftclParse routines for this
 *	it is too complicated. Assume that the contour levels are the
 *	second argument passed.
 */
   levels = getFloatArray (interp, argv[2], &nlevel);
   if (levels == (float *)NULL) goto error_exit;

/*
 *	Get optional data from input
 */

   rmin = 0;
   cmin = 0;

   if (ftclPresent("rmin")) rmin = ftclGetInt("rmin");
   if (rmin < 0) rmin = 0;

   if (ftclPresent("rmax")) 
      {
      rmax = ftclGetInt("rmax");
      if (rmax > nr) rmax = nr;
      }
   else
      {
      rmax = nr;
      }

   if (ftclPresent("cmin")) cmin = ftclGetInt("cmin");
   if (cmin < 0) cmin = 0;

   if (ftclPresent("cmax")) 
      {
      cmax = ftclGetInt("cmax");
      if (cmax > nc) cmax = nc;
      }
   else
      {
      cmax = nc;
      }

   if ((rmin > rmax) || (cmin > cmax))
      {
      Tcl_SetResult(interp, "dervishCont: Invalid pixel selection", TCL_VOLATILE);
      goto error_exit;
      }

   flag = 1;
   if (ftclPresent("flag")) 
      {if (strcasecmp(ftclGetStr("flag"), "CURRENT")  == 0) flag = -1;}

/*
 *	Get the transformation array. Assume that it has six elements
 */

   if (ftclPresent("tr"))
      {
      string = ftclGetList ("tr");
      tr = getFloatArray (interp, string, &ntr);
      if (ntr != 6)
         {
         Tcl_SetResult(interp, "dervishCont: Invalid tr array size", TCL_VOLATILE);
         goto error_exit;
         }
      }
   else
      {
      ntr = 6;
      tr = (float *)malloc(ntr*sizeof(float));
      tr[0] = 0.;
      tr[1] = 1.;
      tr[2] = 0.;
      tr[3] = 0.;
      tr[4] = 0.;
      tr[5] = 1.;
      }
/*
 *	Get space for the data array and fill it with the pixel values.
 *	The space should be freed before exiting this routine. 
 *	The fortran routine makes use of a two dimentional array but
 *	from a c routine we need to pass a one dimentional array, so
 *	the data need to be set as follows:
 *	   data(0,0), data(1,0), data(2,0)... 
 *	   data(0,1), data(1,1), data(2,1)...
 *         data(0,2), data(2,1), data(2,2)...
 *	For optimization purposes one could make the fortran 2-d array
 *	to contain information as data(column, row).
 */
   jdim = (rmax - rmin +1);
   idim = (cmax - cmin +1);
   data = (float *)malloc (idim*jdim*sizeof(float));
   if (data == (float *)NULL)
      {
      Tcl_SetResult(interp, "dervishCont: Internal allocation error", TCL_VOLATILE);
      goto error_exit;
      }

   {
      register float *dataptr;
      register IMDATA *pxl;

      dataptr = data;
      for (ir = rmin; ir <= rmax; ir++)
         {
         pxl = rows[ir] + cmin;
         while (pxl < rows[ir]+cmax )
            {
            *dataptr++ = *pxl++;
            }
         }
   }
/*
 *	Call the contouring routine
 */

   nlevel *= flag;	/* set the line style */
   pgplot_pgcont (data, &idim, &jdim, &rmin, &rmax, &cmin, &cmax, 
                  levels, &nlevel, tr);

/*
 *	Free allocated space and delete the interpreter before returning
 */

   free (userCmd);
   free (data);
   free (levels);
   free (tr);
   Tcl_DeleteInterp(new_interp);
   return (TCL_OK);

error_exit:
   if (userCmd !=  (char *)NULL) free (userCmd);
   if (data    != (float *)NULL) free (data);
   if (levels  != (float *)NULL) free (levels);
   if (tr      != (float *)NULL) free (tr);
   Tcl_DeleteInterp(new_interp);
   return (TCL_ERROR);
}

/**************************** Declare TCL verbs ********************************/
/*
 *	This routine should be called by the main program in order to
 *	declare the defined verbs.
 */

void plotTclDeclare (Tcl_Interp *interp) 
{
/*
 *      Defined tcl verbs for the plotting routines
 */
    Tcl_CreateCommand(interp, "dervishHist", dervishHist,  (ClientData) 0,
            (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "dervishCont", dervishCont,  (ClientData) 0,
            (Tcl_CmdDeleteProc *) NULL);
}
