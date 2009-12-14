#
# Prepare regions from fpC files for $frame, filters $f1, $f2, $f3, ready
# to be made into PPM files (and thence e.g. tiff via ppm2tiff)
#
# The regions are shifted to the r' centre; sky subtracted, and scaled
# to have the same sensitivity as the r' data
#
# If delta_offset is provided, it's a list of extra offsets to be applied to
# to the usual row and column offsets in f1, f2, and f3; these will usually
# be read from an atlas image. E.g. [list "1.2 2.3" "0 0" "0.2 1.2"]
#
# Possible values of use_calib:
#   0x1		Get flux20 from psField
#   0x2		Get sky level from psField
#   0x4		Use a known value for the sky;
#                 don't use regStatsFromQuartiles or the skylevel array
#   0x8		Read astrometric offsets
#
# If the skylevel array is provided, use it to set the sky level rather
# than regStatsFromQuartiles
#
# If skygrad is provided, it's an array of U16 regions to be subtracted
# from the input regions; not all filters need be present
#
# See make_ppm_file for an illustration of all the required steps
#
# If defined, the proc prepare_regions_callback will be called
# with the field and filter as arguments
#
proc prepare_regions {frame r1 r2 r3 {f1 r} {f2 g} {f3 u} \
			  {binfac 1} {use_calib 0x0} {delta_offset ""} \
			  {read_fieldparams 1} {_skylevel ""} \
			  {_skygrad ""} {_sky ""}} {
   if {$_skylevel != ""} {
      upvar $_skylevel skylevel
   }
   if {$_skygrad != ""} {
      upvar $_skygrad skygrad
   }
   if {$_sky != ""} {
      upvar $_sky sky
   }
   global calibs fieldparams openit rand

   set soft_bias 1000
   set rand [lindex [handleListFromType RANDOM] 0]
   if {$rand == ""} {
      set rand [phRandomNew 10000:2]
   }
   
   set ref_color "r"
   if {![info exists fieldparams] || [catch {schemaGetFromType _FIELDSTAT}]} {
      set read_fieldparams 1
   }

   if !$read_fieldparams {
      regsub -all {(.)}  [exprGet $fieldparams.filters] {\1 } psfilterlist
      set fieldstat [genericNew _FIELDSTAT]
      loop i 0 [llength $psfilterlist] {
	 foreach rc "row col" {
	    handleSet $fieldstat.${rc}offset<$i> 0
	 }
      }
   } else {
      set cbfile $openit(psdir)/psField-$openit(run)-$openit(col)-[format %04d $frame].fit 
      if 0 {
	 if ![file exists $cbfile] {
	    set cbfile $openit(psdir)/psCB-$openit(run)-$openit(col).fit
	 }
      }
      
      set hdr [hdrReadAsFits [hdrNew] $cbfile]
      set psfilterlist [string trim [hdrGetAsAscii $hdr FILTERS]]
      hdrDel $hdr
      
      declare_schematrans [llength $psfilterlist] 

      if [catch {
	 set calibs [offsetsRead $frame $ref_color $cbfile $psfilterlist]
	 set transfile $openit(asdir)/asTrans-$openit(run).fit
	 if ![file exists $transfile] {
	    set transfile $openit(asdir)/asTrans-$openit(run)-$openit(col).fit
	 }
	 if ![file exists $transfile] {
	    set transfile $openit(asdir)/$openit(col)/asTrans-$openit(run).fit
	 }
	 if ![file exists $transfile] {
	    set transfile $openit(asdir)/$openit(col)/asTrans-$openit(run)-$openit(col).fit
	 }
	 read_trans $calibs $transfile $openit(col) $psfilterlist
	 if [catch {
	    set fieldstat \
		[read_fieldstat $openit(objdir) $openit(run) $openit(col) $frame]
	 }] {
	    set fieldstat \
		[read_fieldstat $openit(dir) $openit(run) $openit(col) $frame]
	 }
	 
	 if ![info exists fieldparams] {
	    set ref_band_index [lsearch $psfilterlist $ref_color]
	    assert {$ref_band_index >= 0}
	    set fieldparams [fieldparamsNew $psfilterlist]
	    handleSet $fieldparams.ref_band_index $ref_band_index
	 }
	 loop i 0 [llength $psfilterlist] {
	    handleSetFromHandle \
		$fieldparams.frame<$i>.toGCC $calibs.calib<$i>->toGCC
	    handleSet $calibs.calib<$i>->toGCC 0x0
	    handleSetFromHandle \
		$fieldparams.frame<$i>.flux20 $calibs.calib<$i>->flux20

	    set skyreg [binregionNewFromConst [exprGet $calibs.calib<$i>->sky]]
	    handleSetFromHandle $fieldparams.frame<$i>.sky &$skyreg
	    handleDel $skyreg; unset skyreg

	    foreach rc "row col" {
	       handleSet $fieldparams.frame<$i>.${rc}Binning 1
	    }
	 }

	 if [info exists calibs] {
	    calib1byframeDel $calibs; unset calibs
	 }
	 fitsBinForgetSchemaTrans NULL
      } msg] {
	 global errorInfo; echo $errorInfo
	 echo $msg. Guessing offsets
	 set use_calib 0x0
      }
   }
   #
   # These flux20s look nice
   #
   array set flux20_good [list \
			      u 200 \
			      g 1000 \
			      r 1000 \
			      i 1000 \
			      z 200 \
			      ]

   global is_coadd
   if {[info exists is_coadd] && $is_coadd} {
      array set vals  [list \
				 u 1010 \
				 g 2360 \
				 r 1900 \
				 i 1400 \
				 z 240 \
				 ]
      foreach n [array names vals] {
	 set flux20_good($n) [expr 1e6/$vals($n)]
      }
   }
   #
   # These flux20s are approximately the measured values
   #
   array set flux20_nominal [list \
			      u 1010 \
			      g 2360 \
			      r 1900 \
			      i 1400 \
			      z 240 \
			      ]
   
   if [expr $use_calib & 0x1] {
      loop i 0 [llength $psfilterlist] {
	 set f [lindex $psfilterlist $i]
	 
	 #set flux20 [exprGet $calibs.calib<$i>->flux20]
	 set flux20 [exprGet $fieldparams.frame<$i>.flux20]
	 #
	 # Convert to pretty values
	 #
	 if [info exists flux20_good($f)] {
	    set flux20s($f) \
		[expr $flux20*1.0*$flux20_good($f)/$flux20_nominal($f)]
	 } else {
	    set flux20s($f) 1.0
	 }
      }
   } else {
      array set flux20s [array get flux20_good]
   }

   if [expr $use_calib & 0x2] {
      loop i 0 [llength $psfilterlist] {
	 set f [lindex $psfilterlist $i]
	 #set sky($f) [exprGet $calibs.calib<$i>->sky]
	 set sky($f) [binregionInterpolate *$fieldparams.frame<$i>.sky 0 0]
      }
   } elseif {![info exists sky]} {
      #array set sky [list u 10  g 5  r 5  i 5  z 5]
      array set sky [list u 0  g 0  r 0  i 0  z 0]
   }
   #
   # Give user control if they want it
   #
   if {[info commands prepare_regions_callback] != ""} {
      foreach f "$f1 $f2 $f3" {
	 if [catch {
	    prepare_regions_callback $frame $f
	 } msg] {
	    echo \"prepare_regions_callback $frame $f\" failed: $msg
	 }
      }
   }

   set ref_color [exprGet $fieldparams.ref_band_index]
   loop i 1 4 {
      set reg [set r$i]
      #
      # Remove a gradient?
      #
      if [info exists skygrad([set f$i])] {
	 skySubtract $reg $reg $skygrad([set f$i]) $rand
      }
      #
      # Shift to the r' coordinates
      #
      if [expr $use_calib & 0x8] {
	 set iband [lsearch $psfilterlist [set f$i]]
	 set delta [offsetDo $fieldparams 700 1024 $iband $ref_color]
	 set drow [expr [lindex $delta 0] + \
		       [exprGet $fieldstat.rowoffset<$iband>]]
	 set dcol [expr [lindex $delta 2] + \
		       [exprGet $fieldstat.coloffset<$iband>]]
	 
	 if {$delta_offset != ""} {
	    set drow [expr $drow + [lindex [lindex $delta_offset [expr $i-1]] 0]]
	    set dcol [expr $dcol + [lindex [lindex $delta_offset [expr $i-1]] 1]]
	 }
	 
	 regIntShift $reg $drow $dcol -filtsize 11 -out [set r$i]
	 handleSet $reg.row0 [expr -$drow]
	 handleSet $reg.col0 [expr -$dcol]
      } 

      if {$binfac != 1} {
	 set breg [regIntBin NULL $reg $binfac $binfac -slow]
	 set nreg [handleBindFromHandle [handleNew] *$breg.reg]
	 handleSet $breg.reg 0; binregionDel $breg
	 regDel $reg; set reg [set r$i $nreg]
      }
      #
      # reinstate the original sky value
      #
      if ![info exists sky([set f$i])] {
	 set sky([set f$i]) 0
      }
      if [expr $use_calib & 0x4] {
	 set bkgd [expr int($sky([set f$i]))-$soft_bias]
      } else {
	 if [info exists skylevel] {
	    set med [expr $soft_bias + $skylevel([set f$i])]
	 } else {
	    regStatsFromQuartiles -clip 2 -med med $reg -coarse 1
	 }

	 set bkgd [expr int($sky([set f$i]) + 0.5 - $med)]
      }
      regIntConstAdd $reg $bkgd
      #
      # Scale to same sensitivity as r'
      #
      if [info exists flux20s([set f$i])] {
	 set flux20 $flux20s([set f$i])
      } else {
	 set flux20 1000
      }
      if 0 {
	 set rat [expr 1.0*$flux20s(r)/$flux20]
      } else {
	 set rat [expr 1.0*1000/$flux20]
      }
      if {$rat != 1} {
	 regIntLincom $reg $reg [expr $sky([set f$i])*(1 - $rat)] $rat 0
      }
   }

   if $read_fieldparams {
      genericDel $fieldstat;		# it's actually a _FIELDSTAT
      if 0 {
	 fieldparamsDel $fieldparams; unset fieldparams
      }
   }
   
   return "$r1 $r2 $r3"
}

#
# Convert a set of 3 U16 regions to 3 U8 ones, using the specified method,
# as described in the comment above regU16ToU8LUTGet
# 
proc regU16ToU8Convert {r1 r2 r3 {method -1:15s} {type 0} {param 1}} {
   set lut [regU16ToU8LUTGet $r1 $r2 $r3 $method $type $param]
   
   set ans [regIntRGBToU8LUT $r1 $r2 $r3 $lut]
   
   regDel $lut

   return $ans
}

#
# Build a lookup table to convert three U16 to U8 regions using a LUT.
#
# The LUT is usually built from the mean of the 3 images; typically this is
# the total intensity of an RGB set.
#
# If the method is "histeq", histogram equalise all the pixels in the
# image (but see type == 5). Otherwise it's of the form min:max to do a
# stretch in min--max. If an S or s is appended, min and max are taken
# to be numbers of sigma relative to the median. You can also specify e.g.
# m-2:m+100 to stretch in the median -1,+100
#
# For sqrt, log, asinh, sqrt(asinh), or histeq stretch set type to 1, 2, 3,
# 4, or 5 respectively.
#
# As an alternative, you can specify a method of LUT:hhh, in which case
# hhh is taken to be an LUT previously returned by this proc
#
proc regU16ToU8LUTGet {r1 r2 r3 {method -1:15s} {type 0} {param 1}} {
   global verbose

   if [regexp {LUT:(.*)} $method foo lut] {
      return $lut
   }

   if [regexp {^([mM])?([+-]?[0-9]*(\.[0-9]+)?)?:([mM])?([+-]?[0-9]+(\.[0-9]+)?)?([sS])?$}  \
	   $method {} m1 min {} m2 max {} s] {
      if [regexp {^AAA[0-9]+:[0-9]+$} "$min:$max"] {
	 ;
      } else {
	 set I [regNew [exprGet $r1.nrow] [exprGet $r1.ncol]]
	 regIntLincom $I $r1 0 0 [expr 1.0/3.0]
	 regIntLincom $I $r2 0 1 [expr 1.0/3.0]
	 regIntLincom $I $r3 0 1 [expr 1.0/3.0]
	 
	 regStatsFromQuartiles -med med -csigma sig $I -coarse 1
	 regDel $I

	 if {$m1 != "" && $min == ""} { set min 0 }
	 if {$m2 != "" && $max == ""} { set max 0 }
	 if {$m1 != "" || $m2 != ""} {
	    if {$m1 != ""} {
	       set min [expr int($med + $min)]
	    }
	    if {$m2 != ""} {
	       set max [expr int($med + $max)]
	    }
	 } elseif {$s != ""} {
	    set min [expr int($med + $min*$sig)]
	    set max [expr int($med + $max*$sig)]
	 }
      }

      set lut [u16ToU8LUTGet -min $min -max $max -type $type -param $param]
      if {[info exists verbose] && $verbose >= 10} {
	 echo LUT: med,min,max = $med, $min, $max
      }
   } elseif {$method == "histeq"} {
      set lut [u16ToU8LUTGet -histeq]
   } else {
      error "Unknown method: $method"
   }

   if 0 {
      set maxReg [regNew -type U8 1 [exprGet $lut.ncol]]
      regIntSetVal $maxReg 255
      regSubtract $maxReg $lut

      regDel $lut
      set lut $maxReg
   }

   return $lut
}

###############################################################################
#
# Return regions from fpC files for $frame, filters $f1, $f2, $f3. If you
# specify regions instead of files, copies of the regions will be returned
#
# See make_ppm_file for a way to make a PPM file from specified frames
#
proc read_files {frame {f1 r} {f2 g} {f3 u} {reconstruct 0}} {
   loop i 1 4 {
      upvar 0 f$i f
      
      if [regexp {h[0-9]+} $f] {
	 set r$i [regNew [exprGet $f.nrow] [exprGet $f.ncol]]
	 regIntCopy [set r$i] $f
      } else {
	 if !$reconstruct {
	    set file [find_fpC $f $frame]

	    if {$file == ""} {
	       echo "I can't find fpC for $f $frame; reconstructing"
	       set reconstruct 1
	    } else {
	       global TYPE_PIX_NAME
	       set scr [regReadAsFits [regNew] $file]

	       if {[exprGet -enum $scr.type] == "TYPE_FL32" &&
		   $TYPE_PIX_NAME == "U16"} {
		  set fpC_zero 0; set fpC_scale 5
		  #set fpC_zero 0; set fpC_scale 1
		  
		  regIntLincom $scr "" $fpC_zero $fpC_scale 0
		  
		  set r$i [regNew [exprGet $scr.nrow] [exprGet $scr.ncol]]
		  regIntCopy [set r$i] $scr
		  hdrCopy $scr.hdr [set r$i].hdr; hdrFreeAll $scr.hdr
		  regDel $scr
	       } else {
		  set r$i $scr
	       }
	    }
	 }

	 if ![info exists r$i] {
	    set r$i [recon $f $frame {} {} 0 "binnedSky"]
	 }
      }
   }

   return "$r1 $r2 $r3"
}

proc find_fpC {filter frame} {
   global openit

   if ![info exists fpC_format] {
      set fpC_format {fpC-%06d-%s%d-%04d.fit  $run $filter $camCol $field}
   }
   
   regsub {^0*} $openit(run) {} run
   foreach d [list $openit(corrdir) \
		  $openit(objdir) \
		  $openit(corrdir)/$openit(col) \
		  $openit(objdir)/$openit(col)] {
      set file [get_corr_filename \
		    $d $run $filter $openit(camCol) $frame]
      if [file exists $file] {
	 return "$file"
      } else {
	 foreach suff [compressionTypes] {
	    if [file exists $file.$suff] {
	       return "$file"
	    }
	 }
      }
   }

   return ""
}

