#
# Procs to handle binned data
#
if {[info commands photoVersion] == ""} { # photo cannot handle FRAMEINFO,
   proc photoVersion {} { return "NOCVS" }; # so use dervish/astrotools
}

proc binData {args} {
   global table openit data_root

   set verbose 0
   set idB 0;				# process idB files?
   set idFF 0;				# process idFF files?
   set idR 0;				# process idR files?
   set planFiles 0;			# create plan files?
   set opts [list \
		 [list [info level 0] \
 "Given a run and range of fields, generate another run that looks as
 if it were taken with the camera binned NxN"] \
		 [list <newRun> INTEGER -1 newRun "New run number"] \
		 [list {[camCol0]} INTEGER 0 camCol0 \
		      "Starting camCol (default: 1)"] \
		 [list -oldRun STRING "" run "Run that is to be binned"] \
		 [list -field0 INTEGER 0 field0 "Starting field"] \
		 [list -field1 INTEGER 0 field1 "Ending field (default: field0)"] \
		 [list -camCol1 INTEGER 0 camCol1 \
 "Ending camCol: default 6 unless camCol0 is set, when camCol1"] \
		 [list -suffix STRING "-NxN" suffix \
		      "Append this suffix to infile to make binned filename"] \
		 [list -filters STRING "u g r i z" filters \
		      "Which filters to include"] \
		 [list -idB CONSTANT 1 idB "Create binned idB files too?"] \
		 [list -idFF CONSTANT 1 idFF "Create binned idFF files too?"] \
		 [list -idR CONSTANT 1 idR "Create binned idR files?"] \
		 [list -planFiles CONSTANT 1 planFiles "Create plan files?"] \
		 [list -binned INTEGER 2 binfac "How hard data was binned"] \
		 [list -nframe INTEGER 0 nframe \
		      "Include this many input fields in each output field"] \
		 [list -rerun INTEGER 0 newRerun "Rerun to create"] \
		 [list -orerun INTEGER 0 rerun "Rerun for old run"] \
	         [list -psPlan STRING "" planfile \
		      "psPlan file to use (default psPlan(-camCol)?.par)"] \
		 [list -compression STRING "Z" comp \
		      "Suffix for desired compression"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty?"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # And now arguments
   #
   if {$suffix == "-NxN"} {
      set suffix "-${binfac}x${binfac}"
   }

   if {$run == ""} {
      set run $newRun
   }
   if {$rerun == 0} {
      set rerun $newRerun
   }

   if {$camCol0 <= 0} {
      set camCol0 1
      if {$camCol1 <= 0} {
	 set camCol1 6
      }
   } else {
      if {$camCol1 <= 0} {
	 set camCol1 $camCol0
      }
   }

   if {$field1 <= 0} {
      set field1 [expr $field0 + $nframe - 1]
   }
   if {($field1 - $field0 + 1)%$nframe != 0} {
      error "Your field range must be a multiple of $nframe"
   }

   if {$comp != "" && ![regexp {^\.} $comp]} {
      set comp ".$comp"
   }
   #
   # Deal with plan file
   #
   global psplan
   if [info exists psplan] { unset psplan }

   global cmdlinePlan
   array set cmdlinePlan [list camCol $camCol0]

   set defplanfile "[envscan \$PHOTO_DIR]/etc/psPlan.par"

   if {$planfile == ""} {
      set dir "$data_root/$run/$rerun"
      
      if {$rerun == -1} {
	 if ![catch { set dirs [glob $root/$run/\[0-9\]*\[0-9\]] }] {
	    set dir [lindex $dirs 0]
	 }
      }

      set planfile $dir/photo/psPlan
      if {![file exists $planfile.par] &&
	  [file exists $planfile-$camCol.par]} {
	 append planfile "-$camCol"
      }
      append planfile ".par"
   } else {
      if ![regexp {^/} $planfile] {
	 set planfile $root/$run/$rerun/photo/$planfile
      }
   }

   set cmdline cmdlinePlan
   foreach file "defplanfile planfile cmdline" {
      verb_echo 3 "   processing input file [set $file]"
      process_plan psplan [read_planfile [set $file]]
   }
   #
   # Remove `default' values, and set the plan as variables in this scope
   # (or global, if so declared above)
   #
   foreach var [keylget psplan] {
      set val [keylget psplan $var]
      if {$val == "default"} {
	 keyldel psplan $var
      } elseif {$val == "nodefault"} {
	 error "** keyword $var has value nodefault! **"
      } else {
	 if [regexp {^(display.*|.*_format)$} $var] {
	    global $var
	 }

	 set $var $val
      }
   }
   set camCol [keylget psplan camCol]
   #
   # Create output directories and plan files.
   #
   if $planFiles {
      createPlanFiles $newRun $newRerun -dir -canon \
	  -startField $field0 -endField $field1 -filter $filters
   }
   #
   # We know what we should do; do it
   #
   if $idR {
      global idR_format
      if ![info exists idR_format] {
	 set idR_format \
	     {idR-%06d-%s%d-%04d.fit  $run $filter $camCol $field}
      }
      
      loop camCol $camCol0 [expr $camCol1 + 1] {
	 set_run -rerun $rerun $run $camCol
	 
	 set indir $openit(fielddir)
	 set outdir $data_root/$newRun/$newRerun/fields/$camCol
	 foreach f $filters {
	    set field_ctr 0; set ofield $field0
	    loop field $field0 [expr $field1 + 1] {
	       if $verbose {
		  puts -nonewline [format "%d %-3d %s\r" $camCol $field $f]
		  flush stdout
	       }
	       
	       set infile [get_raw_filename $indir $run $f $camCol $field]
	       set idR [regReadAsFits [regNew] $infile]
	       
	       set outfile \
		   [get_raw_filename $outdir $newRun $f $camCol $ofield]$comp
	       regsub {(\.fits?)} $outfile "$suffix\\1" outfile
	       
	       if {$nframe == 1} {
		  set breg [regIntBin -slow NULL $idR $binfac $binfac]
		  hdrCopy $idR.hdr $breg.hdr
		  regWriteAsFits *$breg.reg $outfile
		  binregionDel $breg
		  incr ofield
	       } else {
		  if {$field_ctr == 0} {
		     set nrow [exprGet $idR.nrow]
		     set ncol [exprGet $idR.ncol]
		     set oreg [regNew [expr $nframe*$nrow] $ncol]
		     hdrCopy $idR.hdr $oreg.hdr
		  }
		  
		  set sreg [subRegNew $oreg $nrow $ncol [expr $field_ctr*$nrow] 0]
		  regIntCopy $sreg $idR
		  regDel $sreg
		  
		  incr field_ctr
		  #
		  # Time to write a superField?
		  #
		  if {$field_ctr == $nframe} {
		     set field_ctr 0
		     
		     set breg [regIntBin -slow NULL $oreg $binfac $binfac]
		     regWriteAsFits *$breg.reg $outfile
		     binregionDel $breg
		     
		     regDel $oreg; unset oreg
		     incr ofield
		  }
	       }
	       #
	       # Clean up this input field
	       #
	       regDel $idR
	    }
	 }
      }
      
      if $verbose {
	 echo ""
      }
      
      if [info exists oreg] {
	 regDel $oreg
      }
   }
   #
   # idB files too?
   #
   if $idB {
      global idB_format
      if ![info exists idB_format] {
	 set idB_format {idB-%06d-%s%d.fit  $run $filter $camCol}
      }
      
      set_run -rerun $rerun $run $camCol0
      set dir $openit(photo)/calib
      set odir $data_root/$newRun/$newRerun/photo/calib

      loop camCol $camCol0 [expr $camCol1 + 1] {
	 foreach f $filters {
	    set infile [get_bias_filename $dir $run $f $camCol]
	    set outfile [get_bias_filename $odir $newRun $f $camCol]
	    regsub {(\.fits?)} $outfile "$suffix\\1" outfile

	    set reg [regReadAsFits [regNew] $infile]
	    set breg [regIntBin -slow NULL $reg 1 $binfac]
	    hdrCopy $reg.hdr $breg.reg->hdr

	    regWriteAsFits *$breg.reg $outfile

	    binregionDel $breg
	    regDel $reg	    
	 }
      }
   }
   #
   # Flat fields too?
   #
   if $idFF {
      global idFF_format
      if ![info exists idFF_format] {
	 set idFF_format {idFF-%s-%s%d.fit   $iterSeason $filter $camCol}
      }
      set iter 001090

      set_run $run $camCol0

      eval set dir $trueflatDir
      set odir $dir-binned$binfac
      if ![file exists $odir] {
	 mkdir -path $odir
      }

      loop camCol $camCol0 [expr $camCol1 + 1] {
	 foreach f $filters {
	    set infile [get_idFF_filename   $dir $iter $f $camCol]
	    set outfile [get_idFF_filename $odir $iter $f $camCol]

	    set reg [regReadAsFits [regNew] $infile]
	    set breg [regIntBin -slow NULL $reg 1 $binfac]
	    hdrCopy $reg.hdr $breg.reg->hdr

	    regWriteAsFits *$breg.reg $outfile

	    binregionDel $breg
	    regDel $reg	    
	 }
      }
   }
}

###############################################################################
#
# Bin an opFile (or idFramelog)
#
proc binOpFile {args} {
   set verbose 0

   set opts [list \
		 [list [info level 0] \
		      "Produce a binned opFile (e.g. \"opBC\", or \"all\")"] \
		 [list <infile> STRING "" infile "File to be binned"] \
		 [list -inputdir STRING "." inputdir \
		      "Directory with unbinned files"] \
		 [list -outputdir STRING "" outputdir \
		      "Directory to write binned files (default: inputdir)"] \
		 [list {[suffix]} STRING "-NxN" suffix \
		      "Append this suffix to infile to make binned filename"] \
		 [list -binned INTEGER 2 binfac "How hard data was binned"] \
		 [list -nframe INTEGER 2 nframe \
		      "Include this many input fields in each output field"] \
		 [list -scanrate DOUBLE 1 scanrate  \
		      "The data were taken scanrate faster than sidereal"] \
		 [list -frame0 INTEGER -1 frame0 \
		      "Starting frame when processing idFramelogs"] \
		 [list -newRun INTEGER -1 newRun "New run number"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 [list -scaled DOUBLE 1 scaled "Data were scaled by this factor"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$outputdir == ""} {
      set outputdir $inputdir
   }

   if {$infile == "all" || $infile == "All"} {
      if {$inputdir == ""} {
	 set opfiles [glob op*.par]
      } else {
	 foreach file [glob $inputdir/op*.par] {
	    lappend opfiles [file tail $file]
	 }
      }
      foreach file $opfiles {
	 if {[regexp {^opRunlist} $file] || [regexp {[0-9]x[0-9]} $file]} {
	    if $verbose {
	       echo Skipping $file
	    }
	    continue
	 }
	 
	 if $verbose {
	    echo Processing $file
	 }
	 binOpFile -inputdir $inputdir \
	     -nframe $nframe -bin $binfac -scan $scanrate -scaled $scaled $file
      }

      return
   }

   set known_type 0
   foreach t "opBC opCamera opConfig opECalib idFrameLog" {
      set $t [regexp "^$t" $infile]
      incr known_type [set $t]
   }

   if !$known_type {
      error "$infile is not of a supported type"
   }
   if $idFrameLog {
      if {$newRun < 0} {
	 error "You must specify a new run number to bin a idFrameLog file"
      }
      #
      # Frames has its own C struct called FRAMEINFO; redefine it so we
      # can read the idFrameLog file
      #
      typedef struct {
	 int frame;
	 double mjd;
	 double ra;
	 double dec;
	 float spa;
	 float ipa;
	 float ipaRate;
	 double az;
	 double alt;
      } FRAMEINFO;
   }
   #
   # Do the work
   #
   if ![file exists $inputdir/$infile] {
      error "I cannot find $inputdir/$infile"
   }

   set chainList [param2Chain $inputdir/$infile hdr]

   if $idFrameLog {
      if {$suffix == "-NxN"} {
	 set suffix ""
      }
      set outfile [file root $infile]$suffix.par
      regsub -- {-[0-9][0-9][0-9][0-9][0-9][0-9]-} $outfile \
	  [format -%06d- $newRun] outfile
   } else {
      if {$suffix == "-NxN"} {
	 set suffix "-${binfac}x${binfac}"
      }
      
      set outfile [file root $infile]$suffix.par
   }

   set outfile "$outputdir/$outfile"

   set fd [open $outfile "w"]
   puts $fd "\#"
   puts $fd "\# Created from $infile by binning ${binfac}x${binfac}; nframe = $nframe"
   puts $fd "\# scanrate = $scanrate * Sidereal"
   if {$scaled != 1} {
      puts $fd "\# Data were scaled by $scaled"
   }
   puts $fd "\# using [photoVersion] at [fmtclock [getclock]]"
   puts $fd "\#"
   close $fd

   if $idFrameLog {
      keylset hdr run $newRun
   }
   
   foreach ichain $chainList {
      set type  [exprGet -enum $ichain.type]

      set ochain [chainNew $type]

      loop i 0 [chainSize $ichain] {
	 set el [chainElementRemByPos $ichain 0]
	 
	 if {$type == "CCDBC"} {
	    foreach f "dfcol0 dfrow0" {
	       set val [expr [exprGet $el.$f]/$binfac]
	       handleSet $el.$f $val
	    }
	    foreach f "dfncol dfnrow" {
	       set val [expr ([exprGet $el.$f] + $binfac - 1)/$binfac]
	       handleSet $el.$f $val
	    }
	    #
	    # 
	    #
	    # Make sure that each binned defect only appears in the file once
	    #
	    if [check_opBC $el keys] {	# already on the chain
	       genericDel $el
	       continue;
	    }
	 } elseif {$type == "CCDCONFIG"} {
	    handleSet $el.rowBinning $binfac
	    handleSet $el.colBinning $binfac

	    handleSet $el.nrows \
		[expr ([exprGet $el.nrows]*$nframe + $binfac - 1)/$binfac]
	    handleSet $el.ncols [expr [exprGet $el.ncols]/$binfac]

	    foreach f [list nDataRow] {
	       loop i 0 4 {
		  set val [expr [exprGet $el.${f}$i]*$nframe/$binfac]
		  handleSet $el.${f}$i $val

		  set val [expr [exprGet $el.${f}${i}good]*$nframe/$binfac]
		  handleSet $el.${f}${i}good $val
	       }
	    }

	    loop i 0 4 {
	       foreach f [list sPreBias nPreBias sPostBias nPostBias \
			      sOverScan nOverScan sMapOverScan nMapOverScan \
			      sOverScanRows nOverScanRows sDataSec nDataSec \
			      sDataRow sCCDRowSec sCCDColSec] {
		  set val [expr [exprGet $el.${f}$i]/$binfac]
		  handleSet $el.${f}$i $val
		  
		  set val [expr [exprGet $el.${f}${i}good]/$binfac]
		  handleSet $el.${f}${i}good $val
	       }
	    }
	 } elseif {$type == "CCDGEOMETRY"} {
	    #
	    # JAM tells me that the opCamera file is always unbinned
	    # (sdss-astrom msg 494), so only frameOffset and rowOffset
	    # should be modified
	    #
	    # It's pretty clear that this is the right thing to do for the
	    # photo chips; I'm not so sure for the astro but as we don't run
	    # them binned it's moot.
	    #
	    handleSet $el.rowOffset [expr [exprGet $el.rowOffset]/$binfac]
	    handleSet $el.frameOffset [expr [exprGet $el.frameOffset]/$nframe]
	    handleSet $el.xmag \
		[expr [exprGet $el.xmag] - 2.5*log10($scaled/$scanrate)]

	 } elseif {$type == "DEWARGEOMETRY"} {
	    ;				# nothing to do
	 } elseif {$type == "ECALIB"} {
	    if {$scaled != 1} {
	       loop i 0 4 {
		  foreach f [list gain] {
		     set val [expr [exprGet $el.${f}$i]/$scaled]
		     handleSet $el.${f}$i $val
		  }
	       }
	    }
	 } elseif {$type == "FRAMEINFO"} {
	    if {$frame0 < 0} {
	       set frame0 [exprGet $el.frame]
	    }
	    
	    set frame [exprGet $el.frame]
	    if {$frame >= $frame0 && ($frame - $frame0)%$nframe == 0} {
	       handleSet $el.frame [expr $frame0 + ($frame - $frame0)/$nframe]
	    } else {
	       genericDel $el
	       continue
	    }
	 } else {
	    error "Unsupported type: $type"
	 }
	 
	 chainElementAddByPos $ochain $el
      }
      
      chain2Param $outfile $ochain $hdr -append
      
      chainDel $ichain
      chainDestroy $ochain genericDel
   }

   return $outfile
}

#
# Check if an opBC chain element is unique
#
proc check_opBC {el _keys} {
   upvar $_keys keys
   set key K
   foreach f "program camRow camCol dfcol0 dfncol dfrow0 dfnrow" {
      lappend key [exprGet $el.$f]
   }
   set key [join $key ":"]
   if [info exists keys($key)] {	# already on the chain
      return 1
   }

   set keys($key) 1
   
   return 0
}

###############################################################################
#
# Fix problems in idR files due to binning
#
proc fixBinnedFrame {args} {
   set copy 0;				# Modify and return a copy of reg?
   
   set opts [list \
		 [list [info level 0] \
		      "Add/remove columns at a CCDs amplifier split"] \
		 [list <reg> STRING "" reg "REGION to fix"] \
		 [list -bin INTEGER 2 bin "Factor data's binned by"] \
		 [list -copy CONSTANT 1 copy "Modify a copy of <reg>"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set nrow [exprGet $reg.nrow]
   set ncol [exprGet $reg.ncol]
   if $copy {
      set nreg [regNew $nrow $ncol]
      regIntCopy $nreg $reg
      set reg $nreg
   }

   if {$bin == 1} {
      return $reg;			# nothing to do
   } elseif {$bin == 2} {
      set npix 1;			# number of binned pixels to insert
      #
      # OK, we need to bias subtract and average the first and last
      # column in the data, and insert it in the middle of the chip
      # with the correct bias for the right-hand-side of the chip.
      #
      # Actually, there's no need to fix up the bias, but it makes
      # the idR files prettier
      #
      set nover 10;			# number of overclock columns
      
      set vec [vectorGetFromRegion $reg 0 -1 [expr -$nover/$bin] -1 \
		   0 -med -nbin 4]
      set rbias [vectorExprGet $vec<[expr ($nover/$bin - 1) - 1]>]
      vectorExprDel $vec
      
      set dbias [expr -$rbias];		# correction for double bias

      set col0 [regNew $nrow $npix];	# first column
      set sreg [subRegNew $reg $nrow $npix 0 0]
      regIntCopy $col0 $sreg
      regDel $sreg

      set colN [regNew $nrow $npix];	# last column
      set sreg [subRegNew $reg $nrow $npix 0 [expr $ncol - $npix]]
      regIntCopy $colN $sreg
      regDel $sreg

      regIntLincom $col0 $colN $dbias 1 1;# Desired central pixel
      
      regDel $colN
      #
      # save block of columns that we're going to move
      #
      set rhs [regNew $nrow [expr $ncol/2 - $npix]]
      set sreg \
	  [subRegNew $reg $nrow [expr $ncol/2 - $npix] 0 [expr $ncol/2]]
      regIntCopy $rhs $sreg
      regDel $sreg
      #
      # Copy that saved block into correct place
      #
      set sreg \
	  [subRegNew $reg $nrow [expr $ncol/2 -$npix] 0 [expr $ncol/2 + $npix]]
      regIntCopy $sreg $rhs
      regDel $sreg
      #
      # Move sum of first and last columns to the centre
      #
      set sreg [subRegNew $reg $nrow $npix 0 [expr $ncol/2]]
      regIntCopy $sreg $col0
      regDel $sreg
      
      regDel $col0

      regDel $rhs
   } else {
      error "I cannot handle bin = $bin"
   }

   return $reg
}

proc fixBinnedFrameRun {args} {
   global openit
   set verbose 0

   set opts [list \
		 [list [info level 0] \
 "Add/remove columns at a CCDs amplifier split for some filters in
 all fields in a run/camCol"] \
		 [list <run> INTEGER 0 run "The run in question"] \
		 [list -rerun INTEGER -1 rerun "The desired rerun"] \
		 [list <camCol> INTEGER 0 camCol "Desired camCol"] \
		 [list <filters> STRING "" filters \
 "Which filters to fix; if \"\" or \"default\", use hard-coded table"] \
		 [list <startField> INTEGER 0 startField \
		    "Desired first (or only) field to process"] \
		 [list {[endField]} INTEGER 0 endField \
		      "Desired last field to process (default: <startField>)"]\
		 [list -bin INTEGER 2 bin "Factor data's binned by"] \
		 [list -suffix STRING "-NxN" suffix \
		      "Append this suffix to infile to make binned filename"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   if {$suffix == "-NxN"} {
      set suffix "-${bin}x${bin}"
   }

   set 2ampChips [list "" \
		      "u     i z" \
		      "u g r i z" \
		      "u g   i z" \
		      "  g r i z" \
		      "u g r   z" \
		      "  g r i z" \
		     ]
   if {$filters == "" || $filters == "default"} {
      set filters [lindex $2ampChips $camCol]
   }

   if {$endField == 0} {
      set endField $startField
   }
   
   set_run $run -rerun $rerun $camCol

   loop field $startField [expr $endField + 1] {
      foreach f $filters {
	 set file [format idR-$openit(run)-${f}$camCol-%04d.fit $field]

	 if $verbose {
	    puts -nonewline "$file\r"; flush stdout
	 }

	 if [catch {set files [glob $openit(fielddir)/$file*]} msg] {
	    error "I cannot find any files like $file in $openit(fielddir)"
	 }
	 if {[llength $files] != 1} {
	    error "More then one possible input file: $files"
	 }

	 set file [lindex $files 0]
	 regsub {(\.fits?)} $file "$suffix\\1" outfile

	 set reg [regReadAsFits [regNew] $file]

	 fixBinnedFrame $reg -bin $bin

	 regWriteAsFits $reg $outfile
	 regDel $reg
      }
   }
   if $verbose {
      echo ""
   }
}
