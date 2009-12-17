/* this is simple_c.c, a simple example of calling PGPLOT from c */
/* Chris Stoughton  May 6, 1993 */

main()
{
  float xs[] = { 1.0, 2.0, 3.0,  4.0,  5.0 };
  float ys[] = { 1.0, 4.0, 9.0, 16.0, 25.0 };

/* for call to pgplot_pgbegin */
  int unit, nxsub, nysub; 
  char string[10];
/* for call to pgplot_pgenv */
  float xmin, xmax, ymin, ymax;
  int just, axis;
/* for call to pgplot_pglabel */
  char xlab[80],ylab[80],tlab[80];
/* for call to pgpoint */
  int npoints, symbol;
/* for setup and call to pgline */
  int i, nlines;
  float xr[60], yr[60];

  unit = 0; strcpy(string, "?"); nxsub = 1; nysub = 1;
  pgplot_pgbegin(&unit, string, &nxsub, &nysub);

  xmin = 0.0; xmax = 10.0; ymin = 0.0; ymax = 20.0; just = 0; axis = 1;
  pgplot_pgenv(&xmin, &xmax, &ymin, &ymax, &just, &axis);

  strcpy(xlab,"(x)"); strcpy(ylab,"(y)"); strcpy(tlab,"A Simple Graph");
  pgplot_pglabel(xlab,ylab,tlab);

  npoints = 5; symbol = 9;
  pgplot_pgpoint(&npoints, xs, ys, &symbol);

  for (i=0; i<60; i++){
    xr[i] = 0.1*i;
    yr[i] = xr[i]*xr[i];
  }
  nlines = 60;
  pgplot_pgline(&nlines, xr, yr);

  pgplot_pgend();
}

