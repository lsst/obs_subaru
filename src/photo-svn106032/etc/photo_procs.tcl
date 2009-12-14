#
# Support procs for the postage stamp and frames pipelines
#
# Echo a message if $verbose is large enough
#
# Guidelines for verbose levels (higher levels include lower ones):
#   0 - no messages
#   1 - run, camcol, filter info, plus indicate when running modules
#   2 - print timing information
#   3 - simple diognostics for each frame and filter
#   
proc verb_echo {level args} {
   global verbose diagnostics logfile; 

   set string [join $args]

   if { [info exists verbose] && $verbose >= $level } { 
      set indent [format %${level}s " "]
      echo $indent$string
      flush stdout
   }

   if { [info exists logfile] && $logfile != "" &&
	[info exists diagnostics] && $diagnostics >= $level } {
      set indent [format %${level}s " "]
      if [catch {
	 puts $logfile "$indent$string"
	 flush $logfile
      }] {				# try again after tickling NFS
	 puts $logfile "$indent$string"
	 flush $logfile
      }
   }
}

#
# Determine the type of REGION in use
#
# Change default region type
#

if {[info commands regNew.vanilla] == ""} {
   rename regNew regNew.vanilla

   proc photoRegNew {args} {
      if {[lsearch -exact $args "-type"] < 0} {
	 set sch [schemaGetFromType PHOTO_INFO]
	 regexp {[^*]*} [keylget sch pixtype] type

	 append args " -type $type"
      }
      eval regNew.vanilla $args
   }

   alias regNew photoRegNew
}

if ![info exists ROWS] {
   set reg [regNew]
   set TYPE_PIX [exprGet (int)$reg.type]
   set TYPE_PIX_NAME [string range [exprGet -enum $reg.type] 5 end]
   regDel $reg; unset reg

   set ROWS rows_[string tolower $TYPE_PIX_NAME]
}


# this procedure a) deletes every item on a chain
#                b) deletes the chain structure itself
# the args are handle to chain in question, and TCL verb to delete
#   an instance of the item on the chain
#
proc chainDestroy { chain delproc }  {
   set crsr [chainCursorNew $chain]
   while {[set el [chainWalk $chain $crsr]] != ""}  {
      $delproc $el
   }
   chainCursorDel $chain $crsr
   chainDel $chain
}

#
# The equivalent of shAssert in C. Use as:
#    assert {[exprGet (int)$tapeimages.type]==$TYPE_U16}
# Note the {} to preserve the variables so that we can print the assertion
#
proc assert {expr} {
   set pass [uplevel expr \{$expr\}]
   if {!$pass} {
      if {[info level] > 1} {
	 set proc "proc [lindex [info level -1] 0]"
      } else {
	 set proc "global"
      }
      
      error \
	  "Assertion fails ($proc): [uplevel join \"$expr\"] ($expr)"
   }
}

######################################################################
# Get the hardware parameters for a particular ccd. This is used by both
# the ps and frames_pipelines
#
proc get_ccdpars_for_filter {configlist econfiglist defectlist filter camCol \
				 planlist } {
   global N_LINEAR_COEFFS

   set camRow [keylget planlist ccdrow_$filter]
   if ![getsoftpar linearity_correction linearity_correction] {
      set linearity_correction 1
   }

   # Get the opConfig info for this ccd
   if {$configlist != ""} {
      set ccd [find_this_ccd $camRow $camCol $configlist]
   }
   # get the electronic info for this ccd
   if {$econfiglist != ""} {
      set eccd [find_this_ccd $camRow $camCol $econfiglist]
   }
   
   set ccdpars [ccdparsNew]
   
   # Copy the relevent information into the ccdpars.

   #    First find out which amps are being used. There is no guarantee that
   #    the first two amps are used (or the first, in the one amp case. 
   #    Let's call the first amp used '0' and the second '1'. In the two
   #    amp case, these should correspond to right  and left amps, but
   #    in the 1 amp case it wil be called '0' whether it's right or left.
   set ampcount 0
   loop i 0 3 {
      if {[exprGet $ccd.amp$i]} {
	 if {$ampcount >= 2} {
	    error 'Too many CCD amplifiers being used'
	 }
	 if {$ampcount == 0} {
	    handleSet $ccdpars.amp0 $i
	    set ampno($ampcount) $i
	 } else {
	    handleSet $ccdpars.amp$ampcount $i
	    set ampno($ampcount) $i
	 }
	 incr ampcount
      }
   }

   handleSet $ccdpars.namps $ampcount
   handleSet $ccdpars.iRow $camRow
   handleSet $ccdpars.iCol $camCol
   handleSet $ccdpars.nrows [exprGet $ccd.nrows]
   handleSet $ccdpars.ncols [exprGet $ccd.ncols]
   if 0 {				# we don't have a CCDGEOMETRY from
      ;					# the opCamera. This could be fixed
      handleSet $ccdpars.frameOffset [exprGet $ccdgeom.frameOffset]
   }
   handleSet $ccdpars.rowBinning [exprGet $ccd.rowBinning]
   handleSet $ccdpars.colBinning [exprGet $ccd.colBinning]
   assert {[exprGet $ccd.rowBinning] == [exprGet $ccd.colBinning]}

   set nDataCol 0
   loop i 0 $ampcount {
      set nDataRow [exprGet $ccd.nDataRow$ampno($i)]
      incr nDataCol [exprGet $ccd.nDataSec$ampno($i)]
      handleSet $ccdpars.sPreBias$i [exprGet $ccd.sPreBias$ampno($i)]
      handleSet $ccdpars.nPreBias$i [exprGet $ccd.nPreBias$ampno($i)]
      handleSet $ccdpars.sPostBias$i [exprGet $ccd.sPostBias$ampno($i)]
      handleSet $ccdpars.nPostBias$i [exprGet $ccd.nPostBias$ampno($i)]
      handleSet $ccdpars.sPrescan$i [exprGet $ccd.sOverScan$ampno($i)]
      handleSet $ccdpars.nPrescan$i [exprGet $ccd.nOverScan$ampno($i)]
      handleSet $ccdpars.sPostscan$i [exprGet $ccd.sMapOverScan$ampno($i)]
      handleSet $ccdpars.nPostscan$i [exprGet $ccd.nMapOverScan$ampno($i)]
      handleSet $ccdpars.sData$i [exprGet $ccd.sDataSec$ampno($i)] 
      handleSet $ccdpars.nData$i [exprGet $ccd.nDataSec$ampno($i)]

      if [info exists eccd] {
	 handleSet $ccdpars.gain$i [exprGet $eccd.gain$ampno($i)]
	 
	 if {[lsearch [join [schemaGetFromType ECALIB]] readNoiseDN$ampno($i)]
	     >= 0} {
	    handleSet $ccdpars.fullWell$i [exprGet $eccd.fullWellDN$ampno($i)]
	    handleSet $ccdpars.readNoise$i [exprGet $eccd.readNoiseDN$ampno($i)]
	 } else {
	    global warned_opECalib
	    if ![info exists warned_opECalib] {# XXX old opECalib file
	       set warned_opECalib 1;
	       echo "Warning: opECalib file is old (readnoise/fullwell in e-)"
	    }
	    
	    handleSet $ccdpars.readNoise$i \
		[expr [exprGet $eccd.readNoise$ampno($i)]/[exprGet $ccdpars.gain$i]]
	    handleSet $ccdpars.fullWell$i \
		[expr [exprGet $eccd.fullWell$ampno($i)]/[exprGet $ccdpars.gain$i]]
	 }
      }
   }
   handleSet $ccdpars.nDataCol $nDataCol
   handleSet $ccdpars.nDataRow $nDataRow
   #
   # Now for linearity corrections.  If the inputs are the old-style tables,
   # we'll ignore them
   #
   if [info exists eccd] {
      loop i 0 $ampcount {
	 if {!$linearity_correction || [exprGet $eccd.DN$ampno($i)<0>] > 0} {
	    verb_echo 0 \
		"Ignoring CCD linearity table for CCD $camRow,$camCol amp $i"
	    handleSet $ccdpars.n_linear_coeffs$i 0
	 } else {
	    global LINEARITY_TYPE
	    if ![info exists LINEARITY_TYPE] {
	       set_array_from_define LINEARITY_TYPE
	    }
	    
	    set type [expr abs([exprGet $eccd.DN$ampno($i)<0>])]
	    set nl [expr abs([exprGet $eccd.DN$ampno($i)<1>])]
	    assert {$nl <= $N_LINEAR_COEFFS}
	    assert {$type > 0 && ($type == $LINEARITY_TYPE(LINEAR_ILLEGAL) ||
				  $type < $LINEARITY_TYPE(LINEAR_NTYPE))}
	    
	    if {$i == 0} {
	       handleSet $ccdpars.linearity_type $type
	    } else {
	       assert {$type == [exprGet (int)$ccdpars.linearity_type]}
	    }
	    handleSet $ccdpars.n_linear_coeffs$i $nl
	    loop j 0 $nl {
	       handleSet $ccdpars.linear_coeffs$i<$j> \
		   [exprGet $eccd.linearity$i<$j>]
	    }
	    
	    switch $type [list \
			   $LINEARITY_TYPE(LINEAR_NONE) { \
			      assert {$nl == 0} \
			   } \
			   $LINEARITY_TYPE(LINEAR_QUADRATIC) { \
			      assert {$nl == 1} \
			   } \
			   $LINEARITY_TYPE(LINEAR_LOG) { \
			      assert {$nl == 1} \
			   } \
			   $LINEARITY_TYPE(LINEAR_THRESHLOG) { \
			      assert {$nl == 2} \
			   } \
			   ]
	 }
      }
   }

   # Make sure that datasec for 2-amp case are contiguous
   if {$ampcount > 1} {
      assert {[expr [exprGet $ccdpars.sData0] + [exprGet $ccdpars.nData0]] == \
		   [exprGet $ccdpars.sData1]}
   }

   # get the defects for this ccd
   if {$defectlist != ""} {
      set defects [find_defects $defectlist $camRow $camCol]
      handleSetFromHandle $ccdpars.CCDDefect &$defects
      handleDel $defects
   }

   return $ccdpars
}

