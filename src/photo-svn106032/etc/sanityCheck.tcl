#############################################################################
# This file contains procedures to check that the photometric and postage 
# stamp pipelines produce reasonable looking output. 
#
set help_procs ""

#
# choose PGPlot colors for each filter
#
set color_index(u) 4 
set color_index(g) 3
set color_index(r) 7 
set color_index(i) 8 
set color_index(z) 2     

#
# Tell pgplot where to send the file if the device demands hardcopy
#
# Note that due to a bug in pgstateSet (PR# 303) this string must be used
# via the PGPLOT_DEV environment variable rather than a pgstateSet -device
# command.
#
proc devicename {device file} {
   if {[regexp {^/C?PS$} $device] || \
       $device == "/VCPS" || $device == "/VPS"} {
      set device \"${file}\"$device
   }

   if 1 {
      return $device
   } else {
      # the following is a hack until the name truncation bug
      # gets fixed in ASTROTOOLS
      global env
      set env(PGPLOT_DEV) $device
      return " "
   }
}

#
# end a plot
proc endplot {pg} {
   pgstateClose $pg
   pgstateDel $pg
}

###########################################################################

lappend help_procs plot_fpstats

proc plot_fpstats {args} {
   set opts [list \
		 [list [info level 0] \
		      "Plot up the statistics stored in the fpFieldStat file"]\
		 [list <device> STRING "" device \
		      "Output device; /PS (for hardcopy) or /XWINDOW"] \
		 [list <startField> INTEGER 0 startfield \
		      "Initial field to plot"] \
		 [list <endField> INTEGER 0 endfield "last field to plot"] \
		 [list <dir> STRING "" outputDir \
 "Directory containing fpFieldStat file, and destination for ps plots"] \
		 [list <run> INTEGER 0 run "The run in question"] \
		 [list <camCol> INTEGER 0 col "The camCol in question"] \
		 [list {[filters]} STRING "all" filters \
 "Filters to plot; may be a list of filters, or \"all\", or \"summary\""] \
		 [list -rerun STRING "" rerun "Rerun number"] \
		 ]

      if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   global plan
   if [info exists plan] {
      keylget plan fieldList fieldList; # try to get fieldList
   }
   #
   # Read header
   #
   if [info exists fieldList] {
      set field0 [lindex $fieldList $startfield]
   } else {
      set field0 $startfield
   }
   set hdr [read_fieldstat -hdr $outputDir $run $col $field0]

   set field [hdrGetAsInt $hdr FIELD]; assert { $field == $field0 }
   set ref_color [string trim [hdrGetAsAscii $hdr REFCOLOR]]
   set filterlist [string trim [hdrGetAsAscii $hdr FILTERS]]
   if {$filters == "all"} {
      set filters $filterlist
      set all 1
   } else {
      set all 0
   }
   hdrFreeAll $hdr; hdrDel $hdr
   #
   # are we making hardcopies?
   #
   if [regexp {^/C?PS$} $device] {
      set device "/CPS"
      set file "yes";           # it'll be replaced by filename below
   } else {
      set file ""
   }
   #
   # A temp VECTOR
   #
   set vtmp [vectorExprNew 0]
   #
   # Read the FIELDSTATs files
   #   
   if ![info exists fieldstats] {
      set fieldstats [chainNew _FIELDSTAT]
   }

   loop field $startfield [expr $endfield+1] {
      if [info exists fieldList] {
	 set field [lindex $fieldList $field]
      }
      if [catch {
	 set fstat [read_fieldstat $outputDir $run $col $field]
      } msg] {
	 incr field -1
	 echo $msg
	 break
      }
      chainElementAddByPos $fieldstats $fstat; handleDel $fstat
   }
   set endfield [expr $field - 1]
    
   # Get frame id's
   set frames [vectorExprEval ($startfield,$endfield)]
   vNameSet $frames ""

   set date [getdate]

   # For each color:
   foreach filter $filters {
      if {[set i [lsearch $filterlist $filter]] < 0} {
	 if {$filter != "summary"} {
	    echo Filter $filter is not in $filterlist
	 }
	 continue;
      }

      # Set up the plot
      set pg [pgstateNew]
      set deviceA [devicename $device \
		       [format $outputDir/fpPlots-%06d-$filter$col.ps $run]] 
      pgstateSet $pg -device $deviceA \
      -xfract 0.22 -yfract 0.35 -nxwindow 4 -nywindow 3 -symb 7

      pgstateOpen $pg

      topLabel 20 "Frames, frames: $startfield-$endfield" \
	  $run $col $date $filter $rerun

      # Plot nobjects 
      set nbright [vFromChain $fieldstats nbrightobj<$i>]
      vNameSet $nbright ""
      set nfaint  [vFromChain $fieldstats nfaintobj<$i>]
      vNameSet $nfaint ""
      set nobj [vectorExprEval $nbright+$nfaint]
      vNameSet $nobj ""
      set nCR [vFromChain $fieldstats ncr<$i>]
      vNameSet $nCR ""
      
      set ymin 0; set ymax [vExtreme $nobj max]
      if {$ymax < [vExtreme $nCR max]} {
	 set ymax [vExtreme $nCR max]
      }

      vPlot $pg $frames $nobj \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      titlePlot "Nobj (bright/faint/total)" 40
      xlabel Field
      vectorExprDel $nobj
      
      pgstateSet $pg -isNewplot 0 -symb 4
      
      vPlot $pg $frames $nbright \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      vectorExprDel $nbright

      pgstateSet $pg -isNewplot 0 -symb 5
      
      vPlot $pg $frames $nfaint \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      vectorExprDel $nfaint

      pgstateSet $pg -isNewplot 1

      set ymax 500
      set ymax_CR [vExtreme $nCR max];
      if {$ymax_CR > $ymax} {
	 set ymax $ymax_CR
      }
      vPlot $pg $frames $nCR \
	  -ymin 0 -ymax $ymax -xmin $startfield -xmax $endfield
      titlePlot "N Cosmic Rays" 40
      xlabel Field
      vectorExprDel $nCR

      # Plot sky (ADD SIGMA AS ERRORBARS)
      set skies "sky_frames sky_psp sky_frames_sub"
      foreach s $skies {
     set $s [vFromChain $fieldstats $s<$i>]
      }

      vectorExprSet $vtmp \
      "($sky_frames concat $sky_psp) concat $sky_frames_sub"

      vNameSet $sky_frames ""

      pgstateSet $pg -icMark 2;		# red crosses
      vPlot $pg $frames $sky_frames -ymin [vExtreme $vtmp min] \
      -ymax [vExtreme $vtmp max] -xmin $startfield -xmax $endfield
      
      titlePlot "Median sky, ADU" 40
      xlabel Field

      pgstateSet $pg -isNewplot 0 -isLine 1
      vPlot $pg $frames $sky_psp;   # connected line
      pgstateSet $pg -isNewplot 0 -isLine 0 -symb 6 -icMark 4
      vPlot $pg $frames $sky_frames_sub;# blue squares

      foreach s $skies {
     vectorExprDel [set $s]
      }
      pgstateSet $pg -isNewplot 1 -isLine 0 -icLine 1 -icMark 1

      # Plot saturation level
      set slevel [vFromChain $fieldstats saturation_level<$i>]
      vNameSet $slevel ""
      vPlot $pg $frames $slevel \
      -ymin 0 -ymax 65535 -xmin $startfield -xmax $endfield
      titlePlot "Adopted Saturation Level" 40
      xlabel Field
      vectorExprDel $slevel

      # Plot n_eff for psf
      set neff_psf [vFromChain $fieldstats neff_psf<$i>]
      vectorExprSet $neff_psf 0.400^2*$neff_psf
      vNameSet $neff_psf "n_eff/FWHM (arcsec)"

      pgstateSet $pg -isLine 0
      vPlot $pg $frames $neff_psf -ymin 0 -ymax [vExtreme $neff_psf max] \
      -xmin $startfield -xmax $endfield

      vectorExprSet $neff_psf \
      2*sqrt(2*ln(2))*sqrt($neff_psf/(4*3.14159));# effective FWFM/arcsec
      pgstateSet $pg -isLine 0 -symb 4 -isNew 0
      vPlot $pg $frames $neff_psf
      pgstateSet $pg -isLine 1 -isNew 0
      vPlot $pg $frames $neff_psf
      
      titlePlot "n_eff/\"FWHM\" for PSF" 40
      xlabel Field

      vectorExprDel $neff_psf
      pgstateSet $pg -isNew 1 -isLine 0

      # Plot median fiber and psf color
      foreach t "fiber psf" {
     upvar 0 median_${t}color median_color
     set median_color [vFromChain $fieldstats median_${t}color<$i>]
     vNameSet $median_color ""
      }

      vectorExprSet $vtmp "($median_psfcolor concat $median_fibercolor)"
      vectorExprSet $vtmp "$vtmp if($vtmp < 1.0e10)"
      set ymin [expr [vExtreme $vtmp min] - 0.1]
      set ymax [expr [vExtreme $vtmp max] + 0.1]
      
      foreach t "fiber psf" {
     upvar 0 median_${t}color median_color

     # Play games with +- 0.1 to stop TCL converting the sum to an int,
     # and then complaining (PR 6122). Grrr.
     if {[vectorExprGet \
	      "sum($median_color > 1.0e9 ? 1.0e9 : $median_color) +0.1"] - 0.1\
	     == 0.0} {
        vectorExprSet $median_color (1000,1001)
        vPlot $pg $frames $median_color \
        -ymin 0 -ymax 1 -xmin $startfield -xmax $endfield
        pgPtext [expr 0.5*($startfield+$endfield)] 0.5 0 0.5 \
        "(no redder band)"
     } else {
        catch {
           vPlot $pg $frames $median_color -ymin $ymin -ymax $ymax \
           -xmin $startfield -xmax $endfield
        }
     }
     titlePlot "Median $t Colour" 40
     xlabel Field
     vectorExprDel $median_color
      }

      # Model magnitude aperture correction
      
      if [catch {
     foreach t "dev exp" {
        set ${t}_ap_corr \
        [vFromChain $fieldstats ${t}_ap_correction<$i>]
        vNameSet [set ${t}_ap_corr] ""
     }

     vectorExprSet $vtmp "$dev_ap_corr concat $exp_ap_corr"
     vectorExprSet $vtmp "($vtmp - 0.05) concat ($vtmp + 0.05)"
     
     pgstateSet $pg -icMark 2
     vPlot $pg $frames $dev_ap_corr  -ymin [vExtreme $vtmp min] \
         -ymax [vExtreme $vtmp max] -xmin $startfield -xmax $endfield
     pgstateSet $pg -isNewplot 0 -icMark 4
     vPlot $pg $frames $exp_ap_corr  -ymin [vExtreme $vtmp min] \
         -ymax [vExtreme $vtmp max] -xmin $startfield -xmax $endfield
     titlePlot "Star's galaxy ap. corr" 40
     xlabel Field
     
     pgstateSet $pg -isNewplot 1 -icMark 1
     vectorExprDel $dev_ap_corr
     vectorExprDel $exp_ap_corr
      }] {
     ;
      }

      # Plot Q/U, and axis ratio
      set Q [vFromChain $fieldstats q<$i>]; vNameSet $Q ""
      set U [vFromChain $fieldstats u<$i>]; vNameSet $U ""

      vectorExprSet $vtmp "$frames if($U < 1.0e10)"
      set ymax 0.1; set ymin -$ymax
      foreach v "U Q" {
	 vectorExprSet [set $v] "[set $v] if([set $v] < 1.0e10)"
	 if {[vExtreme [set $v] min] < $ymin} {
	    set ymin [vExtreme [set $v] min]
	 }
	 if {[vExtreme [set $v] max] > $ymax} {
	    set ymax [vExtreme [set $v] max]
	 }
      }

      if {[expr abs($ymin)] > $ymax} {
     set ymax [expr abs($ymin)]
      } elseif {-$ymax < $ymin} {
     set ymin [expr -$ymax]
      }
      pgstateSet $pg
      vPlot $pg $vtmp $Q \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      pgstateSet $pg -isNewplot 0 -symb 4
      vPlot $pg $vtmp $U \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      titlePlot "Objects' Median Q/U" 40
      xlabel Field

      pgstateSet $pg -isNewplot 1 -isLine 1 -symb 0
      vPlot $pg $U $Q \
      -ymin $ymin -ymax $ymax -xmin $ymin -xmax $ymax
      if {[vectorExprGet dimen($U)] > 0} {
     pgPoint [vectorExprGet $U<0>] [vectorExprGet $Q<0>] -6;# dot at first field
      }

      pgSfs 2;              # don't fill circle
      pgSls 4;              # dotted lines
      loop r 5 100 5 {
     set r [expr (1 - 0.01*$r)/(1 + 0.01*$r)]
         pgCirc 0 0 $r;         # every 0.05 in axis ratio
      }
      pgSls 1;              # solid line
      titlePlot "Objects' Median U/Q" 40
      xlabel U; ylabel Q
      
      vectorExprDel $Q; vectorExprDel $U

      # Plot adopted values of cr_cond32
      set cr_cond32 [vFromChain $fieldstats adopted_cond3_fac2<$i>]

      set ymax [vExtreme $cr_cond32 max]
      if {$ymax < 0.75} {
	 set ymax 0.75
      }
      pgstateSet $pg -isNewplot 1 -symb 4 -isLine 0
      vPlot $pg $frames $cr_cond32 -ymin 0 \
	  -ymax $ymax -xmin $startfield -xmax $endfield
      titlePlot "CR cond3_2" 40
      xlabel Field

      vectorExprDel $cr_cond32
      #
      # Done with that colour
      #
      pgClose; pgstateDel $pg
   }
   #
   # Now plot the stats for the field as a whole
   #
   if {$all || $filters == "summary"} {
      set pg [pgstateNew]
      set deviceA \
	  [devicename $device [format $outputDir/fpPlots-%06d-$col.ps $run]]
      pgstateSet $pg -device $deviceA -nxwindow 2 -nywindow 2 -symb 7
      pgstateOpen $pg
      
      titlePlot "FramesPipeline: run=$run    ccdcol=$col              $date" 20
      
      # Plot return code from each field
      set status [vFromChain $fieldstats status]
      vNameSet $status "status"
      vPlot $pg $frames $status \
      -ymin -1 -ymax 3 -xmin $startfield -xmax $endfield
      titlePlot "Return code for field" 40
      vectorExprDel $status

      # Plot nobjects 
      set nobj [vFromChain $fieldstats nobjects]
      vNameSet $nobj Nobjects
      set nchild [vFromChain $fieldstats nchild]
      
      set ymin 0
      set ymax [vExtreme $nobj max]
      if {[vExtreme $nchild max] > $ymax} {
	 set ymax [vExtreme $nchild max]
      }
      
      vPlot $pg $frames $nobj \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      titlePlot "Objects detected (line: nchildren)" 40
      vectorExprDel $nobj
      
      pgstateSet $pg -isNewplot 0 -isLine 1
      vPlot $pg $frames $nchild \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      vectorExprDel $nchild
      pgstateSet $pg -isNewplot 1 -isLine 0
      
      # Plot nstars
      set nstars [vFromChain $fieldstats nstars]
      vNameSet $nstars Nstars
      vPlot $pg $frames $nstars \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      titlePlot "Total number of stars" 40
      vectorExprDel $nstars
      
      # Plot ngals objects
      set ngals [vFromChain $fieldstats ngals]
      vNameSet $ngals Ngals
      vPlot $pg $frames $ngals \
      -ymin $ymin -ymax $ymax -xmin $startfield -xmax $endfield
      titlePlot "Total number of galaxies" 40
      vectorExprDel $ngals

      endplot $pg
   }

   # cleanup

   vectorExprDel $vtmp

   chainDestroy $fieldstats fieldstatDel
   vectorExprDel $frames

   return ""
}

###########################################################################

lappend help_procs plot_pspstats2

proc plot_pspstats2 {args} {
      set opts [list \
		 [list [info level 0] \
 "Plots up the information stored in the psCT and psCB files, which must
 have been read independently"] \
		 [list <device> STRING "" device \
		      "Output device; /PS (for hardcopy) or /XWINDOW"] \
		 [list <startFrame> INTEGER 0 startframe \
		      "Initial frame to plot"] \
		 [list <endFrame> INTEGER 0 endframe "last frame to plot"] \
		 [list <outputDir> STRING "" outputDir \
		      "Output directory for plots"] \
		 [list <run> INTEGER 0 run "The run in question"] \
		 [list <camCol> INTEGER 0 col "The camCol in question"] \
		 [list <filters> STRING "" filterlist "Filters to plot"] \
		 [list {[frameinfo]} STRING "" _frameinfolist \
		      "Array of FRAME_INFO"] \
		 [list {[flux20_m]} STRING "" _flux20_m \
		      "Array of flux20_m"] \
		 [list {[flux20_c]} STRING "" _flux20_c \
		      "Array of flux20_c"] \
		 [list {[flux20_chi2]} STRING "" _flux20_chi2 \
		      "Array of flux20_chi2"] \
		 [list -rerun STRING "" rerun "Rerun number"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if { $_frameinfolist != ""} {
      upvar $_frameinfolist frameinfolist
   }
   if { $_flux20_m != ""} {
      upvar $_flux20_m flux20_m
   }
   if { $_flux20_c != ""} {
      upvar $_flux20_c flux20_c
   }
   if { $_flux20_chi2 != ""} {
      upvar $_flux20_chi2 flux20_chi2
   }

   set runstr  [format "%06d" $run]
   set vtmp [vectorExprNew 0]
   
   # Read in data
   set CBfile $outputDir/psCB-$runstr-$col.fit
   set CTfile $outputDir/psCT-$runstr-$col.fit

   set hdr [hdrReadAsFits [hdrNew] $CBfile]
   set bias_scale [hdrGetAsInt $hdr DSCALE]
   set filters [string trim [hdrGetAsAscii $hdr FILTERS]]
   hdrDel $hdr

   if {$filterlist == "all"} {
      set filterlist $filters 
   }
   set nfilters [llength $filterlist]

   set ctimes [read_ctimes $CTfile $filterlist]
   set cframes [read_cframes $CBfile $filterlist $startframe $endframe]
   set Nframes [chainSize $cframes]
   
   # Get frame id's
   set frames [vFromChain $cframes field]
   set first_frame [exprGet $frames.vec<0>]
   set last_frame [exprGet $frames.vec<[expr $Nframes - 1]>]
   # remove not needed elements from cframes
   set delta [expr $startframe - $first_frame]  
   if {$delta > 0} {
      loop i 0 $delta {
          chainElementRemByPos $cframes 0
      }
   }
   set delta [expr $last_frame - $endframe]  
   if {$delta > 0} {
      loop i 0 $delta {
          set nlast [expr [chainSize $cframes] - 1]
          chainElementRemByPos $cframes $nlast
      }
   }
   vectorExprDel $frames    
   set frames [vFromChain $cframes field]
   vNameSet $frames ""
   set date [getdate]
   set runstr [format %06d $run]

   # For each color:
   loop i 0 $nfilters {
      if [catch {
      
      # get filtername
      set cal [chainElementGetByPos $cframes 0]
      set filter [exprGet $cal.calib<$i>->filter<0>]
      #
      # Set up the plot and label x-axes
      #
      set pg [pgstateNew]
      set device [devicename $device $outputDir/psPlots-$runstr-$filter$col.ps] 
      pgstateSet $pg -device $device \
      -xfract 0.22 -yfract 0.35 -nxwindow 4 -nywindow 3 -symb 7
      pgstateOpen $pg
      
      topLabel 20 "PSP, frames: $startframe-$endframe" \
	  $run $col $date $filter $rerun

      # Plot sky
      set sky [vFromChain $cframes calib<$i>->sky]
      vNameSet $sky ""
      vPlot $pg $frames $sky -ymin [vExtreme $sky min] \
      -ymax [vExtreme $sky max] -xmin $startframe -xmax $endframe
      titlePlot "skyTypical (ADU)" 40
      xlabel Field
      vectorExprDel $sky

      # Plot bias
      set lbias [vFromChain $cframes calib<$i>->lbias]
      vectorExprSet $lbias $lbias/$bias_scale
      set rbias [vFromChain $cframes calib<$i>->rbias]
      vectorExprSet $rbias $rbias/$bias_scale
      vNameSet $lbias ""

      set lmin [vExtreme $lbias min] 
      set lmax [vExtreme $lbias max] 
      set rmin [vExtreme $rbias min] 
      set rmax [vExtreme $rbias max] 

      if {$lmin < $rmin} {
     set min $lmin
      } else {
     set min $rmin
      }
      if {$lmax > $rmax} {
     set max $lmax
      } else {
     set max $rmax
      }

      vPlot $pg $frames $lbias -ymin $min -ymax $max  \
      -xmin $startframe -xmax $endframe
      pgstateSet $pg -isNewplot 0 -symb 4
      vPlot $pg $frames $rbias -ymin $min -ymax $max  \
      -xmin $startframe -xmax $endframe
      pgstateSet $pg -isNewplot 1 -symb 7
      titlePlot "Bias drift (ADU)" 40
      xlabel Field

      vectorExprDel $lbias
      vectorExprDel $rbias

      #
      # Plot number of stars included in average PSF for each frame
      #
      set nstar [vFromChain $cframes calib<$i>->psf_nstar]
      vNameSet $nstar ""

      set filter [lindex $filterlist $i] 
      if [info exists frameinfolist($filter)] {
     set npoints [vFromChain $frameinfolist($filter) Nprof]
     vNameSet $npoints ""
      }

      set nmax [expr [vExtreme $nstar max]+1]
      if [info exists npoints] {
     set nmax2 [vExtreme $npoints max]
     if {$nmax2 > $nmax} {
        set nmax $nmax2
     }
      }
      vPlot $pg $frames $nstar \
      -ymin 0 -ymax $nmax -xmin $startframe -xmax $endframe
      pgstateSet $pg -isLine 1 -isNewplot 0
      vPlot $pg $frames $nstar \
      -ymin 0 -ymax $nmax -xmin $startframe -xmax $endframe
      pgstateSet $pg -isLine 0 -isNewplot 0
      vectorExprDel $nstar
      xlabel Field

      # Plot number of points in composite profile, if available
      if [info exists npoints] {
     pgstateSet $pg -isNewplot 0 -symb 4
     vPlot $pg $frames $npoints \
         -ymin 0 -ymax $nmax -xmin $startframe -xmax $endframe
     titlePlot "N*/frame & N pts/prof" 40

     vectorExprDel $npoints
      } else {
     titlePlot "N*/frame" 40
      }

      #
      # plot psfs
      #
      pgstateSet $pg -isNewplot 1 -symb 7      
      set sigmax1 [vFromChain $cframes calib<$i>->psf->sigmax1]
      set sigmax2 [vFromChain $cframes calib<$i>->psf->sigmax2]
      set min [vExtreme $sigmax1 min]
      set max [vExtreme $sigmax2 max]
      if {$max > 15.0} {
          set max 10.0
      }
      vNameSet $sigmax1 ""
      if {[vExtreme $sigmax1 min] < 0.8} {
     pgstateSet $pg -icMark 2
      }
      vPlot $pg $frames $sigmax1 -xmin $startframe -xmax $endframe \
      -ymin 0.0 -ymax $max
      pgstateSet $pg -isNewplot 0 -symb 4 -icMark 1
      vPlot $pg $frames $sigmax2 -xmin $startframe -xmax $endframe \
      -ymin 0.0 -ymax $max

      # 1 Gauss width
      if [info exists frameinfolist($filter)] {
     pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3 
     set width [vFromChain $frameinfolist($filter) PSFwidth]
     vNameSet $width ""
     vPlot $pg $frames $width -xmin $startframe -xmax $endframe \
         -ymin 0.0 -ymax $max
     vectorExprDel $width
      }
      titlePlot "PSF: \\gs\\d1\\u and \\gs\\d2\\u (pixels)" 40
      xlabel Field
      vectorExprDel $sigmax1
      vectorExprDel $sigmax2
      
      # Plot psf->b
      pgstateSet $pg -isNewplot 1 -symb 7 -icMark 1 -isLine 0 -icLine 1
      set b [vFromChain $cframes calib<$i>->psf->b]
      vNameSet $b ""
      extremes $b min_b max_b 0.2
      if {$min_b < 0} {
         pgstateSet $pg -icMark 2
      }
      vPlot $pg $frames $b -ymin 0.0 -ymax $max_b -xmin $startframe -xmax $endframe
      pgstateSet $pg -icMark 1
      titlePlot "PSF: amplitude ratio b" 40
      xlabel Field
      vectorExprDel $b

      # Plot profile data, cp(3) & 100*cp(5)
      if [info exists frameinfolist($filter)] {
         set p1 [vFromChain $frameinfolist($filter) pt1] 
         set p2 [vFromChain $frameinfolist($filter) pt2] 
         vectorExprSet $p2 100*$p2      
         set min [vExtreme $p1 min]
         set max [vExtreme $p1 max]
         set min1 [vExtreme $p2 min]
         set max1 [vExtreme $p2 max]
         if {$min1 < $min} {
           set min $min1
         }
         if {$max1 > $max} {
           set max $max1
         }
     
         vNameSet $p1 ""
         pgstateSet $pg -icMark 3
         vPlot $pg $frames $p1 -xmin $startframe -xmax $endframe \
            -ymin 0.0 -ymax $max
         pgstateSet $pg -isNewplot 0 -symb 4 -icMark 1
         vPlot $pg $frames $p2 -xmin $startframe -xmax $endframe \
            -ymin 0.0 -ymax $max
      } else {
         set p1 [vectorExprEval (1000,1001)]
         set p2 [vectorExprEval (1000,1001)]
         vPlot $pg $frames $p1 \
            -ymin 0 -ymax 1 -xmin $startframe -xmax $endframe
         pgPtext [expr 0.5*($startframe+$endframe)] 0.5 0 0.5 \
             "(not available)"
      }
      titlePlot "PSF: cp(3), 100*cp(5)" 40
      xlabel Field

      vectorExprDel $p1
      vectorExprDel $p2

      # Plot I[r(wing=core)]
      pgstateSet $pg -isNewplot 1 
      set rwc [vectorExprNew [chainSize $cframes]]
      get_rwc $cframes $i rwc
      vectorExprSet $rwc 100*$rwc
      vNameSet $rwc ""
      extremes $rwc min_p max_p 0.2
      vPlot $pg $frames $rwc -ymin 0.0 -ymax $max_p -xmin $startframe -xmax $endframe
      titlePlot "PSF: 100*i(wing=core)" 40
      xlabel Field      
      vectorExprDel $rwc
 
      # Plot psf->beta
      set beta [vFromChain $cframes calib<$i>->psf->beta]
      vNameSet $beta ""
      extremes $beta min_b max_b 0.2
      vPlot $pg $frames $beta -ymin $min_b -ymax $max_b -xmin $startframe \
            -xmax $endframe
      titlePlot "PSF: wing exponent \\gb" 40
      xlabel Field
      vectorExprDel $beta

      # Plot aperture correction
      set app_corr [vFromChain $cframes calib<$i>->app_correction]
      vNameSet $app_corr ""
      vPlot $pg $frames $app_corr -ymin [vExtreme $app_corr min] \
      -ymax [vExtreme $app_corr max] -xmin $startframe -xmax $endframe
      titlePlot "PSF: app_corr." 40
      xlabel Field
      vectorExprDel $app_corr

      # Plot psf->coeffs.L
      pgstateSet $pg -isNewplot 1 -symb 7
      set L [vFromChain $cframes calib<$i>->psf->coeffs.L]
      vNameSet $L ""
      if {[vExtreme $L max] > 2} {
          pgstateSet $pg -icMark 2
      }
      vPlot $pg $frames $L -ymin -0.05 -ymax [vExtreme $L max] \
      -xmin $startframe -xmax $endframe
      titlePlot "PSF: \\gx\\u2\\d/\\gn" 40
      # line showing requested chi2
      if [info exists frameinfolist($filter)] {
          set chisq_max [vFromChain $frameinfolist($filter) chisq_max]
          vNameSet $chisq_max ""
          pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3 -lineStyle 2 
          vPlot $pg $frames $chisq_max   
          pgstateSet $pg -isNewplot 1 -isLine 0 -icLine 1 -lineStyle 1 
          vectorExprDel $chisq_max 
      }
      pgstateSet $pg -icMark 1
      xlabel Field
      vectorExprDel $L

      # Plot patch calibration (flux20 uncorrected for extinction vs. time)
      set nctimes [exprGet $ctimes.nElements]  
      if {$nctimes > 0} {
         set Ngoodcalibs 0
         # some patches might not have all colors, count good ones
         loop ii 0 $nctimes {
             set onecalib [chainElementGetByPos $ctimes $ii]
             if {[exprGet $onecalib.calib<$i>->flux20Err] > 0} {
                incr Ngoodcalibs
             }
         }
         set times [vectorExprNew $Ngoodcalibs] 
         set pflux20 [vectorExprNew $Ngoodcalibs] 
         set pflux20Err [vectorExprNew $Ngoodcalibs] 
         # populate plotting vectors
         loop ii 0 $Ngoodcalibs {
             set onecalib [chainElementGetByPos $ctimes $ii]
             if {[exprGet $onecalib.calib<$i>->flux20Err] > 0} {
                  set mjd [exprGet $onecalib.mjd]
                  set frac_mjd [expr $mjd - [int $mjd]] 
                  handleSet $times.vec<$ii> $frac_mjd 
                  handleSet $pflux20.vec<$ii> [exprGet $onecalib.calib<$i>->flux20]
                  handleSet $pflux20Err.vec<$ii> [exprGet $onecalib.calib<$i>->flux20Err]              
             }
         } 

         vectorExprSet $pflux20 $pflux20/1000.0 
         vectorExprSet $pflux20Err $pflux20Err/1000.0 
         vNameSet $pflux20 ""
         # these are the values for each patch
         vPlot $pg $times $pflux20 -vectorYErr $pflux20Err \
         -xmax [vExtreme $times max] \
         -xmin [vExtreme $times min] \
         -ymin [vExtreme [vectorExprSet $vtmp $pflux20-$pflux20Err] min] \
         -ymax [vExtreme [vectorExprSet $vtmp $pflux20+$pflux20Err] max]
         titlePlot "MT calibration (kADU)" 40
 
         # plot error bars
         set Npatch [chainSize $ctimes]
         vPlotErr $times $pflux20 $pflux20Err $Npatch $pg 
         vectorExprDel $pflux20Err
         # plot calibration
           # this is the fit to flux20(t) 
           if {1} {
              set a [vectorExprNew 2]
              set tmin [vExtreme $times min]
              set tmax [vExtreme $times max]
              set edge 0.02
              set tL [expr $tmin + $edge *($tmax - $tmin)]
              set tR [expr $tmax - $edge *($tmax - $tmin)]
              handleSet $a.vec<0> $tL
              handleSet $a.vec<1> $tR
              set b [vectorExprNew 2]
              pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3
              if [info exists flux20_m($i)] { 
                 handleSet $b.vec<0> [expr ($tL * $flux20_m($i) + $flux20_c($i))/1000]  
                 handleSet $b.vec<1> [expr ($tR * $flux20_m($i) + $flux20_c($i))/1000] 
                 vPlot $pg $a $b
              }
              vectorExprDel $a; vectorExprDel $b;  
           }       
      } else {
     # put a dummy plot in the same place
     set pflux20 [vectorExprEval 1]
     vPlot $pg $pflux20 $pflux20
     titlePlot "No mtPatch available" 40
      }
      xlabel "Time (mjd fraction)"
      vectorExprDel $pflux20; 

      # Plot flux20 for each frame
      set flux20 [vFromChain $cframes calib<$i>->flux20]
      vectorExprSet $flux20 $flux20/1000.0 
      vNameSet $flux20 ""
      set flux20Err [vFromChain $cframes calib<$i>->flux20Err]
      vectorExprSet $flux20Err $flux20Err/1000.0 
      set fmin [vExtreme [vectorExprSet $vtmp $flux20-$flux20Err] min]
      set fmax [vExtreme [vectorExprSet $vtmp $flux20+$flux20Err] max]
      pgstateSet $pg -isNewplot 1 -isLine 0 -icLine 1 -lineStyle 1 
      vPlot $pg $frames $flux20 -vectorYErr $flux20Err \
      -xmin $startframe -xmax $endframe -ymin $fmin -ymax $fmax
      titlePlot "Flux at mag20 (kADU)" 40
      xlabel Field
      vectorExprDel $flux20; vectorExprDel $flux20Err
      # mark the positions of calibration patches 
      if [info exists frameinfolist($filter)] {
         # vertical lines
         set nframes [chainSize $frameinfolist($filter)]
         set edge 0.04
         set f20min [expr $fmin + $edge*($fmax - $fmin)]
         set f20max [expr $fmax - $edge*($fmax - $fmin)]
         loop i 0 $nframes { 
            set frameinfo [chainElementGetByPos $frameinfolist($filter) $i]
        set flux20 [exprGet $frameinfo.patch_flux20]
            if {$flux20 > 0} {
               set frame [expr $startframe + $i]
               set a [vectorExprNew 2]
               handleSet $a.vec<0> $frame
               handleSet $a.vec<1> $frame
               set b [vectorExprNew 2]
               handleSet $b.vec<0> $f20min
               handleSet $b.vec<1> $f20max
               pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3
               vPlot $pg $a $b
               vectorExprDel $a; vectorExprDel $b; 
            }
         }
      }
      # catch for the filter loop
      } msg] {
     echo "filter $filter: $msg"
      }

      endplot $pg
      if {$nctimes > 0} {
     vectorExprDel $times
      }
   }      
   vectorExprDel $frames
   vectorExprDel $vtmp

   chainDestroy $ctimes calib1bytimeDel
   chainDestroy $cframes calib1byframeDel
}



proc vPlotErr {x y yerr N pg {dx 0}} {

 catch {
    # determine the width of error bar
    if {$dx == 0} { 
       set Nm1 [expr $N - 1]
       set delx [expr [exprGet $x.vec<$Nm1>] - [exprGet $x.vec<0>]] 
       set dx [expr 0.025 * $delx] 
    } 

    loop i 0 $N {
        set xbar [vectorExprNew 6]
        set vec [exprGet $x.vec<$i>]
        handleSet $xbar.vec<0> [expr $vec - $dx]
        handleSet $xbar.vec<1> [expr $vec + $dx]
        handleSet $xbar.vec<2> $vec
        handleSet $xbar.vec<3> [exprGet $xbar.vec<2>]
        handleSet $xbar.vec<4> [exprGet $xbar.vec<0>]
        handleSet $xbar.vec<5> [exprGet $xbar.vec<1>]
        set ybar [vectorExprNew 6]
        set vec [exprGet $y.vec<$i>]
        set vecErr [exprGet $yerr.vec<$i>]
        handleSet $ybar.vec<0> [expr $vec<$i> + $vecErr] 
        handleSet $ybar.vec<3> [expr $vec<$i> - $vecErr] 
        handleSet $ybar.vec<1> [exprGet $ybar.vec<0>]
        handleSet $ybar.vec<2> [exprGet $ybar.vec<0>] 
        handleSet $ybar.vec<4> [exprGet $ybar.vec<3>]
        handleSet $ybar.vec<5> [exprGet $ybar.vec<3>]  
        # mask
        set mask [vectorExprEval "$xbar < 1.0e+20 && $ybar < 1.0e+20 ? 1 : 0"]     
        pgstateSet $pg -isNewplot 0 -isLine 1 
        vPlot $pg $xbar $ybar -vectorMask $mask
        vectorExprDel $xbar; vectorExprDel $ybar; vectorExprDel $mask
    } 
    pgstateSet $pg -isLine 0 -isNewplot 1 
  }

}


#
# Get the date
#
proc getdate {} {
   set ct [split [ctime]]
   regsub {:[0-9][0-9]$} [lindex $ct 3] "" time
   return "$time [lindex $ct 2] [lindex $ct 1] [lindex $ct 4]"
}
#
# plot point types used with identification
#      pos is order within the set of points to be identified
#      symb is the point type
#      title is the id
#      xmin xmax ymin ymax are the limits of the plot
#
proc vPlotLegend {pos symb title xmin xmax ymin ymax} {
  set dx [expr ($xmax-$xmin)/40]
  if ($dx==0) {set dx 0.05}
  set dy [expr ($ymax-$ymin)/20]
  set x [expr $xmin+$dx]
  set y [expr $ymax-$pos*$dy]
  pgPoint $x $y $symb
  pgText [expr $x+$dx] $y $title
}

#
# Draw title upper-left of the screen.
#
proc titlePlot {ytitle scale} {
    set win [lindex [pgQwin] 0] 
    set xmin [lindex $win 0]
    set xmax [lindex $win 1]
    set ymin [lindex $win 2]
    set ymax [lindex $win 3]

    pgText $xmin [expr $ymax+2*(($ymax)-($ymin))/$scale] $ytitle
}

#
# Label the top of a panel of plots
#
proc topLabel {scale title args} {
   global color_index

   set win [lindex [pgQwin] 0] 
   set xmin [lindex $win 0]
   set xmax [lindex $win 1]
   set ymin [lindex $win 2]
   set ymax [lindex $win 3]

   if {$args == ""} {
      pgText $xmin [expr $ymax + 2*($ymax - $ymin)/$scale] $title
   } else {
      set run [lindex $args 0]
      set col [lindex $args 1]
      set date [lindex $args 2]
      set filter [lindex $args 3]
      set rerun [lindex $args 4]

      set fmt "%s  Run: %5d"
      if {$rerun != ""} {
	 append fmt ":$rerun"
      }
      append fmt ", camCol: %d,  %s"
      
      pgText $xmin [expr $ymax + 2*($ymax - $ymin)/$scale] \
	  [format $fmt $title $run $col $date]
   
      if [info exists color_index($filter)] {
	 set ci $color_index($filter)
      } else {
	 set ci 1
      }
      pgSci $ci
      pgText [expr $xmin + 0.9*($xmax - $xmin)] \
      [expr $ymax+2*(($ymax)-($ymin))/$scale] "Filter: $filter"
      pgSci 1
   }
}

#
# Label the x/y-axis of a plot
#
proc xlabel {lab} {
   pgMtext B 2.3 0.5 0.5 $lab
}

proc ylabel {lab} {
   pgMtext L 2.0 0.5 0.5 $lab
}


# given a chain of calib1byframe, find the radii where the PSF 
# core and wing contributions are equal, and return the vector 
# corresponding intensities 
proc  get_rwc {cframes i r_wc} {

      upvar $r_wc rwc
 
      # get vectors with psf parameters
      set sigmax1 [vFromChain $cframes calib<$i>->psf->sigmax1]
      set sigmax2 [vFromChain $cframes calib<$i>->psf->sigmax2]
      set b [vFromChain $cframes calib<$i>->psf->b]   
      set p0 [vFromChain $cframes calib<$i>->psf->p0]
      set sigmap [vFromChain $cframes calib<$i>->psf->sigmap]
      set beta [vFromChain $cframes calib<$i>->psf->beta]   
      set n [chainSize $cframes]

      # for each frame calculate r(wing=core)
      loop j 0 $n {
          set s1 [exprGet $sigmax1.vec<$j>]  
          set s2 [exprGet $sigmax2.vec<$j>] 
          set bb [exprGet $b.vec<$j>] 
          set pp [exprGet $p0.vec<$j>] 
          set sp [exprGet $sigmap.vec<$j>] 
          set bt [exprGet $beta.vec<$j>]   
          handleSet $rwc.vec<$j> [find_rwc $s1 $s2 $bb $pp $sp $bt]
      }

      # delete psf vectors
      vectorExprDel $sigmax1
      vectorExprDel $sigmax2
      vectorExprDel $b
      vectorExprDel $p0
      vectorExprDel $sigmap
      vectorExprDel $beta
} 


# given psf parameters, find the radius where the core and wing
# contributions are equal, and return the corresponding intensity
proc find_rwc {s1 s2 bb pp sp bt} {

   if {[expr 1 + $bb] < $pp} {
      return 1.0
   }
   if {$pp <= 0} {
      return 0.0
   }

   set r 0

   while {$r < 50} {
       # first bracket the zero
       set r [expr $r + 2]
       if {[psf_func 0 $r $s1 $s2 $bb $pp $sp $bt] < 0} {
           # bracketted
           set rL [expr $r - 2]
           set rR $r
           # improve the root
           set iter 0
           while {$iter < 10} {
              incr iter
              set rm [expr 0.5*($rL + $rR)]
              set fm [psf_func 0 $rm $s1 $s2 $bb $pp $sp $bt] 
              set psf [psf_func 1 $rm $s1 $s2 $bb $pp $sp $bt] 
              if {[expr abs($fm/$psf)] < 0.01} {
                 return $psf
              } else {
                 if {$fm > 0} { 
                    set rL $rm
                 } else { 
                    set rR $rm
                 }
              }
           }
           return $psf
       }   
   }

   return 0.0

}

# given a radius and psf parameters, evaluate psf(r) for type != 0,
# and psf_core(r)-psf_wing(r) for type = 0
proc psf_func {type r s1 s2 bb pp sp bt {g1Amplitude 1.0}} {
  set arg [expr $r * $r / 2.0 / $s1 / $s1]
  if {$arg < 100} {
      set core [expr exp(-$arg)] 
  } else {
      set core 0
  }
  set core [expr $core*$g1Amplitude]
  set arg [expr $r * $r / 2.0 / $s2 / $s2]
  if {$arg < 100} {
      set core [expr $core + $bb * exp(-$arg)] 
  }
  set arg [expr 1 / (1.0 + $r * $r / $sp / $sp / $bt)]
  set i0 [expr 1.0 + $bb + $pp]  
  if {$type == 0} {
      return [expr ($core - $pp * pow($arg, 1.0*$bt/2)) / $i0]
  } else {
      return [expr ($core + $pp * pow($arg, 1.0*$bt/2)) / $i0]
  }
}

######################################################################
proc read_ctimes {fitsfile filterlist} {
   
   set hdr [hdrReadAsFits [hdrNew] $fitsfile]
   set filters [hdrGetAsAscii $hdr FILTERS]
   set ncolors [llength $filters]
   hdrFreeAll $hdr; hdrDel $hdr

   foreach filter $filterlist {
      if {[set psindex($filter) [lsearch $filters $filter]] == -1} {
         error "$fitsfile does not contain info for filter $filter"
      }
   }
   calc_crc $fitsfile
   set ctimes [chainNew CALIB1BYTIME]
   declare_io_schematrans_for_calib1bytime
   set calibio [calibIoNew $ncolors]

   if {[catch {set fd [fitsBinTblOpen $fitsfile r]} msg]} {
       echo $msg
   } elseif {[catch {
      set nrow [fitsBinTblHdrRead $fd CALIB_IO]
      loop i 0 $nrow {
     fitsBinTblRowSeek $fd $i  0
     fitsBinTblRowRead $fd $calibio
     set calib1bytime [calib1bytimeNewFromCalibIo $calibio $filterlist \
                   psindex]
     chainElementAddByPos $ctimes $calib1bytime TAIL AFTER
      }
   } msg]} {

       echo "Failed to read $fitsfile for old calib:"
       echo $msg
       catch {
          fitsBinTblClose $fd
       }
    } else {
       calc_crc $fitsfile
       fitsBinTblClose $fd
    }
    fitsBinForgetSchemaTrans CALIB_IO
   calibIoDel $calibio
   return $ctimes
}

proc read_cframes {cbfile filterlist startframe endframe} {

   set hdr [hdrReadAsFits [hdrNew] $cbfile]
   set filters [hdrGetAsAscii $hdr FILTERS]
   set ncolor [llength $filters]
   set field0 [hdrGetAsInt $hdr FIELD0]
   set nfields [hdrGetAsInt $hdr NFIELDS]
   hdrFreeAll $hdr; hdrDel $hdr

   if {$startframe < $field0} {
      error "read_cframes: $cbfile does not contain info for field $startframe"
   } elseif { $endframe >= [expr $field0 + $nfields]} {
      error "read_cframes: $cbfile does not contain info for field $endframe"
   }

   foreach filter $filterlist {
      if {[set psindex($filter) [lsearch $filters $filter]] == -1} {
         error "$cbfile does not contain info for filter $filter"
      }
   }

   declare_io_schematrans_for_calib1byframe

   set fd [fitsBinTblOpen $cbfile r]
   set nrow [fitsBinTblHdrRead $fd CALIB_IO]

   set calibio [calibIoNew [llength $filters]]
   set cframes [chainNew CALIB1BYFRAME]
   loop row 0 $nrow {

      fitsBinTblRowSeek $fd $row 0
      fitsBinTblRowRead $fd $calibio 
      set calib1byframe [calib1byframeNewFromCalibIo $calibio $filterlist \
                 psindex]
      if {([expr $field0 + $row] >= $startframe) && ([expr $field0 + $row] <= $endframe)} {         
         chainElementAddByPos $cframes $calib1byframe TAIL AFTER
      }
   }

   calibIoDel $calibio
   fitsBinTblClose $fd
   fitsBinForgetSchemaTrans CALIB_IO
   return $cframes
}

# test for composite profile going negative
proc testCompProf {psFieldChain {details 0}} {

    set Nel [chainSize $psFieldChain]
    if {$Nel <= 0} {return 0} 
    set el [chainElementGetByPos $psFieldChain 0]
    set ncolors [exprGet $el.ncolors]

    set QAflag 0
    loop j 0 $ncolors {
       set badfields($j) ""  
    }                
    loop i 0 $Nel {
       set el [chainElementGetByPos $psFieldChain $i]
       loop j 0 $ncolors {
          set nprof [exprGet $el.calib<$j>->prof->nprof]
          set prof  [exprGet $el.calib<$j>->prof->mean<[expr $nprof-1]>]
          if {$prof < 0} {
             incr QAflag
             if {$details} {
                lappend badfields($j) $i
             }
          }
       }
    }

    if {$details} {
       loop j 0 $ncolors {
          lappend QAflag $badfields($j)   
       }                
    }

   return $QAflag

}

# read a list of psField files and print a nicely formatted table of
# various parameters: for type=1 print PSF parameters, for any other value
# print other parameters for PSP output
# psFieldroot is $dir/psField-$runstr-$col-
proc read_psField {psFieldroot {type 1} {filterlist {u g r i z}} {sf 0} {ef 1000} {psPSF 0}} {

   # get the chain of calib1byframe
   set chain [get_cal_chain_from_psField $psFieldroot $sf $ef $psPSF]

   # get ncolors
   set cal [chainElementGetByPos $chain 0]
   set ncolors [exprGet $cal.ncolors]
   handleDel $cal

   if {$type == 1} {
      set header " psp_status status    n PSFwidth s1_2G  s2_2G  b_2G   \
                  s1     s2     b     p0    sp  beta   N*  psf_err CPcnts"
   } elseif {$type == 2} {
      set header  " psp_status status    n   PSFwidth  sky   s_err s_sig   flux20  flux20Err \
                    ap_corr_run  ap_corr_runErr"
   } elseif {$type == 3} {
      set header  " psp_status status    n   PSFwidth  sky   s_err \
                    s_sig        lbias   rbias     lOEdrift rOEdrift"
   }

   #### print out nice tables ####
   set Nframes [chainSize $chain]
   echo " "; echo " "
   loop ifilter 0 $ncolors {
      set el0 [chainElementGetByPos $chain 0]
      set this_filter [exprGet $el0.calib<$ifilter>->filter]
      handleDel $el0
      if {[lsearch $filterlist $this_filter] > -1} {
         echo " === Filter $this_filter ==="
         echo $header
         loop i 0 $Nframes {
             set cal [chainElementGetByPos $chain $i]
             set iFr [exprGet $cal.field]       
             set string [string_from_calib1 $cal $ifilter $type] 
             if {$iFr >= $sf && $iFr <= $ef} {
             echo $string
             }
         }
         echo " "
     }
   }

   return 0

}


# stuff a string with the data from calib1byframe
# type = 1 for PSF data, type = 2 for sky, type = 3 for bias,  etc.
proc string_from_calib1 {cal ifilter {type 1}} {

             set iFr [exprGet $cal.field]        
             set aux [exprGet -enum $cal.psp_status]
             set psp_status [format "%8s" [string range $aux 10 [expr [string length $aux]-1]]] 
             set aux [exprGet -enum $cal.calib<$ifilter>->status]
             set status [format "%7s" [string range $aux 10 [expr [string length $aux]-1]]] 
             set psf_err [exprGet $cal.calib<$ifilter>->psf_ap_correctionErr]
             set width [exprGet $cal.calib<$ifilter>->psf->width]
             set sky [exprGet $cal.calib<$ifilter>->sky]
             set skyerr [exprGet $cal.calib<$ifilter>->skyerr]
             set skysig [exprGet $cal.calib<$ifilter>->skysig]
             set flux20 [exprGet $cal.calib<$ifilter>->flux20]
             set acr [exprGet $cal.calib<$ifilter>->ap_corr_run]
             set acrErr [exprGet $cal.calib<$ifilter>->ap_corr_runErr]
             set nstar [exprGet $cal.calib<$ifilter>->psf_nstar] 
             set sigG1 [exprGet $cal.calib<$ifilter>->psf->sigma1_2G]
             set sigG2 [exprGet $cal.calib<$ifilter>->psf->sigma2_2G]
             set bG    [exprGet $cal.calib<$ifilter>->psf->b_2G]
             set sig1 [exprGet $cal.calib<$ifilter>->psf->sigmax1]
             set sig2 [exprGet $cal.calib<$ifilter>->psf->sigmax2]
             set b    [exprGet $cal.calib<$ifilter>->psf->b]
             set p0    [exprGet $cal.calib<$ifilter>->psf->p0]
             set sigmap    [exprGet $cal.calib<$ifilter>->psf->sigmap]
             set beta    [exprGet $cal.calib<$ifilter>->psf->beta]
             set lB [exprGet $cal.calib<$ifilter>->lbias]
             set rB [exprGet $cal.calib<$ifilter>->rbias]
             set lBD [exprGet $cal.calib<$ifilter>->lbiasOEdiff]
             set rBD [exprGet $cal.calib<$ifilter>->rbiasOEdiff]

             # from DGPSF
             set psfCounts [format "%6.3f" [exprGet $cal.calib<$ifilter>->psf->psfCounts]] 
             # from COMP_PROF
             set psf_counts  [format "%6.3f" \
                             [exprGet $cal.calib<$ifilter>->prof->psfCounts]]
             set flux20Err [exprGet $cal.calib<$ifilter>->flux20Err]
             if [catch {
                      set string1 [format " %5s %7.3f  %6.2f %6.2f %5.2f %6.2f\
                                   %6.2f %4.2f %6.4f %5.2f %5.2f %4d %7.3f %8.4g " \
                      $iFr $width $sigG1 $sigG2 $bG \
                      $sig1 $sig2 $b $p0 $sigmap $beta $nstar $psf_err $psf_counts ]         
                      set string2 [format "  %6s  %6.2f  %6.2f %5.2f %5.2f  %8s %8s %11.3f %10.3f" \
                      $iFr $width $sky $skyerr $skysig $flux20 $flux20Err $acr $acrErr]
                      set string3 [format "  %6s  %6.2f  %6.2f %5.2f %5.2f   %8s %8s   %8s %8s" \
                      $iFr $width $sky $skyerr $skysig $lB $rB $lBD $rBD]
             }] {
                set string1 "problems reading"
                set string2 $string1  
              }
             if {$type == 1} {    
                 return "$psp_status $status $string1"
             } elseif {$type == 2} {
                 return "$psp_status $status $string2"              
             } elseif {$type == 3} {
                 return "$psp_status $status $string3"              
             }

}



# given a psField file, return the value of requested record in given filter
proc get_from_psField {psField filter record {psPSF 0}} {

   global allow_old_psField
     set allow_old_psField 1

     set cal [get_calib1byframe_from_psField $psField all $psPSF] 
     set ncolors [exprGet $cal.ncolors]
     loop i 0 $ncolors {
     if {$filter == [exprGet $cal.calib<$i>->filter]} {
            set ifilter $i
         }
     }
     if {![info exist ifilter]} {
         echo "filter $f doesn't exist"
         return ""
     }
   if {$record == "status"} {
      set value [exprGet (int)$cal.calib<$ifilter>->$record]
   } else {
      set value [exprGet $cal.calib<$ifilter>->$record]
   }
     calib1byframeDel $cal

     return $value

}



# read a list of psField files and plot composite profiles
# psFieldroot is $dir/psField-$runstr-$col-
# for 
# type = 1: plot median profiles
# type = 2: plot mean profiles
# type = 3: plot median profiles and overplot model fit as symbols
# type = 4: as in 3, but also plot each fitted component separately
# type = 5: plot the curves of growth (by using median profile)
# n.b. type = 3 and 4 is useful only for small number of fields
proc plot_comp_prof {device psFieldroot {type 1} {sf 0} {ef 1000}   \
                {title ""} {filterlist {u g r i z}} {outfile ""}} {

   # get the chain of calib1byframe
   set chain [get_cal_chain_from_psField $psFieldroot $sf $ef 0]
   set all_filters {u g r i z}

   # get some sizes
   set Nfields [chainSize $chain] 
   # get ncolors
   set Nfilters [llength $filterlist]
   set cal [chainElementGetByPos $chain 0]
   set ncolors [exprGet $cal.ncolors]
   handleDel $cal  
   # this really needs to be a check for each and every filter
   assert { $Nfilters <= $ncolors }
   initProfileExtract
   # fake central pixel (need it on log scale) by 0.1 arcsec = 1/4 pixel
   if {$type != 5} {
       set radii "0.25 [profileRadii]" 
   } else {
       set radii "0.00 [profileRadii]" 
   }
   if {$type == 1 || $type == 3 || $type == 4 || $type == 5} {
       set pType med
   } else {
       set pType mean
   }

   #### plot profiles ####

   # Set up the plot and label
   set pg [pgstateNew]
   if {$outfile == ""} {
      set outfile comp_profs_$type.ps
   }
   set deviceA [devicename $device $outfile]
   set nxwindow 1; set nywindow 2
   set xf 0.3; set yf 0.35 
   if {$Nfilters > 2} {
      set nxwindow 2; set nywindow 3
      set xf 0.35; set yf 0.4 
   }
   if {$Nfilters > 4} {
      set nywindow 4; 
      set xf 0.3; set yf 0.4   
   } 
   pgstateSet $pg -device $deviceA -xfract $xf -yfract $yf \
      -nxwindow $nxwindow -nywindow $nywindow -symb 7 -isLine 0   
   pgstateOpen $pg
   pgstateSet $pg -isNewplot 1 -isLine 1  -icMark 3 -symb 4 -icLine 2 -lineWidth 3 -lineStyle 1 

   ### loop over bands
   foreach f $filterlist {
      set j [lsearch $all_filters $f] 
      # get and plot each profile vector
      # loop over fields  
      loop i 0 $Nfields {
         set cal [chainElementGetByPos $chain $i]   
         set Nprof [exprGet $cal.calib<$j>->prof->nprof]
         set prof [vectorExprNew $Nprof]
         set sig [vectorExprNew $Nprof]
         set rad [vectorExprNew $Nprof]
         set radD [vectorExprNew $Nprof]
         if {$type == 3 || $type == 4} {
             set mod [vectorExprNew $Nprof]
         }
         if {$type == 5} {
             set growth [vectorExprNew $Nprof]
             set linrad [vectorExprNew $Nprof]
         }
         set ff [exprGet $cal.calib<$j>->prof->$pType<0>]
         set Lff [expr -log10($ff)]
         set counts 0.0
         loop k 0 $Nprof {
             set a [catch {
                         set aux [expr $Lff+log10([exprGet $cal.calib<$j>->prof->$pType<$k>])]
                  }]
             if {$a} {
                 set aux -99.9
             }            
             handleSet $prof.vec<$k> $aux
             set fac1 [exprGet $cal.calib<$j>->prof->sig<$k>]
             set fac2 [exprGet $cal.calib<$j>->prof->$pType<$k>]
             set errfac [expr ($fac1+$fac2)/$fac2]
             handleSet $sig.vec<$k> [expr log10(abs($errfac))]
             if {$type != 5} {
                handleSet $rad.vec<$k> [expr log10(0.4*[lindex $radii $k])]             
                if {$k > 0} {
                   set rD [expr sqrt(pow([lindex $radii $k],2)+pow([lindex $radii [expr $k-1]],2))]
                } else {
                   set rD [lindex $radii $k]
                }
                handleSet $radD.vec<$k> [expr log10(0.4*$rD)] 
             }
             if {$type == 5} {
                set radius1 [lindex $radii $k]
                set radius2 [lindex $radii [expr $k + 1]]
                # dA in arcsec^2
                set area [expr 0.4*0.4*3.14159*(pow($radius2,2)-pow($radius1,2))]   
                # counts for area in pix^2 and I(r) normalized to 1 for r=0
                set counts [expr $counts + $area * [exprGet $cal.calib<$j>->prof->$pType<$k>]/$ff]             
                handleSet $growth.vec<$k> $counts
                # radius in arcsec
                handleSet $linrad.vec<$k> [expr 0.4*$radius2]
             }

             # calculate model
             if {$type == 3 || $type == 4} {
                set radius [lindex $radii $k]
                set s1 [exprGet $cal.calib<$j>->psf->sigmax1]
                set s2 [exprGet $cal.calib<$j>->psf->sigmax2]
                set b    [exprGet $cal.calib<$j>->psf->b]
                set p    [exprGet $cal.calib<$j>->psf->p0]
                set sigp    [exprGet $cal.calib<$j>->psf->sigmap]
                set beta    [exprGet $cal.calib<$j>->psf->beta]
                set psf [psf_func 1 $radius $s1 $s2 $b $p $sigp $beta]              
                if {$k == 0} { 
                    set psf0 $psf; set psf 1.0
                } else {
                    set psf [expr $psf/$psf0]
                }
                handleSet $mod.vec<$k> [expr log10(abs($psf))] 
             }
         }
         vNameSet $rad "log(r/arcsec)"
         vNameSet $prof "log(I(r)/I(0))"
         if {$type == 3 || $type == 4} {
             vNameSet $mod "log(I(r)/I(0))"
             pgstateSet $pg -icMark 4  -isLine 1 
             vPlot $pg $rad $mod -xmin -1.2 -xmax 1.5 -ymin -6.5 -ymax 0.5
             pgstateSet $pg -icMark 4 -isLine 0 -symb 4 -isNewplot 0
             vPlot $pg $radD $prof -xmin -1.2 -xmax 1.5 -ymin -6.5 -ymax 0.5
             pgstateSet $pg -icMark 4 -isNewplot 0 -isLine 1
         } elseif {$type == 5} {
             # this plots the curve of growth
             # first renormalize the curve of growth to 1 at the last point
             set fluxTot [exprGet $growth.vec<[expr $Nprof-1]>]
             loop l 0 $Nprof {
                handleSet $growth.vec<$l> [expr [exprGet $growth.vec<$l>]/$fluxTot]
             } 
             vNameSet $linrad "r (arcsec)"
             #vNameSet $growth "int\[prof(r)/prof(0)dA\]_0^r"
             vPlot $pg $linrad $growth -xmin 0.0 -xmax 35.0 -ymin 0.0 -ymax 2.0
             pgstateSet $pg -icMark 4 -isNewplot 0 -isLine 1 -symb 4               
         } else {
             vPlot $pg $rad $prof -xmin -1.2 -xmax 1.5 -ymin -6.5 -ymax 0.5
             pgstateSet $pg -icMark 4 -isNewplot 0 -isLine 1 -symb 4
         }
         if {$type == 4} {
             pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1 -lineStyle 2
             set Npt 60
             set fineR [vectorExprNew $Npt]
             set Vpsf1 [vectorExprNew $Npt]
             set Vpsf2 [vectorExprNew $Npt]
             set Vpsf3 [vectorExprNew $Npt]
             loop k 0 $Npt {
                set logrpix [expr -1.0 + 0.05*$k]
                set rrpix [expr pow(10.0,$logrpix)]
                set psf1 [expr [psf_func 1 $rrpix $s1 $s2 0.0 0.0 $sigp $beta]/(1+$b+$p)]
                set psf2 [expr (1+$b)*[psf_func 1 $rrpix $s1 $s2 $b 0.0 $sigp $beta 0.0]/(1+$b+$p)]
                set psf3 [expr (1+$p)*[psf_func 1 $rrpix $s1 $s2 0.0 $p $sigp $beta 0.0]/(1+$b+$p)]
                handleSet $fineR.vec<$k> [expr log10(0.4*$rrpix)]
                handleSet $Vpsf1.vec<$k> [expr log10(1.0e-12+abs($psf1/$psf0))] 
                handleSet $Vpsf2.vec<$k> [expr log10(1.0e-12+abs($psf2/$psf0))] 
                handleSet $Vpsf3.vec<$k> [expr log10(1.0e-12+abs($psf3/$psf0))]  
             }
             pgstateSet $pg -icLine 3 -isNewplot 0 -isLine 1
             vPlot $pg $fineR $Vpsf1 -xmin -1.2 -xmax 1.5 -ymin -6.5 -ymax 0.5
             pgstateSet $pg -icLine 3 -isNewplot 0 -isLine 1
             vPlot $pg $fineR $Vpsf2 -xmin -1.2 -xmax 1.5 -ymin -6.5 -ymax 0.5
             pgstateSet $pg -icLine 6 -isNewplot 0 -isLine 1
             vPlot $pg $fineR $Vpsf3 -xmin -1.2 -xmax 1.5 -ymin -6.5 -ymax 0.5
             pgstateSet $pg -icLine 2 -isNewplot 0 -isLine 0 -lineWidth 3 -lineStyle 1
             vListDel [list $fineR $Vpsf1 $Vpsf2 $Vpsf3]
         }
         if {$j == 0 && $i == 0} {
             if {$type == 1} {
                 set label "$title, median composite profile"
             } elseif {$type == 2} {
                 set label "$title, mean composite profile"
             } elseif {$type == 3 || $type == 4} {
                 set label "$title, symbols: data, lines: model"
             } else {
                 set label "$title, lines: curve of growth for med. comp. prof."
             }
             topLabel 20 $label
         }
         vectorExprDel $prof
         vectorExprDel $sig
         vectorExprDel $rad; vectorExprDel $radD
         if {$type == 3 || $type == 4} {
           vectorExprDel $mod   
         }                 
         if {$type == 5} {
             vectorExprDel $growth 
             vectorExprDel $linrad
         }
     }
     if {$type != 5} {
        pgText -1.2 -5.0 "Filter $f" 
        pgText -1.2 -6.5 "R:" 
        loop i 0 11 {
           pgText [expr log10(0.4*[lindex $radii $i])] -6.5 $i
        }
     } else {
        pgText 23.0 0.1 "Filter $f" 
        pgText 0.0 1.6 "R:" 
        loop i 5 11 {
           pgText [expr 0.4*[lindex $radii $i]] 1.6 $i
        }
     }
     pgstateSet $pg -isNewplot 1
  }
 
  endplot $pg
  genericChainDestroy $chain

  return 0

}
####### end of plot_comp_prof




# read a list of psField files and plot the ratio of fluxes encircled
# by radius(index2) and radius(index1), and determined from the median 
# composite profile vs. seeing 
# psFieldroot is $dir/psField-$runstr-$col-
# profType can be "med" or "mean"
proc plot_corrApCorr {device psFieldroot index1 index2 {profType "med"} {sf 0} {ef 1000}   \
                  {title ""} {filterlist {u g r i z}} {outfile ""} {psPSF 0}} {

   assert { $index1 < $index2 }
   set all_filters {u g r i z}

   # get the chain of calib1byframe
   set chain [get_cal_chain_from_psField $psFieldroot $sf $ef $psPSF]
   if {[chainSize $chain] == 0} {
     return 1
   }


   # get some sizes
   set Nfields [chainSize $chain] 
   # get ncolors
   set Nfilters [llength $filterlist]
   set cal [chainElementGetByPos $chain 0]
   set ncolors [exprGet $cal.ncolors]
   handleDel $cal  
   # this really needs to be a check for each and every filter
   assert { $Nfilters <= $ncolors }
   initProfileExtract
   # fake central pixel
   set radii "0.0 [profileRadii]" 

   #### plot ####
   # Set up the plot and label
   set pg [pgstateNew]
   if {$outfile == ""} {
      set outfile corrApCorr.ps
   }
   set deviceA [devicename $device $outfile]
   set nxwindow 1; set nywindow 2
   set xf 0.3; set yf 0.35 
   if {$Nfilters > 2} {
      set nxwindow 2; set nywindow 3
      set xf 0.35; set yf 0.4 
   }
   if {$Nfilters > 4} {
      set nywindow 4; 
      set xf 0.3; set yf 0.4   
   } 
   pgstateSet $pg -device $deviceA -xfract $xf -yfract $yf \
      -nxwindow $nxwindow -nywindow $nywindow -symb 7 -isLine 0   
   pgstateOpen $pg
   pgstateSet $pg -isNewplot 1 -isLine 0 -icMark 2 -symb 4 

   if {$index1 < 0 || $index2 < 0} {
       set el [chainElementGetByPos $chain 0]
       set index1 [exprGet $el.calib<0>->nann_ap_frame]
       set index2 [exprGet $el.calib<0>->nann_ap_run]
   }

   ### loop over bands
   set seeing_list {}
   set model_list {}
   foreach f $filterlist {
      set j [lsearch $all_filters $f]      
      # get and plot each profile vector
      set seeing [vFromChain $chain calib<$j>->psf->width]
      lappend seeing_list $seeing
      set cac [vectorExprNew $Nfields]       
      # loop over fields  
      loop i 0 $Nfields {
         set cal [chainElementGetByPos $chain $i]   
         set Nprof [exprGet $cal.calib<$j>->prof->nprof]
         if {$Nprof > $index2} { 
             set comp_prof $cal.calib<$j>->prof->$profType          
             set flux_ratio [get_wc_correction $index1 $index2 $radii $comp_prof]
             handleSet $cac.vec<$i> [expr 2.5*log10($flux_ratio)]
         } else { 
             handleSet $cac.vec<$i> -1000.0
         }           
      }
      vNameSet $seeing "Filter $f, seeing (FWHM, arcsec)"
      set ymax [expr 1.2*[vExtreme $cac max]]
      set ymin [expr 0.8*[vExtreme $cac min]]
      if {$ymax < 0.1} {
         set ymax 0.1
      }
      if {$ymax < 0.2} {
         set ymin 0.0
      }
      vPlot $pg $seeing $cac -xmin 1.0 -xmax 3.2 -ymin $ymin -ymax $ymax
      ### model
      set Clist [vFitPolynom $seeing $cac 2]
      set model [vEvalPolynom $seeing $Clist]
      lappend model_list $model
      pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 4
      vPlot $pg $seeing $model
      set C0 [lindex $Clist 0]; set C1 [lindex $Clist 1]; set C2 [lindex $Clist 2] 
      pgText 2.6 [expr $ymin+0.4*($ymax-$ymin)] "[format %6.3f $C0]"
      pgText 2.6 [expr $ymin+0.2*($ymax-$ymin)] "[format %6.3f $C1]"
      pgText 2.6 [expr $ymin+0.0*($ymax-$ymin)] "[format %6.3f $C2]"
      # title    
      if {$j == 0} {
          set label "$title, 2.5*log\[flux($index2)/flux($index1)\]"           
          topLabel 20 $label
      }
      pgstateSet $pg -isNewplot 1 -isLine 0 -icMark 2 -symb 4 
      vectorExprDel $cac      
  }

  pgstateSet $pg -isLine 1  
  loop i 0 [llength $filterlist] {
      if {[lindex $filterlist $i] == "i"} {
          pgstateSet $pg -icLine 2 -lineStyle 2 -lineWidth 3
      } else {
          pgstateSet $pg -icLine 4 -lineStyle 1 -lineWidth 1
      }
      set s [lindex $seeing_list $i]; set m [lindex $model_list $i] 
      vPlot $pg $s $m -xmin 1.0 -xmax 3.2 -ymin $ymin -ymax $ymax 
      pgstateSet $pg -isNewplot 0
      vListDel [list $s $m]
  }
  
     
  endplot $pg
  genericChainDestroy $chain

  return 0

}
####### end of plot_corrApCorr #######


# for a given list with C(j), j=0,...,M-1
# generate an M-th order polynom for x=xmin-xmax
# and npoints points such that
# y = sum[C(j)*(x-xo)^j]
proc get_polynom {xmin xmax npoints x0 Clist} {

     set xVec [vectorExprNew $npoints]
     set yVec [vectorExprNew $npoints]
     set delx [expr ($xmax-$xmin)/($npoints-1.0)]
     loop i 0 $npoints {
        set y 0
        set x [expr $xmin + $i*$delx]
        loop j 0 [llength $Clist] {
            set C [lindex $Clist $j]
            set xaux [expr $x-$x0]
            set xauxj [expr pow($xaux,$j)]
            set y [expr $y + $C*$xauxj]
        }
        handleSet $xVec.vec<$i> $x
        handleSet $yVec.vec<$i> $y
     }

     return [list $xVec $yVec]  
}


# read a list of psField files and print composite profiles
# psFieldroot is $dir/psField-$runstr-$col-
# for 
# type = 1: print median profiles
# type = 2: print mean profiles
proc print_comp_prof {psFieldroot {type 1} {sf 0} {ef 1000}   \
                  {filterlist {u g r i z}} {outfile ""}} {

   set all_filters {u g r i z}

   # get the chain of calib1byframe
   set chain [get_cal_chain_from_psField $psFieldroot $sf $ef 0]

   # get some sizes
   set Nfields [chainSize $chain] 
   # get ncolors
   set Nfilters [llength $filterlist]
   set cal [chainElementGetByPos $chain 0]
   set ncolors [exprGet $cal.ncolors]
   handleDel $cal  
   # this really needs to be a check for each and every filter
   assert { $Nfilters <= $ncolors }
   initProfileExtract
   # fake central pixel 
   set radii "0.0 [profileRadii]" 
   if {$type == 1} {
       set pType med
   } else {
       set pType mean
   }

   if {$outfile != "" } {
      set outf [open $outfile "w"] 
   } else {
      set outf ""
   }

   #### print profiles ####
 
   ### loop over fields  
   loop i 0 $Nfields {
      set cal [chainElementGetByPos $chain $i]   
      echo "Field: [expr $i]"
      if {$outf != "" } {puts $outf "Field: [expr $i]"}
      set str "  radius(pix)"
      foreach f $filterlist {
         set str "$str         $f"
      }
      echo $str
      if {$outf != "" } {puts $outf $str}
      ### loop over radii 
      loop k 0 10 {
         set r [lindex $radii $k]
         set str "   $k  [format "%7.2f" $r]   "
         ## loop over bands
         foreach f $filterlist {
            set j [lsearch $all_filters $f] 
            # get and plot each profile vector
            set Nprof [exprGet $cal.calib<$j>->prof->nprof]
            if {$Nprof > $k} {
                set prof [exprGet $cal.calib<$j>->prof->$pType<$k>]
                set str "$str [format "%9.2e" $prof]" 
            } else {
                set str "$str  --------" 

            }
         }
         echo $str
         if {$outf != "" } {puts $outf $str}
      }
  }
  genericChainDestroy $chain

  return 

}
####### end of print_comp_prof




# read a list of psField files and plot the power-law index fitted
# to points whose (0-indexed) radial indices are given in list $indices
# psFieldroot is $dir/psField-$runstr-$col-
# profType can be "med" or "mean"
proc plot_beta {device psFieldroot indices {profType "med"} {sf 0} {ef 1000}   \
                  {title ""} {filterlist {u g r i z}} {outfile ""} {psPSF 0}} {

   assert {[llength $indices] > 1}
   assert {[lsearch $indices 0] == -1}
   set all_filters {u g r i z}

   # get the chain of calib1byframe
   set chain [get_cal_chain_from_psField $psFieldroot $sf $ef $psPSF]

   # get some sizes
   set Nfields [chainSize $chain] 
   # get ncolors
   set Nfilters [llength $filterlist]
   set cal [chainElementGetByPos $chain 0]
   set ncolors [exprGet $cal.ncolors]
   handleDel $cal  
   # this really needs to be a check for each and every filter
   assert { $Nfilters <= $ncolors }
   initProfileExtract
   # fake central pixel
   set radii "0.0 [profileRadii]" 

   #### plot ####
   # Set up the plot and label
   set pg [pgstateNew]
   if {$outfile == ""} {
      set outfile corrApCorr.ps
   }
   set deviceA [devicename $device $outfile]
   set nxwindow 1; set nywindow 2
   set xf 0.3; set yf 0.35 
   if {$Nfilters > 2} {
      set nxwindow 2; set nywindow 3
      set xf 0.35; set yf 0.4 
   }
   if {$Nfilters > 4} {
      set nywindow 4; 
      set xf 0.3; set yf 0.4   
   } 
   pgstateSet $pg -device $deviceA -xfract $xf -yfract $yf \
      -nxwindow $nxwindow -nywindow $nywindow -symb 7 -isLine 0   
   pgstateOpen $pg
   pgstateSet $pg -isNewplot 1 -isLine 0 -icMark 2 -symb 4 


   ### loop over bands
   set fields [vFromChain $chain field]
   foreach f $filterlist {
      set j [lsearch $all_filters $f]      
      # get and plot each profile vector
      set beta [vectorExprNew $Nfields]       
      # loop over fields  
      loop i 0 $Nfields {
         set cal [chainElementGetByPos $chain $i]   
         set Nprof [exprGet $cal.calib<$j>->prof->nprof]
         set comp_prof $cal.calib<$j>->prof->$profType          
         handleSet $beta.vec<$i> [fit_beta_from_prof $indices $radii $comp_prof]         
      }
      vNameSet $fields "field"
      set ymax [expr 1.2*[vExtreme $beta max]]
      set ymin [expr 0.8*[vExtreme $beta min]]
      if {$ymin > 2.0} {set ymin 2.0}
      if {$ymax < 4.0} {set ymax 4.0}
      set xmax [vExtreme $fields max]
      set xmin [vExtreme $fields min]
      vPlot $pg $fields $beta -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax
      # title    
      if {$j == 0} {
          set label "$title, beta determined from points ($indices)"           
          topLabel 20 $label
      }
      pgstateSet $pg -isNewplot 1 -isLine 0 -icMark 2 -symb 4 
      vectorExprDel $beta      
  }
       
  endplot $pg
  vectorExprDel $fields
  genericChainDestroy $chain

  return 0

}
####### end of plot_beta #######



# given a set of points whose (0-indexed) radial indices are given in list 
# $indices, the list of radii and a profile, fit a power law to these points
# and return the power-law index
proc fit_beta_from_prof {indices radii profile} { 

    set Npoints [llength $indices]
    assert { $Npoints > 1 }

    if [catch {
          set x [vectorExprNew $Npoints] 
          set y [vectorExprNew $Npoints] 
          loop k 0 $Npoints {
             set index [lindex $indices $k]
             set r [lindex $radii $index]
             set prof [exprGet $profile<$index>]
             handleSet $x.vec<$k> [expr log($r)]
             handleSet $y.vec<$k> [expr log($prof)]          
          } 
          # now call fitter
          set Clist [vFitPolynom $x $y 1]
          set beta [lindex $Clist 1] 
        } msg] { 
        echo "problem fitting beta: $msg"               
        set beta 3.0
    }  

    return [format "%6.3f" [expr -1*$beta]] 
}



# read a psCB file 
proc read_psCB {cbfile {filterlist {u g r i z}} {startframe 0} {endframe 1000}} {

   set hdr [hdrReadAsFits [hdrNew] $cbfile]
   set filters [hdrGetAsAscii $hdr FILTERS]
   set ncolor [llength $filters]
   set field0 [hdrGetAsInt $hdr FIELD0]
   set nfields [hdrGetAsInt $hdr NFIELDS]
   hdrFreeAll $hdr; hdrDel $hdr

   if {$startframe < $field0} {
      echo "read_cframes: $cbfile does not contain info for field $startframe"
      echo "              setting startframe to $field0"
      set startframe $field0
   }
   if { $endframe >= [expr $field0 + $nfields]} {
      echo "read_cframes: $cbfile does not contain info for field $endframe"
      set endframe [expr $field0 + $nfields - 1] 
      echo "              setting endframe to $endframe"
   }

   set temp_list $filterlist
   set filterlist {}
   foreach filter $temp_list {
      if {[set psindex($filter) [lsearch $filters $filter]] == -1} {
         echo "$cbfile does not contain info for filter $filter"
      } else {
         set filterlist [concat $filterlist $filter] 
      }
   }

   echo " This file contains data for filters $filters,"
   echo " for frames $field0-[expr $field0 + $nfields]."

   declare_io_schematrans_for_calib1byframe

   set fd [fitsBinTblOpen $cbfile r]
   set nrow [fitsBinTblHdrRead $fd CALIB_IO]

   set calibio [calibIoNew [llength $filters]]
   set cframes [chainNew CALIB1BYFRAME]
   loop row 0 $nrow {

      fitsBinTblRowSeek $fd $row 0
      fitsBinTblRowRead $fd $calibio 
      set calib1byframe [calib1byframeNewFromCalibIo $calibio $filterlist \
                 psindex]
      chainElementAddByPos $cframes $calib1byframe TAIL AFTER
   }

   calibIoDel $calibio
   fitsBinTblClose $fd
   fitsBinForgetSchemaTrans CALIB_IO
   
   #### print out nice tables ####
   set Nframes [chainSize $cframes]
   echo " "; echo " "
   foreach filter $filterlist {
      echo " === Filter $filter ==="
      echo "      PSP status     n     sky  s_err s_sig   flux20  flux20Err"
      set ifilter [lsearch $filterlist $filter]
      loop i 0 $Nframes {
          set iFr [expr $i + $field0]  
          set cal [chainElementGetByPos $cframes $i]
          set PSPstatus [format "%17s" [exprGet -enum $cal.status]]
          set width [exprGet $cal.calib<$ifilter>->psf->width]
          set sky [exprGet $cal.calib<$ifilter>->sky]
          set skyerr [exprGet $cal.calib<$ifilter>->skyerr]
          set skysig [exprGet $cal.calib<$ifilter>->skysig]
          set flux20 [exprGet $cal.calib<$ifilter>->flux20]
      set flux20Err [exprGet $cal.calib<$ifilter>->flux20Err]
      set string [format "  %3s  %6.2f %5.2f %5.2f  %8s %8s" \
                      $iFr $sky $skyerr $skysig $flux20 $flux20Err]
          if {$iFr >= $startframe && $iFr <= $endframe} {
         echo "$PSPstatus $string"
          }
      }
      echo " "
   }

   return 0
}



proc read_psCT {fitsfile {filterlist {u g r i z}}} {

     set ctimes [read_ctimes $fitsfile {u g r i z}]
     set hdr [hdrReadAsFits [hdrNew] $fitsfile]
     set filters [hdrGetAsAscii $hdr FILTERS]
     hdrDel $hdr
    
     set filter_str "  patch         mjd  "
     foreach f $filterlist {
        set filter_str "$filter_str          $f"
  
     }      
     echo $filter_str
     loop i 0 [chainSize $ctimes] {
     set calib1bytime [chainElementGetByPos $ctimes $i]
         if {[catch {set mjd [exprGet $calib1bytime.mjd]} msg]} {
             echo "failed to read mjd for patch $i"
             echo $msg  
             set mjd -1.0
         }
         set string1 [format " %4d      %13.6f" [expr $i+1] $mjd]
         set string2 [format "                    err:"]
         foreach f $filterlist {  
            set color [lsearch $filters $f]   
        if {$color < 0} {
                error "filter $f doesn't exists in file $fitsfile"
        }   
            if {[catch {set f20($color) [exprGet $calib1bytime.calib<$color>->flux20]} msg]} {
                echo "failed to read f20, color = $color"
                echo $msg  
                set f20($color) -1.0
            }
            if {[catch {set f20Err($color) [exprGet $calib1bytime.calib<$color>->flux20Err]} msg]} {
                echo "failed to read f20Err, color = $color"
                echo $msg  
                set f20Err($color) -1.0
            }
            set string1 "$string1  [format "%9.2f" $f20($color)]"
            set string2 "$string2  [format "%9.2f" $f20Err($color)]"

         }
         echo $string1 
         echo $string2 
      }

}





proc plot_sigmaStokes {device display_clip frame_info starlist run camCol \
			   outputDir nsigma_sigma nsigma_QU nsigma_RMS_rad} {
   global color_index 
 
   set date [ctime]
   set runstr  [format "%06d" $run]

   #
   # are we making hardcopies?
   #
   if {[string compare $device /PS] == 0} {
      set file "yes";           # it'll be replaced by filename below
   } else {
      set file ""
   }


   if [catch {      
      # get filtername and frame number
      set frame [exprGet $frame_info.frame]
      set filter [exprGet $frame_info.filter<0>]
      set nstars_total [chainSize $starlist]
  
      # Set up the plot and label x-axes
      set pg [pgstateNew]
      set device [devicename $device $outputDir/psPlotsQU-$runstr-$camCol-$filter-$frame.ps]
      pgstateSet $pg -device $device \
      -xfract 0.4 -yfract 0.4 -nxwindow 2 -nywindow 1 -symb 7
      pgstateOpen $pg
      
      if [info exists color_index($filter)] {
	 set ci $color_index($filter)
      } else {
	 set ci 1
      }
      pgSci $ci

      titlePlot "PSPipeline: run = $run camCol = $camCol filter = $filter frame = $frame" 20 
      pgSci 1

      # generate chains for various types of stars
      set good_stars [chainNew STAR1]
      set sig1_stars [chainNew STAR1]
      set sig2_stars [chainNew STAR1]
      set b_stars [chainNew STAR1]
      set good_QUstars [chainNew STAR1]
      set QU_stars [chainNew STAR1]
      set allstars [chainNew STAR1] 
      set nstars [exprGet $frame_info.Nallstars]

      set i1 [exprGet $frame_info.firststar]
      set i2 [expr [exprGet $frame_info.firststar] + $nstars]  

      # form chains 
      loop i $i1 $i2 {
          set j [expr $i - $i1]
          set flag [exprGet $frame_info.starflags<$j>]
          set star [chainElementGetByPos $starlist $i]  
          if {$flag == 0} {
              chainElementAddByPos $good_stars $star TAIL AFTER
          }
          if {$flag == 1 || $flag == 3 || $flag == 5 || $flag == 7} {
              chainElementAddByPos $sig1_stars $star TAIL AFTER             
          }
          if {$flag == 2 || $flag == 3 || $flag == 6 || $flag == 7} {
              chainElementAddByPos $sig2_stars $star TAIL AFTER             
          }
          if {$flag == 4 || $flag == 5 || $flag == 6 || $flag == 7} {
              chainElementAddByPos $b_stars $star TAIL AFTER             
          }
          set FbadQU [exprGet $frame_info.FbadQU]
          set FbadCell [exprGet $frame_info.FbadCell]
          if {$flag != $FbadQU && $flag != $FbadCell} {
              chainElementAddByPos $good_QUstars $star TAIL AFTER              
          } 
          if {$flag == $FbadQU} {
              chainElementAddByPos $QU_stars $star TAIL AFTER              
          }  
          # for finding the ranges  
          chainElementAddByPos $allstars $star TAIL AFTER                                
          handleDel $star
      }

      # check that numbers are OK compared to those stored in frame_info 
      set NbadQU [exprGet $frame_info.NbadQU]
      set NbadCell [exprGet $frame_info.NbadCell]
      set NgoodQU [expr $nstars - $NbadCell - $NbadQU]
      set Nhere [chainSize $good_QUstars]
      # assert { $NgoodQU == $Nhere }
      set Ngood [exprGet $frame_info.NgoodBasis]
      set Nhere [chainSize $good_stars]
      #assert { $Ngood == $Nhere }
      set Nstars [exprGet $frame_info.Nallstars]
      set Nhere [chainSize $allstars]
      #assert { $Nstars == $Nhere }
 
      # find the range of b, sigma2, and Q & U
      set b [vFromChain $good_QUstars dgpsf->b]
      set sig2 [vFromChain $good_QUstars dgpsf->sigmax2]
      set Q [vFromChain $allstars Q]
      set U [vFromChain $allstars U]

      set scale 0.1
      extremes $b min_b max_b $scale
      extremes $sig2 min_sig2 max_sig2 $scale
      extremes $Q min_Q max_Q $scale
      extremes $U min_U max_U $scale
      if {$min_U < -1} {set min_U -1.0}
      if {$min_Q < -1} {set min_Q -1.0}
      if {$max_U > 1} {set max_U 1.0}
      if {$max_Q > 1} {set max_Q 1.0}

      vectorExprDel $b    
      vectorExprDel $sig2
      vectorExprDel $Q
      vectorExprDel $U
      chainDel $allstars
 
      set ng [chainSize $good_stars]
      set ngQU [chainSize $good_QUstars]
      set nQU [chainSize $QU_stars]
      set ns1 [chainSize $sig1_stars]
      set ns2 [chainSize $sig2_stars]
      set nb [chainSize $b_stars]
      set nbadchisq 0
      set NbadMod 0
      set nw [expr $ngQU - $nbadchisq - $NbadMod - $ng]
 
      # generate and plot vectors 
      # 1) Q vs. U
        # good stars
      set Q [vFromChain $good_QUstars Q]
      set U [vFromChain $good_QUstars U]
      vNameSet $Q "Q"
      vNameSet $U "U"
      vPlot $pg $Q $U -xmin $min_Q -xmax $max_Q -ymin $min_U -ymax $max_U 
      vectorExprDel $Q
      vectorExprDel $U

      # rejected on QU
      if {$nQU > 0} {
        pgstateSet $pg -isNewplot 0 -symb 4 -icMark 2
        set Q [vFromChain $QU_stars Q]
        set U [vFromChain $QU_stars U]
        vNameSet $Q "Q"
        vNameSet $U "U"
        vPlot $pg $Q $U  
        vectorExprDel $Q
        vectorExprDel $U 
      }
       
        # circle
      set xc [exprGet $frame_info.Qmedian]
      set yc [exprGet $frame_info.Umedian]
      set rad [exprGet $frame_info.dQUmed] 
      set rad [expr $rad + $nsigma_QU * [exprGet $frame_info.dQUsig]]
      pgstateSet $pg -icMark 2
      pgSfs 2
      pgCirc $xc $yc $rad

        # text
      set xtxt $min_Q
      set ytxt [expr $min_U - 0.37 * ($max_U - $min_U)]
      set txt "$ng accepted stars ($nQU rejected on QU, $nw on \
               \\gs\\d1\\u, \\gs\\d2\\u, and b)"
      if [info exists color_index($filter)] {
	 set ci $color_index($filter)
      } else {
	 set ci 1
      }
      pgSci $ci

      pgText $xtxt $ytxt $txt
      pgSci 1

      set ytxt [expr $min_U - 0.45 * ($max_U - $min_U)]
      set txt "$ns1 rejected on \\gs\\d1\\u, $ns2 rejected on \\gs\\d2\\u, \
               $nb rejected on b"
      pgText $xtxt $ytxt $txt

      pgstateSet $pg -isNewplot 1 -symb 7 -icMark 1 

      # 2) b vs. sigma2
        # good stars
      set b [vFromChain $good_stars dgpsf->b]
      set sig2 [vFromChain $good_stars dgpsf->sigmax2]
      vNameSet $b "b (amplitude ratio)"
      vNameSet $sig2 "\\gs\\d2\\u (pixels)"
      vPlot $pg $b $sig2 -xmin $min_b -xmax $max_b -ymin $min_sig2 -ymax $max_sig2 
      vectorExprDel $b
      vectorExprDel $sig2
        # rejected on sigma1
      if {$ns1 > 0} {
      pgstateSet $pg -isNewplot 0 -symb 10 -icMark 3
      set b [vFromChain $sig1_stars dgpsf->b]
      set sig2 [vFromChain $sig1_stars dgpsf->sigmax2]
      vPlot $pg $b $sig2  
      vectorExprDel $b
      vectorExprDel $sig2
      }
        # rejected on sigma2
      if {$ns2 > 0} {
      pgstateSet $pg -isNewplot 0 -symb 3 -icMark 4
      set b [vFromChain $sig2_stars dgpsf->b]
      set sig2 [vFromChain $sig2_stars dgpsf->sigmax2]
      vPlot $pg $b $sig2  
      vectorExprDel $b
      vectorExprDel $sig2
      }
        # rejected on b
      if {$nb > 0} {
      pgstateSet $pg -isNewplot 0 -symb 4 -icMark 2
      set b [vFromChain $b_stars dgpsf->b]
      set sig2 [vFromChain $b_stars dgpsf->sigmax2]
      vPlot $pg $b $sig2  
      vectorExprDel $b
      vectorExprDel $sig2
      }
        # box
      set x0 [exprGet $frame_info.bmed]
      set y0 [exprGet $frame_info.sig2med]
      set s1 [exprGet $frame_info.bsig] 
      set s2 [exprGet $frame_info.sig2sig] 
      set xLL [expr $x0 - $nsigma_sigma * $s1]
      set yLL [expr $y0 - $nsigma_sigma * $s2]
      set xUR [expr $x0 + $nsigma_sigma * $s1]
      set yUR [expr $y0 + $nsigma_sigma * $s2]
      pgstateSet $pg -icBox 4
      pgRect $xLL $xUR $yLL $yUR
      pgstateSet $pg -icBox 1
 
      pgstateSet $pg -isNewplot 1 -symb 7 -icMark 1
 
      } msg] {
     echo "filter $filter: $msg"
   } 

   # cleanup 
   chainDel $good_stars      
   chainDel $sig1_stars     
   chainDel $sig2_stars  
   chainDel $b_stars  
   chainDel $good_QUstars    
   chainDel $QU_stars 
 
   endplot $pg
}


# determine range of a vector; if only a single point make it equal to scale 
proc extremes {vector vec_min vec_max scale} {
  upvar $vec_min min
  upvar $vec_max max
 
  set scale [expr $scale / 2.0]
  set min [vExtreme $vector min]
  set max [vExtreme $vector max]
  if {$max == "inf"} {
      set max $min 
  }
  if {$min >= $max} {
      set max [expr $min + $scale * abs($min)]
      set min [expr $min - $scale * abs($min)]
  }
} 


proc plot_rejection {device startframe endframe outputDir run col filterlist \
			 {_frameinfolist ""} {rerun ""}} {
   if { $_frameinfolist != ""} {
      upvar $_frameinfolist frameinfolist
   }
 
   set runstr  [format "%06d" $run]
   set nfilters [llength $filterlist]

   set date [getdate]
   set runstr [format %06d $run]

   # For each color:
   loop i 0 $nfilters {

      # get filter name
      set filter [lindex $filterlist $i]

      if [catch {

      # get vectors for plotting
      set frames [vFromChain $frameinfolist($filter) frame] 
      set Nstars [vFromChain $frameinfolist($filter) Nallstars] 
      set Ngood [vFromChain $frameinfolist($filter) NgoodBasis] 
      set NbadSSb [vFromChain $frameinfolist($filter) NbadSSb] 
      set NbadCell [vFromChain $frameinfolist($filter) NbadCell] 
      set NbadQU [vFromChain $frameinfolist($filter) NbadQU] 
      set NsizeShape [vectorExprEval $NbadSSb+$NbadQU]
      set pGood [vectorExprEval 100*$Ngood/$Nstars]
      set pBadSSb [vectorExprEval 100*$NbadSSb/$Nstars]
      set pBadCell [vectorExprEval 100*$NbadCell/$Nstars]
      set pBadQU [vectorExprEval 100*$NbadQU/$Nstars]
      set psizeShape [vectorExprEval 100*$NsizeShape/$Nstars]

      set min 0
      set max [vExtreme $Nstars max]
      #set startframe [expr int([vExtreme $frames min])]
      #set endframe [expr int([vExtreme $frames max])]

     
      # Set up the plot and label x-axes
      set pg [pgstateNew]
      set device [devicename $device $outputDir/psPlotsRejecStats-$runstr-$filter$col.ps]
      pgstateSet $pg -device $device -lineWidth 3 \
      -xfract 0.4 -yfract 0.4 -nxwindow 2 -nywindow 1 -symb 7
      pgstateOpen $pg
      
      topLabel 40 "PSP, frames: $startframe-$endframe" \
	  $run $col $date $filter $rerun

      ### Plot N of stars ###
      # Plot Nstars
      vNameSet $Nstars "N stars"
      pgstateSet $pg -isLine 1 
      vPlot $pg $frames $Nstars -xmin $startframe -xmax $endframe \
         -ymin $min -ymax $max 
      # Plot Ngood
      pgstateSet $pg -icLine 2 -isNewplot 0  
      vPlot $pg $frames $Ngood -xmin $startframe -xmax $endframe \
         -ymin $min -ymax $max 
      # Plot NbadSSb
      #pgstateSet $pg -icLine 4 -isNewplot 0 -lineStyle 4 -lineWidth 1
      #vPlot $pg $frames $NbadSSb -xmin $startframe -xmax $endframe \
      #     -ymin $min -ymax $max 
      # Plot NbadCell
      pgstateSet $pg -icLine 3 -isNewplot 0 -lineStyle 2
      vPlot $pg $frames $NbadCell -xmin $startframe -xmax $endframe \
         -ymin $min -ymax $max 
      # Plot NbadQU 
      #pgstateSet $pg -icLine 5 -isNewplot 0 -lineStyle 4
      #vPlot $pg $frames $NbadQU -xmin $startframe -xmax $endframe \
      #      -ymin $min -ymax $max 
      # Plot NsizeShape 
      pgstateSet $pg -icLine 5 -isNewplot 0 -lineStyle 4
      vPlot $pg $frames $NsizeShape -xmin $startframe -xmax $endframe \
             -ymin $min -ymax $max 

      # text
      set xtxt $startframe
      set ytxt [expr -3-$max/3.5] 
      set txt "white: all, red: good, green dashed: cell, cyan dot-dashed: size & QU rej. "
      pgText $xtxt $ytxt $txt
   

  
      ### Plot perc. of stars ###
      set max 100
      vNameSet $pGood "% of stars"
      # Plot pGood
      pgstateSet $pg -icLine 2 -isNewplot 1 -lineStyle 1  
      vPlot $pg $frames $pGood -xmin $startframe -xmax $endframe \
         -ymin $min -ymax $max 
      # Plot pBadSSb
      #pgstateSet $pg -icLine 4 -isNewplot 0 -lineStyle 4 -lineWidth 1
      #vPlot $pg $frames $pBadSSb -xmin $startframe -xmax $endframe \
      #     -ymin $min -ymax $max 
      # Plot pBadCell
      pgstateSet $pg -icLine 3 -isNewplot 0 -lineStyle 2
      vPlot $pg $frames $pBadCell -xmin $startframe -xmax $endframe \
         -ymin $min -ymax $max 
      # Plot pBadQU 
      #pgstateSet $pg -icLine 4 -isNewplot 0 -lineStyle 4
      #vPlot $pg $frames $pBadQU -xmin $startframe -xmax $endframe \
      #     -ymin $min -ymax $max 
      # Plot pBadQU 
      pgstateSet $pg -icLine 5 -isNewplot 0 -lineStyle 4
      vPlot $pg $frames $psizeShape -xmin $startframe -xmax $endframe \
           -ymin $min -ymax $max 


   
      # clean vectors for this filter
      vectorExprDel $Nstars
      vectorExprDel $Ngood
      vectorExprDel $NbadSSb
      vectorExprDel $NbadCell
      vectorExprDel $NbadQU
      vectorExprDel $NsizeShape
      vectorExprDel $frames
      vectorExprDel $pGood  
      vectorExprDel $pBadSSb
      vectorExprDel $pBadCell
      vectorExprDel $pBadQU
      vectorExprDel $psizeShape
  
      endplot $pg

      } msg] {
     echo "filter $filter: $msg"
      }
   }
  
}

# plot cell profiles for all and selected stars in two panels, with 
# various kinds of average profiles also marked 
proc plot_profiles {device starlist frame_info run camCol outputDir} {
   global color_index 
 
   set date [ctime]
   set runstr  [format "%06d" $run]
   set frame [exprGet $frame_info.frame]
   set filter [exprGet $frame_info.filter<0>]
   set nstars_total [chainSize $starlist]
   # profile to plot (1: mean, 2: median)
   set prof_type 1

   if [catch {      

      # Set up the plot and label x-axes
      set pg [pgstateNew]
      set device [devicename $device $outputDir/psPlotsCellProf-$runstr-$camCol-$filter-$frame.ps]
      pgstateSet $pg -device $device \
      -xfract 0.25 -yfract 0.2 -nxwindow 2 -nywindow 1 -symb 1
      pgstateOpen $pg
      
      if [info exists color_index($filter)] {
	 set ci $color_index($filter)
      } else {
	 set ci 1
      }
      pgSci $ci

      titlePlot "PSPipeline: run = $run camCol = $camCol filter = $filter frame = $frame" 20 
      pgSci 1

      # generate chains for various types of stars
      set good_stars [chainNew STAR1]
      set allstars [chainNew STAR1] 

      set nstars [exprGet $frame_info.Nallstars]

      set i1 [exprGet $frame_info.firststar]
      set i2 [expr [exprGet $frame_info.firststar] + $nstars]  

      # form chains 
      loop i $i1 $i2 {
          set j [expr $i - $i1]
          set flag [exprGet $frame_info.starflags<$j>]
          set badstar($j) $flag
          set oddness($j) 0
          set maxchi2($j) 0
          set goodflags($j) 0
          set star [chainElementGetByPos $starlist $i] 
          # good stars 
          if {$flag == 0 && [exprGet $star.frame] == [exprGet $frame_info.frame] \
                           && [exprGet $star.PSFflag] == 1} {
              chainElementAddByPos $good_stars $star TAIL AFTER
          } 
          # all stars
          chainElementAddByPos $allstars $star TAIL AFTER                                
          handleDel $star
      }

      # check that numbers are OK compared to those stored in frame_info 
      set Ngood [exprGet $frame_info.NgoodBasis]
      set Nhere [chainSize $good_stars]
      set Nstars [exprGet $frame_info.Nallstars]
      set Nhere [chainSize $allstars]

      # plot profiles for all stars
      set min -5
      set max 5
      set min -4
      set max 1
      loop i 0 $Nstars {
          set star [chainElementGetByPos $allstars $i] 
          set Npts [exprGet $star.cprof->ncell] 
          set cells [vIndex $Npts] 
          set profile [cellprof $star 1 2]
          set errors [cellprof $star 2 2]
          set flag [exprGet $frame_info.starflags<$i>]
          vNameSet $cells "cell #"
          vNameSet $profile "log(normalized cell profile)"
          vPlot $pg $cells $profile -vectorYErr $errors -xmin -3 -xmax 45 -ymin $min -ymax $max 
          pgstateSet $pg -isNewplot 0 -symb 5 -icMark 3
          pgstateSet $pg -isNewplot 0 -symb 1 -icMark 1    
          vectorExprDel $cells
          vectorExprDel $profile
          vectorExprDel $errors
      }

      ### average profile
      set cprof_bar [cellprofNew]
      getCprof $allstars 0 [expr $Nstars - 1] badstar $prof_type 2 $cprof_bar oddness maxchi2
      set Npts [exprGet $cprof_bar.ncell] 
      set cells [vIndex $Npts] 
      # profile
      set aver_prof [cellprof2vec $cprof_bar 1]
      pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineWidth 3
      vPlot $pg $cells $aver_prof -xmin -3 -xmax 45 -ymin $min -ymax $max  
      vectorExprDel $aver_prof 
      # profile + errors
      set aver_prof [cellprof2vec $cprof_bar 3]
      pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 4 -lineStyle 1 -lineWidth 1
      vPlot $pg $cells $aver_prof -xmin -3 -xmax 45 -ymin $min -ymax $max  
      vectorExprDel $aver_prof    
      # profile - errors
      set aver_prof [cellprof2vec $cprof_bar 4]
      pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 4 -lineStyle 1 -lineWidth 1
      vPlot $pg $cells $aver_prof -xmin -3 -xmax 45 -ymin $min -ymax $max  
      vectorExprDel $aver_prof 
      vectorExprDel $cells
      cellprofDel $cprof_bar    


      # text
      set xtxt 0
      set ytxt $max 
      set txt "Profiles for all stars (N = $Nstars)"
      if [info exists color_index($filter)] {
	 set ci $color_index($filter)
      } else {
	 set ci 1
      }
      pgSci $ci

      pgText $xtxt $ytxt $txt
      pgSci 1


      pgstateSet $pg -isNewplot 1 -isLine 0 -symb 20 -icMark 2 -lineStyle 1
      # plot profiles for all stars
      loop i 0 $Nstars {
          set star [chainElementGetByPos $allstars $i] 
          set Npts [exprGet $star.cprof->ncell] 
          set cells [vIndex $Npts] 
          set profile [cellprof $star 1 2]
          set errors [cellprof $star 2 2]
          set flag [exprGet $frame_info.starflags<$i>]
          vNameSet $cells "cell #"
          vNameSet $profile "log(normalized cell profile)"
          vPlot $pg $cells $profile -vectorYErr $errors -xmin -3 -xmax 45 -ymin $min -ymax $max 
          pgstateSet $pg -isNewplot 0 -symb 1 -icMark 1    
          vectorExprDel $cells
          vectorExprDel $profile
          vectorExprDel $errors
      }
      # overplot profiles for good stars
      loop i 0 [chainSize $good_stars]  {
          set star [chainElementGetByPos $good_stars $i] 
          set Npts [exprGet $star.cprof->ncell] 
          set cells [vIndex $Npts] 
          set profile [cellprof $star 1 2]
          set errors [cellprof $star 2 2]
          vNameSet $cells ""
          vNameSet $profile ""
          vPlot $pg $cells $profile -vectorYErr $errors -xmin -3 -xmax 45 -ymin $min -ymax $max
          pgstateSet $pg -isNewplot 0 -symb 5 -icMark 4
          pgstateSet $pg -isNewplot 0 -symb 20 -icMark 2
          vectorExprDel $cells
          vectorExprDel $profile
          vectorExprDel $errors
      }

      ### average profile
      set cprof_bar [cellprofNew]
      getCprof $allstars 0 [expr $Nstars - 1] badstar $prof_type 2 $cprof_bar oddness maxchi2
      set Npts [exprGet $cprof_bar.ncell] 
      set cells [vIndex $Npts] 
      # profile
      set aver_prof [cellprof2vec $cprof_bar 1]
      pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3 -lineWidth 3
      vPlot $pg $cells $aver_prof -xmin -3 -xmax 45 -ymin $min -ymax $max  
      vectorExprDel $aver_prof 
      # profile + errors
      set aver_prof [cellprof2vec $cprof_bar 3]
      pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 4 -lineStyle 1 -lineWidth 1
      vPlot $pg $cells $aver_prof -xmin -3 -xmax 45 -ymin $min -ymax $max  
      vectorExprDel $aver_prof    
      # profile - errors
      set aver_prof [cellprof2vec $cprof_bar 4]
      pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 4 -lineStyle 1 -lineWidth 1
      vPlot $pg $cells $aver_prof -xmin -3 -xmax 45 -ymin $min -ymax $max  
      vectorExprDel $aver_prof 
      vectorExprDel $cells
      cellprofDel $cprof_bar    


      # text
      set xtxt 0
      set ytxt $max 
      set txt "Profiles for $Ngood good stars"
      if [info exists color_index($filter)] {
	 set ci $color_index($filter)
      } else {
	 set ci 1
      }
      pgSci $ci

      pgText $xtxt $ytxt $txt
      pgSci 1

      chainDel $allstars
      chainDel $good_stars 

      endplot $pg 

      } msg] {
     echo "filter $filter: $msg"
   } 

}

# return a vector with N elements with the values equal to index 
proc vIndex {N} {
   return [vectorExprEval (0,($N-1))]
}

 
# return a vector with either the data (type=1) or the errors (type=2)
# from the CELL_PROF of a given STAR1 (use log10)
# for scaling == 1 scale by the central pixel intensity
# for scaling == 2 scale by the average of first 7 pixels 
# for scaling == 3 scale by the aperture flux
proc cellprof {star type scaling} {

   set N [exprGet $star.cprof->ncell] 
   set vector [vectorExprNew $N]

   # get normalization scale
   if {$scaling == 1} {
       scale =  [exprGet $star.cprof->data<0>] 
   }
   if {$scaling == 2} {
       set scale 0
       loop i 0 7 {     
          set scale [expr $scale + [exprGet $star.cprof->data<$i>]] 
       }
       set scale [expr $scale/7.0]
   }
   if {$scaling == 3} {
       scale =  [exprGet $star.apCounts] 
   }

   set ID 0
   loop i 0 $N {     
       set value [exprGet $star.cprof->data<$i>]
       if { $value == 0 } { 
            set value 1.0e-10 
       }     
       if { $value <= 0 && $type == 3 } {
          if { $ID == 0 } {
              echo " *** negative values *** "
              echo " starID: [exprGet $star.id], Ncell: [exprGet $star.cprof->ncell]" 
              set ID 1
          }
          set scaled_value [expr $value/$scale]
          echo "      i: $i, value: $value, error: [exprGet $star.cprof->sig<$i>] scaled value: $scaled_value" 
       } 
       if {$type  == 1} {
           set value [expr $value/$scale]
       } else {
           set err [exprGet $star.cprof->sig<$i>]
           if { $err <= 0 } {
              echo " === negative errors === "
              if { $ID == 0} {
                  echo " === negative errors === "
                  echo "     starID: [exprGet $star.id], Ncell: [exprGet $star.cprof->ncell]" 
                  set ID 1
                  echo "     i: $i, error: $err" 
              } else {
                  echo " negative error for i = $i: $err" 
              }
           } 
           set value [expr 100*$err/$value] 
       }
       if {$value <= 0} {
          set value 1.0e-10
       }
       set value [expr log10($value)]
       handleSet $vector.vec<$i> $value
   }   

   if { $ID == 0  && $type == 3} {
      echo "*********** STAR [exprGet $star.id]: NO negative values!**********" 
   }
   return $vector
}



# return a vector with either the data (type=1), errors (type=2)
# data + errors (type=3), data - errors (type=4) (use log10)
# from the given CELL_PROF 
proc cellprof2vec {cprof type} {

   # total number of points
   set N [exprGet $cprof.ncell] 
   set vector [vectorExprNew $N]

   loop i 0 $N {    
       set data [exprGet $cprof.data<$i>]
       set sig [exprGet $cprof.sig<$i>]
       if {$type  == 1} {
           set value $data 
       }  
       if {$type  == 2} {
           set value $sig 
       }  
       if {$type  == 3} {
           set value [expr $data + $sig ]
       } 
       if {$type  == 4} {
           set value [expr $data - $sig ]
       } 
       if { $value <= 0 } { 
            set value 1.0e-10 
       }     
       set value [expr log10($value)]
       handleSet $vector.vec<$i> $value
   }   

   return $vector
}


#####################################################################
# these procs (show_frame & show_obj) are helpers for a set of 
# robert's tools to compare SPICAM and HST data

proc show_frame { frame filter } {

     set frame_string [format "%04d" $frame]
     set camCol 1
     set chip_string [format "%s%1d" $filter $camCol]
     set reg [regReadAsFits [regNew] /u/rhl/photodata/groth/output/fpC-050600-$chip_string-$frame_string.fit]    
     mtv $reg;   
     show_recon $frame $filter;      #show groth field with OBJC numbers
}


proc show_obj { frame object } {

     set table [openit $frame];      #opens a table for groth frame
     p_objc table $object;           #prints info from OBJC for object above
     mtv_blend table $object;        #shows atlas image for object 
     # mtv_groth shows HST atlas image for object above
     if [catch {
           mtv_groth   
     } msg] {
#    echo "$msg"
     }
}



# return canonical gain for a given chip
proc JEGgain {column filter} {

     #Column 1
       set gains(1,r)   4.23
       set gains(1,i)   5.08
       set gains(1,u)   1.60
       set gains(1,z)   4.85
       set gains(1,g)   3.06
     #Column 2
       set gains(2,r)   4.74
       set gains(2,i)   4.97
       set gains(2,u)   1.72
       set gains(2,z)   5.12
       set gains(2,g)   3.69
     #Column 3
       set gains(3,r)   4.29
       set gains(3,i)   5.15
       set gains(3,u)   1.64
       set gains(3,z)   5.00
       set gains(3,g)   4.05
     #Column 4
       set gains(4,r)   5.17
       set gains(4,i)   4.77
       set gains(4,u)   1.48
       set gains(4,z)   4.78
       set gains(4,g)   3.94
     #Column 5
       set gains(5,r)   4.27
       set gains(5,i)   4.50
       set gains(5,u)   1.43
       set gains(5,z)   3.61
       set gains(5,g)   4.32
     #Column 6
       set gains(6,r)   4.89
       set gains(6,i)   5.30
       set gains(6,u)   2.04
       set gains(6,z)   4.74
       set gains(6,g)   4.01

     return $gains($column,$filter)
} 


 
##############################################################################
#
#Procedure read_allSky.tcl takes as input parameters and one quantity and plots 
#it things across the fields in the separate filters.
#
#pq, physical quantity, may be anything read_cframes allows, for example sky,
#skysig, skyerr, skyslope, lbias, rbias, flux20, &c.
#
#Usage: read_allSky inputDir device pq col startframe endframe filterlist mjd run  
#
#General Notes: 
#       -Does not assume each psCB file contains all filters or all fields
#       -Fields and frames are confusingly interchanged, but mean the same thing.
#       -The bottom-right chart is created by makeMyBox with respect to the 
#        top-left one, so that it will appear even if only one filter is available.
#
#Parameter Notes:
#       -To use Robert's /u/dss/data/mjd/run/output/column directories, as inputDir,
#        enter "system" Otherwise, note that each psCB file must be in a directory 
#        named after its column.  Example:  psCB-000075-1.fit must be in $inputDir/1/
#       -device: /XWINDOW or /PS or /cps (for color postscript)
#       -cols: a TCL list of all the desired columns.  To plot all the columns for
#        which data exists, enter 7 as this list.
#       -To plot sky and sky +/- error in counts, enter skyanderr as the physical
#        quantity (pq).
#       -To plot sky and sky +/- error in magnitudes, enter skymaganderr as pq
#       -To plot sky in magnitudes without error, enter skymag as pq
#       -To plot quantities defined in the psf structure, enter psf-> followed by that
#        quantity.  Example:  To plot PSF FWHM enter psf->width as the pq.
#       -xlabel, ylabel, and plotlabel are strings which name the axes and the plots
#       -wantedfilters: a TCL list of all the desired filters.  To plot all filters, 
#        enter "all" for the wantedfilters.  Note that if all filters are not plotted,
#        there will be no chart.
#       -mjd and run are necessary if Robert's data directory tree is used, but only 
#        run is necessary if the data is from some other directory
#
#
#
# ZI: changed to operate on psField files (instead of a single psCB file)
#
#
#############################################################################  

proc read_allSky {{inputDir system} {dirtype none} {deviceA /XWINDOW} \
		      {pqs skymag} {cols "all"} {startframe 0} {endframe 10000}\
		      {xlabel field} {ylabels mag/arcsec2} \
		      {plotlabels "Sky Brightness"}  \
		      {run -1} {wantedfilters all} {rerun -1}} {
   global data_root
   global QAscore use_psPSF PSP_STATUS

   set photoVer [photoVersion]
   
   if {![info exists use_psPSF]} {set use_psPSF 0}

   if {$use_psPSF} {
      echo "   *** using psPSF files! ***"
   }

   if {$cols == "all"} {set cols {1 2 3 4 5 6}}
   if {[string compare $inputDir system]==0} {
      set inputDir $data_root/$run/objcs
   }

   set runstr  [format "%06d" $run]
   if {$wantedfilters == "all"} {
      set wantedfilters {u g r i z}
   }
   #Initialize filterIndex array: will be used to tell whether info exists
   foreach col $cols {
      foreach wantedfilter $wantedfilters {
	 set filterIndex($col,$wantedfilter) -1
      }
   }

   # canonical gains from JEG
   loop c 1 7 {
      foreach f {u g r i z} {
	 set gains($c,$f) [JEGgain $c $f]
      }
   } 

   ######## big loop over quantities to plot #######
   foreach pq $pqs {

      echo " == WORKING on $pq =="

      set iq [lsearch $pqs $pq]
      set ylabel [lindex [lindex $ylabels $iq] 0]
      set plotlabel [lindex [lindex $plotlabels $iq] 0]
      if {$use_psPSF && $pq == "skymag"} {
	 echo "    ### Can't plot skymag with psPSF files - returning. ###"
	 return
      }


      #PART I: Find out basic information about each column; create cframes
      foreach col $cols {

	 #Sanity Check: is data available?
	 if {$dirtype == "C"} {
	    set inDir($col) $inputDir/$col
	 } else {
	    set inDir($col) $inputDir
	 }


	 if {!$use_psPSF} {
	    if [catch {set file_list [glob $inDir($col)/psField*.fit]}] {
	       set file_list {}
	    }
	 } else {
	    if [catch {set file_list [glob $inDir($col)/psPSF*.fit]}] {
	       set file_list {}
	    }
	    
	 }

	 set field0 10000
	 set fieldN -1
	 foreach file $file_list {
            set N [string length $file]
            set field [string range $file [expr $N - 8] [expr $N - 5]]
            while {[string range $field 0 0] == 0} {
	       set field [string range $field 1 \
			      [expr [string length $field] - 1]]
            }
            if [catch { set aux [expr 1 + $field] }] {
               continue
            }
            if {$field < $field0} {
	       set field0 $field
	       set file0($col) $file
            }
            if {$field > $fieldN} {
	       set fieldN $field
	       set fileN($col) $file
            }

	 }

	 if {$fieldN < $field0} {
            echo "     NO DATA FOR COLUMN $col, field = $fieldN  is less that starting field = $field0"
            continue
	 }



	 #Create filterIndex
	 set hdr [hdrReadAsFits [hdrNew] $file0($col)]
	 set filefilters($col) [string trim [hdrGetAsAscii $hdr FILTERS]]
	 set nfilefields($col) [hdrGetAsInt $hdr NFIELDS]
	 set filefield0($col) [hdrGetAsInt $hdr FIELD0]
	 hdrDel $hdr
	 
	 # the above is not guaranteed any more...
	 set filefield0($col) $field0
	 set nfilefields($col) [expr $fieldN - $field0 + 1]
	 
	 foreach wantedfilter $wantedfilters {
	    if {[set filterIndex($col,$wantedfilter) \
		     [lsearch $filefilters($col) $wantedfilter]] == -1} {
	       puts "We don't have filter $wantedfilter in column $col"
	    } else {
	       set skipfilter($wantedfilter) 0
            }
	 }
	 #
	 # (1) If user wants frames we don't have, restrict the
	 # boundaries and inform.
	 #
	 # (2) Create the cframes($col) vector.  If user's frame boundaries
	 # are inside ours, drop some of its frames.  If they're the same,
	 # don't change a thing.
	 #
	 set temp_first_frame $filefield0($col)
	 set temp_last_frame [expr $filefield0($col) + $nfilefields($col) -1]
	 set startframeByCol($col) $temp_first_frame
	 set endframeByCol($col) $temp_last_frame
	 #(1)
	 if {$startframe < $temp_first_frame} {
	    if {$startframe > 0} {
	       echo "Frames $startframe - [expr $temp_first_frame - 1] in column"\
		   "$col not available.  We'll start at $temp_first_frame."
	    }
	    set startframeByCol($col) $temp_first_frame
	 } else {
	    set startframeByCol($col) $startframe
	 }
	 if {$endframe > $temp_last_frame} {
	    if {$endframe < 9999} {
	       echo "Frames [expr $temp_last_frame + 1] - $endframe in column " \
		   "$col not available.  We'll end at $temp_last_frame."
	    }
	    set endframeByCol($col) $temp_last_frame
	 } else {
	    set endframeByCol($col) $endframe
	 }

	 echo "WORKING on col $col, getting data for fields "\
	     "$startframeByCol($col) - $endframeByCol($col) from $inDir($col)"
	 
	 #(2)
	 set runstr [format "%06d" $run]
	 if {$use_psPSF} {
            set psFieldroot $inDir($col)/psPSF-$runstr-$col-
	 } else {
            set psFieldroot $inDir($col)/psField-$runstr-$col-
	 }

	 #################################
	 ##### READING psField FILES #####
	 if {$pq == [lindex $pqs 0]} {           
	    set cframes($col) \
		[get_cal_chain_from_psField $psFieldroot \
		     $startframeByCol($col) $endframeByCol($col) $use_psPSF] 
	    # get photo version
	    set fieldstr [format %04d $startframeByCol($col)]
	    set psField ${psFieldroot}$fieldstr.fit
	    set hdr [hdrReadAsFits [hdrNew] $psField]
	    set photoVer [hdrGetAsAscii $hdr VERSION]
	    hdrDel $hdr       
	 }        
	 #################################

	 set frameIndexByCol($col) [vFromChain $cframes($col) field]
	 vNameSet $frameIndexByCol($col) ""
	 #frameIndexByCol will serve as an array of x-vectors for the plots,
	 #in case different columns have differing amounts of data
      }
      foreach wantedfilter $wantedfilters {
	 if {![info exists skipfilter($wantedfilter)]} {
	    set skipfilter($wantedfilter) 1
            #echo Skipping $wantedfilter completely
	 }
      }  

      # now that we have all start- and end-frames,
      # find out x boundaries by filter
      foreach filter $wantedfilters {
	 if {$skipfilter($filter)} {continue}
	 foreach col $cols {
	    if {$filterIndex($col,$filter) == -1} {continue}
	    lappend startframeList($filter) $startframeByCol($col)
	    lappend endframeList($filter) $endframeByCol($col)
	 }
	 set startframeVec($filter) \
	     [vectorExprEval "{$startframeList($filter)}"]
	 set endframeVec($filter) [vectorExprEval "{$endframeList($filter)}"]
	 set startframeArray($filter) [vExtreme $startframeVec($filter) min]
	 set endframeArray($filter) [vExtreme $endframeVec($filter) max]
	 
      }
      foreach thing {startframeList endframeList startframeVec endframeVec} {
	 catch {unset $thing}
      }

      # PART II: Read data into pqvec vector-array,
      # and find pq-min and -max for each filter
      foreach filter $wantedfilters {
	 if {$skipfilter($filter)} {continue}
	 set pqminList($filter) {} 
	 set pqmaxList($filter) {} 
	 foreach col $cols {
	    if {![info exists cframes($col)] ||
		$filterIndex($col,$filter) == -1} {
	       continue
	    }
	    set pos $filterIndex($col,$filter)
	    # filterIndex tells where data on that filter is in the
	    # filefilters($col) list, which is where it will be in this
	    # calib array thing
	    #
	    # Load the pq into my 2x2 "array", with special case of skymag.
	    if {$pq =="skymag" || $pq == "skymaganderr" || $pq =="skycol"} {
	       if {$pq == "skymaganderr"} {
		  set sky [vFromChain $cframes($col) calib<$pos>->sky]
		  set skyerr [vFromChain $cframes($col) calib<$pos>->skyerr]
		  set flux20 [vFromChain $cframes($col) calib<$pos>->flux20]
		  set skyPlusErr [vectorExprEval $sky+$skyerr]
		  set skyMinusErr [vectorExprEval $sky-$skyerr]
		  foreach v "$sky $skyPlusErr $skyMinusErr" {
		     set q1 [vectorExprEval ln((6.25*$v)/$flux20)/ln(10)]
		     vectorExprSet $v 20-(2.5*$q1)
		  }
		  set pqvec($col,$filter) $sky
		  set pqvecMinusErr($col,$filter) $skyMinusErr
		  set pqvecPlusErr($col,$filter) $skyPlusErr
	       } else {
		  set sky [vFromChain $cframes($col) calib<$pos>->sky]
		  set flux20 [vFromChain $cframes($col) calib<$pos>->flux20]
		  set q1 [vectorExprEval ln((6.25*$sky)/$flux20)/ln(10)]
		  set pqvec($col,$filter) [vectorExprEval 20-(2.5*$q1)]
	       }
	       #foreach vec "$sky $flux20 $q1" {vectorExprDel $vec} 
	    } elseif {$pq == "skyanderr"} {
	       set sky [vFromChain $cframes($col) calib<$pos>->sky]
	       set skyerr [vFromChain $cframes($col) calib<$pos>->skyerr]
	       set pqvec($col,$filter) $sky
	       set pqvecMinusErr($col,$filter) [vectorExprEval $sky-$skyerr]
	       set pqvecPlusErr($col,$filter) [vectorExprEval $sky+$skyerr]
	    } elseif {$pq == "status"} {
	       set pqvec($col,$filter) [cframes2status $cframes($col) $pos]
	       vectorExprSet $pqvec($col,$filter) \
               "$pqvec($col,$filter) == $PSP_STATUS(EXTENDED_KL) ? 0.5 : $pqvec($col,$filter)"
	       vectorExprSet $pqvec($col,$filter) \
               "$pqvec($col,$filter) >= $PSP_STATUS(SPARSE) ? -0.5 : $pqvec($col,$filter)"
	    } elseif {$pq == "KolmExp" || $pq == "deriv"} {
	       set pqvec($col,$filter) \
		   [vFromChain $cframes($col) calib<$pos>->psf->width]
	    } elseif {$pq == "beta_prof"} {
	       set pqvec($col,$filter) [cframes2beta $cframes($col) $pos]
	    } else {
	       # added by ZI
	       if {$pq == "Egain" || $pq == "EgainN"} {
		  set sky [vFromChain $cframes($col) calib<$pos>->sky]
		  set skysig [vFromChain $cframes($col) calib<$pos>->skysig]
		  set dv [vFromChain $cframes($col) calib<$pos>->dark_variance]
		  # this is where the noise is calculated
		  set pqvec($col,$filter) \
		      [vectorExprEval sqrt(($skysig*$skysig-$dv)/$sky)]
		  if {$pq == "EgainN"} {
		     set gain $gains($col,$filter)
		     # normalized noise
		     set pqvec($col,$filter) \
			 [vectorExprEval $pqvec($col,$filter)*sqrt($gain)] 
		     # median smooth over 3 frames
		     # (i.e. get rid of 1 point transients)
		     set N [chainSize $cframes($col)]
		     vMedianSmooth $pqvec($col,$filter) $N 3
		  }
	       } else {
		  if {$pq != "QAtables"} {
		     set pqvec($col,$filter) \
			 [vFromChain $cframes($col) calib<$pos>->$pq]
		     if {$pq == "skyslope"} {
			set pqvec($col,$filter) \
			    [vectorExprEval 1000*$pqvec($col,$filter)] 
		     }
		  } else {
		     set pqvec($col,$filter) [vectorExprNew 0]
		  }
	       }
	    }
	    #Maintain a running list of each maximum and minimum in each pqvec
	    extremes $pqvec($col,$filter) newmin newmax 1
	    lappend pqminList($filter) $newmin
	    if {$pq == "EgainN"} {
	       lappend pqmaxList($filter) 2
	    } else {
	       lappend pqmaxList($filter) $newmax
	    }
	    vNameSet $pqvec($col,$filter) ""
	 }

	 # Find the global max and min over the present filter,
	 # and load into array
	 set temp_pqminVec [vectorExprEval "{$pqminList($filter)}"]
	 set temp_pqmaxVec [vectorExprEval "{$pqmaxList($filter)}"]
	 # if there is only one column, then the "extremes" algorithm
	 # confuses us
	 if {[llength $cols] > 1} {
	    extremes $temp_pqminVec pqminArray($filter) junk 1
	    extremes $temp_pqmaxVec junk pqmaxArray($filter) 1
	 } else {
	    set pqminArray($filter) [vExtreme $temp_pqminVec min]
	    set pqmaxArray($filter) [vExtreme $temp_pqmaxVec max]
	    if {$pqmaxArray($filter) == "inf"} {
	       set pqmaxArray($filter) $pqminArray($filter)
	    }
	 }
	 #if in magnitudes, the y-axis should be switched and rounded to .1
	 if {$pq == "skymag" || $pq == "skymaganderr" || $pq =="skycol"} {
            set tempmin $pqminArray($filter)
            set pqminArray($filter) \
		[expr (1+int (10*$pqmaxArray($filter)))/10.]  ;#rounds up
            set pqmaxArray($filter) \
		[expr (int (10*$tempmin))/10.]           ;#rounds down
	 }
	 # if ap.Corr err force a constant range to ease the comparisons
	 if {$pq == "psf_ap_correctionErr" || $pq == "ap_corr_runErr"} {
            set pqminArray($filter) 0.0
            set pqmaxArray($filter) 0.15
	 }              
	 # ditto if ap.Corr bias
	 if {$pq == "ap_corr_run"} {
            set pqminArray($filter) -0.1
            set pqmaxArray($filter)  0.1
	 }              
	 # same for PSP status
	 if {$pq == "status"} {
            set pqminArray($filter) -1
            set pqmaxArray($filter) 6
	 }              
      }
      
      ### special case: sky colors, assumes all 5 filters are requested
      if {$pq =="skycol"} {
	 # matching filters
	 set mf1(u) u; set mf2(u) g;
	 set mf1(g) g; set mf2(g) r;
	 set mf1(i) r; set mf2(i) i;
	 set mf1(z) r; set mf2(z) z;
	 # chip to chip offsets
	 set off(u) 4; set off(g) 8; set off(r) 0; set off(i) 2; set off(z) 6;
	 # title labels
	 set tlab(u) "sky color u-g"
	 set tlab(g) "sky color g-r"
	 set tlab(r) "r sky brightness (mag/arcsec2)"
	 set tlab(i) "sky color r-i"
	 set tlab(z) "sky color r-z"

	 # limits on colors 
	 set c_min(u) -0.1; set c_max(u) 0.6
	 set c_min(g)  0.6; set c_max(g) 1.3
	 set c_min(i)  0.4; set c_max(i) 1.1
	 set c_min(z)  1.5; set c_max(z) 2.5
	 foreach filter {u g z i} {
	    set pqminArray($filter) $c_min($filter)
	    set pqmaxArray($filter) $c_max($filter)
	    foreach col $cols { 
	       set vec1 $pqvec($col,$mf1($filter))
	       set vec2 $pqvec($col,$mf2($filter))
	       set off1 $off($mf1($filter)); set off2 $off($mf2($filter))
	       set sky_color($col,$filter) [sky_color $vec1 $vec2 $off1 $off2]
	    }
	 }
	 # replace plotting vectors
	 foreach filter {u g z i} {
	    foreach col $cols { 
	       vectorExprDel $pqvec($col,$filter) 
	       set pqvec($col,$filter) $sky_color($col,$filter)
	    }
	 } 
      }                

      ### special case: seeing derivative
      if {$pq =="deriv"} {
	 foreach filter {u g r i z} {
	    set pqminArray($filter) -0.3
	    set pqmaxArray($filter)  0.3
	    foreach col $cols {
	       set Npt [exprGet $pqvec($col,$filter).dimen]
	       loop i 0 [expr $Npt-1] {
		  set w2 [vectorExprGet $pqvec($col,$filter)<[expr $i+1]>]
		  set w1 [vectorExprGet $pqvec($col,$filter)<$i>]
		  set dwdt [expr $w2 - $w1]
		  handleSet $pqvec($col,$filter).vec<$i> $dwdt
	       }
	       handleSet $pqvec($col,$filter).vec<[expr $Npt-1]> 0.0
	    }
	 } 

      }


      # special case: Kolmogoroff exponents assumes all 5 filters are requested
      if {$pq =="KolmExp"} {
	 # effective wavelengths
	 set lambdaEff(u) 3540.0
	 set lambdaEff(g) 4760.0
	 set lambdaEff(r) 6280.0
	 set lambdaEff(i) 7690.0
	 set lambdaEff(z) 9250.0 
	 # matching filters
	 set mf1(u) u; set mf2(u) g;
	 set mf1(g) g; set mf2(g) r;
	 set mf1(i) r; set mf2(i) i;
	 set mf1(z) i; set mf2(z) z;
	 # chip to chip offsets
	 set off(u) 4; set off(g) 8; set off(r) 0; set off(i) 2; set off(z) 6;
	 # title labels
	 set tlab(u) "u-g Kolmogoroff exponent"
	 set tlab(g) "g-r Kolmogoroff exponent"
	 set tlab(r) "r PSF width (arcsec)"
	 set tlab(i) "r-i Kolmogoroff exponent"
	 set tlab(z) "i-z Kolmogoroff exponent"

	 foreach filter {u g z i} {
	    # plotting limits for exponents
	    set c_min($filter) -0.6; set c_max($filter) 1.0
	    set pqminArray($filter) $c_min($filter)
	    set pqmaxArray($filter) $c_max($filter)
	    foreach col $cols { 
	       set vec1 $pqvec($col,$mf1($filter))
	       set vec2 $pqvec($col,$mf2($filter))
	       set off1 $off($mf1($filter));
	       set off2 $off($mf2($filter))
	       set L1 $lambdaEff($mf1($filter));
	       set L2 $lambdaEff($mf2($filter));
	       set Kolm_exp($col,$filter) \
		   [Kolm_exponents $vec1 $vec2 $off1 $off2 $L1 $L2]
	    }
	 }
	 # replace plotting vectors
	 foreach filter {u g z i} {
	    foreach col $cols { 
	       vectorExprDel $pqvec($col,$filter) 
	       set pqvec($col,$filter) $Kolm_exp($col,$filter)
	    }
	 } 
      }                


      #PART III: Plot up all the graphs

      # do we have any data?
      set some_data 0
      foreach filter $wantedfilters {
	 if {!$skipfilter($filter)} {
            set some_data 1
	 }   
      }
      if {!$some_data} {return}


      #### QA tables ####
      if {$pq == "QAtables"} {
	 # make 6+1 QA par files with the field quality
	 makeRUN_QAtables cframes $run $rerun $cols $wantedfilters $inputDir
	 # make html file with PSP QA tables
	 makePSPtables cframes $run $rerun $inputDir \
	     $cols $wantedfilters $photoVer
	 continue
      }


      ##### PLOTTING #####
      set pg [pgstateNew]
      if {$pq == "psf->width"} {
	 set filename psp-$runstr-PSFwidth.ps
      } elseif {$pq == "prof->psfCounts"} {
	 set filename psp-$runstr-PSFcounts.ps
      } elseif {$pq == "psf->beta"} {
	 set filename psp-$runstr-PSFbeta.ps
      } elseif {$pq == "deriv"} {
	 set filename psp-$runstr-PSFwidthDerivative.ps
      } else {
	 set filename psp-$runstr-$pq.ps
      }
      set device [devicename $deviceA $inputDir/$filename]
      pgstateSet $pg -device $device \
	  -xfract .2 -yfract .3 -nxwindow 2 -nywindow 3 -isLine 1
      pgstateOpen $pg
      set madeMyBox 0
      pgText .35 1.104 "$plotlabel for run $run ($rerun)"
      pgSch .9

      foreach filter $wantedfilters {
	 if {$skipfilter($filter)} {continue}
	 foreach col $cols {
	    if {![info exists cframes($col)] ||
		$filterIndex($col,$filter) == -1} {
	       continue
	    }
	    if {$deviceA != "/PS"} {pgstateSet $pg -icLine [expr $col + 7]}
	    vPlot $pg $frameIndexByCol($col) $pqvec($col,$filter) \
		-ymin $pqminArray($filter) -ymax $pqmaxArray($filter) \
		-xmin $startframeArray($filter) -xmax $endframeArray($filter)
	    pgstateSet $pg -isNewplot 0

	    if {$pq == "skyanderr" || $pq == "skymaganderr"} {
	       vPlot $pg $frameIndexByCol($col) $pqvecPlusErr($col,$filter) \
		   -ymin $pqminArray($filter) -ymax $pqmaxArray($filter) \
		   -xmin $startframeArray($filter) \
		   -xmax $endframeArray($filter)
	       
	       vPlot $pg $frameIndexByCol($col) $pqvecMinusErr($col,$filter) \
		   -ymin $pqminArray($filter) -ymax $pqmaxArray($filter) \
		   -xmin $startframeArray($filter) -\
		   xmax $endframeArray($filter)
	    }

	    pgMtext B 2.0 .5 .5 $xlabel
	    ylabel $ylabel
            if {$pq =="skycol" || $pq =="KolmExp"} {
               titlePlot "$tlab($filter)" 40
            } else { 
               titlePlot "$plotlabel in ${filter}'" 40 
            }
	    if {!$madeMyBox} {makeMyBox $pg $filter; set madeMyBox 1}

	 }
	 pgstateSet $pg -isNewplot 1
      }

      #Print things up if necessary
      foreach filter $wantedfilters {
	 if {$skipfilter($filter)} {continue}
	 foreach col $cols {
	    if {$filterIndex($col,$filter) == -1} {continue}
	    #puts "Average $pq in $col $filter: [vMean $pqvec($col,$filter)]"
	 }
	 puts ""
      }

      if {$deviceA =="/PS" || $deviceA == "/cps" || $deviceA == "/CPS"} {
	 if {$pq == "psf->width"} {
            set pq psf_width
	 }
	 if {$pq == "psf->psfCounts"} {
            set pq psf_psfCounts
	 }
	 endplot $pg 
      } ;#else {pgstateClose $pg}

   }; # end of loop over quantities

   foreach col $cols {
      chainDestroy $cframes($col) calib1byframeDel
   }
}


### aux proc for treating sky colors: given 2 vectors and their offsets
# off, make a new vector color(i) = vec1(i-off1) - vec(i-off2)
proc sky_color {vec1 vec2 off1 off2} {

     set N [exprGet $vec1.dimen]
     assert {$N == [exprGet $vec2.dimen]}
     set color [vectorExprNew $N]

     set off [expr max($off1,$off2)]

     loop i $off $N {
        set i1 [expr $i - $off1]
        set i2 [expr $i - $off2]
        handleSet $color.vec<$i> [expr [exprGet $vec1.vec<$i1>] - [exprGet $vec2.vec<$i2>]]
     }

     loop i 0 $off  {
        handleSet $color.vec<$i> [exprGet $color.vec<$off>]
     }

     return $color
}

### aux proc for treating Kolmogoroff exponents: given 2 vectors and their offsets
# off, make a new vector Kexp(i) = log(vec2(i-off1)/vec1(i-off2)) / log(lambda1/lambda2)
proc Kolm_exponents {vec1 vec2 off1 off2 lambda1 lambda2} {

     set N [exprGet $vec1.dimen]
     assert {$N == [exprGet $vec2.dimen]}
     assert { [expr $lambda1*$lambda2] > 0}
     set Kexp [vectorExprNew $N]

     set off [expr max($off1,$off2)]

     set aux [expr 1.0/log($lambda1/$lambda2)]
     loop i $off $N {
        set i1 [expr $i - $off1]
        set i2 [expr $i - $off2]
        set w1 [exprGet $vec1.vec<$i1>]; set w2 [exprGet $vec2.vec<$i2>]
    if {$w2 > 0} {
            handleSet $Kexp.vec<$i> [expr $aux * log($w2/$w1)]
        } else {
            handleSet $Kexp.vec<$i> -99.9
        }
     }

     loop i 0 $off  {
        handleSet $Kexp.vec<$i> [exprGet $Kexp.vec<$off>]
     }

     return $Kexp
}



proc makeMyBox {pg filter} {
    foreach thing {pqvec wantedfilters cols filterIndex cframes plotlabel \
           endframeArray startframeArray pqminArray pqmaxArray} {
    upvar $thing $thing
    }

    #make array of colors
    set color(0) "black"
    set color(1) "white"
    set color(2) "light red"
    set color(3) "light green"
    set color(4) "dark blue"
    set color(5) "cyan"
    set color(6) "light purple"
    set color(7) "yellow"
    set color(8) "orange"
    set color(9) "green-yellow"
    set color(10) "sick green"
    set color(11) "light blue"
    set color(12) "dark purple"
    set color(13) "magenta"
    set color(14) "dark grey"
    set color(15) "medium gray"

    set xmin $startframeArray($filter)
    set xmax $endframeArray($filter)
    set ymin $pqminArray($filter)
    set ymax $pqmaxArray($filter)

    set deltax [expr $xmax - $xmin]
    set deltay [expr $ymax - $ymin]

    set yspacing [expr .4*(1.2*$deltay)]

    set basex [expr 1.1*$deltax + .2*$deltax + $xmin]
    set basey [expr (-2 * $yspacing) - (1.25 * $deltay) + $ymin]

    set colwidth [expr [set xlength [expr 1.2*$deltax]]/4]
    set rowheight [expr [set ylength [expr 1.2*$deltay]]/5]
    set dx [expr $colwidth/2.8]
 
    pgSch 1; pgText $basex $basey "Column"
    pgSch 1; pgText [expr $basex + $colwidth] $basey color
    pgSch .7; pgText [expr $basex + 2*$colwidth] $basey "limiting mag."
    pgSch .7; pgText [expr $basex + 3*$colwidth] $basey "median $plotlabel"


    foreach col $cols {
    if {![info exists cframes($col)]} {continue}
    set y [expr $basey - $col*$rowheight]
    pgSch 1; pgText $basex $y $col
    pgSci [expr $col + 7]
    pgSch .75; pgText [expr $basex+$colwidth] $y $color([expr $col+7])
    pgSci 1
    pgSch .5
    set xoffset 0
      foreach filter $wantedfilters {
        if {$filterIndex($col,$filter) == -1} {continue}
        pgText [expr $basex + 3*$colwidth + $xoffset] \
        $y "$filter:[format "%.2f" [expr .005 + [vMedian $pqvec($col,$filter)]]]"
        set xoffset [expr $xoffset + $dx]
      }
    }
    pgSch .9

}

# median smooth a given vector of size N, where w is the box width  
proc vMedianSmooth {vec N w} {
   assert {$w == 3};			# FIX THIS XXX

     set aux [vectorExprEval $vec]   
     set box [vectorExprNew 3]
     
     loop i 0 $N {
        if {$i > 0 && $i < [expr $N-1]} {
           handleSet $box.vec<0> [exprGet $aux.vec<[expr $i-1]>]
           handleSet $box.vec<2> [exprGet $aux.vec<[expr $i+1]>]
        } else {
           if {$i == 0} {
              handleSet $box.vec<0> [exprGet $aux.vec<[expr $i+1]>]
              handleSet $box.vec<2> [exprGet $aux.vec<[expr $i+2]>]
           }
           if {$i == [expr $N - 1]} {
              handleSet $box.vec<0> [exprGet $aux.vec<[expr $i-1]>]
              handleSet $box.vec<2> [exprGet $aux.vec<[expr $i-2]>]
           }
        }
        handleSet $box.vec<1> [exprGet $aux.vec<[expr $i]>]
        handleSet $vec.vec<$i> [vMedian $box]
     }  

     vectorExprDel $aux
     vectorExprDel $box

}

# replace each value in a given vector of size N, by the mean value
# of its neighbors (except at the edges)
proc vMeanSmooth {vec} {

     set aux [vectorExprEval $vec]   
     set N [exprGet $vec.dimen]
     
     loop i 0 $N {
        if {$i > 0 && $i < [expr $N-1]} {
            set vL [exprGet $aux.vec<[expr $i-1]>]
            set vR [exprGet $aux.vec<[expr $i+1]>]
        } else {
           if {$i == 0} {
              set vL [exprGet $aux.vec<0>]
              set vR $vL
           }
           if {$i == [expr $N - 1]} {
	      set vR [exprGet $aux.vec<$i>]
              set vL $vR
           }
        }
	handleSet $vec.vec<$i> [expr 0.5*($vL+$vR)]
     }  

}


# replace each value in a given vector, Y(i) by
# [0.25*Y(i-1) + 0.50*Y(i) + 0.25*Y(i+1)] 
# (except at the edges)
proc vTriangleSmooth {vec} {

     set aux [vectorExprEval $vec]   
     set N [exprGet $vec.dimen]
     
     loop i 0 $N {
        if {$i == 0 || $i == [expr $N-1]} {
            handleSet $vec.vec<$i> [exprGet $aux.vec<$i>]
            continue
        }
        set vL [exprGet $aux.vec<[expr $i-1]>]
        set vR [exprGet $aux.vec<[expr $i+1]>]
        set v0 [exprGet $aux.vec<$i>]
	handleSet $vec.vec<$i> [expr 0.25*($vL+$vR)+0.50*$v0]
     }  

}



#
# e.g. 
# read_allSky /u/dss/data/50990/75/output/psCBandCTfiles C /CPS
#             psf->width {1 2 6} 15 280 field "pixel" "1 Gaussian width"
#     or:  skymag {1 2 6} 15 280 field "mag/arcsec2" "Sky Brightness"
# read_allSky system C /XWINDOW skymag {4} 10 100 field mag/arcsec2 "Sky Brightness" 76 
# OR: 
# set dir /u/dss/data/50990/75/output/psCBandCTfiles; set cols {1 2 3 4 5 6 }
# read_allSky $dir C /CPS psf->width $cols 20 280 field "pixel" "seeing"
# read_allSky $dir C /CPS skymag $cols 20 280 field mag/arcsec2 "Sky Brightness"
# read_allSky $dir C /CPS sky $cols 20 280 field count "Sky Counts" 
# read_allSky $dir C /XWINDOW app_correction $cols 20 280 field " " "Aperture Correction" 
# read_allSky $dir C /XWINDOW gain $cols 20 280 field " "  "Gain" 
# read_allSky $dir {} /XWINDOW app_correction $cols 20 280 field " " "Aperture Correction" 
# read_allSky $dir {} /CPS app_correction $cols 20 280 field " " "Aperture Correction"   

# these proc is a wrapper for Kristian's proc read_allSky: 
# given a dir with 6 subdirs, each containing psField files, plot
# PSFwidth, sky, skymag and gain.  
# e.g. 
# > set device /CPS
# > set dir /data/dp3.b/data 
# > set run 94
# > plotPSPall $device $dir $run 
#
#
# e.g. for testbed simply change data_root
# photo> plotPSPall /CPS $data_root/testbed 745 all 0 1000 all private C
# e.g. for a private dir 
# photo> plotPSPall /XWINDOW $myDir 745 "" all 0 1000 all private C
#
# See plotPSPall2 for a more user-friendly interface
  
proc plotPSPall {device data_root run {rerun ""} {cols "all"} {sF 0} {eF 10000} \
             {quantities "all"} {dirtype1 ""} {dirtype2 "C"}} {

  global use_psPSF
  global QAscore

  set QAscore 0

  if {![info exists use_psPSF]} {set use_psPSF 0}

  if {$quantities == "all"} {
     set quantities [list QAtables status psf->width deriv prof->psfCounts \
                          psf_nstar psf_ap_correctionErr sky skyslope skymag \
                          skycol EgainN flux20 ap_corr_run ap_corr_runErr \
                          KolmExp psf->beta beta_prof]
     set do_all 1
  } else {
     set do_all 0
  }
  # for old PU directory structure set dirtype = "PU", for dirtype = "" follow FNAL's structure,
  # for anything else simply use $data_root as given (e.g. private directories)
  # for dirtype2 == "C" assume that data for each column are listed separately in /1, /2 etc.
  # otherwise assume that all data are in the same directory (e.g. when using private dir)
  if {$dirtype1 == "PU"} {
     set dir $data_root/$run/output
  } elseif {$dirtype1 == ""} {
     set dir $data_root/$run/$rerun/objcs
  } else {
     set dir $data_root 
  }

  set filters {u g r i z}
  if {$cols == "all"} {set cols {1 2 3 4 5 6 }}
  set label {}
  set unit {}
  foreach quant $quantities {
     if {$quant == "psf->width"} {
         lappend label [list "PSFwidth"]
         lappend unit [list "arcsec"]
     } elseif {$quant == "psf->beta"} {
         lappend label [list "beta determined by PSP"]
         lappend unit [list "-"]
     } elseif {$quant == "beta_prof"} {
         lappend label [list "beta determined from comp. prof (6-8)"]
         lappend unit [list "-"]
     } elseif {$quant == "ap_corr_run"} {
         lappend label [list "aperture correction bias"]
         lappend unit [list "mag"]
     } elseif {$quant == "psf_ap_correctionErr"} {
         lappend label [list "5to7 ap. corr."]
         lappend unit [list "mag"]
     } elseif {$quant == "ap_corr_runErr"} {
         lappend label [list "aperture correction error"]
         lappend unit [list "mag"]
     } elseif {$quant == "sky"} {
         lappend label [list "Sky Counts"]
         lappend unit [list "counts"]
     } elseif {$quant == "skyslope"} {
         lappend label [list "Sky Slope"]
         lappend unit [list "milicounts/pixel"]
     } elseif {$quant == "skymag"} {
         lappend label [list "Sky Brightness"]
         lappend unit [list "mag/arcsec2"]
     } elseif {$quant == "EgainN"} {
         lappend label [list "Spurious Noise"]
         lappend unit [list "Noise / Expected Noise"]
     } elseif {$quant == "Egain"} {
         lappend label [list "Observed Gain"]
         lappend unit [list "-"]
     } elseif {$quant == "prof->psfCounts"} {
         lappend label [list "PSFcounts"]
         lappend unit [list "counts"]
     } elseif {$quant == "psf_nstar"} {
         lappend label [list "PSF - Nstar"]
         lappend unit [list "-"]
     } elseif {$quant == "skycol"} {
         lappend label [list "sky color"]
         lappend unit [list "mag"]
     } elseif {$quant == "deriv"} {
         lappend label [list "seeing derivative"]
         lappend unit [list "arcsec / frame"]
     } elseif {$quant == "KolmExp"} {
         lappend label [list "Kolmogoroff exponents"]
         lappend unit [list "-"]
     } elseif {$quant == "status"} {
         lappend label [list "PSP status"]
         lappend unit [list "-"]
     } elseif {$quant == "QAtables"} {
         lappend label [list ""]
         lappend unit [list "-"]
     } else {
         lappend label [list $quant]
         lappend unit [list "-"]
     }     
  }

  if {$rerun == ""} {set rerun -1}
  if {[lsearch $quantities corr5to7] < 0} {
      read_allSky $dir $dirtype2 $device $quantities $cols $sF $eF "field" \
              $unit $label $run $filters $rerun
  }

  # do we want wing correction?
  if {(0 && $do_all) || [lsearch $quantities corr5to7] >= 0} {
      # plot fits to the wing correction vs. seeing
      foreach col $cols {
         set runstr  [format "%06d" $run]
         set psF $col/psField-$runstr-$col- 
         set title "Run $run, col. $col, fields $sF:$eF"
         set file $dir/wing_correction-$runstr-$col.ps
         plot_corrApCorr $device $psF -1 0 med $sF $eF $title $filters $file $use_psPSF
      } 
  } 

  if {$QAscore != 0} {
      return 1
  } else {
      return 0
  }


}

proc plotPSPall2 {args} {
   global data_root

   set flat 0;				# Assume all data are in same directory
					# (as opposed to e.g. objcs/camCol)
   set old_pu_directories 0;		# old Princeton directory scheme

   set opts [list \
		 [list [info level 0] "Produce summary plots for the PSP"] \
		 [list <device> STRING "" device \
		      "Output device; /PS (for hardcopy) or /XWINDOW"] \
		 [list <run> INTEGER 0 run "Desired run"] \
		 [list {[startField]} INTEGER 0 startField "Starting field"] \
		 [list {[endField]} INTEGER 10000 endField "Ending field"] \
		 [list -rerun INTEGER -1 rerun "Desired rerun"] \
		 
		 [list -cols STRING "all" cols "List of columns to plot"] \
		 [list -what STRING "all" quantities \
		      "List of things to plot; choose from
 QAtables status psf->width deriv prof->psfCounts psf_nstar
 psf_ap_correctionErr sky skyslope skymag skycol EgainN flux20
 ap_corr_run ap_corr_runErr KolmExp psf->beta beta_prof"] \
		 [list -root STRING $data_root root "Root directory"] \
		 [list -flat CONSTANT 1 flat \
		      "Assume all desired files are in \$root"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$rerun < 0} { set rerun "" }

   set dirtype1 "";			# i.e. survey standard directories
   if $old_pu_directories { set dirtype1 "PU" }

   set dirtype2 "C"
   if $flat {
      set dirtype1 "flat"
      set dirtype2 ""
   }
   
   plotPSPall $device $root $run $rerun $cols $startField $endField \
             $quantities $dirtype1 $dirtype2
}




###
# The following two structures are for 
# passing information about the field quality */
typedef enum {UNDETERMINED = -1, MISSING = 0, GOOD = 1, ACCEPTABLE = 2, \
              HOLE = 3, BAD = 4} FIELD_QUALITY;

typedef struct {
        int run;
        int camCol;
        int rerun;
        int firstField;   /* starting field for a stretch of given quality */      
        int lastField;    /* ending field for a stretch of given quality */         
        FIELD_QUALITY quality;  
} RUN_QA;

set_array_from_define FIELD_QUALITY "" FIELD_QUALITY


# make new RUN_QA and set its records
proc newRUN_QA {run rerun camCol firstField lastField} {
     set a [genericNew RUN_QA]; 
     handleSet $a.quality UNDETERMINED  
     handleSet $a.rerun $rerun; 
     handleSet $a.run $run; handleSet $a.camCol $camCol 
     handleSet $a.firstField $firstField
     handleSet $a.lastField $lastField
     return $a
}


# given an objcs dir full of psField files, determine and dump RUN_QA chains
# and produce plots
proc doRUN_QAtables {objcDir run rerun \
              {camCols all} {startField -1} {endField -1} {filters "all"}} {

      # default is all columns
      if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}

      # default is all filters
      if {$filters == "all"} {set filters {u g r i z}}

      # read data
      set runstr [format "%06d" $run]
      foreach col $camCols {
         # default is all fields
         if {$startField == -1} {
             set startF [lindex [find_field_range $objcDir/$col psField] 0]
         } else {
             set startF $startField 
         }
         if {$endField == -1} {
             set endF [lindex [find_field_range $objcDir/$col psField] 1]
         } else {
             set endF $endField 
         }  
         set root $objcDir/$col/psField-$runstr-$col-
         set cframes($col) [get_cal_chain_from_psField $root $startF $endF] 
      }

      # dump chains and produce plots
      makeRUN_QAtables cframes $run $rerun $camCols $filters $objcDir 

      # clean
      foreach col $camCols {
         chainDestroy $cframes($col) calib1byframeDel
      }
     
}

# make QA tables for PSP outputs
proc makeRUN_QAtables {calframes run rerun cols filters {dir .}} {

global QAparfile QApars PSP_STATUS

     upvar $calframes cframes 

     set runstr [format %06d $run]
     set outfile pspQA-$runstr.html 

     if {![info exist QAparfile] || $QAparfile == "default"} {
         set QAparfile [envscan \$PHOTO_DIR]/etc/psQApar.par 
     }
     param2Chain $QAparfile QApars
     foreach file "QApars" {
        foreach el [set $file] {
           set key [lindex $el 0]
           set val [join [lrange $el 1 end]]
           keylset $file $key $val
        }
     }
     if {[catch {set Cwidth [keylget QApars Cwidth]}]} {
          set Cwidth 2.0
     }
     # wavelength dependent seeing corrections
     set lam(u) 3540.0
     set lam(g) 4760.0
     set lam(r) 6280.0
     set lam(i) 7690.0
     set lam(z) 9250.0
     foreach f {u g r i z} {
        # applying Kolmogoroff correction: seeing \propto lambda^0.2
        set wav_corr [expr pow($lam($f)/$lam(r),0.2)]
        set CwidthMax($f) [expr $Cwidth * $wav_corr]
     }
     # some defaults in case we don't have them in QA par file
     if {[catch {set ApCerrMax(u) [keylget QApars uApCerrMax]}]} {
          set ApCerrMax(u) 0.1
     }
     if {[catch {set ApCerrMax(g) [keylget QApars gApCerrMax]}]} {
          set ApCerrMax(g) 0.05
     }
     if {[catch {set ApCerrMax(r) [keylget QApars rApCerrMax]}]} {
          set ApCerrMax(r) 0.05
     }
     if {[catch {set ApCerrMax(i) [keylget QApars iApCerrMax]}]} {
          set ApCerrMax(i) 0.05
     }
     if {[catch {set ApCerrMax(z) [keylget QApars zApCerrMax]}]} {
          set ApCerrMax(z) 0.07
     }
     if {[catch {set ApCbiasMax [keylget QApars ApCbiasMax]}]} {
          set ApCbiasMax 0.15
     }
     if {[catch {set fieldBuffer [keylget QApars fieldBuffer]}]} {
          set fieldBuffer 100
     }
     if {[catch {set delwidthMax [keylget QApars delwidthMax]}]} {
          set delwidthMax 0.2
     }

 
     # loop over camera columns
     foreach col $cols {
        set fieldQchain($col) [chainNew RUN_QA] 
        set Nfields [chainSize $cframes($col)]
        # loop over fields
        loop i 0 $Nfields {
           set el [chainElementGetByPos $cframes($col) $i] 
           set field  [exprGet $el.field]
           set fieldQuality [newRUN_QA $run $rerun $col $field $field]
	   if {$i > 0 && $i < [expr $Nfields - 1]} {
                set prev [chainElementGetByPos $cframes($col) [expr $i-1]] 
                set next [chainElementGetByPos $cframes($col) [expr $i+1]] 
                set edge 0
	   } else {
                set edge 1
           }
           # loop over all bands
           foreach f $filters {
              set ifilter [lsearch $filters $f]
              set PSPstatus [exprGet (int)$el.calib<$ifilter>->status]
              if {$PSPstatus == $PSP_STATUS(EXTENDED_KL)} {set PSPstatus 0.5}
              # aux fields do not trigger HOLE
              if {$PSPstatus >= $PSP_STATUS(SPARSE)} {set PSPstatus 0}
              set PSFwidth [exprGet $el.calib<$ifilter>->psf->width] 
              set apCorrErr [exprGet $el.calib<$ifilter>->ap_corr_runErr]
              set apCorrBias [exprGet $el.calib<$ifilter>->ap_corr_run]
              # for catching spikes
	      if {!$edge} {
                   set PSFwidthL [exprGet $prev.calib<$ifilter>->psf->width] 
                   set PSFwidthR [exprGet $next.calib<$ifilter>->psf->width] 
                   set widthBase [expr 0.5*($PSFwidthL + $PSFwidthR)]
              } else {
                   set widthBase $PSFwidth
              }
              # update field quality, based on this filter's data
              set_field_quality $fieldQuality $field $PSFwidth $apCorrErr $apCorrBias $PSPstatus \
                   $CwidthMax($f) $ApCerrMax($f) $ApCbiasMax $widthBase $delwidthMax
           }
           # all filters analyzed, add this field to chain 
           chainElementAddByPos $fieldQchain($col) $fieldQuality
        }
        # dump a file with this chain   
        set runstr [format "%06d" $run]
        set outfile $dir/pspRUN_QA-$runstr-$col.par
        chain2Param $outfile $fieldQchain($col) ""
     }

     # now that all columns are processed, mark bad fields
     mark_bad_fields $dir $run $cols $fieldBuffer 


}


# given the values of PSF width, ap.Corr. error, PSP status, maximum
# values of PSF width and aperture correction error, set the quality field
# in fieldQuality
proc set_field_quality {fieldQuality field PSFwidth apCorrErr apCorrBias \
                PSPstatus PSFwidthMax apCerrMax apCbiasMax widthBase delwidthMax} {
    # field quality can be set only for individual fields, so
    assert {[exprGet $fieldQuality.firstField] == $field}    
    assert {[exprGet $fieldQuality.lastField] == $field}

    # unless proven guilty, presumed innocent
    if {[exprGet -enum $fieldQuality.quality] == "UNDETERMINED"} {    
        handleSet $fieldQuality.quality GOOD  
    }  

    ### PSP status check
    # field is GOOD if status=0 and PSF width and apCorrErr are small
    # if status=1,2 field is ACCEPTABLE if PSF width and apCorrErr are small
    # if status = 3, or either PSF width or apCorrErr are large, field is HOLE
    # if the width varies too much compared to the adjacent fields => HOLE

    if {$PSPstatus != 0} {
        handleSet $fieldQuality.quality ACCEPTABLE
    } 
    # note that this condition is also triggered by odd/even transient (PSPstatus=6)
    if {$PSPstatus > 2} {
        handleSet $fieldQuality.quality HOLE
    } 
    # PSF width check   
    if {$PSFwidth > $PSFwidthMax} {
        handleSet $fieldQuality.quality HOLE
    } 
    # aperture correction error
    if {$apCorrErr > $apCerrMax} {
        handleSet $fieldQuality.quality HOLE
    } 
    # aperture correction bias 
    if {$apCorrBias > $apCbiasMax} {
        handleSet $fieldQuality.quality HOLE
    } 
    # spike catcher
    if {[expr abs($PSFwidth - $widthBase)] > $delwidthMax} {
        handleSet $fieldQuality.quality HOLE
    }   

}
 

# set the quality field in fieldQuality to quality, if the latter 
# is worse than the former
proc update_field_quality {fieldQuality quality} {
   global FIELD_QUALITY
 
    set thisQ $FIELD_QUALITY([exprGet -enum $fieldQuality.quality])
    set newQ $FIELD_QUALITY($quality)
    if {$newQ > $thisQ} {
        handleSet $fieldQuality.quality $quality
    }
}
 


# search (a smallish) chain by selecting the value of enum 
proc chainSearchEnum {chain predicate value {eq 1} } {

    set sel [chainNew [exprGet $chain.type]]
    loop i 0 [chainSize $chain] {
        set el [chainElementGetByPos $chain $i]
        if {$eq == 1} {
           if {$value == [exprGet -enum $el.$predicate]} {
              chainElementAddByPos $sel $el
           } 
        } else {
           if {$value != [exprGet -enum $el.$predicate]} {
              chainElementAddByPos $sel $el
           } 
        } 
    }

    return $sel
}

# read all requested pspRUN_QA-$run-$col*par files and produce file
# pspRUN_QA-$run.par which lists bad chunks of fields
# originally this proc was meant to be a high level proc that combines
# all individual field qualities into a field quality to be used when
# finding which data need be rescanned (i.e. running windows and all that).
# Since Chris indicated that this is no longer necessary since Steve is 
# writing a stand-alone code to do that, I redesigned this proc to simply
# make a union of all 6 columns to be used in makePSPtables 
proc mark_bad_fields {dir run cols fieldBuffer} { 

        # overall QA file
        set runstr [format "%06d" $run]
        set outfile $dir/pspRUN_QA-$runstr.par
 
        set Nfields -1
        # read RUN_QS chain for each column
        foreach col $cols {
           set file $dir/pspRUN_QA-$runstr-$col.par
           set chain($col) [param2Chain $file ""] 
           # make sure that all columns have the same number of fields 
           if {$Nfields == -1} {
               set Nfields [chainSize $chain($col)]
           } else {
               assert {$Nfields == [chainSize $chain($col)] }  
           }
        }

        # analyze, i.e. make the union of all (6) field qualities        
        set fieldQchain [chainNew RUN_QA] 
        set col0 [lindex $cols 0]
        # loop over fields
        loop i 0 [chainSize $chain($col0)] {
           # loop over all available columns
           foreach col $cols {
              set el [chainElementGetByPos $chain($col) $i] 
              set field [exprGet $el.firstField]
	      if {$col == $col0} {
                  set rerun [exprGet $el.rerun]
                  set fieldQuality [newRUN_QA $run $rerun 0 $field $field] 
              } else {
                  assert {$field == [exprGet $fieldQuality.firstField]}
              }     
              update_field_quality $fieldQuality [exprGet -enum $el.quality]
	   }           
           # all columns analyzed, add this field to chain 
           chainElementAddByPos $fieldQchain $fieldQuality
        }
        # dump a file with this chain   
        set runstr [format "%06d" $run]
        set outfile $dir/pspRUN_QA-$runstr.par
        chain2Param $outfile $fieldQchain ""
                
        # clean
        foreach col $cols {
           genericChainDestroy $chain($col) genericDel 
        }  
        genericChainDestroy $fieldQchain genericDel     
}




# make QA tables for PSP outputs
proc makePSPtables {calframes run rerun indir cols filters photoVer} {

global QAparfile QApars
global QAscore PSP_STATUS


     upvar $calframes cframes 

   
     #### set up ####
     set runstr [format %06d $run]
     set outfile pspQA-$runstr.html 

     if {![info exist QAparfile] || $QAparfile == "default"} {
         set QAparfile [envscan \$PHOTO_DIR]/etc/psQApar.par 
     }
     param2Chain $QAparfile QApars
     foreach file "QApars" {
        foreach el [set $file] {
           set key [lindex $el 0]
           set val [join [lrange $el 1 end]]
           keylset $file $key $val
        }
     }
     # get needed parameters
     if {[catch {set PSFcol [keylget QApars PSFcol]}]} {
          set PSFcol 6
     }
     if {[catch {set PSFband [keylget QApars PSFband]}]} {
          set PSFband r
     }
     if {[catch {set Cwidth [keylget QApars Cwidth]}]} {
          set Cwidth 2.0
     }
     # wavelength dependent seeing corrections
     set lam(u) 3540.0
     set lam(g) 4760.0
     set lam(r) 6280.0
     set lam(i) 7690.0
     set lam(z) 9250.0
     foreach f {u g r i z} {
        # applying Kolmogoroff correction: seeing \propto lambda^0.2
        set wav_corr [expr pow($lam($f)/$lam(r),0.2)]
        set CwidthMax($f) [expr $Cwidth * $wav_corr]
     }     
     if {[catch {set pspQAoutdir [keylget QApars pspQAoutdir]}]} {
          set pspQAoutdir default
     }
     if {[catch {set pspQAhttpdir [keylget QApars pspQAhttpdir]}]} {
          set pspQAhttpdir default
     }
     if {[catch {set NstarsMin [keylget QApars NstarsMin]}]} {
          set NstarsMin 10
     }
     if {[catch {set badPSFmax [keylget QApars badPSFmax]}]} {
          set badPSFmax 10.0
     }
     if {[catch {set statusMax [keylget QApars statusMax]}]} {
          set statusMax 2.0
     }
     if {[catch {set SkyMin(u) [keylget QApars uSkyMin]}]} {
          set SkyMin(u) 21.7
     }
     if {[catch {set SkyMin(g) [keylget QApars gSkyMin]}]} {
          set SkyMin(g) 21.5
     }
     if {[catch {set SkyMin(r) [keylget QApars rSkyMin]}]} {
          set SkyMin(r) 20.5
     }
     if {[catch {set SkyMin(i) [keylget QApars iSkyMin]}]} {
          set SkyMin(i) 19.7
     }
     if {[catch {set SkyMin(z) [keylget QApars zSkyMin]}]} {
          set SkyMin(z) 18.5
     }
     if {[catch {set ApCerrMax(u) [keylget QApars uApCerrMax]}]} {
          set ApCerrMax(u) 0.1
     }
     if {[catch {set ApCerrMax(g) [keylget QApars gApCerrMax]}]} {
          set ApCerrMax(g) 0.05
     }
     if {[catch {set ApCerrMax(r) [keylget QApars rApCerrMax]}]} {
          set ApCerrMax(r) 0.05
     }
     if {[catch {set ApCerrMax(i) [keylget QApars iApCerrMax]}]} {
          set ApCerrMax(i) 0.05
     }
     if {[catch {set ApCerrMax(z) [keylget QApars zApCerrMax]}]} {
          set ApCerrMax(z) 0.07
     }
     if {[catch {set fieldBuffer [keylget QApars fieldBuffer]}]} {
          set fieldBuffer 100
     }
     if {[catch {set badFieldMaxCol [keylget QApars badFieldMaxCol]}]} {
          set badFieldMaxCol 5
     }
     if {[catch {set badFieldMaxAll [keylget QApars badFieldMaxAll]}]} {
          set badFieldMaxAll 10
     }
     if {[catch {set noiseMaxAll [keylget QApars noiseMaxAll]}]} {
          set noiseMaxAll 1.3
     }
     if {[catch {set noiseMinAll [keylget QApars noiseMinAll]}]} {
          set noiseMinAll 0.7
     }
     if {[catch {set noiseIgnore [keylget QApars noiseIgnore]}]} {
	  set noiseIgnore {}
     }
     if {[catch {set noiseMaxSpecial [keylget QApars noiseMaxSpecial]}]} {
          set noiseMaxSpecial 2.0
     }
     if {[catch {set noiseMinSpecial [keylget QApars noiseMinSpeciall]}]} {
          set noiseMinSpecial 0.5
     } 


 
     if {$pspQAoutdir == "default"}  {set pspQAoutdir $indir}
     if {$pspQAhttpdir == "default"} {set pspQAhttpdir $indir}

     set Ncols [llength $cols]
     set fac [expr 1.0/$Ncols]
     set Nfilters [llength $filters]


     ######  all set, make the file #####
     # output file 
     set outf [open $pspQAhttpdir/$outfile w]         
     puts $outf "<title>PSP QA for run $run</title>"
     puts $outf "<BODY bgcolor=\"EEEEFF\">"
     if {[file exists [envscan \$PHOTO_DIR]/test/lemming_looker.gif]} {
        exec cp [envscan \$PHOTO_DIR]/test/lemming_looker.gif .lemming_looker.gif
        puts $outf "<IMG src=\"./.lemming_looker.gif\">"
     } else {
        puts $outf "<hr>" 
     }
     puts $outf "<H1><font color=\"blue\">PSP QA for run $run, rerun $rerun</font></H1>"
     puts $outf "<hr>"
     set QAphotoVer [photoVersion]
     puts $outf "Data reduced with photo $photoVer <P>"
     puts $outf "QA produced on [exec date] with photo version $QAphotoVer<hr>"
      
     # numbers of acceptable fields
     set runstr [format "%06d" $run]
     set Qnames [list GOOD ACCEPTABLE]     
     set Qfile $pspQAoutdir/pspRUN_QA-$runstr.par
     set Qchain [param2Chain $Qfile ""]
     set Nall [chainSize $Qchain]
     set warning 0
     puts $outf "<P>"
     puts $outf "<table border cellspacing=0 cellpadding=5>"
     puts $outf "<tr> <td colspan=0 rowspan=1></td>"
     puts $outf "<th colspan=4 align=center><font color=\"blue\"> Field Quality (nFields=$Nall) \
                     </font></th> </tr>"
     puts $outf "<tr><th>quality </th><th> nOK </th><th>nHOLE</th><th>% HOLE</th></tr>"
     # the union of all columns
     set Nline "<tr> <th> <font color=\"blue\">union </font></th>"
     set Ngood 0
     foreach name $Qnames {
         set sel [chainSearchEnum $Qchain quality $name]
         set N [chainSize $sel]; chainDel $sel
         incr Ngood $N
     }
     set Nhole [expr $Nall-$Ngood]
     set Nline "$Nline <th> $Ngood </th> <th> $Nhole </th> "
     genericChainDestroy $Qchain
     set percBad [format "%4.1f" [expr 100.0*($Nall-$Ngood)/$Nall]]            
     if {[expr abs($percBad)] < $badFieldMaxAll} {
         set Nline "$Nline <th> $percBad </th></tr>"
     } else {
         set warning 1; incr QAscore
         set Nline "$Nline  <th><font color=\"red\"> ${percBad}* </font> </th></tr>"
     }    
     # add this line to file 
     puts $outf $Nline     
     foreach col $cols {
        set Ngood 0
        set Nline "<tr> <th> col. $col </th>"
        set Qfile $pspQAoutdir/pspRUN_QA-$runstr-$col.par
        set Qchain [param2Chain $Qfile ""]
        set Nall [chainSize $Qchain]
        foreach name $Qnames {
            set sel [chainSearchEnum $Qchain quality $name]
            set N [chainSize $sel]; chainDel $sel
            incr Ngood $N
        }
        set Nhole [expr $Nall-$Ngood]
        set Nline "$Nline <th> $Ngood </th> <th> $Nhole </th> "
        genericChainDestroy $Qchain
        set percBad [format "%4.1f" [expr 100.0*($Nall-$Ngood)/$Nall]]            
        if {[expr abs($percBad)] < $badFieldMaxCol} {
            set Nline "$Nline <th> $percBad </th></tr>"
        } else {
            set warning 1; incr QAscore
            set Nline "$Nline  <th><font color=\"red\"> ${percBad}* </font> </th></tr>"
        }
        # add this line to file 
        puts $outf $Nline
     }
     puts $outf "</table>" 
     if {$warning} {
         puts $outf "<br> <font color=\"red\"> *Red entries are triggered by \
                      the percentage of unacceptable fields exceeding</font>"
         puts $outf "<br> <font color=\"red\">$badFieldMaxCol in any column, or $badFieldMaxAll for the union of all columns.</font>"
     }
     set filename psp-$runstr-status.ps
     puts $outf "<BR><A HREF=$filename> PSP status plot</A>"
     puts $outf "<hr>" 
              

     ### test mean composite profile 
     set bad 0
     foreach col $cols {
        set Nbad [testCompProf $cframes($col)]
        if {$Nbad > 0} {
            puts $outf "<BR><blink> <font color=\"red\"> There are fields in column $col with negative mean composite profile!!!</font></blink></A>"
            set bad 1
            incr QAscore
        }
     }
     if {$bad} {puts $outf "<hr>"}          


     ## PSF width 
     puts $outf <P>   
     set ifilter [lsearch $filters r]
     if {$ifilter < 0} {
         echo "Filter r not available, returning from makePSPtables"
         return 
     }
     if {[lsearch $cols $PSFcol] < 0} {
         echo "Column $PSFcol for evaluating PSF width statistics not available"
         puts $outf "<strong><font color=\"red\">PSF width should be evaluated for column $PSFcol, \
                     which is not available!? </font></strong>"
         puts $outf "<hr>"
     } else {
         set PSFwidth [vFromChain $cframes($PSFcol) calib<$ifilter>->psf->width]
         set Nfields [exprGet $PSFwidth.dimen]
         set Nbad 0
         loop i 0 $Nfields {
            if {[exprGet $PSFwidth.vec<$i>] > $Cwidth} {incr Nbad}
         }
         set percBad [format "%4.1f" [expr 100.0*$Nbad/$Nfields]]
         # calculate statistics for this vector
         set PSFmean [format "%5.2f" [vMean $PSFwidth]]
         set PSFmedian [format "%5.2f" [vMedian $PSFwidth]] 
         set PSFmin [format "%5.2f" [vExtreme $PSFwidth min]]
         set PSFmax [format "%5.2f" [vExtreme $PSFwidth max]]
         vectorExprDel $PSFwidth
         # write to file
         puts $outf "<strong><font color=\"blue\">PSF width (FWHM arcsec) statistics \
                     for column 6, filter r:</font></strong>"
         puts $outf "<br><strong>Mean PSF width = $PSFmean</strong>"
         puts $outf "<br> Median = $PSFmedian, min = $PSFmin, max = $PSFmax"
         if {$percBad < $badPSFmax} { 
            puts $outf "<BR> There are $Nbad fields ($percBad%) with PSFwidth > $Cwidth arcsec"
         } else {
            puts $outf "<BR><strong><font color=\"red\"> There are $Nbad fields ($percBad%) \
                       with PSFwidth > $Cwidth arcsec</font></strong>"
         }
         puts $outf "<br> This run is $Nfields fields long."
         set filename psp-$runstr-PSFwidth.ps
         puts $outf "<BR><A HREF=$filename> PSF width plot for all 30 chips</A>"
         set filename psp-$runstr-PSFwidthDerivative.ps
         puts $outf "<BR><A HREF=$filename> PSF width derivative plot for all 30 chips</A>"
         puts $outf "<hr>" 


         ## PSP status
         set warning 0
         puts $outf "<P>"
         puts $outf "<table border cellspacing=0 cellpadding=5>"
         puts $outf "<tr> <td colspan=0 rowspan=1></td>"
         puts $outf "<th colspan=6 align=center><font color=\"blue\"> Mean PSP status  \
                     (0=OK) </font></th> </tr>"
         puts $outf "<tr><th>band</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"
         set Nbadfields 0
         foreach col $cols {
            set meanN "<tr> <th> col. $col </th>"
            foreach f $filters {
              set ifilter [lsearch $filters $f]
              set N [cframes2status $cframes($col) $ifilter] 
	      # vectorExprSet $N \
	      # 	   "$N + ($N >= $PSP_STATUS(EXTENDED_KL)) ? 0.5 : 0"
              vectorExprSet $N "$N == $PSP_STATUS(EXTENDED_KL) ? 0.5 : $N"
              vectorExprSet $N "$N >= $PSP_STATUS(SPARSE) ? -0.5 : $N" 
              set absN [vectorExprEval abs($N)]           
              set mean_N [format "%5.2f" [vMean $absN]]
              vectorExprDel $absN
              if {[expr abs($mean_N)] < $statusMax} {
                  set meanN "$meanN <th> $mean_N </th>"
              } else {
                  set warning 1; incr QAscore
                  set meanN "$meanN  <th><font color=\"red\"> ${mean_N}* </font> </th>"
              }
              vectorExprDel $N                
            }
            set meanN "$meanN </tr>"
            puts $outf $meanN
         }
         puts $outf "</table>" 
         if {$warning} {
            puts $outf "<br> <font color=\"red\"> *Red entries are triggered by \
                       mean PSP status > $statusMax </font>"
         }
         set filename psp-$runstr-status.ps
         puts $outf "<BR><A HREF=$filename> PSP status plot</A>"

         ## Nstars
         set warning 0
         puts $outf "<P>"
         puts $outf "<table border cellspacing=0 cellpadding=5>"
         puts $outf "<tr> <td colspan=0 rowspan=1></td>"
         puts $outf "<th colspan=6 align=center><font color=\"blue\"> Mean N stars \
                     for estimating PSF </font></th> </tr>"
         puts $outf "<tr><th>band</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"
         foreach col $cols {
            set meanN "<tr> <th> col. $col </th>"
            foreach f $filters {
              set ifilter [lsearch $filters $f]
              set N [vFromChain $cframes($col) calib<$ifilter>->psf_nstar]
              set mean_N [format "%4.1f" [vMean $N]]
              if {[expr abs($mean_N)] > $NstarsMin} {
                  set meanN "$meanN <th> $mean_N </th>"
              } else {
                  set warning 1; incr QAscore
                  set meanN "$meanN  <th><font color=\"red\"> ${mean_N}* </font> </th>"
              }
              vectorExprDel $N                
            }
            set meanN "$meanN </tr>"
            puts $outf $meanN
         }
         puts $outf "</table>" 
         if {$warning} {
            puts $outf "<br> <font color=\"red\"> *Red entries are triggered by \
                       Nstars < $NstarsMin</font>"
         }
         set filename psp-$runstr-psf_nstar.ps
         puts $outf "<BR><A HREF=$filename> Number of stars used \
                     for estimating PSF</A>"
     } 

     ### bias in aperture corrrection
     set warning 0
     puts $outf "<P>"
     puts $outf "<table border cellspacing=0 cellpadding=5>"
     puts $outf "<tr> <td colspan=0 rowspan=1></td>"
     puts $outf "<th colspan=6 align=center><font color=\"blue\"> Mean m(aperture)-m(psf) Bias (mag) </font></th> </tr>"
     puts $outf "<tr><th>band</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"
     foreach col $cols {
        set meanErr "<tr> <th> col. $col </th>"
        foreach f $filters {
           set ifilter [lsearch $filters $f]
           set err [vFromChain $cframes($col) calib<$ifilter>->ap_corr_run]
           set mean_err [format "%5.2f" [vMean $err]]
           if {[expr abs($mean_err)] < $ApCerrMax($f)} {
                set meanErr "$meanErr <th> $mean_err </th>"
           } else {
               set warning 1; incr QAscore
               set meanErr "$meanErr <th><font color=\"red\"> ${mean_err}* </font> </th>"
           }
           vListDel [list $err]                
        }
        set meanErr "$meanErr </tr>"
        puts $outf $meanErr
     }
     puts $outf "</table>" 
     if {$warning} {
         set apCerrs ""
         foreach f $filters {
            set apCerrs "$apCerrs $ApCerrMax($f)"
         }
         puts $outf "<br> <font color=\"red\"> *Red entries are triggered by</font>"
         puts $outf "<br> <font color=\"red\"> apCorr Bias ($apCerrs) mags in ($filters) </font>"
     }
     set filename psp-$runstr-ap_corr_run.ps
     puts $outf "<BR><A HREF=$filename> Aperture Correction Bias plot</A>"


     ### error in aperture corrrection
     set warning 0
     puts $outf "<P>"
     puts $outf "<table border cellspacing=0 cellpadding=5>"
     puts $outf "<tr> <td colspan=0 rowspan=1></td>"
     puts $outf "<th colspan=6 align=center><font color=\"blue\"> Mean m(aperture)-m(psf) Error (mag) </font></th> </tr>"
     puts $outf "<tr><th>band</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"
     foreach col $cols {
        set meanErr "<tr> <th> col. $col </th>"
        foreach f $filters {
           set ifilter [lsearch $filters $f]
           set err [vFromChain $cframes($col) calib<$ifilter>->ap_corr_runErr]
           set mean_err [format "%5.2f" [vMean $err]]
           if {[expr abs($mean_err)] < $ApCerrMax($f)} {
                set meanErr "$meanErr <th> $mean_err </th>"
           } else {
               set warning 1; incr QAscore
               set meanErr "$meanErr <th><font color=\"red\"> ${mean_err}* </font> </th>"
           }
           vListDel [list $err]                
        }
        set meanErr "$meanErr </tr>"
        puts $outf $meanErr
     }
     puts $outf "</table>" 
     if {$warning} {
         set apCerrs ""
         foreach f $filters {
            set apCerrs "$apCerrs $ApCerrMax($f)"
         }
         puts $outf "<br> <font color=\"red\"> *Red entries are triggered by</font>"
         puts $outf "<br> <font color=\"red\"> apCorr Error > ($apCerrs) mags in ($filters) </font>"
     }
     set filename psp-$runstr-ap_corr_runErr.ps
     puts $outf "<BR><A HREF=$filename> Aperture Correction Error plot</A>"


     # some other useful plots
     puts $outf "<BR> A few related auxiliary plots:"     
     set filename psp-$runstr-psf_ap_correctionErr.ps
     puts $outf "<BR><A HREF=$filename> Aperture Correction Scatter (w/o bias contribution)</A>"
     set filename psp-$runstr-beta_prof.ps
     puts $outf "<BR><A HREF=$filename> Power-law index for the outer PSF</A>"
     set filename psp-$runstr-KolmExp.ps
     puts $outf "<BR><A HREF=$filename> Kolmogoroff exponents</A>"




     #### MEAN SKY BRIGHTNESS #### 
     puts $outf "<P>"
     puts $outf "<table border cellspacing=0 cellpadding=5>"
     puts $outf "<tr> <td colspan=0 rowspan=1></td>"
     puts $outf "<th colspan=6 align=center> <font color=\"blue\">Sky brightness* (mag/arsec2)\
                 </font></th> </tr>"
     puts $outf "<tr><th>band</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"
     set meanSkyline "<tr> <th> mean </th> "
     set minSkyline "<tr> <th> min </th> "
     set maxSkyline "<tr> <th> max </th> "
     set warning 0
     foreach f $filters {
        set ifilter [lsearch $filters $f]
        foreach col $cols {
           set sky [vFromChain $cframes($col) calib<$ifilter>->sky]
           set flux20 [vFromChain $cframes($col) calib<$ifilter>->flux20]
           set q1 [vectorExprEval ln((6.25*$sky)/$flux20)/ln(10)]
           set skymag($col) [vectorExprEval 20-(2.5*$q1)]
           if {$col == [lindex $cols 0]} {
               set meansky($f) [vectorExprEval $skymag($col)]
           } else {
               set aux [vectorExprEval $meansky($f)]
               vectorExprDel $meansky($f)             
               set meansky($f) [vectorExprEval $aux+$skymag($col)]
               vectorExprDel $aux                
           }   
           vListDel [list $sky $flux20 $q1]                
        }
        set aux [vectorExprEval $meansky($f)]
        vectorExprDel $meansky($f)             
        set meansky($f) [vectorExprEval $aux/$Ncols]
        vectorExprDel $aux                

        set meanSky [format "%5.2f" [vMean $meansky($f)]]
        set minSky [format "%5.2f" [vExtreme $meansky($f) min]]
        set maxSky [format "%5.2f" [vExtreme $meansky($f) max]]
        if {$minSky > $SkyMin($f)} {
            set minSkyline "$minSkyline <th> $minSky </th>"
        } else {
            set warning 1; incr QAscore
            set minSkyline "$minSkyline <th><font color=\"red\"> ${minSky}** </font> </th>"
        }
        set meanSkyline "$meanSkyline <th> $meanSky </th>" 
        set maxSkyline "$maxSkyline <th> $maxSky </th>" 
     }
     set meanSkyline "$meanSkyline</tr>"  
     set minSkyline "$minSkyline</tr>"        
     set maxSkyline "$maxSkyline</tr>"  
     puts $outf $meanSkyline
     puts $outf $maxSkyline
     puts $outf $minSkyline
     puts $outf "<caption align=bottom>*Averaged over 6 columns</caption></table>" 
     if {$warning} {
        set mags ""
        foreach f $filters {
           set mags "$mags $SkyMin($f)"
        }
        puts $outf "<br> <font color=\"red\"> **Red entries are triggered by</font>"
        puts $outf "<br> <font color=\"red\"> min Sky < ($mags) mag in ($filters)</font>"
     }
     set filename psp-$runstr-sky.ps
     puts $outf "<BR><A HREF=$filename> Sky brightness (in counts) plot</A>"
     set filename psp-$runstr-skymag.ps
     puts $outf "<BR><A HREF=$filename> Sky brightness (in mag/arcsec2) plot</A>"
     set filename psp-$runstr-skyslope.ps
     puts $outf "<BR><A HREF=$filename> Sky derivative plot</A>"
     set filename psp-$runstr-skycol.ps
     puts $outf "<BR><A HREF=$filename> Sky colors plot</A>"


     # deviant columns
     puts $outf "<P>"
     puts $outf "<table border cellspacing=0 cellpadding=5>"
     puts $outf "<tr> <td colspan=0 rowspan=1></td>"
     puts $outf "<th colspan=6 align=center> <font color=\"blue\">Mean Sky Column offsets </font></th> </tr>"
     puts $outf "<tr><th>band</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"

     set warning 0
     foreach col $cols {
        set meanDelSky "<tr> <th> col. $col </th>"
        foreach f $filters {
           set ifilter [lsearch $filters $f]
           set sky [vFromChain $cframes($col) calib<$ifilter>->sky]
           set flux20 [vFromChain $cframes($col) calib<$ifilter>->flux20]
           set q1 [vectorExprEval ln((6.25*$sky)/$flux20)/ln(10)]
           set skymag($col) [vectorExprEval 20-(2.5*$q1)]
           vListDel [list $sky $flux20 $q1]                
           # find deviant columns
           set delsky($col) [vectorExprEval $skymag($col)-$meansky($f)]
           set aux [format "%5.2f" [vMean $delsky($col)]]
           if {[expr abs($aux)] < 0.15} {
               set meanDelSky "$meanDelSky <th> $aux </th>"
           } else {
               set warning 1; incr QAscore
               set meanDelSky "$meanDelSky <th> <font color=\"red\"> ${aux}* </font></th>"
           }
           vListDel [list $skymag($col) $delsky($col)]
        }
        set meanDelSky "$meanDelSky </tr>"
        puts $outf $meanDelSky
     }
     puts $outf "</table>" 
     if {$warning} {
        puts $outf "<br> <font color=\"red\"> *Red entries are triggered by abs(offset) > \
                    0.15 mag</font>"
     }
     foreach f $filters {
        vectorExprDel $meansky($f)
     }


     ### normalized gains <= noise estimate
     puts $outf "<P>"     
     puts $outf "<br> The noise is estimated from the width of the sky histogram, corrected"
     puts $outf "<br> for the dark variance, and normalized by the Poisson value:"
     puts $outf "<br> norm. spurious noise = sqrt\[gain*(sigma_sky^2-dark_var)/sky\]"
     puts $outf "<br> Values significantly larger than 1 indicate the presence of spurious noise."

     set warning 0
     puts $outf "<P>"
     puts $outf "<table border cellspacing=0 cellpadding=5>"
     puts $outf "<tr> <td colspan=0 rowspan=1></td>"
     puts $outf "<th colspan=6 align=center><font color=\"blue\">Spurious Noise (from sky quartiles)</font></th> </tr>"
     puts $outf "<tr><th>band</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"
     foreach col $cols {
        set gains "<tr> <th> col. $col </th>"
        foreach f $filters {
           set ifilter [lsearch $filters $f]
           set sky [vFromChain $cframes($col) calib<$ifilter>->sky]
           set skysig [vFromChain $cframes($col) calib<$ifilter>->skysig]
           set dv [vFromChain $cframes($col) calib<$ifilter>->dark_variance]
           set gain [vectorExprEval sqrt(($skysig*$skysig-$dv)/$sky*[JEGgain $col $f])]
           # median smooth over 3 frames (i.e. get rid of 1 point transients)
           set N [exprGet $gain.dimen]
           vMedianSmooth $gain $N 3
           set gg [format "%5.2f" [vMedian $gain]]
	   set chip $f${col}
	   if {[lsearch $noiseIgnore $chip] < 0} {
               set gainMin $noiseMinAll; set gainMax $noiseMaxAll
	   } else {
               set gainMin $noiseMinSpecial; set gainMax $noiseMaxSpecial
	   }
           if {$gg > $gainMin && $gg < $gainMax} {
              set gains "$gains <th> $gg </th>"
           } else {
              set warning 1; incr QAscore
              set gains "$gains <th><font color=\"red\"> ${gg}* </font></th>"
           }
           vListDel [list $sky $skysig $dv $gain]
        } 
        set gains "$gains </tr>"
        puts $outf $gains
     }
     puts $outf "</table>" 
     if {$warning} {
        puts $outf "<br> <font color=\"red\"> \
                   *Red entries are triggered by gain<$noiseMinAll or gain>$noiseMaxAll </font>"
	 if {$noiseIgnore != {}} {
             if {[llength $noiseIgnore] == 1} {
                   set word chip
             } else {
                   set word chips
             }
             puts $outf "<br> <font color=\"red\"> except for $word $noiseIgnore \
                   where the allowed range is $noiseMinSpecial-$noiseMaxSpecial</font>"
         }
     }

     set filename psp-$runstr-EgainN.ps
     puts $outf "<BR><A HREF=$filename> Spurious Noise plot</A>"
     puts $outf "<br> Note that bright stars cause single-field spikes which are, for" 
     puts $outf "<br> different bands, aligned in the field number. A temporal increase" 
     puts $outf "<br> in noise can be traced in the r-i-u-z-g order (2 fields shift)." 

     puts $outf "<hr>"
     if {$QAscore != 0} {
         if {$QAscore == 1} {
            puts $outf "<font color=\"red\"> There is one warning for this run. </font>"
         } else {
            puts $outf "<font color=\"red\"> There are $QAscore warnings for this run. </font>"
	 }
     } else {
         puts $outf "There are no warnings for this run. <font color=\"red\">Bravo PSP!</font>"
     }
     puts $outf "<hr>"

     close $outf

     
}


# plot 2 vectors 
proc plot_xy {x  y {device /XWINDOW} {title ""} {type points} {new 1} {finish 1}
          {xmin ""} {xmax ""} {ymin ""} {ymax ""} {mask ""}} {

global pg

  if [catch {
     if {$new} {
        # Set up the plot 
        set pg [pgstateNew]
        set device [devicename $device plot_xy.ps] 
        pgstateSet $pg -device $device -lineWidth 3 \
        -xfract 0.25 -yfract 0.25 -nxwindow 1 -nywindow 1 
        pgstateOpen $pg
     } else {
        pgstateSet $pg -isNewplot 0
     }
     if {$type == "line"} {
        pgstateSet $pg -isLine 1 
     } else {
       if [catch {set aux [expr $type * 1.0]}] {
          set symbol 1
       } else {
          set symbol $type  
       }  
       pgstateSet $pg -symb $symbol 
     }

     # limits 
     if {$xmin == ""} {set xmin [vExtreme $x min]}
     if {$xmax == ""} {set xmax [vExtreme $x max]}
     if {$ymin == ""} {set ymin [vExtreme $y min]}
     if {$ymax == ""} {set ymax [vExtreme $y max]}


     # plot y(x)
     if {$mask != ""} {
        vPlot $pg $x $y -vectorMask $mask -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax  
     } else {
        vPlot $pg $x $y -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax  
     }


     if {$title != ""} {titlePlot $title 40}

     if {$finish} {
         endplot $pg
     }
     return 0
  } msg] {
     echo "$msg"
     return 1
  }
     
}

# plot y = x*x for 0<x<10
proc demo_plot1 { {device /XWINDOW} } {

    # make data for plotting
    set x [vIndex 10]
    set y [vectorExprSet [vectorExprNew 10] $x*$x]
 
    # set axis labels
    vNameSet $x "x name"
    vNameSet $y "y name"

    # plot
    plot_xy $x $y $device  

    vectorsWriteToFile $x $y plot_xy.dat   

  return 0
}

# the central cross-sections through a region
proc demo_plot2 {{region ""} {device /XWINDOW} } {

global pg

    if {$region == ""} { 
       error "You must specify a region." 
    }

    set ncol [exprGet $region.ncol]
    set nrow [exprGet $region.nrow]
    set mcol [expr $ncol - 1]
    set mcol2 [expr $ncol / 2]
    set mrow [expr $nrow - 1]
    set mrow2 [expr $nrow / 2] 

    # open plot
    set pg [pgstateNew]
    set device [devicename $device demo_plot2.ps]
    pgstateSet $pg -device $device -lineWidth 3 \
     -xfract 0.25 -yfract 0.25 -nxwindow 1 -nywindow 1 
    pgstateOpen $pg 

    # plot vertical cross-section
    set x [vIndex $nrow]
    set y [vectorGetFromRegion $region $mrow2 $mrow2 0 $mcol 0] 
    vNameSet $x pixel
    vNameSet $y pixel 
    pgstateSet $pg -symb 7
    vPlot $pg $x $y  
    pgstateSet $pg -isNew 0
    pgstateSet $pg -isLine 1
    vPlot $pg $x $y 
    pgstateSet $pg -isLine 0

    # plot horizontal cross-section
    set x [vIndex $ncol]
    set y [vectorGetFromRegion $region 0 $mrow $mcol2 $mcol2 1] 
    vNameSet $x pixel
    vNameSet $y pixel
    pgstateSet $pg -symb 2
    vPlot $pg $x $y  
    pgstateSet $pg -isLine 1
    vPlot $pg $x $y 
    pgstateSet $pg -isLine 0


    # close plot     
    endplot $pg


  return 0
}
 
proc plot_chains {device xchain xquant ychain yquant {title ""} {type points}} {

    # make vectors
    set x [vFromChain $xchain $xquant]
    set y [vFromChain $ychain $yquant] 
    # this is not necessary
    vNameSet $x $xquant
    vNameSet $y $yquant

    # plot
    plot_xy $x $y $device  $title $type 
  
  return 0
}



proc vectorsWriteToFile {vList filename {title ""} {mask ""} {mode "w"} {header 1}} {

    # open file
    set f [open $filename $mode]
    if {$header} {
       puts $f "#  output from proc vectorsWriteToFile"
       puts $f "#  $title" 
    }
 
    # get number of elements
    set N [exprGet [lindex $vList 0].dimen]

    # loop through all elements
    set format ""; set vecs ""
    loop i 0 $N {
       set str ""
       if {$mask != ""} {
	  set good [exprGet $mask.vec<$i>]
       } else {
          set good 1
       }
       if {$good} {
          loop j 0 [llength $vList] {
	    set vec [lindex $vList $j]
            set val [exprGet $vec.vec<$i>]          
            set str "$str $val"
          }
          puts $f $str  
       }
    }

    # close file
    close $f

    return 0
}



proc plot_histogram {device vector {nbin ""} {min ""} {max ""} {xlabel ""} {title ""} \
             {pg ""} {finish 1} {mask ""}} {

  if [catch {
        if {$pg == ""} {
           # Set up the plot 
           set pg [pgstateNew]
           set device [devicename $device plot_histogram.ps]
           pgstateSet $pg -device $device -lineWidth 3 \
          -xfract 0.25 -yfract 0.25 -nxwindow 1 -nywindow 1 
           pgstateOpen $pg     
        } else {
           pgstateSet $pg -isNewplot 0
        }


        # set up histogram
        if {$nbin == ""} {
           set nbin 10
        }
        if {$min == ""} {
           extremes $vector min aux 0.1 
        }
        if {$max == ""} {
           extremes $vector aux max 0.1 
        }
        if {$xlabel == ""} {
           set xlabel [exprGet $vector.name]
        }

        set hg [hgNew]
        hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlabel -yLabel N

        # fill and plot the histogram
        if {$mask == ""} {           
           set mask1 [vectorExprEval "$vector < $max ? 1 : 0"]
        } else {
           set mask1 [vectorExprEval "$vector < $max ? $mask : 0"]
        }
        set mask2 [vectorExprEval "$vector > $min ? $mask1 : 0"]
        hgFillFromV $hg $vector -vMask $mask2
        vectorExprDel $mask1
        vectorExprDel $mask2
        hgPlot $pg $hg -xmin $min -xmax $max 
        hgDel $hg 

        titlePlot $title 40 
    
        if {$finish} {        
           endplot $pg
        }

  } msg] {
     echo "$msg"
     return 1
  }
 
  return $pg

}




# This procedure takes a Fang file, prints the header and displays 
# postage stamps filter by filter (assumed that stamp size is 65),
# and simultaneously prints out tables of stellar properties 
# measured by SSC. 
proc fang2sao {fangdir fangfile filterlist} {

   set stampsize 65
   set file $fangdir/$fangfile.fit
   set hdr [hdrReadAsFits [hdrNew] $file] 
   echo " "
   echo " -----------------------"
   echo " Primary header for file"
   echo " $file"
   hdrPrint $hdr
   echo " -----------------------" 
   hdrFreeAll $hdr; hdrDel $hdr

   echo " "; echo " " 
   echo "Parameters for stars on leading astrometric chip"
   print_Fang_table $file al 


foreach filter $filterlist {
   set hdu [get_hdu $file $filter stamps]
   set tblcol [handleNewFromType TBLCOL]
   fitsRead $tblcol $file -binary -hdu $hdu
            
   # Check that row is correct
   #echo " " 
   #echo " Header for filter $filter"
   #hdrPrint $tblcol.hdr
   set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
   assert {$hfilter == $filter}
   assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "STAMP"} 

   # Check stampsize 
   assert {[hdrGetAsInt $tblcol.hdr PSSIZE] == $stampsize}

   set stamps [tblcolToPsChain $tblcol]
   handleDelFromType $tblcol
   if {$filter == [lindex $filterlist 0]} {
       echo " There are [chainSize $stamps] stamps in this Fang file"  
       echo " "
   } 

   # print the table with measured parameters
   print_Fang_table $file $filter

   set mosaic [ps_mosaic  $stamps $stampsize]
   display $mosaic "filter $filter"
   regDel $mosaic
}

   echo " "; echo " " 
   echo "Parameters for stars on trailing astrometric chip"
   print_Fang_table $file at


}


# print the table from a Fang file with measured parameters in given filter
# for leading astrometric chip use filter = al, for the trailing one at. 
proc print_Fang_table {file filter {print_hdr 0} } {

   # initialize needed structures
   set tblcol [handleNewFromType TBLCOL]
   if [catch { get_hdu_for_filterset $file params hdu_params}] {
        set hdu_params(r) 2
        set hdu_params(g) 1
   } 

   if {$filter == "al" || $filter == "at"} {
       set name $filter
       set filter r
       # these assume that filters are {r i u z g}
       if {$name == "al"} {
           set hdu(r) [expr $hdu_params(r) - 1]  
       } else {
           set hdu(r) [expr $hdu_params(g) + 1] 
       }
   } else {
      set name $filter
      set hdu($filter) $hdu_params($filter) 
   }

   # read the relevant column from the file and get the sizes
   fitsRead $tblcol $file -binary -hdu $hdu($filter) 
   if {$filter == "al" || $filter == "u" || $print_hdr != 0} {
       hdrPrint $tblcol.hdr
   }
   set list [tblFldGet $tblcol -col 0]
   set Nstars [llength $list]
  
   # print out a nice formatted table with a decent header info
   set line " ===================================================================================================="
   echo " "
   echo $line
   echo " Chip $name"
   echo " star midR midC  rowC  rErr   colC  CErr  Maj  Min   PA    peak    \
           counts    color  c_err  Fit Stat"

   loop i 0 15 {
      set column [tblFldGet $tblcol -col $i]
      loop j 0 $Nstars {
          lappend row($j) [lindex $column $j]          
      }
   }

   loop j 0 $Nstars {
      loop i 0 15 {
          set str [lindex $row($j) $i]
          if {$i == 0 || $i == 1} {   
              set form "%4d"
          }
          if {$i == 2 || $i == 4} {   
              set form "%6.1f"
          }
          if {$i == 3 || $i == 5} {   
              set form "%4.2f"
          }

          if {$i == 6 || $i == 7} {   
              set form "%3.1f"
          }
          if {$i == 8} {   
              set form "%5.1f"
          }
          if {$i == 9} {   
              set form "%8.1f"
          }
          if {$i == 10} {   
              set form "%9.1f"
          }

          if {$i == 11} {   
              set form "%6.2f"
          }

          if {$i == 12} {   
              set form "%5.2f"
          }
          if {$i == 13 || $i == 14 || $i == 15} {   
              set form "%2d"
          }

          if {[catch {set string [format $form $str]}] != 0} {
              set string $str 
          }

          set s($i) [move_blanks $string] 
      }
      set string [format "%5d %4s %4s %6s %4s  %6s %4s  %3s \
                          %3s %5s %8s %9s  %4s %3s %4s %4s" $j \
                          $s(0) $s(1) $s(2) $s(3) $s(4) $s(5) $s(6) \
                          $s(7) $s(8) $s(9) $s(10) $s(11) $s(12) $s(13) $s(14)]
      echo $string 
   }  
   echo $line
   # clean up 
   handleDel $tblcol

}


# plot several qa plots from a list of Fang files to device dev
# files are in $dir, have names $root-[format %4d $n].fit with n1 <= n <= n2
proc plot_fangs {device dir root n1 n2 {outdir ""}} {

global verbose

   if {![info exists verbose]} {set verbose 0}

   # are we making hardcopies?
   if {[string compare $device /PS] == 0} {
       set file "yes";  # it'll be replaced by filename below
   } elseif {[string compare $device /CPS] == 0} {
       set file "yes";  
   } else {
       set file ""
   }

   set Nfields [expr $n2 - $n1 + 1]
   # field number for _each_ star
   set fields_list {}
   # field number for median points
   set frames [vectorExprNew $Nfields] 
   # medians
   foreach filter {u g r i z} { 
      set M($filter) [vectorExprNew $Nfields] 
      set Mc($filter) [vectorExprNew $Nfields] 
      set counts_list($filter) {}
      set color_list($filter) {}
      set colorErr_list($filter) {}
      set Nstars($filter) 0
   }

   #
   # loop over all fang files
   loop n $n1 [expr $n2+1] {
       handleSet $frames.vec<[expr $n - $n1]> $n
       # fang file name
       set n0 [format "%04d" $n]
       set fang_file $dir/$root-$n0.fit
       verb_echo " working on file $fang_file ..."
       # initialize needed structures
       set tblcol [handleNewFromType TBLCOL]
       if [catch { get_hdu_for_filterset $fang_file params hdu_params}] {
          echo "cannot get hdu_params"
          return
       }
       # read the relevant column from the file and get the sizes
       foreach filter {u g r i z} {         
          fitsRead $tblcol $fang_file -binary -hdu $hdu_params($filter) 
          set list [tblFldGet $tblcol -col 10]
          set nn [llength $list]
          incr Nstars($filter) $nn
          # counts
          set counts_list($filter) [concat $counts_list($filter) $list]
      if {$filter == "r"} {
              verb_echo "      $nn elements, total=$Nstars($filter)"
              loop j 0 $nn {
          set fields_list [concat $fields_list [expr 1.0*$n]]
              }
      }
          # colors and color errors
          set columnC [tblFldGet $tblcol -col 11]
          set color_list($filter) [concat $color_list($filter) $columnC]
          set columnCE [tblFldGet $tblcol -col 12]
          set colorErr_list($filter) [concat $colorErr_list($filter) $columnCE]
          # median counts for this frame
          set aux [median_from_list $list $columnCE 0 5] 
          handleSet $M($filter).vec<[expr $n - $n1]> [expr 27.0-2.5*log10(1+abs($aux))]
          # median color for this frame
          set aux [median_from_list $columnC $columnCE 0 5] 
          handleSet $Mc($filter).vec<[expr $n - $n1]> $aux  
       }
       handleDelFromType $tblcol
   }     

   # form vectors for plotting
   set ug [vectorExprNew $Nstars(r)]
   set gr [vectorExprNew $Nstars(r)]
   set ri [vectorExprNew $Nstars(r)]
   set u  [vectorExprNew $Nstars(r)]
   set g  [vectorExprNew $Nstars(r)]
   set r  [vectorExprNew $Nstars(r)]
   set i  [vectorExprNew $Nstars(r)]
   set z  [vectorExprNew $Nstars(r)]
   set fields  [vectorExprNew $Nstars(r)]
   assert { [llength $color_list(u)] == [llength $fields_list] }  
   loop k 0 $Nstars(r) {
      handleSet $ug.vec<$k> [lindex $color_list(u) $k] 
      handleSet $gr.vec<$k> [lindex $color_list(g) $k] 
      handleSet $ri.vec<$k> [lindex $color_list(r) $k] 
      handleSet $u.vec<$k>  [expr 27.0-2.5*log10(1+abs([lindex $counts_list(u) $k]))]
      handleSet $g.vec<$k>  [expr 27.0-2.5*log10(1+abs([lindex $counts_list(g) $k]))]
      handleSet $r.vec<$k>  [expr 27.0-2.5*log10(1+abs([lindex $counts_list(r) $k]))]
      handleSet $i.vec<$k>  [expr 27.0-2.5*log10(1+abs([lindex $counts_list(i) $k]))]
      handleSet $z.vec<$k>  [expr 27.0-2.5*log10(1+abs([lindex $counts_list(z) $k]))]
      handleSet $fields.vec<$k> [lindex $fields_list $k]   
   }  
   set ug_min  0.0
   set ug_max  5.0
   set gr_min -1.0
   set gr_max  3.0
   set ri_min -1.0
   set ri_max  2.0
   set C_min  22.0
   set C_max  10.0
   set f_min [vExtreme $fields min]
   set f_max [vExtreme $fields max]

   # Set up the plot
   set pg [pgstateNew]
   set device [devicename $device $outdir/fangPlots-$root-$n1-$n2.ps]
   pgstateSet $pg -device $device -xfract 0.45 -yfract 0.35 -nxwindow 5 -nywindow 2 -symb 1           
   pgstateOpen $pg      

   # plot u vs. field 
   vPlot $pg $fields $u -xmin $f_min -xmax $f_max -ymin $C_min -ymax $C_max
   pgText $f_min 7.0 "fang files $dir/$root $n1-$n2 (m=27-2.5log10(counts))"
   pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineStyle 1 -lineWidth 1
   vPlot $pg $frames $M(u)
   pgstateSet $pg -isNewplot 1 -isLine 0
   xlabel "field"
   ylabel "u"
   # plot g vs field 
   vPlot $pg $fields $g -xmin $f_min -xmax $f_max -ymin $C_min -ymax $C_max
   pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineStyle 1 -lineWidth 1
   vPlot $pg $frames $M(g)
   pgstateSet $pg -isNewplot 1 -isLine 0
   xlabel "field"
   ylabel "g"
   # plot r vs field 
   vPlot $pg $fields $r -xmin $f_min -xmax $f_max -ymin $C_min -ymax $C_max
   pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineStyle 1 -lineWidth 1
   vPlot $pg $frames $M(r)
   pgstateSet $pg -isNewplot 1 -isLine 0
   xlabel "field"
   ylabel "r"
   # plot i vs field 
   vPlot $pg $fields $i -xmin $f_min -xmax $f_max -ymin $C_min -ymax $C_max
   pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineStyle 1 -lineWidth 1
   vPlot $pg $frames $M(i)
   pgstateSet $pg -isNewplot 1 -isLine 0
   xlabel "field"
   ylabel "i"
   # plot z vs field 
   vPlot $pg $fields $z -xmin $f_min -xmax $f_max -ymin $C_min -ymax $C_max
   pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineStyle 1 -lineWidth 1
   vPlot $pg $frames $M(z)
   pgstateSet $pg -isNewplot 1 -isLine 0
   xlabel "field"
   ylabel "z"
 
   # plot g-r vs. u-g
   vPlot $pg $ug $gr -xmin $ug_min -xmax $ug_max -ymin $gr_min -ymax $gr_max
   xlabel "u-g"
   ylabel "g-r"

   # plot r-i vs. g-r
   vPlot $pg $gr $ri -xmin $gr_min -xmax $gr_max -ymin $ri_min -ymax $ri_max 
   xlabel "g-r"
   ylabel "r-i"

   # plot r vs. r-i (reversed r axis)
   vPlot $pg $ri $r -xmin $ri_min -xmax $ri_max -ymin $C_min -ymax $C_max
   xlabel "r-i"
   ylabel "r"
  
   # plot g-r vs field 
   vPlot $pg $fields $gr -xmin $f_min -xmax $f_max -ymin $gr_min -ymax $gr_max
   pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineStyle 1 -lineWidth 1
   vPlot $pg $frames $Mc(g)
   pgstateSet $pg -isNewplot 1 -isLine 0
   xlabel "field"
   ylabel "g-r"

   # plot r-i vs field 
   vPlot $pg $fields $ri -xmin $f_min -xmax $f_max -ymin $ri_min -ymax $ri_max
   pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 2 -lineStyle 1 -lineWidth 1
   vPlot $pg $frames $Mc(r)
   pgstateSet $pg -isNewplot 1 -isLine 0
   xlabel "field"
   ylabel "r-i"
  
   # finish plot
   endplot $pg

   # clean
   vectorExprDel $ug
   vectorExprDel $gr
   vectorExprDel $ri
   vectorExprDel $fields
   vectorExprDel $frames
   vectorExprDel $u
   vectorExprDel $g
   vectorExprDel $r
   vectorExprDel $i
   vectorExprDel $z
   foreach filter {u g r i z} { 
      vectorExprDel $M($filter)
      vectorExprDel $Mc($filter)
   }
 
}

# return the median of elements in $list, including only the elements with 
# x1 < flag_list < x2 
proc median_from_list {list flag_list x1 x2} {

   set n1 [llength $list] 
   set n2 [llength $flag_list] 
   assert {$n1 == $n2}

   set good {}

   loop i 0 $n1 {
       set el [lindex $list $i]
       set flag [lindex $flag_list $i]
       if {$flag > $x1 && $flag < $x2} {
           set good [concat $good $el]
       }
   }
   set Ngood [llength $good] 
   set vec [vectorExprNew $Ngood]
   loop i 0 $Ngood {
       set el [lindex $good $i]
       handleSet $vec.vec<$i> $el
   }

   set median [vMedian $vec]
   vectorExprDel $vec

   return $median 
}



# moved from ps_pipeline.tcl
proc display_PSmosaic {pstamps stampsize msg {_ids ""}} {
   if {$_ids != ""} {
      upvar $_ids ids
   }

   if {[chainSize $pstamps] > 0} {
      if [info exists ids] { unset ids }
      loop i 0 [chainSize $pstamps] {
	 lappend ids $i
      }
      
      set mosaic [ps_mosaic $pstamps $stampsize 0 ids]
      display $mosaic $msg
      if 0 {
	 set mosname RAWmosaic[pid].fit
	 echo "XXX dumping mosaic with raw stamps to $mosname"
	 regWriteAsFits $mosaic $mosname
      }
      regDel $mosaic
   }
}
     

proc display_stars {filter stampsize starlist ps_rejects wingsize winglist wing_rejects \
                    psf_def_rad display display_psf display_wing display_symmetry} {

      verb_echo 3 \
      "   There are [chainSize $starlist] star1s for filter $filter"

      # info on stamps
      set Ngoodstamps [chainSize $starlist]
      verb_echo 6 \
      "    There are $Ngoodstamps good PS star1s for filter $filter"
      set Nbadstamps [chainSize $ps_rejects]
      verb_echo 6 \
      "    There are $Nbadstamps bad PS star1s for filter $filter"

      # info on wings
      set Ngoodwings [chainSize $winglist]
      verb_echo 6 \
      "    There are $Ngoodwings good wing star1s for filter $filter"
      set Nbadwings [chainSize $wing_rejects]
      verb_echo 6 \
      "    There are $Nbadwings rejected wing star1s for filter $filter"

      # display part
      if {$display} {
         # displaying PS stars
         if {[info exists display_psf] && $display_psf} {
            if {$Ngoodstamps > 0} {
               set mosaic [star1_mosaic $starlist $stampsize ids]
           catch {
          saoReset; saoLabel off
              foreach id $ids {
             saoDrawText [lindex $id 0] [lindex $id 1] \
                 "[lindex [lindex $id 2] 0]:[lindex [lindex $id 2] 2]"
             draw_cross + \
             [expr [lindex $id 0]+[lindex [lindex $id 2] 3]]\
             [expr [lindex $id 1]+[lindex [lindex $id 2] 4]] -e
                     saoDrawCircle [expr [lindex $id 0]+[lindex [lindex $id 2] 3]]\
             [expr [lindex $id 1]+[lindex [lindex $id 2] 4]] \
                         $psf_def_rad -e
              }
           }
               display $mosaic "      corrected $filter stamps (before clipping)" "" 1
               regDel $mosaic;
            }
            # rejects
        if {$display_psf > 1 && $Nbadstamps > 0} {
           set mosaic [star1_mosaic $ps_rejects $stampsize ids]
           if {[catch {
          saoReset; saoLabel off
          foreach id $ids {
             saoDrawText [lindex $id 0] [lindex $id 1] \
             [format "%d:0x%x" [lindex [lindex $id 2] 0] \
                  [lindex [lindex $id 2] 1]]
                     saoDrawCircle [expr [lindex $id 0]+[lindex [lindex $id 2] 3]]\
             [expr [lindex $id 1]+[lindex [lindex $id 2] 4]] \
                         $psf_def_rad -e
          }
          display $mosaic "      $filter stamps rejected for psf fit consideration" "" 1
          regDel $mosaic
           } msg]} {
                   echo $msg
               }
        }
         }
         # displaying wing stars
         if {[info exists display_wing] && $display_wing} {
            # accepted wing stars
            if {$Ngoodwings > 0} {
               set mosaic [star1_mosaic $winglist $wingsize ids]
           catch {
          saoReset; saoLabel off
          foreach id $ids {
             saoDrawText [lindex $id 0] [lindex $id 1] \
                         [lindex [lindex $id 2] 0]
                     saoDrawCircle [expr [lindex $id 0]+[lindex [lindex $id 2] 3]]\
             [expr [lindex $id 1]+[lindex [lindex $id 2] 4]] \
                         $wing_def_rad -e
              }
           }           
               display $mosaic "      corrected $filter wing stars" "" 1
               regDel $mosaic;
            } else {
               echo "there are no good wing stars"
            }
            # rejects
            if {$Nbadwings > 0} {
           set mosaic [star1_mosaic $wing_rejects $wingsize ids]
           catch {
          saoReset; saoLabel off
              foreach id $ids {
             saoDrawText [lindex $id 0] [lindex $id 1] \
                         [format "%d:0x%x" [lindex [lindex $id 2] 0] \
                             [lindex [lindex $id 2] 1]]
                     saoDrawCircle [expr [lindex $id 0]+[lindex [lindex $id 2] 3]]\
             [expr [lindex $id 1]+[lindex [lindex $id 2] 4]] \
                         $wing_def_rad -e
              }
               }
           display $mosaic "      $filter wing stars rejected for psf fit" "" 1
               regDel $mosaic            
        } else {
               echo "there are no rejected wing stars"
            }
         }

     # display symmetry of rejects
     if {[info exists display_symmetry] && $display_symmetry} {
        set mosaic [sym_mosaic $ps_rejects ids]
        if {[exprGet $mosaic.nrow] <= 1} {
           echo No stars were rejected due to asymmetry
        } else {
           saoReset;
           loop i 0 [llength $ids] 2 {
          set id [lindex $ids $i]
          saoDrawText [lindex $id 0] [lindex $id 1] \
            [lindex [lindex $id 2] 0]
          set id [lindex $ids [expr $i+1]]
          saoDrawText [lindex $id 0] [lindex $id 1] \
            [format "0x%x" [lindex [lindex $id 2] 0]]
           }

           display $mosaic "      $filter stars + asymmetry" "" 1
        }
        regDel $mosaic
     }
      }; #end of display part
 
}


proc show_clipping {field display display_PSFstars display_clipping \
			display_profiles starlist stampsize frameinfolist \
			run camCol filter outputDir \
			nsigma_sigma nsigma_QU nsigma_RMS_rad} {
 
   set frame_info [chainElementGetByPos $frameinfolist $field]
   set frame_no [exprGet $frame_info.frame]
   set ngood [exprGet $frame_info.NgoodBasis]
   set nstars [exprGet $frame_info.Nallstars]
   verb_echo 6 \
       "There are $ngood stars used for PSF in filter $filter, frame $frame_no"
   verb_echo 7 \
       "      [expr $nstars - $ngood] stars were clipped.  "
   if {$display} {
      if {[info exists display_PSFstars] && $display_PSFstars} {            
	 set mosaic [star1_mosaic2 $frame_info $starlist $stampsize ids]
	 catch {saoReset;
	    foreach id $ids {
	       saoDrawText [lindex $id 0] [lindex $id 1] \
                   "[lindex [lindex $id 2] 0]:[lindex [lindex $id 2] 2]:[lindex [lindex $id 2] 5]"
	       if {[lindex [lindex $id 2] 5] != 0} {
		  draw_frame2 [lindex $id 0] [lindex $id 1] $stampsize 1  
	       } else {
		  draw_frame [lindex $id 0] [lindex $id 1] $stampsize 1 -i  
	       }
	    }
	 }       
	 display $mosaic " Stamps (framed in yellow) used for PSF determination for filter $filter, frame $frame_no" "" 1
	 if 0 {
	    set mosname mosaic[pid].fit
	    echo "XXX dumping mosaic to $mosname"
	    regWriteAsFits $mosaic $mosname
	 }
	 regDel $mosaic;
      }
   }
   
   if {[info exists display_clipping] && $display_clipping} {
      plot_sigmaStokes /XWINDOW $display_clipping $frame_info \
	  $starlist $run $camCol $outputDir \
	  $nsigma_sigma $nsigma_QU $nsigma_RMS_rad
      if {$display_clipping > 1} {
	 plot_sigmaStokes /PS $display_clipping $frame_info \
	     $starlist $run $camCol $outputDir \
	     $nsigma_sigma $nsigma_QU $nsigma_RMS_rad
      }
   }
   
   if {[info exists display_profiles] && $display_profiles} {            
      plot_profiles /XWINDOW  $starlist $frame_info \
	  $run $camCol $outputDir 
      if {$display_profiles > 1} {            
	 plot_profiles /PS  $starlist $frame_info \
	     $run $camCol $outputDir 

      }
   }
   handleDel $frame_info 
}

###########################################################################

lappend help_procs plot_pspstats

proc plot_pspstats {args} {
   global PSP_STATUS
   
   set opts [list \
		 [list [info level 0] \
		      "Plot up the information stored in the psField files"] \
		 [list <device> STRING "" device \
		      "Output device; /PS (for hardcopy) or /XWINDOW"] \
		 [list <startFrame> INTEGER 0 startframe \
		      "Initial frame to plot"] \
		 [list <endFrame> INTEGER 0 endframe "last frame to plot"] \
		 [list <outputDir> STRING "" outputDir \
		      "Output directory for plots"] \
		 [list <run> INTEGER 0 run "The run in question"] \
		 [list <camCol> INTEGER 0 col "The camCol in question"] \
		 [list <filters> STRING "" filterlist "Filters to plot"] \
		 [list {[frameinfo]} STRING "" _frameinfolist \
		      "Array of FRAME_INFO"] \
		 [list {[flux20_m]} STRING "" _flux20_m \
		      "Array of flux20_m"] \
		 [list {[flux20_c]} STRING "" _flux20_c \
		      "Array of flux20_c"] \
		 [list {[flux20_chi2]} STRING "" _flux20_chi2 \
		      "Array of flux20_chi2"] \
		 [list {[lbiasOEdiff]} STRING "" lbiasOEdiffList \
		      "Left Odd/Even bias values"] \
		 [list {[rbiasOEdiff]} STRING "" rbiasOEdiffList \
		      "Right Odd/Even bias values"] \
		 [list -plots INTEGER 0x0 desired_plots \
		      "Plots to make; e.g. 0x5 for 1st and 3rd (0 => all)"] \
		 [list -rerun STRING "" rerun "Rerun number"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if { $_frameinfolist != ""} {
      upvar $_frameinfolist frameinfolist
   }
   if { $_flux20_m != ""} {
      upvar $_flux20_m flux20_m
   }
   if { $_flux20_c != ""} {
      upvar $_flux20_c flux20_c
   }
   if { $_flux20_chi2 != ""} {
      upvar $_flux20_chi2 flux20_chi2
   }

   set runstr  [format "%06d" $run]
   set vtmp [vectorExprNew 0]
   set CTfile $outputDir/psCT-$runstr-$col.fit

   # get DSCALE
   set field0str [format "%04d" $startframe]
   set psField0 $outputDir/psField-$runstr-$col-$field0str.fit
   set hdr [hdrReadAsFits [hdrNew] $psField0]
   set bias_scale [hdrGetAsInt $hdr DSCALE]
   set filters [string trim [hdrGetAsAscii $hdr FILTERS]]
   hdrDel $hdr

   if {$filterlist == "all"} {
      set filterlist $filters 
   }
   set nfilters [llength $filterlist]
   set ctimes [read_ctimes $CTfile $filterlist]

   # get chain of calib1byframe from psField file
   set psFieldroot $outputDir/psField-$runstr-$col-
   set cframes [get_cal_chain_from_psField $psFieldroot \
		    $startframe $endframe]
   set Nframes [chainSize $cframes]

   
   # Get frame id's
   set frames [vFromChain $cframes field]
   set first_frame [exprGet $frames.vec<0>]
   set last_frame [exprGet $frames.vec<[expr $Nframes - 1]>]
   # remove not needed elements from cframes
   set delta [expr $startframe - $first_frame]  
   if {$delta > 0} {
      loop i 0 $delta {
	 chainElementRemByPos $cframes 0
      }
   }
   set delta [expr $last_frame - $endframe]
   if {$delta > 0} {
      loop i 0 $delta {
	 set nlast [expr [chainSize $cframes] - 1]
	 chainElementRemByPos $cframes $nlast
      }
   }
   vectorExprDel $frames    
   set frames [vFromChain $cframes field]
   vNameSet $frames ""
   set date [getdate]
   set runstr [format %06d $run]

   # For each color:
   loop i 0 $nfilters {
      if [catch {

	 set filter [lindex $filterlist $i] 
	 
	 # get filter index 
	 set cal [chainElementGetByPos $cframes 0]
	 set j [lsearch $filters $filter] 
	 assert { $j > -1 }
	 set cal_filter [exprGet $cal.calib<$j>->filter<0>]

	 # do we need to remove dummy elements from frameinfolist 
	 if [info exists frameinfolist($filter)] {
	    set frame_info [chainElementGetByPos $frameinfolist($filter) 0]
	    set n1 [expr $startframe - [exprGet $frame_info.frame]]
	    set n2 [expr $n1 + $Nframes - 1]
	    set Sframeinfolist($filter) [chainExtract $frameinfolist($filter) $n1 $Nframes]
	 }

	 if {$lbiasOEdiffList != "" && $rbiasOEdiffList != ""} {
	    set lbiasOEdiff [lindex $lbiasOEdiffList $i]
	    set rbiasOEdiff [lindex $rbiasOEdiffList $i]
	 } else {
	    set lbiasOEdiff ""
	    set rbiasOEdiff ""
	 }


	 
	 ###################
	 #  PLOTTING PART  #
	 ###################


	 ######################################################################
	 # PLOT 1
	 if {$desired_plots == 0 || ($desired_plots & 0x1)} {
	    # Set up the plot and label x-axes
	    set pg [pgstateNew]
	    set deviceA [devicename $device $outputDir/psPlots1-$runstr-$filter$col.ps]
	    pgstateSet $pg -device $deviceA \
		-xfract 0.25 -yfract 0.35 -nxwindow 2 -nywindow 3 -symb 7 -isLine 0   
	    pgstateOpen $pg
	    
	    topLabel 20 "PSP p.1, frames: $startframe-$endframe" \
		$run $col $date $filter $rerun

	    # Plot PSP return code from each field
	    set status [cframes2status $cframes $j]
	    set psp_status [cframes2status $cframes -1]

	    foreach v "$status $psp_status" {
               vectorExprSet $v "$v == $PSP_STATUS(EXTENDED_KL) ? 0.5 : $v"
               vectorExprSet $v "$v >= $PSP_STATUS(SPARSE) ? -0.5 : $v"
	    }
	    vNameSet $status "psp_status, status"
	    vNameSet $psp_status ""
	    pgstateSet $pg -icMark 2
	    vPlot $pg $frames $status \
		-ymin -1 -ymax 3.99 -xmin $startframe -xmax $endframe
	    pgstateSet $pg -isLine 1  -isNewplot 0 
	    vPlot $pg $frames $psp_status \
		-ymin -1 -ymax 3.99 -xmin $startframe -xmax $endframe


	    titlePlot "STATUS: 0=OK, 1=lin, 2=const, 3=no PSF" 40
	    vectorExprDel $status
	    vectorExprDel $psp_status

	    # Plot sky
	    set sky [vFromChain $cframes calib<$j>->sky]
	    vNameSet $sky ""
	    pgstateSet $pg -isLine 1 -isNewplot 1
	    vPlot $pg $frames $sky -ymin [vExtreme $sky min] \
		-ymax [vExtreme $sky max] -xmin $startframe -xmax $endframe
	    titlePlot "skyTypical (ADU)" 40
	    xlabel Field
	    vectorExprDel $sky


	    #
	    # Plot number of stars included in average PSF for each frame
	    #
	    if [info exists frameinfolist($filter)] {
	       set nphoto [vFromChain $Sframeinfolist($filter) Nphoto]
	       vNameSet $nphoto ""
	       set ncoeffs [vFromChain $Sframeinfolist($filter) NgoodKLcoeffs]
	       vNameSet $ncoeffs ""
	       set max [vExtreme $nphoto max]
	       set max1 [vExtreme $ncoeffs max]
	       if {$max < $max1} {set max $max1}      

	       # Nphoto
	       pgstateSet $pg -isLine 1 -isNewplot 1
	       vPlot $pg $frames $nphoto \
		   -ymin 0 -ymax $max -xmin $startframe -xmax $endframe
	       # Ncoeffs
	       pgstateSet $pg -isLine 0 -isNewplot 0 -symb 4 
	       vPlot $pg $frames $ncoeffs \
		   -ymin 0 -ymax $max -xmin $startframe -xmax $endframe

	       pgstateSet $pg -isLine 0 -isNewplot 0

	       vectorExprDel $ncoeffs
	       vectorExprDel $nphoto
	       xlabel Field
	       titlePlot "Ncoeffs (symb), Nphoto" 40
	    } else {
	       # put a dummy plot in the same place
	       set dummy [vectorExprEval 1]
	       pgstateSet $pg -isNewplot 1
	       vPlot $pg $dummy $dummy
	       titlePlot "N not available" 40
	       vectorExprDel $dummy         
	    }


	    #
	    # plot psfs
	    #
	    pgstateSet $pg -isNewplot 1 -isLine 1     
	    set PSFwidth [vFromChain $cframes calib<$j>->psf->width]
	    set min [vExtreme $PSFwidth min]
	    set max [vExtreme $PSFwidth max]
	    # distribution of stellar width
	    if [info exists frameinfolist($filter)] {
	       set width_med [vFromChain $Sframeinfolist($filter) width_med]
	       set min1 [vExtreme $width_med min]
	       set max1 [vExtreme $width_med max]
	       if {$min > $min1} {set min $min1}
	       if {$max < $max1} {set max $max1}
	    }

	    if {$max > 15.0} {
	       set max 10.0
	    }
	    if {$min < 0.8} {
	       pgstateSet $pg -icMark 2
	    }
	    vNameSet $PSFwidth ""
	    vPlot $pg $frames $PSFwidth -xmin $startframe -xmax $endframe \
		-ymin $min -ymax $max
	    pgstateSet $pg -isLine 0 -isNewplot 0 -symb 4
	    if [info exists frameinfolist($filter)] {
	       vPlot $pg $frames $width_med -xmin $startframe -xmax $endframe \
		   -ymin $min -ymax $max
	       vectorExprDel $width_med
	       titlePlot "PSF width, center:line, med:symb (arcsec)" 40
	    } else {
	       pgstateSet $pg -isNewplot 1 
	       titlePlot "center PSF width (arcsec) " 40
	    }

	    vectorExprDel $PSFwidth
	    xlabel Field

	    # Plot error due to PSF mismatch
	    # set psf_err [vFromChain $cframes calib<$j>->psf_ap_correctionErr]
            set psf_err [vFromChain $cframes calib<$j>->ap_corr_runErr]
	    vNameSet $psf_err ""
            # bias between ap5 and psf mags
            set apCorr5psf [vFromChain $cframes calib<$j>->psf_ap_correctionBias]
	    # bias between ap5 and ap7
	    if {[info exists frameinfolist($filter)]} {
	       set apCorr5to7 [vFromChain $Sframeinfolist($filter) wingcorr]
	    } else {
               set apCorr5to7 [vectorExprEval 999+$apCorr5psf]
            }
	    vNameSet $apCorr5psf ""
	    vNameSet $apCorr5to7 ""

	    set min -0.09; set max 0.15;
	    pgstateSet $pg -isNewplot 1 -isLine 1 -symb 4 -icMark 2
	    vPlot $pg $frames $psf_err -ymin $min  \
		-ymax $max -xmin $startframe -xmax $endframe

	    pgstateSet $pg -isNewplot 0 -isLine 0 -icMark 2 -symb 4
	    vPlot $pg $frames $apCorr5psf -ymin $min  \
		     -ymax $max -xmin $startframe -xmax $endframe
	    pgstateSet $pg -isNewplot 0 -isLine 0 -icMark 4 -symb 5
	    vPlot $pg $frames $apCorr5to7 -ymin $min  \
		     -ymax $max -xmin $startframe -xmax $endframe

 
	    titlePlot "err (L), b5psf (Ro), b5to7 (Bx)" 40
	    xlabel Field
	    vectorExprDel $psf_err
	    vectorExprDel $apCorr5psf
	    vectorExprDel $apCorr5to7


	    # Plot center aperture correction
	    if [info exists frameinfolist($filter)] {
	       pgstateSet $pg -isNewplot 1 -symb 4  -isLine 0 -icMark 2
	       set ap_corr [vFromChain $Sframeinfolist($filter) ap_corr]
	       set apCorr_max [vFromChain $Sframeinfolist($filter) apCorr_max]
	       set apCorr_min [vFromChain $Sframeinfolist($filter) apCorr_min]
	       vNameSet $ap_corr ""
	       set min 0.8
	       set max 1.2
	       vPlot $pg $frames $ap_corr -ymin $min  \
		   -ymax $max -xmin $startframe -xmax $endframe
	       pgstateSet $pg -isNewplot 0 -isLine 1
	       vPlot $pg $frames $apCorr_min -ymin $min  \
		   -ymax $max -xmin $startframe -xmax $endframe
	       pgstateSet $pg -isNewplot 0
	       vPlot $pg $frames $apCorr_max -ymin $min  \
		   -ymax $max -xmin $startframe -xmax $endframe
	       titlePlot "PSF: ap. corr. (frame center, min, max)" 40
	       xlabel Field
	       vectorExprDel $ap_corr
	       vectorExprDel $apCorr_min
	       vectorExprDel $apCorr_max
	    } else {
	       # put a dummy plot in the same place
	       set dummy [vectorExprEval 1]
	       pgstateSet $pg -isNewplot 1 -isLine 1
	       vPlot $pg $dummy $dummy
	       vectorExprDel $dummy         
	       titlePlot "ap. corr.: not available " 40
	    }

	    
	    # cleanup
	    endplot $pg
	 }



	 ######################################################################
	 # PLOT 2
	 if {$desired_plots == 0 || ($desired_plots & 0x2)} {
	    # Set up the plot and label x-axes
	    set pg [pgstateNew]
	    set deviceA [devicename $device $outputDir/psPlots2-$runstr-$filter$col.ps]
	    pgstateSet $pg -device $deviceA \
		-xfract 0.25 -yfract 0.35 -nxwindow 2 -nywindow 3 -symb 7 -isLine 1   
	    pgstateOpen $pg
	    
	    topLabel 20 "PSP p.2, frames: $startframe-$endframe" $run $col $date $filter $rerun


	    if {0} {
	       pgstateSet $pg -isNewplot 1 -isLine 1     
	       set PSFwidth [vFromChain $cframes calib<$j>->psf->width]
	       set min [vExtreme $PSFwidth min]
	       set max [vExtreme $PSFwidth max]
	       if {$max > 15.0} {
		  set max 10.0
	       }
	       if {$min < 0.8} {
		  pgstateSet $pg -icMark 2
	       }
	       vNameSet $PSFwidth ""
	       vPlot $pg $frames $PSFwidth -xmin $startframe -xmax $endframe \
		   -ymin $min -ymax $max
	       vectorExprDel $PSFwidth
	       titlePlot "PSF width (arcsec)" 40
	       xlabel Field
	    }

	    # Plot bias
	    set lbias [vFromChain $cframes calib<$j>->lbias]
	    vectorExprSet $lbias $lbias/$bias_scale
	    set rbias [vFromChain $cframes calib<$j>->rbias]
	    vectorExprSet $rbias $rbias/$bias_scale
	    vNameSet $lbias ""

	    set min [vExtreme $lbias min] 
	    set max [vExtreme $lbias max] 
	    set rmin [vExtreme $rbias min] 
	    set rmax [vExtreme $rbias max] 
	    if {$rmin < $min} {
	       set min $rmin
	    }
	    if {$rmax > $max} {
	       set max $rmax
	    } 

	    if {$lbiasOEdiff != "" && $lbiasOEdiff != ""} {
	       vectorExprSet $lbiasOEdiff $lbiasOEdiff/$bias_scale
	       vectorExprSet $rbiasOEdiff $rbiasOEdiff/$bias_scale
	       set lminOE [vExtreme $lbiasOEdiff min] 
	       set lmaxOE [vExtreme $lbiasOEdiff max] 
	       set rminOE [vExtreme $rbiasOEdiff min] 
	       set rmaxOE [vExtreme $rbiasOEdiff max] 
	       if {$lminOE < $min} {
		  set min $lminOE
	       }
	       if {$lmaxOE > $max} {
		  set max $lmaxOE
	       } 
	       if {$rminOE < $min} {
		  set min $rminOE
	       }
	       if {$rmaxOE > $max} {
		  set max $rmaxOE
	       } 
	    }

	    pgstateSet $pg -isNewplot 1  -isLine 1
	    vPlot $pg $frames $lbias -ymin $min -ymax $max  \
		-xmin $startframe -xmax $endframe
	    pgstateSet $pg -isNewplot 0 -isLine 1
	    vPlot $pg $frames $rbias -ymin $min -ymax $max  \
		-xmin $startframe -xmax $endframe
	    if {$lbiasOEdiff != "" && $lbiasOEdiff != ""} {
	       pgstateSet $pg -isNewplot 0 -symb 2 -isLine 0
	       vPlot $pg $frames $rbiasOEdiff -ymin $min -ymax $max  \
		   -xmin $startframe -xmax $endframe
	       pgstateSet $pg -isNewplot 0 -symb 5
	       vPlot $pg $frames $lbiasOEdiff -ymin $min -ymax $max  \
		   -xmin $startframe -xmax $endframe
	       pgstateSet $pg -isNewplot 1
	       titlePlot "Bias drift (ADU) (line), OEdiff (*)" 40
	    } else {
	       pgstateSet $pg -isNewplot 1
	       titlePlot "Bias drift (ADU) (line)" 40
	    }
	    xlabel Field

	    vectorExprDel $lbias
	    vectorExprDel $rbias

	    
	    # plot sigma1, sigma2
	    pgstateSet $pg -isNewplot 1 -symb 7 -isLine 0     
	    set sigmax1 [vFromChain $cframes calib<$j>->psf->sigmax1]
	    set sigmax2 [vFromChain $cframes calib<$j>->psf->sigmax2]
	    # pixels -> arcsec
	    vectorExprSet $sigmax1 0.4*$sigmax1 
	    vectorExprSet $sigmax2 0.4*$sigmax2 
	    set max [vExtreme $sigmax2 max]
	    # 2-Gaussian fit
	    set sigmax1_2G [vFromChain $cframes calib<$j>->psf->sigma1_2G]
	    set sigmax2_2G [vFromChain $cframes calib<$j>->psf->sigma2_2G]
	    # pixels -> arcsec
	    vectorExprSet $sigmax1_2G 0.4*$sigmax1_2G 
	    vectorExprSet $sigmax2_2G 0.4*$sigmax2_2G 
	    if {$max < [vExtreme $sigmax2_2G max]} {
	       set max [vExtreme $sigmax2_2G max]
	    }
	    if {$max > 15.0} {
	       set max 10.0
	    }
	    vNameSet $sigmax1 ""
	    if {[vExtreme $sigmax1 min] < 0.8} {
	       pgstateSet $pg -icMark 2 -symb 3  -isLine 0
	    }
	    vPlot $pg $frames $sigmax1 -xmin $startframe -xmax $endframe \
		-ymin 0.0 -ymax $max
	    pgstateSet $pg -isNewplot 0 -symb 4 -icMark 1
	    vPlot $pg $frames $sigmax2 -xmin $startframe -xmax $endframe \
		-ymin 0.0 -ymax $max

	    titlePlot "PSF: \\gs\\d1\\u and \\gs\\d2\\u (arcsec)" 40
	    xlabel Field
	    vectorExprDel $sigmax1
	    vectorExprDel $sigmax2

	    # repeat for 2-Gaussian fit in lines
	    pgstateSet $pg -isNewplot 0 -isLine 1 -icMark 2     
	    vPlot $pg $frames $sigmax1_2G -xmin $startframe -xmax $endframe \
		-ymin 0.0 -ymax $max
	    pgstateSet $pg -isNewplot 0  -icMark 1
	    vPlot $pg $frames $sigmax2_2G -xmin $startframe -xmax $endframe \
		-ymin 0.0 -ymax $max
	    vectorExprDel $sigmax1_2G
	    vectorExprDel $sigmax2_2G

	    
	    # Plot psf->b
	    set b [vFromChain $cframes calib<$j>->psf->b]
	    set b2G [vFromChain $cframes calib<$j>->psf->b_2G]
	    extremes $b min_b max_b 0.2
	    extremes $b2G min_b2G max_b2G 0.2
	    if {$min_b2G < $min_b} {set min_b $min_b2G}
	    if {$max_b2G > $max_b} {set max_b $max_b2G}

	    pgstateSet $pg -isNewplot 1  -isLine 0 -symb 7 -icMark 2 
	    vNameSet $b ""
	    vPlot $pg $frames $b -ymin 0.0 -ymax $max_b -xmin $startframe -xmax $endframe
	    pgstateSet $pg -icMark 1
	    titlePlot "PSF: amplitude ratio b" 40
	    xlabel Field
	    vectorExprDel $b

	    # repeat for 2-Gaussian fit in dashed lines
	    pgstateSet $pg -isNewplot 0 -isLine 1 -icMark 1   
	    vPlot $pg $frames $b2G -ymin 0.0 -ymax $max_b -xmin $startframe -xmax $endframe
	    vectorExprDel $b2G


	    # Plot psf->beta
	    set beta [vFromChain $cframes calib<$j>->psf->beta]
	    vNameSet $beta ""
	    pgstateSet $pg -isNewplot 1 -isLine 0 -symb 7 
	    extremes $beta min_b max_b 0.2
	    vPlot $pg $frames $beta -ymin $min_b -ymax $max_b -xmin $startframe \
		-xmax $endframe
	    titlePlot "PSF: wing exponent \\gb" 40
	    xlabel Field
	    vectorExprDel $beta

	    # Plot psf->p0
	    pgstateSet $pg -isNewplot 1  -isLine 0 -symb 7 
	    set p0 [vFromChain $cframes calib<$j>->psf->p0]
	    set b [vFromChain $cframes calib<$j>->psf->b]   
	    vectorExprSet $p0 1000*$p0/(1+$b)   
	    vNameSet $p0 ""
	    extremes $p0 min_p max_p 0.2
	    vPlot $pg $frames $p0 -ymin $min_p -ymax $max_p -xmin $startframe -xmax $endframe
	    titlePlot "PSF: 1000*p0/(1+b)" 40
	    xlabel Field      
	    vectorExprDel $p0
	    vectorExprDel $b 

	    # Plot I[r(wing=core)]
	    pgstateSet $pg -isNewplot 1  -isLine 0 -symb 7 
	    set rwc [vectorExprNew [chainSize $cframes]]
	    get_rwc $cframes $i rwc
	    vectorExprSet $rwc 100*$rwc
	    vNameSet $rwc ""
	    extremes $rwc min_p max_p 0.2
	    vPlot $pg $frames $rwc -ymin 0.0 -ymax $max_p -xmin $startframe -xmax $endframe
	    titlePlot "PSF: 100*i(wing=core)" 40
	    xlabel Field      
	    vectorExprDel $rwc

	    # cleanup
	    endplot $pg
	 }
	 
	 ######################################################################
	 # PLOT 3
	 # Set up the plot and label x-axes
	 if {$desired_plots == 0 || ($desired_plots & 0x4)} {
	    set pg [pgstateNew]
	    set deviceA [devicename $device $outputDir/psPlots3-$runstr-$filter$col.ps]
	    pgstateSet $pg -device $deviceA \
		-xfract 0.25 -yfract 0.35 -nxwindow 2 -nywindow 3 -symb 7 -isLine 1   
	    pgstateOpen $pg
	    topLabel 20 "PSP p.3, frames: $startframe-$endframe" $run $col $date $filter $rerun
	    pgstateSet $pg -isNewplot 1 -isLine 1     

	    # Plot profile data
	    loop k 0 4 {
	       set i1 [expr $k*2+1]
	       set i2 [expr $k*2+2]
	       set p1 [vFromChain $cframes calib<$j>->prof->med<$i1>] 
	       set p2 [vFromChain $cframes calib<$j>->prof->med<$i2>] 
	       set s1 [vFromChain $cframes calib<$j>->prof->sig<$i1>] 
	       set s2 [vFromChain $cframes calib<$j>->prof->sig<$i2>] 

	       set fac [format "%[expr $k+1].0f"  [expr pow(10,$k)]]
	       vectorExprSet $p1 0.001*$fac*$p1    
	       vectorExprSet $p2 0.001*$fac*$p2 
	       vectorExprSet $s1 0.001*$fac*$s1    
	       vectorExprSet $s2 0.001*$fac*$s2 
	       
	       set min [vExtreme $p1 min]
	       set max [vExtreme $p1 max]
	       set min1 [vExtreme $p2 min]
	       set max1 [vExtreme $p2 max]
	       if {$min1 < $min} {
		  set min $min1
	       }
	       if {$max1 > $max} {
		  set max $max1
	       }
	       if [info exists frameinfolist($filter)] {
		  set t1 [vFromChain $Sframeinfolist($filter) ptarr<$i1>] 
		  set t2 [vFromChain $Sframeinfolist($filter) ptarr<$i2>] 
		  vectorExprSet $t1 0.001*$fac*$t1    
		  vectorExprSet $t2 0.001*$fac*$t2     
		  set min1 [vExtreme $t1 min]
		  set max1 [vExtreme $t1 max]
		  if {$min1 < $min} {
		     set min $min1
		  }
		  if {$max1 > $max} {
		     set max $max1
		  }
		  set min1 [vExtreme $t2 min]
		  set max1 [vExtreme $t2 max]
		  if {$min1 < $min} {
		     set min $min1
		  }
		  if {$max1 > $max} {
		     set max $max1
		  }    
	       }
	       
	       vNameSet $p1 ""
	       pgstateSet $pg -icMark 3 -isNewplot 1 -isLine 0 -symb 4
	       vPlot $pg $frames $p1 -vectorYErr $s1 -xmin $startframe -xmax $endframe \
		   -ymin 0.0 -ymax $max 
	       pgstateSet $pg -isNewplot 0 -symb 7 -icMark 1
	       vPlot $pg $frames $p2 -vectorYErr $s2 -xmin $startframe -xmax $endframe \
		   -ymin 0.0 -ymax $max
	       vectorExprDel $p1
	       vectorExprDel $p2
	       vectorExprDel $s1
	       vectorExprDel $s2

	       # best fit
	       if [info exists frameinfolist($filter)] {
		  pgstateSet $pg -icMark 2 -isNewplot 0 -isLine 1 -icLine 1
		  vPlot $pg $frames $t1 -xmin $startframe -xmax $endframe \
		      -ymin 0.0 -ymax $max 
		  pgstateSet $pg -isNewplot 0  -icMark 2
		  vPlot $pg $frames $t2 -xmin $startframe -xmax $endframe \
		      -ymin 0.0 -ymax $max
		  vectorExprDel $t1
		  vectorExprDel $t2
	       }

	       if {$k==0} {
		  titlePlot "PSF: composite profile, points $i1 and $i2" 40
	       } else {
		  titlePlot "PSF: points $i1 and $i2 (x $fac)" 40
	       }
	       xlabel Field

	    }
	    if [catch {

	       # Plot patch calibration (flux20 uncorrected for extinction vs. time)
	       set nctimes [exprGet $ctimes.nElements]
	       pgstateSet $pg -isNewplot 1  
	       if {$nctimes > 0} {
		  set Ngoodcalibs 0
		  # some patches might not have all colors, count good ones
		  loop ii 0 $nctimes {
		     set onecalib [chainElementGetByPos $ctimes $ii]
		     if {[exprGet $onecalib.calib<$i>->flux20] > 0 &&
			 [exprGet $onecalib.calib<$i>->flux20Err] > 0 } {
			incr Ngoodcalibs
		     }
		  }
                  if {$Ngoodcalibs <= 0} {continue}
		  set times [vectorExprNew $Ngoodcalibs] 
		  set pflux20 [vectorExprNew $Ngoodcalibs] 
		  set pflux20Err [vectorExprNew $Ngoodcalibs] 
		  # populate plotting vectors
		  loop ii 0 $Ngoodcalibs {
		     set onecalib [chainElementGetByPos $ctimes $ii]
		     if {[exprGet $onecalib.calib<$i>->flux20] > 0 &&
			 [exprGet $onecalib.calib<$i>->flux20Err] > 0} {
			set mjd [exprGet $onecalib.mjd]
			set frac_mjd [expr $mjd - [int $mjd]] 
			handleSet $times.vec<$ii> $frac_mjd 
			handleSet $pflux20.vec<$ii> [exprGet $onecalib.calib<$i>->flux20]
			handleSet $pflux20Err.vec<$ii> [exprGet $onecalib.calib<$i>->flux20Err] 
		     } else {
			handleSet $times.vec<$ii> -1000.0
			handleSet $pflux20.vec<$ii> 0.0
			handleSet $pflux20Err.vec<$ii> 0.0
		     }
		  } 

		  vectorExprSet $pflux20 $pflux20/1000.0 
		  vectorExprSet $pflux20Err $pflux20Err/1000.0 
		  vNameSet $pflux20 ""
		  
		  # these are the values for each patch
		  vPlot $pg $times $pflux20 -vectorYErr $pflux20Err \
		      -xmax [vExtreme $times max] \
		      -xmin [vExtreme $times min] \
		      -ymin [vExtreme [vectorExprSet $vtmp $pflux20-$pflux20Err] min] \
		      -ymax [vExtreme [vectorExprSet $vtmp $pflux20+$pflux20Err] max]
		  titlePlot "PT calibration (kADU)" 40
		  
		  # plot error bars
		  vPlotErr $times $pflux20 $pflux20Err $Ngoodcalibs $pg 
		  vectorExprDel $pflux20Err

		  # plot calibration
		  # this is the fit to flux20(t) 
		  if {1} {
		     set a [vectorExprNew 2]
		     set tmin [vExtreme $times min]
		     set tmax [vExtreme $times max]
		     set edge 0.02
		     set tL [expr $tmin + $edge *($tmax - $tmin)]
		     set tR [expr $tmax - $edge *($tmax - $tmin)]
		     handleSet $a.vec<0> $tL
		     handleSet $a.vec<1> $tR
		     set b [vectorExprNew 2]
		     pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3
		     if [info exists flux20_m($i)] { 
			handleSet $b.vec<0> [expr ($tL * $flux20_m($i) + $flux20_c($i))/1000]  
			handleSet $b.vec<1> [expr ($tR * $flux20_m($i) + $flux20_c($i))/1000] 
			vPlot $pg $a $b
		     }
		     vectorExprDel $a; vectorExprDel $b;  
		  }   
		  vectorExprDel $times    
	       } else {
		  # put a dummy plot in the same place
		  set pflux20 [vectorExprEval 1]
		  vPlot $pg $pflux20 $pflux20
		  titlePlot "No mtPatch available" 40
	       }
	       xlabel "Time (mjd fraction)"
	       vectorExprDel $pflux20; 
	       # catch for Plotting patch calibration
	    } msg] {
	       echo "ppc: $msg"
	    }


	    # Plot flux20 for each frame
	    set flux20 [vFromChain $cframes calib<$i>->flux20]
	    vectorExprSet $flux20 $flux20/1000.0 
	    vNameSet $flux20 ""
	    set flux20Err [vFromChain $cframes calib<$i>->flux20Err]
	    vectorExprSet $flux20Err $flux20Err/1000.0 
	    set fmin [vExtreme [vectorExprSet $vtmp $flux20-$flux20Err] min]
	    set fmax [vExtreme [vectorExprSet $vtmp $flux20+$flux20Err] max]
	    pgstateSet $pg -isNewplot 1 -isLine 1 -icLine 1 -lineStyle 1 
	    vPlot $pg $frames $flux20 -vectorYErr $flux20Err \
		-xmin $startframe -xmax $endframe -ymin $fmin -ymax $fmax
	    titlePlot "Flux at mag20 (kADU)" 40
	    xlabel Field
	    vectorExprDel $flux20; vectorExprDel $flux20Err
	    # mark the positions of calibration patches 
	    if [info exists frameinfolist($filter)] {
	       # vertical lines
	       set nframes [chainSize $Sframeinfolist($filter)]
	       set edge 0.04
	       set f20min [expr $fmin + $edge*($fmax - $fmin)]
	       set f20max [expr $fmax - $edge*($fmax - $fmin)]
	       loop i 0 $nframes { 
		  set frameinfo [chainElementGetByPos $Sframeinfolist($filter) $i]
		  set flux20 [exprGet $frameinfo.patch_flux20]
		  if {$flux20 > 0} {
		     set frame [expr $startframe + $i]
		     set a [vectorExprNew 2]
		     handleSet $a.vec<0> $frame
		     handleSet $a.vec<1> $frame
		     set b [vectorExprNew 2]
		     handleSet $b.vec<0> $f20min
		     handleSet $b.vec<1> $f20max
		     pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3
		     vPlot $pg $a $b
		     vectorExprDel $a; vectorExprDel $b; 
		  }
	       }
	    }

	    # cleanup
	    endplot $pg
	 }

	 # free temporary chain
	 if [info exists Sframeinfolist($filter)] {
	    chainDel $Sframeinfolist($filter)
	 }

	 # catch for the filter loop
      } msg] {
	 echo "filter $filter: $msg"
      }
   }; #end of loop over filters

   vectorExprDel $frames
   vectorExprDel $vtmp

   chainDestroy $ctimes calib1bytimeDel
   chainDestroy $cframes calib1byframeDel
}


# given a chain of calib1byframe, and filter index ifilter, 
# return a vector showing PSP_STATUS
# for ifilter < 0 return psp_status
proc cframes2status {cframes ifilter} {

   set status [vectorExprNew [chainSize $cframes]]

   loop i 0 [chainSize $cframes] {
      set el [chainElementGetByPos $cframes $i]
      if {$ifilter < 0} {
          set aux $el.psp_status
      } else {
          set aux $el.calib<$ifilter>->status
      }
      handleSet $status.vec<$i> [exprGet (int)$aux]
   }

   return $status
}



# given a chain of calib1byframe, and filter index ifilter, 
# return a vector of beta determined by fitting a power law
# to median composite profile for points {6, 7, 8}
proc cframes2beta {cframes ifilter} {

   set beta [vectorExprNew [chainSize $cframes]]
   initProfileExtract
   # fake central pixel
   set radii "0.0 [profileRadii]" 

   loop i 0 [chainSize $cframes] {
      set el [chainElementGetByPos $cframes $i]
      set comp_prof $el.calib<$ifilter>->prof->med      
      handleSet $beta.vec<$i> [fit_beta_from_prof [list 6 7 8] $radii $comp_prof]
   }

   return $beta
}


   

###########################################################################

ftclHelpDefine photo plot_PTcalib \
    "
Plots up the PS calibration information stored in the psField 
and psCT files
    
   Usage: plot_PTcalib device startField endField outputDir run \
                                                camCol filterlist

Device is either /PS (for hardcopy) or /XWINDOW. 
The postscript plots will be written to outputDir."


proc plot_PTcalib {device startframe endframe outputDir run col filterlist \
                     {_flux20_m ""} {_flux20_c ""} {_flux20_chi2 ""}} {
   global rerun
 
   if { $_flux20_m != ""} {
      upvar $_flux20_m flux20_m
   }
   if { $_flux20_c != ""} {
      upvar $_flux20_c flux20_c
   }
   if { $_flux20_chi2 != ""} {
      upvar $_flux20_chi2 flux20_chi2
   }

   set runstr  [format "%06d" $run]
   set vtmp [vectorExprNew 0]
   set CTfile $outputDir/psCT-$runstr-$col.fit

   # get DSCALE
   set field0str [format "%04d" $startframe]
   set psField0 $outputDir/psField-$runstr-$col-$field0str.fit
   set hdr [hdrReadAsFits [hdrNew] $psField0]
   set bias_scale [hdrGetAsInt $hdr DSCALE]
   set filters [string trim [hdrGetAsAscii $hdr FILTERS]]
   hdrDel $hdr

   if {$filterlist == "all"} {
      set filterlist $filters 
   }
   set nfilters [llength $filterlist]
   set ctimes [read_ctimes $CTfile $filterlist]

   # get chain of calib1byframe from psField file
   set psFieldroot $outputDir/psField-$runstr-$col-
   set cframes [get_cal_chain_from_psField $psFieldroot  \
                                              $startframe $endframe]
   set Nframes [chainSize $cframes]

   
   # Get frame id's
   set frames [vFromChain $cframes field]
   set first_frame [exprGet $frames.vec<0>]
   set last_frame [exprGet $frames.vec<[expr $Nframes - 1]>]
   # remove not needed elements from cframes
   set delta [expr $startframe - $first_frame]  
   if {$delta > 0} {
      loop i 0 $delta {
          chainElementRemByPos $cframes 0
      }
   }
   set delta [expr $last_frame - $endframe]  
   if {$delta > 0} {
      loop i 0 $delta {
          set nlast [expr [chainSize $cframes] - 1]
          chainElementRemByPos $cframes $nlast
      }
   }
   vectorExprDel $frames    
   set frames [vFromChain $cframes field]
   vNameSet $frames ""
   set date [getdate]
   set runstr [format %06d $run]

   # For each color:
   loop i 0 $nfilters {
      if [catch {
      
      set filter [lindex $filterlist $i] 
      
      # get filter index 
      set cal [chainElementGetByPos $cframes 0]
      set j [lsearch $filters $filter] 
      assert { $j > -1 }
      set cal_filter [exprGet $cal.calib<$j>->filter<0>]

      # Set up the plot and label x-axes
      set pg [pgstateNew]
      set deviceA [devicename $device $outputDir/psPlots4-$runstr-$filter$col.ps]
      pgstateSet $pg -device $deviceA \
      -xfract 0.25 -yfract 0.35 -nxwindow 2 -nywindow 3 -symb 7 -isLine 1   
      pgstateOpen $pg
      topLabel 20 "PSP p.4, frames: $startframe-$endframe" \
	  $run $col $date $filter $rerun
      pgstateSet $pg -isNewplot 1 -isLine 1     

      # Plot patch calibration (flux20 uncorrected for extinction vs. time)
      set nctimes [exprGet $ctimes.nElements]
      pgstateSet $pg -isNewplot 1  
      if {$nctimes > 0} {
         set Ngoodcalibs 0
         # some patches might not have all colors, count good ones
         loop ii 0 $nctimes {
             set onecalib [chainElementGetByPos $ctimes $ii]
             if {[exprGet $onecalib.calib<$i>->flux20Err] > 0} {
                incr Ngoodcalibs
             }
         }
         set times [vectorExprNew $Ngoodcalibs] 
         set pflux20 [vectorExprNew $Ngoodcalibs] 
         set pflux20Err [vectorExprNew $Ngoodcalibs] 
         # populate plotting vectors
         loop ii 0 $Ngoodcalibs {
             set onecalib [chainElementGetByPos $ctimes $ii]
             if {[exprGet $onecalib.calib<$i>->flux20Err] > 0} {
                  set mjd [exprGet $onecalib.mjd]
                  set frac_mjd [expr $mjd - [int $mjd]] 
                  handleSet $times.vec<$ii> $frac_mjd 
                  handleSet $pflux20.vec<$ii> [exprGet $onecalib.calib<$i>->flux20]
                  handleSet $pflux20Err.vec<$ii> [exprGet $onecalib.calib<$i>->flux20Err]              
             }
         } 

         vectorExprSet $pflux20 $pflux20/1000.0 
         vectorExprSet $pflux20Err $pflux20Err/1000.0 
     vNameSet $pflux20 ""
         # these are the values for each patch
     vPlot $pg $times $pflux20 -vectorYErr $pflux20Err \
         -xmax [vExtreme $times max] \
         -xmin [vExtreme $times min] \
         -ymin [vExtreme [vectorExprSet $vtmp $pflux20-$pflux20Err] min] \
         -ymax [vExtreme [vectorExprSet $vtmp $pflux20+$pflux20Err] max]
     titlePlot "PT calibration (kADU)" 40
 
         # plot error bars
         set Npatch [chainSize $ctimes]
         vPlotErr $times $pflux20 $pflux20Err $Npatch $pg 
     vectorExprDel $pflux20Err
         # plot calibration
           # this is the fit to flux20(t) 
       if {1} {
              set a [vectorExprNew 2]
              set tmin [vExtreme $times min]
              set tmax [vExtreme $times max]
              set edge 0.02
              set tL [expr $tmin + $edge *($tmax - $tmin)]
              set tR [expr $tmax - $edge *($tmax - $tmin)]
              handleSet $a.vec<0> $tL
              handleSet $a.vec<1> $tR
              set b [vectorExprNew 2]
              pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3
              if [info exists flux20_m($i)] { 
                 handleSet $b.vec<0> [expr ($tL * $flux20_m($i) + $flux20_c($i))/1000]  
                 handleSet $b.vec<1> [expr ($tR * $flux20_m($i) + $flux20_c($i))/1000] 
                 vPlot $pg $a $b
              }
              vectorExprDel $a; vectorExprDel $b;  
       }   
         vectorExprDel $times    
      } else {
     # put a dummy plot in the same place
     set pflux20 [vectorExprEval 1]
     vPlot $pg $pflux20 $pflux20
     titlePlot "No mtPatch available" 40
      }
      xlabel "Time (mjd fraction)"
      vectorExprDel $pflux20; 

      # Plot flux20 for each frame
      set flux20 [vFromChain $cframes calib<$i>->flux20]
      vectorExprSet $flux20 $flux20/1000.0 
      vNameSet $flux20 ""
      set flux20Err [vFromChain $cframes calib<$i>->flux20Err]
      vectorExprSet $flux20Err $flux20Err/1000.0 
      set fmin [vExtreme [vectorExprSet $vtmp $flux20-$flux20Err] min]
      set fmax [vExtreme [vectorExprSet $vtmp $flux20+$flux20Err] max]
      pgstateSet $pg -isNewplot 1 -isLine 1 -icLine 1 -lineStyle 1 
      vPlot $pg $frames $flux20 -vectorYErr $flux20Err \
      -xmin $startframe -xmax $endframe -ymin $fmin -ymax $fmax
      titlePlot "Flux at mag20 (kADU)" 40
      xlabel Field
      vectorExprDel $flux20; vectorExprDel $flux20Err
      # mark the positions of calibration patches 
      if [info exists frameinfolist($filter)] {
         # vertical lines
         set nframes [chainSize $frameinfolist($filter)]
         set edge 0.04
         set f20min [expr $fmin + $edge*($fmax - $fmin)]
         set f20max [expr $fmax - $edge*($fmax - $fmin)]
         loop i 0 $nframes { 
            set frameinfo [chainElementGetByPos $frameinfolist($filter) $i]
        set flux20 [exprGet $frameinfo.patch_flux20]
            if {$flux20 > 0} {
               set frame [expr $startframe + $i]
               set a [vectorExprNew 2]
               handleSet $a.vec<0> $frame
               handleSet $a.vec<1> $frame
               set b [vectorExprNew 2]
               handleSet $b.vec<0> $f20min
               handleSet $b.vec<1> $f20max
               pgstateSet $pg -isNewplot 0 -isLine 1 -icLine 3
               vPlot $pg $a $b
               vectorExprDel $a; vectorExprDel $b; 
            }
         }
      }

      # cleanup
      endplot $pg

      # catch for the filter loop
      } msg] {
     echo "filter $filter: $msg"
      }
   }; #end of loop over filters

   vectorExprDel $frames
   vectorExprDel $vtmp

   chainDestroy $ctimes calib1bytimeDel
   chainDestroy $cframes calib1byframeDel

}


# plot the column boundary and all stars from PT patches
# as specified in psPlan file
proc overlayPTpatch {planfile {device /XWINDOW} {outfile ""} {camCols ""}} {

global asTrans_format 
      # read plan file
      set plan [read_planfile $planfile]
      foreach el $plan {
	 set var [lindex $el 0]
	 if [regexp {^display} $var] {
	    global $var
	 }
	 set $var [lindex $el 1]
	 keylset psplan $var [set $var]
	 if {[lindex $el 1] == "nodefault"} {
            error "** keyword $var has value nodefault! **"
         }
      }

      eval set baseDir $baseDir
      set defaultDir $baseDir


      foreach dir "bias config fang ff mtPatch output outputFang parameters photomFile transFile trueflat" {
         set dir ${dir}Dir

         if ![info exists $dir] {
	   set $dir $defaultDir
         }
         eval set $dir [set $dir]
         set $dir [envscan [set $dir]]
      }

      set asTrans_format scTrans-[format "%06d" $run]-$camCol.fit
 
      set transFile [get_trans_filename $transFileDir $run "" $camCol]


      echo "startField=$startField, endField=$endField"

      ### start plot 
      set pg [pgstateNew]
      if {$outfile == ""} {
        set outfile overlayPTpatch.ps
      }
      set deviceA [devicename $device $outfile]    
      pgstateSet $pg -device $deviceA -nxwindow 1 -nywindow 1 -symb 7 -isNewplot 1 -isLine 1  
      pgstateOpen $pg

      if {$camCols == ""} {set camCols $camCol}

      set startRun $startField 
      set endRun $endField

      foreach camCol $camCols {
         # read astrometry
         set transFile [get_trans_filename $transFileDir $run "" $camCol]
         astrom_read $startField $endField $camCol \
             astroms $filterlist $transFile node incl   
      
         # make a list of lists {field RA Dec}
         set band0 [lindex $filterlist 0]
         set fieldRADec [get_fieldRADec \
                        $astroms($band0) $node $incl $startRun]
         set fieldRAleft [get_fieldRADec $astroms($band0) \
                        $node $incl $startRun 0 0 0]
         set fieldRAright [get_fieldRADec $astroms($band0) \
                        $node $incl $startRun 0 0 2048]
      
         # make vector with the outline of this column
         set Nfields [llength $fieldRAleft]     
         assert {$Nfields == [llength $fieldRAright]}
         set Npts [expr 2*$Nfields+1]
         set edgeRA [vectorExprNew $Npts]
         set edgeDec [vectorExprNew $Npts]
         loop i 0 $Nfields {
            handleSet $edgeRA.vec<$i> [lindex [lindex $fieldRAleft $i] 1]
            handleSet $edgeRA.vec<[expr 2*$Nfields-1-$i]> [lindex [lindex $fieldRAright $i] 1]
            handleSet $edgeDec.vec<$i> [lindex [lindex $fieldRAleft $i] 2]
            handleSet $edgeDec.vec<[expr 2*$Nfields-1-$i]> [lindex [lindex $fieldRAright $i] 2]
         }
         handleSet $edgeRA.vec<[expr 2*$Nfields]> [exprGet $edgeRA.vec<0>]  
         handleSet $edgeDec.vec<[expr 2*$Nfields]> [exprGet $edgeDec.vec<0>] 
         if {$camCol == [lindex $camCols 0]} { 
            set RAmin [vExtreme $edgeRA min]; set RAmax [vExtreme $edgeRA max] 
            set Decmin [vExtreme $edgeDec min]; set Decmax [vExtreme $edgeDec max] 
            vNameSet $edgeRA "RA (deg)"; vNameSet $edgeDec "Dec (deg)" 
         }
         pgstateSet $pg -icMark 4 -isLine 1  
         vPlot $pg $edgeRA $edgeDec -xmin $RAmin -xmax $RAmax -ymin $Decmin -ymax $Decmax
         vListDel [list $edgeRA $edgeDec]
         pgstateSet $pg -icMark 4 -isLine 0  

         echo "reading PT patches"
         set patches [chainNew CHAIN]
         set mtPatchList [get_mtPatchList $mtPatchDir $mtPatchFormat $mtPatches]
         set ipatch 0
         set PTfields ""
  
         foreach patchfile $mtPatchList {
	    set patch_data [mtpatch_read $patchfile $fieldRADec $PTfields $node $incl] 
	    set patch [lindex $patch_data 0]
            set new_fields($ipatch) [lindex $patch_data 1]
	    set PTfields "$PTfields $new_fields($ipatch)" 
            chainElementAddByPos $patches $patch TAIL AFTER
            set PatchFileName($ipatch) $patchfile
            set Nstars($ipatch) [exprGet $patch.nElements]
            echo "   $patchfile"
            pgstateSet $pg -isNewplot 0 
            set ra [vFromChain $patch ra]; set dec [vFromChain $patch dec]
            vPlot $pg $ra $dec; vListDel [list $ra $dec]
            handleDel $patch
            incr ipatch
          }   
          echo "Read $ipatch patches"; echo
          loop i 0 $ipatch {
             set fields $new_fields($i)
	     if {$fields == " "} {
                set fields none
             }
             echo "Patch $i: Nstars=$Nstars($i), overlapping fields=$fields"
	  }         
      }

      if {[llength $PTfields] > 0} {
           echo; echo "Cummulative overlapping fields = $PTfields"
      } else {
           echo; echo "** THERE ARE NO OVERLAPPING FIELDS! **" 
      }
    
      endplot $pg
      
      patchesDestroy $patches

}


### for comparing different reductions
# given 2 reruns, return statistics (mean median sigma min max) for
# the specified quantities
# dumpData = 0: print only the maximal deviations for each quantity
# dumpData = 1: print the statistics for each chip
# dumpData = 2: print all the data
proc compare_psField {run rr1 rr2 {startField -1} {endField -1} {outdir .} \
                     {filters {u g r i z}} {camCols {1 2 3 4 5 6}} {Qs ""} \
		     {dumpData 1} {dataroot /u/dss/data}} {

global data_root openit 

       set data_root $dataroot

       set runstr [format "%06d" $run]

       if {$Qs == ""} {
           set Qs "psf->width psf->psfCounts ap_corr_run ap_corr_runErr psf_ap_correctionErr sky skyerr"
       }
       # find field range?
       set_run $run 1 -rerun $rr1 
       if {$startField < 0} {
           set startField [lindex [find_field_range $openit(objdir) psField] 0]
       }
       if {$endField < 0 || $endField < $startField} {
           set endField [lindex [find_field_range $openit(objdir) psField] 1]
       }

       # output file
       set filename $outdir/DIFFpsField-$runstr-reruns${rr1}VS${rr2}-fields$startField-$endField.dat
       set outf [open $filename w]
       puts $outf \
            "# The comparison of psField files for run $run, rerun $rr1 vs. $rr2, fields $startField-$endField"
       set user [exec whoami]
       set date [exec date] 
       puts $outf \
            "# Produced by $user on $date using photo [photoVersion]"
       ### get data 
       set Nfields [expr $endField - $startField + 1] 
       foreach camCol $camCols {
          foreach f $filters {
             foreach q $Qs {
	        set dqVec($camCol,$f,$q) [vectorExprNew $Nfields]
             }
	  }
       }

       # loop over camera columns
       foreach camCol $camCols {
          # loop over filters
          foreach f $filters {
             # loop over fields
             loop field $startField [expr $endField + 1] {
	        set ifield [expr $field - $startField]
                #echo $field ($startField-$endField)
                set fieldstr [format "%04d" $field]
                set str " [format "%4d" $field]"
                set_run $run $camCol -rerun $rr1                 
                set psField1 $openit(objdir)/psField-$runstr-$camCol-$fieldstr.fit
                set_run $run $camCol -rerun $rr2                 
                set psField2 $openit(objdir)/psField-$runstr-$camCol-$fieldstr.fit
                foreach q $Qs {
                   set qVal1($camCol,$f,$field,$q) [get_from_psField $psField1 $f $q]
                   set qVal2($camCol,$f,$field,$q) [get_from_psField $psField2 $f $q]
		   set m [expr 0.5*($qVal1($camCol,$f,$field,$q)+$qVal2($camCol,$f,$field,$q))]
		   if {$m > 0} {
		     set dq [expr 100.0*($qVal2($camCol,$f,$field,$q)-$qVal1($camCol,$f,$field,$q))/$m]
                   } else {
                     set dq 0
                   }
                   handleSet $dqVec($camCol,$f,$q).vec<$ifield> $dq
		   if {$dumpData == 2} {
		      set qVal1F [format "%9.6f" $qVal1($camCol,$f,$field,$q)]
		      set qVal2F [format "%9.6f" $qVal2($camCol,$f,$field,$q)]
		      set dqF [format "%9.6f" $dq]
                      set L "$camCol,$f,$field,$q: q1 = $qVal1F q2 = $qVal2F dq=$dqF"
                      puts $outf $L
                      echo $L
                   }
                } 
	     }
          }
       }

       ### process data
       foreach q $Qs {
           set maxq($q) "         0"
           set maxc($q) "-"
           set maxf($q) "-"
       }
       foreach camCol $camCols {
          foreach f $filters {
             foreach q $Qs {
                set line($camCol,$f,$q) " $camCol $f   "
		set mean [format "%10.3e" [vMean $dqVec($camCol,$f,$q)]]
                set line($camCol,$f,$q) "$line($camCol,$f,$q) $mean" 
		set median [format "%10.3e" [vMedian $dqVec($camCol,$f,$q)]]
                set line($camCol,$f,$q) "$line($camCol,$f,$q) $median" 
		set sig [format "%10.3e" [vSigma $dqVec($camCol,$f,$q)]]
                set line($camCol,$f,$q) "$line($camCol,$f,$q) $sig" 
		set min [format "%10.3e" [vExtreme $dqVec($camCol,$f,$q) min]]
                set line($camCol,$f,$q) "$line($camCol,$f,$q) $min" 
		set max [format "%10.3e" [vExtreme $dqVec($camCol,$f,$q) max]]
                set line($camCol,$f,$q) "$line($camCol,$f,$q) $max" 
		if {$max > $maxq($q)} {
                    set maxq($q) $max
                    set maxc(q) $camCol
                    set maxf(q) $f
                }
             }
	  }
       }


       if {$dumpData >= 1} {
          foreach q $Qs {
             puts $outf " "
             puts $outf "# === Statistics for the fractional difference in $q ==="
             puts $outf "# c f        mean      median     sigma       min        max "
             foreach camCol $camCols {
                foreach f $filters {
                   puts $outf "# $line($camCol,$f,$q)"
                } 
	     }
          }
       }

       # final summary
       puts $outf " ";  puts $outf "# == The maximum deviations ==="
       puts $outf "# ==  dqMax (%)  camCol filter  quantity  "
       echo "# == The maximum deviations ==="
       echo "# ==  dqMax (%)  camCol filter  quantity  "
       foreach q $Qs {
           puts $outf " $maxq($q)        $maxc($q)      $maxf($q)     $q  " 
           echo " $maxq($q)        $maxc($q)      $maxf($q)     $q  " 
       }

       # close output files
       close $outf 

}


### aux proc:
# given a chain of PC stars (aka MTstarlist), and an MT patch,
# overlay the two in RA/Dec plot
proc testPTmatches {MTstarlist MTpatch {device /XWINDOW} {outfile ""}} {



      ### start plot 
      set pg [pgstateNew]
      if {$outfile == ""} {
        set outfile testPTmatches.ps
      }
      set deviceA [devicename $device $outfile]    
      pgstateSet $pg -device $deviceA -nxwindow 1 -nywindow 1 -isNewplot 1 -isLine 1  
      pgstateOpen $pg

      # first plot stars from PSP
      set ra [vFromChain $MTstarlist ra]
      set dec [vFromChain $MTstarlist dec]
      vNameSet $ra "RA (deg)"
      vNameSet $dec "Dec (deg)"
      pgstateSet $pg -icMark 2 -symb 3 -isLine 0
      vPlot $pg $ra $dec 
      vListDel "$ra $dec"

      # and now stars from PT patch
      set ra [vFromChain $MTstarlist ra]
      set dec [vFromChain $MTstarlist dec]
      vNameSet $ra ""
      vNameSet $dec ""
      pgstateSet $pg -isNewplot 0 -icMark 4 -symb 4
      vPlot $pg $ra $dec 
      vListDel "$ra $dec"

   endplot $pg

}


### for comparing different reductions
# given 2 psField files reconstruct a grid of PSFs for each
# file and subtract them. Return this region.
# Also print out the numerical values from calib1byframe
proc comparePSFs {psField1 psField2 filter {nrow 6} {ncol 9}} { 

         
    # read KL PSF data from psField files
    if [catch {read_psfBasis psf_basis1 $psField1 $filter}] {
        declare_schematrans 5
    read_psfBasis psf_basis1 $psField1 $filter
    read_psfBasis psf_basis2 $psField2 $filter
        fitsBinForgetSchemaTrans NULL        
    } else {
    read_psfBasis psf_basis2 $psField2 $filter
    }

    # reconstruct PSFs and get the difference
    set mosaic1 [psf_grid $psf_basis1($filter) $nrow $ncol] 
    set mosaic2 [psf_grid $psf_basis2($filter) $nrow $ncol] 
    regIntLincom $mosaic1 $mosaic2 10000 1 -1
    regDel $mosaic2

    psfBasisDel $psf_basis1($filter)
    psfBasisDel $psf_basis2($filter)

    set type 1

    set calib1 [get_calib1byframe_from_psField $psField1 $filter]
    set string1 [string_from_calib1 $calib1 0 $type] 
    calib1byframeDel $calib1

    set calib2 [get_calib1byframe_from_psField $psField2 $filter]
    set string2 [string_from_calib1 $calib2 0 $type] 
    calib1byframeDel $calib2

    echo "       PSP status    n   PSFwidth s1_2G  s2_2G  b_2G   \
                  s1     s2     b     p0    sp  beta   N*    CPcnts"
    echo "File 1:"
    echo "$string1"
    echo "File 2:"
    echo "$string2"

    
    return "$mosaic1 [list $string1] [list $string2]"

}


# take 2 regions from 2 fits files, subtract them and display
proc compare_fits {file1 file2 {offset 10000}} {

    set reg1 [regReadAsFits [regNew] $file1]
    set reg2 [regReadAsFits [regNew] $file2]
    regIntLincom $reg2 $reg1 10000 1 -1
    display $reg2 "10000 + reg2 - reg1"
    regDel $reg1
    regDel $reg2

}







# aux proc to help fits-challenged people; given run, camcol, startField, and 
# endField, dump the values of psf width at the frame center in 5 bands
# e.g. on fsgi03:
# photo> dump_PSFwidth 745 1 30 32 . /usr/sdss/data01/imaging
#
proc dump_PSFwidth {run camcol {startField -1} {endField -1} {outdir .} \
                    {dataroot /data/dp3.b/data}} {

#  get_calib1byframe_from_psField $psField
global data_root openit 

       set data_root $dataroot
       set filters {u g r i z}
       set runstr [format "%06d" $run]
       set_run $run $camcol

       # find field range
       if {$startField < 0} {
           set startField [lindex [find_field_range $openit(objdir) psField] 0]
       }
       if {$endField < 0} {
           set endField [lindex [find_field_range $openit(objdir) psField] 1]
       }

       # output file
       set filename $outdir/PSFwidth-$runstr-${camcol}-$startField-$endField.dat
       set outf [open $filename w]
       puts $outf \
                "# PSF width for run $run, column $camcol, fields $startField-$endField"
       puts $outf \
                "# field  width(u)  width(g)  width(r)  width(i)  width(z)"

       # loop over fields
       loop field $startField [expr $endField + 1] {
           echo $field ($startField-$endField)
           set fieldstr [format "%04d" $field]
           set psField $openit(objdir)/psField-$runstr-$camcol-$fieldstr.fit
           set str " [format "%4d" $field]"
           foreach filter $filters {
               # get PSF width
               set allow_old_psField 1
               set PSFwidth [format "%9.2f" [get_from_psField $psField $filter psf->width]]
               set str "$str $PSFwidth"
           }
           puts $outf $str
       }

       # close output files
       close $outf 

}



# aux proc; given run, camcol, startField, and endField, dump the 
# values of psf width (type = 1), or aperture correction (type = 2),
# at characteristic points on a frame
proc dump_PSFdata {run camcol startField endField {type 1} {filters {u g r i z}} \
             {outdir .} {dataroot /data/dp3.b/data}} {

global data_root openit 
global allow_old_psField

       set data_root $dataroot

       if {$type == 1} {
           set q PSFwidth
       } else {
           set q apCorr
       }

  

       set_run $run $camcol
       set runstr [format "%06d" $run]
       declare_schematrans 5
       fake_setup
       # output files
       foreach f $filters {
          set filename $outdir/${q}-$runstr-${camcol}$f-$startField-$endField.dat
          set outf($f) [open $filename w]
          puts $outf($f) \
                "# $q for run $run, column $camcol, filter $f"
          puts $outf($f) \
                "# field  status  PSFwidth   LL    LR    mL    C     mR    UL     UR"
       }

       # loop over fields
       loop field $startField [expr $endField + 1] {
           set fieldstr [format "%04d" $field]
           set psField $openit(objdir)/psField-$runstr-$camcol-$fieldstr.fit
           # loop over filters
           foreach filter $filters {
               # read PSF data from psField file
               if [catch {read_psfBasis psfBasis $psField $filter 0} msg] {  
                   echo "cannot read PSF_BASIS:"
                   echo $msg
                   fitsBinForgetSchemaTrans NULL           
                   return -1        
               }
               # calculate aperture correction at desired positions
               set q_LL [psfBasis2$q $psfBasis($filter) 0 0]
               set q_LR [psfBasis2$q $psfBasis($filter) 0 2048]
               set q_mL [psfBasis2$q $psfBasis($filter) 680 0]
               set q_C  [psfBasis2$q $psfBasis($filter) 680 1024]
               set q_mR [psfBasis2$q $psfBasis($filter) 680 2048]
               set q_UL [psfBasis2$q $psfBasis($filter) 1361 0]
               set q_UR [psfBasis2$q $psfBasis($filter) 1361 2048]
               set q_str [format "%5.3f %5.3f %5.3f %5.3f %5.3f %5.3f \
                           %5.3f" $q_LL $q_LR  $q_mL $q_C \
                           $q_mR $q_UL $q_UR]
               # get PSF width and PSP status
               set  allow_old_psField 1
               set PSFwidth [get_from_psField $psField $filter psf->width]
               set status [get_from_psField $psField $filter status]
               set str1 "[format "%6d" $field]     $status   [format "%8.3f" $PSFwidth]" 
               puts $outf($filter) " $str1   $q_str"
               if [info exists psfBasis] {
                   free_psfBasis psfBasis
               }
           }
       }

       # close output files
       foreach f $filters {
          close $outf($f) 
       }
  
       # clean
       finish_fake_setup 
       fitsBinForgetSchemaTrans NULL        


}




# wrapper for dump_PSFdata 
proc dump_PSFdata_forRun {run startField endField {type 1} \
                   {outdir .} {dataroot /data/dp3.b/data}} {
    
     loop c 1 7 {
        dump_PSFdata $run $c $startField $endField $type \
               {u g r i z} $outdir $dataroot
     }

}


# aux proc; given run, camcol, field, and filter, produce an image
# with nrow x ncol values, with the image values equal to PSFwidth 
# in arcseconds*1000. This proc also applies a correction for the
# seeing wavelength dependence (\propto lambda^0.2), normalized to
# r band, with the effective wavelengths: 3540, 4760, 6280, 7690,
# and 9250 (Angstroms).


proc PSFwidth2image {run rerun camcol field filter {nrow 28} {ncol 41} \
                 {outdir .} {dataroot /data/dp3.b/data}} {

   global data_root openit allow_old_psField testbed ROWS
       set data_root $dataroot

       # wavelength dependent seeing corrections
       set lam(u) 3540.0
       set lam(g) 4760.0
       set lam(r) 6280.0
       set lam(i) 7690.0
       set lam(z) 9250.0

       foreach f {u g r i z} {
           set wav_corr($f) [expr pow($lam($f)/$lam(r),0.2)]
       }

       set_run $run $camcol -rerun $rerun
       set runstr [format "%06d" $run]
       declare_schematrans 5
       fake_setup

       set Npix_row [expr 1361 / ($nrow - 1)]
       set Npix_col [expr 2048 / ($ncol - 1)]

       # output file name 
       set filename $outdir/PSFwidth-$runstr-${camcol}-$field-$filter.fit
       # output region
       set region [regNew $nrow $ncol]

       set fieldstr [format "%04d" $field]
       if [info exists testbed] {
           set psField $testbed/$camcol/psField-$runstr-$camcol-$fieldstr.fit 
       } else {
           set psField $openit(objdir)/psField-$runstr-$camcol-$fieldstr.fit
       }
       # read PSF data from psField file
       if [catch {read_psfBasis psfBasis $psField $filter 0} msg] {  
             echo "cannot read PSF_BASIS:"
             echo $msg
             fitsBinForgetSchemaTrans NULL           
             return -1        
       }

       # loop over rows
       loop i 0 $nrow {
          set row [expr $i * $Npix_row]
          # loop over cols
          loop j 0 $ncol {  
              set col [expr $j * $Npix_col]
              # calculate PSF width at this position
              set w [psfBasis2PSFwidth $psfBasis($filter) $row $col]
              handleSet $region.$ROWS<$i><$j> [expr int(1000*$w*$wav_corr($filter))]
          }
       }
              
       # store image
       regWriteAsFits $region $filename 

       # clean
       regDel $region
       if [info exists psfBasis] {
           free_psfBasis psfBasis
       }
       finish_fake_setup 
       fitsBinForgetSchemaTrans NULL        


}


# a wrapper for PSFwidth2image: call it for each chip for fields
# corresponding to a given readout time (the time is implied by
# the r-band field number, other bands are offset by -2*fields
# per camera row).   
# If do_mosaic != 0 make a single fits file showing the FOV.
proc PSFsnapshot {run rerun startField endField {nrow 28} {ncol 41} \
              {do_mosaic 1} {outdir .} {dataroot /data/dp3.b/data}} {
    
     set filterlist {r i u z g}

     loop field $startField [expr $endField + 1] {
        echo "PSFsnapshot: field $field ($startField - $endField)" 
        loop camcol 1 7 {
           foreach filter $filterlist {
              set ifilter [lsearch $filterlist $filter]
              set this_field [expr $field - 2 * $ifilter] 
              PSFwidth2image $run $rerun $camcol $this_field $filter \
                         $nrow $ncol $outdir $dataroot 
           }
        }
        if {$do_mosaic} {
            set runstr [format "%06d" $run]
            set fieldstr [format "%04d" $field]
            set filename $outdir/PSFmosaic-$runstr-$fieldstr.fit
            set mosaic [PSFsnapshot2mosaic $run $field $nrow $ncol $outdir]
            regWriteAsFits $mosaic $filename
            regDel $mosaic
        }
     }

} 

# aux proc: assemble 6 snapshots, assumed to have been produced by PSFsnapshot,
# into a single region and save to a fits file
proc assemble6PSFsnapshots {run startField {nrow 28} {ncol 41} {outdir .}} {


     set fac 1.2
     set Nrow [expr int(4*$nrow*2.00000+$nrow)]
     set Ncol [expr int(5*$ncol*1.84458+$ncol)]
     set row_gutter [expr int($fac*$Nrow) - $Nrow]
     set col_gutter [expr int($fac*$Ncol) - $Ncol]

     set aux [expr $Nrow + $row_gutter]
     set row_off [list [expr 2*$aux] [expr 2*$aux] $aux $aux 0 0]
     set aux [expr $Ncol + $col_gutter]
     set col_off [list 0 $aux 0 $aux 0 $aux]
 
     set runstr [format "%06d" $run]     
     loop i 0 6 {
        set field [expr $startField + $i]
        set fieldstr [format "%04d" $field]
        set filename $outdir/PSFmosaic-$runstr-$fieldstr.fit
        set r [regReadAsFits [regNew] $filename]
        set r$i [list $r [lindex $row_off $i] [lindex $col_off $i]]
     }

     set r_list [list $r0 $r1 $r2 $r3 $r4 $r5]
     set 6frames [region2mosaic $r_list [expr 3*$Nrow+2*$row_gutter] \
                                        [expr 2*$Ncol+1*$col_gutter]]
     set filename $outdir/PSFmosaics-$runstr-[format "%04d" $startField]-$fieldstr.fit     
     regWriteAsFits $6frames $filename
     

     regDel $6frames 
     foreach el $r_list {
        regDel [lindex $el 0]
     }

}


# given 30 regions produced by PSFsnapshot, make a mosaic
# displaying them as in the camera's FOV
proc PSFsnapshot2mosaic {run field {nrow 28} {ncol 41} {outdir .}} {

     set filterlist {r i u z g}
     set reg_list {}

     loop camcol 1 7 {
        foreach f $filterlist {
           set ifilter [lsearch $filterlist $f]
           set tf [expr $field - 2 * $ifilter] 
           set runstr [format "%06d" $run]
           set filename \
                  $outdir/PSFwidth-$runstr-${camcol}-$tf-$f.fit
           set reg [regReadAsFits [regNew] $filename]
           set row_off [expr int((4-$ifilter)*$nrow*2.00000)]
           set col_off [expr int(($camcol-1)*$ncol*1.84458)]
           set r [list $reg $row_off $col_off]
           lappend reg_list $r
        }
     }
     
     set Nrow [expr int(4*$nrow*2.00000+$nrow)]
     set Ncol [expr int(5*$ncol*1.84458+$ncol)]
     set mosaic [region2mosaic $reg_list $Nrow $Ncol 0]
     # free allocated regions
     foreach el $reg_list {
        regDel [lindex $el 0]
     }

     return $mosaic

}



# make a mosaic of a list of regions. Regions are specified by
# {region row col} where region is a REGION, and (row, col) is
# the placement of its lower left corner in the master region,
# relative to the master region's lower left corner. The size
# of master region is given by Nrow and Ncol. This proc allows
# individual regions to have pixels off the master region, and
# to be of different size. 
# The individual regions are overlayed in the supplied order.
# If add_regs = 0 then the pixels with overlapping regions are 
# set to the value given by the last region in the list.
# If add_regs != 0 then the pixels with overlapping regions are 
# set to the sum of all values. 
proc region2mosaic {reg_list Nrow Ncol {add_regs 0}} {

   # make master region
   set mosaic [regNew $Nrow $Ncol]
   regClear $mosaic

   # overlay each region
   foreach reg_info $reg_list {

      # get this reg's info 
      set reg [lindex $reg_info 0]
      set reg_nrow [exprGet $reg.nrow]
      set reg_ncol [exprGet $reg.ncol]
      set row [lindex $reg_info 1]
      set col [lindex $reg_info 2]

      # get the overlap coordinates
      set overlap [regs_intersect $mosaic $reg $row $col] 
      set op_nrow [lindex $overlap 0]
      set op_ncol [lindex $overlap 1]
      set op_master_row0 [lindex $overlap 2]
      set op_master_col0 [lindex $overlap 3]
      set op_new_row0 [lindex $overlap 4]
      set op_new_col0 [lindex $overlap 5]

      if {$op_nrow > 0 && $op_ncol > 0} { 
          # subregion in the master region
          set subM [subRegNew $mosaic $op_nrow $op_ncol \
                             $op_master_row0 $op_master_col0]
          # subregion from the new region
          set subN [subRegNew $reg $op_nrow $op_ncol \
                             $op_new_row0 $op_new_col0]
          if {!$add_regs} {regClear $subM}
          regAdd $subM $subN
          regDel $subM; regDel $subN; 
      }
   }
   
   return $mosaic
}

# given 2 regions, and the offset of the second one relative to
# the first one, find their intersection. Return a list  
# {nrow ncol row0_1 col0_1 row0_2 col0_2} where nrow and ncol 
# give the size of the overlapping region, and the values of 
# row0_{1,2} and col0_{1,2} give the position of its lower left
# corner relative to each region's coordinate frame 
# (as in e.g. a call to subRegNew)
proc regs_intersect {reg1 reg2 row_off col_off} {

     # get the reg's sizes (Nrow1, Ncol1, ...)
     foreach coord {row col} {
         set N${coord}1 [exprGet $reg1.n$coord]
         set N${coord}2 [exprGet $reg2.n$coord]
     }    

     # coordinates of the intersecting region in the reg1's frame 
     set row0_1 0 
     if {$row_off > 0} {
         set row0_1 [expr min($row_off,$Nrow1)]
     } 
     set nrow 0
     if {[expr $Nrow2+$row_off] > 0} {
         set nrow [expr max(0,min($Nrow2+$row_off,$Nrow1) - $row0_1)]
     } 
     set col0_1 0 
     if {$col_off > 0} {
         set col0_1 [expr min($col_off,$Ncol1)]
     } 
     set ncol 0
     if {[expr $Ncol2+$col_off] > 0} {
         set ncol [expr max(0,min($Ncol2+$col_off,$Ncol1) - $col0_1)]
     } 

     # coordinates of the intersecting region in the reg2's frame 
     # simply apply (-row_off, -col_off) to (row0_1, col0_1)
     set row0_2 [expr $row0_1 - $row_off]
     set col0_2 [expr $col0_1 - $col_off]

     return [list $nrow $ncol $row0_1 $col0_1 $row0_2 $col0_2]

}



# compare two flat fields, expressed in mags
proc compareFlatfieldsMags {reg1 row1 reg2 row2 {device /XWINDOW} {title ""}} {


    # check regions
    set ncol1 [exprGet $reg1.ncol]
    set nrow1 [exprGet $reg1.nrow]
    if {$row1 > $nrow1} {
        return "region 1 has $nrow1 rows, row $row1 does not exist"
    }
    set ncol2 [exprGet $reg2.ncol]
    set nrow2 [exprGet $reg2.nrow]
    if {$row2 > $nrow2} {
        return "region 2 has $nrow2 rows, row $row2 does not exist"
    }

    # get vectors
    set nSplit 1024
    set x1 [vIndex $nSplit]
    set x2 [vectorExprEval $nSplit+$x1]
    set y1Lraw [vectorGetFromRegion $reg1 $row1 $row1 0 $nSplit 0] 
    set y2Lraw [vectorGetFromRegion $reg2 $row2 $row2 0 $nSplit 0] 
    set y1Rraw [vectorGetFromRegion $reg1 $row1 $row1 $nSplit [expr $ncol1-1]  0] 
    set y2Rraw [vectorGetFromRegion $reg2 $row2 $row2 $nSplit [expr $ncol2-1] 0] 

    set Med1 [vMedian $y1Lraw]
    set Med2 [vMedian $y2Lraw]
    set ratLraw [vectorExprEval -2.5*lg($y1Lraw/$Med1/$y2Lraw*$Med2)]
    set Med1 [vMedian $y1Rraw]
    set Med2 [vMedian $y2Rraw]
    set ratRraw [vectorExprEval -2.5*lg($y1Rraw/$Med1/$y2Rraw*$Med2)]
    

    set buffer 50
    set mask [vectorExprEval "$x1 >= [expr $nSplit-$buffer] ? 1 : 0"]  
    set jumpL [vMedian $ratLraw -vMask $mask]
    vectorExprDel $mask
    set mask [vectorExprEval "$x2 <= [expr $nSplit+$buffer] ? 1 : 0"]  
    set jumpR [vMedian $ratRraw -vMask $mask]
    set jump [expr $jumpR - $jumpL]
    set ratRraw2 [vectorExprEval $ratRraw-$jump] 
    vectorExprDel $ratRraw      
    vectorExprDel $mask    

    set rat [vConcat $ratLraw $ratRraw2]
    vMedianSmooth $rat [exprGet $rat.dimen] 3
    set mask [vectorExprEval "abs($rat) < 0.3 ? 1 : 0"]  
    set spikeMask [get_spike_mask $rat 0.03 $mask]
    vectorExprDel $mask     
    set mean  [vMean $rat -vMask $spikeMask]
    set sigma [vSigma $rat -vMask $spikeMask]

    set ratL [vectorExprEval $ratLraw-$mean] 
    set ratR [vectorExprEval $ratRraw2-$mean] 
    set mask [vectorExprEval "abs($ratL) < 0.3 ? 1 : 0"]  
    set spikeMaskL [get_spike_mask $ratL 0.03 $mask]
    vectorExprDel $mask     
    set mask [vectorExprEval "abs($ratR) < 0.3 ? 1 : 0"]  
    set spikeMaskR [get_spike_mask $ratR 0.03 $mask]
    vectorExprDel $mask     
    vectorExprDel $ratLraw      
    vectorExprDel $ratRraw2      

    vNameSet $ratL "-2.5*log10(FF1/FF2)"
    vNameSet $x1  "pixel"
    # open plot
    set pg [pgstateNew]
    set deviceA [devicename $device compareRegRows.ps]
    pgstateSet $pg -device $deviceA -lineWidth 3  \
     -xfract 0.25 -yfract 0.25 -nxwindow 1 -nywindow 1 
    pgstateOpen $pg 

 
    # plot 
    pgstateSet $pg -isLine 1 -icLine 2 
    vPlot $pg $x1 $ratL -ymin -0.05 -ymax 0.05 -xmax 0 -xmax 2048 -vectorMask $spikeMaskL
    titlePlot $title 40 
    pgstateSet $pg -isNew 0
    vPlot $pg $x2 $ratR -vectorMask $spikeMaskR 

    vectorExprDel $spikeMaskL  
    vectorExprDel $spikeMaskR 
    vectorExprDel $spikeMask  

    pgstateSet $pg -lineStyle 2 -icLine 4 -lineWidth 2
    plot_line $pg -1000 0 3000 0     
    pgstateSet $pg -lineStyle 4 -icLine 3 -lineWidth 1
    plot_line $pg -1000 -0.02 3000 -0.02   
    plot_line $pg -1000 0.02 3000 0.02   
    pgstateSet $pg -lineStyle 3 -icLine 4 -lineWidth 1
    plot_line $pg 1024 -0.5 1024 0.5   

    set rmsstr [format "%5.3f" $sigma]
    set text "rms = $rmsstr"
    pgPtext 600 0.06 0 0 $text

 
    # close plot     
    endplot $pg

    vectorExprDel $x1; vectorExprDel $x2
    vectorExprDel $y1Lraw; vectorExprDel $y2Lraw
    vectorExprDel $y1Rraw; vectorExprDel $y2Rraw
    vectorExprDel $ratL; vectorExprDel $ratR

    return $rmsstr

}
 
# concat two vectors (aux proc)
proc vConcat {v1 v2} {

    set N1 [exprGet $v1.dimen]; set N2 [exprGet $v2.dimen]; 
    set v [vectorExprNew [expr $N1+$N2]]
    loop i 0 $N1 {
	handleSet $v.vec<$i> [exprGet $v1.vec<$i>]
    }
    loop i 0 $N2 {
	handleSet $v.vec<[expr $N1+$i]> [exprGet $v2.vec<$i>]
    }

    return $v
}


# compare rows from 2 regions with the same number of columns. 
proc compareRegRows {reg1 row1 reg2 row2 {device /XWINDOW} {title ""}} {


    # check regions
    set ncol1 [exprGet $reg1.ncol]
    set nrow1 [exprGet $reg1.nrow]
    if {$row1 > $nrow1} {
        return "region 1 has $nrow1 rows, row $row1 does not exist"
    }
    set ncol2 [exprGet $reg2.ncol]
    set nrow2 [exprGet $reg2.nrow]
    if {$row2 > $nrow2} {
        return "region 2 has $nrow2 rows, row $row2 does not exist"
    }

    # get vectors
    set x1 [vIndex $ncol1]
    set y1 [vectorGetFromRegion $reg1 $row1 $row1 0 [expr $ncol1-1] 0] 
    set x2 [vIndex $ncol2]
    set y2 [vectorGetFromRegion $reg2 $row2 $row2 0 [expr $ncol2-1] 0] 
    vNameSet $x1 "pixel";  vNameSet $x2 ""
    vNameSet $y1 "counts"; vNameSet $y2 ""   

    # open plot
    set pg [pgstateNew]
    set deviceA [devicename $device compareRegRows.ps]
    pgstateSet $pg -device $deviceA -lineWidth 3  \
     -xfract 0.25 -yfract 0.25 -nxwindow 1 -nywindow 1 
    pgstateOpen $pg 

    # get extremes
    set ymax [vExtreme $y1 max]
    set ymin [vExtreme $y1 min]
    if {[vExtreme $y2 max] > $ymax} {set ymax [vExtreme $y2 max]}
    if {[vExtreme $y2 min] < $ymax} {set ymin [vExtreme $y2 min]}

    # plot 
    pgstateSet $pg -isLine 1 -icLine 2 
    #vPlot $pg $x1 $y1 -ymin 1950 -ymax 2150
    vPlot $pg $x1 $y1 -ymin $ymin -ymax $ymax
    titlePlot $title 40      

    pgstateSet $pg -icLine 4 -isNew 0
    vPlot $pg $x2 $y2

    # close plot     
    endplot $pg

    vectorExprDel $x1; vectorExprDel $x2
    vectorExprDel $y1; vectorExprDel $y2

  return 0
}
 

# call compareRegRows for 2 flat-field files
proc compareFF {ffReg1 ffReg2 {device /XWINDOW} {title ""} {mag 1}} { 

   set r1 [regReadAsFits [regNew] $ffReg1]
   set r2 [regReadAsFits [regNew] $ffReg2]

   if {$mag} {
       set a [compareFlatfieldsMags $r1 0 $r2 0 $device $title]
   } else {
       set a ""
       compareRegRows $r1 0 $r2 0 $device $title
   }

   regDel $r1
   regDel $r2
 
  return $a
}



# compare two flat fields, expressed in mags
proc compareFlatfieldsMagsBand {dir1 run1 dir2 run2 band {device /XWINDOW} \
	{title ""} {camCols {1 2 3 4 5 6}} {root1 psFF} {root2 psFF} } {


    set row1 0; set row2 0;
    set nSplit 1024
    set x1 [vIndex $nSplit]
    set x2 [vectorExprEval $nSplit+$x1]
    vNameSet $x2  ""


    foreach c $camCols {
	set file ${root1}-[format "%06d" $run1]-${band}${c}.fit
        set reg1 [regReadAsFits [regNew] $dir1/${root1}-[format "%06d" $run1]-${band}${c}.fit]
	set file ${root2}-[format "%06d" $run2]-${band}${c}.fit
        set reg2 [regReadAsFits [regNew] $dir2/${root2}-[format "%06d" $run2]-${band}${c}.fit]
        set y1Lraw [vectorGetFromRegion $reg1 0 0 0 $nSplit 0] 
        set y1Rraw [vectorGetFromRegion $reg1 0 0 $nSplit [expr [exprGet $reg1.ncol]-1] 0] 
        set y2Lraw [vectorGetFromRegion $reg2 0 0 0 $nSplit 0] 
        set y2Rraw [vectorGetFromRegion $reg2 0 0 $nSplit [expr [exprGet $reg2.ncol]-1] 0] 
        set Med1 [vMedian $y1Lraw]
        set Med2 [vMedian $y2Lraw]
        set ratLraw [vectorExprEval -2.5*lg($y1Lraw/$Med1/$y2Lraw*$Med2)]
        set Med1 [vMedian $y1Rraw]
        set Med2 [vMedian $y2Rraw]
        set ratRraw [vectorExprEval -2.5*lg($y1Rraw/$Med1/$y2Rraw*$Med2)]
        vectorExprDel $y1Lraw; vectorExprDel $y2Lraw
        vectorExprDel $y1Rraw; vectorExprDel $y2Rraw

        set buffer 50
        set mask [vectorExprEval "$x1 >= [expr $nSplit-$buffer] ? 1 : 0"]  
        set jumpL [vMedian $ratLraw -vMask $mask]
        vectorExprDel $mask
        set mask [vectorExprEval "$x2 <= [expr $nSplit+$buffer] ? 1 : 0"]  
        set jumpR [vMedian $ratRraw -vMask $mask]
        vectorExprDel $mask    
        set jump [expr $jumpR - $jumpL]
        set ratRraw2 [vectorExprEval $ratRraw-$jump] 
        vectorExprDel $ratRraw      

        set rat [vConcat $ratLraw $ratRraw2]
        vMedianSmooth $rat [exprGet $rat.dimen] 3
        set mask [vectorExprEval "abs($rat) < 0.3 ? 1 : 0"]  
        set spikeMask [get_spike_mask $rat 0.03 $mask]
        vectorExprDel $mask     
        set mean  [vMean $rat -vMask $spikeMask]
        set sigma($c) [vSigma $rat -vMask $spikeMask]
        vectorExprDel $spikeMask  

        set ratL($c) [vectorExprEval $ratLraw-$mean] 
        set ratR($c) [vectorExprEval $ratRraw2-$mean] 
        set mask [vectorExprEval "abs($ratL($c)) < 0.3 ? 1 : 0"]  
        set spikeMaskL($c) [get_spike_mask $ratL($c) 0.03 $mask]
        vectorExprDel $mask     
        set mask [vectorExprEval "abs($ratR($c)) < 0.3 ? 1 : 0"]  
        set spikeMaskR($c) [get_spike_mask $ratR($c) 0.03 $mask]
        vectorExprDel $mask     
        vectorExprDel $ratLraw      
        vectorExprDel $ratRraw2      

        vNameSet $ratR($c) ""
        regDel $reg1; regDel $reg2
    }

 

    # open plot
    set pg [pgstateNew]
    set deviceA [devicename $device compareFlatfieldsMagsBand.ps]
    pgstateSet $pg -device $deviceA -lineWidth 3  \
	-xfract 0.25 -yfract 0.25 -nxwindow 1 -nywindow [expr [llength $camCols]+1]
    pgstateOpen $pg 

    foreach c $camCols {

       if {$c == [lindex $camCols [expr [llength $camCols]-1]]} {
           vNameSet $x1  "pixel"             
       } else {
           vNameSet $x1  ""
       }  
       if {$c == [lindex $camCols [expr [llength $camCols]/2]]} {
            vNameSet $ratL($c) "-2.5*log10(FF1/FF2)"
       } else {
            vNameSet $ratL($c) ""
       }  


       # plot 
       pgstateSet $pg -isLine 1 -icLine 2  -isNew 1 
       vPlot $pg $x1 $ratL($c) -ymin -0.15 -ymax 0.15 -xmax 0 -xmax 2048 -vectorMask $spikeMaskL($c)
       if {$c == [lindex $camCols 0]} {titlePlot $title 40} 
       pgstateSet $pg -isNew 0
       vPlot $pg $x2 $ratR($c) -vectorMask $spikeMaskR($c) 

       pgstateSet $pg -lineStyle 2 -icLine 4 -lineWidth 2
       plot_line $pg -1000 0 3000 0     
       pgstateSet $pg -lineStyle 4 -icLine 3 -lineWidth 1
       plot_line $pg -1000 -0.05 3000 -0.05   
       plot_line $pg -1000 0.05 3000 0.05    
       pgstateSet $pg -lineStyle 3 -icLine 4 -lineWidth 1
       plot_line $pg 1024 -0.5 1024 0.5   

       set rmsstr [format "%5.3f" $sigma($c)]
       set text "col. $c: rms = $rmsstr"
       lappend rmsstrList $rmsstr
       pgPtext 600 0.06 0 0 $text
    }
 
    # close plot     
    endplot $pg

    foreach c $camCols {
       vListDel [list $ratL($c) $ratR($c) $spikeMaskL($c) $spikeMaskR($c)]
    }

    vectorExprDel $x1; vectorExprDel $x2

  return $rmsstrList

}
 




# call compareFF for all 30 chips 
proc compareFFall {dir1 run1 dir2 run2 {device /CPS} {title ""} {mags 1} {single_dir 0}} { 
 
  set runstr1  [format %06d $run1]
  set runstr2  [format %06d $run2]

  foreach c {1 2 3 4 5 6} {     
    set rms($c) "     $c"                                         
    foreach f {u g r i z} {
      set titleF "$title: ${f}${c}"
      if {$single_dir} {
	  set a [compareFF $dir1/psFF-$runstr1-${f}${c}.fit $dir2/psFF-$runstr2-${f}${c}.fit $device $titleF $mags]
      } else {
	  set a [compareFF $dir1/$c/psFF-$runstr1-${f}${c}.fit $dir2/$c/psFF-$runstr2-${f}${c}.fit $device $titleF $mags]
      }
      set rms($c) "$rms($c) $a"
      echo "$c $f: $a"
      if {$device != "/XWINDOW"} {
	 my_mv compareRegRows.ps compareFF-run${run1}-run${run2}-chip${c}${f}.ps
         echo "Your plot is compareFF-run${run1}-run${run2}-chip${c}${f}.ps"
      }
    }  
  } 

  echo " RMS:"
  echo "   u  g  r  i  z"
  foreach c {1 2 3 4 5 6} {     
     echo $rms($c)
  }   

}                                       


# call compareFF for all 30 chips 
proc testFFall {dir run ncol1 ncol2 {tol 0} {same_dir 0}} { 
 
  set runstr  [format %06d $run]
  
  echo " ------------------------------------------------------------"
  echo " column        1        2        3        4        5        6"
  echo " ------------------------------------------------------------"
  foreach f {r i u z g} {
     set line "    $f  "
     foreach c {1 2 3 4 5 6} {  
         if {$same_dir} {
            set FFfile $dir/psFF-$runstr-${f}${c}.fit  
         } else {
            set FFfile $dir/$c/psFF-$runstr-${f}${c}.fit  
         }
         if {$tol < 0} {
             set diff [format "%8.3f" [is_good_flat $FFfile $ncol1 $ncol2 $tol]]
         } else {
             set diff [format "%8.0f" [is_good_flat $FFfile $ncol1 $ncol2 $tol]]
         }
         set line "$line $diff"
     } 
     echo $line                   
  }  
  echo " ------------------------------------------------------------"

}                                       



# compare two chip-column dependent vectors for each chip 
# e.g. set root psSLprof
# compareVectorsFOV $root $dir $run $dir $run /PS $title "" "" 0 -0.15 0.15
proc compareVectorsFOV {root dir1 run1 dir2 run2 {device /XWINDOW} \
	                {title ""} {cols ""} {filters ""} {single_dir 0} \
                        {yminIn ""} {ymaxIn ""} } { 
 
  set runstr1 [format %06d $run1]
  set runstr2 [format %06d $run2]

  # open plot
  set pg [pgstateNew]
  set deviceA [devicename $device compareVectorsFOV.ps]
  pgstateSet $pg -device $deviceA -lineWidth 3  \
     -xfract 0.4 -yfract 0.35 -nxwindow 6 -nywindow 6 
  pgstateOpen $pg 

  if {$cols == ""} {set cols {1 2 3 4 5 6}}
  if {$filters == ""} {set filters {r i u z g}}

  foreach f $filters {
    foreach c $cols {        
      if {$single_dir} {   
         set file1 $dir1/$root-$runstr1-${f}${c}.fit 
         set file2 $dir2/$root-$runstr2-${f}${c}.fit 
      } else {
         set file1 $dir1/$c/$root-$runstr1-${f}${c}.fit 
         set file2 $dir2/$c/$root-$runstr2-${f}${c}.fit 
      }
      # get regions
      set reg1 [regReadAsFits [regNew] $file1]
      set reg2 [regReadAsFits [regNew] $file2]

      # get vectors
      set ncol1 [exprGet $reg1.ncol]
      set ncol2 [exprGet $reg2.ncol]
      set nrow1 [exprGet $reg1.nrow]
      set nrow2 [exprGet $reg2.nrow]
      set x1 [vIndex $ncol1]
      set y1 [vectorGetFromRegion $reg1 0 [expr $nrow1-1] 0 [expr $ncol1-1] 0] 
      set x2 [vIndex $ncol2]
      set y2 [vectorGetFromRegion $reg2 0 [expr $nrow1-1] 0 [expr $ncol2-1] 0] 
      if {$f == [lindex $filters [expr [llength $filters] - 1]]} {
           vNameSet $x1 "pixel" 
      } else {
           vNameSet $x1 "" 
      }
      vNameSet $x2 ""
      if {$c == [lindex $cols [expr [llength $cols] - 1]]} {
          vNameSet $y1 "counts"; 
      } else {
          vNameSet $y1 ""
      }
      vNameSet $y2 ""   
      pgstateSet $pg -isLine 1 -icLine 2

      if {$yminIn == ""} {
         set ymin [vExtreme $y1 min] 
         if {$ymin > [vExtreme $y2 min]} {set ymin [vExtreme $y2 min]}
      } else {
         set ymin $yminIn
      }
      if {$ymaxIn == ""} {
         set ymax [vExtreme $y1 max]
         if {$ymax < [vExtreme $y2 max]} {set ymax [vExtreme $y2 max]}
      } else {
         set ymax $ymaxIn
      } 
      vPlot $pg $x1 $y1 -ymin $ymin -ymax $ymax
      if {$c == 1 && $f == "r"} {
          titlePlot $title 40    
      }  

      pgstateSet $pg -icLine 4 -isNew 0
      vPlot $pg $x2 $y2 -ymin $ymin -ymax $ymax
      pgstateSet $pg -isNew 1

      # clean
      vectorExprDel $x1; vectorExprDel $x2
      vectorExprDel $y1; vectorExprDel $y2
      regDel $reg1
      regDel $reg2      
    }                   
  }    

  # close plot     
  endplot $pg
 

}  



######################################################################
# tools for studying aux files


# aperture correction etc. based on PSFstars*.dat:
proc plotPSFstars {file field1 field2 {SNRmin 100} {device /XWINDOW}} {

    # position, flags etc.
    set fieldInt  [lindex [file2Vector $file 2] 1]
    set col       [lindex [file2Vector $file 3] 1]
    set row       [lindex [file2Vector $file 4] 1]
    set field     [vectorExprEval $fieldInt+$row/1361.0]
    set PSFflag   [lindex [file2Vector $file 6] 1] 
    vNameSet $row "row (pixels)"
    vNameSet $col "col (pixels)"
    vNameSet $field "field"
   
    # quantities measured from observed profiles 
    set width   [lindex [file2Vector $file 10] 1] 
    vNameSet $width "obs. width (arcsec)"
    set psfC    [lindex [file2Vector $file 15] 1]
    set apC     [lindex [file2Vector $file 16] 1]                        
    set apCerr  [lindex [file2Vector $file 17] 1]
    set snr [vectorExprEval $apC/$apCerr]
    set apCorrObs   [vectorExprEval $apC/$psfC]
    vNameSet $psfC "psf counts (obs)"
    vNameSet $apC "aperture counts (obs)"
    vNameSet $snr "SNR"
    vNameSet $apCorrObs "apCounts/psfCounts (obs)"


    # quantities measured from modeled profiles 
    set PSFwidth   [lindex [file2Vector $file 11] 1] 
    vNameSet $PSFwidth "PSF width (arcsec)"
    set psfCPSF    [lindex [file2Vector $file 18] 1]
    set apCPSF     [lindex [file2Vector $file 19] 1]                        
    set apCorrPSF  [vectorExprEval $apCPSF/$psfCPSF]
    vNameSet $apCorrPSF "apCounts/raw psfCounts (PSF)"
    set apCorr     [lindex [file2Vector $file 8] 1]  
    vNameSet $apCorr "adopted aperture correction"

    # hybrid quantities
    set apCorRat [vectorExprEval $apCorr/$apCorrObs]
    vNameSet $apCorRat "adopted apCor/Obs apCor"
   

    # selection
    set Fmask [vectorExprEval "$field >= $field1 && $field <= [expr $field2 + 1] ? 1 : 0"]
    if {$SNRmin > 0} {
        set good  [vectorExprEval "$PSFflag == 1 && $snr > $SNRmin ? $Fmask : 0"]
    } else {
        set good  [vectorExprEval "$PSFflag == 1 ? $Fmask : 0"]
    }
    set Nall  [vectorExprGet sum($Fmask)]
    set Ngood [vectorExprGet sum($good)]
    echo "Out of $Nall stars from fields $field1-$field2"
    if {$SNRmin > 0} {
       echo "selected $Ngood with good PSF flag and SNR>$SNRmin" 
    } else {
       echo "selected $Ngood with good PSF flag" 
    }
    set apCmin 0.9;  set apCmax 1.1
    set Wmin 1.0;  set Wmax 2.0

    ### plot
    if [catch {
        # Set up the plot 
        set pg [pgstateNew]
        set device [devicename $device plotPSFstars.ps]
        pgstateSet $pg -device $device -lineWidth 3 -symb 4 \
          -xfract 0.35 -yfract 0.35 -nxwindow 3 -nywindow 2 
        pgstateOpen $pg
       
        vPlot $pg $apCorr $apCorrPSF -xmin $apCmin -xmax $apCmax -ymin $apCmin -ymax $apCmax -vectorMask $good
        plot_line $pg $apCmin $apCmin $apCmax $apCmax 1 2
        titlePlot "$file: fields $field1-$field2"  40                
        vPlot $pg $width $apCorr -xmin $Wmin -xmax $Wmax -ymin $apCmin -ymax $apCmax -vectorMask $good
        vPlot $pg $snr $apCorRat -xmin 0 -xmax 1000 -ymin 0.8 -ymax 1.2 -vectorMask $good
        vPlot $pg $apCorr $apCorrObs -xmin $apCmin -xmax $apCmax -ymin $apCmin -ymax $apCmax -vectorMask $good
        plot_line $pg $apCmin $apCmin $apCmax $apCmax 1 2
        vPlot $pg $width $PSFwidth  -xmin $Wmin -xmax $Wmax -ymin $Wmin -ymax $Wmax -vectorMask $good
        plot_line $pg $Wmin $Wmin $Wmax $Wmax 1 2
        vPlot $pg $width $apCorRat -xmin $Wmin -xmax $Wmax  -ymin 0.8 -ymax 1.2 -vectorMask $good
                      
        endplot $pg

    } msg] {
       echo "$msg"
       return 1
    }

    vListDel [list $fieldInt $col $row $field $PSFflag]
    vListDel [list $width $psfC $apC $apCerr $snr $apCorrObs]
    vListDel [list $PSFwidth $psfCPSF $apCPSF $apCorrPSF $apCorr]
    vListDel [list $Fmask $good $apCorRat]
 
}


# apCorr*.dat (produced by dump_PSFdata)
proc plot_apCorr {file {device /XWINDOW}} {


    ### get data
    set field  [lindex [file2Vector $file 1] 1] 
    set status [lindex [file2Vector $file 2] 1] 
    set width  [lindex [file2Vector $file 3] 1] 

    loop i 0 7 {
       set aC($i) [lindex [file2Vector $file [expr 4 + $i]] 1]    
    }

    ### plot
    if [catch {
        # Set up the plot 
        set pg [pgstateNew]
        set device [devicename $device plot_apCorr.ps]
        pgstateSet $pg -device $device -lineWidth 3 -symb -1 \
          -xfract 0.25 -yfract 0.35 -nxwindow 2 -nywindow 4 
        pgstateOpen $pg
       
        # set up histogram
        set min 0.7; set max 1.3
        set nbin [expr int(($max-$min)*100)]
        set xlabel "ap. corr."
        
        loop i 0 7 {        
           set hg [hgNew]
           hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlabel -yLabel N
           hgFillFromV $hg $aC($i)       
           hgPlot $pg $hg -xmin $min -xmax $max 
           hgDel $hg 
	   if {$i == 0} {titlePlot $file 40}
        }

        vPlot $pg $width $aC(3) -ymin $min -ymax $max -xmin 1.0
               
        endplot $pg

    } msg] {
       echo "$msg"
       return 1
    }

    vListDel [list $field $status $width]
    loop i 0 7 {
       vListDel [list $aC($i)]   
    }


}


### given a list of tsObj (or fpObj) files, plot
### mmod-mpsf and mapt7-mpsf vs. field for 
### specified filter
### e.g
### test_magTypes $s g 0 220 
proc test_magTypes {listORchain filter {lcFlag 1} {fieldMin 0} {dmMax 0.4} {device /XWINDOW} {title ""}} {

       set dmMin [expr -1*$dmMax] 

       set ifilter [lsearch {u g r i z} $filter]

       if {$lcFlag} {
         # read chain
         set c [tsObjList2chain $list]
         make_float_field $c $fieldMin 
         # select stars
         set s [chainSearch $c "{objc_type == OBJ_STAR} {psfCounts<$ifilter> > 10000}"]
         # and compute aperture counts
         apCounts2tsObj $s
       } else {
         set s $listORchain
       }   
      
       # extract vectors
       set reclist "psfCounts<$ifilter> counts_model<$ifilter> fiberCounts<$ifilter> objc_rowc"
       set veclist [vectorsFromChain $s $reclist] 
       set mpsf   [lindex $veclist 0]
       set mmod   [lindex $veclist 1]
       set mapert [lindex $veclist 2]
       set row [lindex $veclist 3]

       set mmp [vectorExprEval -2.5*lg($mmod/$mpsf)] 
       set pma [vectorExprEval -2.5*lg($mpsf/$mapert)] 

   
       vNameSet $row "field"
       vNameSet $mmp "mmod($filter)-mpsf($filter)"
       vNameSet $pma "mpsf($filter)-mapert7($filter)"

       plot_xy $row $mmp $device $title 4 1 1 "" "" $dmMin $dmMax

     #  plot_xy $row $pma $device $title 4 1 1 "" "" $dmMin $dmMax


     vListDel $veclist

     if {$lcFlag} {
        chainDel $s
        chainDestroy $c genericDel
     }

}



# aux proc for testing aperture/wing corrections
proc test_apCorr {device psFieldroot ifilter field1 field2 {title ""} {outfile ""}} {

     set chain [get_cal_chain_from_psField $psFieldroot $field1 $field2]
     set field [vFromChain $chain field]      
     set seeing [vFromChain $chain calib<$ifilter>->psf->width]      
     # wing correction (multiplicative)
     set wc [vFromChain $chain calib<$ifilter>->ap_corr_run] 
     set wcMag [vectorExprNew [exprGet $wc.dimen]]    
     loop i 0 [exprGet $wc.dimen] {               
        # wing correction in mags                    
        handleSet $wcMag.vec<$i> [expr 2.5*log10([exprGet $wc.vec<$i>])] 
     }
     # bias in aperture correction
     set bias [vFromChain $chain calib<$ifilter>->ap_corr_runErr]   
     # error in aperture correction
     set apCorrErr [vFromChain $chain calib<$ifilter>->psf_ap_correctionErr] 
     # bias in aperture correction corrected by wing correction
     set corrected [vectorExprEval $wcMag+$bias]
     # its median
     set median [format "%5.3f" [vMedian $corrected]]
     echo median for corrected: $median
     set corrcorr [vectorExprEval ($corrected-$median)]
     # chi for aperture correction
     set chi [vectorExprEval ($corrected-$median)/$apCorrErr]  
     set  medianChi [vSigma $chi]
     echo sigma for chi: $medianChi

     ### plot ###
     # Set up the plot and label
     set pg [pgstateNew]
     if {$outfile == ""} {
        set outfile test_apCorr.ps
     }
     set deviceA [devicename $device $outfile]
     set nxwindow 2; set nywindow 3
     set xf 0.3; set yf 0.35 
     pgstateSet $pg -device $deviceA -xfract $xf -yfract $yf \
               -nxwindow $nxwindow -nywindow $nywindow -symb 7 
     pgstateOpen $pg
     pgstateSet $pg -isNewplot 1 -isLine 1  -icMark 3 -symb 4 -icLine 2 -lineWidth 1 -lineStyle 1 

     # seeing
     vNameSet $field ""
     vNameSet $seeing "seeing (arcsec)"
     vPlot $pg $field $seeing -ymin 1.0 -ymax 3.0
     titlePlot "$title" 40

     # wing correction
     vNameSet $bias ""
     vPlot $pg $field $wcMag -ymin 0.0 -ymax 0.2 
     titlePlot "wing correction (mag)" 40

     # median for -2.5*log10(apCounts/psfCounts)
     vNameSet $bias ""
     vPlot $pg $field $bias -ymin -0.15 -ymax 0.15
     titlePlot "median: -2.5*log10(apCounts/psfCounts)" 40

     # width for -2.5*log10(apCounts/psfCounts)
     vNameSet $apCorrErr ""
     vPlot $pg $field $apCorrErr -ymin 0.0 -ymax 0.1
     titlePlot "width: -2.5*log10(apCounts/psfCounts)" 40

     # corrected
     vNameSet $field "field"
     vNameSet $corrcorr ""
     vPlot $pg $field $corrcorr -ymin -0.15 -ymax 0.15
     plot_line $pg 0 0 1000 0 
     titlePlot "(wing corr. + median)-$median" 40

     # chi
     vNameSet $bias ""
     vPlot $pg $field $chi -ymin -3.0 -ymax 3.0 
     plot_line $pg 0 0 1000 0
     titlePlot "total corr./error" 40

     endplot $pg  

     genericChainDestroy $chain 
     vListDel [list $wc $apCorrErr $corrcorr]
     vListDel [list $field $seeing $bias $wcMag $corrected $chi]
} 


proc vLog {vec} {

  # get log of a vector
  set N [exprGet $vec.dimen]
  set log [vectorExprNew $N]
  loop i 0 $N {
     set val [exprGet $vec.vec<$i>]
     if {$val > 0} {
         handleSet $log.vec<$i> [expr log10($val)] 
     } else {
         handleSet $log.vec<$i> -1000.0
     }
  }
  return $log

}

# given a psFangCorr*.dat file plot various QA plots
proc plot_psFangCorr {file {device /XWINDOW} {filters {u g r i z}} \
                           {field1 -1} {field2 -1}} {

global pg

    set filter   [lindex [file2Vector $file 1] 1]
    set Intfield [lindex [file2Vector $file 2] 1]
    set width    [lindex [file2Vector $file 3] 1] 
    set row      [lindex [file2Vector $file 4] 1]
    set col      [lindex [file2Vector $file 5] 1]
    set rowErr   [lindex [file2Vector $file 6] 1]
    set colErr   [lindex [file2Vector $file 7] 1]
    set rowBias  [lindex [file2Vector $file 8] 1]
    set colBias  [lindex [file2Vector $file 9] 1]
    set Rcounts  [lindex [file2Vector $file 10] 1]
    set counts   [vLog $Rcounts]
    set field    [vectorExprEval $Intfield+$row/1361.0]
    set rowChi   [vectorExprEval $rowBias/$rowErr]
    set colChi   [vectorExprEval $colBias/$colErr]

    vNameSet $row "row (pixels)"
    vNameSet $col "col (pixels)"
    vNameSet $width  "PSF width (\")"
    vNameSet $field  "field"
    vNameSet $counts "log(counts)"

    if {$field2 < 0 || $field2 < $field1} {
       set field2 1000
       set title "$file: all fields"
    } else {
       if {$field1 == $field2} {
          set title "$file: field $field1"
       } else {
          set title "$file: fields $field1-$field2"
       }
    }

    set Fmask [vectorExprEval "$field >= $field1 && $field <= $field2 ? 1 : 0"]
    set Nall  [vectorExprGet sum($Fmask)]
    echo "Selected $Nall stars from fields $field1-$field2"
   

    foreach f $filters {
   
        # first get stars from this band
        set ifilter [lsearch {u g r i z} $f]
        set mask [vectorExprEval "$filter == $ifilter ? $Fmask : 0"]       
        set Nf  [vectorExprGet sum($mask)]
        echo "Selected $Nf stars from band $f"

        # now plot all diagrams for this filter 
        if [catch {
          # Set up the plot 
          set pg [pgstateNew]
          set dev [devicename $device psFangCorr-Bias-$f.ps]
          pgstateSet $pg -device $dev -lineWidth 3 -symb 1 \
              -xfract 0.3 -yfract 0.4 -nxwindow 2 -nywindow 6 
          pgstateOpen $pg
       
          set biasMin -0.2; set biasMax 0.2
          vPlot $pg $width $rowBias -vectorMask $mask -ymin $biasMin -ymax $biasMax \
                           -xmin 1.2 -xmax 2.2
          plot_line $pg 0 0.0 10 0.0 1 13
          titlePlot "$title, filter $f" 40
          vPlot $pg $width $colBias -vectorMask $mask -ymin $biasMin -ymax $biasMax \
                           -xmin 1.2 -xmax 2.2
          plot_line $pg 0 0.0 10 0.0 1 13
          vNameSet $rowBias "rowBias (pixels)"
          vPlot $pg $col $rowBias -vectorMask $mask -ymin $biasMin -ymax $biasMax 
          plot_line $pg -1000 0.0 3000 0.0 1 13
          vNameSet $rowBias "" 
          vNameSet $colBias "colBias (pixels)"
          vPlot $pg $col $colBias -vectorMask $mask -ymin $biasMin -ymax $biasMax
          plot_line $pg -1000 0.0 3000 0.0 1 13
          vNameSet $colBias ""
          vPlot $pg $counts $rowBias -vectorMask $mask -xmin 3 -ymin $biasMin -ymax $biasMax 
          plot_line $pg -1000 0.0 3000 0.0 1 13
          vPlot $pg $counts $colBias -vectorMask $mask -xmin 3 -ymin $biasMin -ymax $biasMax
          plot_line $pg -1000 0.0 3000 0.0 1 13
          vPlot $pg $field $rowBias -vectorMask $mask -ymin $biasMin -ymax $biasMax 
          plot_line $pg -1000 0.0 3000 0.0 1 13
          vPlot $pg $field $colBias -vectorMask $mask -ymin $biasMin -ymax $biasMax
          plot_line $pg -1000 0.0 3000 0.0 1 13
          vPlot $pg $field $width -vectorMask $mask -ymin 1.0 -ymax 2.0 
          vPlot $pg $field $width -vectorMask $mask -ymin 1.0 -ymax 2.0 
               
          endplot $pg
        } msg] {
          echo "problems with filter $f:"
          echo "$msg"
          return 1
        }
        
        # now plot all diagrams for this filter 
        if [catch {
          # Set up the plot 
          set pg [pgstateNew]
          set dev [devicename $device psFangCorr-Chi-$f.ps]
          pgstateSet $pg -device $dev -lineWidth 3 -symb 1 \
              -xfract 0.3 -yfract 0.4 -nxwindow 2 -nywindow 5 
          pgstateOpen $pg
       
          set chiMin -20; set chiMax 20
          vPlot $pg $width $rowChi -vectorMask $mask -ymin $chiMin -ymax $chiMax \
                     -xmin 1.2 -xmax 2.2
          plot_line $pg 0 0.0 10 0.0 1 13
          titlePlot "$title, filter $f" 40
          vPlot $pg $width $colChi -vectorMask $mask -ymin $chiMin -ymax $chiMax \
                     -xmin 1.2 -xmax 2.2
          plot_line $pg 0 0.0 10 0.0 1 13
          vNameSet $rowChi "rowBias/rowErr"
          vPlot $pg $col $rowChi -vectorMask $mask -ymin $chiMin -ymax $chiMax  
          plot_line $pg -1000 0 3000 0 1 13
          vNameSet $rowChi ""
          vNameSet $colChi "colBias/colErr"
          vPlot $pg $col $colChi -vectorMask $mask -ymin $chiMin -ymax $chiMax 
          plot_line $pg -1000 0 3000 0 1 13
          vNameSet $colChi ""
          vPlot $pg $counts $rowChi -vectorMask $mask -xmin 3 -ymin $chiMin -ymax $chiMax  
          plot_line $pg 0 0.0 10 0.0 1 13
          vPlot $pg $counts $colChi -vectorMask $mask -xmin 3 -ymin $chiMin -ymax $chiMax
          plot_line $pg 0 0.0 10 0.0 1 13
          vPlot $pg $field $rowChi -vectorMask $mask -ymin $chiMin -ymax $chiMax  
          plot_line $pg 0 0.0 1000 0.0 1 13
          vPlot $pg $field $colChi -vectorMask $mask -ymin $chiMin -ymax $chiMax 
          plot_line $pg 0 0.0 1000 0.0 1 13
               
          endplot $pg
        } msg] {
          echo "problems with filter $f:"
          echo "$msg"
          return 1
        }

        vectorExprDel $mask
       
    }

    # clean
    vListDel [list $filter $width $col $row $field $Intfield $Fmask $Rcounts $counts]
    vListDel [list $colChi $rowChi $colErr $rowErr $colBias $rowBias]


}


# given a PSFstars-*.dat file plot 
# - the width of individual stars vs. field
# - the width histogram
proc plot_star_width {file field1 field2 {wmin ""} {wmax ""} \
                      {device /XWINDOW}} {

global pg

    set Intfield  [lindex [file2Vector $file 2] 1]
    set col     [lindex [file2Vector $file 3] 1]
    set row     [lindex [file2Vector $file 4] 1]
    set PSFflag [lindex [file2Vector $file 6] 1] 
    set width   [lindex [file2Vector $file 10] 1] 
    set field [vectorExprEval $Intfield+$row/1361.0]
    vNameSet $row "row (pixels)"
    vNameSet $col "col (pixels)"
    vNameSet $width "width (arcsec)"
    vNameSet $field "field"
   

    set Fmask [vectorExprEval "$field >= $field1 && $field <= $field2 ? 1 : 0"]
    set good  [vectorExprEval "$PSFflag == 1 ? $Fmask : 0"]
    set Nall  [vectorExprGet sum($Fmask)]
    set Ngood [vectorExprGet sum($good)]
    echo "Selected $Nall stars from fields $field1-$field2, $Ngood are good PSF stars"   
    if {$field1 == $field2} {
        set title "field $field1"
    } else {
        set title "fields $field1-$field2"
    }
    if {$wmin == ""} {set wmin [vExtreme $width min -vMask $Fmask]}
    if {$wmax == ""} {set wmax [vExtreme $width max -vMask $Fmask]}  

    ### width vs. field
    # first plot all stars as crosses
    plot_xy $field $width $device $title 5 1 0 $field1 $field2 $wmin $wmax $Fmask
    # and then only good stars as circles
    plot_xy $field $width $device $title 4 0 1 $field1 $field2 $wmin $wmax $good
    if {$device != "/XWINDOW"} {
        catch {exec mv plot_xy.ps plot_star_width1.ps}
    }

    ### histogram width
    plot_histogram $device $width 20 $wmin $wmax "width (arcsec)" "all: $title" "" 1 $Fmask
    if {$device != "/XWINDOW"} {
        catch {exec mv plot_histogram.ps plot_star_width2.ps}
    }
    plot_histogram $device $width 20 $wmin $wmax "width (arcsec)" "$title: good" "" 1 $good
    if {$device != "/XWINDOW"} {
        catch {exec mv plot_histogram.ps plot_star_width3.ps}
    }
    

    vListDel [list $width $PSFflag $col $row $field $Intfield $Fmask $good]

}



# given a PSFstars-*.dat file plot the distribution of stars in col. vs. field 
# and specially mark stars with wmin < width < wmax
proc plot_star_width_distribution {file field1 field2 wmin1 wmax1 wmin2 wmax2 \
                      {device /XWINDOW}} {

global pg

    set Intfield  [lindex [file2Vector $file 2] 1]
    set col     [lindex [file2Vector $file 3] 1]
    set row     [lindex [file2Vector $file 4] 1]
    set PSFflag [lindex [file2Vector $file 6] 1] 
    set width   [lindex [file2Vector $file 10] 1] 
    set field [vectorExprEval $Intfield+$row/1361.0]
    vNameSet $row "row (pixels)"
    vNameSet $col "col (pixels)"
    vNameSet $width "width (arcsec)"
    vNameSet $field "field"
   

    set Fmask [vectorExprEval "$field >= $field1 && $field <= $field2 ? 1 : 0"]
    set good  [vectorExprEval "$PSFflag == 1 ? $Fmask : 0"]
    set goodW1 [vectorExprEval "$width > $wmin1 && $width < $wmax1 ? $Fmask : 0"]
    set goodW2 [vectorExprEval "$width > $wmin2 && $width < $wmax2 ? $Fmask : 0"]

    set Nall  [vectorExprGet sum($Fmask)]
    set Ngood [vectorExprGet sum($good)]
    set NgoodW1 [vectorExprGet sum($goodW1)]
    set NgoodW2 [vectorExprGet sum($goodW2)]

    echo "Selected $Nall stars from fields $field1-$field2, $Ngood are good PSF stars"   
    echo "$NgoodW1 have the width $wmin1 < width < $wmax1, and"   
    echo "$NgoodW2 have the width $wmin2 < width < $wmax2"   
    if {$field1 == $field2} {
        set title "field $field1, circ: $wmin1 < width < $wmax1, triang: $wmin2 < width < $wmax2"
    } else {
        set title "fields $field1-$field2, circ: $wmin1 < width < $wmax1, triang: $wmin2 < width < $wmax2"
    }
   
    ### col vs. field
    # first plot all stars as crosses
    plot_xy $field $col $device $title 4 1 0 $field1 $field2 0 2048 $goodW1
    # and then only good stars as circles
    plot_xy $field $col $device $title 13 0 1 $field1 $field2 0 2048 $goodW2
    if {$device != "/XWINDOW"} {
        catch {exec mv plot_xy.ps plot_star_width_distribution.ps}
    }

    vListDel [list $width $PSFflag $col $row $field $Intfield $Fmask $good $goodW1 $goodW2]

}

### given an opBC file, print all bad columns for specified chip
proc printBC {opBCfile chipRow chipCol} {

     set bclist [param2Chain $opBCfile  bcpars]
     set defects [find_defects $bclist $chipRow $chipCol]
     loop k 0 [chainSize $defects] {
        set el [chainElementGetByPos $defects $k]
        echo "defect $k at [exprGet $el.dfcol0]"
     }

     chainDestroy $bclist genericDel
     chainDestroy $defects genericDel
}


#### 
# given an /objcs directory with psSL files, make a mosaic for
# specified columns and filters
# rowBin and colBin specify binning factors
proc makeSLmosaic {run {rerun ""} {dir "default"} {camCols ""} \
	           {filters ""} {rowBin 1} {colBin 4} } {

global data_root 
   
    if {$dir == "default"} {
        set dir $data_root/$run/$rerun/objcs          
    }
    if {$camCols == ""} {set camCols {1 2 3 4 5 6}}
    if {$filters == ""} {set filters {g z u i r}}   
 
    set L {}
    set runstr  [format "%06d" $run]
    set Ncol [llength $camCols]
    set Nfilters [llength $filters]
    foreach f $filters {
       foreach c $camCols {
	  lappend L $dir/$c/psSLprof-$runstr-${f}${c}.fit 
       }
    } 

    set gutterRow [expr 2*$rowBin]
    set gutterCol [expr 2*$colBin]      
    set mosaic [fitlist2mosaic $L / $Ncol $Nfilters 0 $gutterRow $gutterCol] 

    set mosaicBinned [regBin $mosaic -row $rowBin -col $colBin -normalize 1]
    regDel $mosaic

  return $mosaicBinned

}

# plot 
proc plot_apC_field {file field1 field2 {device /XWINDOW}} {

    set width [lindex [file2Vector $file 10] 1] 
    set PSFwidth [lindex [file2Vector $file 15] 1] 
    set psfC [lindex [file2Vector $file 11] 1] 
    set PSFpsfC [lindex [file2Vector $file 16] 1] 
    set apC [lindex [file2Vector $file 12] 1]                        
    set apCerr [lindex [file2Vector $file 13] 1]
    set nsr [vectorExprEval $apCerr/$apC]
    set apCorrKL [lindex [file2Vector $file 8] 1]
    set apCorrTrue [vectorExprEval $psfC/$apC]
    set flag [lindex [file2Vector $file 6] 1]
    set fields [lindex [file2Vector $file 2] 1]
    set col [lindex [file2Vector $file 3] 1]
    set row [lindex [file2Vector $file 4] 1]
    set masterRow [lindex [file2Vector $file 5] 1]


    set mask [vectorExprEval "$fields >= $field1 && $fields <= $field2 ? $flag : 0"]
    set Nflag [vectorExprGet sum($flag)]
    set Nmask [vectorExprGet sum($mask)]
    echo "Nflag = $Nflag, from this field: $Nmask"
    vNameSet $row "row (pixels)"
    vNameSet $masterRow "master row (pixels)"
    vNameSet $col "col (pixels)"
    vNameSet $width "stars's width"
    vNameSet $apCorrTrue "psfCounts/apCounts (true ap.corr)"
    vNameSet $apCorrKL "ap. corr. from phPsfSetAtPoint"
    vNameSet $nsr "NSR (=1/SNR)"
    vNameSet $PSFwidth "KL PSF width"
    vNameSet $PSFpsfC "KL psf Counts"
    vNameSet $psfC "stars psf Counts"
    if {$field1 == $field2} {
        set title "field $field1"
    } else {
        set title "fields $field1-$field2"
    }
    # not always available (#if in findPsf.c)
    plot_xy $width $PSFwidth $device $title "" 1 1 1.0 2.7 1.0 2.7 $mask
    plot_xy $psfC  $PSFpsfC  $device $title "" 1 1 "" "" "" "" $mask
    # positions
    plot_xy $col $row $device $title "" 1 1 0 2048 0 1361 $mask
    plot_xy $row $width $device $title "" 1 1 0 1361 1.0 2.7 $mask
    plot_xy $col $width $device $title "" 1 1 0 2048 1.0 2.7 $mask
    # other 
    plot_xy $apCorrTrue $apCorrKL $device $title "" 1 1 0.8 1.2 0.8 1.2 $mask
    plot_xy $nsr $apCorrTrue $device $title "" 1 1 0 0.02 0.8 1.2 $mask
    plot_xy $nsr $apCorrKL $device $title "" 1 1 0 0.02 0.8 1.2 $mask
    plot_xy $width $apCorrTrue $device $title "" 1 1 1.0 2.7 0.8 1.2 $mask

    plot_xy $nsr $width $device $title "" 1 1  0 0.02 1.0 2.7  $mask
    plot_xy $psfC $width $device $title "" 1 1 "" ""  1.0 2.7  $mask



    vListDel [list $width $psfC $apC $apCerr $nsr $apCorrTrue $apCorrKL $flag]
    vListDel [list $col $row $masterRow $fields $mask]

}


# given a chain of objects read from tsObj files, e.g.

# set chain [tsObjList2chain [glob tsObj-001350-5-5-00*.fit] 1] 
# plot histogram vs. chip column for a flux limited sample of stars
proc tsObj2pixcolhist {device chain filter mmax nbin {objc_type "all"}} { 
 
    set filterlist {u g r i z}
    set ifilter [lsearch $filterlist $filter]

    if {$objc_type == "all"} {
        set s [chainSearch $chain "{psfCounts<$ifilter> < $mmax}"]
    } else {
        set s [chainSearch $chain "{psfCounts<$ifilter> < $mmax} {objc_type == 6}"]
    }
    set col [vFromChain $s objc_colc]                             
    plot_histogram $device $col $nbin 0 2048

    vectorExprDel $col
    chainDel $s

}                        
   
# given a list of idR files, plot odd/even difference 
proc testOddEven {root f1 f2 {dev /XWINDOW}} {

     set N [expr $f2-$f1+1]
     set diff [vectorExprNew $N]
     set fields [vectorExprNew $N]

     loop i $f1 $f2 {
        set idR $root-[format "%04d" $i].fit
        set reg [regReadAsFits [regNew] $idR]
        set k [regGetOddEven $reg]
        regDel $reg
        handleSet $diff.vec<$i> [expr [lindex $k 0] - [lindex $k 2]]
        handleSet $fields.vec<$i> $i
     }
     
     # plot
     set title "Odd/even for $root from $f1 to $f2"
     plot_xy $fields $diff $dev $title

     vectorExprDel $diff
}

   
# given an idR file, return its region and a region with odd/even difference 
# subtracted, where odd/even difference is determined from the first 20 columns 
proc testCorrectOddEven {idR} {
 
     set reg [regReadAsFits [regNew] $idR]
     set out [regNew [exprGet $reg.nrow] [exprGet $reg.ncol]]
     regClear $out

     # loop over rows
     loop row 0 [exprGet $reg.nrow] {
         echo row $row
         set bias [subRegNew $reg 1 20 $row 0] 
         set OE   [regGetOddEven $bias]
         handleDel $bias
         set sub    [subRegNew $reg 1 [exprGet $reg.ncol] $row 0]
         set subout [subRegNew $out 1 [exprGet $reg.ncol] $row 0]
         regAddRow $subout $sub
         regAddOE $subout [lindex $OE 0] [lindex $OE 2]
         regDel $sub; regDel $subout
     }
    
  return [list $reg $out]  
      
}

# given a region, add to each odd column pixel -0.5*(odd-even)+0.5,
# and to each even column pixel 0.5*(odd-even)+0.5,
# and correct for the mean level, and add 1000
proc regAddOE {reg odd even} {

     set corr [expr 0.5*($odd+$even)]
     set OEcorr [expr 0.5*($odd-$even)]

     loop row 0 [exprGet $reg.nrow] {
         loop col 0 [exprGet $reg.ncol] {
             set old [exprGet $reg.rows_u16<$row><$col>]
	     if {[expr $col/2*2] != $col} {
                # odd
                set new [expr $old - $OEcorr + 0.5 + 1000 - $corr] 
             } else {
                # odd
                set new [expr $old + $OEcorr + 0.5 + 1000 - $corr] 
             }
             handleSet $reg.rows_u16<$row><$col> $new
	 }
     }
}

   
# given a file name with a region, return a vector with medians along 
# the row direction
proc regFileGetMedianVector {regFile} {
 
     set reg [regReadAsFits [regNew] $regFile]
   return [regGetMedianVector $reg]  
      
}

   
# given a region, return a vector with medians along the row direction
proc regGetMedianVector {reg} {
 
     set regM [regNew 1 [exprGet $reg.ncol]]

     # get 1xNcol region of medians for each column
     regMedianFindByColumn $reg $regM
     
   return [vectorGetFromRegion $regM 0 0 0 [expr [exprGet $regM.ncol]-1] 0 -median]
      
}


# given (run, camcol, filter, startField, endField, fangDir, fieldDir) 
# return the corresponding quartile region and median computed from 
# raw images 
proc checkQuartiles {run camCol filter startField endField fangDir fieldDir} {

   set nfield [expr 1+$endField-$startField]
   set quartiles_nc 2128
   set quartiles [quartilesNew $nfield $quartiles_nc]

   set file [get_scfang_filename $fangDir $run $filter $camCol $startField]
   set hdu [get_hdu $file $filter quarts]

   loop fld $startField [expr $endField+1] {

      set scFang_format "scFang-%06d-%d-%04d.fit $run $camCol $fld" 
      set file [eval format $scFang_format]
      set file $fangDir/$file      

      set tblcol [handleNewFromType TBLCOL]
      fitsRead $tblcol $file -binary -hdu $hdu

      # Check that row is correct
      set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
      assert {$hfilter == $filter}
      assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "QFLAT"} 

      if [catch {
	 tblcolToQuartiles $tblcol $quartiles [expr $fld-$startField]} msg] {
	 error "file $file $filter: $msg"
      }
      set tshift [hdrGetAsInt $tblcol.hdr TSHIFT]

      handleDelFromType $tblcol
   }

   
   set median [handleBindFromHandle [handleNew] *$quartiles.data<1>]
   set quartileReg [regNew -type FL32 [exprGet $median.nrow] [exprGet $median.ncol]]
   set medianReg   [regNew -type FL32 [exprGet $median.nrow] [exprGet $median.ncol]]
   set diffReg   [regNew -type FL32 [exprGet $median.nrow] [exprGet $median.ncol]]

   # quartile region from fangs (except for S32->FL32)
   regMultiplyWithDbl $median [expr 1.0/$tshift] -regOut $quartileReg
   handleDel $median
   quartilesDel $quartiles
  

   ### now read each raw image and populate medianReg 
   set regM [regNew 1 -type FL32 [exprGet $quartileReg.ncol]]

   loop fld $startField [expr $endField+1] {
      set idR_format "idR-%06d-%s%d-%04d.fit $run $filter $camCol $fld" 
      set file [eval format $idR_format]
      echo medianing raw image $file 
      set file $fieldDir/$file      
      set Rreg [regReadAsFits [regNew] $file] 

      # get 1xNcol region of medians for each column
      regMedianFindByColumn $Rreg $regM
      regMultiplyWithDbl $regM 1.0 -regOut $regM

      # copy to output region
      set ifld [expr $fld-$startField]
      set thisRow [subRegNew $medianReg 1 [exprGet $medianReg.ncol] $ifld 0]
      regClear $thisRow
      regAdd $thisRow $regM

      regDel $Rreg
   }        

   regSubtract $quartileReg $medianReg -regOut $diffReg 

   return [list $quartileReg $medianReg $diffReg]

}


# given (run, camcol, filter, startField, endField, fangDir, objcsDir) 
# return the corrected quartile region:
# corrQ = (quartiles-better_bias)*flatfield
proc correctQuartiles {run camCol filter startField endField fangDir objcsDir \
	               {display 1} } {

      set nCol 2048
      set bias_scale 32
      set ff_scale 2048    
    
      ### read better bias and flatfield files
      # psFF file
      set format "psFF-%06d-%s%d.fit $run $filter $camCol" 
      set file $objcsDir/[eval format $format]
      set FFreg [regReadAsFits [regNew -type FL32] $file] 
      if {$display > 1} {
          dervishPlot $FFreg -row 0 -title "flatfield"
      }
      assert { [exprGet $FFreg.ncol] == $nCol }

      set vJumpFFvec [get_jump $FFreg 1024 200 200 4]
      set vJumpFF [exprGet $vJumpFFvec.vec<0>]
      vectorExprDel $vJumpFFvec 

      loop field $startField [expr $endField+1] {

         # psBB files
         set format "psBB-%06d-%s%d-%04d.fit $run $filter $camCol $field" 
         set file $objcsDir/[eval format $format]
         set rawBBreg [regReadAsFits [regNew -type FL32] $file]                 
         set BBreg [subRegNew $rawBBreg [exprGet $rawBBreg.nrow] $nCol 0 40] 

   
  #  correct_gain $BBreg 1024 [expr 1.002*$vJumpFF] $bias_scale 115
 
         if {$display > 1 && $field == $startField} {
             dervishPlot $BBreg -row 0 -title  "better bias for field $field"
         } elseif {$display > 2} {
             dervishPlot $BBreg -row 0 -title  "better bias for field $field"
	 }

         assert { [exprGet $BBreg.ncol] == $nCol }

         # pseudo bias
         set BBp($field) [regNew -type FL32 1 $nCol] 
         regMultWithRow $BBreg $FFreg -scale [expr $bias_scale * $ff_scale] \
                         -regOut $BBp($field) 

         if {$display && $field == $startField} {
             dervishPlot $BBp($field) -row 0 -title  "pseudo better bias ($field)" 
         } elseif {$display > 2} {
             dervishPlot $BBp($field) -row 0 -title  "pseudo better bias ($field)"
         }
         regDel $BBreg; regDel $rawBBreg
      }

      ### quartiles
      set nfield [expr 1+$endField-$startField]
      set quartiles_nc 2128
      set quartiles [quartilesNew $nfield $quartiles_nc]

      set file [eval format "scFang-%06d-%d-%04d.fit" $run $camCol $startField]
      set file $fangDir/$file      
      set hdu [get_hdu $file $filter quarts]
      loop fld $startField [expr $endField+1] {
         set file [eval format "scFang-%06d-%d-%04d.fit" $run $camCol $fld]
         set file $fangDir/$file      

         set tblcol [handleNewFromType TBLCOL]
         fitsRead $tblcol $file -binary -hdu $hdu

         # Check that row is correct
         set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
         assert {$hfilter == $filter}
         assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "QFLAT"} 

         if [catch {
	    tblcolToQuartiles $tblcol $quartiles [expr $fld-$startField]} msg] {
	    error "file $file $filter: $msg"
         }
         set tshift [hdrGetAsInt $tblcol.hdr TSHIFT]
         handleDelFromType $tblcol
      }

      set rawQuartilesScaled [handleBindFromHandle [handleNew] *$quartiles.data<1>]
      assert { [exprGet $rawQuartilesScaled.nrow] == $nfield } 
      set rawQuartilesTrimmed [subRegNew $rawQuartilesScaled $nfield $nCol 0 40]  

      # quartile region on the same scale as idR files
      set rawQ [regNew -type FL32 $nfield $nCol]
      regMultiplyWithDbl $rawQuartilesTrimmed [expr 1.0/$tshift] -regOut $rawQ
      # output region with corrected quartiles
      set corrQ [regNew -type FL32 $nfield $nCol]
      # output region with corrected quartiles scaled by the sky
      set corrScaled [regNew -type FL32 $nfield $nCol]
      # pseudo-raw
      set pRaw [regNew -type FL32 1 $nCol] 


      ### correct raw quartile image, row by row
      ### corrected = pseudo_raw - pseudo_better_bias 
      echo " ** correcting image **" 
      loop irow 0 $nfield {
          set field [expr $startField + $irow]
          set corrRow [subRegNew $corrQ 1 $nCol $irow 0]
          set corrScaledRow  [subRegNew $corrScaled 1 $nCol $irow 0]
          set rawRow  [subRegNew $rawQ  1 $nCol $irow 0]
          # pseudo-raw
          regMultWithRow $rawRow $FFreg -scale $ff_scale -regOut $pRaw
          # subtract pseudo-bias
          regSubtract $pRaw $BBp($field) -regOut $corrRow
          # get sky
          set corrRowu16 [regNew -type U16 1 $nCol]
          regClear $corrRowu16
          regAdd $corrRowu16 $corrRow          
          set corrVec [vectorGetFromRegion $corrRowu16 \
                       0 0 0 [expr $nCol-1] 0]
          set sky [vMean $corrVec]; vectorExprDel $corrVec
          set corrVec [vectorGetFromRegion $corrRowu16 \
                       0 0 0 [expr $nCol-1] 0 -median]
          set skyMed [vMedian $corrVec]; vectorExprDel $corrVec
          regDel $corrRowu16
          echo "correcting field $field, sky=$sky (median=$skyMed)"
          regMultiplyWithDbl $corrRow [expr 10000.0/$sky] -regOut $corrScaledRow        
          regDel $rawRow; regDel $corrRow; regDel $corrScaledRow 
      }

      set corrQu16 [regNew -type U16 $nfield $nCol]
      set corrScaledQu16 [regNew -type U16 $nfield $nCol]
      regClear $corrQu16; regClear $corrScaledQu16
      regAdd $corrQu16 $corrQ
      regAdd $corrScaledQu16 $corrScaled
      regDel $corrScaled 

      # plot median for corrected image
      if {$display} {
          # output region with corrected quartiles as U16
          if {$display > 1} {
             echo "median background for the corrected quartile image (U16)" 
             plotMed $corrQu16
          }
          echo "median background for the scaled corrected quartile image (U16)" 
          plotMed $corrScaledQu16 "0 2048" "9900 10100"
      } 

      # clean
      foreach reg "$FFreg $pRaw $rawQ $rawQuartilesTrimmed" {
         regDel $reg
      } 
      loop field $startField [expr $endField+1] {
         regDel $BBp($field) 
      }
      quartilesDel $quartiles

    echo "returning correct image"
    return "$corrQ $corrQu16 $corrScaledQu16"

}



# given (run, camcol, filter, field, fieldDir, biasDir, objcsDir), return corrected image
# corrected = (idR - psBB/32)*psFF/2048
# 2048*corrected = idR*psFF - psBB*psFF/32
# slowCorrect 745 1 i 400 fields/1 ../photo/calib objcs/1 1
proc slowCorrect {run camCol filter field fieldDir biasDir objcsDir \
		      {display 0} {osWidth 40} {imageWidth 2048}} {

    set bias_scale 32
    set ff_scale 2048    
  
      # read raw image
      set format "idR-%06d-%s%d-%04d.fit $run $filter $camCol $field" 
      set file $fieldDir/[eval format $format]
      set rawRreg [regReadAsFits [regNew] $file] 
      set Rreg [subRegNew $rawRreg [exprGet $rawRreg.nrow] $imageWidth 0 $osWidth] 
      # plot median
      if {$display > 2} {
          echo "median background for the raw image" 
          plotMed $reg 
      }

      # read original bias
      set format "idB-%06d-%s%d.fit $run $filter $camCol" 
      set file $biasDir/[eval format $format]
      set rawiBreg [regReadAsFits [regNew -type FL32] $file] 
      set iBreg [subRegNew $rawiBreg [exprGet $rawiBreg.nrow] [exprGet $Rreg.ncol] 0 $osWidth] 
      if {$display > 1} {
          dervishPlot $iBreg -row 0 -title  "original bias"
      }

      # read better bias
      set format "psBB-%06d-%s%d-%04d.fit $run $filter $camCol $field" 
      set file $objcsDir/[eval format $format]
      set rawBBreg [regReadAsFits [regNew -type FL32] $file]                 
      set BBreg [subRegNew $rawBBreg [exprGet $rawBBreg.nrow] [exprGet $Rreg.ncol] 0 $osWidth] 
      #  regIntLincom $BBreg "" 0 [expr 1.0/$bias_scale] 0
      if {$display > 1} {
          dervishPlot $BBreg -row 0 -title  "better bias"
      }


      # difference in biases is presumed scattered light correction 
      set corrSLreg [regNew -type FL32 [exprGet $rawBBreg.nrow] [exprGet $Rreg.ncol]] 
      regSubtract $BBreg $iBreg -regOut $corrSLreg     
      if {$display > 1} {
          dervishPlot $corrSLreg -row 0 -title  "better bias - original bias"
      }
      

      # read flatfield
      set format "psFF-%06d-%s%d.fit $run $filter $camCol" 
      set file $objcsDir/[eval format $format]
      set FFreg [regReadAsFits [regNew -type FL32] $file] 
      #  regIntLincom $FFreg "" 0 [expr 1.0/$ff_scale] 0
      if {$display > 1} {
          dervishPlot $FFreg -row 0 -title "flatfield"
      }

      ### correct raw image
      # pseudo-raw
      set pRreg [regNew -type FL32 [exprGet $Rreg.nrow] [exprGet $Rreg.ncol]] 
      regMultWithRow $Rreg $FFreg -scale $ff_scale -regOut $pRreg
      
      # pseudo-bias
      # with original bias
      set iBp [regNew -type FL32 1 [exprGet $Rreg.ncol]] 
      regMultWithRow $iBreg $FFreg -scale [expr $bias_scale * $ff_scale] -regOut $iBp
      if {$display > 2} {
          dervishPlot $iBp -row 0 -title  "pseudo original bias"
      }
      # with better bias
      set BBp [regNew -type FL32 1 [exprGet $Rreg.ncol]] 
      regMultWithRow $BBreg $FFreg -scale [expr $bias_scale * $ff_scale] -regOut $BBp
      if {$display > 2} {
          dervishPlot $BBp -row 0 -title  "pseudo better bias"
      }
      # difference in pseudo biases is scattered light
      set SL [regNew -type FL32 1 [exprGet $Rreg.ncol]] 
      regAddWithDbl $BBp 1000
      regSubtract $BBp $iBp -regOut $SL      
      if {$display} {
           dervishPlot $SL -row 0 -title  "sky*scattered_light+1000 =pseudo(BB-iB)"
      }
      regAddWithDbl $BBp -1000

      # plot median for pseudo raw  
      if {$display > 2} {
          echo "median background for the PSEUDO raw image (raw*FF)" 
          plotMed $pRreg 
      }

      # corrected = pseudo_raw - pseudo_better_bias 
      set corr [regNew -type U16 [exprGet $pRreg.nrow] [exprGet $pRreg.ncol]] 
      echo " correcting image" 
      loop irow 0 [exprGet $corr.nrow] {
          set corrRow [subRegNew $corr 1 [exprGet $corr.ncol] $irow 0]
          set rawRow  [subRegNew $pRreg 1 [exprGet $pRreg.ncol] $irow 0]
          regSubtract $rawRow $BBp -regOut $corrRow
          regDel $rawRow; regDel $corrRow 
      }
      # plot median for corrected image
      if {$display > 1} {
          echo "median background for the corrected image" 
          plotMed $corr
      }

      set rList "$Rreg $rawRreg $iBreg $rawiBreg $BBreg $rawBBreg $corrSLreg $FFreg $pRreg $iBp $BBp $SL"
      foreach reg $rList {
         regDel $reg
      } 

    echo "returning correct image"
    return $corr
}



## aux testing proc for overwriting quartiles with medians directly determined
## from raw images
proc quartiles_overwrite {quartiles fieldsDir run filter camCol startfield endfield tshift} {

   set medianQreg [handleBindFromHandle [handleNew] *$quartiles.data<1>]
   set nCol [exprGet $medianQreg.ncol]; set nRow [exprGet $medianQreg.nrow]
   assert { $nRow == [expr $endfield - $startfield + 1] }
   set regM [regNew -type FL32 1 $nCol]

   loop fld $startfield [expr $endfield+1] {
      set ifield [expr $fld - $startfield]
      set idR_format "idR-%06d-%s%d-%04d.fit $run $filter $camCol $fld" 
      set file [eval format $idR_format]
      set file $fieldsDir/$file  
      set Rreg [regReadAsFits [regNew] $file] 
      set medianQrow [subRegNew $medianQreg 1 $nCol $ifield 0]
      regMedianFindByColumn $Rreg $regM
      # set OmedQ [regMedianFindAsDbl $medianQrow]
      # set sky [regMedianFindAsDbl $regM]
      regMultiplyWithDbl $regM $tshift -regOut $medianQrow
      # set NmedQ [regMedianFindAsDbl $medianQrow]
      # echo field $fld: sky = $sky, OmedQ = $OmedQ NmedQ = $NmedQ    
      regDel $Rreg     
   }

   handleDel $medianQreg
   regDel $regM

   return 0
}


# compare medians found by vectorGetFromRegion (which uses phQuartilesGetFromArray)
# those found by regMedianFindByColumn (which uses atRegMedianFindByColumn)
# given a region return 2 vectors,
proc medCompare {reg} {

     set ncol [exprGet $reg.ncol]; set nrow [exprGet $reg.nrow]

     ### median using phQuartilesGetFromArray 
     set phMed [vectorGetFromRegion $reg 0 [expr $nrow-1] 0 [expr $ncol-1] 0 -median]

     ## median using regMedianFindByColumn
     set regM [regNew -type FL32 1 $ncol]
     regMedianFindByColumn $reg $regM
     # this simply gets vector out of 1xNcol region
     set atMed [vectorGetFromRegion $regM 0 0 0 [expr $ncol-1] 0]
     regDel $regM

    
    return "$phMed $atMed"

}

# given a region plot the medians in the row direction 
# n.b. if yminmax != "" then xminmax must be != ""
proc plotMed {reg {xminmax ""} {yminmax ""} {dev /XWINDOW} {title ""} } {

     set ncol [exprGet $reg.ncol]; set nrow [exprGet $reg.nrow]

     ### median using phQuartilesGetFromArray 
     set phMed [vectorGetFromRegion $reg 0 [expr $nrow-1] 0 [expr $ncol-1] 0 -median]
     set c [vIndex $ncol]

     if {$xminmax == ""} {
         plot_xy $c $phMed $dev $title line
     } elseif {$yminmax == ""} {
         plot_xy $c $phMed $dev $title line \
                     1 1 [lindex $xminmax 0] [lindex $xminmax 1]
     } else {
         plot_xy $c $phMed $dev $title line \
                     1 1 [lindex $xminmax 0] [lindex $xminmax 1] \
                         [lindex $yminmax 0] [lindex $yminmax 1]
     }

     vListDel "$c $phMed"

}


proc plotGainVariation {file {dev /XWINDOW}} {


global pg

    set field  [lindex [file2Vector $file 1] 1]
    set gainJ  [lindex [file2Vector $file 2] 1]
    vNameSet $field "field"
    vNameSet $gainJ "left-to-right gain correction / median"

    set median [vMedian $gainJ]
    set gainJmed [vectorExprEval $gainJ/$median]
    set mask [vectorExprEval "$field > 0 ? 1 : 0"]
    set fMin [vExtreme $field min -vMask $mask]
    set fMax [vExtreme $field max]

    set aux [format "%5.3f" $median] 
    set title "median=$aux, file=$file"
    plot_xy $field $gainJmed $dev $title 5 1 1 $fMin $fMax 0.99 1.01

    vListDel "$field $gainJ $gainJmed $mask"


}




# given a list of fits files, read them and return a list of regions
proc fileList2regList {fileList} {
 
     foreach file $fileList {
        lappend regList [regReadAsFits [regNew] $file]
     }

  return $regList

}


# for each region in regList, compute median and return a list of median
# vectors; use phQuartilesGetFromArray 
proc regList2medListPh {regList} {

     set reg [lindex $regList 0] 
     set ncolm1 [expr [exprGet $reg.ncol]-1] 
     set nrowm1 [expr [exprGet $reg.nrow]-1]

     foreach reg $regList {
        ### median using phQuartilesGetFromArray 
        lappend vecList [vectorGetFromRegion $reg 0 $nrowm1 0 $ncolm1 0 -median] 
     }

   return $vecList
    
}




# for each region in regList, compute median and return a list of median
# vectors; use atRegMedianFindByColumn
proc regList2medListAt {regList} {

     set reg [lindex $regList 0] 
     set ncol [exprGet $reg.ncol] 
     set ncolm1 [expr $ncol-1] 

     foreach reg $regList {

        ## median using regMedianFindByColumn
        set regM [regNew -type FL32 1 $ncol]
        regMedianFindByColumn $reg $regM
        # this simply gets vector out of 1xNcol region
        lappend vecList [vectorGetFromRegion $regM 0 0 0 $ncolm1 0]
        regDel $regM
     }

   return $vecList
    
}


  
# given a list of fits files, read them and measure medians by two different
# methods: phQuartilesGetFromArray and atRegMedianFindByColumn
proc fileList2medians {fileList} {
 
     timerStart
     set mem [open mem.trace w]    

     write_mem mem "reading regions"
     set regList [fileList2regList $fileList]

     echo method 1: phQuartilesGetFromArray
     write_mem mem "phQuartilesGetFromArray"
     set vecList1 [regList2medListPh $regList]
     write_mem mem "doing vectors" 
     set m1 0  
     foreach vec $vecList1 {
        set m1 [expr $m1+[vMedian $vec]]
     }  

     echo method 2: atRegMedianFindByColumn     
     write_mem mem "atRegMedianFindByColumn"
     set vecList2 [regList2medListAt $regList]
     write_mem mem "doing vectors" 
     set m2 0  
     foreach vec $vecList2 {
         set m2 [expr $m2+[vMedian $vec]]
     } 

      
     echo method 3: regMedianFindAsDbl
     write_mem mem "regMedianFindAsDbl"
     set m3 0
     foreach reg $regList {
         set m3 [expr $m3+[regMedianFindAsDbl $reg]]        
     }
     
     # cleaning
     write_mem mem "deleting vectors"
     vListDel $vecList1
     vListDel $vecList2
     write_mem mem "deleting regions"
     foreach reg $regList {
        regDel $reg
     }
 
     write_mem mem "done"
     close $mem

    return "$m1 $m2 $m3"

}



###############################################################################
# auc procs to study quartiles

# Given a scFang file, return quartiles for request filter
proc fang2quart {file filter {verbose 0}} {

      set hdu [get_hdu $file $filter quarts]
      set tblcol [handleNewFromType TBLCOL]
      fitsRead $tblcol $file -binary -hdu $hdu
      set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
      assert {$hfilter == $filter}
      assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "QFLAT"} 
      set quartiles [quartilesNew 1 [exprGet $tblcol.rowCnt]]

      if [catch {
	 tblcolToQuartiles $tblcol $quartiles 0} msg] {
	 error "problem with file $file $filter: $msg"
      }
      if $verbose {
	 echo "tshift = [hdrGetAsInt $tblcol.hdr TSHIFT]"
      }
      handleDelFromType $tblcol

    return $quartiles
}

proc fangList2quart {fileList filter} {

      set quartiles [quartilesNew [llength $fileList] 2128]
      set tblcol [handleNewFromType TBLCOL]

      loop i 0 [llength $fileList] {
         set file [lindex $fileList $i]
         set hdu [get_hdu $file $filter quarts]         
         fitsRead $tblcol $file -binary -hdu $hdu
         set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
         assert {$hfilter == $filter}
         assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "QFLAT"} 
         if [catch {
	    tblcolToQuartiles $tblcol $quartiles $i} msg] {
	    error "problem with file $file $filter: $msg"
         }
      }
      handleDelFromType $tblcol

    return $quartiles
}




# compare quartiles (n => 0=q25, 1=q50, 2=q75) 
proc compareQ {q1 q2 n {device /XWINDOW}} {

      set v1 [vectorExprNew 2128]
      set v2 [vectorExprNew 2128]
      set col [vIndex 2128]
      loop i 0 2128 {
         handleSet $v1.vec<$i> [expr [exprGet $q1.data<$n>->rows_s32<0><$i>]/64]
         handleSet $v2.vec<$i> [expr [exprGet $q2.data<$n>->rows_s32<0><$i>]/64]

      }
    

      # Set up the plot
      set pg [pgstateNew]
      pgstateSet $pg -device $device \
              -xfract 0.22 -yfract 0.35 -nxwindow 1 -nywindow 1 -isLine 1 -icLine 2

      pgstateOpen $pg

    
     
      vPlot $pg $col $v1 
      pgstateSet $pg -isNewplot 0 -icLine 4
      vPlot $pg $col $v2


      vListDel "$v1 $v2"

}


proc rawQ2shape {rawQfile idBfile normName shapeName} {

      # renormalization segments
      set cL1 42
      set cL2 1063
      set cR1 1064 
      set cR2 2085 

      # read input raw quartile region
      set rawQ [regReadAsFits [regNew] $rawQfile]
      set Nrow [exprGet $rawQ.nrow]
      set Ncol [exprGet $rawQ.ncol]

      # read bias vector
      set rawiBreg [regReadAsFits [regNew -type FL32] $idBfile] 
      # output region with renormalized quartiles
      set normQ [regNew -type FL32 $Nrow $Ncol]
      regClear $normQ
      # the shape vector (region)
      set shape [regNew -type FL32 1 $Ncol] 

      loop irow 0 $Nrow { 
          
          ## first copy overscan regions
          set rawRow [subRegNew $rawQ 1 40 $irow 0]
          set normRow [subRegNew $normQ 1 40 $irow 0]
          regMultiplyWithDbl $rawRow 1.0 -regOut $normRow    
          regDel $rawRow; regDel $normRow
          set rawRow [subRegNew $rawQ 1 40 $irow 2087]
          set normRow [subRegNew $normQ 1 40 $irow 2087]
          regMultiplyWithDbl $rawRow 1.0 -regOut $normRow    
          regDel $rawRow; regDel $normRow
    
          ## left half median
          set renormVec [vectorGetFromRegion $rawQ $irow $irow $cL1 $cL2 0]
          set medL [vMean $renormVec]; vectorExprDel $renormVec
          # right half median
          set renormVec [vectorGetFromRegion $rawQ $irow $irow $cR1 $cR2 0]
          set medR [vMean $renormVec]; vectorExprDel $renormVec
          # renormalize
          set rawRow [subRegNew $rawQ 1 1024 $irow 40]
          set normRow [subRegNew $normQ 1 1024 $irow 40]
          regMultiplyWithDbl $rawRow [expr 1.0/$medL] -regOut $normRow    
          regDel $rawRow; regDel $normRow
          set rawRow [subRegNew $rawQ 1 1024 $irow 1064]
          set normRow [subRegNew $normQ 1 1024 $irow 1064]
          regMultiplyWithDbl $rawRow [expr 1.0/$medR] -regOut $normRow    
          regDel $rawRow; regDel $normRow
          echo row $irow: medL: $medL medR: $medR
      }

   regMedianFindByColumn $normQ $shape

   regWriteAsFits $normQ $normName
   regWriteAsFits $shape $shapeName
   return "all OK"

   # return "rawQ: $rawQ normQ: $normQ shape: $shape"

}



# set rName $c/psRawQuartile50Region-001345-${f}$c.fit
# set nName $c/psNormQuartile50Region-001345-${f}$c.fit      
# set sName $c/psNormQuartile50ShapeRegion-001345-${f}$c.fit 
# foreach c {1 2 3 4 5 6} {foreach f {u g r i z} {rawQ2shape $rName $nName $sName}}




### given two runs (or two reruns for the same run), plot a four panel
### figure comparing the shapes of background counts, bias vectors,
### scattered light profiles, and flatfield vectors
###
# e.g.
# compareChips 1033 30 2659 30 u 4 "u4, 1033-30 vs 2659-30" /VCPS compare1033-2659-u4.ps 
proc compareChips {run1 rerun1 run2 rerun2 filter camCol {title ""} {dev /XWINDOW} \
	           {outPSname ""} {yRange 0.15} {dataRoot ""} } {

global data_root

     ## data root directory
     if {$dataRoot == ""} {
         set data_root [get_data_root ""]
     } else {
         set data_root $dataRoot
     }    

     ### get the data
     foreach epoch {1 2} {
	if {$epoch == 1} {
           set run $run1
           set rerun $rerun1
        } else {
           set run $run2
           set rerun $rerun2
        }
        ## file names
        set runstr [format %06d $run]
        set objcsDir $data_root/$run/$rerun/objcs/$camCol
        # raw quartiles (background) 
        set Qfile $objcsDir/psQuartileRegion-${runstr}-${filter}${camCol}.fit
        # better bias file
        set Bfile [lindex [glob $objcsDir/psBB*fit] 0]
        # scattered light 
        set SLfile $objcsDir/psSLprof-$runstr-${filter}${camCol}.fit
        # flatfield vector
        set FFfile $objcsDir/psFF-$runstr-${filter}${camCol}.fit
        ## read all regions
        set Qreg [regReadAsFits [regNew] $Qfile]
        set Breg [regReadAsFits [regNew] $Bfile]
        set SLreg [regReadAsFits [regNew] $SLfile]
        set FFreg [regReadAsFits [regNew] $FFfile]
        ## make proper vectors (normalized, and expressed in mags)
        # return "$Qreg $Breg $SLreg $FFreg"
        # raw quartiles
        set sQreg [subRegNew $Qreg [exprGet $Qreg.nrow] 2048 0 40]
        set Qmed($epoch) [medVecFromReg $sQreg]; regDel $sQreg; regDel $Qreg
        # bias 
        set sBreg [subRegNew $Breg [exprGet $Breg.nrow] 2048 0 40]
        set Bmed($epoch) [medVecFromReg $sBreg]; regDel $sBreg; regDel $Breg
        # scattered light 
        set SLmed($epoch) [medVecFromReg $SLreg]; regDel $SLreg
        # flatfield  
        set FFmed($epoch) [medVecFromReg $FFreg]; regDel $FFreg
        # return "$Qmed($epoch) $Bmed($epoch) $SLmed($epoch) $FFmed($epoch)" 
        ## for getting rid of odd/even effect
        vecReBin $Qmed($epoch) 2
        vecReBin $Bmed($epoch) 2
        vecReBin $SLmed($epoch) 2
        ## for getting rid of amplifier break
        vecFixJump $Qmed($epoch) 1024 50
        vecFixJump $Bmed($epoch) 1024 50
        vecFixJump $FFmed($epoch) 1024 50
     }

     ### we have all vectors, now plot
     ## setup the plot
     set pg [pgstateNew]
     set device [devicename $dev $outPSname] 
     pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
          -xfract 0.28 -yfract 0.28 -nxwindow 2 -nywindow 3 -symb 3
     set icLineDef [exprGet $pg.icLine]
     pgstateOpen $pg
 
     # x vector
     set pix [vIndex 2048]
   
     # and now plot
    
     ### panel 1: (1+SL)*FF 
     foreach epoch {1 2} {     
        set vec($epoch) [vectorExprEval 2.5*lg((1+$SLmed($epoch))*$FFmed($epoch))]
        # correct for median offset
        set median [vMean $vec($epoch)]
        set LNvec($epoch) [vectorExprEval $vec($epoch)-$median]
        vectorExprDel $vec($epoch)
     }
     pgstateSet $pg -icLine 4 -lineWidth 2
     vPlot $pg $pix $LNvec(1) -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange          
     pgstateSet $pg -isNewplot 0    
     pgstateSet $pg -icLine 2 -lineWidth 2
     vPlot $pg $pix $LNvec(2) -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange  
     ## needed for later
     set LNdiff [vectorExprEval $LNvec(1)-$LNvec(2)]   
     vListDel "$LNvec(1) $LNvec(2)"
     # to guide the eye...
     pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1
     plot_line $pg 0 0 2048 0 1 14
     plot_line $pg 2048 -1000 2048 1000 1 14
     plot_line $pg 0 -1000 0 1000 1 14
     xlabel "chip column (pixel)"
     ylabel "(1+SL)*FF (mag)" 
     titlePlot "$title" 20       
     pgstateSet $pg -isNewplot 1    

     ### panel 2: change of (1+SL)*FF and change of bias 
     set median [vMean $LNdiff]
     set LNdiffNorm [vectorExprEval $LNdiff-$median)]
     set biasDiff [vectorExprEval 2.5*lg($Bmed(1)/$Bmed(2))]
     set median [vMean $biasDiff]
     set biasDiffNorm [vectorExprEval $biasDiff-$median)]
     pgstateSet $pg -icLine 4 -lineWidth 2
     vPlot $pg $pix $LNdiffNorm -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange          
     # to guide the eye...
     pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1
     plot_line $pg 0 0 2048 0 1 14
     plot_line $pg 2048 -1000 2048 1000 1 14
     plot_line $pg 0 -1000 0 1000 1 14
     pgstateSet $pg -isNewplot 0    
     pgstateSet $pg -icLine 2 -lineWidth 2
     vPlot $pg $pix $biasDiffNorm -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange          
     vListDel "$LNdiffNorm $biasDiff $biasDiffNorm"
     xlabel "chip column (pixel)"
     ylabel "rel. var. (bkgd & bias)" 
     pgstateSet $pg -isNewplot 1    

     ### panel 3: (1+SL)1 vs. (1+SL)2
     foreach epoch {1 2} {     
        set vec($epoch) [vectorExprEval 2.5*lg((1+$SLmed($epoch)))]
        # find median and take log
        set median [vMean $vec($epoch)]
        set LNvec($epoch) [vectorExprEval $vec($epoch)-$median]
        vectorExprDel $vec($epoch)
     }
     pgstateSet $pg -icLine 4 -lineWidth 2
     vPlot $pg $pix $LNvec(1) -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange          
     pgstateSet $pg -isNewplot 0    
     pgstateSet $pg -icLine 2 -lineWidth 2
     vPlot $pg $pix $LNvec(2) -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange  
     vListDel "$LNvec(1) $LNvec(2)"
     # to guide the eye...
     pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1
     plot_line $pg 0 0 2048 0 1 14
     plot_line $pg 2048 -1000 2048 1000 1 14
     plot_line $pg 0 -1000 0 1000 1 14
     xlabel "chip column (pixel)"
     ylabel "(1+SL) (mag)" 
     pgstateSet $pg -isNewplot 1       
     
     ### panel 4: FF1 vs. FF2
     foreach epoch {1 2} {     
        set vec($epoch) [vectorExprEval 2.5*lg($FFmed($epoch))]
        set median [vMean $vec($epoch)]
        set LNvec($epoch) [vectorExprEval $vec($epoch)-$median]
        vectorExprDel $vec($epoch)
     }
     pgstateSet $pg -icLine 4 -lineWidth 2
     vPlot $pg $pix $LNvec(1) -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange          
     pgstateSet $pg -isNewplot 0    
     pgstateSet $pg -icLine 2 -lineWidth 2
     vPlot $pg $pix $LNvec(2) -xmin 100 -xmax 1950 -ymin -$yRange -ymax $yRange  
     vListDel "$LNvec(1) $LNvec(2)"
     # to guide the eye...
     pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1
     plot_line $pg 0 0 2048 0 1 14
     plot_line $pg 2048 -1000 2048 1000 1 14
     plot_line $pg 0 -1000 0 1000 1 14
     xlabel "chip column (pixel)"
     ylabel "flatfield (mag)" 
           
     # info
     plot_info "Top left: bias-subtracted background comparison (blue:1, red:2)" 0 -2.5 -0.5
     plot_info "Top right: change in background (blue) and bias (red)" 0 -2.5 -0.6
     plot_info "Bottom left: scattered light comparison (blue:1, red:2)" 0 -2.5 -0.7
     plot_info "Bottom right: flatfield comparison (blue:1, red:2)" 0 -2.5 -0.8

     
             
    # end plot
    endplot $pg

    # clean 
    set list $pix
    foreach epoch {1 2} {
	set list "$list $Qmed($epoch)"
	set list "$list $Bmed($epoch)"
	set list "$list $SLmed($epoch)"
	set list "$list $FFmed($epoch)"
    }
    vListDel $list

 return

}


proc medVecFromReg {reg} {

     set ncol [exprGet $reg.ncol]
     set regM [regNew -type FL32 1 $ncol]
     regMedianFindByColumn $reg $regM
     # this simply gets vector out of 1xNcol region
     set medVec [vectorGetFromRegion $regM 0 0 0 [expr $ncol-1] 0]
     regDel $regM

 return $medVec

} 


## renormalize vector such that its mean value for 
## elements nSplit-nBuffer is the same as for nSplit+nBuffer
proc vecFixJump {vec nSplit nBuffer} {

      ## first get the mean values
      set buffer [vectorExprNew $nBuffer]
      loop i [expr $nSplit-$nBuffer] $nSplit {
         handleSet $buffer.vec<[expr $i-$nSplit+$nBuffer]> [exprGet $vec.vec<$i>] 
      }
      set medianL [vMedian $buffer]
      loop i $nSplit [expr $nSplit+$nBuffer] {
         handleSet $buffer.vec<[expr $i-$nSplit]> [exprGet $vec.vec<$i>] 
      }
      set medianR [vMedian $buffer]
      vectorExprDel $buffer
      ## jump at nSplit
      set Jmedian [expr $medianR/$medianL] 
      set FFcorrectionL [expr 2.0*$Jmedian/(1+$Jmedian)]
      set FFcorrectionR [expr 2.0/(1+$Jmedian)]
      loop k 0 [exprGet $vec.dimen] {
          set val [exprGet $vec.vec<$k>]
	  if {$k < $nSplit} {
              handleSet $vec.vec<$k> [expr $FFcorrectionL*$val]
          } else {
              handleSet $vec.vec<$k> [expr $FFcorrectionR*$val]
          }
      }   

  return
}



# given a vector, replace each element by the mean value of
# N values in a window starting from its position
# e.g. for getting rid of odd/even effect
# vecReBin $MyVec 2
proc vecReBin {vec N} {

    set aux [vectorExprNew [exprGet $vec.dimen]]
    
    loop i 0 [exprGet $aux.dimen] {
       set sum 0 
       set Ngood 0
       loop j $i [expr $i+$N] {
	  if {$j < [exprGet $vec.dimen]} {
             set sum [expr $sum+[exprGet $vec.vec<$j>]]
             incr Ngood
          }
	  handleSet $aux.vec<$i> [expr $sum/$Ngood]
       }
    }

    loop i 0 [exprGet $aux.dimen] {
       handleSet $vec.vec<$i> [exprGet $aux.vec<$i>]
    }
    vectorExprDel $aux

  return
}



## wrapper for testing uberflats
# e.g.
# sdr; set uberflatsDir /u/dss/products/Linux/photoop/v0_6/flats/v3
# uberflatsTest $uberflatsDir
proc uberflatsTest {uberflatDir {idFFiter 2}} {

global data_root 

    set iterstr  [format "%03d" $idFFiter]

    # list of uberflats run boundaries (flats are good up to that run)   
    set uberlist "1 941 1659 1869 2121 2166 3311"
    ### what a mess, uberflats have different seasons, make translation
    set seasons(1)    "10"
    set seasons(941)  "20"
    set seasons(1659) "20"
    set seasons(1869) "30"
    set seasons(2121) "40"
    set seasons(2166) "50"
    set seasons(3311) "60 70 80 90"


    set retList "" 
    foreach r $uberlist {
       set runstr  [format "%06d" $r]
       ## loop over corresponding seasons  
       foreach s $seasons($r) {
          set sstr  [format "%03d" $s]
	  set pathRoot1 $data_root/idFFdir/iteration${idFFiter}/idFF-${iterstr}$sstr
          set pathRoot2 $uberflatDir/idFF-$runstr
	  set outputPSnameRoot Compare_iter${idFFiter}-s${s}_uberflats_$runstr
          set titleRoot "Iteration ${idFFiter} flats vs. uberflats, season $s vs. $runstr,"
	  set stats($r,$s) [idFFcompareAll $pathRoot1 $pathRoot2 $outputPSnameRoot $titleRoot]
	  set retList "$retList [list $r $s $stats($r,$s)]"
       }
    }

    foreach r $uberlist {
       foreach s $seasons($r) {
          echo $r $s: $stats($r,$s)
       }
    }    

  return $retList 

}


## little aux proc to call idFFcompar for all filters 
## e.g.
# set pathRoot1 $data_root/idFFdir/iteration2/idFF-002030
# set pathRoot2 /u/dss/products/Linux/photoop/v0_6/flats/v3/idFF-001659
# set outputPSnameRoot Compare_iter2-s30_uberflats_1659
# set titleRoot "Iteration 2 flats vs. uberflats, season 30 vs. 1659,"
# idFFcompareAll $pathRoot1 $pathRoot2 $outputPSnameRoot $titleRoot
proc idFFcompareAll {pathRoot1 pathRoot2 outputPSnameRoot titleRoot} {

    set line ""
    foreach f {u g r i z} {
       set outputPSname ${outputPSnameRoot}_$f.ps
       set title "$titleRoot filter $f" 
       set s [idFFcompare $pathRoot1 $pathRoot2 $f $title /VCPS $outputPSname]
       echo "$f   $s"
       set line "$line   $s"
    }

  return $line
} 


### compare two independent set of flats specified by path root 
### for a given band and for all camera columns)
### e.g.
# set filter u
# set pathRoot1 $data_root/idFFdir/iteration2/idFF-002030
# set pathRoot2 /u/dss/products/Linux/photoop/v0_6/flats/v3/idFF-001659
# set outputPSname Compare_iter2-s30_uberflats_1659_$filter.ps
# set title "Iteration 2 flats vs. uberflats, season 30 vs. 1659, filter $filter" 
# idFFcompare $pathRoot1 $pathRoot2 $filter $title /VCPS $outputPSname
proc idFFcompare {pathRoot1 pathRoot2 filter \
                      {title ""} {dev /XWINDOW} {outputPSname ""} } {

    set camCols "1 2 3 4 5 6"
    set dmMax 120 
    set dmMin [expr -1*$dmMax]

    # x vector
    set pix [vIndex 2048]
    
    ### setup the plot
    set pg [pgstateNew]
    set device [devicename $dev $outputPSname] 
    pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
                -xfract 0.27 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
    set icLineDef [exprGet $pg.icLine]
    pgstateOpen $pg


    set rmsSum 0.0
    set rmsMax 0.0
    ### read data for each camCol and plot 
    foreach c $camCols {
       # loop over the two sets being compared
       foreach it "1 2" {
	  if {$it == 1} {
              set pathRoot $pathRoot1
	  } else {
              set pathRoot $pathRoot2
          }
          set idFFfile ${pathRoot}-${filter}${c}.fit
          set idFFreg [regReadAsFits [regNew] $idFFfile]
          fixFF $idFFreg 
          # get vector out of this 1xNcol region
          set Ncol [exprGet $idFFreg.ncol]
          set aux [vectorGetFromRegion $idFFreg 0 0 0 [expr $Ncol-1] 0]
          # rescale it to ~1
          set tshift [hdrGetAsInt $idFFreg.hdr TSHIFT]
          set FF($it) [vectorExprEval 1.0*$aux/$tshift]
          vectorExprDel $aux
          regDel $idFFreg
          # express FF in milimags
          set aux [vectorExprEval 1000*2.5*lg($FF($it))]
          set mean [vMean $aux]
          set mFF($c,$it) [vectorExprEval $aux-$mean] 
          vectorExprDel $aux 
          vectorExprDel $FF($it)       
       }

       # we have two vectors, mFF($c,1) and mFF($c,2), expressed in milimags, 
       # get their difference as dFF($c)
       set aux [vectorExprEval $mFF($c,1)-$mFF($c,2)] 
       set mean [vMean $aux]
       set dFF($c) [vectorExprEval $aux-$mean]       
       vectorExprDel $aux          
     
       ### for each camCol we have vectors mFF($c,1), mFF($c,2) and dFF($c), now plot them
       pgstateSet $pg -icLine 2 -lineWidth 1 -lineStyle 1
       vPlot $pg $pix $mFF($c,1) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
       pgstateSet $pg -isNewplot 0
       pgstateSet $pg -icLine 4 -lineWidth 1 -lineStyle 1
       vPlot $pg $pix $mFF($c,2) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
       # difference as blue-red dashed line
       pgstateSet $pg -isNewplot 0
       pgstateSet $pg -icLine 1 -lineWidth 3 -lineStyle 1
       vPlot $pg $pix $dFF($c) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
       #pgstateSet $pg -isNewplot 0
       #pgstateSet $pg -icLine 7 -lineWidth 3 -lineStyle 2 
       #vPlot $pg $pix $dFF($c) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax

       if {1} {
         # to guide the eye...
         pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1 -icLine 15 -lineStyle 1
         plot_line $pg -1000 0 10000 0 1 4 
         plot_line $pg -1000 -20 10000 -20 1 4 
         plot_line $pg -1000 20 10000 20 1 4 
         plot_line $pg 0 -1000 0 1000 1 2
         plot_line $pg 2048 -1000 2048 1000 1 2
       }
       xlabel "chip column (pixel)"
       ylabel "dm (milimag)" 
       pgstateSet $pg -isNewplot 1

       # which camCol 
       pgText 1250 -100 "camCol $c"

       ### stats
       set sig [format "%4.1f" [vSigma $dFF($c)]]
       pgText 250 -100 "rms=$sig"
       set rmsSum [expr $rmsSum+$sig]
       if {$sig > $rmsMax} {set rmsMax $sig}
       if {$c == [lindex $camCols 0]} {titlePlot "$title" 20} 
       if {$c == [lindex $camCols 4]} {
          # info
          plot_info "The differences in flatfield vectors" 0 -1.0 -0.6
          plot_info "Blue: the first set, red: the second set" 0 -1.0 -0.8
          plot_info "Black: the difference (rms in milimags)" 0 -1.0 -1.0
       }    
  
       # clean
       vListDel "$mFF($c,1) $mFF($c,2) $dFF($c)"
    }

    vListDel $pix

    ## end plot
    endplot $pg   

    set ret1 [format "%4.1f" [expr $rmsSum/6.0]] 
    set ret2 [format "%4.1f" $rmsMax] 
    return "$ret1 $ret2" 
}


### compare two idFF iterations for a pair of seasons (in a given band 
### and for all camera columns)
### e.g.
# set idFFdir $data_root/idFFdir
# idFFiterCompare $idFFdir 10 70 1 2 u /VCPS $outputPSname
proc idFFiterCompare {idFFdir season1 season2 iter1 iter2 filter \
                      {title ""} {dev /XWINDOW} {outputPSname ""} } {

    set s(1) $season1
    set s(2) $season2
    set iter(1) $iter1
    set iter(2) $iter2
     
    set camCols "1 2 3 4 5 6"
    set dmMax 80 
    set dmMin [expr -1*$dmMax]

    # x vector
    set pix [vIndex 2048]
    
    ### setup the plot
    set pg [pgstateNew]
    set device [devicename $dev $outputPSname] 
    pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
                -xfract 0.27 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
    set icLineDef [exprGet $pg.icLine]
    pgstateOpen $pg


    ### read data for each camCol and plot 
    foreach c $camCols {
       # loop over the two iterations being compared
       foreach it "1 2" {
          set itstr [format %03d $it]
          # loop over the two seasons being differenced
          foreach season "1 2" {
             set sstr [format %03d $s($season)]
	     set idFFfile ${idFFdir}/iteration${it}/idFF-${itstr}${sstr}-${filter}${c}.fit
             set idFFreg [regReadAsFits [regNew] $idFFfile]
             fixFF $idFFreg
             # get vector out of this 1xNcol region
             set Ncol [exprGet $idFFreg.ncol]
             set aux [vectorGetFromRegion $idFFreg 0 0 0 [expr $Ncol-1] 0]
             # rescale it to ~1
             set tshift [hdrGetAsInt $idFFreg.hdr TSHIFT]
             set FF($season) [vectorExprEval 1.0*$aux/$tshift]
             vectorExprDel $aux
             regDel $idFFreg
	  }
          # we have two vectors, FF(1) and FF(2), get their difference in magnitudes
          # and store as dFF($c,$it)
          set aux [vectorExprEval 1000*2.5*lg($FF(1)/$FF(2))]
          set mean [vMean $aux]
          set dFF($c,$it) [vectorExprEval $aux-$mean]
          vectorExprDel $aux          
       }
       # we have two vectors, dFF($c,1) and dFF($c,2), get their difference 
       set ddFF($c) [vectorExprEval $dFF($c,1)-$dFF($c,2)]

       ### for each camCol we have vectors dFF($c,1), dFF($c,2) and ddFF($c), now plot them
       pgstateSet $pg -icLine 2 -lineWidth 1
       vPlot $pg $pix $dFF($c,1) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
       pgstateSet $pg -isNewplot 0
       pgstateSet $pg -icLine 3 -lineWidth 1 -lineStyle 2
       vPlot $pg $pix $dFF($c,2) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
       pgstateSet $pg -isNewplot 0
       pgstateSet $pg -icLine 4 -lineWidth 3 -lineStyle 1
       vPlot $pg $pix $ddFF($c) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
       if {1} {
         # to guide the eye...
         pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1 -icLine 15
         plot_line $pg -1000 0 10000 0 1 4 
         plot_line $pg -1000 -20 10000 -20 1 4 
         plot_line $pg -1000 20 10000 20 1 4 
         plot_line $pg 0 -1000 0 1000 1 2
         plot_line $pg 2048 -1000 2048 1000 1 2
       }
       xlabel "chip column (pixel)"
       ylabel "dm (milimag)" 
       pgstateSet $pg -isNewplot 1

       # which camCol 
       pgText 1000 -50 "camCol $c"

       if {$c == [lindex $camCols 0]} {titlePlot "$title" 20} 
       if {$c == [lindex $camCols 4]} {
          # info
          plot_info "The differences in flatfield vectors for seasons $season1 vs. $season2" 0 -1.0 -0.6
          plot_info "Red: idFF iteration $iter1, green: idFF iteration $iter2" 0 -1.0 -0.8
          plot_info "Blue: difference between iterations $iter1 and $iter2" 0 -1.0 -1.0
       }    
  
       # clean
       vListDel "$dFF($c,1) $dFF($c,2) $ddFF($c)"
    }


    vListDel $pix

    ## end plot
    endplot $pg   

  return 

}


#
# This must be after any procs for which you need help
#
set_ftclHelp phOutputs help_procs;	# ftclHelpDefine's on all listed procs