###############################################################################
#
# Create a PPM file for a range of fields
#
proc make_ppm_file {args} {
   set fix_saturated_LUT 1;		# fix colours of pixels above LUT's end
   set fix_with_U16 0;			# fix colours using U16 regions 
   set make_fpC 0;			# create fpC files as required
   set medianFilter 0;			# median filter image?
   set monochromatic 0;			# Make monochromatic? image 
   set separate "";			# Treat each band separately?
   set use_fpM 0;			# Ignore SATUR pixels in fpM files 
   set verbose 0;			# Be chatty?

   set opts [list \
		 [list make_ppm_file {Make a PPM file of a frame
 Given a range of frame numbers and an array openit with elements:

   $openit(asdir)   directory containing the atTrans files
   $openit(objdir)  directory containing the fpC files
   $openit(psdir)   directory containing the psCB files
   $openit(run)     [format %06d $run]
   $openit(col)     dewar number (alias column number)

 (as set by e.g. the set_run command),

 read the fpC files for the specified filters and frame, and write
 PPM files containing a true-colour images of the frames in question;
 they will be named "$basename-frame.ppm", and a list of the files written
 will be returned

 e.g.
	make_ppm_file r75-c1 21 -stretch 0:100s -type sqrt
 A small (quick) portion of the frame would be
       make_ppm_file r75-c1 21 -nrow 500 -ncol 500

 If the filters are specified as (e.g.)  r:$r where $r is a handle to a
 region, those regions will be used as the data to be converted. In this
 case, frame0 must equal frame1.

 If {row,col}0 and n{row,col} are provided, trim the regions after shifting.
 if n{row,col} == 0 it means, "to the edge"

 Note that the same LUT will be used for all the output files

 To create PPM files from atlas images, try e.g.
	do-frames -fpC -recon -rerun 7 94 5 260 262
	set_run -rerun 7 94 5
	make_ppm_file monster 260 262 -disp -stretch 0:100s -type sqrt

 (N.b. a variant on an asinh/sqrta stretch is an asinh:Q stretch, e.g.
     -type asinh:Q7 -stretch m+2:10
 In this case, the stretch starts at 2, and the linear part is exactly
 the same as a -stretch m+2:m+12 stretch, but the Q controls how the
 asinh turns on; Q == 0 is exactly linear.  So set Q == 0, tune the
 range (i.e. the number after the :) to get faint features right, then
 turn up Q until you're happy.)
  
		 }] \
		 [list <name> STRING "" basename \
		      "Output files are named <name>-frame.ppm"] \
		 [list <frame0> INTEGER 0 frame0 "Starting frame"] \
		 [list {[frame1]} INTEGER -1 frame1 \
		      "Ending frame (default: frame0)"] \
		 [list {[red-filter]} STRING "i" f1 \
		      "SDSS filter to use for RED (or r:region-handle)"] \
		 [list {[green-filter]} STRING "r" f2 \
		      "SDSS filter to use for GREEN (or r:region-handle)"] \
		 [list {[blue-filter]} STRING "g" f3 \
		      "SDSS filter to use for BLUE (or r:region-handle)"] \
		 [list {[bin]} INTEGER 1 binfac \
		      "How much to bin PPM file"] \
		 [list {[stretch]} STRING "0:300s" method \
		      "Desired stretch; if ends in \"s\", in units of sigma"] \
		 [list {[type]} STRING "asinh:Q10" type \
		      "Desired type of stretch; lin, sqrt, log, asinh:b, asinh:Q\#, sqrta:b"] \
		 [list {[nrow]} INTEGER -1 nrow \
		      "Number of rows to display; if -ve relative to top"] \
		 [list {[ncol]} INTEGER -1 ncol \
		    "Number of columns to display; if -ve relative to right"] \
		 [list {[row0]} INTEGER 0 row0 \
		      "Row-origin of desired region"] \
		 [list {[col0]} INTEGER 0 col0 \
		      "Column-origin of desired region"] \
		 [list {[display]} INTEGER 0 display \
		      "Display PPM files as they are created"] \
		 [list {[use_calib]} STRING "0x8" use_calib \
		      "Control where calibrations are read from"] \
		 [list -bin INTEGER 1 binfac \
		      "How much to bin PPM file"] \
		 [list -stretch STRING "0:10s" method \
		      "Desired stretch; if ends in \"s\", in units of sigma"] \
		 [list -type STRING "lin" type \
		      "Desired type of stretch; lin, sqrt, log, asinh:b, asinh:Q\#, sqrta:b, sqrt:Q\#"] \
		 [list -monochromatic CONSTANT 1 monochromatic "make a monochromatic image"] \
		 [list -nrow INTEGER -1 nrow \
		      "Number of rows to display; if -ve relative to top"] \
		 [list -ncol INTEGER -1 ncol \
		    "Number of columns to display; if -ve relative to right"] \
		 [list -row0 INTEGER 0 row0 \
		      "Row-origin of desired region"] \
		 [list -col0 INTEGER 0 col0 \
		      "Column-origin of desired region"] \
		 [list -display CONSTANT 1 display \
		      "Display PPM files as they are created"] \
		 [list -LUT STRING "" input_lut \
		      "The LookUp Table to use"] \
		 [list -getLUT STRING "" return_lut \
		      "Return the LUT in this variable"] \
		 [list -fpC CONSTANT 1 make_fpC "Make fpC files as needed"] \
		 [list -medianFilter CONSTANT 1 medianFilter "3x3 median filter colour image?"] \
		 [list -fixupLevel INTEGER 0 fixupLevel \
		      "Fix colours of areas above this intensity"] \
		 [list -nofix_saturated_LUT CONSTANT 0 fix_saturated_LUT \
		      "Don't fix the colours of pixels that are brighter
 than the top end of the LUT's stretch"] \
		 [list -fix_with_U16 CONSTANT 1 fix_with_U16 \
		      "Use U16 regions to find colour of saturated regions"] \
		 [list -use_fpM CONSTANT 1 use_fpM \
		  "Ignore U16 pixels labelled SATUR in fpM files"] \
		 [list -saturLevel INTEGER 0 saturLevel \
		      "Pixels at least this bright are to be considered saturated"] \
		 [list -scalebar DOUBLE 0.0 scalebar \
		      "Draw a scalebar this many arcsec long"] \
		 [list -use_calib STRING "0x8" use_calib \
		      "Control where calibrations are read from (0x8: read astrometric offsets)"] \
		 [list -sky STRING "" _sky_array \
		      "Array, indexed by filternames, of sky levels"] \
		 [list -dsky STRING "" _sky_grad_array \
		      "Array, indexed by filternames, of S32 binregions to
 subtract from U16 input frames (passed to skySubtract)"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty?"]\
		 [list -separate CONSTANT "-separate" separate \
		      "Treat each band separately? (Not recommended)"] \
		 [list -subtract_function STRING "" subtract \
		      "Selection function for objects to _subtract_ from fpC"] \
		 [list -show STRING "{}" sel_show \
		      "Show objects for which this proc returns true (see select)"] \
		]
   if {[shTclParseArg $args $opts make_ppm_file] == 0} {
      return 
   }

   if {[regexp {\.ppm$} $basename] && $frame0 != $frame1} {
      error "An absolute name such as $basename only makes sense when processing a single field (remove the .ppm)"
   }

   if {$_sky_array == ""} {
      set sky "{}"
   } else {
      upvar $_sky_array sky_array
      set sky "sky_array"
   }

   if {$_sky_grad_array == ""} {
      set sky_grad ""
   } else {
      upvar $_sky_grad_array sky_grad_array
      set sky_grad "sky_grad_array"
   }

   if {$frame1 < 0} {
      set frame1 $frame0
   }

   if {$return_lut != ""} {
      upvar $return_lut lut
   }

   set param "";			# extra parameter for stretch
   switch -regexp $type {
      {^lin(ear)?$} {
	 set type 0
      }
      {^sqrt$} {
	 set type 1
      }
      {^log$} {
	 set type 2
      }
      {^histeq$} {
	 set type 5
      }
      {^(asinh|sqrta)(:Q?([0-9]+(\.[0-9]+)?))?$} {
	 if [regexp {^(asinh|sqrta)(:(Q)?([0-9]+(\.[0-9]+)?))?$} $type {} type {} Q soften] {
	    if {$Q == "Q"} {
	       regexp {^([mM]?)?([+-]?[0-9]*(\.[0-9]+)?)?:([mM]?)?([+-]?[0-9]+(\.[0-9]+)?)?([sS])?$}  \
		   $method {} m1 min {} m2 max {} s
	       if {$s != ""} {
		  echo "I cannot process asinh:Q with a stretch in sigma; ignoring s"
	       }

	       set Q $soften
	       if {$m2 != ""} {
		  echo "When you specify Q, the stretch is min:range"
	       }
	       set range $max

	       if {$Q == 0} {
		  set soften 1e6;		# == linear
		  set method ${m1}$min:${m1}[expr $min + $range]
	       } else {
		  set alpha [expr 1.0/$range]
		  set Qmax 1e10;	# XXX
		  if {$Q > $Qmax} {
		     set Q $Qmax
		  }
		  set range [expr int(sinh($Q)/($alpha*$Q))]

		  set method ${m1}$min:${m1}[expr $min + $range]
		  set soften [expr 1/($alpha*$Q)]
	       }
	    }
	    set param "$soften"
	 }
	 if {$type == "asinh"} {
	    set type 3
	 } else {
	    set type 4
	 }
      }
   }

   if {$use_fpM || $saturLevel || $fixupLevel} {
      if !$fix_with_U16 {
	 error "-use_fpM and -saturLevel only make sense with -fix_with_U16"
      }
   }

   foreach f "f1 f2 f3" {
      if [regexp {([a-zA-Z0-9_.]*):(h[0-9]+)} [set $f] foo filter r] {
	 set $f $filter
	 lappend regs $r
	 set input_regs($r) 1
      }
   }
   if [info exists regs] {
      if {$frame0 != $frame1} {
	 error "You can only specify regions for a single frame"
      }
      eval read_files $frame0 [join $regs " "]
   }

   if {$input_lut != ""} {
      set lut $input_lut
   } else {
      set lut ""
   }

   #
   # Is $sel_show actually a logical expression, or
   # a logical:size:symbol (e.g. star&&HB:10:#)
   #
   if [regexp {^([^:]+)(:([^:]*))?(:([^:]*))?$} $sel_show {} sel_expr \
	   {} show_size {} show_symbol] {
      set sel_show $sel_expr
   }
   if {![info exists show_symbol] || $show_symbol == ""} {
      set show_symbol "\#"
   }
   if {![info exists show_size] || $show_size == ""} {
      set show_size 5
   }

   set sel_show [expand_logical_select $sel_show]
   if {$sel_show == ""} {
      set sel_show "{}"
   }

   loop frame $frame0 [expr $frame1 + 1] {
      if ![info exists regs] {
	 global openit

	 if {$subtract != ""} {
	    set missing "$f1 $f2 $f3"
	 } elseif $make_fpC {
	    set missing {}
	    foreach f "$f1 $f2 $f3" {
	       if {[find_fpC $f $frame] == ""} {
		  lappend missing $f
	       }
	    }
	 }
	 
	 if {($make_fpC && $missing != "") || $subtract != ""} {
	    regsub {^0*} $openit(run) {} run
	    do-frames -rerun $openit(reprocess) \
		$run $openit(col) $frame0 -filter $missing -fpC \
		-subtract $subtract
	 }
	 set regs [read_files $frame $f1 $f2 $f3]
      }

      if [catch {
	 if {$frame == 0} {		# special case; raw files
	    foreach r $regs {
	       if {[exprGet -enum $r.type] == "TYPE_FL32"} {
		  regIntConstAdd $r [softBiasGet]
	       }
	    }
	 } else {
	    set regs [eval prepare_regions $frame $regs \
			  $f1 $f2 $f3 $binfac $use_calib {""} 1 $sky $sky_grad]
	    
	    if $use_fpM {
	       declare_schematrans 5;	# 5 should be safe
	       loop i 0 3 {
		  read_mask [lindex "$f1 $f2 $f3" $i] $frame [lindex $regs $i]
	       }
	    }
	 }
	    
	 if {$saturLevel <= 0} {
	    set fixAllSatur ""
	 } else {
	    global MASK_TYPE
	    
	    loop i 0 3 {
	       set reg [lindex $regs $i]

	       set sm [reg2spanmask $reg -types SATUR -level $saturLevel]

	       if {[exprGet (int)$reg.mask] != 0x0} {
		  set omask [handleBindFromHandle [handleNew]  \
				 *((SPANMASK*)$reg.mask)]
		  
		  objmaskChainAndObjmaskChain *$sm.masks<$MASK_TYPE(SATUR)> \
		      *$omask.masks<$MASK_TYPE(SATUR)>

		  spanmaskDel $omask
	       }

	       set fixAllSatur -fixAllSaturated
	       handleSetFromHandle $reg.mask (MASK*)&$sm
	       handleDel $sm
	    }
	 }

	 if 0 {
	    set i -1
	    foreach reg $regs {
	       set i [incr i]
	       mtv -s [expr $i+1] $reg
	       puts -nonewline "(U16) [set f[expr $i+1]] : "
	       if {[gets stdin] == "q"} {
		  error $reg
	       }
	    }
	 }

	 if $monochromatic {
	    regIntLincom [lindex $regs 0] [lindex $regs 1] 0 0.333 0.333
	    regIntLincom [lindex $regs 0] [lindex $regs 2] 0 1     0.333
	    
	    regIntCopy [lindex $regs 1] [lindex $regs 0]
	    regIntCopy [lindex $regs 2] [lindex $regs 0]
	 }

	 if {$lut == ""} {
	    set lut [eval regU16ToU8LUTGet $regs $method $type $param]
	 }
	 
	 set uregs [eval regIntRGBToU8LUT $regs $lut $separate]

	 if $medianFilter {
	    eval regU8Median $uregs
	 }

	 if 0 {
	    set i -1
	    foreach reg $uregs {
	       set i [incr i]
	       mtv -s [expr $i+1] $reg
	       puts -nonewline "(U8) [set f[expr $i+1]] : "
	       if {[gets stdin] == "q"} {
		  error $reg
	       }
	    }
	 }
	 
	 set parents ""
	 if {$row0 != 0 || $col0 != 0 || $nrow > 0 || $ncol > 0} {
	    if {$nrow <= 0} {
	       set nrow [expr [exprGet [lindex $uregs 0].nrow] - $row0]
	    }
	    if {$ncol <= 0} {
	       set ncol [expr [exprGet [lindex $uregs 0].ncol] - $col0]
	    }

	    foreach ureg $uregs {
	       lappend parents $ureg
	       lappend new [subRegNew $ureg $nrow $ncol $row0 $col0]
	    }

	    set uregs $new; unset new
	 }

	 if $fix_saturated_LUT {
	    if {$parents == ""} {
	       if $fix_with_U16 {
		  set lut_min [exprGet $lut.row0]
		  set lut_max [exprGet $lut.col0]
		  if {$fixupLevel <= 0 && $fixAllSatur == ""} {
		     set fixupLevel $lut_max
		  }

		  eval fixSaturatedU8Regs $uregs $regs \
		      -min $lut_min -neighbor -fixup $fixupLevel $fixAllSatur
	       } else {
		  eval fixSaturatedU8Regs $uregs $fixAllSatur
	       }
	    } else {		# $regs are different size; no fancy processing
	       eval fixSaturatedU8Regs $uregs $fixAllSatur
	    }
	 }
	 #
	 # Draw a scalebar?
	 #
	 if {$scalebar > 0} {
	    set scale_row0 [expr int($nrow/10) + 1]
	    set scale_col0 [expr int($ncol/10) + 1]
	    foreach reg $uregs {
	       draw_scalebar $reg $scalebar -bin $binfac \
		   -row0 $scale_row0 -col0 $scale_col0
	    }

	    set scalebar -1;		# only in one output file
	 }

	 if {$sel_show != "{}"} {
	    global table openit
	    global objcIo

	    set table [openit -noAI $frame]

	    set n1 1
	    set n2 [keylget table OBJnrow]
      
	    loop i $n1 [expr $n2 + 1] {
	       catch {catObjDel $objcIo -deep};	# it may not exist
	       set objcIo [objcIoRead table $i]

	       if {$sel_show != "all" && ![eval $sel_show $objcIo]} {
		  continue;
	       }

	       set rowc [exprGet $objcIo.objc_rowc]
	       set colc [exprGet $objcIo.objc_colc]
	       set sizes [split $show_size "-"]
	       foreach reg $uregs {
		  loop size [lindex $sizes 0] [expr [lindex $sizes end] + 1] {
		     if [catch { draw_cross_in_region $reg $show_symbol $rowc $colc -size $size } msg] {
			echo draw_cross_in_region $reg $show_symbol $rowc $colc -size $size: $msg
		     }
		  }
	       }
	    }

	    objfileClose table
	 }
	 
	 if {$basename == "none"} {
	    set file ""
	    set files ""
	 } else {
	    if [regexp {\.ppm$} $basename] {# already a full filename
	       set file $basename
	    } else {
	       set file $basename-$frame.ppm
	    }
	    if $verbose {
	       puts -nonewline "$file\r"; flush stdout
	    }
	    eval truecolorPPMWrite $file $uregs;
	    lappend files $file

	    if $display {
	       display_ppm_file -geom +1+1 $file
	    }
	 }
	 
	 if 0 {				# RHL
	    foreach f $uregs {
	       mtv $f
	       puts -nonewline "$f : "; gets stdin
	    }
	 }
	 
	 foreach r "$regs $uregs $parents" {
	    if ![info exists input_regs($r)] {
	       regDel $r
	    }
	 }
	 unset regs
      } msg] {
	 if {$lut != ""} { regDel $lut }
	 global errorInfo
	 error "$msg $errorInfo"
      }
   }
   if $verbose {
      echo ""
   }

   if {$lut != "" && $input_lut == "" && $return_lut == ""} {
      regDel $lut
   }

   return $files
}

###############################################################################
#
# Write an assemblePPM input file for a pair of runs, and a set of
# camCols/fields
#
proc make_assemblePPM {args} {
   global fieldparams table

   set opts [list \
		 [list [info level 0] ""] \
		 [list <name> STRING "" basename \
		      "Input files are named <name>-run-camCol-frame.ppm"] \
		 [list <run1> STRING 0 run1 "First run"] \
		 [list <camCol1> STRING "" camCol1 \
		      "Camera columns in run1 in form 1:6"] \
		 [list <field1> STRING "" field1 \
		      "Camera columns in run1 in form 11:106"] \
		 [list {[run2]} STRING "" run2 "Second run"] \
		 [list {[camCol2]} STRING "" camCol2 \
		      "Camera columns in run2 in form 1:6"] \
		 [list {[field2]} STRING "" field2 \
		      "Camera columns in run2 in form 11:106"] \
		 [list -outFile STRING "" outfile \
		      "Name of file to write"] \
		 [list -hdr STRING "" hdr "Header to write to outfile"] \
		 [list -rerun1 STRING {} rerun1 "Desired rerun for run one"] \
		 [list -rerun2 STRING {} rerun2 "Desired rerun for run two"] \
		 [list -set_run_args STRING "" set_run_args \
		      "Extra arguments to set_run"] \
		 [list -nrow INTEGER 1370 nrow \
		      "How many rows of input ppm files to include in mosaic"]\
		 [list -ncol INTEGER [expr 2048 - 120] ncol \
		      "How many cols of input ppm files to include in mosaic"]\
		 [list -row0 INTEGER 64 row0 \
		      "First row of input ppm files to include in mosaic"]\
		 [list -col0 INTEGER 64 col0 \
		      "First col of input ppm files to include in mosaic"]\
		 [list -Nrow INTEGER 0 Nrow \
		      "Number of rows of input ppm files in output mosaic"] \
		 [list -Ncol INTEGER 0 Ncol \
		      "Number of columns of input ppm files in output mosaic"]\
		 [list -binfac INTEGER 1 binfac \
		      "How much input ppm files are binned
 (N.b. You'll probably want to set -n{row,col}/-{row,col}0)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # Parse e.g. camCol1, giving camCol11 and camCol12
   #
   set runs $run1
   set ${run1}_rerun \{$rerun1\}
   if {$run2 == ""} {
      if {$camCol2 != "" || $field2 != ""} {
	 error "You cannot specify camCol/field without run2"
      }
   } else {
      set ${run2}_rerun \{$rerun2\}
      lappend runs $run2
   }
   
   foreach r "1 2" {
      foreach el "camCol field" {
	 if {[set ${el}$r] != ""} {
	    if [regexp {^([0-9]+)$} [set ${el}$r] {} [set run$r]_${el}_1] {
	       set [set run$r]_${el}_2 [set [set run$r]_${el}_1]
	    } elseif {![regexp {^([0-9]+):([0-9]+)$} [set ${el}$r] {} \
			    [set run$r]_${el}_1 [set run$r]_${el}_2]} {
	       error "${el}$r ([set ${el}$r]) is not of form nnn:nnn"
	    }
	 }
      }
   }
   #
   # Lookup the astrometry
   #
   set rc [expr 1361/2]; set cc [expr 2048/2];# reference point in image
   foreach r $runs {
      loop c [set ${r}_camCol_1] [expr [set ${r}_camCol_2] + 1] {
	 eval set_run $set_run_args $r $c  -rerun "[set ${r}_rerun]"
	 loop f [set ${r}_field_1] [expr [set ${r}_field_2] + 1] {
	    set table [openit -noAI $f]
	    if ![info exists fieldparams0] {
	       set node0 [keylget table node]
	       set incl0 [keylget table incl]

	       set fieldparams0 $fieldparams;
	       unset fieldparams;	# so it'll be read by openit

	       set munu [transApply *$fieldparams0.frame<2>.toGCC "r" \
			      $rc 0 $cc 0]
	       set mu0 [keylget munu mu]; set nu0 [keylget munu nu]
	       #
	       # Read next field to get a good estimate of scale, the
	       # number of pixels per degree
	       #
	       if {[set ${r}_field_1] != [set ${r}_field_2]} {
		  set table [openit -noAI [expr $f + 1]]
		  
		  set munu [transApply *$fieldparams.frame<2>.toGCC "r" \
				$rc 0 $cc 0]
		  set scale 1361.0/([expr [keylget munu mu] - $mu0])
	       } else {
		  set dr 500.0
		  set munu [transApply *$fieldparams0.frame<2>.toGCC "r" \
				[expr $rc + $dr] 0 $cc 0]
		  set scale $dr/([expr [keylget munu mu] - $mu0])
	       }

	       set dr 0; set dc 0

	       set drmin 0; set dcmin 0;	# minimum values of dr/dc
	    } else {
	       set node [keylget table node]
	       set incl [keylget table incl]

	       set munu [transApply *$fieldparams.frame<2>.toGCC "r" \
			 $rc 0 $cc 0]
	       set eq [GCToEq [keylget munu mu] [keylget munu nu] \
			   -node $node -incl $incl]
	       set munu0 [eqToGC [keylget eq ra] [keylget eq dec] \
			   -node $node0 -incl $incl0]

	       if 1 {
		  set rowcol [transInverseApply *$fieldparams0.frame<2>.toGCC "r"\
				  [keylget munu0 mu] 0 [keylget munu0 nu] 0]
		  
		  set dr [expr 0 - int([keylget rowcol row] - $rc)]
		  set dc [expr int([keylget rowcol col] - $cc)]
	       } else {
		  set dr [expr int($scale*($mu0 - [keylget munu0 mu]))]
		  set dc [expr int($scale*([keylget munu0 nu] - $nu0))]
	       }
	    }

	    objfileClose table

	    set drow($r:$c:$f) $dr
	    set dcol($r:$c:$f) $dc

	    if {$r == [lindex $runs 0] && $dr < $drmin} {
	       set drmin $dr
	    }
	    if {$dc < $dcmin} {
	       set dcmin $dc
	    }
	 }
      }
   }
   if [info exists fieldparams0] {
      fieldparamsDel $fieldparams0
   }
   #
   # Process those offsets
   #
   foreach r $runs {
      loop c [set ${r}_camCol_1] [expr [set ${r}_camCol_2] + 1] {
	 loop f [set ${r}_field_1] [expr [set ${r}_field_2] + 1] {
	    set drow($r:$c:$f) [expr $drow($r:$c:$f) - $drmin]
	    set dcol($r:$c:$f) [expr $dcol($r:$c:$f) - $dcmin]
	 }
      }
   }
   #
   # Open output file
   #
   if {$outfile == ""} {
      set fd "stdout"
   } else {
      set fd [open $outfile "w"]
   }

   puts $fd "\#"
   puts $fd "\# [format %-20s "run1-camCol-field"] $run1 $camCol1 $field1"
   if {$run2 != ""} {
      puts $fd "\# [format %-20s "run2-camCol-field"] $run2 $camCol2 $field2"
   }
   puts $fd "\#"

   if {$hdr != ""} {
      foreach line [split $hdr "\n"] {
	 if ![regexp {^\#} $line] {
	    set line "\# $line"
	 }
	 puts $fd "$line";
      }
      puts $fd "\#";
   }

   if {$Nrow == 0} {
      set Nrow [expr [set ${run1}_field_2] - [set ${run1}_field_1] + 1]
   }
   if {$Ncol == 0} {
      if {$run2 == ""} {
	 set Ncol 1
      } else {
	 set Ncol [expr [set ${run1}_camCol_2] - [set ${run1}_camCol_1] + 1 + \
		    [set ${run2}_camCol_2] - [set ${run2}_camCol_1] + 1]
      }
   }

   if {$binfac != 1} {
      foreach el [array name drow] {
	 set drow($el) [expr $drow($el)/$binfac]
	 set dcol($el) [expr $dcol($el)/$binfac]
      }
   }
   #
   # Set overall size of image.  The first run specifies the total number
   # of rows; search all runs for the total number of columns
   #
   set r [lindex $runs 0]; set c [set ${r}_camCol_1]; set f [set ${r}_field_1]
   set NROW [expr $drow($r:$c:$f) + $nrow]
   set NCOL 0
   foreach r $runs {
      set c [set ${r}_camCol_2]; set f [set ${r}_field_2]
      set nc [expr $dcol($r:$c:$f) + $ncol]
      if {$nc > $NCOL} {
	 set NCOL $nc
      }
   }

   puts $fd "mosaic		0 0  $NROW $NCOL"

   foreach r $runs {
      loop c [set ${r}_camCol_1] [expr [set ${r}_camCol_2] + 1] {
	 loop f [set ${r}_field_1] [expr [set ${r}_field_2] + 1] {
	    set dr $drow($r:$c:$f);
	    set dc $dcol($r:$c:$f)
	    set nr $nrow; set nc $ncol
	    set r0 $row0; set c0 $col0
	    if {$dr < 0} {
	       incr nr $dr; incr r0 -$dr; set dr 0
	    }
	    if {$dc < 0} {
	       incr nc $dc; incr c0 -$dc; set dc 0
	    }

	    if {$nr > 0 && $nc > 0} {
	       puts $fd [format "%-40s %5d %5d  $nr $nc  $r0 $c0" \
			     "$basename-$r-$c-$f.ppm" $dr $dc]
	    }
	 }
      }
   }
   #
   # Cleanup
   #
   if {$fd != "stdout"} {
      close $fd
   }

   return $outfile
}

###############################################################################
#
# Make a PPM file from an atlas image
#
if ![info exists AI_prefix] {
   set AI_prefix "AI"
}

proc make_ppm_file_ai {{_table table} {n -1} {f1 i} {f2 r} {f3 g} \
			{binfac 1} {method 0:10s} {type lin} \
		        {nrow -4} {ncol -4} {row0 4} {col0 4} \
			   {display 0} {use_calib 0x4} {nochildren 0} \
			   {scalebar 0.0}} {
   upvar $_table table
   global objcIo

   if [regexp -- {-fix(Saturated)?$} $type] {
      set fixSaturated 1
   } else {
      set fixSaturated 0
   }

   set param ""
   switch -regexp $type {
      {^lin} {
	 set type 0
      }
      {^sq} {
	 set type 1
      }
      {^log} {
	 set type 2
      }
      {^histeq} {
	 set type 5
      }
      {^asinh(:Q?([0-9]+(\.[0-9]+)?))?$} {
	 if [regexp {^asinh(:(Q)?([0-9]+(\.[0-9]+)?))?$} $type {} {} Q soften] {
	    if {$Q == "Q"} {
	       regexp {^([mM]?)?([+-]?[0-9]*(\.[0-9]+)?)?:([mM]?)?([+-]?[0-9]+(\.[0-9]+)?)?([sS])?$}  \
		   $method {} m1 min {} m2 max {} s
	       if {$s != ""} {
		  echo "I cannot process asinh:Q with a stretch in sigma; ignoring s"
	       }

	       set Q $soften
	       if {$m2 != ""} {
		  echo "When you specify Q, the stretch is min:range"
	       }
	       set range $max
	       if {$Q == 0} {
		  set soften 1e6;		# == linear
		  set method ${m1}$min:${m1}[expr $min + $range]
	       } else {
		  set alpha [expr 1.0/$range]
		  set Qmax 1e10;	# XXX
		  if {$Q > $Qmax} {
		     set Q $Qmax
		  }
		  set range [expr int(sinh($Q)/($alpha*$Q))]

		  set method ${m1}$min:${m1}[expr $min + $range]
		  set soften [expr 1/($alpha*$Q)]
	       }
	    }
	    set param "$soften"
	 }
	 set type 3
      }
      {^histeq$} {
	 set type 5
      }
      {^sqrta(:([0-9]+(\.[0-9]+)?))?$} {
	 if [regexp {^sqrta(:([0-9]+(\.[0-9]+)?))?$} $type {} {} soften] {
	    set param "$soften"
	 }
	 set type 4
      }
   }

   global gutter
   set gutter0 $gutter; set gutter 20
   foreach f "$f1 $f2 $f3" {
      if $nochildren {
	 set f [string toupper $f]
      }

      lappend regs [get_objcIo_blend_mosaic table $n $f "" 0 "" mosaic_geom "" $binfac]
   }
   #
   # Find the offsets in the atlas images. If there's no atlas image
   # (i.e. a BRIGHT object) read the next atlas image (i.e. the non-BRIGHT
   # sibling) to get the offsets
   #
   if {[exprGet $objcIo.aimage->id] > 0} {
      set read_next_object 0
   } else {
      set read_next_object 1
      set objcIo [objcIoRead table next]
   }

   foreach f "$f1 $f2 $f3" {
      set i [lsearch [keylget table filters] $f]
      lappend delta_offset [list \
				[exprGet $objcIo.aimage->drow<$i>] \
				[exprGet $objcIo.aimage->dcol<$i>]]
   }
   if $read_next_object {		# get the original back
      set objcIo [objcIoRead table prev]
   }

   set gutter $gutter0

   if 0 {
      array set sky [list u 10  g 5  r 5  i 5  z 5]
   } else {
      array set sky [list u 0 g 0 r 0 i 0 z 0]
   }

   set regs [prepare_regions [keylget table field] \
		 [lindex $regs 0] [lindex $regs 1] [lindex $regs 2] \
		 $f1 $f2 $f3 1 $use_calib $delta_offset 0 "" "" sky]
   #
   # Unpack the mosaic geometry
   #
   set i -1
   foreach v [list nreg nr nc rsize csize big_gutter] {
      set $v [lindex $mosaic_geom [incr i]]
   }
   foreach v [list rsize csize big_gutter] {
      #set $v [expr [set $v]/$binfac]
   }

   if [catch {
      #
      # Build a lookup table if needed, if blended only use the parent
      #
      if {![info exists lut]} {
	 foreach r $regs {
	    lappend sregs [subRegNew $r $rsize $csize 0 0]
	 }

	 set lut [eval regU16ToU8LUTGet $sregs $method $type $param]
	 
	 foreach sr $sregs {
	    regDel $sr
	 }
      }
      #
      # Convert to 8-bit regions using that LUT
      #
      set uregs [eval regIntRGBToU8LUT $regs $lut]
      #
      # Set saturated parts of image to a uniform (correct) colour?
      #
      if $fixSaturated {
	 eval fixSaturatedU8Regs $uregs; # $regs -min [exprGet $lut.row0]
      }
      #
      # We made the gutters oversized as the edges of the atlas images
      # in each band don't line up; rebuild the regions with the correct
      # sized gutters set to 0
      #
      # Clip each parent/child if so desired
      #
      if {$nrow <= 0} {
	 set nrow [expr $rsize - $row0 + $nrow]
      }
      if {$ncol <= 0} {
	 set ncol [expr $csize - $col0 + $ncol]
      }
      
      set ncol_mosaic [expr $nc*$ncol+($nc-1)*$gutter];# desired size
      set nrow_mosaic [expr $nr*$nrow+($nr-1)*$gutter]

      foreach ureg $uregs {
	 set nmosaic [regNew $nrow_mosaic $ncol_mosaic -type U8]
	 lappend parents $ureg
	 lappend new $nmosaic
	 regIntSetVal $nmosaic 100

	 loop i 0 $nreg {
	    set ir [expr int($i/$nc)]
	    set ic [expr $i - $nc*$ir]
	    set r0 [expr $ir*($rsize+$big_gutter) + $row0]
	    set c0 [expr $ic*($csize+$big_gutter) + $col0]

	    set nr0 [expr $ir*($nrow+$gutter)]
	    set nc0 [expr $ic*($ncol+$gutter)]

	    set tmp [subRegNew $ureg $nrow $ncol $r0 $c0]
	    set ntmp [subRegNew $nmosaic $nrow $ncol $nr0 $nc0]
	    regIntCopy $ntmp $tmp
	    regDel $tmp; regDel $ntmp
	 }
      }
      set uregs $new; unset new
      #
      # Draw a scalebar?
      #
      if {$scalebar > 0} {
	 foreach reg $uregs {
	    draw_scalebar $reg $scalebar -bin $binfac
	 }
      }
      #
      # Generate the ppm file
      #
      global AI_prefix
      set basename "$AI_prefix-[keylget table run]-[keylget table camCol]-[keylget table field]_"
      set file $basename[exprGet $objcIo.id].ppm
      eval truecolorPPMWrite $file $uregs;
      lappend files $file
      
      if $display {
	 set size [expr ($nrow > $ncol) ? $nrow : $ncol]
	 if {$size < 20} {
	    set expand 8
	 } elseif {$size < 40} {
	    set expand 4
	 } elseif {$size < 80} {
	    set expand 2
	 } else {
	    set expand 1
	 }
	 display_ppm_file -geom +1+1 -expand $expand $file
      }
      #
      # Cleanup
      #
      foreach r "$regs $uregs $parents" { 
	 regDel $r
      }
      unset regs
   } msg] {
      if [info exists lut] { regDel $lut }
      global errorInfo
      error "$msg $errorInfo"
   }
   
   if [info exists lut] { regDel $lut }

   return $files
}

proc mtv_blend_ppm {args} {
   set display 1; set fixSaturated 0; set remove 0; set nochildren 0
   set opts [list \
		 [list mtv_blend_ppm \
		"Create a \"true\" colour ppm for an atlas image and family"] \
		 [list {[table]} STRING "table" _table \
		      "Name of table as returned by openit"] \
		 [list {[n]} INTEGER -1 n \
		   "ID number of desired object (default: last object read)"] \
		 [list -filters STRING "irg" filters \
		      "3 filters to use in constructing output file"] \
		 [list -binned INTEGER 1 binfac \
		      "How much to bin PPM file"] \
		 [list -stretch STRING "0:40" method \
		      "stretch to use, e.g. 0:40
 (if it ends in s, e.g. 0:10s, in units of sigma)"] \
		 [list -type STRING "asinh:Q8" type \
		      "type of stretch: asinh:n (n ~ 5), lin, log, sqrt, sqrta"] \
		 [list -nodisplay CONSTANT 0 display \
		      "don't attempt to display file after creation"] \
		 [list -remove INTEGER 10 remove \
 		      "remove file after # seconds (0 ==> don't remove)"] \
		 [list -fixSaturated CONSTANT 1 fixSaturated \
 		      "Set saturated regions to a constant (correct) colour"] \
		 [list -scalebar DOUBLE 0.0 scalebar \
		      "Draw a scalebar this many arcsec long"] \
		 [list -nochildren CONSTANT 1 nochildren \
 		      "Don't display children"] \
		 ]
   if {[shTclParseArg $args $opts mtv_blend_ppm] == 0} {
      return 
   }
   if !$display { set remove 0 }

   upvar $_table table

   regsub -all { } $filters "" filters
   set f1 [string range $filters 0 0]
   set f2 [string range $filters 1 1]
   set f3 [string range $filters 2 2]

   if [keylget table many_fields val] {# a many-field fpObjc/tsObj file
      if {[keylget table fileSchema] == "TSOBJ"} {
	 set objcIo [objcIoRead table $n]
	 set_run [exprGet $objcIo.run] [exprGet $objcIo.camCol]
      } else {
	 set_run -rerun [keylget table rerun] \
	     [keylget table run] [keylget table camCol]
      }
   }

   if $fixSaturated {
      append type "-fixSaturated"
   }

   set file [make_ppm_file_ai table $n $f1 $f2 $f3 $binfac $method $type \
		 -4 -4 4 4 $display [expr 0x4|0x8] $nochildren $scalebar]

   if { [keylget table many_fields val] &&
	![keylget table is_summary_file]} {# a many-field tsObj file
      global fieldparams
      if [info exists fieldparams] {
	 fieldparamsDel $fieldparams
	 unset fieldparams
      }
   }

   if $remove {
      if [catch {
	 after [expr int(1000*$remove)] unlink $file
      } msg] {
	 echo $msg
      }
      return ""
   } else {
      return $file
   }
}

###############################################################################
#
# Draw a scalebar into a region
#
proc draw_scalebar {args} {
   set const 0;

   set opts [list \
		 [list [info level 0] "Draw a scalebar in a region"] \
		 [list <reg> STRING "" reg "Region (or an sao number) to draw scalebar in"] \
		 [list <len> DOUBLE 0 len "Length of scalebar (arcsec)"] \
		 [list -bin INTEGER 1 binfac "How much image is binned by"] \
		 [list -row0 INTEGER 2 row0 "Starting row for scalebar"] \
		 [list -col0 INTEGER 3 col0 "Starting column for scalebar"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set pixscale [expr 0.396*$binfac];# XXX
   
   set len [expr int($len/$pixscale + 0.5)]
   set h2 [expr int($len/25.0 + 0.5)];# half-height or bar ends
   if {$h2 < 2} {
      set h2 2
   }
   
   set r0 [expr $row0 + $h2]; set c0 $col0;# start of scalebar
   set r1 $r0;            set c1 [expr $c0 + $len];# end of bar

   if [regexp {^[0-9]$} $reg] { 	# an sao number
      set r0 [expr $r0 + 0.5];  	# centre of pixel
      set c0 [expr $c0 + 0.5]
      set c1 [expr $c1 + 0.5]
      saoDrawPolygon -s $reg $r0 $c0 $r0 $c1
      saoDrawPolygon -s $reg [expr $r0 - $h2] $c0 [expr $r0 + $h2] $c0
      saoDrawPolygon -s $reg [expr $r0 - $h2] $c1 [expr $r0 + $h2] $c1 -i
   } else {
      set c1 [expr $c1 - 1];            # we draw from c0 to c0 pixel if len==1
      if {$r0 + $h2 >= [exprGet $reg.nrow] ||
	  $c0 >= [exprGet $reg.ncol] || $c1 >= [exprGet $reg.ncol]} {
	 ;				# It won't fit; ignore scalebar
      } else {
	 loop c $c0 [expr $c1 + 1] {
	    regPixSetWithDbl $reg $r0 $c 255
	 }
	 loop r [expr $r0 - $h2] [expr $r0 + $h2 + 1] {
	    regPixSetWithDbl $reg $r $c0 255
	    regPixSetWithDbl $reg $r $c1 255
	 }
      }
   }
}

###############################################################################
#
# Display a .ppm file
#
proc display_ppm_file {args} {
   set eightbit 0
   set opts [list \
		 [list display_ppm_file "Display a .ppm file"] \
		 [list <file> STRING "" file "The file to display"] \
		 [list -expand INTEGER 1 expand \
		      "How much to expand the display"] \
		 [list -geometry STRING "" geom "X-style geometry spec"] \
		 [list -8 CONSTANT 1 eightbit "Display in 8-bit colour"] \
		 ]
   if {[shTclParseArg $args $opts display_ppm_file] == 0} {
      return 
   }

   set flags0 ""
   if {$geom != ""} {
      lappend flags0 -geometry $geom
   }

   global no_real_xv;			# xv is a fake at FNAL
   if [info exists no_real_xv] {
      set msg_xv "(no xv)"
   }
   if {[info exists no_real_xv] || [catch {
      set flags $flags0
      
      if {$eightbit} {
	 lappend flags -8
	 lappend flags -perfect
      }

      global darwin
      if {0 && [info exists darwin] && $darwin} {
	 exec sh -c "open $file &" &
      } else {
	 exec sh -c "xv -raw -expand $expand $flags $file &" &
      }
   } msg_xv]} {
      if [catch {
	 set flags $flags0
      
	 eval exec display $flags $file &
      } msg_display] {
	 if [catch {
	    set flags $flags0
      
	    eval exec xloadimage $flags $file &
	 } msg_xloadimage] {
	    echo "xv, display, and xloadimage all failed: $msg_xv $msg_display $msg_xloadimage"
	 }
      }
   }
}

###############################################################################
#
# Here's a gutted version of run_frames_pipeline, that only produces
# corrected frames. Note that you must have run the PSP first. We
# could produce a gutted version of that too, if it proved needed
#
proc make_corrected_frames {args} {
   global env photoEnv data_root
   global softpars hardpars plan defpars
   global TYPE_U8 TYPE_U16 MASK_TYPE
   global rawncols overlap corrows corcols
   global filterlist nfilter ref_color
   global fieldstat 
   global fieldparams
   global ccdpars ccdrow camCol run runstr field
   global verbose
   global write_fpC_files
   global id_values

   set reconstruct 0;			# reconstruct from fpAtlas/fpBIN files
   set sky_subtract 0;			# sky subtract after flat fielding?
   set write_fpM_files 0;		# write fpM files?
   set write_fpFieldStat_files 0;	# write fpFieldStat files?
   set opts [list \
		 [list [info level 0] ""] \
		 [list {[planfile]} STRING "fpPlan.par" planfile \
		      "fpPlan file to use"] \
		 [list -bkgd STRING "NULL" bkgd "\
 Region to use a background;
   \"\"      for blank,
   \"binned\"    to use the fpBIN file
   \"binnedSky\" to use the fpBIN file, with a constant sky and noise added
   \"brightSky\" to use the fpBIN file, along with the local sky that we
                 subtracted, and with noise added\
 "] \
		 [list -reconstruct_fpC CONSTANT 1 reconstruct \
		      "Reconstruct fpC files from fpAtlas/fpBIN files"] \
		 [list -selection_function STRING "" select \
		      "Proc to select desired objects; see \"help select\""] \
		 [list -sky_subtract CONSTANT 1 sky_subtract \
		      "Sky subtract?"] \
		 [list -fpM CONSTANT 1 write_fpM_files \
		      "Write fpM as well as fpC files?"] \
		 [list -subtract STRING "" subtract \
		      "Selection function for objects to _subtract_ from fpC"] \
		 [list -fpFieldStat CONSTANT 1 write_fpFieldStat_files \
		      "Write fpFieldStat as well as fpC files?"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }


   set kos [chainNew KNOWNOBJ]

   regsub {^~/} $planfile "$env(HOME)/" planfile
   if {[file isdirectory $planfile] && [file exists $planfile/fpPlan.par]} {
      set planfile "$planfile/fpPlan.par"
   }
   #
   # Read and set all the parameters in the default and _then_ the
   # specified planfile, in local scope, and also set this list fpplan.
   #
   # Note that this means that the specified planfile will _override_
   # any parameters set in the default one
   # 
   set defplanfile "[envscan \$PHOTO_DIR]/etc/fpPlan.par"

   # Get the location of the plan file from photoEnv, if provided.
   # Otherwise use "." as the directory containing the plan file.
   if {![regexp {^/} $planfile] && [info exists photoEnv(FRAMES_PLAN)]} {
      set ffile "$photoEnv(FRAMES_PLAN)/$planfile"
      if [file exists "$ffile"] {
	 set planfile "$ffile"
      }
   }
   #
   # Process planfiles
   #
   set cmdline cmdlinePlan
   if [info exists fpplan] { unset fpplan }
   foreach file "defplanfile cmdline planfile cmdline" {
      process_plan fpplan [read_planfile [set $file]]
   }
   #
   # Remove `default' values, and set the plan as variables in this scope
   # (or global, if they so desired).
   #
   # If the value is "nodefault", complain
   #
   foreach var [keylget fpplan] {
      set val [keylget fpplan $var]
      if {$val == "default"} {
	 keyldel fpplan $var
      } elseif {$val == "nodefault"} {
	 error "** keyword $var has value nodefault! **"
      } else {
	 if {[regexp {^display} $var] ||
	     [regexp {_format$} $var] ||
	     [regexp {^truth_} $var] ||
	     [regexp {^(read|write)_(fpC|fpAtlas)_files} $var] ||
	     [regexp {^fpC_(zero|scale)$} $var] ||
	     $var == "read_psBB_files" || $var == "psBBDir" ||
	     $var == "compress_image_files" ||
	     $var == "write_test_info" || $var == "allow_old_psField"} {
	    global $var
	 }
	 
	 set $var $val
      }
   }
   #
   # Process some plan variables
   #
   if [info exists version] {
      if {$version != [photoVersion]} {
	 error [concat \
		    "Plan file specifies version $version, " \
		    "but you are using version [photoVersion]"]
      }
   }

   if {$compress_image_files == "0"} {
      set compress_image_files "";		# no compression
   } elseif {$compress_image_files == "1"} {
      set compress_image_files "R";		# default
   }
   regsub {^[.]} $compress_image_files "" compress_image_files

   set id_values(allow_mismatch) $allow_id_mismatch
   if {$read_fpC_files && $write_fpC_files} {
      error "You may not read and write fpC files"
   }
#
#  Expand all the directory names. If any are omitted, use $defaultDir.
# Note that all names are expanded, so they can contain a variable, e.g.:
# baseDir	/u/rhl/data
# run		745
# defaultDir	$baseDir/$run
#
# Process the second list (bias etc) second so that they may refer to the first
#
   eval set baseDir $baseDir
   foreach dir [concat \
		    "config image output ps" \
		    "bias ff fpC ko parameters psBB transFile"] {
      set dir ${dir}Dir

      if ![info exists $dir] {
	 if {$dir == "koDir"} {
	    verb_echo 2 "You haven't specified any known objects"
	    continue;
	 }
	 if [info exists defaultDir] {
	    set $dir $defaultDir
	 } else {
	    echo "No defaultDir; unable to set $dir"; continue
	 }
      }
      eval set $dir [set $dir]
      
      set $dir [envscan [set $dir]]

      verb_echo 5 [format %-15s $dir] = [set $dir]
   }

   if $read_fpC_files {
      set biasDir ""       
   } elseif $read_psBB_files {
      set biasDir $psDir
   }
   
   set runstr [format %06d $run]
   #
   # If we are to reconstruct the fields from fpAtlas/fpBIN files
   # we don't need to process fpParam etc. files
   #
   if $reconstruct {
      return [reconstruct_fpC_files $fpCDir $bkgd \
		 $filterlist $run $camCol $rerun $startField $endField $select]
   }

   if {$bkgd != "NULL"} {
      error "-bkgd only makes sense with -reconstruct_fpC"
   }
   #
   # Read the software and hardware parameters, but don't set them.
   # Let the individual procs do that as necessary.
   # Also reate a CCDPARS with a defectlist for each filter

   param2Chain [envscan \$PHOTO_DIR]/etc/fpParam.par defpars

   if {[info exists parametersDir] && [info exists parameters] &&
       [file exists $parametersDir/$parameters]} {
      param2Chain $parametersDir/$parameters softpars
   } else {
      set softpars ""
   }
   #
   # choose the _last_ occurrence of each keyword, in accordance with
   # behaviour of the plan files
   #
   foreach file "defpars softpars" {
      foreach el [set $file] {
	 set key [lindex $el 0]
	 set val [join [lrange $el 1 end]]
	 keylset $file $key $val
      }
   }
   #
   # Set param values from the plan file
   #
   if {[keylget fpplan no_overlap no_overlap] && $no_overlap} {
      if {[keylget softpars scan_overlap scan_overlap] &&
	  $scan_overlap != 0} {
	 echo \
	  "You shouldn't specify no_overlap in fpPlan and scan_overlap in fpParam (ignoring scan_overlap)"
      }
      keylset softpars scan_overlap 0
   }
   if [keylget fpplan no_CR_removal] {
      if {[keylget softpars cr_min_sigma cr_min_sigma] &&
	  $cr_min_sigma != 0} {
	 error \
       "You mayn't specify no_CR_removal in fpPlan and cr_min_sigma in fpParam"
      }
      keylset softpars cr_min_sigma 0
   }

   fetch_ccdpars $configDir $ccdConfig $ccdECalib $ccdBC \
       ccdpars $filterlist $camCol $fpplan
   #
   # There's a bug in dervish by which regInfoGet allocates memory the first
   # time that it's called; we call it here to ensure that this happens
   # outside the main photo loop. saoDisplay has a similar problem
   #
   if 1 {
      global sao
      set reg [regNew 1 1]; regClear $reg
      regInfoGet $reg
      if {$display != 0} {
	 global display_filterlist
	 if {![info exists sao]} { echo Creating SAODisplays... }
	 foreach f "default" {
	    if {[lsearch $display_filterlist $f] >= 0} {
	       if {![info exists sao($f)]} {
		  puts "Display for $f"
		  display $reg "" $f 0 -1
	       }
	       saoReset $sao($f); saoLabel off $sao($f)
	    }
	 }
      }
      regDel $reg
   }

######################################################################
# Initialize each of the modules
######################################################################
   set nfilter [llength $filterlist]
   set fieldstat [fieldstatNew]
   set fieldparams [fieldparamsNew $filterlist]
   handleSet $fieldparams.run $run;	# can be useful for diagnostics/gdb
   handleSet $fieldparams.camCol $camCol

   init_read_frames $filterlist rawimages

   init_correct_frames $biasDir biasvector $ffDir flatvector \
       $rawncols $corrows $corcols \
       cr_keep cr_min_sigma cr_min_e cr_cond3 cr_cond32 fullWell

   set ob [object1New]
   set null [handleBindFromHandle [handleNew] *$ob.region]
   object1Del $ob

   set sky_object_size [getsoftpar sky_object_size]
   set nsky_objects [getsoftpar nsky_objects]
   set nsky_failures [getsoftpar nsky_failures]
   check_calibfile \
       $psDir/psField-$runstr-$camCol-[format %04d $startField].fit $filterlist

   init_random rand
   init_find_objects $fieldparams $corrows $corcols \
       ffo_levels ffo_median_size ffo_psf_filt_size \
       ffo_binshifts ffo_binned_thresholds

   init_measure_objects $fieldparams ref_color $rand
   
   declare_schematrans [llength $filterlist] 

   # set the global "ccdrow" array, which tells which row each filter is in
   foreach f $filterlist {
      set ccdrow($f) [keylget fpplan ccdrow_$f]
   }

   upvar startMem startMem; set startMem [memNextSerialNumber];

   loop field $startField [expr $endField+1] {
      echo [format "Correcting field %d-%d-%d" \
		[exprGet $fieldparams.run] [exprGet $fieldparams.camCol] \
		$field]      

      #
      # Reset the random seed, so each field will be repeatable. We XOR the
      # fp_random_seed with a number based on the run and field number, and
      # then use tclX's random number to propagate the randomness into all
      # the seed's digits. The result is deterministic, but is stored in the
      # header anyway in case you need it.
      #
      random seed [expr ([getsoftpar fp_random_seed] ^ ($run << 4)) ^ \
		    [string range [format "1%02d%02d%02d%02d" \
				       $field $field $field $field] 0 9]]
      set seed [random 99999999]
      phRandomSeedSet $rand $seed

      # Restore to state before entering loop

      correct_field

      loop_reset [memNextSerialNumber] biasvector corrimages mergedlist \
	  $kos $calib $fieldstat
   }
   # End loop for each field

   fieldstatDel $fieldstat; unset fieldstat
   fieldparamsDel $fieldparams; unset fieldparams

   # Fini each module
   fini_read_frames $filterlist rawimages
   fini_correct_frames ccdpars
   fini_random $rand
   fini_measure_objects

   fitsBinForgetSchemaTrans NULL;

   # delete other stuff that's left over
   foreach f $filterlist {
      if [info exists biasvector($f)] {
	 regDel $biasvector($f)
      }
      regDel $flatvector($f)
   }
   overlayDelAll

   return 0
}

#
# Actually correct a field
#
proc correct_field {} {
   uplevel {
      handleSet $fieldparams.fieldnum $field

      set fieldstr [format %04d $field]

      set kos [chainNew KNOWNOBJ]

      set psFieldFile $psDir/psField-$runstr-$camCol-$fieldstr.fit 

      set calib [read_calib1byframe $field $psFieldFile $filterlist]
      read_trans $calib \
	  [get_trans_filename $transFileDir $run "" $camCol] $camCol $filterlist
      #
      # loop over each filter, reading from disk and flat fielding
      #
      foreach f $filterlist {
	 set ifilter [lsearch $filterlist $f]

	 # Read in next frame
	 read_frames $f $rawimages($f) corrimages($f) biasvector($f) $imageDir

	 if {[info exists display_raw] && $display_raw} {
	    display $rawimages($f) "$f raw frame" default 1 1
	 }

	 # Flatfield the frame; reset the saturation level first
	 set fix_bias [getsoftpar fix_bias]
	 if {[info exists fix_bias] && $fix_bias} {
	    set fix_bias_flg "-fix_bias"
	 } else {
	    set fix_bias_flg ""
	 }

	 handleSet $fieldparams.frame<$ifilter>.fullWell<0> $fullWell(0,$f)
	 handleSet $fieldparams.frame<$ifilter>.fullWell<1> $fullWell(1,$f)

	 just_correct_frame $f $rawimages($f) $corrimages($f) $biasvector($f) \
		 $flatvector($f) $calib skyTypical skySigma $fpCDir \
		 $cr_keep $cr_min_sigma $cr_min_e $cr_cond3 $cr_cond32

	 if $sky_subtract {
	    just_sky_subtract $f $corrimages($f) $rand $ffo_median_size
	 }
	 #
	 # Did they want any objects removed?
	 #
	 if {$subtract != ""} {
	    subtract_objects $corrimages($f) $f $subtract
	 }
	 #
	 # And write results
	 #
	 display $corrimages($f) "$f corrected frame" $f 1

	 if {![info exists write_fpC_files] || $write_fpC_files} {
	    set outfile $fpCDir/fpC-$runstr-$f$camCol-$fieldstr.fit

	    if ![file isdirectory $fpCDir] {
	       mkdir -path $fpCDir
	    }

	    verb_echo 2 "Writing $outfile"
	    
	    if {$compress_image_files != ""} {
	       append outfile ".$compress_image_files"
	    }

	    hdrInsWithInt $corrimages($f).hdr "SOFTBIAS" [softBiasGet] \
		"software \"bias\" added to all DN"
	    hdrInsWithAscii $corrimages($f).hdr "BUNIT" "DNs"
	    hdrInsWithAscii $corrimages($f).hdr "FILTER" $f "filter used"
	    hdrInsWithInt $corrimages($f).hdr "CAMCOL" $camCol \
		"column in the imaging camera"
	    insert_id_values -frames $corrimages($f).hdr	    
	    #
	    # Add WCS information to header
	    #
	    set index [lsearch $filterlist $f]
	    set trans [handleBindFromHandle [handleNew] \
			   *$calib.calib<$index>->toGCC]
	    atWcsAddToHdr $trans $corrimages($f).hdr \
		[exprGet $calib.calib<$index>->node] \
		[exprGet $calib.calib<$index>->incl]
	    handleDel $trans
      
	    cleanCorrectedFrameHeader $corrimages($f).hdr;# remove blanks etc.
      
	    regWriteAsFits $corrimages($f) $outfile
	 }
	 #
	 # Write out mask?
	 #
	 if $write_fpM_files {
	    write_fpM $f $corrimages($f) $fpCDir
	 }
      }
      #
      # Write out fieldstat?
      #
      if $write_fpFieldStat_files {
	 global FIELD
	 write_fieldstat $fpCDir $field $FIELD(OK)
      }
   }
}

proc just_correct_frame {filter rawimage corrimage biasvector \
			    flatvector calib skyTyp skyErr outdir \
			    cr_keep cr_min_sigma cr_min_e cr_cond3 cr_cond32 \
			     {psfBasis ""}} {
   upvar $skyTyp skyTypical $skyErr skySigma
   global PHOTO_CONSTS
   global corrows corcols
   global filterlist ccdpars
   global runstr camCol ccdrow field
   global fieldstat fieldparams
   global compress_image_files id_values
   global fix_bias
   
   set fieldstr [format %04d $field]
   
   set gain0 [exprGet $ccdpars($filter).gain0]
   if {[exprGet $ccdpars($filter).namps] == 1} {
      set gain1 $gain0
   } else {
      set gain1 [exprGet $ccdpars($filter).gain1]
   }
   set gain [expr ($gain0 + $gain1)/2.0]

   # Check that we grabbed the right calib
   set index [lsearch $filterlist $filter]
   assert {[exprGet $calib.calib<$index>->filter<0>] == $filter}

   set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<$index>]

   set nrow [exprGet $corrimage.nrow]; set ncol [exprGet $corrimage.ncol]
   set gainreg [binregionNewFromConst [exprGet $calib.calib<$index>->gain] \
		    -nrow $corrows -bin_row $corrows \
		    -ncol $corcols -bin_col $corcols -shift $PHOTO_CONSTS(MAX_U16)]
   handleSetFromHandle $fparams.gain &$gainreg;  # do we need to handleDel $gainReg? XX RHL

   foreach g "gain0 gain1" {
      handleSet $fparams.$g [set $g]
   }
   
   set row $ccdrow($filter)
   					# sky level/error from PSP
   set skyTypical($filter) [exprGet $calib.calib<$index>->sky]
   set skySigma($filter) [exprGet $calib.calib<$index>->skysig]

   hdrCopy $rawimage.hdr $corrimage.hdr; hdrFreeAll $rawimage.hdr
   
   verb_echo 4 "Flatfielding the $filter filter"
   # Flatfield the image
   set fix_bias [getsoftpar fix_bias]
   if {[info exists fix_bias] && $fix_bias} {
      set fix_bias_flg "-fix_bias"
   } else {
      set fix_bias_flg ""
   }
   
   set bias_scale [ hdrGetAsInt $biasvector.hdr TSHIFT]
   # need psf width for horizontal trail correction  
   set psf_temp [dgpsfNew]  
   handleSetFromHandle $fparams.psf &$psf_temp
   handleSet $fparams.psf->width [exprGet  $calib.calib<$index>->psf->width]
   set minval [expr [exprGet $calib.calib<$index>->sky] - \
		   2*[exprGet $calib.calib<$index>->skysig]]
   #
   # Set the fparams.sky from the PSP in case there are any FILCOL defects
   #
   set skyreg [binregionNewFromConst $skyTypical($filter)]
   handleSetFromHandle $fparams.sky &$skyreg
   
   eval correctFrames $rawimage $biasvector $flatvector $ccdpars($filter) \
       $fparams $bias_scale \
       -regout $corrimage \
       -leftbias [exprGet $calib.calib<$index>->lbias] \
       -rightbias [exprGet $calib.calib<$index>->rbias] \
       -minval $minval $fix_bias_flg
   
   dgpsfDel $psf_temp
   handleSet $fparams.sky 0x0; binregionDel $skyreg; unset skyreg
   #
   # Deal with CRs
   #
   if {$skySigma($filter) > 0} {
       #
       # Deal with CRs
       #
       if {$psfBasis == "" || $cr_cond32 > 0} {
	  set psfBasisCR "NULL"
       } else {
	  set psfBasisCR $psfBasis
       }
       set ncr [eval findCR $corrimage \
		    $skyTypical($filter) -bkgd $skyTypical($filter) \
		    $skySigma($filter) *$calib.calib<$index>->psf $gain \
		    -min_sigma $cr_min_sigma -min_e $cr_min_e \
		    -cond3 $cr_cond3 -cond32 [expr abs($cr_cond32)] \
		    [lindex {-keep {}} [expr $cr_keep?0:1]] \
		    -psfBasis $psfBasisCR -adopted_cond32 c32]

       handleSet $fieldstat.adopted_cond3_fac2<$index> $c32
       handleSet $fieldstat.nCR<$index> $ncr
   }

   handleDel $fparams
}


proc just_sky_subtract {filter in rand median_size } {
   global fieldparams filterlist MASK_TYPE sao

   set index [lsearch $filterlist $filter]
   #
   # Find desired parameters for frame
   #
   set nbit 10;			# number of bits to shift sky
   #
   # Unpack variables
   #
   set nrow [exprGet $in.nrow]
   set ncol [exprGet $in.ncol]
   set soft_bias [softBiasGet]
   #
   # Median smooth image, subtract sky (but remember to add back the
   # the software bias). If median_size <= 0, assume that the sky
   # has already been subtracted
   #
   if {$median_size <= 0} {
      set skyreg [handleBindFromHandle [handleNew] \
		      *$fieldparams.frame<$index>.sky]
   } else {
      set filt_c $median_size;
      set filt_r $median_size;
      set skysigreg [binregionNew]
      set skyreg [medianSky $in $filt_c $filt_r -shift $nbit \
		      -skysig $skysigreg];
      skyEstimateDebias -clipped $skyreg \
	  [exprGet $fparams.gain0] [exprGet $fparams.gain1]
      regIntConstAdd *$skyreg.reg [expr -$soft_bias*(1<<$nbit)];
      skySubtract $in $in $skyreg $rand
   }

   binregionDel $skyreg
   catch { binregionDel $skysigreg };	# may not exist
}

###############################################################################
#
# Subtract a set of objects from an fpC file
#
proc subtract_objects {corrimage filter subtract} {
   global rerun run camCol field
   set_run -rerun $rerun $run $camCol

   verb_echo 1 "Subtracting \"$subtract\" objects"

   set reg [recon $filter $field "" "" 0 "" !BRIGHT&&($subtract)]
   regIntLincom $corrimage $reg 1000 1 -1

   #error "$filter $subtract $corrimage $reg"

   regDel $reg

   return $corrimage
}

###############################################################################
#
# Reconstruct a set of fpC files from fpAtlas/fpBIN files
#
proc reconstruct_fpC_files {fpCDir bkgd filterlist \
			       run camCol rerun startField endField {sel ""}} {
   global display openit table

   if ![info exists display] {		# usually set in fpPlan file
      set display 0
   }

   uplevel #0 [list set_run -rerun $rerun $run $camCol]

   set fsao $display;			# 0 => no display

   loop field $startField [expr $endField + 1] {
      set openit(field) $field
      makeFpC $fpCDir $field -filter $filterlist -bkgd $bkgd -fsao $fsao \
	  -select $sel
   }

   return 0
}


###############################################################################
#
# Create coloured CM and three-colour diagrams
#
proc make_colored_diagrams {args} {
   global field objcIo table

   set z0 -0.8; set z1 2.1;		# default values for [xy][01]
   set y0_CM 24.2; set y1_CM 13.7;	# default y[01] if -CM

   set erase_regs 0;			# erase regs before running
   set ticks 0;				# generate tick marks
   set verbose 0;			# Be chatty?

   set opts [list \
		 [list [info level 0] "\
 Given a range of fields (or an open table, as returned by openit), create
 a set of 3 regions representing the e.g. g, r, and i images of the g-r-i
 3-colour diagram, or alternatively a CM diagram, coded in gri colour. If
 desired, convert these regions to a true-colour ppm image.

 E.g.
 set_run 756 3
 make_colored_diagrams regs -erase -sel sel_good&&sel_star&sel_gri \\
     -filters \"g r i\" -field 756 -maglim 23.5 -CM g-r:g \\
     -ppm_file Pal5-gr -ppm_stretch 0:1000 -smooth 1 -ticks
 display_ppm_file Pal5-gr-756.ppm
 "] \
		 [list <regs> STRING "" _regs \
		      "Array of regions to set, indexed by filter names.
 If it doesn't exist, regions will be created"] \
		 [list -field INTEGER 0 field0 \
		      "First field to plot (if 0, use current table)"] \
		 [list -field1 INTEGER 0 field1 \
		      "Last field to plot (if 0, just plot field0)"] \
		 [list -filters STRING "g r i" filters "Filters to use"] \
		 [list -CM STRING "" CM \
 "generate a CM (not 3-colour) diagram. E.g. -CM g-r:g to plot g-r v. g"] \
		 [list -ticks CONSTANT 1 ticks "Generate ticks"] \
		 [list -lweight INTEGER 1 lweight "Width of axes/ticks"] \
		 [list -x0 DOUBLE $z0 x0 "Lower range of x-axis"] \
		 [list -x1 DOUBLE $z1 x1 "Upper range of x-axis"] \
		 [list -y0 DOUBLE 0.0 y0 "Lower range of y-axis
 ($z0, unless -CM is specified when == $y0_CM)"] \
		 [list -y1 DOUBLE  0.0 y1 "Upper range of y-axis
 ($z1, unless -CM is specified when == $y1_CM)"] \
		 [list -smooth DOUBLE 2.0 sigma \
		      "Smooth regions with a Gaussian of this sigma"] \
		 [list -select STRING "" select "Selection function"] \
		 [list -maglim DOUBLE 100.0 maglim \
		      "Magnitude limit (in first band)"] \
		 [list -erase CONSTANT 1 erase_regs \
		      "Clear regions before use"] \
		 [list -type STRING "psf" magtype "Type of magnitude to use"] \
		 [list -margin INTEGER 0 ymargin \
		      "Add this many pixels of margin"] \
		 [list -xmargin INTEGER 0 xmargin \
		      "Add this many pixels of margin in x"] \
		 [list -nrow INTEGER 512 nrow "row-size of regions"] \
		 [list -ncol INTEGER 512 ncol "col-size of regions"] \
		 [list -n1 INTEGER 1 n1 "first object to display"] \
		 [list -n2 INTEGER -1 n2 "last object to display"] \
		 [list -ppm_file STRING "" ppm_file \
		      "Make a PPM file with this name"] \
		 [list -ppm_stretch STRING "" ppm_stretch \
		      "Use this stretch for PPM file"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty?"]\
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$field0 == 0 && ![info exists table]} {
      error "Please open a table with set_run/openit and try again"
   }

   if {$CM == ""} {
      set CM 0
      set magband 1;			# use first band for magnitude limit
   } else {
      if ![regexp {^([a-zA-Z])-([a-zA-Z]):([a-zA-Z])$} \
	       $CM {} CMband1 CMband2 magband] {
	 error "-CM should be in the form \"g-r:g\""
      }
      set CM 1
   }

   if {$magtype == "psf"} {
      set mag ${magtype}Counts
   } else {
      set mag counts_$magtype
   }

   if {$y0 == $y1} {
      if $CM {
	 set y0 $y0_CM; set y1 $y1_CM
      } else {
	 set y0 $z0; set y1 $z1
      }
   }
   #
   # Set margins
   #
   if {$ticks && $ymargin == 0} {
      set ymargin 75
   }
   if {$xmargin == 0} {
      set xmargin [expr int(1.25*$ymargin)]
   }
   #
   # Create output regions if necessary
   #
   upvar $_regs regs

   foreach f $filters {
      if $erase_regs {
	 if [info exists regs($f)] {
	    catch {regDel $regs($f)}
	    unset regs($f)
	 }
      }

      if ![info exists regs($f)] {
	 set xmargin_l $xmargin; set xmargin_r [expr int(0.3*$xmargin)]
	 set ymargin_l $ymargin; set ymargin_r [expr int(0.3*$ymargin)]
	 set regs($f) \
	     [regNew [expr $nrow + $ymargin_l + $ymargin_r] \
		  [expr $ncol + $xmargin_l + $xmargin_r]]
	 regIntSetVal $regs($f) 0
      }
   }
   set nrow_full [exprGet $regs([lindex $filters 0]).nrow]
   set ncol_full [exprGet $regs([lindex $filters 0]).ncol]
   #
   # Set axis labels
   #
   if $CM {
      set xlab "$CMband1 - $CMband2"
      set ylab "$magband"
   } else {
      set xlab "[lindex $filters 0] - [lindex $filters 1]"
      set ylab "[lindex $filters 1] - [lindex $filters 2]"
   }
   #
   # If this is a CM diagram, lookup which filters are desired
   #
   if $CM {
      foreach b "CMband1 CMband2 magband" {
	 set ib [expr [lsearch $filters [set $b]] + 1]

	 if {$ib == 0} {
	    error "Unknown band [set $b]"
	 }

	 set $b $ib
      }
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]
   #
   # Calculate required mapping from colour to row/col
   #
   set scalex [expr 1.0*$nrow/(1.0*$x1 - $x0)]
   set scaley [expr 1.0*$ncol/($y1 - $y0)]

   if {$field1 == 0} { set field1 $field0 }

   loop field $field0 [expr $field1 + 1] {
      if {$field != 0} {
	 if $verbose {
	    puts -nonewline "$field "; flush stdout
	    if {($field - $field0 + 1)%20 == 0} { echo "" }
	 }
	 set table [openit -noAI $field]
	 index_table table -identity
	 
	 if {$field != $field0} {
	    set n2 -1
	 }
      }
      #
      # Lookup array indices for those bands.
      # We may only just have acquired a table
      #
      if ![info exists ifilters] {
	 foreach f $filters {
	    lappend ifilters [lsearch [keylget table filters] $f]
	 }
      }
      #
      # Read data
      #
      if {$n2 == -1} {
	 set n2 [keylget table OBJnrow]
      }
      
      loop i $n1 [expr $n2 + 1] {
	 catch {catObjDel $objcIo -deep};	# it may not exist
	 set objcIo [objcIoRead table $i]
	 
	 if {"$select" != "" && ![eval $select $objcIo]} {
	    continue;
	 }
	 
	 set k 0
	 foreach j $ifilters {
	    set m[incr k] [get_mag_from_counts $objcIo $j $magtype]
	 }
	 
	 if {[set m$magband] > $maglim} {
	    continue
	 }
	 
	 if $CM {
	    set c1 [expr [set m$CMband1] - [set m$CMband2]];
	    
	    set x [expr int(($c1 - $x0)*$scalex + 0.5)]
	    set y [expr int(([set m$magband] - $y0)*$scaley + 0.5)]
	 } else {				# 3-colour
	    set c1 [expr $m1 - $m2]; set c2 [expr $m2 - $m3]
	    
	    set x [expr int(($c1 - $x0)*$scalex + 0.5)]
	    set y [expr int(($c2 - $y0)*$scaley + 0.5)]
	 }
	 
	 if {$x >= 0 && $x < $ncol && $y >= 0 && $y < $nrow} {
	    set k 0
	    foreach j $ifilters {
	       set cts[incr k] [exprGet $objcIo.$mag<$j>]
	    }

	    set ival 5000.5;		# n.b. a float not an int
	    if {$cts3 > $cts2} {
	       if {$cts3 > $cts1} {
		  set ival1 [expr $cts1*$ival/$cts3]
		  set ival2 [expr $cts2*$ival/$cts3]
		  set ival3 $ival
	       } else {
		  set ival1 $ival
		  set ival2 [expr $cts2*$ival/$cts1]
		  set ival3 [expr $cts3*$ival/$cts1]
	       }
	    } else {
	       if {$cts2 > $cts1} {
		  set ival1 [expr $cts1*$ival/$cts2]
		  set ival2 $ival
		  set ival3 [expr $cts3*$ival/$cts2]
	       } else {
		  set ival1 $ival
		  set ival2 [expr $cts2*$ival/$cts1]
		  set ival3 [expr $cts3*$ival/$cts1]
	       }
	    }
	    
	    set k 0
	    incr x $xmargin; incr y $ymargin
	    foreach f $filters {
	       #regPixSetWithDbl $regs($f) $y $x [set ival[incr k]]
	       regPixSetWithDbl $regs($f) $y $x \
		   [expr [regPixGet $regs($f) $y $x] + [set ival[incr k]]]
	    }
	 }
      }
   }
   if $verbose {
      echo ""
   }
   #
   # Add soft bias
   #
   foreach f $filters {
      regIntConstAdd $regs($f) 1000
   }
   #
   # Smooth?
   #
   if {$sigma > 0} {
      foreach f $filters {
	 convolveWithGaussian $regs($f) $regs($f) NULL 21 $sigma
      }
   }
   #
   # Tick marks?
   #
   if $ticks {
      set ticksize0 10
      set tickval 65535
      #
      # First draw the box
      #
      set hlw [expr int($lweight/2)]
      foreach f $filters {
	 loop lw -$hlw [expr $hlw + 1] {
	    loop r 0 [expr $nrow + 2*$hlw] {# y axis/axes
	       regPixSetWithDbl $regs($f) \
		 [expr $ymargin - $hlw + $r] [expr $xmargin + $lw] $tickval
	       regPixSetWithDbl $regs($f) \
		 [expr $ymargin - $hlw + $r] [expr $xmargin + $lw + $nrow - 1]\
		   $tickval
	    }
	    loop c 0 [expr $ncol + 2*$hlw] {# x-axis/es
	       regPixSetWithDbl $regs($f) \
		 [expr $ymargin + $lw] [expr $xmargin - $hlw + $c] $tickval
	       regPixSetWithDbl $regs($f) \
		 [expr $ymargin + $lw + $nrow - 1] [expr $xmargin - $hlw + $c]\
		   $tickval
	    }
	 }
      }
      #
      # Then the ticks
      #
      set psize 24;			# pointsize of labels
      set xtick ""; set ytick ""

      set x [expr int($x0) - 1]; set dx 0.5
      while {$x <= int($x1) + 1} {
	 lappend xtick $x
	 set x [expr $x + $dx]
      }

      if $CM {
	 set y [expr int($y0) + 1]
	 while {$y >= int($y1) + 1} {
	    lappend ytick $y
	    set y [expr $y - 1]
	 }
      } else {
	 set y [expr int($y0) - 1]
	 while {$y <= int($y1) + 1} {
	    lappend ytick $y
	    set y [expr $y + 0.5]
	 }
      }

      foreach x $xtick {
	 set ix [expr int(($x - $x0)*$scalex + 0.5)]
	 
	 if {$ix >= 0 && $ix < $ncol} {
	    if {$x == int($x)} {
	       set ticksize [expr 2*$ticksize0]
	    } else {
	       set ticksize $ticksize0
	    }

	    incr ix $xmargin

	    set dx 0.7
	    if {$x < 0} {
	       set dx [expr $dx + 0.7]
	    }
	    append labels \
		[format "text %d,%d \"$x\" " \
		     [expr int($ix - $dx*$psize)] \
		     [expr int($nrow_full - $ymargin - 1 + 1.5*$psize)]]

	    foreach f $filters {
	       loop lw -$hlw [expr $hlw + 1] {
		  loop r 0 $ticksize {
		     regPixSetWithDbl $regs($f) \
			 [expr $ymargin + $r] [expr $ix + $lw] $tickval
		     regPixSetWithDbl $regs($f) \
			 [expr $ymargin + $nrow - $r - 1] [expr $ix + $lw] \
			 $tickval
		  }
	       }
	    }
	 }
      }

      foreach y $ytick {
	 set iy [expr int(($y - $y0)*$scaley + 0.5)]
	 
	 if {$iy >= 0 && $iy < $nrow} {
	    if {($CM && $y/5 == 0.2*$y) || (!$CM && $y == int($y))} {
	       set ticksize [expr 2*$ticksize0]
	    } else {
	       set ticksize $ticksize0
	    }

	    incr iy $ymargin

	    set dx 2.0
	    if {$y < 0} {
	       set dx [expr $dx + 0.7]
	    }
	    append labels [format "text %d,%d \"$y\" " \
				[expr int($xmargin - $dx*$psize)] \
				[expr int($nrow_full - $iy - 1 + 0.45*$psize)]]

	    foreach f $filters {
	       loop lw -$hlw [expr $hlw + 1] {
		  loop c 0 $ticksize {
		     regPixSetWithDbl \
			 $regs($f) [expr $iy+$lw] [expr $xmargin + $c] $tickval
		     regPixSetWithDbl $regs($f) \
			 [expr $iy+$lw] [expr $xmargin + $ncol - $c - 1] \
			 $tickval
		  }
	       }
	    }
	 }
      }
      #
      # Axis labels
      #
      append labels \
	  [format "text %d,%d \"$xlab\" " \
	       [expr $xmargin + int(0.5*($x1 - $x0)*$scalex + 0.5)] \
	       [expr int($nrow_full - 0.6*$psize)]]
      append labels \
	  [format "text %d,%d \"$ylab\" " \
	       [expr int(0.4*$psize)] \
	       [expr int($nrow_full - $ymargin - (0.5*($y1 - $y0)*$scaley + 0.5))]]
   }
   #
   # Make PPM file?
   #
   if {$ppm_file == ""} {
      set ppm_file "none"
   }

   loop i [expr [llength $filters]-1] -1 -1 {
      set f [lindex $filters $i]
      lappend reglist $f:$regs($f)
   }
   
   if {$ppm_stretch == ""} {
      set ppm_stretch 0:[expr int($ival/pow($sigma,2))]
   }
   
   array set sky [list u 11  g 6  r 6  i 6  z 6];# will get put back by prepare_regions
   
   set file [eval make_ppm_file $ppm_file [keylget table field] [keylget table field]\
		 $reglist -stretch $ppm_stretch -use_calib [expr 0x1] \
		 -nofix_saturated_LUT -type sqrt -sky sky]
   
   if [info exists labels] {
      set font "helvetica"
      if {$lweight > 1} {
	 append font "-bold"
      }
      exec mogrify -fill white -font ps:$font -pointsize $psize \
	  -draw "$labels" $file
   }

   return $file
}

proc sel_r {o {b 2}} {			# detected in r?
   global OBJECT1

   foreach c "1 2 3" {			# OK in u, g, and r?
      if {([exprGet $o.flags<$c>] & $OBJECT1(SATUR))} {
	 return 0
      }

      if ![sel_good $o $c] {
	 return 0
      }
   }
   

   if {([exprGet $o.flags<2>] & $OBJECT1(BINNED1))} {
      return 1
   }

   return 0
}

proc sel_g {o {b 2}} {			# detected in g?
   global OBJECT1

   foreach c "1 2 3" {			# OK in u, g, and r?
      if {([exprGet $o.flags<$c>] & $OBJECT1(SATUR))} {
	 return 0
      }

      if ![sel_good $o $c] {
	 return 0
      }
   }
   

   if {([exprGet $o.flags<1>] & $OBJECT1(BINNED1))} {
      return 1
   }

   return 0
}

proc sel_gr {o {b 2}} {			# detected in g and r?
   global OBJECT1

   foreach c "1 2 3" {			# OK in u, g, and r?
      if {([exprGet $o.flags<$c>] & $OBJECT1(SATUR))} {
	 return 0
      }

      if ![sel_good $o $c] {
	 return 0
      }
   }
   

   if {([exprGet $o.flags<1>] & $OBJECT1(BINNED1)) &&
       ([exprGet $o.flags<2>] & $OBJECT1(BINNED1))} {
      return 1
   }

   return 0
}

proc sel_gri {o {b 2}} {		# detected in g, r, and i?
   global OBJECT1

   foreach c "1 2 3" {			# OK in u, g, and r?
      if ![sel_good $o $c] {
	 return 0
      }
   }

   if {([exprGet $o.flags<1>] & $OBJECT1(BINNED1)) &&
       ([exprGet $o.flags<2>] & $OBJECT1(BINNED1)) &&
       ([exprGet $o.flags<3>] & $OBJECT1(BINNED1))} {
      return 1
   }

   return 0
}

###############################################################################

proc scramble {args} {
   set ticks 0;				# draw ticks?
   set verbose 0
   
   set opts [list \
		 [list [info level 0] \
 "Take a set of e.g. gri images and make an image in e.g. g-r v. r-i space"] \
		 [list <regions> STRING "" _regions "Input images"] \
		 [list <filters> STRING "" filters "Filter names"] \
		 [list <sky> STRING "" _sky "Array of sky values"] \
		 [list -sigma DOUBLE 0 sigma \
		      "Smooth with an N(0,sigma^2) Gaussian"] \
		 [list -x0 DOUBLE -0.5 x0 "Lower range of x-axis"] \
		 [list -x1 DOUBLE 2 x1 "Upper range of x-axis"] \
		 [list -y0 DOUBLE -0.5 y0 "Lower range of y-axis"] \
		 [list -y1 DOUBLE 2 y1 "Upper range of y-axis"] \
		 [list -psize INTEGER 24 psize "Point size for labels"] \
		 [list -margin INTEGER 0 ymargin \
		      "Add this many pixels of margin"] \
		 [list -xmargin INTEGER 0 xmargin \
		      "Add this many pixels of margin in x"] \
		 [list -nrow INTEGER 512 Nrow "row-size of region"] \
		 [list -ncol INTEGER 512 Ncol "col-size of region"] \
		 [list -ticks CONSTANT 1 ticks "Generate ticks"] \
		 [list -xlabel STRING "" xlab "X-axis label"] \
		 [list -ylabel STRING "" ylab "Y-axis label"] \
		 [list -lweight INTEGER 1 lweight "Width of axes/ticks"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 ]   

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_regions regions
   set nrow [exprGet [lindex $regions 0].nrow];#get size of regions 
   set ncol [exprGet [lindex $regions 0].ncol]
   #
   # Set margins
   #
   if {$ticks && $ymargin == 0} {
      set ymargin 75
   }
   if {$xmargin == 0} {
      set xmargin [expr int(1.25*$ymargin)]
   }
   
   #
   # Create output regions if necessary
   #
   set xmargin_l $xmargin; set xmargin_r [expr int(0.3*$xmargin)]
   set ymargin_l $ymargin; set ymargin_r [expr int(0.3*$ymargin)]
   
   set i 0
   foreach f $filters {		# create output regions
      set regs($i) [lindex $regions $i]
      
      set hregs($i) \
	  [regNew -type FL32 [expr $Nrow + $ymargin_l + $ymargin_r] \
	       [expr $Ncol + $xmargin_l + $xmargin_r]]
      regClear $hregs($i)
      
      incr i
   }
   set nrow_full [exprGet $hregs(0).nrow]
   set ncol_full [exprGet $hregs(0).ncol]
   
   set rscale [expr 1.0*$Nrow/($y1 - $y0)]
   set cscale [expr 1.0*$Ncol/($x1 - $x0)]
   set minval 100;			# XXX range of pixel values to include
   loop r 0 $nrow {
      if $verbose {
	 puts -nonewline "$r\r"; flush stdout
      }
      loop c 0 $ncol {
	 set v0 [regPixGet $regs(0) $r $c]
	 set v1 [regPixGet $regs(1) $r $c]
	 set v2 [regPixGet $regs(2) $r $c]
	 
	 if {$v0 == 0 || $v1 == 0 || $v2 == 0} {
	    continue
	 }
	 if {$v0 < $minval || $v1 < $minval || $v2 < $minval} {
	    continue
	 }

	 set hr [expr int((1.0*$v2/$v1 - $y0)*$rscale + 0.5)]
	 set hc [expr int((1.0*$v1/$v0 - $x0)*$cscale + 0.5)]
	 
	 if {0} {
	    puts -nonewline "$v0 $v1 $v2 $hr $hc [expr 1.0*$v0/$v1] [expr 1.0*$v1/$v2]\n"; flush stdout
	 }
	 
	 #set hr $r; set hc $c;	# RHL XXX
	 
	 if {$hr >= 0 && $hr < $Nrow && $hc >= 0 && $hc < $Ncol} {
	    incr hr $ymargin_l; incr hc $xmargin_l
	    
	    regPixSetWithDbl $hregs(0) $hr $hc [expr $v0 + [regPixGet $hregs(0) $hr $hc]]
	    regPixSetWithDbl $hregs(1) $hr $hc [expr $v1 + [regPixGet $hregs(1) $hr $hc]]
	    regPixSetWithDbl $hregs(2) $hr $hc [expr $v2 + [regPixGet $hregs(2) $hr $hc]]
	 }
      }
   }
   
   if $verbose {
      echo ""
   }
   
   set max 0
   loop i 0 3 {
      set vals [regStatsFind $hregs($i)]
      if {[keylget vals high] > $max} {
	 set max [keylget vals high]
      }
   }
   set scale [expr 60000.0/$max]
   
   unset regions
   loop i 0 3 {
      regMultiplyWithDbl $hregs($i) $scale
      set reg [regIntConvert $hregs($i) -type U16]
      regDel $regs($i);
      regDel $hregs($i)
      
      lappend regions $reg; set hregs($i) $reg
      
      regStatsFromQuartiles -clip 2 -med med $reg -coarse 1
      
      set f [lindex $filters $i]
      if ![info exists sky($f)] {
	 set sky($f) 0
      }
      set bkgd [expr int($sky($f) + [softBiasGet] + 0.5 - $med)]
      regIntConstAdd $reg $bkgd
      #mtv $reg; puts -nonewline "$f "; gets stdin
      
      if {$sigma > 0} {
	 convolveWithGaussian $reg $reg "" 11 $sigma
      }
   }
   #
   # Tick marks?
   #
   if !$ticks {
      return ""
   }

   set ticksize0 7
   set tickval 65535
   #
   # First draw the box
   #
   set hlw [expr int($lweight/2)]
   loop i 0 3 {
      loop lw -$hlw [expr $hlw + 1] {
	 loop r 0 [expr $Nrow + 2*$hlw] {# y axis/axes
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin - $hlw + $r] [expr $xmargin + $lw] $tickval
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin - $hlw + $r] [expr $xmargin + $lw + $Nrow - 1]\
		$tickval
	 }
	 loop c 0 [expr $Ncol + 2*$hlw] {# x-axis/es
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin + $lw] [expr $xmargin - $hlw + $c] $tickval
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin + $lw + $Nrow - 1] [expr $xmargin - $hlw + $c]\
		$tickval
	 }
      }
   }
   #
   # Then the ticks
   #
   set xtick ""; set ytick ""
   
   set x [expr int($x0) - 1]; set dx 0.5
   while {$x <= int($x1) + 1} {
      lappend xtick $x
      set x [expr $x + $dx]
   }
   
   set y [expr int($y0) - 1]
   while {$y <= int($y1) + 1} {
      lappend ytick $y
      set y [expr $y + 0.5]
   }
   
   foreach x $xtick {
      set ix [expr int(($x - $x0)*$cscale + 0.5)]
      
      if {$ix >= 0 && $ix < $Ncol} {
	 if {$x == int($x)} {
	    set ticksize [expr 2*$ticksize0]
	 } else {
	    set ticksize $ticksize0
	 }
	 
	 incr ix $xmargin
	 
	 set dx 0.7
	 if {$x < 0} {
	    set dx [expr $dx + 0.7]
	 }
	 if {$ticksize > $ticksize0} {
	    append labels \
		[format "text %d,%d \"$x\" " \
		     [expr int($ix - $dx*$psize)] \
		     [expr int($nrow_full - $ymargin - 1 + 1.5*$psize)]]
	 }
	 
	 loop i 0 3 {
	    loop lw -$hlw [expr $hlw + 1] {
	       loop r 0 $ticksize {
		  regPixSetWithDbl $hregs($i) \
		      [expr $ymargin + $r] [expr $ix + $lw] $tickval
		  regPixSetWithDbl $hregs($i) \
		      [expr $ymargin + $Nrow - $r - 1] [expr $ix + $lw] \
		      $tickval
	       }
	    }
	 }
      }
   }
   
   foreach y $ytick {
      set iy [expr int(($y - $y0)*$rscale + 0.5)]
      
      if {$iy >= 0 && $iy < $Nrow} {
	 if {$y == int($y)} {
	    set ticksize [expr 2*$ticksize0]
	 } else {
	    set ticksize $ticksize0
	 }
	 
	 incr iy $ymargin
	 
	 set dx 2.0
	 if {$y < 0} {
	    set dx [expr $dx + 0.7]
	 }
	 if {$ticksize > $ticksize0} {
	    append labels [format "text %d,%d \"$y\" " \
			       [expr int($xmargin - $dx*$psize)] \
			       [expr int($nrow_full - $iy - 1 + 0.45*$psize)]]
	 }
	 
	 loop i 0 3 {
	    loop lw -$hlw [expr $hlw + 1] {
	       loop c 0 $ticksize {
		  regPixSetWithDbl \
		      $hregs($i) [expr $iy+$lw] [expr $xmargin + $c] $tickval
		  regPixSetWithDbl $hregs($i) \
		      [expr $iy+$lw] [expr $xmargin + $Ncol - $c - 1] \
		      $tickval
	       }
	    }
	 }
      }
   }
   #
   # Axis labels
   #
   append labels \
       [format "text %d,%d \"$xlab\" " \
	    [expr $xmargin + int(0.5*($x1 - $x0)*$cscale + 0.5)] \
	    [expr int($nrow_full - 0.6*$psize)]]
   append labels \
       [format "text %d,%d \"$ylab\" " \
	    [expr int(0.4*$psize)] \
	    [expr int($nrow_full - $ymargin - (0.5*($y1 - $y0)*$rscale +0.5))]]

   return $labels
}

################################################
# procs for making pretty pictures...

# given run, col, field, row and col, for $stamps != 0 cut g-r-i 
# $stampsize pixe lstamps, and for $ppm != 0 produce a ppm color image. 
# if $stamps < 0 remove fits files after producing ppm file
# stretch is log by default. for other options see comments above
# regU16ToU8LUTGet 
# n.b. gri combination works best, especially for low SB objects
# n.b. fits files are 40 pixels wider than requested size so that 
# color images can be trimmed around the edges. 
proc get_ppm_stamp {run camCol field row col {stampsize 750} \
	            {name gps_out}  {stamps 1} {ppm 1} {stretch -1} \
		    {filters {g r i}} {use_fpC 0} {rerun -1}} {

# needed for set_run
global data_root
global openit
global ignore_marks cross_length cross_width cross_clearance

     # account for trimming
     set trim_size 20
     set maxtrim_row [expr $row - $stampsize/2]
     set maxtrim_col [expr $col - $stampsize/2]
     if {$maxtrim_row < 0}  {set maxtrim_row 0}     
     if {$maxtrim_col < 0}  {set maxtrim_col 0}     
     if {$maxtrim_row < $trim_size} {set trim_size $maxtrim_row}
     if {$maxtrim_col < $trim_size} {set trim_size $maxtrim_col}
     set sizewithedge [expr $stampsize + 2*$trim_size]
     if {$trim_size < 5} {
         echo "  ## your stamp is positioned too close to the frame edge ##"
         echo "  ##  --- there may be some undesirable color effects --- ##"
     }

     if {$rerun < 0} {
        set rerun [get_highest_dir $data_root/$run]
        if {$rerun < 0} {
            error "Directory $data_root/$run doesn't contain any reruns!?"
        }  
     }

     # first cut g r i stamps
     if {$stamps != 0} {
       foreach filter $filters {
         # get the reconstructed frames
         echo "cutting $filter stamp..."
	 if {!$use_fpC} {
             set frame [get_field $run $camCol $field $filter $data_root $rerun]
         } else {
             set runstr [format "%06d" $run]
             set fieldstr [format "%04d" $field]
             if {![info exists openit]} {
                 set_run $run $camCol -rerun $rerun 
	     } 
             set fpC_file $openit(objdir)/fpC-${runstr}-${filter}${camCol}-$fieldstr.fit
             if [catch {
                         set frame [regReadAsFits [regNew] $fpC_file]
	        } msg] {
                error "problem with fpC file: $msg"               
             } 
         }
         # if requested, mark the cross
         if {[info exist ignore_marks] && $ignore_marks < 0 && $filter == "r"} {           
             if {![info exist cross_length]} {
                 set cross_length 30

             }
             if {![info exist cross_width]} {
                 set cross_width 1

             }
             if {![info exist cross_clearance]} {
                 set cross_clearance 15

             }
             mark_cross $frame $row $col $cross_length \
                        $cross_width $cross_clearance
         }
         # given the frame, cut out the stamp
         set stamp [stamp_from_region $frame $row $col $sizewithedge offsets]
         regWriteAsFits $stamp $name.$filter.fit
         regDel $stamp
         regDel $frame
       }
         echo "stamps are cut"
     } else {
         echo "bypassing cutting stamps"
     }
     
     foreach filter $filters {
        set ifilter [lsearch $filters $filter]
        set f($ifilter) $filter:$name.$filter.fit
     }

     # make ppm file
     if {$ppm} {
       echo "producing ppm file"
       # default stretch works nicely for low SB objects
       if {$stretch < 0 && $stretch != "histeq"} {set stretch 2:300s}
       # log stretch seems to work best
       set type 2
       if [catch {
	 3fits2ppm $run $camCol $field . $f(2) $f(1) $f(0) $name $stretch $type $trim_size
         echo "moving $name-$field.ppm to $name.ppm" 
         exec mv $name-$field.ppm $name.ppm
       } msg] {
         echo "Problems in 3fits2ppm: $msg"
       }
       if {$stamps < 0} {
           foreach filter $filters {
              exec rm $name.$filter.fit
           }
       }
     } else {
         echo "bypassing ppm file"
     }
  
     echo "Done."

}

# given 3 fits files from a given run, column and field, make a ppm file
# fits files are in $dir and are specified as $filter:$file_name
# e.g.
# set f1 g:stamp1.745.2.215.g.fit, etc. for r and i
# 3fits2ppm 745 2 215 . $f1 $f2 $f3 test_stamp1 
#
proc 3fits2ppm {run column field dir f1 f2 f3 name {stretch 10} {s_type 2} \
                    {trimmed_edge_width 0} } {
     global openit

     set openit(indir) $openit(asdir)

     # form arguments for make_ppm_file
     set reg1 [regReadAsFits [regNew] \
              [string range $f1 2 [string length $f1]]]
     set arg1 [string range $f1 0 0]:$reg1
     set reg2 [regReadAsFits [regNew] \
              [string range $f2 2 [string length $f2]]]
     set arg2 [string range $f2 0 0]:$reg2
     set reg3 [regReadAsFits [regNew] \
              [string range $f3 2 [string length $f3]]]
     set arg3 [string range $f3 0 0]:$reg3
     # stretch limits
     if {$stretch != "histeq" && [string first : $stretch] == -1} {
         set type 2:${stretch}s
     } else {
         set type $stretch
     }
     # edge trimming
     if {$trimmed_edge_width > 0} {
         set nrow [exprGet $reg1.nrow]
         set ncol [exprGet $reg1.ncol]
         if {$trimmed_edge_width > $nrow/2 || $trimmed_edge_width > $ncol/2} {
             error "edge width for trimming is larger than 1/2 the stamp size !?!"
         }
         set row0 $trimmed_edge_width 
         set col0 $trimmed_edge_width 
         set nrow [expr $nrow - 2*$trimmed_edge_width] 
         set ncol [expr $ncol - 2*$trimmed_edge_width]   
     } else {
         set row0 0
         set col0 0
         set nrow -1
         set ncol -1          
     }
   
     # call the work horse
     if [catch {
         make_ppm_file $name $field $field $arg1 $arg2 $arg3 -bin 1  \
                      -stretch $type -type $s_type -nrow $nrow -ncol $ncol \
                      -row0 $row0 -col0 $col0 -nofix_saturated_LUT      
     } msg] {
        echo "problems in make_ppm_file: $msg"
     } 

     regDel $reg1; regDel $reg2; regDel $reg3

     # XXX leftover regions from make_ppm_file
     set list [handleListFromType REGION]
     foreach el $list {
        regDel $el
     }

}


# serial production of pretty pictures: 
# make ppm files for a list of stamps assuming that $stampslist includes 
# names of the form $run-$col-$field-*-$filter.fit (i.e. output from 
# catalog_match.tcl procedures) and that the current directory contains
# 3 fits files for {g r i} bands. 
# trimmed_edge_width specifies how many pixels to trim off each edge
# stretching is controlled by $stretch and $s_type, for details see 
# comments above regU16ToU8LUTGet 
# N.B.
# this proc is supposed to work together with procs for matching a guest
# catalog to SDSS imaging data. One can make color stamps of guest objects
# by performing following steps (for more details see Photo Primer):
# 1) 
# match the guest catalog to an SDSS run by using guest2stamps, e.g.  
# photo> guests2stamps 1 1 $run $guest_cat $rad 1 $dir 790 {g r i}
# => this will produce g-r-i 790x790 pixels stamps in directory $dir for all 
# guest objects that have at least one SDSS object from run $run closer 
# than $rad arcsec
# 2)
# produce ppm files from these stamps by running stamps2ppm in the local dir. 
# For this first produce a list of all stamps (in one band), e.g. 
# ls *g.fit > stamps_list
# and then 
# stamps2ppm stamps_list
# n.b. in this example fits files are 790x790 pixels, while ppm files are 
# trimmed for 20 pixels around the edges and thus are 750x750 pixels (5'x5')
# 3)
# the first two steps will results in 3 fits files and 1 ppm file for each
# object. after ppm files are produced all fits files can be deleted (one
# can also produce fits mosaics (in one band) by using fits2mosaics, see
# below). 
# n.b. ppm files can be converted to other formats by using Unix utilities, 
# e.g. pnmtops for producing PS files
proc stamps2ppm {stampslist {trimmed_edge_width 20} {stretch -1} {s_type 2}} {

    set stamp_list [open $stampslist]

    while {![eof $stamp_list]} { 
	set stamp [gets $stamp_list]
        if {$stamp != ""} {
          echo " working on $stamp" 
          # get root
          set i [string last . $stamp]
          set root [string range $stamp 0 [expr $i -3]]
          # get run, col and field
          set i [string first - $stamp]
          set run [string range $stamp 0 [expr $i -1]]
          set stamp [string range $stamp  [expr $i +1] [string length $stamp]]
          set i [string first - $stamp] 
          set col [string range $stamp 0 [expr $i -1]]
          set stamp [string range $stamp  [expr $i +1] [string length $stamp]]
          set i [string first - $stamp] 
          set field [string range $stamp 0 [expr $i -1]]
          set f1 g:$root-g.fit
          set f2 r:$root-r.fit
          set f3 i:$root-i.fit
          if {$stretch < 0} {
              # empirically good choice for low SB objects
              set scale 2:300s
          } else {
              set scale $stretch
          }       
          3fits2ppm $run $col $field . $f3 $f2 $f1 $root $scale $s_type \
                       $trimmed_edge_width
        }
    }

}


# a simple proc to take a list of fits files and produce a mosaic
# a more sophisticated way of making mosaics is by proc stamps2mosaic from
# catalog_match.tcl. For example, in local dir
# photo> ls *.fit > stamps.list
# photo> stamps2mosaic stamps.list . r 36
# where each mosaic will have 36 stamps. For more details like e.g.
# showing mosaics on FSAO display, or defining rectangular mosaics see 
# comments above stamps2mosaic
proc fits2mosaics {file_list} {
 
     set stamps [chainNew REGION]
     
     foreach file $file_list {
          set reg [regReadAsFits [regNew] $file]
	  if {[info exists stamp_size]} {
	      assert {$stamp_size == [exprGet $reg.nrow]}
          } else {
              set stamp_size [exprGet $reg.nrow]
          }
          chainElementAddByPos $stamps $reg TAIL AFTER  
          handleDel $reg                 
     }

     set mosaic [ps_mosaic $stamps $stamp_size 0 $file_list 0]      
     regWriteAsFits $mosaic mosaic.fit
     regDel $mosaic

     chainDestroy $stamps regDel

}



# given run, col, field, row and col, for $stamps != 0 cut g-r-i 
# $stampsize pixe lstamps, and for $ppm != 0 produce a ppm color image. 
# if $stamps < 0 remove fits files after producing ppm file
# stretch is log by default. for other options see comments above
# regU16ToU8LUTGet 
# n.b. gri combination works best, especially for low SB objects
# n.b. fits files are 40 pixels wider than requested size so that 
# color images can be trimmed around the edges. 
proc get_bw_stamp {run camCol field row col filter {stampsize 750} {name ""} } {

     # get the reconstructed frame
     set frame [get_field $run $camCol $field $filter]
     set stamp [stamp_from_region $frame $row $col $stampsize offsets]
     if {$name != ""} {
         regWriteAsFits $stamp $name
         regDel $stamp
         regDel $frame
         return
     } else {
         return $stamp
     }
}

#=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

proc fmosaic {args} {
   set display 0
   set hardness 0
   set rescale 0
   set subtractSky 1;
   set ticks 0;				# draw ticks?
   set verbose 0
   
   set opts [list \
		 [list [info level 0] \
		      "Produce a ppm file from external floating point mosaics"] \
		 [list <ppmfile> STRING "" ppmfile \
		      "Name of output PPM file"] \
		 [list <fileFmt> STRING "" fileFmt \
		      "Name of input files, with %s for filtername"] \
		 [list {[filters]} STRING "i r g" filters \
		      "Desired filters (In order R G B)"] \
		 [list -scale DOUBLE 1 scale "How much to scale up data"] \
		 [list -rescale CONSTANT 1 rescale "Rescale individual bands?"]\
		 [list -bin INTEGER 1 binfac \
		      "How much to bin PPM file"] \
		 [list -stretch STRING "m:65000" stretch \
		      "Desired stretch; if ends in \"s\", in units of sigma"] \
		 [list -type STRING "asinh:Q10" type \
		      "Desired type of stretch; lin, sqrt, log, asinh:b, sqrta:b"] \
		 [list -ppm_args STRING "" ppm_args \
		      "Extra arguments for make_ppm_file"] \
		 [list -display CONSTANT 1 display \
		      "Display PPM file"] \
		 [list -fsao INTEGER -1 fsao "Plot regions on this FSAO"] \
		 [list -skysubSize INTEGER 0 skysubSize \
		      "Scale on which sky should be estimated"] \
		 [list -noSubtractSky CONSTANT 0 subtractSky "Don't subtract the sky"] \
		 [list -sigma DOUBLE 0 sigma \
		      "Smooth with an N(0,sigma^2) Gaussian"] \
		 [list -saturLevel INTEGER 0 saturLevel \
		      "Saturation level of images (over soft bias)"] \
		 [list -hardness CONSTANT 1 hardness \
		      "Produce an image of the distribution of hardness ratios in the image"] \
		 [list -x0 DOUBLE -0.5 x0 "Lower range of x-axis"] \
		 [list -x1 DOUBLE 7 x1 "Upper range of x-axis"] \
		 [list -y0 DOUBLE -0.5 y0 "Lower range of y-axis"] \
		 [list -y1 DOUBLE 7 y1 "Upper range of y-axis"] \
		 [list -margin INTEGER 0 ymargin \
		      "Add this many pixels of margin"] \
		 [list -xmargin INTEGER 0 xmargin \
		      "Add this many pixels of margin in x"] \
		 [list -trim STRING "" trim \
		      "Range of pixels to display; \"r0:r1 c0:c1\""] \
		 [list -nrow INTEGER 512 Nrow "row-size of hardness region"] \
		 [list -ncol INTEGER 512 Ncol "col-size of hardness region"] \
		 [list -ticks CONSTANT 1 ticks "Generate ticks"] \
		 [list -lweight INTEGER 1 lweight "Width of axes/ticks"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 [list -flux20 STRING "" \
		      _flux20_nominal_user "Array to override some flux20_nominal values"]\ 
		 ]   

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # These flux20s look nice
   # {3_6,4_5,5_8}: Spitzer IRAC bands
   #
   array set flux20_good [list \
			      u 200 \
			      g 1000 \
			      r 1000 \
			      i 1000 \
			      z 200 \
			      R 1000 \
			      I 1000 \
			      G 1000 \
			      B 1000 \
			      Bw 1000 \
			      j 1000 \
			      h 1000 \
			      k 1000 \
			      f450 1000 \
			      f606 1000 \
			      f814 1000 \
			      f475w 1000 \
			      v555w 1000 \
			      f625w 1000 \
			      f775w 1000 \
			      i814w 1000 \
			      f850lp 1000 \
			      h160w 1000 \
			      3_6  1000 \
			      4_5  1000 \
			      5_8  1000 \
			      XS   1000 \
			      XM   1000 \
			      XH   1000 \
			      S   1000 \
			      M   1000 \
			      H   1000 \
			      ]

   #
   # These flux20s are approximately the measured values
   #
   # N.b. Larger numbers increase the band's intensity in the PPM file
   #
   array set flux20_nominal [list \
			      u 1010 \
			      g 2360 \
			      r 1900 \
			      i 1400 \
			      z 240 \
			      R 1000 \
			      I 1000 \
			      G 1500 \
			      B 1000 \
			      Bw 1000 \
			      j 1000 \
			      h 1000 \
			      k 1000 \
			      f450 1800 \
			      f606 100 \
			      f814 1000 \
			      f475w 1000 \
			      v555w 1521 \
			      f625w 600 \
			      f775w 500 \
			      i814w 1000 \
			      f850lp 300 \
			      h160w 6118 \
			      3_6  500 \
			      4_5  500 \
			      5_8  100 \
			      XS   8000 \
			      XM   1300 \
			      XH   2000 \
			      S   8000 \
			      M   1300 \
			      H   2000 \
			      ]
   if {$_flux20_nominal_user != ""} {
      upvar $_flux20_nominal_user flux20_nominal_user
      if {[llength $_flux20_nominal_user] == 1} { # an array name
	 array set flux20_nominal [array get flux20_nominal_user]
      } else {
	 array set flux20_nominal $_flux20_nominal_user
      }
   }

   array set sky [list \
		      u    10  g    5  r 5    i 5  z 5 \
		      f450 -1  f606 0  f814 0 \
		      f475w -10 f625w 0 f775w 0 \
		      ]
   #
   # Factors to convert to pretty values
   #
   foreach f $filters {
      if {$rescale && [info exists flux20_good($f)]} {
	 set flux20s($f) [expr 1.0*$flux20_good($f)/$flux20_nominal($f)]
      } else {
	 set flux20s($f) 1.0
      }
   }
   #
   # Read all of the data
   #
   set have_wcs 1;			# assume that all filter have WCS info
   
   foreach f $filters {
      set file [format $fileFmt $f]

      if [catch {set reg [regReadAsFits [regNew] $file]} msg] {
	 foreach reg $regions {
	    regDel $reg
	 }
	 error "Reading $file: $msg"
      }

      if [catch { hdrGetAsDbl $reg.hdr CRPIX1 }] {# no WCS
	 set have_wcs 0
      }

      set rawregions($f) $reg
      if 0 {
	 mtv -s [incr fsao] $reg
	 puts -nonewline "Raw $f "; if {[gets stdin] == "q"} { error "$f $reg" }
      }
   }
   #
   # Shift to coordinate system of first band
   #
   foreach f $filters {
      set reg $rawregions($f)

      if 0 {
	 set stats [regStatsFind $reg]
	 set low [keylget stats low]

	 regIntConstAdd $reg [expr -$low]
      }

      regMultiplyWithDbl $reg $scale

      if 0 {
	 mtv -s [incr fsao] $reg
	 puts -nonewline "Scaled raw $f "; if {[gets stdin] == "q"} { error "$f $reg" }
      }
      if 0 {				# XXX
	 set sreg [subRegNew $reg 1023 1380 0 0]
	 set ireg [regIntConvert $sreg -type U16]
	 regDel $sreg
      } else {
	 set ireg [regIntConvert $reg -type U16]
      }
      hdrCopy $reg.hdr $ireg.hdr
      regDel $reg; set reg $ireg

      if ![info exists nrow] {
	 set nrow [exprGet $ireg.nrow];#get size of regions 
	 set ncol [exprGet $ireg.ncol]
      }
      #
      # Shift to common coordinate system
      #
      if !$have_wcs {
	 set drow 0; set dcol 0
      } else {
	 foreach v [list CRPIX1 CRPIX2 CRVAL1 CRVAL2 \
			CD1_1 CD1_2 CD2_1 CD2_2 \
			CDELT1 CDELT2 CROTA2 \
		       ] {
	    catch {
	       set $v [hdrGetAsDbl $reg.hdr $v]
	       #echo RHL $v [set $v]
	    }
	 }

	 if {![info exists CD1_1] && ![info exists CDELT1]} {
	    echo "I can find neither CD1_1 nor CDELT1"
	    set drow 0; set dcol 0
	 } else {
	    if ![info exists CD1_1] {
	       set CD1_1 $CDELT1
	       set CD1_2 0
	       set CD2_1 0
	       set CD2_2 $CDELT2
	    }
	    
	    if {$f == [lindex $filters 0]} {
	       set row0 $CRPIX2; set col0 $CRPIX1
	       set ra0  $CRVAL1; set dec0 $CRVAL2
	       
	       set drow 0; set dcol 0
	    } else {
	       set dra  [expr ($CRVAL1 - $ra0)*cos(3.14159/180*$dec0)]
	       set ddec [expr ($CRVAL2 - $dec0)]

	       set row $CRPIX2; set col $CRPIX1
	       
	       set det [expr $CD1_1*$CD2_2 - $CD1_2*$CD2_1]
	       
	       set drow [expr $row - $row0 + (-$dra*$CD2_1 + $ddec*$CD1_1)/$det]
	       set dcol [expr $col - $col0 + ($dra*$CD2_2 - $ddec*$CD1_2)/$det]

	       if 1 {
		  echo Flipping sign of drow/col
		  set drow [expr -$drow]; set dcol [expr -$dcol]
	       }
	    }
	 }
      }

      if {$drow != 0 || $dcol != 0} {
	 set oreg [regNew $nrow $ncol]
	 regIntShift $reg [expr -$drow] [expr -$dcol] -filtsize 11 -out $oreg
	 regDel $reg; set reg $oreg; unset oreg
      }
      
      handleSet $reg.row0 [expr -$drow]
      handleSet $reg.col0 [expr -$dcol]
      #
      # Scale to a "Pretty" set of relative weights
      #
      set rat [expr 1.0/$flux20s($f)]
      if {$rat != 1} {
	 regIntLincom $reg $reg 0 $rat 0
      }
      #
      # Smooth a little?
      #
      if {!$hardness && $sigma > 0} {
	 convolveWithGaussian $reg $reg "" 11 $sigma
      }
      #
      # Subtract (most of) sky
      #
      if !$subtractSky {
	 if ![info exists sky($f)] {
	    set sky($f) 0
	 }
	 set bkgd $sky($f)

	 if {[exprGet -enum $reg.type] == "TYPE_U16"} {
	    set bkgd [expr $bkgd - [softBiasGet]/$flux20s($f)]
	 }
	 regIntConstAdd $reg $bkgd
      } else {
	 if {$skysubSize <= 0} {
	    regStatsFromQuartiles -clip 2 -med med -min min $reg -coarse 1
	    #set med [expr ($min + $med)/2];# XXX
	    #set med [expr $min + 10];			# XXX
	    #set med [regPixGet $reg 0 0]
	    
	    if ![info exists sky($f)] {
	       set sky($f) 0
	    }
	    set bkgd [expr int($sky($f) + 0.5 - $med)]
	    if !$hardness {
	       incr bkgd [softBiasGet]
	    }
	    #echo $bkgd
	    regIntConstAdd $reg $bkgd
	 } else {
	    global rand
	    
	    if ![info exists rand] {
	       set seed 12345
	       set rand [phRandomNew 100000 -seed $seed -type 2]
	    }
	    
	    set nbit 1
	    set skyreg [medianSky $reg $skysubSize $skysubSize -shift $nbit]
	    regIntConstAdd *$skyreg.reg [expr -[softBiasGet]*(1<<$nbit)];
	    
	    skySubtract $reg $reg $skyreg $rand
	    binregionDel $skyreg
	 }
      }
      #
      # Trim regions?
      #
      if [regexp {([0-9]+)[-:]([0-9]+) +([0-9]+)[-:]([0-9]+)$} $trim {} \
	      r0 r1 c0 c1] {
	 if [catch {
	    set sreg [subRegNew $reg [expr $r1-$r0+1] [expr $c1-$c0+1] $r0 $c0]
	 } msg] {
	    echo Failed to trim to $r0-$r1 $c0-$c1 ([exprGet $reg.nrow]x[exprGet $reg.ncol]): $msg
	 } else {
	    set tmp [regIntCopy "" $sreg]
	    regDel $sreg; regDel $reg
	    set reg $tmp
	 }
      }
      #
      # Find saturated pixels?
      #
      if {$saturLevel > 0} {
	 global MASK_TYPE
	 set sm \
	     [spanmaskNew -nrow [exprGet $reg.nrow] -ncol [exprGet $reg.ncol]]

	 set levels [vectorExprEval [expr [softBiasGet] + $saturLevel]]
	 set objs [regFinder $reg $levels -npixel_min 1]
	 vectorExprDel $levels; unset levels

	 spanmaskSetFromObjectChain $objs $sm $MASK_TYPE(SATUR) 0
	 objectChainDel $objs

	 handleSetFromHandle $reg.mask (MASK*)&$sm
	 handleDel $sm; unset sm
      }

      if {$fsao >= 0} {
	 mtv -s [expr [lsearch $filters $f] + 1] $reg
	 puts -nonewline "$f "; if {[gets stdin] == "q"} { error "$f $reg" }
      }

      lappend regions $reg      
   }
   #
   # Done with prepare regions; generate the PPM file
   #
   set psize 24;			# pointsize of labels

   if $hardness {
      set labels [scramble regions $filters sky -sigma $sigma \
		      -x0 $x0 -x1 $x1 -y0 $y0 -y1 $y1 \
		      -ticks -margin $ymargin -xmargin $xmargin \
		      -xlab "[lindex $filters 1]/[lindex $filters 2]" \
		      -ylab "[lindex $filters 0]/[lindex $filters 1]" \
		      -nrow $Nrow -ncol $Ncol -lweight $lweight -psize $psize]
   } else {
      if {$saturLevel >= 0} {
	 lappend ppm_args "-fix_with_U16"
      }
   }

   if ![regexp {\.ppm$} $ppmfile] {
      append ppmfile ".ppm"
   }
   if $hardness {
      regsub {.ppm$} $ppmfile {-H.ppm} ppmfile
   }

   if 0 {
      loop i 0 3 {
	 mtv -s [expr $i + 1] [lindex $regions $i]; puts -nonewline "[lindex $filters $i] ";
	 if {[gets stdin] == "q"} {
	    error [lindex $regions $i]
	 }
      }
   }
   
   set result [eval make_ppm_file $ppmfile 0 0 \
		   [lindex $filters 0]:[lindex $regions 0] \
		   [lindex $filters 1]:[lindex $regions 1] \
		   [lindex $filters 2]:[lindex $regions 2] \
		   -stretch $stretch -type $type $ppm_args]
   #
   # Axis labels
   #
   if [info exists labels] {
      set font "helvetica"
      if {$lweight > 1} {
	 append font "-bold"
      }
      exec mogrify -fill white -font ps:$font -pointsize $psize \
	  -draw "$labels" $ppmfile
   }
   #
   # Cleanup
   #
   foreach reg $regions {
      regDel $reg
   }

   if $display {
      display_ppm_file -geom +1+1 $result
   }

   return $result
}

proc scramble {args} {
   set ticks 0;				# draw ticks?
   set verbose 0
   
   set opts [list \
		 [list [info level 0] \
 "Take a set of e.g. gri images and make an image in e.g. g-r v. r-i space"] \
		 [list <regions> STRING "" _regions "Input images"] \
		 [list <filters> STRING "" filters "Filter names"] \
		 [list <sky> STRING "" _sky "Array of sky values"] \
		 [list -sigma DOUBLE 0 sigma \
		      "Smooth with an N(0,sigma^2) Gaussian"] \
		 [list -x0 DOUBLE -0.5 x0 "Lower range of x-axis"] \
		 [list -x1 DOUBLE 2 x1 "Upper range of x-axis"] \
		 [list -y0 DOUBLE -0.5 y0 "Lower range of y-axis"] \
		 [list -y1 DOUBLE 2 y1 "Upper range of y-axis"] \
		 [list -psize INTEGER 24 psize "Point size for labels"] \
		 [list -margin INTEGER 0 ymargin \
		      "Add this many pixels of margin"] \
		 [list -xmargin INTEGER 0 xmargin \
		      "Add this many pixels of margin in x"] \
		 [list -nrow INTEGER 512 Nrow "row-size of region"] \
		 [list -ncol INTEGER 512 Ncol "col-size of region"] \
		 [list -ticks CONSTANT 1 ticks "Generate ticks"] \
		 [list -xlabel STRING "" xlab "X-axis label"] \
		 [list -ylabel STRING "" ylab "Y-axis label"] \
		 [list -lweight INTEGER 1 lweight "Width of axes/ticks"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 ]   

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_regions regions
   set nrow [exprGet [lindex $regions 0].nrow];#get size of regions 
   set ncol [exprGet [lindex $regions 0].ncol]
   #
   # Set margins
   #
   if {$ticks && $ymargin == 0} {
      set ymargin 75
   }
   if {$xmargin == 0} {
      set xmargin [expr int(1.25*$ymargin)]
   }
   
   #
   # Create output regions if necessary
   #
   set xmargin_l $xmargin; set xmargin_r [expr int(0.3*$xmargin)]
   set ymargin_l $ymargin; set ymargin_r [expr int(0.3*$ymargin)]
   
   set i 0
   foreach f $filters {		# create output regions
      set regs($i) [lindex $regions $i]
      
      set hregs($i) \
	  [regNew -type FL32 [expr $Nrow + $ymargin_l + $ymargin_r] \
	       [expr $Ncol + $xmargin_l + $xmargin_r]]
      regClear $hregs($i)
      
      incr i
   }
   set nrow_full [exprGet $hregs(0).nrow]
   set ncol_full [exprGet $hregs(0).ncol]
   
   set rscale [expr 1.0*$Nrow/($y1 - $y0)]
   set cscale [expr 1.0*$Ncol/($x1 - $x0)]
   set minval 100;			# XXX range of pixel values to include
   loop r 0 $nrow {
      if $verbose {
	 puts -nonewline "$r\r"; flush stdout
      }
      loop c 0 $ncol {
	 set v0 [regPixGet $regs(0) $r $c]
	 set v1 [regPixGet $regs(1) $r $c]
	 set v2 [regPixGet $regs(2) $r $c]
	 
	 if {$v0 == 0 || $v1 == 0 || $v2 == 0} {
	    continue
	 }
	 if {$v0 < $minval || $v1 < $minval || $v2 < $minval} {
	    continue
	 }

	 set hr [expr int((1.0*$v2/$v1 - $y0)*$rscale + 0.5)]
	 set hc [expr int((1.0*$v1/$v0 - $x0)*$cscale + 0.5)]
	 
	 if {0} {
	    puts -nonewline "$v0 $v1 $v2 $hr $hc [expr 1.0*$v0/$v1] [expr 1.0*$v1/$v2]\n"; flush stdout
	 }
	 
	 #set hr $r; set hc $c;	# RHL XXX
	 
	 if {$hr >= 0 && $hr < $Nrow && $hc >= 0 && $hc < $Ncol} {
	    incr hr $ymargin_l; incr hc $xmargin_l
	    
	    regPixSetWithDbl $hregs(0) $hr $hc [expr $v0 + [regPixGet $hregs(0) $hr $hc]]
	    regPixSetWithDbl $hregs(1) $hr $hc [expr $v1 + [regPixGet $hregs(1) $hr $hc]]
	    regPixSetWithDbl $hregs(2) $hr $hc [expr $v2 + [regPixGet $hregs(2) $hr $hc]]
	 }
      }
   }
   
   if $verbose {
      echo ""
   }
   
   set max 0
   loop i 0 3 {
      set vals [regStatsFind $hregs($i)]
      if {[keylget vals high] > $max} {
	 set max [keylget vals high]
      }
   }
   set scale [expr 60000.0/$max]
   
   unset regions
   loop i 0 3 {
      regMultiplyWithDbl $hregs($i) $scale
      set reg [regIntConvert $hregs($i) -type U16]
      regDel $regs($i);
      regDel $hregs($i)
      
      lappend regions $reg; set hregs($i) $reg
      
      regStatsFromQuartiles -clip 2 -med med $reg -coarse 1
      
      set f [lindex $filters $i]
      if ![info exists sky($f)] {
	 set sky($f) 0
      }
      set bkgd [expr int($sky($f) + [softBiasGet] + 0.5 - $med)]
      regIntConstAdd $reg $bkgd
      if 0 {
	 mtv $reg; puts -nonewline "$f "; gets stdin
      }
      
      if {$sigma > 0} {
	 convolveWithGaussian $reg $reg "" 11 $sigma
      }
   }
   #
   # Tick marks?
   #
   if !$ticks {
      return ""
   }

   set ticksize0 7
   set tickval 65535
   #
   # First draw the box
   #
   set hlw [expr int($lweight/2)]
   loop i 0 3 {
      loop lw -$hlw [expr $hlw + 1] {
	 loop r 0 [expr $Nrow + 2*$hlw] {# y axis/axes
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin - $hlw + $r] [expr $xmargin + $lw] $tickval
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin - $hlw + $r] [expr $xmargin + $lw + $Nrow - 1]\
		$tickval
	 }
	 loop c 0 [expr $Ncol + 2*$hlw] {# x-axis/es
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin + $lw] [expr $xmargin - $hlw + $c] $tickval
	    regPixSetWithDbl $hregs($i) \
		[expr $ymargin + $lw + $Nrow - 1] [expr $xmargin - $hlw + $c]\
		$tickval
	 }
      }
   }
   #
   # Then the ticks
   #
   set xtick ""; set ytick ""
   
   set x [expr int($x0) - 1]; set dx 0.5
   while {$x <= int($x1) + 1} {
      lappend xtick $x
      set x [expr $x + $dx]
   }
   
   set y [expr int($y0) - 1]
   while {$y <= int($y1) + 1} {
      lappend ytick $y
      set y [expr $y + 0.5]
   }
   
   foreach x $xtick {
      set ix [expr int(($x - $x0)*$cscale + 0.5)]
      
      if {$ix >= 0 && $ix < $Ncol} {
	 if {$x == int($x)} {
	    set ticksize [expr 2*$ticksize0]
	 } else {
	    set ticksize $ticksize0
	 }
	 
	 incr ix $xmargin
	 
	 set dx 0.7
	 if {$x < 0} {
	    set dx [expr $dx + 0.7]
	 }
	 if {$ticksize > $ticksize0} {
	    append labels \
		[format "text %d,%d \"$x\" " \
		     [expr int($ix - $dx*$psize)] \
		     [expr int($nrow_full - $ymargin - 1 + 1.5*$psize)]]
	 }
	 
	 loop i 0 3 {
	    loop lw -$hlw [expr $hlw + 1] {
	       loop r 0 $ticksize {
		  regPixSetWithDbl $hregs($i) \
		      [expr $ymargin + $r] [expr $ix + $lw] $tickval
		  regPixSetWithDbl $hregs($i) \
		      [expr $ymargin + $Nrow - $r - 1] [expr $ix + $lw] \
		      $tickval
	       }
	    }
	 }
      }
   }
   
   foreach y $ytick {
      set iy [expr int(($y - $y0)*$rscale + 0.5)]
      
      if {$iy >= 0 && $iy < $Nrow} {
	 if {$y == int($y)} {
	    set ticksize [expr 2*$ticksize0]
	 } else {
	    set ticksize $ticksize0
	 }
	 
	 incr iy $ymargin
	 
	 set dx 2.0
	 if {$y < 0} {
	    set dx [expr $dx + 0.7]
	 }
	 if {$ticksize > $ticksize0} {
	    append labels [format "text %d,%d \"$y\" " \
			       [expr int($xmargin - $dx*$psize)] \
			       [expr int($nrow_full - $iy - 1 + 0.45*$psize)]]
	 }
	 
	 loop i 0 3 {
	    loop lw -$hlw [expr $hlw + 1] {
	       loop c 0 $ticksize {
		  regPixSetWithDbl \
		      $hregs($i) [expr $iy+$lw] [expr $xmargin + $c] $tickval
		  regPixSetWithDbl $hregs($i) \
		      [expr $iy+$lw] [expr $xmargin + $Ncol - $c - 1] \
		      $tickval
	       }
	    }
	 }
      }
   }
   #
   # Axis labels
   #
   append labels \
       [format "text %d,%d \"$xlab\" " \
	    [expr $xmargin + int(0.5*($x1 - $x0)*$cscale + 0.5)] \
	    [expr int($nrow_full - 0.6*$psize)]]
   append labels \
       [format "text %d,%d \"$ylab\" " \
	    [expr int(0.4*$psize)] \
	    [expr int($nrow_full - $ymargin - (0.5*($y1 - $y0)*$rscale +0.5))]]

   return $labels
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

proc make_grayscale {args} {
   set opts [list \
		 [list [info level 0] "Make a .ppm file from a region"] \
		 [list <file> STRING "" file "Output file"] \
		 [list <reg> STRING "" reg "Input region"] \
		 [list -stretch STRING "m:100" stretch \
		      "Desired stretch; if ends in \"s\", in units of sigma"] \
		 [list -type STRING "asinh:Q10" type \
		      "Desired type of stretch; lin, sqrt, log, asinh:b, sqrta:b"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set reg [regIntCopy NULL $reg]

   regStatsFromQuartiles -clip 2 -med med -min min $reg -coarse 1
   regIntConstAdd $reg [expr [softBiasGet] - $med + 200]
   make_ppm_file $file 0 0 r:$reg g:$reg i:$reg -stretch $stretch -type $type

   regDel $reg

   return $file
}