# Get ccdpars for all filters in filterlist
proc fetch_ccdpars {configDir hardfile efile bcfile ccdpars filterlist camcol planlist} {
   upvar $ccdpars ccd

   global hardpars

   foreach file "hardfile efile bcfile" {
      if {[set $file] != "" && ![regexp {^/} [set $file]]} {
	 set $file $configDir/[set $file]
      }

      if ![file exists [set $file]] {
	 error "No such file: [set $file]"
      }
   }
   # we can't read into our own struct any more so we have to copy stuff

   if ![file exists $hardfile] {
      error "I cannot find $hardfile"
   }
   if {$hardfile == "/dev/null"} {
      set configlist [list ]
   } else {
      set configlist [param2Chain $hardfile hardpars]
      calc_crc $hardfile
   }

   if ![file exists $bcfile] {
      error "I cannot find $bcfile"
   }
   if {$bcfile == "/dev/null"} {
      set bclist [list ]
   } else {
      set bclist [param2Chain $bcfile  bcpars]
      calc_crc $bcfile
   }
   
   if ![file exists $efile] {
      error "I cannot find $efile"
   }
   if {$efile == "/dev/null"} {
      set econfiglist [list ]
   } else {
      set econfiglist [param2Chain $efile epars]
      calc_crc $efile
   }

   foreach filter $filterlist {
      set ccd($filter) [get_ccdpars_for_filter $configlist $econfiglist \
			    $bclist $filter $camcol $planlist]
   }

   if {$configlist != ""} { chainDestroy $configlist genericDel }
   if {$econfiglist != ""} { chainDestroy $econfiglist genericDel }
   if {$bclist != ""} { chainDestroy $bclist genericDel }
}

#
# Append tables giving the values of all the TYPEs in $args to a file,
# one type per HDU. The types are expected to be enums of #defines
#
proc append_types_to_file {file args} {
   foreach type $args {
      set ch [enum2Define $type]
      set hdr [hdrNew]
      hdrInsWithAscii $hdr "TYPENAME" $type
      schema2Fits $ch $file $hdr -append
      hdrDel $hdr
      chainDestroy $ch genericDel
   }
}

#
# overplot the regions defined by a CCDPARS
#
proc plot_ccdpars {_ccdpars {f default}} {
   upvar $_ccdpars ccdpars
   global rawnrows sao

   set rs 0; set re [expr $rawnrows - 1]
   loop amp 0 [exprGet $ccdpars($f).namps] {
      if {[exprGet $ccdpars($f).amp$amp] >= 0} {# amp is in use
	 foreach type [list PreBias PostBias Prescan Postscan Data] {
	    set ncol [exprGet $ccdpars($f).n${type}$amp]
	    if {$ncol > 0} {
	       set cs [exprGet $ccdpars($f).s${type}$amp]
	       set ce [expr $cs + $ncol]
	       set cs [expr $cs + 0.2]; set ce [expr $ce - 0.2]
	       if {$type == "Data"} {
		  set ei "e"
	       } else {
		  set ei "i"
	       }

	       saoDrawPolygon -s $sao($f) -$ei \
		   $rs $cs $rs $ce $re $ce $re $cs $rs $cs
	    }
	 }
      }
   }
}

# This proc finds all defects belonging to a given ccd and copies the info
# into photo's defect structure.
proc find_defects {defectlist camRow camCol} {
   global DFACTION

   set defects [chainNew CCDDEFECT]
   set crsr [chainCursorNew $defectlist]
   while {[set defect [chainWalk $defectlist $crsr]] != ""} {
      if {[exprGet $defect.camRow] == $camRow && \
	      [exprGet $defect.camCol] == $camCol } {
	 # copy to a new ccdDefect
	 set new [ccddefectNew] 
	 handleSet $new.iRow $camRow
	 handleSet $new.iCol $camCol
	 handleSet $new.dfcol0 [exprGet $defect.dfcol0]
	 handleSet $new.dfncol [exprGet $defect.dfncol]
	 handleSet $new.dftype [exprGet $defect.dftype -enum]
	 if {[exprGet -enum $new.dftype] == "BADBLK"} {
	    handleSet $new.dfaction $DFACTION(FILCOL)
	 } else {
	    handleSet $new.dfaction $DFACTION(BADCOL)
	 }
	 chainElementAddByPos $defects $new TAIL AFTER
	 
	 handleDel $new
	 handleDel $defect
      }
   }
   chainCursorDel $defectlist $crsr
   return $defects
}



# This proc walks down a chain of structs which must contain camRow
# and camCol as two of their elements and returns a handle to the
# first struct with matching camRow and camCol.

proc find_this_ccd {camRow camCol chain} {

   loop i 0 [exprGet $chain.nElements] {
      set foo [chainElementGetByPos $chain $i]
      if {[exprGet $foo.camRow] == $camRow && \
	      [exprGet $foo.camCol] == $camCol} {
	 break
      } else {
	 handleDel $foo; unset foo
      }
   }

   if [info exists foo] {
      return $foo
   } else {
      error "I Cannot find CCD ($camRow, $camCol) in chain $chain"
   }
}





######################################################################
# Given a chain of RC2GC and the index a struct in that list
#  produce a TRANS structure for that RC2GC

proc transFromRC2GC { rc2gc_chain index field filter} {
   global ccdpars camCol

   set size [chainSize $rc2gc_chain]
   assert { ($index >= 0) && ($index < $size) }

   # now, get the appropriate member of the CHAIN
   set el [chainElementGetByPos $rc2gc_chain $index]
   if {$field != [exprGet $el.field]} {
      handleSet $el.field $field; echo XXX FAKING element.FIELD
   }
  
   assert { [exprGet $el.field] == $field }

  # convert the RC2GC information into a new TRANS structure
  set trans [transNew]
  handleSet $trans.id $field
  handleSet $trans.a [exprGet $el.a]
  handleSet $trans.b [exprGet $el.b]
  handleSet $trans.c [exprGet $el.c]
  handleSet $trans.d [exprGet $el.d]
  handleSet $trans.e [exprGet $el.e]
  handleSet $trans.f [exprGet $el.f]

  handleDel $el
  return $trans
}
    

##########################################################################
# Make and write a primary header for output fits tables
#
proc write_primary_header {file run camcol field } {

   set hdr [hdrNew]
   hdrInsWithLogical $hdr SIMPLE T
   hdrInsWithInt $hdr BITPIX 8
   hdrInsWithInt $hdr NAXIS  0
   hdrInsWithLogical $hdr EXTEND T
   hdrInsWithInt $hdr RUN $run "Imaging Run Number"
   hdrInsWithInt $hdr CAMCOL $camcol "Column in imaging camera"
   hdrInsWithInt $hdr FIELD $field "Field sequence number within the run"
   hdrInsertLine $hdr 8 "END"
   
   hdrWriteAsFits $hdr $file
   hdrDel $hdr

}
######################################################################
# Display a region if $display is true. If $mask is true, show the mask;
# if $new is true, and $display is positive, create a new saoimage. If
# $large is true (or |$display| >= 10), create an image that's 1028x929;
# large enough to display an entire frame at a demagnification of only 2
#
# If \$cmd is not empty, treat it as a command to be evaluated in the
# caller's namespace
#

if ![info exists saoMaskColors] {
   set saoMaskColors [list green yellow blue red black magenta cyan white]
   set mask_color [list \
		    "INTERP		green" \
		    "SATUR		green" \
		    "NOTCHECKED		yellow" \
		    "BRIGHTOBJECT	blue" \
		    "OBJECT		red" \
		    "BINOBJECT		magenta" \
		    "CATOBJECT		cyan" \
		    "SUBTRACTED		cyan" \
		    "CR			cyan" \
		    "GHOST		orange" \
		   ]
}

