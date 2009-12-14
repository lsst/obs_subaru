#
# Convert Medium Deep Survey .mle+.mob files to a Yanny param file, as read
# by param2Chain.
#
# E.g.  MDS2par ux712i.par ux712.mle ux712.mob 1
# to write the filter==1 data to ux712i.par
#
proc MDS2par {args} {
   set old_mle 0;			# an old style mle file?
   set all_fields 0;			# output all fields?
   
   set opts [list \
		 [list [info level 0] \
	    "Convert a pair of MDS .mle/mod files to a Yanny parameter file"] \
		 {<outfile> STRING "" outfile "Output file"} \
		 {<mlefile> STRING "" mlefile "Input .mle file"} \
		 {<mobfile> STRING "" mobfile "Input .mob file"} \
		 {<filter> INTEGER 0 filter "desired filter (0 or 1)"} \
		 {-all CONSTANT 1 all_fields "Write all fields to <outfile>"} \
		 {-old CONSTANT 1 old_mle "old-style .mle file"} \
		 {-n1 INTEGER 1 n1 "first line to read"} \
		 {-n2 INTEGER -1 n2 "last line to read (if < 0, read to EOF)"}\
		 {-id0 STRING "" id0 \
 "Assume that first field name ends \"\$id0\", and add (field - id0)*1000 to
 all ids. E.g. -id0 b would give object 23 in field u26xd an id of 2023"}\
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![regexp {^[12]$} $filter] {
      error "Please choose filter 1 or 2"
   }

   if {$id0 != ""} {
      set i [char_to_int $id0]
      if {$i > 0} {
	 set id0 [expr $i + 10]
      }
   }

   set mle [open $mlefile r]
   if [catch {set mob [open $mobfile r]} msg] {
      close $mle
      return -code error -errorinfo $msg
   }

   set out [open $outfile w]

   if $old_mle {
      foreach in "mle mob" {
	 loop i 0 13 {
	    set line [gets [set $in]]
	    puts $out "# $line"
	 }
      }
      puts $out ""
   }

   if $all_fields {
      puts $out "
 typedef struct {
   int iseq;	# sequential number (brightness order) of image in the field
   int ic;	# Filter#
   int mf;	# model# (0=Star 1-Disk, 2-Bulge 3-Disk+Bulge)
   int nfit;	# number of fitted parameters
   int ig;	# group (CCD chip) number (PC=1 WFC=2,3,4)
   int no;	# mask id (find location order)
   int ntot;	# total number of pixels in selected region
   int npix;	# total number of pixels in object above 1-sigma
   double fc;	# log_e likelihood function   ( 1.41 chi^2 ) at minimum
   double w1;	# gradient estimate of the fit at the minimum
   double w2;	# number of Likelihood function evaluations
   double w3;	# number of significant digits of fit
   int ier;	# IMSL status of minimization (saddle, etc.)
   double pe1;	# exponent of 1st component 2=Gaussian 1=exponential (Disk)
   double pe2;	# exponent of 2nd component 0.25=deVaucouleurs (Bulge)
   double sec;	# second CPU for convergence
   double ccx;	# X coordinate of centroid from MLE Model Pixels
   double cxe;	# error in ccx Pixels
   double ccy;	# Y coordinate of centroid from MLE Model Pixels
   double cye;	# error in ccy Pixels
   double sk;	# MLE model sky magnitude
   double ske;	# error in sk
   double gm;	# total MLE Model magnitude
   double gme;	# error in gm
   double hlrg;	# log_e half light radius of MLE Model
   double hlrge; # error in hlrg
   double pa;	# Orientation (East from North) of MLE Model radians
   double pae;	# error in pa radians
   double ard;	# Disk axis ratio of MLE Model
   double arde;	# error in ard
   double btt;	# Bulge to Total ratio of MLE Model
   double btte;	# error in btt
   double arb;	# Bulge axis ratio of MLE Model
   double arbe;	# error in arb
   double hlfg;	# log_e ratio of Bulge/Disk half light radius of MLE Model
   double hlfge; # error in hlfg
   double pad;	# Orientation offset (set to zero) radians
   double pade;	# error in pad radians
   double cxd;	# X centroid offset Pixels (set to zero)
   double cxde;	# error in cxd Pixels 
   double cyd;	# Y centroid offset Pixels (set to zero)
   double cyde;	# error in cyd Pixels
   double snril; # log_10 integrated Signal to Noise ratio within 1-sigma isophote
   double pfac;	# inverse fraction of mle model within region
   double flsta; # Star-galaxy Likelihood Ratio-Test (only if hlr < 1 pixel)
   double flsdb; # Disk-Bulge  Likelihood Ratio-Test
   double flbtd; # Disk+Bulge  Likelihood Ratio-Test (only if hlr > 2 pixel)
   int nio;	# size of square region read for analysis (nio,nio) Pixels
   int iclass;	#  0 = no-object  1 = object 2 = galaxy 3 = star;       4 = disk   5 = bulge  6 = disk+bulge
   double magin; # Model Flux within region fitted to total Modal Flux as a mag
   double magres; # (Integrated observed image/Integrated Model flux) as a magnitude
   char class\[10\];	# character name of mle classification

   char mdsid\[10\];	# MDS field identification
   char MLE\[10\];	# keyword in output .dat files

   double fr1;	# fraction of object pixels in region which are above 1-sigma
   double fr2;	# fraction of object flux   contained in pixels above 1-sigma
   int ntot_mob; # total number of pixels in selected region
   double sk_mob;	# observed sky magnitude estimate
   double cxm;	# X coordinate of centroid of object pixels above 1-sigma
   double cym;	# Y coordinate of centroid of object pixels above 1-sigma
   double gm_mob; # 1-sigma isophotal magnitude
   double hlrg_mob; # log_e half light radius estimate from observed moments
   double pa_mob; # position angle (anti-clockwise from +Y) from observed moments
   double ar;	# axis ratio estimated from observed moments
   double rm1;	# log_e of observed first radial moment  
   double cmag;	# completeness magnitude (from noise model: OK differentially)
   double ra;	# J2000 Right Ascension (degrees) from observed centroid
   double dec;	# J2000 Declination (degrees) from observed centroid
   double snril_mob; # log_10 integrated Signal to Noise ratio
   int nsnp;	# pixels above 5-sigma
   int nhot;	# pixels assumed to be \"hot\", and removed
   double sumle; # Integrated sum log_e ( one/sqrt(2 pi)/sigma(rms error) )
   double pswm;	# pixel scale in arc-seconds
   double hlmx;	# log_e maximum half-light radius imposed on the MLE model
   double spg;	# PSF subpixelation 
   double sph;	# PSF subpixelation for high resolution center if used.
   int i0;	# Lower limit in X of sub-region selected for Analysis
   int i1;	# Upper limit in X of sub-region selected for Analysis
   int i0;	# Lower limit in Y of sub-region selected for Analysis
   int i1;	# Upper limit in Y of sub-region selected for Analysis
   char mdsid_mob\[10\]; # MDS field identification
   char MOB\[10\];	# keyword in output .dat files
 } MDS;
"
   } else {
      puts $out "
 typedef struct {
   int iseq;	# sequential number (brightness order) of image in the field
   int ic;	# Filter#
   int mf;	# model# (0=Star 1-Disk, 2-Bulge 3-Disk+Bulge)
   int ig;	# group (CCD chip) number (PC=1 WFC=2,3,4)
   double pe1;	# exponent of 1st component 2=Gaussian 1=exponential (Disk)
   double pe2;	# exponent of 2nd component 0.25=deVaucouleurs (Bulge)
   double ccx;	# X coordinate of centroid from MLE Model Pixels
   double cxe;	# error in ccx Pixels
   double ccy;	# Y coordinate of centroid from MLE Model Pixels
   double cye;	# error in ccy Pixels
   double gm;	# total MLE Model magnitude
   double gme;	# error in gm
   double hlrg;	# log_e half light radius of MLE Model
   double hlrge; # error in hlrg
   double pa;	# Orientation (East from North) of MLE Model radians
   double pae;	# error in pa radians
   double ard;	# Disk axis ratio of MLE Model
   double arde;	# error in ard
   double btt;	# Bulge to Total ratio of MLE Model
   double btte;	# error in btt
   double arb;	# Bulge axis ratio of MLE Model
   double arbe;	# error in arb
   double hlfg;	# log_e ratio of Bulge/Disk half light radius of MLE Model
   double hlfge; # error in hlfg
   double snril; # log_10 integrated Signal to Noise ratio within 1-sigma isophote
   double flsta; # Star-galaxy Likelihood Ratio-Test (only if hlr < 1 pixel)
   double flsdb; # Disk-Bulge  Likelihood Ratio-Test
   double flbtd; # Disk+Bulge  Likelihood Ratio-Test (only if hlr > 2 pixel)
   int iclass;	#  0 = no-object  1 = object 2 = galaxy 3 = star;       4 = disk   5 = bulge  6 = disk+bulge
   char class\[10\];	# character name of mle classification

   char mdsid\[10\];	# MDS field identification

   double ra;	# J2000 Right Ascension (degrees) from observed centroid
   double dec;	# J2000 Declination (degrees) from observed centroid
 } MDS;
"
   }
   puts $out ""

   set lineno 0
   while {[gets $mle mle_line] >= 0 && [gets $mob mob_line]} {
      incr lineno

      if {$lineno < $n1} {
	 continue;
      }

      regexp {([0-9]?[0-9])([0-9])$} [lindex $mle_line 1] foo ic mf

      if {$ic != $filter} {
	 continue;
      }
      #
      # If requested, make the IDs in different fields unique
      #
      if {$id0 != ""} {
	 set id [lindex $mle_line 0]
	 regexp {.$} [lindex $mle_line 51] field_id
	 set i [char_to_int $field_id]
   
	 if {$i < 0} {
	    set i $field_id
	 } else {
	    incr i 10
	 }

	 incr id [expr 1000*($i - $id0)]
	 set mle_line [lreplace $mle_line 0 0 $id]
      }
      
      set mle_line [join [lreplace $mle_line 1 1 "$ic $mf"]]
      regsub {stelar} $mle_line stellar mle_line
      if $all_fields {
	 puts $out "MDS [join $mle_line { }] \\"
	 puts $out [lrange $mob_line 3 end]
      } else {
	 puts $out "MDS [lrange $mle_line 0 2] [lrange $mle_line 4 4] [lrange $mle_line 13 14] [lrange $mle_line 16 19] [lrange $mle_line 22 35] [lrange $mle_line 42 42] [lrange $mle_line 44 46] [lrange $mle_line 48 48] [lrange $mle_line 51 52] [lrange $mob_line 15 16]"	     
      }

      if {$n2 > 0 && $lineno > $n2} {
	 break;
      }
   }

   close $mle; close $mob; close $out
}

#
# Return the value of a character, with a == 0
#
proc char_to_int {c} {
   return [string first $c "abcdefghijklmnopqrstuvwxyz"]
}

#
# Convert a Yannified MDS.mle file into a truth table, as read by photo
#
# The infile can be a chain, to avoid having to repeatedly read param files;
# this chain must have elements "ra", "dec", and either "gm" or "R"
#
# e.g.
#   set_run 745 1
#   param2Truth 303 ux712i.par ux712.truth 25  iseq iclass hlrg pa btt
# to write a truth table for run 745, column 1, field 303
#
# This truth table would then be specified to photo as:
#   truth_file		ux712.truth
#   truth_format	%g %g %g  %g %g %*g %*g %*g
#   truth_cols 		1 2 3  4 5
#
# If you also specify
#    write_test_info	1
# the iseq and iclass values for each matched object will appear as
# $objcIo.test->true_id and $objcIo.test->true_type in the outputs.
#
proc param2Truth {field infile outfile maglim args} {
   global openit

   if ![info exists openit(col)] {
      error "Please use set_run and then try again"
   }
   regsub {^0*} $openit(run) "" run
   set camCol $openit(col)

   if {$outfile == ""} {
      set out stdout
   } else {
      set out [open $outfile w]
   }
   #
   # Write header
   #
   puts $out "#"
   puts $out "# Infile: $infile"
   puts $out "# Run $run  camCol $camCol  field $field"
   puts $out "# Maglim: $maglim"
   puts $out "#"
   set outline "# row col mag "
   foreach a $args {
      append outline " $a"
   }
   puts $out $outline
   puts $out "#"
   flush $out
   #
   # To work; read .par file
   #
   if [file exists $infile] {
      set objChain [param2Chain $infile hdr]
   } elseif [regexp {^h[0-9]+$} $infile] {
      set objChain $infile
   } else {
      error "No such file: $infile"
   }
   #
   # See what sort of magnitude to use
   set sch [schemaGetFromType [exprGet $objChain.type]]
   foreach el "gm R" {
      if {[keylget sch $el foo]} {
	 set magType $el
	 break;
      }
   }
   if ![info exists magType] {
      if {$objChain != $infile} {
	 chainDestroy $objChain genericDel
      }
      error "I cannot find a magnitude in the schema"
   }

   set nobj [chainSize $objChain]
   loop i 0 $nobj {
      puts -nonewline "$i      \r"
      set obj [chainElementGetByPos $objChain $i]
      set id [exprGet $obj.iseq]
      if {$out == "stdout"} {
	 puts -nonewline stdout "$id\r"; flush stdout
      }

      set ra [exprGet $obj.ra]
      set dec [exprGet $obj.dec]
      set mag [exprGet $obj.$magType]

      if {$maglim > 0 && $mag > $maglim} {
	 handleDel $obj      
	 continue
      }

      if [catch {
	 set vals [concat [where_in_run -camCol $camCol $run $ra $dec]]
      } msg] {
	 echo $msg
	 handleDel $obj      
	 continue;
      }
      if {$vals == -1} {
	 handleDel $obj      
	 continue;
      }
      
      set f_camCol [lindex $vals 1]
      set f_field [lindex $vals 2]
      set row [lindex $vals 3]
      set col [lindex $vals 4]
      
      if {$f_field != $field || $f_camCol != $camCol} {
	 handleDel $obj      
	 continue;
      }

      set outline [format "%.1f %.1f %.2f " $row $col $mag]
      foreach a $args {
	 append outline " [exprGet $obj.$a]"
      }
      
      puts $out $outline; flush $out
      echo $outline

      handleDel $obj      
   }
   

   if {$objChain != $infile} {
      chainDestroy $objChain genericDel
   }
   
   if {$out == "stdout"} {
      echo ""
   } else {
      close $out
   }

   return $nobj
}

###############################################################################
#
# Prune a truth table, removing duplicates
#
proc prune_truth {args} {
   set force 0
   
   set opts [list \
		 [list [info level 0] "Open a photo truth table"] \
		 {<outfile> STRING "" outfile "Truth table file"} \
		 {<truth_table> STRING "" truth_table "Truth table file"} \
		 {{[format]} STRING "%g %g %g  %g %g %g  %g %g" format \
		      "Format to read lines in truth table.
 If a number, equivalent to that many %gs"} \
		 {{[cols]} STRING "" cols \
		      "Which columns to read from file (default: 1..n)"} \
		 {-tol DOUBLE 2 tol "Tolerance for match"} \
		 {-binning INTEGER 2 binning "Binning to use in row/column"} \
		 {-force CONSTANT 1 force "Allow <outfile> to be overwritten"}\
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {!$force && [file exists $outfile]} {
      error "File $outfile already exists"
   }

   open_truthtable $truth_table $format $cols -bin $binning

   if [catch {set tfd [open $truth_table r]} msg] {
      infofileFini
      error $msg
   }
   if [catch {set ofd [open $outfile w]} msg] {
      catch {close $tfd}
      infofileFini
      error $msg
   }

   while {[gets $tfd line] != -1} {
      if [regexp {^ *#} $line] {
	 puts $ofd $line
	 continue;
      }

      set rowc [lindex $line 0]
      set colc [lindex $line 1]
      set id [lindex $line 3]

      set vals [lookup_truthtable $rowc $colc $tol]

      set match_id [lindex $vals 0]
      if {$match_id == 0} {	# no match
	 continue;
      } elseif {$match_id != $id} {	# matched the wrong object
	 continue
      }

      puts $ofd $line
   }
   close $tfd; close $ofd

   infofileFini
}

#
# Having read a .par file, print the listed properties of the desired object
#
proc p_mds {objChain n args} {
   set obj [chainElementGetByPos $objChain [expr $n-1]]
   foreach a $args {
      echo [format "%-8s %s" $a [exprGet $obj.$a]]
   }
   handleDel $obj
}

###############################################################################
#
# Display an object with a given ID number in some MDS field
#
proc open_mds_truthtable {args} {
   set opts [list \
		 [list [info level 0] "Open a photo truth table"] \
		 {<file> STRING "" file "Truth table file"} \
		 {-binning INTEGER 2 binning "Binning to use in row/column"} \
		 {-sao INTEGER 0 fsao \
	       "Display objects in table on this saoimage; don't open table"} \
	     ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   open_truthtable $file "unknown" -binning $binning -sao $fsao
}

alias close_mds_truthtable close_truthtable

proc mtv_mds {args} {
   set noreg 0; set fullres 0; set skysub 0

   set opts [list \
		 [list [info level 0] \
	    "Show an object from an MDS field"] \
		 {<table> STRING "" _table "Keyed list from openit"} \
		 {<filefmt> STRING "" filefmt "Format giving the filename"} \
		 {<id> INTEGER 0 id "MDS ID of desired object"}\
		 {<ig> INTEGER 0 ig "CCD object is found in"}\
		 {<ccx> DOUBLE 0 ccx "x-coordinate in CCD"}\
		 {<ccy> DOUBLE 0 ccy "x-coordinate in CCD"}\
		 {-angle DOUBLE 0 angle "rotate image by <angle>"} \
		 {-size INTEGER 31 size "Size of desired stamp"} \
		 {-id0 STRING "" id0 \
 "Assume that first field name ends \"\$id0\", and add (field - id0)*1000 to
 all ids. E.g. -id0 b would give object 23 in field u26xd an id of 2023"}\
		 {-sao INTEGER 0 fsao "Display region on this saoimage"}\
		 {-zoom INTEGER 1 zoom "How much to zoom display"}\
		 {-noregion CONSTANT 1 noreg "don't return a region"}\
		 {-full CONSTANT 1 fullres "Use the full resolution images"} \
		 {-subtractSky CONSTANT 1 skysub "Subtract the median sky?"} \
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table

   if {$id == 0} {			# no match
      echo No match in MDS dataset
      if $noreg {
	 return ""
      } else {
	 set reg [regNew 1 1]; regClear $reg
	 
	 return $reg
      }
   }
   #
   # Map that ID to a field name.  We've mapped e.g. u26xa to 10*1000 + id
   # if id0 == "1"
   #
   set field_id [expr int($id/1000)]
   switch -regexp $id0 {
      {^Groth$} {
	 #
	 # Ratnatunga simply omits field 7, calling Groth's field 8 `7'.
	 # Furthermore, Ratnatunga's u26x1 is Groth's V29. Sigh
	 #
	 if {$field_id <= 2} {
	    incr field_id 29
	 } elseif {$field_id <= 5} {
	    incr field_id 1
	 } else {
	    incr field_id 2
	 }
      }
      {^[0-9]+$} {
	 incr field_id $id0
	 if {$field_id >= 10} {
	    set field_id [format "%c" [expr $field_id - 10 + 97]];# 97 == 'a'
	 }
      }
      {.*} {
	 ;
      }
   }

   set soft_bias 1000
   #
   # Find the file.
   #
   set hstfile [format $filefmt $field_id $ig]
   
   set regCam [regReadAsFits [regNew] $hstfile]
   set nrow [exprGet $regCam.nrow]; set ncol [exprGet $regCam.ncol]

   
   if $fullres {
      set rc $ccy; set cc $ccx
      switch $ig {
	 1 { set angle [expr $angle - 90] }
	 2 { set angle [expr $angle + 180] }
	 3 { set angle [expr $angle + 90] }
	 4 { ; }
      }
   } else {
      #
      # Find the pixel centre.  We know which chip it was in and its centre
      #
      set vals [find_object_in_binned_mds $ig $ccx $ccy]
      set rc [lindex $vals 1]; set cc [lindex $vals 0]
   }

   set rsize $size; set csize $size;
   set rc [expr int($rc - $rsize/2)]; set cc [expr int($cc - $csize/2)]
   set rrc 0; set rcc 0;

   if {$rc < 0} {
      incr rsize $rc
      set rrc [expr -$rc]
      set rc 0; 
   }
   if {$rc > $nrow - $rsize} {
      set rsize [expr $nrow - $rc]
   }
   if {$cc < 0} {
      incr csize $cc
      set rcc [expr -$cc]
      set cc 0; 
   }
   if {$cc > $ncol - $csize} {
      set csize [expr $ncol - $cc]
   }
   #
   # get desired subregion
   #
   set Result [regNew $size $size]; regIntSetVal $Result $soft_bias

   set subreg [subRegNew $regCam $rsize $csize $rc $cc]
   set rResult [subRegNew $Result $rsize $csize $rrc $rcc]
   regPixCopy $subreg $rResult
   regIntConstAdd $rResult $soft_bias

   regDel $rResult
   regDel $subreg
   regDel $regCam
   #
   # Subtract the sky?
   #
   if $skysub {
      regStatsFromQuartiles $Result -cmedian sky
      set sky [expr int($sky + 1000.5) - 1000]
      regIntConstAdd $Result [expr $soft_bias - $sky ]
   }
   #
   # Rotate if desired
   #
   #set angle 0;				# RHL XXX
   if {$angle != 0} {
      set old $Result
      # don't use -sinc to rotate, as the PSF is undersampled
      set Result [regIntRotate $old $angle -bkgd $soft_bias]
      regDel $old
   }

   if {$fsao > 0} {
      display_region $Result 0 0 $fsao
      if {$zoom > 1} {
	 if {$zoom%2 != 0} {
	    set zoom [expr $zoom/2]
	 }
	 if {$zoom > 1} {
	    saoZoom -s $fsao $zoom
	 }
      }
   }

   if $noreg {
      regDel $Result
      return ""
   } else {
      return $Result
   }
}

#
# MDS makes the WFPC image available as an 800x800 mosaic; given
# an unbinned chip ID and centre, return the corresponding pixel
# in this mosaic
#
# This algorithm comes from Kavan Ratnatunga's mds_cgi.f, to whom thanks.
#
proc find_object_in_binned_mds {ig cx cy} {
   array set cost [list 1 0    2 -1   3  0   4 1]
   array set sint [list 1 1    2  0   3 -1   4 0]
   array set x0   [list 1 413  2 423  3 375  4 376]
   array set y0   [list 1 389  2 414  3 418  4 378]

   if {$ig == 1} {
      set scale 0.228
   } else {
      set scale 0.5
   }

   set ix [expr $x0($ig) + $scale*($cx*$cost($ig) - $cy*$sint($ig))]
   set iy [expr $y0($ig) + $scale*($cx*$sint($ig) + $cy*$cost($ig))]

   return "$ix $iy"
}

#
# Here's a wrapper for use from mtv_objc_list
#
proc show_groth {args} {
   global table objcIo groth_root

   set fullres 0; set quiet 0; set noreg 0
   set opts [list \
		 [list [info level 0] \
	    "Display an object from the Groth strip images.

 The match to the groth strip catalogues is via a truth table or
 chain in the table list (see openit's -truth argument)
 "] \
		 {-s INTEGER 2 fsao "fsaoimage to use"} \
		 {-angle DOUBLE -55 angle "Angle to rotate image by"}\
		 {-size INTEGER 41 size "Size of non-rotated image"} \
		 {-tol INTEGER 5 tol "Tolerance in match, SDSS pixels"} \
		 {-full CONSTANT 1 fullres "Use the full resolution images"} \
		 {-quiet CONSTANT 1 quiet "Don't complain about bad matches"} \
		 {-noregion CONSTANT 1 noreg "don't return a region"}\
		]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set rowc [exprGet $objcIo.objc_rowc]
   set colc [exprGet $objcIo.objc_colc]

   if ![keylget table truth_table ttable] {
      error "You haven't provided a truth table or chain"
   }
   

   if {![regexp {^h[0-9]+$} $ttable] ||
       [catch { set type [handleType $ttable] }]} {# not a handle
      set vals [lookup_truthtable $rowc $colc $tol]
      set id [lindex $vals 0]
      set type [lindex $vals 1]
      set ig [lindex $vals 2]
      set ccx [lindex $vals 3]
      set ccy [lindex $vals 4]
   } else {
      if {$type != "CHAIN"} {
	 error "$truthfile is not a CHAIN"
      }

      set ch [chainSearch $ttable "{iseq == [exprGet $objcIo.firstId]}"]
      set nch [chainSize $ch]
      if {$nch == 0} {
	 set id 0;			# no match
      } else {
	 if {$nch > 1} {
	    if !$quiet {
	       echo "Found $nch matches for iseq == $hst_id; using first"
	    }
	 }
	 set h [chainElementGetByPos $ch 0]
	 
	 set id [exprGet $h.iseq]
	 set ig [exprGet $h.ig]
	 set ccx [exprGet $h.ccx]
	 set ccy [exprGet $h.ccy]
	 
	 handleDel $h
      }
      chainDel $ch
   }

   if {$id == 0} {			# no match
      if !$quiet {
	 echo No match in MDS dataset
      }
      return ""
   }

   global groth_reg
   if [info exists groth_reg] {
      catch { regDel $groth_reg }
      unset groth_reg
   }

   set zoom [expr int(512/$size)]
   if $fullres {
      set groth_reg \
	  [mtv_mds table $groth_root/fullimages/v%02dh_%d.fits \
	       $id $ig $ccx $ccy \
	       -id0 Groth -angle $angle -size $size -subtractSky -full \
	       -sao $fsao -z $zoom]
   } else {
     set groth_reg \
	 [mtv_mds table $groth_root/images/u26x%sv4.m04.fit $id $ig $ccx $ccy \
	      -id0 1 -angle $angle -size $size -subtractSky \
	      -sao $fsao -z $zoom]
   }

   if $noreg {
      regDel $groth_reg; unset groth_reg
   }
}

###############################################################################
#
# Here's a `select' proc to show MDS matches via e.g. mtv_objc_list
#
proc sel_mds {o {b 2}} {
   global OBJECT1

   set flgs [exprGet $o.objc_flags]
   #
   # Reject bright and blended objects
   #
   if {$flgs & $OBJECT1(BRIGHT)} {
      return 0
   }

   if {($flgs & $OBJECT1(BLENDED)) && !($flgs & $OBJECT1(NODEBLEND))} {
      return 0
   }
   #
   # OK, does it have an MDS match?
   #
   set vals [lookup_truthtable [exprGet $o.objc_rowc] [exprGet $o.objc_colc]]
   set id [lindex $vals 0]

   if {$id == 0} {
      return 0
   }

   catch { show_groth -quiet -s 3 -full -size 81 }

   return 1
}

###############################################################################
#
# Produce a summary of matches of a tsObj file with a truth table
#
proc match_truth {args} {
   set opts [list \
		 [list [info level 0] \
	   "Produce a summary of matches of a tsObj file with a truth table
 Usually only the parameters in the truth table are output, but you can
 specify extra fields with -values; in this case you must provide a .par
 file to read them from (and be prepared to wait longer)"] \
		 {<outfile> STRING "" outfile "Output file"} \
		 {{[_table]} STRING "table" _table "table from e.g. openit"} \
		 {-parfile STRING "" parfile ".par file to read for -values"}\
		 {-n1 INTEGER 1 n1 "first line to read"} \
		 {-n2 INTEGER -1 n2 "last line to read (if < 0, read to EOF)"}\
		 {-values STRING "" values "extra fields from .par file"}\
		 {-tol DOUBLE 5 tol "Tolerance in positional match"}\
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table
   global objcIo OBJECT1

   if ![keylget table truth_table ttable] {
      error "Your table has no truth table file"
   }

   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }

   if {"[keylget table fileSchema]" == "OBJC_IO"} {
       set is_fpObjc 1
   } else {
       set is_fpObjc 0
   }
   #
   # Maybe read .par file for extra fields
   #
   if {$values != ""} {
      if {$parfile == ""} {
	 error "You must provide a parfile if you ask for -values"
      }
      set objChain [param2Chain $parfile hdr]

      set h [chainElementGetByPos $objChain 0]
      foreach v $values {
	 if [catch { exprGet $h.$v } msg] {
	    handleDel $h
	    chainDestroy $objChain genericDel
	    error $msg
	 }
      }
      handleDel $h
   }

   set fd [open $outfile "w"]
   puts $fd "#"
   puts $fd "# Run [keylget table run]"
   puts $fd "# camCol [keylget table camCol]"
   puts $fd "# field [keylget table field]"
   puts $fd "#"
   puts $fd "# Extra fields: $values"
   puts $fd "#"

   loop i $n1 [expr $n2 + 1] {
      catch {catObjDel $objcIo -deep};	# it may not exist
      set objcIo [objcIoRead table $i]
      
      puts -nonewline "[exprGet $objcIo.id]   \r"; flush stdout

      set flgs [exprGet $objcIo.objc_flags]
      #
      # Reject bright and blended objects
      #
      if {($flgs & ($OBJECT1(BRIGHT) | $OBJECT1(EDGE))) ||
	  (($flgs & $OBJECT1(BLENDED)) && !($flgs & $OBJECT1(NODEBLEND)))} {
	 continue;
      }
      #
      # Look for a match
      #
      set truth_vals \
	  [lookup_truthtable \
	       [exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc] $tol]
      set hst_id [lindex $truth_vals 0]
      set hst_type [lindex $truth_vals 1]
      set hst_ig [lindex $truth_vals 2]
      set hst_ccx [lindex $truth_vals 3]
      set hst_ccy [lindex $truth_vals 4]

      if {$hst_id == 0} {		# no match
	 continue;
      }
      #
      # Is anything extra required?
      #
      set extras {}
      if {$values != ""} {
	 set ch [chainSearch $objChain "{iseq == $hst_id}"]
	 set nch [chainSize $ch]
	 if {$nch == 0} {
	    echo "Failed to find iseq == $hst_id"
	    foreach v $values {
	       lappend extras 0
	    }
	 } else {
	    if {$nch > 1} {
	       echo "Found $nch matches for iseq == $hst_id; using first"
	    }
	    set h [chainElementGetByPos $ch 0]

	    foreach v $values {
	       lappend extras [exprGet $h.$v]
	    }
	    handleDel $h
	 }
	 chainDel $ch
      }
      #
      # Measure something from the groth data
      #
      global groth_reg groth_root
      if [info exists groth_reg] {
	 catch { regDel $groth_reg }
	 unset groth_reg
      }
      
      set groth_reg \
	  [mtv_mds table $groth_root/fullimages/v%02dh_%d.fits \
	       $hst_id $hst_ig $hst_ccx $hst_ccy -id0 Groth -subtractSky \
	       -sao -1 -size 71 -full]
      
      #echo ID $i; mtv $groth_reg;			# XXX

      lappend extras "GrothPetro:"
      if [catch {
	 set pvals [getPetrosian $groth_reg -scale 0.25]
      }] {
	 lappend extras -1
	 lappend extras -1
	 lappend extras -1
      } else {
	 lappend extras [keylget pvals petroRad] 
	 lappend extras [keylget pvals petroR50] 
	 lappend extras [keylget pvals petroR50Err]
      }
      #
      # Write output record
      #
      set line "$hst_id $hst_type [exprGet $objcIo.id]"
      if {0 && $is_fpObjc} {
	 lappend line [exprGet (int)$objcIo.type]
	 lappend line "[exprGet $objcIo.rowc] [exprGet $objcIo.colc]"
	 lappend line [exprGet $objcIo.flags]
	 lappend line [exprGet $objcIo.flags2]
	 loop i 0 [exprGet $objcIo.ncolor] {
	    lappend line [exprGet $objcIo.color<$i>->flags]
	    lappend line [exprGet $objcIo.color<$i>->flags2]
	    lappend line [exprGet $objcIo.color<$i>->type]
	    lappend line [exprGet $objcIo.color<$i>->psfCounts]
	    lappend line [exprGet $objcIo.color<$i>->counts_exp]
	    lappend line [exprGet $objcIo.color<$i>->exp_L]
	    lappend line [exprGet $objcIo.color<$i>->deV_L]
	    lappend line [exprGet $objcIo.color<$i>->r_exp]
	    lappend line [exprGet $objcIo.color<$i>->r_expErr]
	    lappend line [exprGet $objcIo.color<$i>->r_deV]
	    lappend line [exprGet $objcIo.color<$i>->r_deVErr]
	 }
      } else {
	 lappend line [exprGet (int)$objcIo.objc_type]
	 lappend line "[exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc]"
	 lappend line [exprGet $objcIo.objc_flags]
	 lappend line [exprGet $objcIo.objc_flags2]
	 loop i 0 [exprGet $objcIo.ncolor] {
	    lappend line [exprGet $objcIo.flags<$i>]
	    lappend line [exprGet $objcIo.flags2<$i>]
	    lappend line [exprGet (int)$objcIo.type<$i>]
	    lappend line [exprGet $objcIo.psfCounts<$i>]
	    lappend line [exprGet $objcIo.counts_exp<$i>]	    
	    lappend line [exprGet $objcIo.exp_L<$i>]
	    lappend line [exprGet $objcIo.deV_L<$i>]
	    lappend line [exprGet $objcIo.r_exp<$i>]
	    lappend line [exprGet $objcIo.r_expErr<$i>]
	    lappend line [exprGet $objcIo.r_deV<$i>]
	    lappend line [exprGet $objcIo.r_deVErr<$i>]
	 }
      }
      puts $fd [join [concat $line " " [lrange $truth_vals 2 end] $extras]]
      flush $fd
   }

   if [info exists objChain] {
      chainDestroy $objChain genericDel
   }
   
   close $fd
}


###############################################################################
#
# Write summary tsObj/fpAtlas image files from matches of a tsObj file with
# a truth table
#
proc write_matched_tsObj {args} {
   set family 0
   set opts [list \
		 [list [info level 0] \
 "Write summary tsObj/fpAtlas image files from matches of a tsObj file with
 a truth table.  idStr is either the identifying string for the tables, or
 the name of the list returned by open_mytables"]\
		 {<idStr> STRING "" idStr "String to identify output tables"} \
		 {{[_table]} STRING "table" _table "table from e.g. openit"} \
		 {-n1 INTEGER 1 n1 "first line to read"} \
		 {-n2 INTEGER -1 n2 "last line to read (if < 0, read to EOF)"}\
		 {-family CONSTANT 1 family \
		      "Write objcIo's entire family to the table"} \
		 {-fix_type STRING "" _fix_type \
 "Name of array indexed by HST ID, giving the correct type. If it's negative,
 ignore this object"} \
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table
   if {$_fix_type != ""} {
      upvar $_fix_type fix_type
   }
   global objcIo OBJECT1

   if ![keylget table truth_table ttable] {
      error "Your table has no truth table file"
   }

   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }

   if {"[keylget table fileSchema]" == "OBJC_IO"} {
       set is_fpObjc 1
   } else {
       set is_fpObjc 0
   }
   #
   # Open output files, unless the idStr is actually the name of a list
   # returned by open_mytables
   #
   upvar $idStr _fds
   if [info exists _fds] {		# no need to open it
      set opened_fds 0
      set fds $_fds
   } else {
      set opened_fds 1
      set fds [open_mytables -tsObj table $idStr]
   }
   #
   # Do we want family members?
   #
   if $family {
      set family "-children"
   } else {
      set family ""
   }
   #
   # Actually read tables
   #
   loop i $n1 [expr $n2 + 1] {
      catch {catObjDel $objcIo -deep};	# it may not exist
      set objcIo [objcIoRead table $i]
      
      puts -nonewline "[exprGet $objcIo.id]   \r"; flush stdout

      set flgs [exprGet $objcIo.objc_flags]
      #
      # Reject bright and blended objects
      #
      if {($flgs & ($OBJECT1(BRIGHT) | $OBJECT1(EDGE))) ||
	  (($flgs & $OBJECT1(BLENDED)) && !($flgs & $OBJECT1(NODEBLEND)))} {
	 continue;
      }
      #
      # Look for a match
      #
      set truth_vals [lookup_truthtable \
		    [exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc]]
      set hst_id [lindex $truth_vals 0]
      set hst_type [lindex $truth_vals 1]
      #
      # Did we want to override the MDS type?
      #
      if [info exists fix_type($hst_id)] {
	 set hst_type $fix_type($hst_id)
	 if {$hst_type < 0} {		# bad match
	    set hst_id 0
	 }
      }

      if {$hst_id == 0} {		# no match
	 continue;
      }

      set primaryID [exprGet $objcIo.id]

      if {$family != ""} {
	 set objcIo [read_parent table $objcIo] 
      }
      #
      # Write record
      #
      eval write_mytables $family fds $objcIo table -id $primaryID \
	  -callback write_matched_tsObj_callback
   }
   #
   # Close tables if we opened them
   #
   if $opened_fds {
      close_mytables fds
   }
}

proc write_matched_tsObj_callback {objcIo} {
   global OBJECT2
   #
   # Put ID and type into the objcIo
   #
   uplevel 1 {
      if {[exprGet $objcIo.id] == $primaryID} {
	 handleSet $objcIo.firstId $hst_id
	 handleSet $objcIo.firstLambda $hst_type
      }
   }
}

#
# Here's an array just waiting to be passed to write_matched_tsObj via
# the -fix_type argument.  These are all brightish stars that MDS misclassifies
#
if ![info exists fix_groth_type] {
   array set fix_groth_type \
       [list \
	    6141  -1 \
	    10002 3 \
	    11196 3 \
	    12001 3 \
	    14001 3 \
	    19002 3 \
	    24154 3 \
	    26002 3 \
	    ]
}
###############################################################################
#
# Procs to call from mtv_objc_list
#
proc mtv_objc_list_proc_g {args} {
   if {[lindex $args 1] == "help"} {
      return {g[roth -size n -full]}
   }

   set fsao ""
   if {[lsearch $args "-sao"] >= 0} {
      set fsao [lindex $args [expr [lsearch $args "-sao"] + 1]]
   }
   
   if {$fsao == ""} {
      set fsao 3;		# use fsao 3, if it exists
      if {[lsearch [saoGetRegion] $fsao] < 0} {
	 set fsao 2
      }
   }
   
   eval show_groth -s $fsao [lrange $args 1 end]
}

proc mtv_objc_list_proc_G {args} {
   if {[lindex $args 1] == "help"} {
      return {G[roth -size n]}
   }

   set size ""
   if {[lsearch $args "-size"] < 0} {
      set size "-size 81"
   }

   eval mtv_objc_list_proc_g {g} -full $size [lrange $args 1 end]
}

proc mtv_objc_list_proc_k {args} {
   global objcIo table

   if {[lindex $args 1] == "help"} {
      return {k[pno]}
   }
   
   set rowc [exprGet $objcIo.objc_rowc]
   set colc [exprGet $objcIo.objc_colc]

   if ![keylget table truth_table ttable] {
      error "You haven't provided a truth table or chain"
   }
   
   set tol 10
   
   if {![regexp {^h[0-9]+$} $ttable] ||
       [catch { set type [handleType $ttable] }]} {# not a handle
      set vals [lookup_truthtable $rowc $colc $tol]

      set id [lindex $vals 0]
      set U [lindex $vals 1]
      set B [lindex $vals 2]
      set R [lindex $vals 3]
      set I [lindex $vals 4]

      if {$id == 0} {
	 echo No match
      } else {
	 echo $id $U $B $R $I
      }
   }
}

proc mtv_objc_list_proc_K {args} {
   global objcIo table

   if {[lindex $args 1] == "help"} {
      return {K[avan]}
   }
   
   set rowc [exprGet $objcIo.objc_rowc]
   set colc [exprGet $objcIo.objc_colc]

   if ![keylget table truth_table ttable] {
      error "You haven't provided a truth table or chain"
   }
   
   set tol 10
   
   if {![regexp {^h[0-9]+$} $ttable] ||
       [catch { set type [handleType $ttable] }]} {# not a handle
      set vals [lookup_truthtable $rowc $colc $tol]
      
      set id     [lindex $vals 0]

      if {$id == 0} {
	 echo No match
      } else {
	 set iclass [lindex $vals 1];	# class
	 set hlrg   [format %.3f [expr exp([lindex $vals 6])]];# r_e
	 set btt    [lindex $vals 8];	# bulge/total
	 echo $id $iclass $hlrg $btt
      }
   }
}

###############################################################################
#
# Read Robert Brunner/Andy Connolly's catalogues from the KPNO imaging of
# the Groth strip, returning a chain of KPNO_GROTHs
#
# The positions are offset by (drow, dcol)
#
typedef struct {
   int iseq;
   double ra;
   double dec;
   double U;
   double B;
   double R;
   double I;
} KPNO_GROTH

proc read_kpno {file {drow 0.0} {dcol 0.0}} {
   set fd [open $file "r"]
   
   set ch [chainNew KPNO_GROTH]

   set iseq 0
   while {[gets $fd line] >= 0} {
      if [regexp {^ *(\#.*)$} $line] {
	 continue;
      }

      set i -1
      set hr [lindex $line [incr i]]
      set min [lindex $line [incr i]]
      set sec [lindex $line [incr i]]
      set deg [lindex $line [incr i]]
      set dmin [lindex $line [incr i]]
      set dsec [lindex $line [incr i]]
      set U [lindex $line [incr i]]
      set B [lindex $line [incr i]]
      set R [lindex $line [incr i]]
      set I [lindex $line [incr i]]

      incr iseq;
      set ra [expr 15*($hr + ($min + $sec/60.0)/60.0)]
      set dec [expr $deg + ($dmin + $dsec/60.0)/60.0]

      set obj [genericNew KPNO_GROTH]
      foreach el "iseq ra dec U B R I" {
	 handleSet $obj.$el [set $el]
      }

      chainElementAddByPos $ch $obj
      handleDel $obj
   }

   close $fd
   
   return $ch
}

#
# Match the chain returned by read_kpno with the SDSS reductions of
# runs 1468/1469 columns 5/6 (or whatever you request)
#
proc match_kpno {args} {
   set opts [list \
		 [list [info level 0] \
 "Match the chain returned by read_kpno with the SDSS reductions of runs
 \$runs columns \$cols

 Note that this will include all objects that lie within the specified
 area; you must provide a truth table format if you want to know if they
 are actually present in the data
 "] \
		 {<outfile> STRING "" outfile "Output file"} \
		 {<chain> STRING "" ch "Chain as read by read_kpno"} \
		 {-runs STRING "1468 1469" runs \
		      "runs to search for matches"} \
		 {-cols STRING "5 6" cols \
		      "camera columns to search for matches"} \
		 {-truthFmt STRING "" truthFmt \
		      "Format to specify truth files; expects to be used as
    format \$truthFmt run camCol field
 "
		 } \
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set fd [open $outfile "w"]

   set tfile "";			# current truth file
   loop i 0 [chainSize $ch] {
      set obj [chainElementGetByPos $ch $i]

      foreach el "iseq ra dec U B R I" {
	 set $el [exprGet $obj.$el]
      }
      puts -nonewline "$iseq $ra $dec           \r"

      set f_run -9999; set f_camCol -9999; set f_field -9999;
      set row -9999; set col -9999; set matched -1

      foreach run $runs {
	 foreach camCol $cols {
	    if [catch {
	       set vals [concat [where_in_run -camCol $camCol $run $ra $dec]]
	    } msg] {
	       echo $msg
	       continue;
	    }

	    if {$vals != -1} {
	       set f_run [lindex $vals 0]
	       set f_camCol [lindex $vals 1]
	       set f_field [lindex $vals 2]
	       set row [lindex $vals 3]
	       set col [lindex $vals 4]

	       if {$truthFmt != ""} {
		  set ntfile [format $truthFmt $f_run $f_camCol $f_field]
		  if {$ntfile != $tfile} {
		     set tfile $ntfile
		     if ![file exists $tfile] {
			echo "$tfile doesn't exist"
			break
		     } else {
			open_truthtable $tfile "unknown"
		     }
		  }

		  set vals [lookup_truthtable $row $col]

		  if {[lindex $vals 0] == 0} {
		     set matched 0
		  } else {
		     set matched 1
		  } 
	       }

	       break;
	    }
	 }
      }

      puts $fd \
	  [format "%d %d %d %d %.3f %.3f  %d %.7f %.7f %.4f %.4f %.4f %.4f" \
	       $matched $f_run $f_camCol $f_field $row $col \
	       $iseq $ra $dec $U $B $R $I]
      flush $fd
   }

   echo ""
   close $fd
}

###############################################################################
#
# RHL devel code; remove as convenient
#
if ![info exists groth_root] {
   set groth_root /u/rhl/groth
}

proc openitt {field} {
   global openit groth_root

   regsub {^0*} $openit(run) "" run
   set tfile $groth_root/u26x-$run-$openit(col)-$field.truth
   if [file exists $tfile] {
      set ret [openit -truth $tfile $field]
   } else {
      set ret [openit $field]
   }
}

proc make_truth {r c f} {
   set_run -ts $r $c;
   param2Truth $f u26x.par u26x-$r-$c-$f.truth 24.5 iseq iclass ig ccx ccy gm hlrg snril btt
}

proc make_truth_kpno {r c f} {
   set_run -ts $r $c;
   set ch [read_kpno brunner.dat 2.5 3.5]
   param2Truth $f $ch kpno-$r-$c-$f.truth 24.5 iseq U B R I
   chainDestroy $ch genericDel
}

proc make_match {r c f} {
   global groth_root

   set_run -rerun 666 $r $c;
   set table [openit -truth $groth_root/u26x-$r-$c-$f.truth $f];
   match_truth u26x-$r-$c-$f.match table -par u26x.par -tol 5
}

proc make_match_kpno {r c f} {
   global groth_root

   set_run -ts $r $c;
   set table [openit -truth $groth_root/kpno-$r-$c-$f.truth $f];
   match_truth kpno-$r-$c-$f.match table
}

#
# Procs for use with mtv_objc_list to hand classify objects
#
proc write_classify {type {file "groth.classify"}} {
   global objcIo table
   
   set rowc [exprGet $objcIo.objc_rowc]
   set colc [exprGet $objcIo.objc_colc]

   set rcf [get_run_camCol_field table $objcIo]
   set run [lindex $rcf 0]
   set camCol [lindex $rcf 1]
   set field [lindex $rcf 2]

   set vals [lookup_truthtable $rowc $colc]
   set truth_id [lindex $vals 0]
   set truth_type [lindex $vals 0]

   set line "[format %-6s $type] $run $camCol $field [exprGet $objcIo.id] [exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc]  $truth_id  [exprGet $objcIo.objc_type] $truth_type"
   set fd [open $file "a"]
   puts $fd $line
   close $fd

   echo $line

   return $line
}

if 0 {
   proc mtv_objc_list_proc_gf {args} {
      if {[lindex $args 1] == "help"} {
	 return {gf[roth -full]}
      }
      
      eval show_groth -full [lrange $args 1 end]
   }
   
   proc mtv_objc_list_proc_L {args} {
      if {[lindex $args 1] == "help"} {
	 return {L{ist}}
      }
      
      echo Options:
      echo "L List  A AGN  E edge  G galaxy  I irregular  S star  U undo  X exp  V deV"
      echo "(undo => ignore previous estimate)"
   }
   
   
   proc mtv_objc_list_proc_A {args} {
      if {[lindex $args 1] == "help"} {
	 return {A{GN}}
      }
      
      write_classify AGN
   }
   
   proc mtv_objc_list_proc_E {args} {
      if {[lindex $args 1] == "help"} {
	 return {E{dge}}
      }
      
      write_classify edge
   }
   
   proc mtv_objc_list_proc_G {args} {
      if {[lindex $args 1] == "help"} {
	 return {G{alaxy}}
      }
      
      write_classify galaxy
   }
   
   proc mtv_objc_list_proc_I {args} {
      if {[lindex $args 1] == "help"} {
	 return {I{rr}}
      }
      
      write_classify irr
   }
   
   proc mtv_objc_list_proc_S {args} {
      if {[lindex $args 1] == "help"} {
	 return {S{tar}}
      }
      
      write_classify star
   }
   
   proc mtv_objc_list_proc_U {args} {
      if {[lindex $args 1] == "help"} {
	 return {U{ndo/known}}
      }
      
      write_classify undo
   }
   
   proc mtv_objc_list_proc_V {args} {
      if {[lindex $args 1] == "help"} {
	 return {{de}V}
      }
      
      write_classify deV
   }
   
   proc mtv_objc_list_proc_X {args} {
      if {[lindex $args 1] == "help"} {
	 return {{e}X{p}}
      }
      
      write_classify exp
   }
}
