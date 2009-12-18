-------------------------------------------------------------------------------
David Schlegel, Princeton, 5 August 1998
-------------------------------------------------------------------------------
 
The data files for
  "Maps of Dust IR Emission for Use in Estimation of Reddening
  and CMBR Foregrounds", Schlegel, D., Finkbeiner, D., & Davis, M.,
  ApJ, 1998, in press,
can be copied from one of the following sites: 
  http://astro.princeton.edu/~schlegel/dust/index.html
  http://astro.berkeley.edu/davis/dust/index.html

This set of self-contained C routines are for reading the SFD dust maps.
The maps are simple FITS files in Lambert (polar) projection, with one
file for each hemisphere of the projection.
 
The simplest interface is with "dust_getval".  A more direct interface
for reading Lambert-projection files is lambert_getval() in C, or
lambert_testf in Fortran.

Report problems with the C interface to David Schlegel at
"schlegel@astro.princeton.edu".

Compilation
-----------
Compile everything with "make all", which will put the binaries in
the directory specified by the variable "DEST" in "Makefile".
Different compiler flags should be selected in "Makefile" if you
are using a machine with little-endian architechture, such as a DEC.
 
This software is written to ANSI C specifications.  This is supported
by the Gnu C compiler, but not necessarily all other compilers.
The Gnu C compiler can be obtained via anonymous FTP to
"prep.ai.mit.edu".  First read the file "/pub/gnu/GNUinfo/FTP"
for information about downloading.

General information
-------------------
ASCII data files are allowed to have any number of comment lines with a hash
mark ("#") in the first column.  Such lines will be ignored by all programs.
 
Images are always 0-indexed, as opposed to the IRAF or SAOimage convention
of 1-indexing.

Available maps
--------------
   The following maps are available by setting MAP in the call to "dust_getval":
 
   File names      MAP
   --------------  ----  ----------------------------------------------------
   SFD_i100*.fits  I100  100-micron map in MJy/Sr
   SFD_xmap*.fits  X     X-map, temperature-correction factor
   SFD_temp*.fits  T     Temperature map in degrees Kelvin for n=2 emissivity
   SFD_dust*.fits  Ebv   E(B-V) in magnitudes
   SFD_mask*.fits  mask  8-bit mask
 
   The default is to read the 4096^2 images, with a scale of approximately
   (2.37 arc min)^2 per pixel.

Calling sequence
----------------
   dust_getval gall galb map=map ipath=ipath interp=interp \
    infile=infile outfile=outfile

Simple examples
---------------
   Read the reddening value E(B-V) at Galactic (l,b)=(121,-21.5),
   nterpolating from the nearest 4 pixels, and output to the screen:
     % dust_getval 121 -21.5 interp=y
   In this example, the returned value should be E(B-V) = 0.342254 mag.

   You may wish to know if there are any mask bits set for this position:
     % dust_getval 121 -21.5 map=mask
     121.000 -21.500  1hcons OK      OK      OK      OK      big_obj OK
   This will report "1hcons", signifying that IRAS only scanned this position
   once (out of a possible 3 HCONs).  Also, the "big_obj" flag is reported
   as set, signifying that this position falls near the LMC, SMC or M31
   (the latter in this case).

   Read the temperature map at positions listed in the file "dave.in",
   interpolating from the nearest 4 pixels, and output to file "dave.out".
   The path name for the temperature maps is "/u/schlegel/".
     % dust_getval map=T ipath=/u/schlegel/ interp=y \
       infile=dave.in outfile=dave.out

Optional inputs
---------------
   Either the coordinates "gall" and "galb" must be set, or these coordinates
   must exist in the file "infile".  Output is written to standard output
   or the file "outfile".

   gall:       Galactic longitude(s) in degrees
   galb:       Galactic latitude(s) in degrees
   map:        Set to one of the following (default is "Ebv"):
               I100: 100-micron map in MJy/Sr
               X   : X-map, temperature-correction factor
               T   : Temperature map in degrees Kelvin for n=2 emissivity
               Ebv : E(B-V) in magnitudes
               mask: Mask values
   infile:     If set, then read "gall" and "galb" from this file
   outfile:    If set, then write results to this file
   interp:     Set this flag to "y" to return a linearly interpolated value
               from the 4 nearest pixels.
               This is disabled if map=='mask'.
   verbose:    Set this flag to "y" for verbose output, printing pixel
               coordinates and map values
   ipath:      Path name for dust maps; default to path set by the
               environment variable $DUST_DIR/maps, or to the current
               directory.

Mask values
-----------
   The mask has 8 bits containing information about the processing history
   for each position on the sky.  The lowest two bits contain a 2-bit integer
   of the number of HCON's in the IRAS/ISSA plates (0 through 3).  The other
   bits are set to 1 if the following conditionals are true:
 
   Bit  Name       Comments
   ---  ---------  -------------------------------------------------------
   0-1             Number of HCON's in IRAS/ISSA plates
     2  'asteroi'  Asteroid has been removed from one HCON
     3  'glitch '  Glitch has been removed from one HCON
     4  'source '  Source (star or galaxy) has been removed
     5  'no_list'  No point source list available for this part of the sky
     6  'big_obj'  Position near LMC, SMC, or M31; no sources removed
     7  'no_IRAS'  No IRAS data; filled with DIRBE data
 
   A call to "dust_getval" with "map=mask" returns the number of HCON's
   followed by the Name of a bit if it is set, or "OK" if not set.