proc display {reg {msg ""} {color "default"} {show_mask 0} {new 0} {cmd ""} {large 0}} {
   global display display_row display_col display_zoom display_filterlist sao
   global filterlist MASK_TYPE saoMaskColors mask_color saoflags

   set reg "($reg)"

   if {$color == ""} {
      set color default
   }
   
   if {![info exists display] || !$display} {
      return;
   }

   if {$color == "default"} {
      if {[llength [array names sao]] > 0} {
         set color [lindex [array names sao] 0]
      }
   } else {
      if {[info exists display_filterlist] && $display_filterlist != "all" &&
	  [lsearch $display_filterlist $color] == -1} {
	 return
      }
   }
   
   if {![info exists sao($color)] && !$new} {
      set new 1
   }
   if $new {
      if {![info exists sao($color)]} {
	 if {$large || [expr abs($display)] >= 10} {
	    set sao($color) \
		[saoDisplay $reg "$saoflags -geometry 1028x929+1+1"]
	 } else {
	    set sao($color) [eval saoDisplay $reg $saoflags]
	 }

	 set icolor [lsearch $filterlist $color];#allow integer indexing too 
	 set sao($icolor) $sao($color)
      } else {
	 eval saoDisplay $reg $sao($color) $saoflags
      }
      catch { saoReset $sao($color) }
   } else {
      if {$show_mask >= 0} {
	 eval saoDisplay $reg $sao($color) $saoflags
      }
   }
   if [catch {
      if {$show_mask && [exprGet (int)$reg.mask] != 0} {
	 saoMaskColorSet $saoMaskColors
	 
	 if {[exprGet ((SPANMASK*)$reg.mask)->cookie] == 0xc00cee} {
	    set mask [maskNew [exprGet $reg.nrow] [exprGet $reg.ncol]]
	    set sm [handleBindFromHandle [handleNew] (SPANMASK)*$reg.mask]
	    set i 0
	    foreach mt [array names MASK_TYPE] {
	       set i [lsearch $saoMaskColors [keylget mask_color $mt]]
	       if {$i < 0} {
		  continue
	       }

	       set m [handleBindFromHandle [handleNew] \
			  *$sm.masks<$MASK_TYPE($mt)>]
	       maskSetFromObjmaskChain $m $mask [expr 1<<$i]
	       handleDel $m
	       
	       incr i
	    }
	    if [catch {
	       saoMaskDisplay $mask -reverse -p bitset -s $sao($color)
	    }] {
	       saoMaskDisplay $mask -p bitset -s $sao($color); # fsao doesn't recognise -reverse
	    }
	    
	    handleDel $sm
	    maskDel $mask
	 } else {
	    saoMaskDisplay *$reg.mask -p bitset -s $sao($color)
	 }
      }
   } errmsg] {
      error "Region $reg: $errmsg"
   }
   
   if {$show_mask >= 0} {
      if {[info exists display_row] && [info exists display_col] &&
	  $display_row >= 0} {
	 saoPan  -s $sao($color) $display_row $display_col
      }
      if {[info exists display_zoom] && $display_zoom != 1} {
	 saoZoom  -s $sao($color) $display_zoom
      }
   }

   if {$cmd != ""} {
      uplevel $cmd
   }
   
   if {$color != "default" && $color != ""} {
      saoDrawText -s $sao($color) 10 10 $color
   }

   if {$display == 1 && $new >= 0} {
      if { $msg != "" } {
	 puts -nonewline [format "%-50s" "$msg"]
	 if {$color != "default" && $color != ""} {
	    puts -nonewline "($color)"
	 }
      }
      puts -nonewline " hit return to continue (! to keep going) ";
      set ans [gets stdin]
      if {$ans == "q"} {
	 error "Region: $reg"
      } elseif {$ans == "!"} {
	 set display -10
      }
   }

   return $reg
}


proc tv {reg {display 1}} {
  global saoflags
   
  if { [saoGetRegion] == ""} {
      eval saoDisplay $reg $saoflags
   } else {
      eval saoDisplay $reg $display $saoflags
   }
}
#
# Display an OBJMASK
#
proc display_objmask {args} {
   set opts [list \
		 [list [info level 0] ""] \
		 [list <objmask> STRING "" om "The objmask to display"] \
		 [list <reg> STRING "" reg "Superimposed on this region"] \
		 [list {[val]} INTEGER 1 val "Mask value to use"] \
		 [list -sao INTEGER 1 sao "saoimage to use (default: 1)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set nrow [exprGet $reg.nrow]
   set ncol [exprGet $reg.ncol]

   set ch [chainNew OBJMASK]
   chainElementAddByPos $ch $om

   set mask [maskNew $nrow $ncol]
   maskSetFromObjmaskChain $ch $mask $val

   display_region $reg 0 0 $sao
   display_mask $mask $sao

   maskDel $mask
   chainDel $ch
}

proc cpuTime {} {

   set timer [lindex [lindex [timerLap] 1] 1]
   set otime [lindex [lindex $timer 0] 1]
   set utime [lindex [lindex $timer 1] 1]
   set stime [lindex [lindex $timer 2] 1]
   
   set str [format "   CPU time %.2f\t(utime %0.2f\tstime %0.2f)" \
		$otime $stime $utime]
   return $str
}

#
# proc to get the value of a particular software parameter. 
# If the parameter in not in the keyed list softpars, get
# it from the default list defpars (from the parfile in
# $PHOTO_DIR/etc); if that fails usually throw an error.
#
# Like keylget, the value is returned unless you specify it as an extra
# argument, in which case 0 (failure) or 1 is returned with no errors thrown
#
proc getsoftpar {parameter {_value ""}} {
   global softpars defpars cmdlineParam
   
   if {$_value != ""} {
      upvar $_value value
   }

   if {[info exists cmdlineParam] && [keylget cmdlineParam $parameter value]} {
      if {$_value == ""} {
	 return $value
      } else {
	 return 1
      }
   }

   if {[info exists softpars] && [keylget softpars $parameter value]} {
      if { $value == "default"} {
	 if ![keylget defpars $parameter value] {
	    if {$_value == ""} {
	       error \
	      "I cannot find a default value for software parameter $parameter"
	    } else {
	       return 0
	    }
	 }
      }
   } else {
      if {![info exists defpars] || ![keylget defpars $parameter value]} {
	 if {$_value == ""} {
	    error "I cannot find software parameter $parameter"
	 } else {
	    return 0
	 }
      }
      verb_echo 6 "using parameter $parameter from default file"
   }

   if {$_value == ""} {
      return $value
   } else {
      return 1
   }
}

# proc to get the value of a particular QA parameter. 
# If the parameter in not in the keyed list QApars, get
# it from the default list defQApars (from the QAparfile in
# $PHOTO_DIR/etc)
proc getQApar {parameter} {

   global QApars defQApars

   if {[catch {keylget QApars $parameter}]} {
      set param [keylget defQApars $parameter]
      verb_echo 6 "using $parameter from default QAparam file"
   } else {
      set param [keylget QApars $parameter]
      if { $param == "default"} {
	 set param [keylget defQApars $parameter]
      }
   }
   return $param
}


# wrapper for post-processing QA on PSP outputs
proc pspQA {run camCol startField endField outputDir {QAparfile default} {diagFile psDiag}} {

global QApars defQApars

      # get QA parameters
      if {$QAparfile == "default"} {
          set QAparfile [envscan \$PHOTO_DIR]/etc/psQApar.par
      }
      if {[check_file $QAparfile "QA software parameters"]} {
          error "Cannot locate file $QAparfile"
      } else {
          param2Chain $QAparfile QApars
      } 
      set defQApars $QApars
 
      set QAquantities [list sky drift sigma1 sigma2 b sigmap p0 beta  \
                          PTchi2 flux20]
      foreach quant $QAquantities {
         set w1($quant) [getQApar qa_$quant\_w1]
         set w2($quant) [getQApar qa_$quant\_w2]
         set f1($quant) [getQApar qa_$quant\_f1]
         set f2($quant) [getQApar qa_$quant\_f2]
      }
      # aux: do not have info on flux20_chi2
      loop i 0 5 {
         set flux20_chi2($i) -1
      }

      # output file
      set runstr [format %06d $run]
      set logfile [open $outputDir/$diagFile-$runstr-$camCol.par w]

      # work 
      set QAflag [doQA $logfile $startField $endField $outputDir $run $camCol \
	               all $QAquantities w1 w2 f1 f2 flux20_chi2]
      close $logfile; unset logfile
  

   return $QAflag

}

#
# Given a type such as OBJECT1_FLAGS that exists as a schema, generate
# a tcl array giving its values
#
# e.g. set_array_from_define OBJECT1_FLAGS OBJECT1_ OBJECT1
# defines $OBJECT1(BRIGHT) as 2
#
proc set_array_from_define {type {prefix ""} {_arr ""}} {
   if {$_arr == ""} {
      set _arr $type
   }
   upvar $_arr arr

   if [info exists arr] { unset arr }
   set flg [genericNew $type]
   foreach el [schemaGetFromType $type] {
      handleSet $flg [lindex $el 0]
      regsub "$prefix" [lindex $el 0] "" el
      set arr($el) [exprGet (int)$flg]
   }
   genericDel $flg
}

#
# Like set_array_from_define, but used for scalars.
#
# E.g. set_value_from_define N_LINEAR_COEFFS
# defines $N_LINEAR_COEFFS as 4
#
proc set_value_from_define {type {prefix ""} {_var ""}} {
   if {$_var == ""} {
      set _var $type
   }
   upvar $_var var
   
   if [info exists var] { unset var }
   set flg [genericNew $type]
   assert {[llength [schemaGetFromType $type]] == 1}
   set el [lindex [schemaGetFromType $type] 0]
   handleSet $flg [lindex $el 0]
   regsub "$prefix" [lindex $el 0] "" el
   set var [exprGet (int)$flg]

   genericDel $flg
}
#
# We need the numerical value of various TYPE_???s to get around their being
# printed as "(enum) TYPE_???" by exprGet
#
foreach t {U8 U16 S32 FL32} {
   set tmp [regNew -type $t]
   set TYPE_$t [exprGet (int)$tmp.type]
   regDel $tmp
}

#
# write memory stats to a file, if $mem is defined
#
proc write_mem {_mem str} {
   upvar $_mem mem
   if [info exists mem] {
      set timer [lindex [lindex [timerLap] 1] 1]
      set utime [lindex [lindex $timer 1] 1]
      set stime [lindex [lindex $timer 2] 1]
      puts $mem "[memSerialNumber] \
	  [memTotalBytes] [memActualBytes] [memBytesInUse] [memBytesInPool] \
          $utime $stime $str"
      flush $mem
   }
}

#
# Defragment dervish memory
#
proc defragment_memory {} {
   if {[info commands memDefragment] == ""} {
      echo \
	  "Astrotools [astrotoolsVersion] doesn't support defragmenting memory"
      return
   }
   if {[memBlocksizeSet] == 0} {
      memDefragment -free
   } else {
      memDefragment
   }
} 



#
# Add a file's CRC to the array crc
#
proc calc_crc {args} {
   global crc crc_per_field crc_nbyte_max

   if [info exists crc_nbyte_max] {
      set crc_nbyte_max_d $crc_nbyte_max
   } else {
      set crc_nbyte_max_d 0
   }
   
   set per_field 0;			# file is only relevant for this field
   
   set opts [list \
		 [list [info level 0] "\
 Add a file's CRC to the array crc, using the first nbyte bytes of the file,
 or all if nbyte is -ve.

 If nbyte isn't provided, and the variable \$crc_nbyte_max exists, use it
 as the value of nbyte (if it's negative, no CRC is calculated; if 0 the
 whole file is included)

 If -field is provided, the CRC is only relevent to the current field, and
 will be deleted from the crc array before the next field is processed
 "] \
		 [list <file> STRING "" file "File to CRC"] \
		 [list {[nbyte]} INTEGER $crc_nbyte_max_d nbyte \
		      "Number of bytes to CRC"] \
		 [list -field CONSTANT 1 per_field \
		      "file is only relevant for this field"] \
		]
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   
   if ![file exists $file] {		# look for it compressed
      foreach s [compressionTypes] {
	 if [file exists ${file}.$s] {
	    set suffix $s; break
	 }
      }
      if [info exists suffix] {
	 append file ".$suffix"
      }
   }
   if ![file exists $file] {
      error "No such file (even compressed): $file"
   }
   
   if {$per_field && $nbyte >= 0} {
      set crc_per_field($file) 1
   }

   if {$nbyte >= 0} {
      set crc($file) [crcCalcFromFile $file -nbyte $nbyte]
   }
}

#
# Reset all per-field CRCs
#
proc crc_reset_field {} {
   global crc crc_per_field
   if [info exists crc_per_field] {
      foreach f [array names crc_per_field] {
	 if [info exists crc($f)] {
	    unset crc($f)
	 } else {
	    echo "file $f is in crc_per_field array, but not crc array"
	 }
      }
      unset crc_per_field
   }
}

proc show_crc {args} {
   global crc  crc_nbyte_max
   
   set opts {
      {show_crc "Print the saved file CRCs in a convenient format\n"}
      {-basename CONSTANT 1 basename "Don't print file's full pathname"}
      {-outfile STRING "" outfile "Write CRCs to this file"}
      {-show STRING "" show "Only print files matching regexp <show>"}
      {-not STRING "" omit "Omit files matching regexp <omit>"}
   }
   set basename 0
   if {[shTclParseArg $args $opts show_crc] == 0} {
      return 
   }
   
   if {$outfile == "" || $outfile == "-"} {
      set fd "stdout"
   } else {
      set fd [open $outfile "w"]
   }

   if ![info exists crc_nbyte_max] {
      puts $fd "The variable \$crc_nbyte_max is -ve or doesn't exist, so no CRCs were calculated"
      return
   }

   if {$crc_nbyte_max == 0} {
      puts $fd "All bytes were included in CRC"
   } else {
      puts $fd "First $crc_nbyte_max bytes were included in CRC"
   }

   foreach file [array names crc] {
      set tmp([file tail $file]) $crc($file)
      if !$basename {
	 set full([file tail $file]) $file
      }
   }

   foreach file [lsort -ascii [split [array names tmp]]] {
      if $basename {
	 set filename $file
      } else {
	 set filename $full($file)
      }

      if {[regexp $show $filename] &&
	  ($omit == "" || ![regexp $omit $filename])} {
	 puts $fd [format "%-71s 0x%05x" $filename $tmp($file)]
      }
   }

   if {$fd != "stdout"} {
      close $fd
   }
}

#
# Support for writing CRC arrays to a FITS binary table
#
typedef struct {
   char file<101>;
   int crc;
} CRC;

proc crcNew {} {
   return [genericNew CRC]
}
proc crcDel {crc} {
   return [genericDel $crc]
}

#
# Write a set of CRCs to a fits binary table. The mode should be
# "w", or "a"; a header may be provided if so desired
#
proc crc2Fits {file mode {ihdr ""}} {
   global crc crc_nbyte_max

   if [info exists crc_nbyte_max] {
      set nbyte $crc_nbyte_max
   } else {
      set nbyte -1
   }

   switch $mode {
      "a" {
	 set flags "-append"
      }
      "w" {
	 set flags ""
      }
      default { error "Unknown mode $mode" }
   }

   set crc_chain [chainNew CRC]
   
   foreach f [array names crc] {
      set el [genericNew CRC]
      handleSet $el.file $f
      handleSet $el.crc  $crc($f)
      chainElementAddByPos $crc_chain $el
      handleDel $el
   }

   set hdr [hdrNew]
   if {$ihdr != ""} {
      hdrCopy $ihdr $hdr
   }
   hdrInsWithInt $hdr "NBYTE" $nbyte "Max number of bytes used for CRC"

   if 0 {		#XXXXX STEP processing ?? $file is too long??
      eval echo RHL schema2Fits $crc_chain $file $hdr $flags
   } else {
      eval schema2Fits $crc_chain $file $hdr $flags
   }

   hdrDel $hdr
   chainDestroy $crc_chain crcDel

   return 1
}

#
# Set an array of CRCs from a fits binary table, returning the number of bytes
# of each file included in the CRCs
#
proc fits2Crc {args} {
   set print 0;				# print the CRCs?
   set opts [list \
		 [list [info level 0] "\
 Set an array of CRCs from a fits binary table, returning the number of bytes
 of each file included in the CRCs

 The typical usage of this command is probably
   set crc_nbyte_max \[fits2Crc scFang crc 18\]; show_crc
 which is equivalent to a simple
   fits2Crc scFang -print
 "] \
		 [list <file> STRING "" file "File to read CRC from.
 May be of the form table:{psp,frames}"] \
		 [list {[arr]} STRING "crc" _crc "Array to set"] \
		 [list -hdu INTEGER -1 hdu \
 "Which HDU to read (set for you for scFang/psField/fpFieldStat files)"] \
		 [list -print CONSTANT 1 print "Print the CRCs?"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   global crc_nbyte_max;		# set this variable automatically
   upvar $_crc crc

   if [regexp {^table(:(frames|psp))?$} $file] {
      global openit

      if ![info exists openit(objdir)] {
	 error "Please use set_run to specify desired run"
      }
      if {$file == "table" || $file == "table:frames"} {
	 set file $openit(objdir)/fpFieldStat-$openit(run)-$openit(col)-[format %04d $openit(field)].fit
      } elseif {$file == "table:psp"} {
	 set file \
	     [get_psField_filename $openit(psdir) $openit(run) $openit(col) $openit(field)]
      } else {
	 error "unknown file type: $file"
      }
   }

   if {$hdu < 0} {
      switch -regexp $file {
	 "fpFieldStat" { set hdu 3 }
	 "psField"     { set hdu 9 }
	 "scFang"      { set hdu 18 }
	 "default"     { error "Please specify an HDU" }
      }
   }

   set hdr [hdrNew]
   set ch [fits2Schema $file CRC $hdr -hdu $hdu]
   if [info exists crc] {
      unset crc
   }
   loop i 0 [chainSize $ch] {
      set el [chainElementGetByPos $ch $i]
      set crc([exprGet $el.file]) [exprGet $el.crc]
   }
   chainDestroy $ch crcDel

   if [catch {
      set crc_nbyte_max [hdrGetAsInt $hdr "NBYTE"]
   } msg] {
      echo "$hdr: $msg"
      set crc_nbyte_max -1;
   }
   hdrDel $hdr

   if $print {
      show_crc
   } else {
      return $crc_nbyte_max
   }
}

#
# Insert required ID keyword/value pairs in a header
#
proc insert_id_values {type hdr} {
   global id_values Npatches camCol

   if {![info exists id_values(VERSION)]} {
      set id_values(VERSION) [photoVersion]
   }
   if {![info exists id_values(ASTR_VER)]} {
      set id_values(ASTR_VER) [astrotoolsVersion]
   }
   if {![info exists id_values(DERV_VER)]} {
      set id_values(DERV_VER) [dervishVersion]
   } 
   if {![info exists id_values(BIAS_ID)]} {
      set id_values(BIAS_ID) "unknown"
   }

   if {$type == "-frames"} {
      if {![info exists id_values(FRAME_ID)]} {
	 set id_values(FRAME_ID) [idStamp]
      }
      if {![info exists id_values(KO_VER)]} {
	 set id_values(KO_VER) "devel"
      }

      foreach k "VERSION DERV_VER ASTR_VER ASTRO_ID BIAS_ID FRAME_ID KO_VER PS_ID" {
	 hdrInsWithAscii $hdr $k $id_values($k)
      }
   } elseif {$type == "-psp"} {
      if {![info exists id_values(VERSION)]} {
	 set id_values(VERSION) [photoVersion]
      }
      if {![info exists id_values(PS_ID0)]} {
	 set id_values(PS_ID0) [idStamp]
      }
      # we want a unique PS_ID for each camera column (PR 2097)
      set id_values(PS_ID) "$id_values(PS_ID0) camCol $camCol"
      set idList "VERSION DERV_VER PS_ID ASTR_VER BIAS_ID SSC_ID"
      # we don't want ASTRO_ID keyword if astrom run _after_ PSP (PR 2906)
      if {[info exists id_values(ASTRO_ID)]} {
	  if {$id_values(ASTRO_ID) != "not available" && $id_values(ASTRO_ID) != "SSC     "} {
             set idList "$idList ASTRO_ID"
          }
      }
      foreach k $idList {
	 hdrInsWithAscii $hdr $k $id_values($k)
      } 
      if {[info exists id_values(EXCAL_ID)]} {
         hdrInsWithAscii $hdr EXCAL_ID $id_values(EXCAL_ID)
      }
      if {$Npatches > 0} {
         if {$Npatches > 99} {
            error "insert_id_values: KALXX_ID keyword does not support > 99 MT patches"
         }
         loop i 1 [expr $Npatches + 1] {
            set k KAL[format %02d $i]_ID 
	    hdrInsWithAscii $hdr $k $id_values($k)
         }  
      }
   } else {
      error "Unknown pipeline: $type"
   }
    
}

proc photoVersion {} {
   set name "\$Name$";
   
   if {$name != [photoName]} {
      error "TCL and C Name strings don't match: $name v. [photoName]"
   }
   
   if {[regexp {Name: ([^\* ]+) \$$} $name foo name]} {
      return $name
   } else {
      return "NOCVS:[versionStamp PHOTO_DIR]"
   }
} 

#
# Draw a cross on an saoimage. Type should be x, *, o, or +
#
proc draw_cross {type r_cen c_cen {opts -i} {size 2}} {
   if {$type == "+"} {
      eval saoDrawPolygon \
	  [expr $r_cen-$size] [expr $c_cen] \
	  [expr $r_cen+$size] [expr $c_cen] $opts
      eval saoDrawPolygon \
	  [expr $r_cen] [expr $c_cen+$size] \
	  [expr $r_cen] [expr $c_cen-$size] $opts
   } elseif {$type == "x"} {
      set size [expr $size/sqrt(2.0)]
      eval saoDrawPolygon \
	  [expr $r_cen-$size] [expr $c_cen-$size] \
	  [expr $r_cen+$size] [expr $c_cen+$size] $opts
      eval saoDrawPolygon \
	  [expr $r_cen-$size] [expr $c_cen+$size] \
	  [expr $r_cen+$size] [expr $c_cen-$size] $opts
   } elseif {$type == "*"} {
      eval saoDrawPolygon \
	  [expr $r_cen-$size] [expr $c_cen] \
	  [expr $r_cen+$size] [expr $c_cen] $opts
      eval saoDrawPolygon \
	  [expr $r_cen-0.5*$size] [expr $c_cen+sqrt(3.0)/2*$size] \
	  [expr $r_cen+0.5*$size] [expr $c_cen-sqrt(3.0)/2*$size] $opts
      eval saoDrawPolygon \
	  [expr $r_cen-0.5*$size] [expr $c_cen-sqrt(3.0)/2*$size] \
	  [expr $r_cen+0.5*$size] [expr $c_cen+sqrt(3.0)/2*$size] $opts
   } elseif {$type == "o"} {
      eval saoDrawCircle $r_cen $c_cen $size $opts
   } else {
      error "Unknown type of cross: $type"
   }
}

###############################################################################

proc draw_cross_in_region {args} {
   set opts [list \
		 [list [info level 0] "Draw a symbol in a region"] \
		 [list <reg> STRING "" reg "The region"] \
		 [list <type> STRING "+" type "The desired symbol: + x \# * o"] \
		 [list <rowc> DOUBLE 0.0 r_cen "Row position"] \
		 [list <colc> DOUBLE 0.0 c_cen "Column position"] \
		 [list -size INTEGER 2 size "size of symbol"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$type != "+" && $type != "#"} {
      echo Setting type == +
      set type "+"
   }
   
   set r_cen [expr int($r_cen + 0.5)]
   set c_cen [expr int($c_cen + 0.5)]
   if {$type == "+"} {
      set r0 [expr $r_cen - $size]
      set r1 [expr $r_cen + $size]
      set c0 [expr $c_cen - $size]
      set c1 [expr $c_cen + $size]

      loop c $c0 [expr $c1 + 1] {
	 regPixSetWithDbl $reg $r_cen $c 255
      }
      loop r $r0 [expr $r1 + 1] {
	 regPixSetWithDbl $reg $r $c_cen 255
      }
   } elseif {$type == "\#"} {
      set r0 [expr $r_cen - $size]
      set r1 [expr $r_cen + $size]
      set c0 [expr $c_cen - $size]
      set c1 [expr $c_cen + $size]

      loop c $c0 [expr $c1 + 1] {
	 regPixSetWithDbl $reg $r0 $c 255
	 regPixSetWithDbl $reg $r1 $c 255
      }
      loop r $r0 [expr $r1 + 1] {
	 regPixSetWithDbl $reg $r $c0 255
	 regPixSetWithDbl $reg $r $c1 255
      }
   } elseif {$type == "@"} {
      set r0 [expr $r_cen - $size]
      set r1 [expr $r_cen + $size]
      set c0 [expr $c_cen - $size]
      set c1 [expr $c_cen + $size]

      loop c $c0 [expr $c_cen - 1] {
	 regPixSetWithDbl $reg $r_cen $c 255
      }
      loop c [expr $c_cen + 2] [expr $c1 + 1] {
	 regPixSetWithDbl $reg $r_cen $c 255
      }
      loop r $r0 [expr $r_cen - 1] {
	 regPixSetWithDbl $reg $r $c_cen 255
      }
      loop r [expr $r_cen + 2] [expr $r1 + 1] {
	 regPixSetWithDbl $reg $r $c_cen 255
      }
   }
}

#
# Draw a frame around a stamp in a mosaic  
#
proc draw_frame {r0 c0 size {width 1} {opts -e}} {
    loop i 0 $width {
       set rf [expr $r0 + $i]
       set cf [expr $c0 + $i]
       set s [expr $size - $i - $i] 
       saoDrawPolygon $rf $cf \
                      $rf [expr $cf + $s] \
                      [expr $rf + $s] [expr $cf + $s] \
                      [expr $rf + $s] $cf $opts
    }
}


#
# Draw a cornerless frame around a stamp in a mosaic  
#
proc draw_frame2 {r0 c0 size {width 1} {opts -e}} {
    loop i 0 $width {
       set rf [expr $r0 + $i]
       set cf [expr $c0 + $i]
       set s [expr $size - $i - $i] 
       set s3 [expr $s/3]
       saoDrawPolygon $rf [expr $cf + $s3] \
                      $rf [expr $cf + $s - $s3] \
                      [expr $rf + $s3] [expr $cf + $s] \
                      [expr $rf + $s - $s3] [expr $cf + $s] \
                      [expr $rf + $s] [expr $cf + $s - $s3] \
                      [expr $rf + $s] [expr $cf + $s3] \
                      [expr $rf + $s - $s3] $cf \
                      [expr $rf + $s3] $cf $opts
    }
}

#
# display all the peaks in an obj1, on display $sao($filter)
#
proc draw_peaks {obj1 filter {drow 0} {dcol 0} {opts -e} {nosatur 1} {label_peaks 0}} {
   global sao

   if {[exprGet (int)$obj1.peaks] == 0} {
      return;
   }
   
   set npeak [exprGet $obj1.peaks->npeak]
   
   loop j 0 $npeak {
      if {$label_peaks <= 1} {
	 set id [exprGet $obj1.id]
      } else {
	 set id [exprGet $obj1.peaks->peaks<$j>->id]
      }
      
      set rowc [expr [exprGet $obj1.peaks->peaks<$j>->rowc]+$drow]
      set colc [expr [exprGet $obj1.peaks->peaks<$j>->colc]+$dcol]

      if {$j == 0} {
	 draw_cross x $rowc $colc "-s $sao($filter) $opts"
	 if $label_peaks {
	    saoDrawText -s $sao($filter) $rowc [expr $colc+5] $id -e
	 }
      } else {
	 if {$nosatur && [exprGet $obj1.peaks->peaks<$j>->flags] & 0x100} {# PEAK_SATUR
	    #echo $nosatur;
	 } else {
	    draw_cross + $rowc $colc "-s $sao($filter) $opts"
	    if $label_peaks {
	       saoDrawText -s $sao($filter) $rowc [expr $colc+5] $id
	    }
	 }
      }
   }
}


# this procedure takes care of an error in SAO Image
proc set_mtv {args} {
   global env sao saoflags filterlist

   set rot 0;				# rotate image?
   set size ""

   while {$args != "" && [regexp -- {^-} [lindex $args 0]]} {
      if [regexp -- {-la.*} [lindex $args 0]] {
	 set size "1036x943"
      } elseif [regexp -- {-rot.*} [lindex $args 0]] {
	 set rot 1
      } else {
	 echo "Unknown argument to set_mtv: [lindex $args 0]"
      }
      set args [lreplace $args 0 0]
   }

   if {$args == ""} {
      set args "default"
   }

   set nfilter [llength $args]
   
   if {$nfilter <= 2} {
      set geom(0) "-geometry $size+1+1"
      set geom(1) "-geometry $size+513+1"
   } elseif {$nfilter <= 4} {
      set geom(0) "-geometry $size+1-448"
      set geom(1) "-geometry $size+513-448"
      set geom(2) "-geometry $size+1+328"
      set geom(3) "-geometry $size+513+328"
   } else {
      set geom(0) "-geometry $size+1-448"
      set geom(1) "-geometry $size+380-448"
      set geom(2) "-geometry $size-1-448"
      set geom(3) "-geometry $size+1+328"
      set geom(4) "-geometry $size+513+328"
   }

   set saoargs "$saoflags"
   if [info exists env(DS9_DIR)] {
      set reg [regNew 1 1];
   } else {
      set reg [regNew 1 1500];
   }
   regClear $reg; # regPixSetWithDbl $reg 0 0 10; # ds9 doesn't like constant images
   
   loop i [expr $nfilter-1] -1 -1 {
      set f [lindex $args $i]
      if $rot {
	 set saoargs "$saoargs -rot 1"
      }

      if ![info exists sao($f)] {	# we don't have an sao reserved for $f
	 set saos [lsort -integer [saoGetRegion]];	# available image displays
	 foreach s [array names sao] {	# see which are taken
	    if [regexp {^[0-9]} $s] {
	       continue
	    }
	    if {[set i [lsearch $saos $sao($s)]] >= 0} {
	       set saos [lreplace $saos $i $i]
	    }
	 }

	 if {[llength $saos] > 0} {
	    set sao($f) [lindex $saos 0]

	    global next_ds9_frame
	    set next_ds9_frame $sao($f)
	 }
      }

      if {![info exists sao($f)]} {
	 set sao($f) [saoDisplay $reg "$geom($i) -lowerleft $saoargs"]
      }
      if [catch {saoReset $sao($f)}] {
	 set sao($f) [saoDisplay $reg "$geom($i) -lowerleft $saoargs"]
      }
      
      if {$nfilter > 1} {
	 saoDrawText -s $sao($f) 2 2 $f
      }
   }
   regDel $reg
}

###############################################################################
#
# It may happen (especially in Princeton) that a file appears not to exist
# because it's on an automounted filesystem. This proc attempts to convince
# amd that it should remount before regReadAsFits fails to read a file
#
proc ping_amd {file} {
   if [file exists $file] {
      file atime $file
   } else {
      catch {
	 file atime $file
      }
   }
}

proc compressionTypes {} {
   return [list gz R Z];		# supported types of compression
}


#
# Uncompress a file
#
proc uncompress_file {file} {
   if [file exists $file] {
      return 1;				# nothing to do
   }

   foreach gz [compressionTypes] {
      if [file exists $file.$gz] {
	 echo "Decompressing $file.$gz"
	 if {$gz == "gz"} {
	    exec gunzip $file.$gz
	    return 1
	    break
	 } elseif {$gz == "Z"} {
	    exec uncompress $file.$gz
	    return 1
	 } else {
	    echo "Uh oh, I don't know how to handle fpM*.$gz files"
	    return 0
	 }
      }
   }

   return 0
}

if {[info commands regReadAsFits.vanilla] == ""} {
   rename regReadAsFits regReadAsFits.vanilla
   proc regReadAsFits {args} {
      set hdu 0
      set reg ""; set file "";		# set so that regReadAsFits -h works
      set _args ""

      while {$args != ""} {
	 set a [lindex $args 0]; set args [lrange $args 1 end]

	 if [regexp {^-} $a] {
	    if {$a == "-hdu"} {
	       set hdu [lindex $args 0]; set args [lrange $args 1 end]
	    } else {
	       lappend _args $a
	    }
	 } elseif {$reg == ""} {
	    set reg $a
	 } elseif {$file == ""} {
	    set file $a
	 } else {
	    error "Unknown argument: $a"
	 }
      }
      set args $_args

      global env errorInfo

      ping_amd $file

      if [regexp "(.+)\.([join [compressionTypes] |])$" $file "" file suffix] {
	 regsub "\.([join [compressionTypes] |])$" $file "" file
	 lappend suffices $suffix
      }

      if [file exists $file] {
	 if [catch {
	    eval {regReadAsFits.vanilla -hdu $hdu $reg $file} $args
	 } msg] {
	    if [regexp {Attempting to read into region} $msg] {
	       regsub {^TYPE_} [exprGet -enum $reg.type] {} type
	       set tmp \
		   [regNew [exprGet $reg.nrow] [exprGet $reg.ncol] -type $type]
	       
	       if [catch {		# a subregion; try a region instead
		  eval {regReadAsFits.vanilla -hdu $hdu $tmp $file} $args
	       } msg] {
		  ;			# OK, that worked
	       } else {
		  #
		  # We're probably reading an fpC into an idR region,
		  # so read data into lower left of region
		  #
		  if {[exprGet $reg.nrow] < [exprGet $tmp.ncol]} {
		     set stmp [subRegNew $tmp [exprGet $reg.nrow] [exprGet $reg.ncol] 0 0]
		     regIntCopy $reg $stmp
		     regDel $stmp
		  } else {
		     regIntCopy $reg $tmp
		  }
		  hdrFreeAll $reg.hdr
		  hdrCopy $tmp.hdr $reg.hdr
		  hdrFreeAll $tmp.hdr; regDel $tmp
		  
		  return $reg
	       }
	       
	       regDel $tmp		  
	    }

	    set msg "Reading $file: $msg"
	    echo $msg
	    return -code error -errorinfo $msg
	 }
      } else {				# maybe it's really compressed?
	 foreach suffix [compressionTypes] {
	    lappend suffices $suffix
	 }

	 foreach suffix $suffices {	# look for a file
	    ping_amd $file.$suffix
	    if [file exists $file.$suffix] {
	       break;
	    }
	 }
	 
	 if [file exists $file.$suffix] {
	    if {$suffix == "gz"} {
	       set pipeline "gunzip -cf"
	    } elseif {$suffix == "R"} {
	       set pipeline \
		   "$env(ASTROTOOLS_DIR)/bin/sdssdecompress -H $hdu -cf"
	    } elseif {$suffix == "Z"} {
	       set pipeline "uncompress -cf"
	    } else {
	       error "Unknown file suffix: $suffix"
	    }
	    set fd [open "|$pipeline $file.$suffix"]
	    if [catch {
	       eval regReadAsFits.vanilla $reg -pipe $fd $args
	    } msg] {
	       catch {
		  close $fd
	       }
	       
	       if [regexp {Attempting to read into region} $msg] {
		  regsub {^TYPE_} [exprGet -enum $reg.type] {} type
		  set tmp \
		   [regNew [exprGet $reg.nrow] [exprGet $reg.ncol] -type $type]

		  set fd [open "|$pipeline $file.$suffix"]
		  if [catch {		# a subregion; try a region instead

		     eval regReadAsFits.vanilla $tmp -pipe $fd $args
		  } msg] {
		     catch { close $fd }
		  } else {
		     catch { close $fd }
		     #
		     # We're probably reading an fpC into an idR region,
		     # so read data into lower left of region
		     #
		     if {[exprGet $reg.nrow] < [exprGet $tmp.ncol]} {
			set stmp [subRegNew $tmp [exprGet $reg.nrow] [exprGet $reg.ncol] 0 0]
			regIntCopy $reg $stmp
			regDel $stmp
		     } else {
			regIntCopy $reg $tmp
		     }
		     hdrFreeAll $reg.hdr
		     hdrCopy $tmp.hdr $reg.hdr
		     hdrFreeAll $tmp.hdr; regDel $tmp
		     return $reg
		  }

		  regDel $tmp		  
	       }
	       
	       set msg "Reading $file: $msg"
	       echo $msg
	       return -code error -errorinfo $msg
	    }
	    if [catch { close $fd } msg] {
	       echo "Closing $file: $msg"
	    }
	 } else {
	    if [catch {
	       eval {regReadAsFits.vanilla $reg $file} $args
	    } msg] {
	       set msg "Reading $file: $msg"
	       echo $msg
	       return -code error -errorinfo $msg
	    }
	    set reg $msg	      
	 }
      }

      return $reg
   }
}

if {[info commands regWriteAsFits.vanilla] == ""} {
   rename regWriteAsFits regWriteAsFits.vanilla
   proc regWriteAsFits {args} {
      global env errorInfo

      set reg ""; set file "";		# set so that regReadAsFits -h works
      set _args ""
      foreach a $args {
	 if [regexp {^-} $a] {
	    lappend _args $a
	 } elseif {$reg == ""} {
	    set reg $a
	 } elseif {$file == ""} {
	    set file $a
	 } else {
	    error "Unknown argument: $a"
	 }
      }
      set args $_args

      ping_amd $file
      if ![regexp ".+\.([join [compressionTypes] |])$" $file {} suffix] {
	 if [catch {
	    eval {regWriteAsFits.vanilla $reg $file} $args
	 } errorInfo] {
	    echo $errorInfo
	    return -code error -errorinfo $errorInfo
	 }
      } else {				# write a compressed file
	 if {[lsearch $args "-image"] < 0} {
	    set redir ">"
	 } else {
	    set redir ">>"
	 }

	 if {$suffix == "gz"} {
	    set pipeline "gzip -cf"
	 } elseif {$suffix == "R"} {
	    set pipeline "$env(ASTROTOOLS_DIR)/bin/sdsscompress -cf"
	 } elseif {$suffix == "Z"} {
	    set pipeline "compress -cf"
	 } else {
	    error "Unknown file suffix: $suffix"
	 }
	 
	 set fd [open "| $pipeline $redir $file" "w"]
	 if [catch {
	    eval regWriteAsFits.vanilla $reg -pipe $fd $args
	 } msg] {
	    echo $msg
	    catch {close $fd}

	    regsub "\.([join [compressionTypes] |])$" $file "" ufile
	    echo "sdsscompress failed to write $file;\n   writing $ufile"

	    if [catch {
	       eval regWriteAsFits.vanilla $reg $ufile $args
	    } msg] {
	       set real_errorInfo $errorInfo; lappend real_errorInfo $msg
	       catch {close $fd}
	       return -code error -errorinfo $real_errorInfo
	    }
	 }
	 catch {close $fd}
      }
   }
}
if {[info commands fitsBinTblOpen.vanilla] == ""} {
   rename fitsBinTblOpen fitsBinTblOpen.vanilla
   proc fitsBinTblOpen {file mode args} {
      ping_amd $file
      eval fitsBinTblOpen.vanilla $file $mode $args
   }
}
if {[info commands fitsBinTblHdrWrite.vanilla] == ""} {
   rename fitsBinTblHdrWrite fitsBinTblHdrWrite.vanilla
   proc fitsBinTblHdrWrite {fd args} {
      if [catch {
	 eval fitsBinTblHdrWrite.vanilla $fd $args
      }] {				# try again
	 eval fitsBinTblHdrWrite.vanilla $fd $args
      }
   }
}

#
# Return the inverse hyperbolic sin of x
#
proc asinh {x} {
   if {$x >= 0} {
      return [expr log($x + sqrt(1 + pow($x,2)))]
   } else {
      return [expr -log(-$x + sqrt(1 + pow($x,2)))]
   }
}

###############################################################################
#
# Convert a HDR as an array
#
proc hdr2array {hdr _arr} {
   upvar $_arr arr
   set n [hdrGetLineTotal $hdr]
   loop i 0 $n {
      set line [hdrGetLineCont $hdr $i]
      regexp {^(........)= *('[^']+'|[^ ]+)} $line foo key val
      set key [string trimright $key]
      set val  [string trimleft [string trimright $val " '"] " '"]
      set arr($key) $val
   }

   return $n
}

#
# Convert $source to a list of lists "name value" and return the list
#
# $source is either a file, in which case it's read, or else a global array;
# note that such arrays may be set from the command line.
#
# It is not an error for neither the file nor the array to exist, unless
# $check is true
#
proc read_planfile {source {check 0}} {
   set plan ""

   if [file exists $source] {
      param2Chain $source plan
   } else {
      global $source;			# maybe it's an array name?
      if [array exists $source] {
	 upvar #0 $source arr
	 foreach el [array names arr] {
	    lappend plan [list $el $arr($el)]
	 }
      } elseif {$check != 0} {
	 error "$source is neither a filename nor a global array"
      }
   } 

   return $plan
}

###############################################################################
#
# Process a plan read from a planfile, setting the keyed list planlist
#
set recurse_level 0;			# recursion level in includes

proc process_plan {_planlist plan} {
   upvar $_planlist planlist
   global check_planfile includeDir recurse_level

   if [info exists planlist] {
      set defaultfile 0
   } else {
      if [info exists check_planfile] {
	 unset check_planfile
      }
      set defaultfile 1
   }
   
   foreach el $plan {
      set var [lindex $el 0]
      set val [lindex $el 1]
      
      if {$var == "check_planfile" || $var == "filterlist"} {
	 set $var $val;		# we need it set before we use it
      }
		
      if {[info exists check_planfile] && $check_planfile && !$defaultfile} {
	 if {[regexp {^ccdrow_(.*)} $var {} f] &&
	     [lsearch $filterlist $f] >= 0} { # OK, a ccdrow_XX for one of our filters
	    verb_echo 3 "Accepting $var although it isn't in default plan file"
	 } elseif ![keylget planlist $var foo] {
	    set msg "Unknown plan variable $var"
	    if [regexp {^_} $var] {	# variables starting _ need not exist in the default plan file
	       if ![regexp {^__} $var] {
		  echo $msg
	       }
	    } elseif {$check_planfile <= 1} {
	       echo $msg
	    } else {
	       error $msg
	    }
	 }
      }
      #
      # Is this a request to include another file?
      #
      if {$var == "include" && $val != "default"} {
	 set file $val

	 if {$recurse_level > 5} {
	    echo "Attempt to use too many levels of recursion; ignoring $file"
	 } else {
	    if ![regexp {/} $file] {
	       eval set file $includeDir/$file
	    }

	    incr recurse_level
	    process_plan planlist [read_planfile $file 1]
	    incr recurse_level -1
	 }
      }
      
      keylset planlist $var $val

      if [regexp {^((base|include)Dir|run|rerun|camCol)$} $var] {
	 global $var
	 
	 set $var $val
      }
   }
}

###############################################################################
#
# Return a keyed-list of parameters upon which psp and frames must agree
#
# If these parameters appear in the PHOTO_DIR/etc/{fp,ps}Param.par files,
# they must agree with the values here (this is asserted upon)
#
proc getDefaultparams_photo {_defpars} {
   upvar $_defpars defpars
   
   keylset defpars_photo \
       scan_overlap 128

   foreach p [keylget defpars_photo] {
      if ![keylget defpars $p defval] {
	 error "Parameter $p is set in getDefaultparams_photo but is not in defpars"
      }
      if {$defval != "default" && $defval != [keylget defpars_photo $p]} {
	 error "Value of $p from getDefaultparams_photo differs from that in defpars"
      }
   }
}

###############################################################################
#
# A callback to return the name of the rawfile to read; may be redefined
# by the user (especially useful when processing simulations)
#
proc get_raw_filename {dir run filter camCol field} {
   global idR_format

   if {$dir != ""} {
      set file "$dir/"
   }
   append file [eval format $idR_format]

   return $file
}

proc get_corr_filename {dir run filter camCol field} {
   global fpC_format

   if ![info exists fpC_format] {
      set fpC_format {fpC-%06d-%s%d-%04d.fit  $run $filter $camCol $field}
   }
      
   if {$dir != ""} {
      set file "$dir/"
   }
   append file [eval format $fpC_format]

   return $file
}

proc get_mask_filename {dir run filter camCol field} {
   global fpM_format

   regsub {^0*(.)} $run {\1} run

   if {$dir != ""} {
      set file "$dir/"
   }
   if [info exists fpM_format] {
      append file [eval format $fpM_format]
   } else {
      append file [format "fpM-%06d-%s%d-%04d.fit"  $run $filter $camCol $field]
   }

   return $file
}

#
# Callback for bias file
#
proc get_bias_filename {dir run filter camCol {field 0}} {
   global idB_format psBB_format

   regexp {^0*([0-9]*[0-9])$} $run {} run;# trim leading 0s

   if {$dir != ""} {
      set file "$dir/"
   }
   if {$field == 0} {
      append file [eval format $idB_format]
   } else {
      append file [eval format $psBB_format]
   }

   return $file
}

#
# Callback for flat field file (psFF)
#
proc get_ff_filename {dir run filter camCol {field 0}} {
   global psFF_format
   
   if {$dir != ""} {
      set file "$dir/"
   }
   append file [eval format $psFF_format]

   return $file
}

#
# Callback for flat field file (idFF)
#
proc get_idFF_filename {dir iterSeason filter camCol} {
   global idFF_format
   
   if {$dir != ""} {
      set file "$dir/"
   }
   append file [eval format $idFF_format]

   return $file
}

#
# Callback for asTrans file
#
proc get_trans_filename {dir run filter camCol {field 0}} {
   global asTrans_format
   
   regexp {^0*([0-9]*[0-9])$} $run {} run;# trim leading 0s

   if {$dir != ""} {
      set file "$dir/"
   }
   append file [eval format $asTrans_format]

   return $file
}

#
# Callback for scFang file
#
proc get_scfang_filename {dir run filter camCol field} {
   global scFang_format
   
   if {$dir != ""} {
      set file "$dir/"
   }
   append file [eval format $scFang_format]

   return $file
}

#
# Callback for psField file
#
proc get_psField_filename {dir run camCol field} {
   global psField_format
   
   regexp {^0*([1-9][0-9]*|0)$} $run {} run # strip leading 0s

   if {$dir != ""} {
      set file "$dir/"
   }
   append file [eval format $psField_format]

   return $file
}


###############################################################################

proc vFitPolynom {x y M} {

     # first make sure that the vectors are of the same length
     assert { [exprGet $x.dimen] == [exprGet $y.dimen] }
     set N [exprGet $x.dimen]
     # and then make sure that there are enough points
     assert { $N > $M }
 
     # form the matrix A such that A * F = B, where F = (a, b, c)
     # A = ((N,Sx,Sxx,...,Sx^M),(Sx, Sxx,...,Sx^M+1),...,(Sx^M,...,Sx^2M)) 
     # and B = (Sy, Syx, ..., Syx^M))

     # first get the sums
     set alpha0 [vectorExprNew [expr 2*$M+1]]
     set alpha1 [vectorExprNew [expr $M+1]]

     # this loops over all vector elements and adds the contribution
     # to the appropriate sum (i.e. matrix element)

     loop i 0 [expr 2*$M+1] {
	handleSet $alpha0.vec<$i> [vectorExprGet sum($x^$i)]
     }
     loop i 0 [expr $M+1] {
	handleSet $alpha1.vec<$i> [vectorExprGet sum($y*$x^$i)]
     }

     # the hard part is over, now invert the matrix
     # since vMatrixInvert takes a list of vectors, each row of A
     # is an (M+1)-element vector, and there are M+1 such rows  
     set Aveclist {}
     # loop over rows
     loop i 0 [expr $M+1] {
        set vec [vectorExprEval "$alpha0<($i,$i+$M)>"]
        lappend Aveclist $vec
     } 
     # all set, invert the matrix
     vMatrixInvert $Aveclist

     # evaluate the coefficients
     set Clist {}
     loop i 0 [expr $M+1] {
        set vec [lindex $Aveclist $i]
        lappend Clist [vectorExprGet sum($vec*$alpha1)]
	vectorExprDel $vec
     }

     vectorExprDel $alpha0; vectorExprDel $alpha1
     
     return $Clist

}


# given a list of coefficients returned by vFitPolynom, evaluate 
# and return vector y = Sum[C(k)*x^k], k=0..M
proc vEvalPolynom {x Clist} {
   set y [vectorExprEval [lindex $Clist 0]]
   loop i 1 [llength $Clist] {
      vectorExprSet $y "$y + [lindex $Clist $i]*$x^$i"
   }

   return $y

}

###############################################################################
#
# Define help strings for commands; for each command you want help on,
# you must make "procname -help" work, and say e.g.
#    lappend ftclHelp_procs showFocus
#
# I typically do this just above the start of the definition; the array
# name (here ftclHelp_procs) is arbitrary
#
# and at the end of the file say:
#    set_ftclHelp $prodName ftclHelp_procs
#
proc set_ftclHelp {help_class _proc_list} {
   upvar $_proc_list proc_list

   if [info exists proc_list] {
      foreach p $proc_list {
	 ftclHelpDefine $help_class $p "\n[eval $p -help]";
      }
      unset proc_list
   }
}

###############################################################################
#
# Like memBlocksGetRange, except that certain blocks may be omitted. The
# blocks are set using memBlocksIgnore; e.g.
#      set n1 [memSerialNumber]
#      allocate lots of unfreed memory that we don't care about
#      memBlocksIgnore $n1 [memSerialNumber]
#
# The list of ignored blocks (as set by memBlocksIgnore) may be pruned to
# only contain blocks that are still allocated using memBlocksIgnorePrune
#
proc memBlocksGetRangeExcept {n1 n2} {
   assert "memBlocksGetRangeExcept no longer useful -- use dervishes memBlocksGet"
}

proc memBlocksIgnore {{n1 0} {n2 0}} {
    assert "memBlocksIgnore no longer useful -- use dervishes memIgnoreBlocks"
}

proc memBlocksIgnorePrune {} {
    assert "memBlocksIgnorePrune no longer useful"
}

# given a list of coefficients returned by vFitPolynom, evaluate 
# and return y = Sum[C(k)*x^k], k=0..M
proc yEvalPolynom {x Clist} {

   set y [lindex $Clist 0]
   loop i 1 [llength $Clist] {
      set y [expr $y + [lindex $Clist $i]*pow($x,$i)]
   }

   return $y

}
###############################################################################
#
# Set the random number generator's seed
#
# We XOR fp_random_seed with a number based on the run and field number, and
# then use tclX's random number to propagate the randomness into all
# the seed's digits. The result is deterministic, but is stored in the
# header anyway in case you need it.
#
# If "type" is anything but run, we modify this initial seed (also in a
# deterministic way).  This can be used to e.g. reset the seeds just before
# sky subtraction.
#
proc set_random_seed {rand type args} {
   global run camCol field

   set seed [expr ([getsoftpar fp_random_seed] ^ ($run << 4)) ^ \
		 [string range [format "%d%02d%02d" \
				    [expr $camCol+$field] $field $field] 0 9]]
   random seed [expr abs($seed)]
   set seed [random 99999999]
   
   switch -- $type {
      "field" {}
      "sky" {
	 set f [lindex $args 0]
	 if ![regexp {^[a-zA-Z]$} $f] {
	    error \
  "Please provide a filter name (not \"$f\") with set_random_seed rand \"sky\""
	 }
	 set iband [lsearch "ugriz" $f]
	 loop i 0 [expr $iband + 2] {	# +2 so -1 works (unknown filter name)
	    set seed [random $seed]
	 }
      }
   }

   phRandomSeedSet $rand $seed

   return $seed
}

###############################################################################
#
# Return a list of names in canonical filter order
#
proc canonizeFilterlist { names } {
   set filternames ""
   foreach f "o l r i u z g t s" {
      if {[lsearch $names $f] >= 0} {
	 lappend filternames $f
      }
   }

   if {[llength $names] != [llength $filternames]} {# at least one unknown name
      foreach f $names {
	 if {[lsearch $filternames $f] < 0} {
	    lappend filternames $f
	 }
      }
   }

   return $filternames
}

###############################################################################
#
# Make a hard link or copy a file
#
proc link_or_copy {infile outfile {verbose 0}} {
   if [catch {link $infile $outfile} msg] {  # hard link failed
      if $verbose {
         echo "Failed to make hardlink: $msg. Copying file"
      }
      if [catch { set in [open $infile] } msg] {
         error $msg
      }
      if [catch { set out [open $outfile w] } msg] {
	 catch {close $in}
         error $msg
      }

      copyfile $in $out
      close $in; close $out
   }
}

###############################################################################
#
# Rename or move a file
#
proc rename_or_move {args} {
   set verbose 0;

   set opts [list \
		 [list [info level 0] \
		      "Rename or move a file, even across file systems"] \
		 [list <infile> STRING "" infile "File to be moved"] \
		 [list <outfile> STRING "" outfile "Destination filename"] \
		 [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if [catch {frename $infile $outfile} msg] {  # rename failed
      if $verbose {
         echo "Failed to rename: $msg. Copying file"
      }
      if [catch { set in [open $infile] } msg] {
         error $msg
      }
      if [catch { set out [open $outfile w] } msg] {
	 catch {close $in}
         error $msg
      }

      copyfile $in $out
      close $in; close $out
      
      unlink $infile
   }
}

###############################################################################
#
# Set the CCD-related fields in fieldparams
#
proc set_ccds_in_fieldparams {_ccdpars fieldparams filterlist {_fullWell ""}} {
   upvar $_ccdpars ccdpars
   if {$_fullWell != ""} {
      upvar $_fullWell fullWell
   }
   
   handleSet $fieldparams.ncol \
       [exprGet $ccdpars([lindex $filterlist 0]).nDataCol]

   loop i 0 [llength $filterlist] {
      set f [lindex $filterlist $i]

      if {[exprGet $ccdpars($f).amp0] >= 0} {
	 set fullWell(0,$f) [exprGet $ccdpars($f).fullWell0]
	 if {[exprGet $ccdpars($f).amp1] >= 0} {
	    set fullWell(1,$f) [exprGet $ccdpars($f).fullWell1]
	 } else {
	    set fullWell(1,$f) $fullWell(0,$f)
	 }
      } else {
	 assert {[exprGet $ccdpars($f).amp1] >= 0}

	 set fullWell(1,$f) [exprGet $ccdpars($f).fullWell1]
	 set fullWell(0,$f) $fullWell(1,$f)
      }
      loop j 0 2 {
	 handleSet $fieldparams.frame<$i>.fullWell<$j> [set fullWell($j,$f)]
      }

      handleSet $fieldparams.frame<$i>.namp \
	  [expr ([exprGet $ccdpars($f).amp0] < 0 ? 0 : 1) + \
	        ([exprGet $ccdpars($f).amp1] < 0 ? 0 : 1)]
      foreach cr {col row} {
	 handleSet $fieldparams.frame<$i>.${cr}Binning \
	     [exprGet $ccdpars($f).${cr}Binning]
      }
      #
      # Set astrometric offsets for binned 2-amp chips
      #
      set astrom_dcols [get_astrom_dcol \
			    [exprGet $fieldparams.frame<$i>.colBinning] \
			    [exprGet $fieldparams.frame<$i>.namp]]

      foreach lr "left right" {
	 handleSet $fieldparams.frame<$i>.astrom_dcol_$lr \
	     [keylget astrom_dcols $lr]
      }

      if ![getsoftpar electronic_ghost_amplitude_$f ga] {
	 getsoftpar electronic_ghost_amplitude_default ga
      }

      if [info exists ga] {		# not needed in PSP
	 handleSet $fieldparams.frame<$i>.electronic_ghost_amplitude $ga
      }
   }
}

#
# Given the binning and number of amps, return the number of pixels
# that must be added to a measured position to go to the position that
# is reported to astrom (i.e. the number of "missing" pixels in the device
# to the _left_ of a specified column.
#
# The result is a keyed list (keys: "left" and "right") for the two
# halves of the CCD
#
proc get_astrom_dcol {colBinning namp} {
   if {$namp == 1} {
      keylset astrom_dcol "left"  0
      keylset astrom_dcol "right" 0
   } else {
      switch $colBinning {
	 "1" {
	    keylset astrom_dcol "left"  0
	    keylset astrom_dcol "right" 0
	 }
	 "2" {
	    keylset astrom_dcol "left"  0
	    keylset astrom_dcol "right" 0
	 }
	 "4" {
	    keylset astrom_dcol "left"  0
	    keylset astrom_dcol "right" 1.5
	 }
	 "default" {
	    error "Unknown binning factor $colBinning"
	 }
      }
   }

   return $astrom_dcol
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

proc binregionFromRegion {args} {
   global PHOTO_CONSTS TYPE_FL32

   set opts [list \
		 [list [info level 0] "Return a BINREGION containing the given REGION"] \
		 [list <reg> STRING "" reg "REGION containing data"] \
		 [list -bin_row INTEGER 1 bin_row "Bining in row direction"] \
		 [list -bin_col INTEGER 1 bin_col "Bining in column direction"] \
		 [list -rsize INTEGER -1 rsize "True number of rows in unbinned image"] \
		 [list -csize INTEGER -1 csize "True number of column in unbinned image"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$rsize < 0} {
      set rsize [exprGet $reg.nrow]
   }
   if {$csize < 0} {
      set csize [exprGet $reg.ncol]
   }

   set breg [binregionNew]

   handleSetFromHandle $breg.reg &$reg
   handleSet $breg.bin_row $bin_row
   handleSet $breg.bin_col $bin_col
   handleSet $breg.rsize $rsize
   handleSet $breg.csize $csize

   if {[exprGet (int)$reg.type] == $TYPE_FL32} {
      handleSet $breg.shift $PHOTO_CONSTS(MAX_U16)
   }

   return $breg
}

#
# These are provided by ds9.tcl in dervish which isn't yet current in Feb 2003
#
if ![info exists sao_is_ds9] {
   set sao_is_ds9 0
   proc saoDrawCache {args} {}
}