Data format
-----------
   All maps are stored as FITS files, in pairs of 4096x4096 (or MxM) pixel
   Lambert projections.  The NGP projection covers the northern Galactic
   hemisphere, centered at b=+90 deg, with latitude running clockwise.
   The SGP projection covers the southern Galactic hemisphere, centered at
   b=-90 deg, with latitude running counter-clockwise.  Galactic coordinates
   (l,b) are converted to pixel positions (x,y) via
      x = 2048 SQRT {1 - n sin(b)}  cos(l) + 2047.5
      y = - 2048 n SQRT{1 - n sin(b)} sin(l) + 2047.5
   where n=+1 for the NGP, and n=-1 for the SGP.
   Pixel numbers are zero-indexed, with the center of the lower left pixel
   having position (x,y)=(0,0).  These Lambert projections are minimally
   distorted at high Galactic latitudes, with the distortion approaching
   40% at b=0 deg.  The pixel size of (2.372 arcmin)^2 well-samples the
   FWHM of 6.1 arcmin.

Caveats
-------
   The caveats to using these maps to measure reddening or extinction
   can be summarized as follows:

(1) Every effort has been made to remove both extragalactic sources
    and unresolved (Galactic) sources from the dust maps at |b| > 5 deg,
    and unconfused regions at lower latitudes.  Some sources will remain
    owing to confusion or unusual FIR colors.  No sources fainter than
    0.6 Jy at 60 microns are removed, although these are not significant
    contaminants.
(2) The IRAS satellite did not scan a strip amounting to 3% of the sky.
    In addition, we remove a circle of radius $2\degree$
    centered at (l,b)=(326.28 deg,+51.66 deg) contaminated by Saturn.
    These regions are replaced with DIRBE data, and have no point sources
    removed.
(3) The 100 micron passband for the DIRBE satellite is somewhat different
    than the 100 micron passband on IRAS.  For a  20 K blackbody, the
    difference is small, but the color temperature of sources is sometimes
    very different than that of the cirrus.  This sometimes results in
    artifacts in the temperature-correction term (on scales of one degree).
    Our remedy is described in Section 3.3 of the text.
(4) The LMC, SMC and M31 are not removed from the maps, nor are sources
    within their Holmberg radius.  Accurate reddenings THROUGH these
    galaxies is not possible since their temperature structure is not
    sufficiently resolved by DIRBE.  Typical reddenings TOWARDS these
    galaxies is estimated from the median dust emission in surrounding
    annuli: E(B-V)=0.075 mag for the LMC, 0.037 mag for the SMC, and
    0.062 mag for M 31.
(5) At low Galactic latitudes (|b| < 5 deg), most contaminating sources
    have not been removed from the maps, and the temperature structure
    of the Galaxy is not well resolved.  Furthermore, no comparisons between
    our predicted reddenings and observed reddening have been made in these
    regions.  Thus, our predicted reddenings here should not be trusted,
    though inspection of the maps might be of some use.
(6) The normalization of the dust column density to reddening has a formal
    uncertainty of 10\%.
(7) Should one wish to change the DIRBE/IRAS dust zero-point to be consistent
    with the Burstein-Heiles maps, we suggest subtracting 0.020 mag in E(B-V).

Extinction in Different Bandpasses
----------------------------------
   Assuming an R_V=3.1 extinction curve, the dust maps should be multiplied
   by the value in the final column to determine the extinction in a given
   passband.  The standard optical-IR bandpasses are represented by the CTIO
   and UKIRT transmission curves.  For further details, see Appendix B of
   the text.

   Filter name       LamEff  A/A(V)   A/E(B-V
   ----------------  ------  -------  -------
   Landolt U           3372    1.664    5.434
   Landolt B           4404    1.321    4.315
   Landolt V           5428    1.015    3.315
   Landolt R           6509    0.819    2.673
   Landolt I           8090    0.594    1.940
   CTIO U              3683    1.521    4.968
   CTIO B              4393    1.324    4.325
   CTIO V              5519    0.992    3.240
   CTIO R              6602    0.807    2.634
   CTIO I              8046    0.601    1.962
   UKIRT J            12660    0.276    0.902
   UKIRT H            16732    0.176    0.576
   UKIRT K            22152    0.112    0.367
   UKIRT L'           38079    0.047    0.153
   Gunn g              5244    1.065    3.476
   Gunn r              6707    0.793    2.590
   Gunn i              7985    0.610    1.991
   Gunn z              9055    0.472    1.540
   Spinrad R           6993    0.755    2.467
   APM b_J             4690    1.236    4.035
   Stromgren u         3502    1.602    5.231
   Stromgren b         4676    1.240    4.049
   Stromgren v         4127    1.394    4.552
   Stromgren beta      4861    1.182    3.858
   Stromgren y         5479    1.004    3.277
   Sloan u'            3546    1.579    5.155
   Sloan g'            4925    1.161    3.793
   Sloan r'            6335    0.843    2.751
   Sloan i'            7799    0.639    2.086
   Sloan z'            9294    0.453    1.479
   WFPC2 F300W         3047    1.791    5.849
   WFPC2 F450W         4711    1.229    4.015
   WFPC2 F555W         5498    0.996    3.252
   WFPC2 F606W         6042    0.885    2.889
   WFPC2 F702W         7068    0.746    2.435
   WFPC2 F814W         8066    0.597    1.948
   DSS-II g            4814    1.197    3.907
   DSS-II r            6571    0.811    2.649
   DSS-II i            8183    0.580    1.893

