set help_procs ""

ftclHelpDefine phOutputs objc_intro \
    "

This file provides code to read photo's output, the OBJC_IO and ATLAS_IMAGE
files; optionally the test info file is also read

Usage:
	set tab \[objfileOpen OBJFILE AIFILE\]
	set obj \[objcIoRead tab 101\]
	set obj \[objcIoRead tab 93\]
	(etc.)
	objfileClose table

or, instead of using objIoRead, try
       mtv_objc table 77
to display object 77, or
       p_objc table 77
to print it. You can reconstruct a complete frame with reconstruct_frame."

ftclHelpDefine phOutputs select \
    "
Usage: select objcIo \[band\]

There is no actual proc called select, but you can write your own
to control some of the phOutputs commands.

<select> is the name of a procedure taking one or two arguments, a
handle to an OBJC_IO and an optional band (default: 2); only if it
returns true will the object be displayed.

<select> can also be a logical expression of <select> procs, e.g.
   sel_cr||(sel_star&&!sel_galaxy)

e.g.

proc sel_star {obj {band -1}} {
   global OBJECT1

   if {\$band < 0} {
      set band0 0
      set band1 \[exprGet \$obj.ncolor\]
   }  else {
      set band0 \$band;
      set band1 \[expr \$band + 1\]
   }

   loop band \$band0 \$band1 {
      set flgs \[exprGet \$obj.flags<\$band>\]

      if {\$flgs & (\$OBJECT1(EDGE)|\$OBJECT1(BLENDED))} {
	 return 0;
      }
   }
      
   if {\[exprGet -enum \$obj.objc_type\] == \"OBJ_STAR\"} {
      return 1
   } else {
      return 0
   }
}
"

##############################################################################

lappend help_procs objfileOpen
proc objfileOpen {args} {
   global openit

   set opts [list \
		 [list [info level 0] "\
 Open a pair of photo's output files, <file> for OBJCs and <aifile> for the
 atlas images. Return a list of Useful Things for other objc routines.

 If the optional argument testfile is provided, also read the TEST_INFO file\
 "] \
		 [list <file> STRING "" file "fpObjc (or tsObj) file to open"]\
		 [list <aifile> STRING "" aifile "fpAtlas file to open"]\
		 [list {[hdu]} INTEGER 1 hdu "Desired HDU"] \
		 [list {[testfile]} STRING "" testfile \
		      "TEST_INFO file to open"] \
		 [list {[OBJfd]} STRING "" OBJfd \
		      "descriptor for preopened fpObjc file"]\
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set is_fpObjc [regexp {fpObjc} $file]

   set hdr [hdrReadAsFits [hdrNew] $file]

   set filters [string trim [hdrGetAsAscii $hdr "FILTERS"]]
   set run [hdrGetAsInt $hdr "RUN"]
   set camCol [hdrGetAsInt $hdr "CAMCOL"]
   if $is_fpObjc {
      set version [hdrGetAsAscii $hdr "VERSION"]
      set ref_band [hdrGetAsInt $hdr "REF_BAND"]
      set field0 [hdrGetAsInt $hdr "FIELD"]
      set nfields 1
   } else {
      if {[catch { set version [hdrGetAsAscii $hdr "PHOT_VER"] } msg] &&
	  [catch { set version [hdrGetAsAscii $hdr "PHOTO_VE"] }]} {
	 if [catch {
	    set version [hdrGetAsAscii $hdr "PCALVERS"]
	 }] {
	    if {![info exists openit(version)] ||
		$openit(version) != "unknown"} {
	       echo $msg;		# try to only print message once
	    }
	    set version "unknown"
	 }
      }
      set version [string trimright $version]
      
      if [catch {set ref_band [hdrGetAsInt $hdr "REF_BAND"]}] {
	 set ref_band [hdrGetAsAscii $hdr "REF_BAND"]
	 set ref_band [lsearch $filters [string trimright $ref_band]]
      }
      set field0 [hdrGetAsInt $hdr "FIELD0"]
      set nfields [hdrGetAsInt $hdr "NFIELDS"]      
   }

   if [catch {				# present in summary files
      set rerun [hdrGetAsAscii $hdr "RERUN"]
   }] {
      set rerun ""
   }

   set field [expr $field0 + $hdu - 1]
   #
   # OK, free header and proceed
   #
   hdrFreeAll $hdr; hdrDel $hdr
   set i_ncolor [llength $filters]

   declare_schematrans $i_ncolor 

   if $is_fpObjc {
      if {$hdu != 1} {
	 error "Only the first HDU is useful for fpObjc files"
      }
      if {$OBJfd != ""} {
	  error "nextHdu only makes sense with tsObj files"
      }
      set OBJfd [fitsBinTblOpen $file r]
      set fileSchema "OBJC_IO"
   } else {
       if {$OBJfd == ""} {
	   set OBJfd [tsObjOpen $file $hdu]
       } else {			# preopened
	   fitsBinTblEnd $OBJfd;# finish this HDU and skip to next one
       }
      set fileSchema "TSOBJ"
   }
   set OBJnrow [fitsBinTblHdrRead -quiet $OBJfd $fileSchema]

   if {$aifile == ""} {
       set ATLfd ""
       set ATLnrow $OBJnrow
   } else {
       set ATLfd [fitsBinTblOpen $aifile r]
       set ATLnrow [fitsBinTblHdrRead -quiet $ATLfd "ATLAS_IMAGE"]
   }

   if {$OBJnrow != $ATLnrow} {
      echo [concat \
	     "Different number of rows in $fileSchema and ATLAS_IMAGE tables:"\
		 $file $aifile]
   }

   if {$testfile == "" || ![file exists $testfile]} {
      set TSTfd ""
   } else {
      if [catch {
	 set TSTfd [fitsBinTblOpen $testfile r]
	 set TSTnrow [fitsBinTblHdrRead -quiet $TSTfd "TEST_INFO"]
	 if {$OBJnrow != $TSTnrow} {
	    echo \
		"Different number of rows in $fileSchema and TEST_INFO tables (ignoring TEST_INFO)"
	    fitsBinTblClose $TSTfd; set TSTfd ""
	 }
      } msg] {
	 echo "Error opening TST file $testfile: $msg"
	 if [info exists TSTfd] {
	    catch { fitsBinTblClose TSTfd }
	 }
	 set TSTfd ""
      }
   }
   #
   # Find ncolor from table (if it's there)
   #
   if $is_fpObjc {
      set objcIo [objcIoNew]; handleSet $objcIo.ncolor -1
      fitsBinTblRowRead $OBJfd $objcIo
      fitsBinTblRowUnread $OBJfd
      set ncolor [exprGet $objcIo.ncolor]
      catObjDel $objcIo -deep

      if {$ncolor < 0} {		# not in table
	 set ncolor $i_ncolor
      }
      
      if {$ncolor != $i_ncolor} {
	 error "Expected ncolor == $i_ncolor, but found $ncolor in table"
      }
   } else {
      set ncolor $i_ncolor
   }
   #
   # Create returned table
   #
   keylset table is_summary_file 0
   keylset table rerun [string trimright $rerun]
   foreach el [list fileSchema OBJfd ATLfd TSTfd ncolor OBJnrow \
		   filters run camCol field nfields ref_band version] {
      lappend table [list $el [set $el]]
   }
      
   return $table
}

lappend help_procs objcVersion
proc objcVersion {args} {
   if {[lindex [info level [expr [info level] - 1]] 0] == "p_objc"} {
      if {[lindex $args 1] == "0"} {
	 upvar table table
	 return [objcVersion]
      }
      
      return ""
   }

   set psp_version 0
   set opts [list \
		 [list [info level 0] "\
      Return the version of photo used to prepare the output files;
 you must open a file before using this command"] \
		 [list {[table]} STRING "table" _table \
		      "name of table variable, as returned by openit"] \
		 [list -psp CONSTANT 1 psp_version "Return version of PSP"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table

   if $psp_version {
      set version "pspVersion"
   } else {
      set version version
   }

   if [keylget table $version version] {
      return $version
   } else {
      error "You don't seem to have opened an fpObjc/tsObj file"
   }
}

##############################################################################

lappend help_procs objfileClose 
proc objfileClose {args} {
   set opts [list \
		 [list [info level 0] \
 "Close a table, given the name of a list returned by objfileOpen"] \
		 [list <table> STRING "table" _table \
		      "name of table variable, as returned by openit"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table
   global fieldparams

   if [info exists fieldparams] {
      catch {
	 fieldparamsDel $fieldparams
      }
      unset fieldparams
   }
   
   set OBJfd [keylget table OBJfd]
   set ATLfd [keylget table ATLfd]
   set TSTfd [keylget table TSTfd]

   fitsBinTblClose $OBJfd; 
   if {$ATLfd != ""} {
       fitsBinTblClose $ATLfd
   }
   if {$TSTfd != ""} {
      fitsBinTblClose $TSTfd
   }

   if [keylget table truth_table ttable] {
      if {![regexp {^h[0-9]+$} $ttable] ||
	  [catch { set type [handleType $truthfile] }]} {
	 close_truthtable;		# not a CHAIN
      }
   }

   unset table
   set openit(hdu) -1

   global objcIo;
   if {[info exists objcIo]} {
      catch {
	 catObjDel $objcIo -deep
      }

      unset objcIo
   }

   fitsBinForgetSchemaTrans NULL;
}

###############################################################################
#
# A utility to return the run/camCol/field from wherever they're hidden
#
proc get_run_camCol_field {_table objcIo} {
   upvar $_table table
   
   if {"[keylget table fileSchema]" == "OBJC_IO"} {
       set is_fpObjc 1
   } else {
       set is_fpObjc 0
   }

   foreach v "run camCol field" {
      if $is_fpObjc {
	 lappend ret [keylget table $v]
      } else {
	 lappend ret [exprGet $objcIo.$v]
      }
   }

   return $ret
}

##############################################################################

lappend help_procs offsetsRead

proc offsetsRead {args} {
   set opts [list \
		 [list [info level 0] "\
 Read a PSP calibration file <calibfile>, usually a psField file, but psCB
 files are also allowed"] \
		 [list <field> INTEGER 0 field "Desired field"] \
		 [list <ref_color> STRING "r" ref_color "Reference colour"] \
		 [list <calibfile> STRING "" calibfile "Calibration file"] \
		 [list <filters> STRING "" filterlist "Filters"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   
   if [regexp {psField} $calibfile] {
      global allow_old_psField
      if ![info exists allow_old_psField] {
	 if [catch {schemaGetFromType _CALIB_IO}] {
	    set hdr [hdrNew]	 
	    set fd [fitsBinTblOpen $calibfile r -hdr $hdr]
	    set file_filterlist [hdrGetAsAscii $hdr "FILTERS"]
	    hdrDel $hdr
	    catch {
	       foreach f $file_filterlist {
		  fitsBinTblHdrRead $fd "NULL"
	       }
	       fitsBinTblHdrRead $fd _CALIB_IO
	    }
	    fitsBinTblClose $fd
	 }

	 set sch [schemaGetFromType _CALIB_IO]
	 if {!([keylget sch psf_sigma1 foo] ||[keylget sch PSF_SIGMA1 foo]) ||
	     !([keylget sch psp_status foo] || [keylget sch PSP_STATUS foo])} {
	    echo "Reading psField files in compatibility mode"
	    set allow_old_psField 1
	 }
      }
	  
      set calibs [read_calib1byframe $field $calibfile $filterlist]
   } else {
      set calibs [read_psCB $field $calibfile $filterlist]
   }

   set ncolors [exprGet $calibs.ncolors]
   assert {$ncolors == [llength $filterlist]}

   return $calibs
}

#
# This proc is only required to read old psCB files. It reads the fits table
# schema directly as the CALIB_IO schema may have changed
#
proc read_psCB {field cbfile filterlist} {
   set nfilter [llength $filterlist]
   
   # first read the header, and figure out where the filters we need
   # are in the input file which has info for all filters
   set hdr [hdrReadAsFits [hdrNew] $cbfile]
   set psfilterlist [string trim [hdrGetAsAscii $hdr FILTERS]]
   set field0 [hdrGetAsInt $hdr FIELD0]
   set nfields [hdrGetAsInt $hdr NFIELDS]
   hdrDel $hdr

   foreach filter $filterlist {
      if {[set psindex($filter) [lsearch $psfilterlist $filter]] == -1} {
         error "$cbfile does not contain info for filter $filter"
      }
   }
   
   # Read in the calib for this field

   if {[lsearch [typesList] _CALIB_IO] < 0} {# we don't know file's schema
      set tbl [tblColNew]
      fitsRead $tbl $cbfile -hdu 1
      typeNewFromTbl $tbl _CALIB_IO
      handleDelFromType $tbl
   }

   set trans [schemaTransNewAll _CALIB_IO]
   fitsBinDeclareSchemaTrans $trans _CALIB_IO
   handleDel $trans

   if {[catch {set fd [fitsBinTblOpen $cbfile r]} msg] ||
       [catch {set nrow [fitsBinTblHdrRead $fd _CALIB_IO]} msg]} {
      error "Error opening $cbfile: $msg"
   }
   set _calibio [genericNew _CALIB_IO]
   set row [expr $field-$field0]

   if { $row < 0 || $row >= $nfields} {
      error "read_calib: $cbfile does not contain an hdu for field $field"
   }

   fitsBinTblRowSeek $fd $row 0
   if [catch {fitsBinTblRowRead $fd $_calibio} msg] {
      error "Error reading $cbfile: $msg"
   }
   assert {[exprGet $_calibio.field] == $field}

   set calibio [calibIoNew 5]
   handleSet $calibio.ncolor $nfilter

   foreach el [schemaGetFromType CALIB_IO] {
      set calib_io_el([lindex $el 0]) 1
   }

   foreach el [schemaGetFromType _CALIB_IO] {
      set name [lindex $el 0]
      set type [lindex $el 1]

      set Name $name
      if ![info exists calib_io_el($Name)] {# wrong schema names?
	 regsub {counts$} $Name Counts Name
	 regsub {err$} $Name Err Name
	 regsub {2g$} $Name 2G Name
	 if ![info exists calib_io_el($Name)] {# missing in CALIB_IO
	    continue;
	 }
      }
      
      regexp {^([A-Z_a-z0-9]+)} $type prim
      if [regexp {\[[0-9]\]} $type] {
	 if [regexp {\[[0-9]\]\[[0-9]+\]} $type] {
	    continue;			# ignore 2-d fields
	 }
	 loop i 0 5 {
	    handleSetFromHandle ($prim)$calibio.$Name<$i> $_calibio.$name<$i>
	 }
      } else {
	 handleSetFromHandle ($prim)$calibio.$Name $_calibio.$name
      }
   }

   set calib1byframe [calib1byframeNewFromCalibIo $calibio $filterlist psindex]

   genericDel $calibio
   fitsBinTblClose $fd

   loop i 0 $nfilter {
      assert {[exprGet $calib1byframe.calib<$i>->psf->b] >= 0}
   }
   return $calib1byframe
}

##############################################################################
#
# Fields in tsObj files to use to save the run/column/field information
#
proc set_alias_ondisk {} {
   global alias_inmem alias_ondisk

   if [info exists alias_ondisk] {
      return
   }

   if [catch {
      set sch [schemaGetFromType TSOBJ]
      set candidate_aliases [list rosatExtLike rosatDetectLike rosatExposure \
				 rosatext rosathr1 rosathr2]
   }] {
      set sch [schemaGetFromType OBJC_IO]
      loop i 0 5 {
	 lappend candidate_aliases texture<$i>
      }
   }
   
   set alias_inmem [list run camCol field]
   set nalias 0
   foreach f $candidate_aliases {
      if ![regexp {^([^<]+)} $f {} fbase] { set fbase $f }
      if [keylget sch $fbase foo] {	# present in schema
	 lappend alias_ondisk $f
	 incr nalias
      }
      if {$nalias == 3} { break; }
   }
   
   if {$nalias < 3} {
      error "Failed to find enough fields to use as aliases"
   }
}

lappend help_procs objcIoRead

proc objcIoRead {args} {
   set opts [list \
		 [list [info level 0] "\
 Read the data from a <table>; get the <n>th OBJC_IO which is both returned
 and saved in the global \$objcIo. If n is < 0, and there is already a global
 \$objcIo, return it instead

 You can specify <n> as next:r or prev:r to go forward or backwards <r>
 rows in the table; if omitted <r> defaults to 1.

 If the optional third argument is false (0), a missing object doesn't raise
 an exception.
 "] \
		 [list <table> STRING "" _table "Name of table file"] \
		 [list <id> STRING "" id "ID of object"] \
		 [list {[abort]} INTEGER 1 abort_if_missing \
		      "Abort if object isn't in file"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table
   global objcIo

   set OBJfd [keylget table OBJfd]
   set ATLfd [keylget table ATLfd]
   set TSTfd [keylget table TSTfd]
   set ncolor [keylget table ncolor]
   set nrow [keylget table OBJnrow]
   
   if {$id == "+"} {
      set id "next"
   } elseif {$id == "-"} {
      set id "prev"
   } 

   if {$id < 0} {
      if [info exists objcIo] {
	 return $objcIo
      } else {
	 error "No [keylget table fileSchema] has been read"
      }
   } elseif [regexp {(next|prev)(:([0-9]+))?} $id foo NP foo nskip] {
      if {$nskip == ""} {
	 set nskip 0
      }

      if {$NP == "next"} {
	 fitsBinTblRowSeek $OBJfd $nskip 1
      } else {
	 fitsBinTblRowSeek $OBJfd [expr -($nskip + 2)] 1
      }
   } else {
      if {[keylget table index index] && $index != "quiet"} { #look up in index
	 global $index
	 if [info exists ${index}($id)] {
	    set n [set ${index}($id)]
	 } else {				# not present in table
	    if $abort_if_missing {
	       error "Object $id is not present in the table"
	    } else {
	       return ""
	    }
	 }
      } else {
	 set n [expr $id-1];		# IDs are 1-indexed
      }

      if {$n >= $nrow} {
	 error "There are only $nrow objects in the table"
      } 
	      
      if {$n < 0 } {
	 error "Table row must be >= 0 (and object ID >=1)"
      } 
      
      fitsBinTblRowSeek $OBJfd $n 0
   }
   
   if {"[keylget table fileSchema]" == "OBJC_IO"} {
       set is_fpObjc 1
   } else {
       set is_fpObjc 0
   }

   if $is_fpObjc {
      set objcIo [objcIoNew]
   } else {
      set objcIo [genericNew "TSOBJ"]

      set aimage [atlasImageNew -ncolor $ncolor]
      handleSetFromHandle $objcIo.aimage &$aimage
      handleDel $aimage

      set test [testInfoNew $ncolor]
      handleSetFromHandle $objcIo.test &$test
      handleDel $test
   }
   
   fitsBinTblRowRead $OBJfd $objcIo

   if {$ATLfd != ""} {
      if [regexp {(next|prev)(:([0-9]+))?} $id foo NP foo nskip] {
	 if {$nskip == ""} {
	    set nskip 0
	 }
	 
	 if {$NP == "next"} {
	    fitsBinTblRowSeek $ATLfd $nskip 1
	 } else {
	    fitsBinTblRowSeek $ATLfd [expr -($nskip + 2)] 1
	 }
      } else {
	 fitsBinTblRowSeek $ATLfd $n 0
      }
      fitsBinTblRowRead $ATLfd *$objcIo.aimage
   }
   if {$TSTfd != ""} {
      if [regexp {(next|prev)(:([0-9]+))?} $id foo NP foo nskip] {
	 if {$nskip == ""} {
	    set nskip 0
	 }
	 
	 if {$NP == "next"} {
	    fitsBinTblRowSeek $TSTfd $nskip 1
	 } else {
	    fitsBinTblRowSeek $TSTfd [expr -($nskip + 2)] 1
	 }
      } else {
	 fitsBinTblRowSeek $TSTfd $n 0
      }

      fitsBinTblRowRead $TSTfd *$objcIo.test
   }
   handleSet $objcIo.ncolor $ncolor

   if [keylget table is_summary_file] {
      if {!$is_fpObjc &&
	  ![keylget table many_fields val]}  {# a per-field tsObj file
	 regsub {^0*} [keylget table run] "" run
	 handleSet $objcIo.run $run
	 handleSet $objcIo.camCol [keylget table camCol]
	 handleSet $objcIo.field [keylget table field]
      } else {
	 #
	 # get run, field, etc. from some of the TSOBJ rosat fields
	 #
	 global alias_inmem alias_ondisk
	 if ![info exists alias_ondisk] {
	    set_alias_ondisk
	 }
	 
	 loop i 0 [llength $alias_inmem] {
	    set alias [lindex $alias_ondisk $i]
	    set what [lindex $alias_inmem $i]
	    set val [exprGet $objcIo.$alias]
	    
	    if !$is_fpObjc {
	       handleSet $objcIo.$what $val
	    }
	    keylset table $what $val
	    
	    handleSet $objcIo.$alias -1
	 }
      }
   }

   if {![regexp {next|prev} $id] &&
       [exprGet $objcIo.id] != $id && ![info exists index]} {
      echo "Object ID is not the same as its row number;" \
	  "consider using \"openit -index\";\n" \
	  "running \"index_table table -identity\" for you"
      index_table table -identity
   }

   return $objcIo
}

lappend help_procs read_objc

proc read_objc {args} {
   set opts [list \
		 [list [info level 0] "\
 Read the object with the given ID number into an OBJC_IO; the resulting
 handle is returned, and also set as the global \$objcIo"] \
		 [list <table> STRING "" _table "Name of table variable"] \
		 [list <n> STRING "" n "Desired object"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table
   global objcIo openit

   catch {catObjDel $objcIo -deep};	# it may not exist

   set objcIo [objcIoRead table $n]
	 
   return $objcIo
}

#
# Return an object's parent.
#
# If the table is sparse, the object ID doesn't give us the row number,
# and indexing only works reliably within a single run/camCol/field
#
#
proc read_parent {_table o} {
   upvar $_table table
   global OBJECT1 table

   set p [exprGet $o.parent]
   if {$p == -1} {			# no parent
      return $o
   }

   if {([exprGet $o.objc_flags] & $OBJECT1(BLENDED)) && \
	   !([exprGet $o.objc_flags] & $OBJECT1(NODEBLEND))} {
      return $o;			# `parent' is BRIGHT sibling
   }
   #
   # We are a child. Look for parent
   #
   set id [exprGet $o.id]
   return [read_objc table prev:[expr $id - $p - 1]]

   error "I cannot find the parent, ID $p"
}


##############################################################################
#
# regDel, allowing for SPANMASKs
#
if {[info command shRegDel] == ""} {
   rename regDel shRegDel

   proc regDel {reg} {
      if {[exprGet (int)($reg).mask] != 0 &&
	  [exprGet ((SPANMASK*)($reg).mask)->cookie] == 0xc00cee} {
	 set sm [handleBindNew [exprGet ($reg).mask] SPANMASK]
	 spanmaskDel $sm
	 handleSet $reg.mask 0x0
      }
      shRegDel $reg
   }
}

##############################################################################
#
# A utility routine to display regions and their masks (SPANMASKs or MASKs)
#
if ![info exists saoflags] {
   if [info exists env(DS9_DIR)] {
      set saoflags "-linear"
   } else {
      set saoflags "-histeq -lowerleft"
   }
}
proc display_region { reg {large 0} {mask 1} {disp 1} } {
   global saoflags

   if $large {
      set opts "-geometry 1028x929+1+1"
   } else {
      set opts "-geometry +1+1"
   }
   set opts "$opts $saoflags"

   while {[lsearch [saoGetRegion] $disp] < 0} {
      set treg [regNew 1 1]; saoDisplay $treg $opts; regDel $treg
   }
   saoDisplay $reg $disp $opts

   if {$mask && [exprGet $reg.mask] != 0x0} {
      set rmask [handleBindFromHandle [handleNew] *$reg.mask]
      if [catch { display_mask $rmask $disp } msg] {
	 echo $msg
      }
      handleDel $rmask
   }

   return $reg
}

lappend help_procs display_mask

proc display_mask {args} {
   set opts [list \
		 [list [info level 0] "\
 Display the SPANMASK previously read in with get_mask. Mask size must match
 that of the region already displayed.

 The possible mask colours are given by \$saoMaskColors, and their mapping
 to bitplanes by \$mask_color"] \
		 [list <rmask> STRING "" rmask "SPANMASK to display"] \
		 [list <fsao> INTEGER 1 disp "FSAO image number"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   global MASK_TYPE saoMaskColors mask_color

   saoMaskColorSet $saoMaskColors
   
   if {[exprGet ((SPANMASK*)&$rmask)->cookie] == 0xc00cee} {
      set sm [handleBindFromHandle [handleNew] *(SPANMASK*)&$rmask]
      set mask [maskNew [exprGet $sm.nrow] [exprGet $sm.ncol]]
      set i 0
      foreach mt [array names MASK_TYPE] {
         if ![keylget mask_color $mt i] {
	    continue;
	 }
	 
	 set i [lsearch $saoMaskColors $i]
	 if {$i < 0} {
	    continue
	 }
	 
	 set m [handleBindFromHandle [handleNew] *$sm.masks<$MASK_TYPE($mt)>]
	 maskSetFromObjmaskChain $m $mask [expr 1<<$i]
	 handleDel $m
	 
	 incr i
      }
      handleDel $sm
   } else {
      set mask $rmask
   }
   if [catch {
      saoMaskDisplay $mask -p bitset -s $disp -reverse
   }] {
      saoMaskDisplay $mask -p bitset -s $disp; # fsao doesn't understand -reverse
   }

   if {$mask != $rmask} { maskDel $mask }
}

##############################################################################
set gutter 3;				# width of gutter for mtv_objc

#
# get the size of an atlas image (passed as an (ATLAS_IMAGE *))
#
proc get_aisize {aimage _nrow _ncol {_rmin ""} {_cmin ""} {filter ""}} {
   upvar $_nrow nrow  $_ncol ncol
   if {$_rmin != ""} {
      upvar $_rmin rmin
   }
   if {$_cmin != ""} {
      upvar $_cmin cmin
   }

   set aimage ($aimage);		# may be an expression

   if {[exprGet $aimage->npix] == 0} {
      set nrow 0; set ncol 0; set rmin 0; set cmin 0
      return
   }

   set mm [handleBindFromHandle [handleNew] *$aimage->master_mask]
   set rmin [exprGet $mm.rmin]
   set cmin [exprGet $mm.cmin]
   set nrow [expr [exprGet $mm.rmax]-$rmin+1]
   set ncol [expr [exprGet $mm.cmax]-$cmin+1]
   handleDel $mm

   if {$filter != ""} {
      set rmin [expr $rmin + [exprGet $aimage->drow<$filter>]]
      set cmin [expr $cmin + [exprGet $aimage->dcol<$filter>]]
   }
}

#
# Decide how to lay out the images in ncolor bands
#
proc get_mosaic_geometry {nreg _nr _nc {rsize 1} {csize 1}} {
   upvar $_nr nr $_nc nc

   set nc 1; set nr 1
   while {[expr $nr*$nc] < $nreg} {
      if {[expr $nc*$csize] > [expr $nr*$rsize]} {
	 incr nr
      } else {
	 incr nc
      }
   }
   if {[expr $nr*($nc - 1)] >= $nreg} { incr nc -1 }
   if {[expr ($nr - 1)*$nc] >= $nreg} { incr nr -1 }
}

#
# Given the number of bands, decide where to put the index'th image
#
proc get_mosaic_index {nr nc which _ir _ic} {
   upvar $_ir ir $_ic ic

   set ir [expr int($which/$nc)]
   set ic [expr $which - $ir*$nc]
}

#
# Return a mosaic of colours $color0...$ncolor from objc (an OBJC or OBJC_IO)
#
# If compar is a region, it'll be included (shifted to the proper centre
# if there's only one colour and the global recenter_comparison doesn't
# exist and equal 0).  If <compar> isn't a region, but is a proc, that
# proc will be called, with argument <objc>, and should return a region,
# which will be deleted after use.
#
# If the global recenter_atlas_images is true, the atlas images will
# be shifted in each band so that their pixels lie at the center of a pixel
#
if ![info exists recenter_atlas_images] {
   set recenter_atlas_images 0
}

proc get_objc_mosaic {args} {
   set const 0;

   set opts [list \
		 [list [info level 0] \
		      "Return a mosaic of colours color0...ncolor from objc (an OBJC or OBJC_IO)"] \
		 [list <objc> STRING "" objc "The OBJC in question"] \
		 [list <color0> INTEGER "" color0 "Starting colour"] \
		 [list {[ncolor]} INTEGER -1 ncolor "Number of colours (default: all)"] \
		 [list {[mask]} INTEGER 1 mask "Include mask?"] \
		 [list {[compar]} STRING "" compar "Region or proc to provide comparison image"] \
		 [list -binfac INTEGER 1 binfac "How much to bin output region"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }


      
   global gutter recenter_atlas_images
   global fieldparams OBJECT1

   if {$ncolor < 0} {
      set ncolor [expr [exprGet $objc.ncolor] - $color0]
   }

   get_aisize $objc.aimage nrow ncol
   if {$nrow == 0} {
      return ""
   }

   if {$compar != ""} {			# we've got an extra image
      incr ncolor
   }
   
   get_mosaic_geometry $ncolor nr nc

   set full_nrow [expr $nr*($nrow+$gutter)-$gutter]
   set full_ncol [expr $nc*($ncol+$gutter)-$gutter]
   set reg [regNew $full_nrow $full_ncol -mask]

   regIntSetVal $reg [softBiasGet]

   loop i 0 $ncolor {
      set c [expr $color0+$i]

      if {$compar == "" || $i < $ncolor - 1} {
	 get_aisize $objc.aimage nrow ncol rmin cmin $c
      }

      get_mosaic_index $nr $nc $i ir ic
      
      set r0 [expr $rmin-$ir*($nrow+$gutter)]
      set c0 [expr $cmin-$ic*($ncol+$gutter)]

      if {$compar != "" && $i == $ncolor - 1} {
	 set r0 [expr $ir*($nrow+$gutter)]
	 set c0 [expr $ic*($ncol+$gutter)]

	 if {![regexp {^h[0-9][0-9]} $compar] && [info command $compar]} {
	    set delete_compar 1
	    set compar [eval $compar $objc]
	 }
	 set cnrow [exprGet $compar.nrow]
	 set cncol [exprGet $compar.ncol]
	 #
	 # If there's only one object in the main mosiac, shift the
	 # comparison to the same centre
	 #
	 global recenter_comparison
	 if {$ncolor == 2 &&
	     (![info exists recenter_comparison] || $recenter_comparison)} {
	    set clist [objectCenterFind $compar \
			   [expr $cnrow/2] [expr $cncol/2] \
			   0 -bkgd [softBiasGet] -sigma 1]
	    set rowc [keylget clist rowc]; set colc [keylget clist colc]
	    set dr [exprGet $objc.rowc<$color0>]; set dr [expr $dr - int($dr)]
	    set dc [exprGet $objc.colc<$color0>]; set dc [expr $dc - int($dc)]

	    set scompar [regNew $cnrow $cncol]
	    regIntShift $compar -out $scompar \
		[expr $cnrow/2 + $dr - $rowc] [expr $cncol/2 + $dc - $colc]
	    set compar $scompar
	 }
	 #
	 # And copy comparison object to region
	 #	 
	 set sreg [subRegNew $reg $nrow $ncol $r0 $c0]

	 set cmp [regNew $nrow $ncol]; regIntSetVal $cmp [softBiasGet]
         if {$cnrow > $nrow} {
            if {$cncol > $ncol} {
               set tmp [subRegNew $compar $nrow $ncol \
                            [expr int(0.5*($cnrow-$nrow))] \
                            [expr int(0.5*($cncol-$ncol))]]
               regIntCopy $cmp $tmp
            } else {
               set tmp [subRegNew $compar $nrow $cncol \
                            [expr int(0.5*($cnrow-$nrow))] 0]
               set ctmp [subRegNew $cmp $nrow $cncol 0 0]
               regIntCopy $ctmp $tmp; regDel $ctmp
            }
         } else {
            if {$cncol > $ncol} {
               set tmp [subRegNew $compar $cnrow $ncol \
                            0 [expr int(0.5*($cncol-$ncol))]]
               set ctmp [subRegNew $cmp $cnrow $ncol 0 0]
               regIntCopy $ctmp $tmp; regDel $ctmp
            } else {
               set tmp [subRegNew $cmp $cnrow $cncol \
			    [expr int(0.5*($nrow-$cnrow))] \
			    [expr int(0.5*($ncol-$cncol))]]
               regIntCopy $tmp $compar
            }
         }
	 #
	 # Adjust flux in comparison to roughly equal real object.
	 # Assume that the u and z fluxes are small, and make some
	 # allowance (if they are included in the composite)
	 #
	 set ndiv [expr $ncolor-1];	# don't include this object
	 if {$color0 == 0 && $ndiv > 1} {
	    set ndiv [expr $ndiv-1];	# u' hardly contributes
	 }
	 if {[expr $color0 + $ncolor] == 5 && $ndiv > 1} {
	    set ndiv [expr $ndiv-0.5];	# z' isn't much better
	 }
	 
	 regStatsFromQuartiles $reg -counts regcts;
	 set regcts [expr ($regcts - $full_nrow*$full_ncol*[softBiasGet].0)/$ndiv]
	 regStatsFromQuartiles $cmp -counts cts;
	 set cts [expr $cts - $nrow*$ncol*[softBiasGet]]
	 set frac [expr ($regcts+0.0)/$cts]
	 #
	 # draw a boundary for $compar
	 #
	 set rb [expr $r0 > 0 ? 1 : 0]
	 set cb [expr $c0 > 0 ? 1 : 0]
	 set breg [subRegNew $reg \
	       [expr $nrow+$rb] [expr $ncol+$cb] [expr $r0-$rb] [expr $c0-$cb]]
	 regIntSetVal $breg 0
	 regDel $breg
         #
         # and insert $compar into the mosaic
         #
         if {$frac < 0} {		# something's wrong
	    set frac 0.1
	 }
         regIntLincom $sreg $cmp [expr (1-$frac)*[softBiasGet]] 0 $frac

	 if {[set rand [lindex [handleListFromType RANDOM] 0]] == ""} {
	    set rand [phRandomNew 100000:2]
	 }
	 regIntNoiseAdd $sreg $rand 800 5
	 
         regDel $tmp; regDel $sreg; regDel $cmp
	 if [info exists scompar] {
	    regDel $scompar;		# we're done with it
	 }
	 if [info exists delete_compar] {
	    regDel $compar
	 }
      } else {
	 set sreg [subRegNew $reg $nrow $ncol \
		       [expr $ir*($nrow+$gutter)] \
		       [expr $ic*($ncol+$gutter)] -mask]

	 if {1 && ([exprGet $objc.objc_flags] & $OBJECT1(BRIGHT))} { # bright atlas images aren't sky subtracted
	    global write_bright_atlas_images
	    if ![info exists write_bright_atlas_images] {
	       set write_bright_atlas_images 0; # not usually true
	    }

	    if {!$write_bright_atlas_images && [exprGet $objc.parent] == -1} {
	       set bkgd 0; # the parent atlas image is actually the _faint_ parent
	    } else {
	       set bkgd [exprGet $fieldparams.frame<$c>.global_sky];
	    }
	 } else {
	    set bkgd 0
	 }
	 set clip {}
	 if {$bkgd != 0} {
	    #set clip "-clip"
	 }

	 set rowc [exprGet $objc.objc_rowc]
	 set colc [exprGet $objc.objc_colc]

	 if 0 {				# overwritten by regSetFromAtlasImage
	    if {[set rand [lindex [handleListFromType RANDOM] 0]] == ""} {
	       global rand
	       set rand [phRandomNew 100000:2]
	    }
	    
	    set sigma [expr 256*[binregionInterpolate *$fieldparams.frame<$c>.skyErr \
				     [expr int($rowc)] [expr int($colc)]]]; # assume 256x256 binning XXX

	    eval regSetValFromAtlasImage $sreg $c *$objc.aimage -row0 $bkgd $rmin -col0 $cmin -sigma 4 -rand $rand
	 }
	 eval regSetFromAtlasImage *$objc.aimage $c $sreg -row0 $rmin -col0 $cmin -sky -$bkgd $clip
	 
	 if {[info exists recenter_atlas_images] && $recenter_atlas_images} {
	    if ![info exists fieldparams] {
	       echo "I need a fieldparams to to sub-pixel alignment"
	    } else {
	       set dd [offsetDo $fieldparams $rowc $colc \
			   [exprGet $fieldparams.ref_band_index] $c]
	       set drow [lindex $dd 0]
	       set dcol [lindex $dd 2]

	       set adrow [exprGet $objc.aimage->drow<$c>]
	       set adcol [exprGet $objc.aimage->dcol<$c>]

	       set dr [expr 0.5 - ($rowc - int($rowc))];# center up r'
	       set dc [expr 0.5 - ($colc - int($colc))]

	       set dr [expr $dr + ($adrow - $drow)];# center other bands wrt r'
	       set dc [expr $dc + ($adcol - $dcol)]
	    
	       regIntShift $sreg -out $sreg -filtsize 7 $dr $dc
	    }
	 }
	 
	 if {$mask} {
	    handleSet $sreg.mask->row0 [expr $r0 + $ir*($nrow+$gutter)]
	    handleSet $sreg.mask->col0 [expr $c0 + $ic*($ncol+$gutter)]
	    
	    set om [handleBindFromHandle [handleNew] *$objc.aimage->mask<$c>]
	    
	    maskSetFromObjmask $om *$sreg.mask [expr 1<<2]
	    
	    handleDel $om
	 }
	 
	 regDel $sreg
      }
   }

   if {$binfac > 1} {
      set breg [regBin $reg -both $binfac -normalize 1]
      regDel $reg; set reg $breg
   }

   return $reg
}

lappend help_procs mtv_objc

proc mtv_objc {args} {
   set opts [list \
		 [list [info level 0] "
 Display the atlas image with a given ID number; the <col>-band image is
 shown if <col> is a filtername, otherwise all bands (default). If no table is
 specified, \"table\" is assumed.

 The apertures used to extract profiles may be shown with a subsequent
 apertures_objc command

 If compar is provided, it's a region that should be displayed along with
 the atlas images (e.g. a stellar image)."] \
		 [list {[table]} STRING "table" _table \
		      "Name of table variable, as returned by openit"] \
		 [list {[n]} INTEGER -1 n \
		      "ID of desired object (or -1 for last object read)"] \
		 [list {[mask]} INTEGER 0 mask "Show detection mask"] \
		 [list {[colors]} STRING "all" col "Desired bands"] \
		 [list {[compar]} STRING "" compar "Comparison region"] \
		 [list {-fsao} INTEGER 1 fsao "fsaoimage to use"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table
   global objcIo gutter

   if [catch { saoReset } msg] {
      if [regexp {^Failed to find frame} $msg] {
	 global errorInfo
	 return -code error -errorinfo $errorInfo $msg
      }
   }

   if {$_table == $objcIo} {		# use pre-existing objcIo
      if ![info exists table] {
	 set table [list]
      }
      set tableIsObjc 1
      global filterlist
      set filters $filterlist
   } else {
      set tableIsObjc 0
      set filters [keylget table filters]
   }

   set c [lsearch $filters $col]
   if {$c < 0} {
      set color0 0
      set ncolor [llength $filters]
   } else {
      set color0 $c
      set ncolor 1
   }

   if !$tableIsObjc {
      if {$n > 0} {
	 catch {catObjDel $objcIo -deep};		# it may not exist
      }
      set objcIo [objcIoRead table $n]
   }
   
   set reg [get_objc_mosaic $objcIo $color0 $ncolor $mask $compar]

   if {$reg == ""} {			# no pixel data
      return $objcIo
   }
	 
   display_region $reg 0 $mask $fsao

   if !$tableIsObjc {
      if {[exprGet $objcIo.parent] >= 0} {
	 set flg "-e";			# red
      } else {
	 set flg "-i";			# yellow
      }

      set rcf [get_run_camCol_field table $objcIo]
      saoDrawText -s $fsao \
	  [expr [exprGet $reg.nrow]-2] [expr [exprGet $reg.ncol]-5] \
	  "objc_[lindex $rcf 0]-[lindex $rcf 1]-[lindex $rcf 2]:[exprGet $objcIo.id]" \
	  $flg
   }
      
   global sao_is_ds9
   if !$sao_is_ds9 {
      set ncol [exprGet $reg.ncol]
      if {$ncol > 512} {
	 saoZoom 0.5
      } elseif {$ncol < 256} {
	 if {$ncol < 128} {
	    if {$ncol < 64} {
	       saoZoom 8;
	    } else {
	       saoZoom 4
	    } else {
	       saoZoom 2
	    }
	 }
      }
   }

   regDel $reg

   return $objcIo
}

proc mtv_objc_debug {args} {
   set opts [list \
		 [list [info level 0] "Display an objc, prompting the user"] \
		 [list <obj> STRING "" obj "The object to display"] \
		 [list -fsao INTEGER 1 fsao "Desired fsaoimage"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   global skip_mtv_objc
   if ![info exists skip_mtv_objc] {
      set skip_mtv_objc 0
   }
   if {$skip_mtv_objc < 0} {
      ;
   } elseif {$skip_mtv_objc > 0} {
      incr skip_mtv_objc -1
   } else {
      if {[handleType $obj] == "OBJC"} {
	 global objcIo
	 set objcIo [objcIoNewFromObjc $obj]
	 mtv_objc -fsao $fsao $objcIo
	 objcIoDel $objcIo; unset objcIo
      } else {
	 mtv_objc -fsao $fsao $obj
      }
      puts -nonewline "Next [exprGet $obj.id] (<cr>nq\#): "; # next, quit, skip # objects
      set reply [gets stdin]
      if {$reply == "q"} {
	 error "$obj"
      } elseif {[regexp {^-?[0-9]+$} $reply]} {
	 set skip_mtv_objc $reply
	 if {$skip_mtv_objc < 0} {
	    echo "OK, I shan't show you any more objects"
	 } else {
	    echo "skipping $skip_mtv_objc objcs"
	 }
      }
   }
}

###############################################################################
#
# Usage: get_objcIo_blend_mosaic \[table\] \[n\] \[col\] \[peak_list\] \[mask\]
#
# return a mosaic of the <n>th OBJC_IO in a table and all of its children.
# <col> is the colour to use (default: all), and \[mask\] indicates whether
# the mask bits should be set. If <n> is omitted, use the last object
# read from an output table, which defaults to "table".
#
# As a special case if <col> is upper case (think, "Grown Up"), no children
# will be displayed; e.g. <col> == R will display r' parents/non-blends
#
# See mtv_blend to display the mosaic, or mtv_mosaic_blend to display
# a mosaic of blends
#
proc get_objcIo_blend_mosaic {_table n {col all} {_peaks ""} {mask 0} {compar ""} {_mosaic_geom ""} {sel_merge ""} {binfac 1}} {
   if {$_mosaic_geom != ""} {
      upvar $_mosaic_geom mosaic_geom
   }

   upvar $_table table
   if {$_peaks != ""} {
      upvar $_peaks peaks
   }
   global gutter objcIo OBJECT1

   if {[regexp {^[UGRIZ]*$} $col]} {
      set no_children 1
      set col [string tolower $col]
   }
   set c [lsearch [keylget table filters] $col]

   if {$c < 0} {
      set color0 0
      set ncolor [keylget table ncolor]
   } else {
      set color0 $c
      set ncolor 1
   }

   if {$n >= 0 && [info exists objcIo]} {
      catObjDel $objcIo -deep
   }
   set objcIo [objcIoRead table $n]; set obj(0) $objcIo
   set mosaics(0) [get_objc_mosaic $objcIo $color0 $ncolor $mask $compar -binfac $binfac]
   set id [exprGet $objcIo.id]
   set parent [exprGet $objcIo.parent]

   if ![info exists no_children] {
      set mi -1;			# if sel_merge, merge into this one
      set ii 0;				# will be incr
      set brightParent [expr [exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)]
      set brightChildren 0;	# until we know better

      set nchild [exprGet $objcIo.nchild]
      loop i 1 [expr $nchild + 1] {
	 incr ii
	 
	 if [catch { set objcIo [objcIoRead table next] } msg] {
	    break;
	 }
	 #
	 # If parent is bright, only show bright children
	 #
	 if {$brightParent && $i == 1} {
	    if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
	       get_aisize $objcIo.aimage nrow ncol
		if {$nrow*$ncol > 0} {
		   set brightChildren 1
		}
	    }
	 }
	 if {$brightChildren && !([exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT))} {
	    set nchild $i
	    break
	 }
	 
	 set mosaics($ii) \
	     [get_objc_mosaic $objcIo $color0 $ncolor $mask $compar -binfac $binfac]
	 set obj($ii) $objcIo
	 #
	 # Did they want us to merge some of those children?
	 #
	 global default_sel_merge
	 if [info exists default_sel_merge] {
	    set sel_merge $default_sel_merge
	 }
	 
	 if {$sel_merge != ""} {
	    set verbose 0
	    if 0 {
	       set other obj(0)
	    } else {
	       global fieldparams
	       set other $fieldparams
	    }
	    if [sel_merge $obj($ii) $other] {
	       if {$mi < 0} {
		  set mi $ii
		  if $verbose {
		     echo Merged object is $mi
		  }
	       } else {
		  if $verbose {
		     echo Merging $i into $mi
		  }
		  regIntLincom $mosaics($mi) $mosaics($ii) -[softBiasGet] 1 1

		  regDel $mosaics($ii)
		  unset mosaics($ii)
		  catObjDel $obj($ii)
		  
		  incr ii -1
	       }
	    }
	 }
      }
      if {$nchild > 0} {
         objcIoRead table prev:[expr $nchild - 1]
      }
   }
   #
   # OK, we now have a tcl array mosaics of regions. Mosaic _them_
   #
   if 0 {				# RHL
      foreach reg [lsort -integer [array names mosaics]] {
	 mtv $mosaics($reg); puts -nonewline "$reg: "; gets stdin
      }
   }
   
   set csize 0; set rsize 0
   set nreg [llength [array names mosaics]]
   loop i 0 $nreg {
      if {$mosaics($i) != ""} {
	 set ncsize [exprGet $mosaics($i).ncol]
	 set nrsize [exprGet $mosaics($i).nrow]

	 if {$ncsize > $csize} { set csize $ncsize };# Can be true if using deblend_as_stellar
	 if {$nrsize > $rsize} { set rsize $nrsize }
      }
   }
   if {$csize*$rsize == 0} {
      set msg "No regions appear to be present in mosaics array"
      if 0 {
	 error $msg
      } else {
	 echo $msg
	 set rsize 1; set csize 1
	 set peaks {}
      }
   }
   
   get_mosaic_geometry $nreg nr nc $rsize $csize

   set ncol [expr $nc*$csize+($nc-1)*$gutter]
   set nrow [expr $nr*$rsize+($nr-1)*$gutter]

   if {$nrow*$ncol < 0 || $nrow*$ncol > 100*1024*1024} {	# shMalloc may fail if asked for 100M
      ;					# in addition to sub-mosaics
      foreach i [array names mosaics] {
	 if {$mosaics($i) != ""} {
	    regDel $mosaics($i)
	 }
	 catObjDel $obj($i)
      }

      echo "Request for too large a region: ${nrow}x$ncol"
      set peaks [list]
      return [regNew 1 1]
   }

   set mosaic_geom [list $nreg $nr $nc $rsize $csize $gutter $nrow $ncol]

   if $mask { set maskflg -mask } else { set maskflg "" }
   set reg [eval regNew $nrow $ncol $maskflg]
   regIntSetVal $reg 10

   loop i 0 [expr $nr*$nc] {
      set ir [expr int($i/$nc)]
      set ic [expr "$i - $nc*$ir"]
      set r0 [expr $ir*($rsize+$gutter)]
      set c0 [expr $ic*($csize+$gutter)]

      set tmp [subRegNew $reg $rsize $csize $r0 $c0]
      regIntSetVal $tmp [softBiasGet]; regDel $tmp

      if {![info exists mosaics($i)] ||
	  $mosaics($i) == ""} {		# no pixel data
	 if {$i > 0 && [info exists obj($i)]} {
	    catObjDel $obj($i) -deep
	 }
	 
	 continue;
      }

      set tmp \
	  [subRegNew $reg \
	       [exprGet $mosaics($i).nrow] [exprGet $mosaics($i).ncol] $r0 $c0]
      regIntCopy $tmp $mosaics($i)
      regDel $tmp
      if $mask {
	 set tmp \
	  [subMaskNew *$reg.mask \
	       [exprGet $mosaics($i).nrow] [exprGet $mosaics($i).ncol] $r0 $c0]
	 maskCopy *$mosaics($i).mask $tmp
	 maskDel $tmp
      }

      if {$_peaks != ""} {
	 set npeaks [set_peaks table $obj($i) $r0 $c0 $col]
	 if [info exists peaks] {
	    set peaks [concat $peaks $npeaks]
	 } else {
	    set peaks $npeaks
	 }
      }

      if {$i > 0} {
	 catObjDel $obj($i) -deep
      }
      regDel $mosaics($i)
   }

   set objcIo $obj(0)

   return $reg
}

###############################################################################

ftclHelpDefine phOutputs mtv_blend \
    "
Usage: mtv_blend \[table\] \[n\] \[mask\] \[col\] \[regName\] \[compar\]

Show a mosaic of the <n>th OBJC_IO in a table and all of its children.
<col> is the colour to use (default: all), \[mask\] indicates whether
the mask bits should be displayed (default 0).

If <n> is omitted, use the last object read from an output table, which
defaults to \"table\". 

If <regName> is provided, the mosaic REGION will be available to the caller
as \$regName.

If <compar> is provided, it's either a region, or the name of a procedure
that returns one. The region will be suitably scaled and included in the
mosaic. If <compar>'s a proc, the region will be deleted after use.

See mtv_objc_blend if you want to look at an OBJC, or mtv_mosaic_blend
to display a mosaic of blends read back from OBJC_IO tables
"

proc mtv_blend {{_table table} {n -1} {mask 0} {col all} {_reg ""} {compar ""}} {
   upvar $_table table
   if {$_reg != ""} { upvar $_reg reg }
   global gutter objcIo peaks

   catch {saoReset}
   if [info exists peaks] { unset peaks }
   set reg [get_objcIo_blend_mosaic table $n $col peaks $mask $compar]

   display_region $reg 0 $mask
   show_peaks_list $peaks
   
   if {[exprGet $objcIo.parent] >= 0} {
      set flg "-e"
   } else {				# red
      set flg "-i";			# yellow
   }
   
   set rcf [get_run_camCol_field table $objcIo]
   set run [lindex $rcf 0]
   set camCol [lindex $rcf 1]
   set field [lindex $rcf 2]

   set id [exprGet $objcIo.id]
   saoDrawText \
       [expr [exprGet $reg.nrow]-2] [expr [exprGet $reg.ncol]-25] \
       "objc_$run-$camCol-$field:[exprGet $objcIo.id]" $flg

   global sao_is_ds9
   if !$sao_is_ds9 {
      if {[exprGet $reg.ncol] < 128 && [exprGet $reg.nrow] < 128} {
	 saoZoom 4
      } elseif {[exprGet $reg.ncol] < 256 && [exprGet $reg.nrow] < 256} {
	 saoZoom 2
      }
   }

   if {$_reg == ""} {
      regDel $reg
   }

   return $objcIo
}

###############################################################################
#
# Expand a logical expression of select procs,
# e.g. sel_cr||(sel_star&&!sel_galaxy)
#
proc expand_logical_select {select {procname __select}} {
   if {"$select" == "" || ![regexp {[()&|!]} $select]} {
      if {[info proc $select] == "" && [info proc sel_$select] != ""} {
	 set select sel_$select
      }
      return $select
   }

   set oselect $select

   regsub -all {&&} $select "\\&" select 
   regsub -all {\|\|} $select "|" select
   regsub -all {([()&!|])} $select { \1 } select_expr
   
   set select "\
 proc $procname {o {b -1}} {
   if { "
   foreach el [split $select_expr] {
      if {$el == "" || [regexp {[()!]} $el] || [regexp {^[0-9]+$} $el]} {
	 append select " $el"
      } elseif [regexp {[&|]} $el] {
	 append select "${el}$el"
      } else {
	 if {[info proc $el] == ""} {
	    if {[info proc sel_$el] != ""} {
	       set el sel_$el
	    } else {
	       error "No such proc: $el ($oselect)"
	    }
	 }
	 append select " \[$el \$o \$b\] "
      }
   }
   append select "\
   } {
      return 1;
    } else {
      return 0;
    }
 }
 ";
   eval $select;
   
   return $procname
}

###############################################################################

ftclHelpDefine phOutputs mtv_mosaic_blend \
    "
Usage: mtv_mosaic_blend table size_r size_c \[select\] \[col\] \[n1\] \[n2\] \[fsao\]

The objects are mosaiced assuming a size of \$size_r*\$size_c for each. The
array ids is set, so that you can overlay the atlas images' ids with the
proc mosaic_id. Only the band specified by <col> is shown (default: all);
As a special case if <col> is upper case (think, \"Grown Up\"), no children
will be displayed; e.g. <col> == R will display r' parents/non-blends.

If <select> is provided, provided, it's the name of a procedure used
to select objects; for details say \"help select\".

If <save> is true, the region containing the mosaic will be returned,
otherwise it's deleted after being displayed.
"
alias mtv_blend_mosaic mtv_mosaic_blend;# old name for command

proc mtv_mosaic_blend {_table size_r size_c {select ""} {col all} {n1 1} {n2 -1} {fsao 1} {save 0}} {
   upvar $_table table
   global objcIo gutter OBJECT1

   set c [lsearch [keylget table filters] [string tolower $col]]
   if {$c == -1 && ![regexp {all|ALL} $col]} {
      error "$col is not a valid filtername"
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }

   upvar ids ids; catch { unset ids }
   set n 0
   loop i $n1 [expr $n2+1] {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $i]
      
      if {"$select" != "" && ![eval $select $objcIo]} {
	 continue;
      }

      set ids($n) $i
      incr n
   }
   if {$n == 0} {
      error "No objects are selected"
   }
      
   get_mosaic_geometry $n nr nc $size_r $size_c
   
   set ncol [expr $nc*$size_c+($nc-1)*$gutter]
   set nrow [expr $nr*$size_r+($nr-1)*$gutter]
   set reg [regNew $nrow $ncol]

   regIntSetVal $reg [expr [softBiasGet] + 50]
   loop ir 0 $nr {
      loop ic 0 $nc {
	 set r0 [expr $ir*($size_r+$gutter)]
	 set c0 [expr $ic*($size_c+$gutter)]
	 
	 set subreg [subRegNew $reg $size_r $size_c $r0 $c0]
	 regIntSetVal $subreg 0
	 
	 regDel $subreg
      }
   }

   set ids(nc) $nc; set ids(size_r) $size_r; set ids(size_c) $size_c;
   set ids(gutter) $gutter
   loop i 0 $n {
      set ir [expr int($i/$nc)]
      set ic [expr "$i - $nc*$ir"]
      set r0 [expr $ir*($size_r+$gutter)]
      set c0 [expr $ic*($size_c+$gutter)]

      set mosaic [get_objcIo_blend_mosaic table $ids($i) $col peaks($i)]
      
      if {[set rsize [exprGet $mosaic.nrow]] > $size_r} {
	 set rsize $size_r
      } 
      if {[set csize [exprGet $mosaic.ncol]] > $size_c} {
	 set csize $size_c
      } 

      set submos [subRegNew $mosaic $rsize $csize 0 0]
      set subreg [subRegNew $reg $rsize $csize $r0 $c0]
      regIntCopy $subreg $submos

      regDel $subreg; regDel $submos; regDel $mosaic
   }
   #
   # Actually do the display
   #
   catch {saoReset $fsao}
   display_region $reg 0 1 $fsao
   saoDrawCache
   loop i 0 $n {
      set ir [expr int($i/$nc)]
      set ic [expr "$i - $nc*$ir"]
      set r0 [expr $ir*($size_r+$gutter)]
      set c0 [expr $ic*($size_c+$gutter)]
      foreach p $peaks($i) {
	 set r [lindex $p 2]; set c [lindex $p 3]
	 if {$r < $size_r && $c < $size_c} {
	    draw_cross [lindex $p 1] [expr $r0+$r] [expr $c0+$c] \
		"[lindex $p 4] -s $fsao"
	 }
      }
   }
   saoDrawCache -off

   if $save {
      return $reg
   } else {
      regDel $reg
   }
}

###############################################################################
#
# Return a mosaic of an OBJC and its children
#
ftclHelpDefine phOutputs mtv_objc_blend \
    "
Usage: mtv_objc_blend <objc> \[color0\] \[ncolor\] \[peaks\]

Return a mosaic of an object <objc> and all of its children. \[color0\] is the
colour to start with (default: 0), \[ncolor\] the number of colours to display
(default: all).

See also display_mtv_objc_blend to display the region and associated peaks list
"

proc mtv_objc_blend {objc {color0 0} {ncolor -1} {_peaks ""}} {
   global gutter OBJECT1

   if {$_peaks != ""} {
      upvar $_peaks peaks
      catch {unset peaks}
   }

   if {$ncolor < 0} {
      set ncolor [expr [exprGet $objc.ncolor] - $color0]
   }
   
   set i -1

   set obj(0) $objc
   set mosaics([incr i]) [get_objc_mosaic $objc $color0 $ncolor]

   handleDel [objcDescendentNext $objc]
   while {[set child [objcDescendentNext NULL]] != ""} {
      set mosaics([incr i]) [get_objc_mosaic $child $color0 $ncolor]
      set obj($i) $child
   }
   #
   # OK, we now have a tcl array mosaics of regions. Mosaic _them_
   #
   set nreg [llength [array names mosaics]]
   set csize [exprGet $mosaics(0).ncol]
   set rsize [exprGet $mosaics(0).nrow]
   
   get_mosaic_geometry $nreg nr nc $rsize $csize

   set ncol [expr $nc*$csize+($nc-1)*$gutter]
   set nrow [expr $nr*$rsize+($nr-1)*$gutter]

   set reg [regNew $nrow $ncol]
   regIntSetVal $reg 10

   loop i 0 [expr $nr*$nc] {
      set ir [expr int($i/$nc)]
      set ic [expr "$i - $nc*$ir"]
      set r0 [expr $ir*($rsize+$gutter)]
      set c0 [expr $ic*($csize+$gutter)]

      set tmp [subRegNew $reg $rsize $csize $r0 $c0]
      regIntSetVal $tmp [softBiasGet]; regDel $tmp
      
      if {![info exists mosaics($i) ||
	    $mosaics($i) == ""} {	# no pixel data
	 continue;
      }

      set tmp \
	  [subRegNew $reg \
	       [exprGet $mosaics($i).nrow] [exprGet $mosaics($i).ncol] $r0 $c0]
      regIntCopy $tmp $mosaics($i)
      regDel $tmp

      set c 0
      loop j 1 $nreg {
	 set obj1 [handleBindFromHandle [handleNew] *$obj($j).color<$c>]
	 set om [handleBindFromHandle [handleNew] *$objc.aimage->mask<$c>]

	 set rowc [expr [exprGet ($obj1).rowc] - [exprGet $om.rmin] + $r0]
	 set colc [expr [exprGet ($obj1).colc] - [exprGet $om.cmin] + $c0]
	 handleDel $om

	 if {$_peaks != ""} {
	    if {[expr [exprGet ($obj1).flags]&$OBJECT1(DEBLENDED_AS_PSF)]} {
	       set type psf
	    } else {
	       set type other
	    }
	    if {$i == 0} {		# parent image
	       set color "y"
	    } elseif {$i == $j} {	# primary peak for this panel
	       set color "y"
	    } else {
	       set color "r"
	    }

	    lappend peaks "$rowc $colc $color $type"
	 }
	 handleDel $obj1
      }

      regDel $mosaics($i)
   }
   loop i 1 $nreg {			# n.b. don't delete obj(0)
      handleDel $obj($i)
   }
   
   return $reg
}

ftclHelpDefine phOutputs display_mtv_objc_blend \
    "
Usage: display_mtv_objc_blend <reg> <peaks> \[mask\]

Display a region and peaks list returned by display mosaic, e.g.
	set reg \[mtv_objc_blend \$objc 0 -1 peaks\]
        display_mtv_objc_blend \$reg peaks

The positions of all children are shown by crosses; in each panel the
child is indicated in yellow, and all other centres in red. Objects
that the deblender considered as PSFs are drawn as x's; otherwise +'s are used
"

proc display_mtv_objc_blend {reg _peaks {mask 0}} {
   upvar $_peaks peaks

   display_region $reg 0 $mask

   if {![info exists peaks]} {
      return $reg
   }
   
   saoReset
   foreach p $peaks {
      set rowc [lindex $p 0]
      set colc [lindex $p 1]
      set color [lindex $p 2]
      if {$color == "r"} {
	 set flg "-e"
      } elseif {$color == "y"} {
	 set flg "-i"
      } else {
	 error "Unknown colour: $color"
      }

      if {[lindex $p 3] == "psf"} {
	 saoDrawPolygon \
	     [expr $rowc-2] [expr $colc-2] [expr $rowc+2] [expr $colc+2] \
	     $flg
	 saoDrawPolygon \
	     [expr $rowc-2] [expr $colc+2] [expr $rowc+2] [expr $colc-2] \
	     $flg
      } else {
	 saoDrawPolygon \
	     [expr $rowc-2] $colc [expr $rowc+2] $colc $flg
	 saoDrawPolygon \
	     $rowc [expr $colc-2] $rowc [expr $colc+2] $flg
      }
   }

   return $reg
}   

lappend help_procs mtv_objc_list
lappend help_procs mtv_list

alias mtv_list mtv_objc_list

proc mtv_objc_list {args} {
   set show_family 0
   set nodisplay 0
   set opts [list \
		 [list [info level 0] "\
    Display the atlas images for all objects in <table> (or all in
 (<n1>..<n2> inclusive if specified). If <select> is provided, it's
 the name of a procedure; for details say \"help select\". You'll
 be prompted after each object; hit <CR> to return, or <h> for help.

 If your response isn't recognised, a proc mtv_objc_list_proc_? will be
 called with your response as arguments; \"?\" is the first word of
 your response. See mtv_objc_list_proc_V as an example.  If the second
 argument is \"help\", the proc should simply return a (short) help string.

 If no table is supplied it'll default to \"table\".

 If <post_proc> is provided, it's the name of a proc expecting a single
 argument, a table, to be executed after the image is displayed
 (e.g. you could use show_peaks).  As a special case, if it's \"mtv_blend\",
 the command mtv_blend rather than mtv_objc will be used to display
 each object selected (also see -children); if it's \"nodisplay\", the
 object won't be shown (but you can use \"d\" to display it later)
 "] \
		 [list {[table]} STRING "table" _table \
		      "name of table list variable (as set by openit)"] \
		 [list {[select]} STRING "" select "Selection function"] \
		 [list {[post_proc]} STRING "" post_proc \
		      "Function to call after image is displayed"] \
		 [list {[n1]} INTEGER 1 n1 "first object to display"] \
		 [list {[n2]} INTEGER -1 n2 "last object to display"] \
		 [list -select STRING "" _select "Selection function"] \
		 [list -post_proc STRING "" _post_proc \
		      "Function to call after image is displayed"] \
		 [list -children CONSTANT 1 show_family \
		      "Show blends' families"] \
		 [list -nodisplay CONSTANT 1 nodisplay \
		      "Don't display objects (but you can use \"d\")"] \
		 [list -n1 INTEGER 1 _n1 "first object to display"] \
		 [list -n2 INTEGER -1 _n2 "last object to display"] \
		 [list -id STRING "" id "ID or rowc,colc of desired object"] \
		 ]
   #
   # Allow the user to omit "-id" and table.
   #
   # That is, the forms
   #   mtv_objc_list id
   #   mtv_objc_list <rowc>,<colc>
   #   mtv_objc_list <rowc> <colc>
   #   mtv_objc_list <ra>,<dec> ra
   #   mtv_objc_list <ra> <dec> ra
   # should all be accepted
   #
   if {[regexp {^[-+]?[0-9]+(\.[0-9]*)?$} [lindex $args 0]] &&
       [regexp {^[-+]?[0-9]+(\.[0-9]*)?$} [lindex $args 1]]} {
      set args [lreplace $args 0 1 [lindex $args 0],[lindex $args 1]]
   }
   
   global ra_regexp
   set ra_regexp {((ra|RA)(dec|Dec)?)|(eq|Eq|[Jj]2000)$}
   if [regexp $ra_regexp [lindex $args 1]] {
      set args [lreplace $args 0 1 "[lindex $args 0]:[lindex $args 1]"]
   }
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # Allow the user to omit "-id" and table.
   #
   # That is, the forms
   #   mtv_objc_list id
   #   mtv_objc_list <rowc>,<colc>
   #   mtv_objc_list <rowc> <colc>
   #   mtv_objc_list <ra>,<dec> ra
   #   mtv_objc_list <ra> <dec> ra
   # should all be accepted
   #
   if [regexp {^[-+]?[0-9]+(\.[0-9]*)?([,: ][-+]?[0-9]+(\.[0-9]*)?)?} $_table] {
      if {$id == ""} {
	 set id $_table; set _table table
      }
   }

   if {$_n1 != 1} {
      if {$n1 != 1} {
	 error "You may not specify both \[n1\] and -n1"
      }
      set n1 $_n1
   }
   if {$_n2 != -1} {
      if {$n2 != -1} {
	 error "You may not specify both \[n2\] and -n2"
      }
      set n2 $_n2
   }

   if {$_post_proc != ""} {
      if {$post_proc != ""} {
	 error "You may not specify both \[post_proc\] and -post_proc"
      }
      set post_proc $_post_proc
   }

   if [regexp {^nodisp} $post_proc] {
      set nodisplay 1
   }

   if {$_select != ""} {
      if {$select != ""} {
	 if ![regexp {[-+][0-9]+(\.[0-9]*)?$} $_select] {
	    error "You may not specify both \[select\] and -select ($select, $_select)"
	 }
      } else {
	 set select $_select
      }
   }

   if {$id != ""} {
      if {$n1 != 1 || $n2 != -1} {
	 error "You may not specify n1 or n2 in addition to -id"
      }
      set n1 $id
   }

   upvar $_table table
   global objcIo OBJECT1 phDefault_elements phDefault_mtv_list_s

   if ![info exists phDefault_mtv_list_s] {
      set phDefault_mtv_list_s "pan center erase ids"
   }

   if {$n1 == 1 && $select == "sel_list"} {
      global obj_list
      if [info exists obj_list] {
	 set n1 [lindex $obj_list 0]
      }
   }
   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }

   if {"[keylget table fileSchema]" == "OBJC_IO"} {
       set is_fpObjc 1
   } else {
       set is_fpObjc 0
   }

   if $is_fpObjc {
      foreach v "run camCol field" {
	 set $v [keylget table $v]
      }
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   global OBJ_TYPE
   foreach el [array names OBJ_TYPE] {
      set iOBJ_TYPE($OBJ_TYPE($el)) $el
   }

   if [regexp {,} $n1] {		# is n1 really rowc,colc?
      set n1 [find_objc table $n1]
   }

   set default_ans "";			# what to do if user hits <CR>
   #
   # Set up for automatic scalebars (set with the "scale" command)
   #
   global default_scalebar_args
   if ![info exists default_scalebar_args] {
      set default_scalebar_args 0
   }

   set i $n1
   while {$i <= $n2} {
      catch {catObjDel $objcIo -deep};	# it may not exist
      set objcIo [objcIoRead table $i]

      # run etc. may be different for each object in summary tables
      foreach v "run camCol field" {
	 if $is_fpObjc {
	    set $v [keylget table $v]
	 } else {
	    set $v [exprGet $objcIo.$v]
	 }
      }
      
      if {"$select" != "" && ![eval $select $objcIo]} {
	 incr i;
	 continue;
      }
      
      if !$nodisplay {
	 if {$show_family || $post_proc == "mtv_blend"} {
	    if [catch { mtv_blend table $i } msg] {
	       echo "$msg; displaying single object"
	       mtv_objc; show_peaks
	    }
	    if {$post_proc != "" && $post_proc != "mtv_blend"} {
	       $post_proc table
	    }
	 } else {
	    read_objc table $i
	    if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
	       mtv_blend;		# will show non-bright child
	    } else {
	       mtv_objc
	    }
	    
	    if {$post_proc != ""} {
	       $post_proc table
	    }
	 }
      }

      set ii $i
      while 1 {
	 if [catch {set id [exprGet $objcIo.id]}] {
	    echo "Proceeding to next object"
	    set ans ""
	    break
	 }

	 if [keylget table is_summary_file] {
	    set rownum " \[$i\]"
	 } else {
	    set rownum ""
	 }

	 set d_a $default_ans
	 if {$d_a != ""} {
	    append d_a "\b"
	 }

	 set catId [exprGet $objcIo.catID]
	 if {$catId > 0} {
	    set catId " (catId: $catId)"
	 } else {
	    set catId ""
	 }

	 puts -nonewline stdout \
	     [format "%d-%d-%d:%-3d$rownum (%.2f,%.2f)  %s%s  $d_a" \
		  $run $camCol $field [exprGet $objcIo.id] \
		  [exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc] \
		  $iOBJ_TYPE([exprGet (int)$objcIo.objc_type]) $catId]

	 if 0 {				# interacts with broken ftcl editor
	    while {[select {stdin} {} {} 0.1] == ""} {
	       update;			# process e.g. mouseclicks
	    }
	 }

	 saoDrawCache -off
	 if {$default_scalebar_args > 0} {
	    eval draw_scalebar 1 $default_scalebar_args
	 }

	 while [catch { set ans [gets stdin] } err] {
	    select {stdin} {} {} 0.5
	    puts -nonewline stdout "."; flush stdout

	    if {[incr niter] >= 10 || ![regexp {Interrupted system call} $err]} {
	       return -code error -errorinfo "($niter) $err"
	    }
	 }

	 if {$ans == ""} {
	    set ans $default_ans
	 }
	 #
	 # Is there a user proc for this command?
	 #
	 if [regexp {^[a-zA-z0-9_]} [lindex $ans 0]] {
	    set proc mtv_objc_list_proc_[lindex $ans 0]
	    if {[info commands $proc] == "$proc"} {
	       if [catch { eval $proc $ans } msg] {
		  echo $msg
	       }
	       continue;
	    }
	 }

	 switch -regexp -- [lindex $ans 0] {
	    {^!} {
	       global phOutput_command
	       if [catch {
		  if {$ans == "!!"} {
		     if ![info exists phOutput_command] {
			error "No previous command is known"
		     }
		     set command $phOutput_command
		  } else {
		     set command \
			 [string range $ans 1 [expr [string length $ans]-1]]
		  }

		  if ![regexp {^(tb)$} $command] {
		     set phOutput_command $command
		  }

		  uplevel #0 [list eval $command]
	       } msg] {
		  echo $msg
	       }
	    }
	    {^([0-9]+|[-+][0-9]*|[-+]?[0-9:]+(\.[0-9]*)?(,| +)[-+]?[0-9:]+(\.[0-9]*)? *([^ ]+)?) *$} {
	       if [regexp {^[+-]$} $ans] {
		  set default_ans "$ans"

		  incr i ${ans}1
	       } else {
		  set default_ans ""
		  
		  if [regexp {^[+-][0-9]+$} $ans] {
		     incr i ${ans}
		  } elseif [regexp {,|[0-9:]+(\.[0-9]+)? +[-+]?[0-9]} $ans] {
		     regsub {[|,]} $ans { } ans
		     set i [eval find_objc table $ans]
		  } else {
		     set i $ans
		  }
	       }

	       if [catch {
		  eval read_objc table $i
	       } msg] {
		  echo $msg
		  continue;
	       }

	       if !$is_fpObjc {
		  foreach v "run camCol field" {# may differ for each object
		     set $v [exprGet $objcIo.$v]
		  }
	       }
      
	       if !$nodisplay {
		  if [catch {
		     if {$show_family || $post_proc == "mtv_blend"} {
			mtv_blend
		     } else {
			mtv_objc
		     }
		  } msg] {
		     echo "$msg; displaying single object"
		     mtv_objc; show_peaks
		  }
	       }
	    }
	    {^c} {
	       classify_objc
	    }
	    {^d} {
	       if [catch { mtv_blend } msg] {
		  echo "$msg; displaying single object"
		  mtv_objc; show_peaks
	       }
	    }
	    {^F} {
	       set parent [exprGet $objcIo.parent];
	       if {$parent < 0} {
		  bell; echo "Object has no parent; displaying object"
	       }
	       
	       set objcIo [read_parent table $objcIo]
	       set i [fitsBinTblRowSeek [keylget table OBJfd] 0 1]
	       
	       global default_sel_merge
	       if [info exists default_sel_merge] {
		  echo Calling $default_sel_merge to merge some children
	       }
	       
	       if [catch { mtv_blend } msg] {
		  echo "$msg; displaying single object"
		  mtv_objc; show_peaks
	       }
	    }
	    {^f} {
	       pflags
	       set flags2 [exprGet $objcIo.objc_flags2]
	       loop j 0 [exprGet $objcIo.ncolor] {
		  set flags2 [expr $flags2 | [exprGet $objcIo.flags2<$j>]]
		  if $flags2 {
		     break;
		  }
	       }
	       if $flags2 {
		  pflags2
	       } else {
		  echo No OBJECT2 flags are set
	       }
	    }
	    {^h} {
	       echo \
 c\[lassify\]  d\[isplay\]  F\[amily\]  f\[flags\]  h\[elp\]  n\[next\] (default)
	       echo \
 N\[ext skipping families\]  ppm \[-h|args\] p\[rint \[args\]\]
	       echo \
		   P\[rint args\]  q\[uit\] \{S,s\} \[how\] sc\[ale\] len \[-h \[fpC\] \[filter\] how\]  v\[elocity\]  !expr  !!
	       echo \
 nnn (display object nnn)  +-\[n\]  r c (objc at r,c)  alpha delta (ra|j2000)  
	       echo \
 Otherwise, call proc \"mtv_objc_list_proc_?\"
	       loop c 0 26 {
		  foreach C "0 32" {
		     set cc [format %c [expr 65 + $C + $c]];# e.g. 'A' or 'a'
		     set proc mtv_objc_list_proc_$cc
		     if {[info commands $proc] == "$proc"} {
			puts -nonewline " [$proc $cc help]"
		     }
		  }
	       }
	       echo ""
	    }
	    {^N} {
	       set default_ans "N"

	       if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
		  set objcIo [read_objc table next]
		  set bright_parent 1
	       } else {
		  set bright_parent -1;	# unknown
	       }
	       
	       set nchild [exprGet $objcIo.nchild]
	       if {$nchild > 0} {
		  incr i $nchild
		  if {$bright_parent == 1} {
		     incr i
		  }
	       } else {
		  set parent [exprGet $objcIo.parent]
		  if {$parent > 0} {
		     incr i [expr $parent - [exprGet $objcIo.id]]

		     set objcIo [read_parent table $objcIo]

		     if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
			incr i 2;	# we were a single non-BRIGHT child
		     } else {
			incr i [exprGet $objcIo.nchild]
		     }
		  }
	       }
	       
	       break
	    }
	    {^(n|$)} {
	       set default_ans ""
	       break
	    }
	    {^(ppm|PPM)} {
	       global phDefault_ppm_args

	       if ![info exists phDefault_ppm_args] {
		  set phDefault_ppm_args ""
	       }

	       if [regexp -- {^-h} [lindex $ans 1]] {
		  echo phDefault_ppm_args: \
		      [join $phDefault_ppm_args] (\"ppm gri\" to reset)
		  continue
	       }

	       set args ""
	       foreach a [lreplace $ans 0 0] {
		  if [regexp {^([ugriz])([ugriz])([ugriz])$} $a foo r g b] {
		     if [regexp {^(ugr|gri|riz)$} $a] {
			set tmp $b; set b $r; set r $tmp
		     }
		     
		     lappend args "-filter ${r}${g}${b}"
		  } elseif [regexp {^-?(log|lin|sqrt|asinh)} $a] {
		     regsub -- {^-} $a {} a
		     lappend args "-type $a"
		  } else {
		     if ![regexp {^-ty(pe)?} $a] {
			lappend args $a
		     }
		  }
	       }

	       if {$args == ""} {
		  set args "$phDefault_ppm_args"
	       } else {
		  set phDefault_ppm_args $args
	       }
	       
	       if {[lsearch $args "-remove"] < 0} {
		  lappend args "-remove 2"
	       }

	       if [catch {
		  eval mtv_blend_ppm table -1 [join $args]
	       } msg] {
		  echo $msg
	       }
	    }
	    {^p} {
	       set what [lreplace $ans 0 0]
	       if {$what == "-s"} {
		  echo $phDefault_elements
		  continue
	       } elseif [regexp {^\+} [lindex $what 0]] {
		  if {[lindex $what 0] == "+"} {
		     set what "[lreplace $what 0 0]"
		  } else {
		     regsub {^\+ *} $what "" what
		  }
		  if [info exists phDefault_elements] {
		     set what "$phDefault_elements $what"
		  }
	       } elseif [regexp {^\-} [lindex $what 0]] {
		  if {[lindex $what 0] == "-"} {
		     set what "[lreplace $what 0 0]"
		  } else {
		     regsub {^\- *} $what "" what
		  }
		  if [info exists phDefault_elements] {
		     foreach w $what {
			set ind [lsearch $phDefault_elements $w]
			if {$ind >= 0} {
			   set phDefault_elements \
			       [lreplace $phDefault_elements $ind $ind]
			}
		     }
		     set what $phDefault_elements
		  } else {
		     set what ""
		  }
	       }

	       if {$what != ""} {
		  set phDefault_elements $what
	       }

	       if [catch {
		  p_objc table -1 all $what
	       } msg] {
		  echo $msg
	       }
            }
	    {^P} {
	       if ![info exists phDefault_elements] {
		  set phDefault_elements ""
	       }
	       
               p_objc table -1 all $phDefault_elements
            }
	    {^q} { set ans "q"; break; }
	    {^[sS](how)?$} {
	       if {[regexp {^S} $ans] ||
		   [lsearch "u g r i z" [lindex $ans 1]] >= 0 ||
		   [lsearch [saoGetRegion] "2"] < 0} {
		  if [regexp -- {^-?fpC$} [lindex $ans 1]] {
		     set fpC "-fpC"; set ans [lreplace $ans 1 1]
		  } else {
		     set fpC ""
		  }
		  if [regexp {^[a-zA-Z]$} [lindex $ans 1]] {
		     set f [lindex $ans 1]
		  } else {
		     set f "r"
		  }
		  global openit
		  array set _openit [array get openit]
		  uplevel \#0 [eval list this_field -filter $f $fpC -IDs]
		  array set openit [array get _openit]
		  read_objc table $i
	       }

	       set ans [lreplace $ans 0 0]
	       set args ""

	       global displayed_band
	       if [regexp {^[ugriz]$} [lindex $ans 0]] {
		  set f [lindex $ans 0]
		  set ans [lreplace $ans 0 0]
		  set displayed_band [lsearch [keylget table filters] $f]
	       }

	       if {[lsearch [saoGetRegion] 2] >= 0} {
		  lappend args -s 2
	       }
	       
	       if {[llength $ans] == 0} {
		  set ans $phDefault_mtv_list_s
	       } elseif {$ans != "-h"} {
		  if {[lindex $ans 0] == "-t"} {
		     set ans [lrange $ans 1 end]
		  } else {
		     set phDefault_mtv_list_s $ans
		  }
	       }

	       eval lappend args $ans

	       if {![info exists displayed_band] || $displayed_band == ""} {
		  set displayed_band [keylget table ref_band]
	       }
	       eval show_objc -band $displayed_band $args

	       if {$ans == "-h"} {
		  echo "Default: $phDefault_mtv_list_s"
		  echo "    s -t ... won't remember arguments"
	       }
	    }
	    {^sc(ale|alebar)?$} {
	       set len [lindex $ans 1]
	       if {$len == ""} {
		  set len $default_scalebar_args
	       }
	       set default_scalebar_args $len
	       if {$len > 0} {
		  eval draw_scalebar 1 $len
	       }
	    }
	    {^v} { p_objc table -1 all \
		       "rowv rowvErr colv colvErr vel velErr a aErr rowcc rowcErr colcc colcErr psfMag psfMagErr psfColour" }
	    default { bell }
	 }
      }

      if {$ans == "q"} {
	 break;
      }

      incr i
   }
}

#
# Here are the procs that do the work
#
proc mtv_objc_list_proc_O {args} {
   if {[lindex $args 1] == "help"} {
      return {O[bjcVersion]}
   }

   uplevel \#0 [list eval echo \[objcVersion [lrange $args 1 end]]]
}

proc mtv_objc_list_proc_V {args} {
   if {[lindex $args 1] == "help"} {
      return {V[elocity plot]}
   }

   uplevel plot_moving
}

###############################################################################
#
# Plot an object's position as a function of band
#
ftclHelpDefine phOutputs p_objc \
    "
Usage: plot_moving \[table\] \[n\] \[device\]

Plot the row- and column- positions of the object <n> in table, showing
photo's velocity. (This is the command run by mtv_objc_list's V command)

By default, plot the current object
"

proc plot_moving {{_table table} {n -1} {device "/XWINDOW"}} {
   upvar $_table table
   global fieldparams objcIo OBJECT1 OBJECT2

   if ![info exists fieldparams] {
      error "I need a fieldparams to plot relative positions"
   }

   if {$n > 0} {
      catch {catObjDel $objcIo -deep};		# it may not exist
   }
   set objcIo [objcIoRead table $n]

   set filterlist [split [exprGet $fieldparams.filters] {}]
   loop i 0 [exprGet $objcIo.ncolor] {
      set f [lindex $filterlist $i]
      set flux20($f) [exprGet $fieldparams.frame<$i>.flux20]
   }
   #
   # Get magnitudes for offsetDo
   #
   if {[keylget table fileSchema] == "OBJC_IO"} {
      set is_fpObjc 1
   } else {
      set is_fpObjc 0
   }

   loop i 0 [exprGet $objcIo.ncolor] {
      set f [lindex $filterlist $i]
      set val [exprGet $objcIo.psfCounts<$i>]
      
      if $is_fpObjc {
	 set val [expr ($val <= 0) ? 30 : \
		  20 - 2.5*log10($val/[exprGet $fieldparams.frame<$i>.flux20])]
      }
      
      set psfMags($f) $val
   }

   set pg [lindex [handleListFromType PGSTATE] 0]
   if {$pg == ""} {
      set pg [pgstateNew]
   }
   
   set deviceA [devicename $device moving.ps]
   pgstateSet $pg -device $deviceA -icError 1 -nxwindow 2 -nywindow 1 \
       -xfract 0.2 -yfract 0.15
   pgstateOpen $pg
   pgAsk 0
   
   set rcf [get_run_camCol_field table $objcIo]
   set run [lindex $rcf 0]
   set camCol [lindex $rcf 1]
   set field [lindex $rcf 2]

   topLabel 20 \
       [format "Object %d-%d-%d:%d" $run $camCol $field [exprGet $objcIo.id]]

   set pos [lrange [get_canonical_positions] 1 end]
   loop i 0 [exprGet $objcIo.ncolor] {
      set dd [lindex [lindex $pos $i] 1]
      set rowc [lindex $dd 0]; set rowcErr [lindex $dd 1]
      set colc [lindex $dd 2]; set colcErr [lindex $dd 3]

      if {([exprGet $objcIo.flags<$i>] & $OBJECT1(CANONICAL_CENTER)) ||
	  ([exprGet $objcIo.flags<$i>] & $OBJECT1(PEAKCENTER)) ||
	  ([exprGet $objcIo.flags2<$i>] & $OBJECT2(INTERP_CENTER))
       } {

	 lappend bad_xval [lindex "4 8 0 2 6" $i]
	 foreach v "row col" {
	    lappend bad_$v [set ${v}c]
	    lappend bad_${v}Err [set ${v}cErr]
	 }
	 continue;
      }
            
      lappend xval [lindex "4 8 0 2 6" $i]
      foreach v "row col" {
	 lappend $v [set ${v}c]
	 lappend ${v}Err [set ${v}cErr]
      }
   }

   if ![info exists xval] {
      echo "There are no good points; sorry"
      set xval bad_xval
      foreach v "row col" {
	 set ${v} [set bad_${v}]
	 set ${v}Err [set bad_${v}Err]
      }
   }

   set x [vectorExprEval "{$xval}"]

   if [info exists bad_xval] {	# points with bad positions
      set bad_x [vectorExprEval "{$bad_xval}"]
   }
	 
   foreach v "row col" {
      set $v [vectorExprEval "{[set $v]}"];
      set ${v}Err [vectorExprEval "{[set ${v}Err]} + 10^-6"];

      set ycen [vectorExprGet \
		    "sum([set $v]/[set ${v}Err]^2)/sum(1/[set ${v}Err]^2)"]
      vectorExprSet [set $v] "[set $v] - $ycen"

      if [info exists bad_xval] {	# points with bad positions
	 set bad_$v [vectorExprEval "{[set bad_$v]} - $ycen"];
	 set bad_${v}Err [vectorExprEval "{[set bad_${v}Err]} + 10^-6"];
      }
   }

   set tmp [vectorExprEval "$x"]
   if [info exists bad_x] {
      vectorExprSet $tmp "$tmp concat $bad_x"
   }
   set xmin [vExtreme $tmp min]
   set xmax [vExtreme $tmp max]
   
   vectorExprSet $tmp "$row concat $col"
   if [info exists bad_x] {
      vectorExprSet $tmp "$tmp concat $bad_row concat $bad_col"
   }
   set ymin [vExtreme $tmp min]
   set ymax [vExtreme $tmp max]

   foreach v "row col" {
      pgstateSet $pg -isNew 1 -isLine 0

      vPlot $pg $x [set $v] -vectorYErr [set ${v}Err] \
	  -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax
      xlabel Frame

      if [info exists bad_x] {		# points with bad positions
	 pgstateSet $pg -isNew 0 -isLine 0 -icMark 2 -icError 2
	 vPlot $pg $bad_x [set bad_$v] -vectorYErr [set bad_${v}Err]
      }
      
      pgstateSet $pg -isNew 0 -isLine 1 -icMark 1 -icError 1

      set xcen [vectorExprGet "sum($x)/dimen($x)"]
      set ycen [vectorExprGet "sum([set $v])/dimen([set $v])"]

      set vel [exprGet $objcIo.${v}v]
      if !$is_fpObjc {			# convert deg/day to pix/frame
	 set vel [expr $vel*(54.5/24)*(1361.0/2048)/0.400];
      }

      vectorExprSet $tmp "$ycen + ($x - $xcen)*$vel"

      ylabel "$v (Pixels)"
      vPlot $pg $x $tmp
   }

   vectorExprDel $x; vectorExprDel $tmp
   if [info exists bad_x] {
      vectorExprDel $bad_x      
   }
   foreach v "row col" {
      vectorExprDel [set $v]; vectorExprDel [set ${v}Err]
      if [info exists bad_$v] {
	 vectorExprDel [set bad_$v]; vectorExprDel [set bad_${v}Err]
      }
   }
}

###############################################################################

ftclHelpDefine phOutputs mtv_objc_mosaic \
    "
Usage: mtv_objc_mosaic table colour size \[select\] \[n1\] \[n2\]

Display the atlas images for the specified colour for all objects in
<table> (or all in <n1>..<n2> inclusive if specified). If <select> is
provided, it's the name of a procedure used to select objects; for details
say \"help select\".

The objects are mosaiced assuming a size of \$size*\$size for each. The
array ids is set, so that you can overlay the atlas images' ids with the
proc mosaic_id
"

proc mtv_objc_mosaic {_table col size {select ""} {n1 1} {n2 -1}} {
   upvar $_table table
   global objcIo gutter OBJECT1

   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   set c [lsearch [keylget table filters] $col]
   if {$c == -1} {
      error "$col is not a valid filtername"
   }
   
   upvar ids ids; catch { unset ids }
   set n 0
   loop i $n1 [expr $n2+1] {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $i]
      
      if {"$select" != "" && ![eval $select $objcIo]} {
	 continue;
      }

      set ids($n) $i
      incr n
   }
      
   get_mosaic_geometry $n nr nc

   set ncol [expr $nc*$size+($nc-1)*$gutter]
   set nrow [expr $nr*$size+($nr-1)*$gutter]
   set reg [regNew $nrow $ncol]
   regIntSetVal $reg [softBiasGet]

   set ids(nc) $nc; set ids(size_r) $size; set ids(size_c) $size
   set ids(gutter) $gutter
   loop i 0 $n {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $ids($i)]

      get_aisize $objcIo.aimage nrow ncol
      if {$nrow == 0} {
	 continue;
      }

      set ir [expr int($i/$nc)]
      set ic [expr "$i - $nc*$ir"]
      set r0 [expr -$ir*($size+$gutter)]
      set c0 [expr -$ic*($size+$gutter)]
      set r0 [expr $r0+[exprGet $objcIo.aimage->master_mask->rmin]]
      set c0 [expr $c0+[exprGet $objcIo.aimage->master_mask->cmin]]

      regSetFromAtlasImage *$objcIo.aimage $c $reg -row0 $r0 -col0 $c0
   }

   return $reg
}

ftclHelpDefine phOutputs mosaic_id \
    "
Usage: mosaic_id \[sao\#\]

Draw the ID numbers for the image in a mosaic (made with mtv_objc_mosaic).
"

proc mosaic_id {{s 1}} {
   upvar ids ids

   set size_r $ids(size_r); set size_c $ids(size_c)
   set nc $ids(nc); set g $ids(gutter)

   saoDrawCache
   foreach n [array names ids] {
      if {![regexp {^[0-9]*$} $n]} {
	 continue;
      }

      set r [expr int($n/$nc)]; set c [expr $n-$r*$nc]
      saoDrawText [expr $r*($size_r+$g)-5] [expr $c*($size_c+$g)-5] \
	  "$ids($n)" -s $s
   }
   saoDrawCache -off
}

###############################################################################
ftclHelpDefine phOutputs find_objc \
    "
Usage: find_objc table rowc colc \[select\] \[n1\] \[n2\] \[tol\] \[band\]

Return the object that's closest to the specified position (in band band,
default 2). If specified, only objects satisfying <select> (see help select)
are considered. Once an object is found that is closer than tol,
stop looking for a better match

As a special case, you may write rowc and colc together (separated by a
colon, comma, or slash) and omit colc --- useful if you are cut-and-pasting.

If specified, only objects with IDs in the range n1--n2 are checked.
"

proc find_objc {_table rowc {colc ""} {select ""} {n1 1} {n2 -1} {tol 2} {band 2}} {
   upvar $_table table
   global objcIo ra_regexp

   if {$colc == ""} {			# maybe rowc is really rowc[,:/]colc
      if [regexp {^([^:,/]+)[:,/]([^:,/]+)([:,/]([^:,/]+))?} \
	      $rowc {} rowc colc {} ra] {
	 ;				# OK
      } else {
	 error "Please provide both row and column"
      }

      if {$ra != ""} {
	 set select $ra
      }
   }
   #
   # Maybe they specified Ra/Dec?
   #
   if [regexp $ra_regexp $select] {
      set select ""
      
      global openit
      if ![info exists openit(run)] {
	 error "You don't seem to have used set_run; sorry"
      }

      regsub {^0*} $openit(run) {} run
      set vals [where_in_run $run -rerun $openit(reprocess) \
		    -camCol $openit(camCol) $rowc $colc]

      if {$vals == -1} {
	 echo "I cannot find ra=$rowc dec=$colc in run-camCol == $run-$openit(camCol)"
      } else {
	 set found_field [lindex $vals 2]
	 set rowc [lindex $vals 3]
	 set colc [lindex $vals 4]
	 
	 set field [keylget table field]
	 if {$found_field < $field} {
	    incr rowc -1361
	 } elseif {$found_field > $field} {
	    incr rowc 1361
	 }
      }
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   if {$n1 == 1 && $select == "sel_list"} {
      global obj_list
      if [info exists obj_list] {
	 set n1 [lindex $obj_list 0]
      }
   }
   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }

   set d2_min 100000
   set tol2 [expr $tol*$tol]
   loop i $n1 [expr $n2 + 1] {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $i]

      if {$band >= [exprGet $objcIo.ncolor]} {
	 set band 0
      }
      
      if {"$select" != "" && ![eval $select $objcIo]} {
	 continue;
      }

      set d2 [expr pow([exprGet $objcIo.rowc<$band>] - $rowc, 2) + \
		  pow([exprGet $objcIo.colc<$band>] - $colc, 2)]
      if {$d2 < $d2_min} {
	 set i_min $i
	 set d2_min $d2

	 if {$d2 < $tol2} {
	    break;
	 }
      }
   }
   if ![info exists i_min] {
      error "No objects matched"
   }

   global verbose
   if {[info exists verbose] && $verbose} {
      echo "ID = $i_min  (distance: [format %.1f [sqrt $d2_min]])"
   }

   return $i_min
}

###############################################################################

ftclHelpDefine phOutputs apertures_objc \
    "
Usage: apertures_objc \[table\] \[sectors\] \[col\]

Superimpose the radial profile apertures on the result of an mtv_objc command.
If sectors is true, draw the sector boundaries too (default: false)
If col is a valid filter, assume that only that colour is displayed; otherwise
all are displayed (default). If no table is specified, use \"table\".
"

proc apertures_objc {{_table table} {sectors 0} {col all}} {
   upvar $_table table
   global gutter objcIo

   set c [lsearch [keylget table filters] $col]

   if {$c < 0} {
      set color0 0
      set ncolor [keylget table ncolor]
   } else {
      set color0 $c
      set ncolor [expr $c+1]
   }

   get_aisize $objcIo.aimage nrow ncol
   get_mosaic_geometry $ncolor nr nc

   set objc_colc [exprGet $objcIo.objc_colc]
   set objc_rowc [exprGet $objcIo.objc_rowc]
   loop i $color0 $ncolor {
      get_aisize $objcIo.aimage nrow ncol rmin cmin $i

      set om [handleBindFromHandle [handleNew] *$objcIo.aimage->mask<$i>]

      get_mosaic_index $nr $nc $i ir ic
      
      set colc [exprGet $objcIo.colc<$i>]
      set rowc [exprGet $objcIo.rowc<$i>]
      set nprof [exprGet $objcIo.nprof<$i>]
      set col0 \
	  [expr [exprGet $om.col0]-[exprGet $objcIo.aimage->dcol<$i>]+$cmin]
      set row0 \
	  [expr [exprGet $om.row0]-[exprGet $objcIo.aimage->drow<$i>]+$rmin]

      show_annuli [expr $objc_rowc-$row0+$ir*($nrow+$gutter)] \
	  [expr $objc_colc-$col0+$ic*($ncol+$gutter)] $nprof $sectors

      if {!$sectors} {
	 saoDrawPolygon \
	     [expr $rowc-$row0+$ir*($nrow+$gutter)-2] \
	     [expr $colc-$col0+$ic*($ncol+$gutter)] \
	     [expr $rowc-$row0+$ir*($nrow+$gutter)+2] \
	     [expr $colc-$col0+$ic*($ncol+$gutter)]
	 saoDrawPolygon \
	     [expr $rowc-$row0+$ir*($nrow+$gutter)] \
	     [expr $colc-$col0+$ic*($ncol+$gutter)-2] \
	     [expr $rowc-$row0+$ir*($nrow+$gutter)] \
	     [expr $colc-$col0+$ic*($ncol+$gutter)+2]
      }
      
      handleDel $om
   }
}

###############################################################################
#
# Show measure object's annuli
#
ftclHelpDefine phOutputs show_annuli \
    "
Usage: show_annuli rowc colc {n} {sectors}

Show the annuli that measure objects uses for radial profiles, centred
at (<rowc>, <colc>); only the innermost <n> are shown (default: all).
If sectors is true, also draw sector boundaries (default: false)
If number is true, number the annuli (default: false)
"

proc show_annuli {rowc colc {n 0} {sectors 0} {number 0} {sao ""}} {
   set i 0
   set radii [profileRadii]
   if {[lindex $radii 1] == 0.0} {
      echo "Initialising profile extraction code"
      initProfileExtract
      set radii [profileRadii]
   }
   if {$n <= 0} {
      set n [llength $radii]
   }
   loop i 0 $n {
      set r [lindex $radii $i]
      eval saoDrawCircle $sao $rowc $colc $r
   }

   if $number {
      saoDrawCache
      eval saoDrawText $sao [expr $rowc + 0.5*[lindex $radii 0]] $colc 1
      loop i 1 $n {
	 set r [expr 0.5*([lindex $radii [expr $i-1]]+[lindex $radii $i])]
	 eval saoDrawText $sao [expr $rowc+$r] $colc [expr $i + 1]
      }
      saoDrawCache -off
   }

   if {$sectors} {
      set rmax [lindex $radii [expr $n-1]]
      set pi [expr acos(-1)]
      foreach theta {15 45 75} {
	 set st [expr sin($theta*$pi/180)]
	 set ct [expr cos($theta*$pi/180)]
	 eval saoDrawPolygon $sao \
	     [expr $rowc+$rmax*$ct] [expr $colc+$rmax*$st] \
	     [expr $rowc-$rmax*$ct] [expr $colc-$rmax*$st]
	 eval saoDrawPolygon $sao \
	     [expr $rowc+$rmax*$ct] [expr $colc-$rmax*$st] \
	     [expr $rowc-$rmax*$ct] [expr $colc+$rmax*$st]
      }
   }
}

###############################################################################
#
# Show object's Petrosian radii (more precisely, OBJECT1->petroRad)
#
ftclHelpDefine phOutputs show_petro \
    "
Usage: show_petro \[table\] \[col\]

Overlay a mosaic created with mtv_objc with the Petrosian radii. If <col>
is specified, only overlay that colour. <table> defaults to \"table\".
"

proc show_petro {{_table table} {col all} {sao ""}} {
   upvar $_table table
   global gutter objcIo

   set c [lsearch [keylget table filters] $col]
   
   if {$c < 0} {
      set color0 0
      set ncolor [keylget table ncolor]
   } else {
      set color0 $c
      set ncolor [expr $c+1]
   }

   get_aisize $objcIo.aimage nrow ncol
   get_mosaic_geometry $ncolor nr nc

   set objc_colc [exprGet $objcIo.objc_colc]
   set objc_rowc [exprGet $objcIo.objc_rowc]
   loop i $color0 $ncolor {
      get_aisize $objcIo.aimage nrow ncol rmin cmin $i

      set om [handleBindFromHandle [handleNew] *$objcIo.aimage->mask<$i>]

      get_mosaic_index $nr $nc $i ir ic
      
      set colc [exprGet $objcIo.colc<$i>]
      set rowc [exprGet $objcIo.rowc<$i>]
      set nprof [exprGet $objcIo.nprof<$i>]
      set col0 \
	  [expr [exprGet $om.col0]-[exprGet $objcIo.aimage->dcol<$i>]+$cmin]
      set row0 \
	  [expr [exprGet $om.row0]-[exprGet $objcIo.aimage->drow<$i>]+$rmin]

      saoDrawPolygon $sao \
	  [expr $rowc-$row0+$ir*($nrow+$gutter)-2] \
	  [expr $colc-$col0+$ic*($ncol+$gutter)] \
	  [expr $rowc-$row0+$ir*($nrow+$gutter)+2] \
	  [expr $colc-$col0+$ic*($ncol+$gutter)]
      saoDrawPolygon $sao \
	  [expr $rowc-$row0+$ir*($nrow+$gutter)] \
	  [expr $colc-$col0+$ic*($ncol+$gutter)-2] \
	  [expr $rowc-$row0+$ir*($nrow+$gutter)] \
	  [expr $colc-$col0+$ic*($ncol+$gutter)+2]
      saoDrawCircle $sao \
	  [expr $rowc-$row0+$ir*($nrow+$gutter)] \
	  [expr $colc-$col0+$ic*($ncol+$gutter)] \
	  [exprGet $objcIo.petroRad<$i>]

      handleDel $om
   }
}

###############################################################################
#
# Show all the object's PEAKs (from the TEST_INFO file)
#
ftclHelpDefine phOutputs show_peaks \
    "
Usage: show_peaks \[table\] \[objcIo\] \[r0\] \[r0\]

Overlay a mosaic created with mtv_objc with the peaks recorded in the
TEST_INFO file. Peaks detected in the image's band are drawn in red,
ones that are in the OBJC are in yellow. The brightest peak in the band
is drawn as an x, all other peaks are drawn as +s.  Objects that have
OBJECT2_DEBLEND_NOPEAK set are drawn as *.

The default value of <table> is \"table\".
"

proc set_peaks {{_table table} {obj ""} {r0 0 } {c0 0} {col all}} {
   upvar $_table table
   global fieldparams gutter OBJECT1 OBJECT2
   if {$obj == ""} {
      global objcIo
      set obj $objcIo
   }

   if {[exprGet $obj.aimage->npix] == 0} {# no ATLAS_IMAGE; probably bright
      return
   }

   set filters [keylget table filters]

   set c [lsearch $filters $col]
   if {$c < 0} {
      set color0 0
      set ncolor [keylget table ncolor]
   } else {
      set color0 $c
      set ncolor 1
   }

   set aimage [handleBindFromHandle [handleNew] *$obj.aimage]
   
   get_aisize &$aimage nrow ncol
   get_mosaic_geometry $ncolor nr nc

   set peaks {}
   loop i 0 $ncolor {
      set c [expr $color0 + $i]
      get_aisize &$aimage nrow ncol rmin cmin $c

      set om [handleBindFromHandle [handleNew] *$aimage.mask<$c>]

      get_mosaic_index $nr $nc $i ir ic
      
      set nprof [exprGet $obj.nprof<$c>]
      set col0 [expr [exprGet $om.col0]+$cmin]
      set row0 [expr [exprGet $om.row0]+$rmin]
      #
      # Astrometric offsets
      #
      if [info exists fieldparams] {
	 set dd [offsetDo $fieldparams \
		     [exprGet $obj.objc_rowc] [exprGet $obj.objc_colc] \
		     [exprGet $fieldparams.ref_band_index] $i]
	 set drow [lindex $dd 0]
	 set dcol [lindex $dd 2]
      } else {
	 set drow [exprGet $aimage.drow<$c>]
	 set dcol [exprGet $aimage.dcol<$c>]
      }
      #
      # First the peaks detected in this band, drawn in red;
      # npeak_saved is the maximum number of peaks saved in TEST_INFO
      #
      set nchild [exprGet $obj.nchild]
      if {$nchild > 0} {
	 loop j 0 [expr $nchild+1] {
	    if {$j > 0} {
	       set obj [read_objc table "+"]
	    }
	    
	    set objc_colc($j) [expr [exprGet $obj.colc<$c>] + $c0]
	    set objc_rowc($j) [expr [exprGet $obj.rowc<$c>] + $r0]
	    set objc_id($j) [exprGet $obj.id]
	    
	    if {([exprGet $obj.flags2<$c>] & $OBJECT2(DEBLEND_NOPEAK))} {
	       set objc_ptype($j) "o"
	    } else {
	       set objc_ptype($j) "*"
	    }
	    
	    if {$j == 0} {
	       set objc_ei($j) "-e"
	       set objc_ptype($j) "x"
	       set objc_id($j) [expr -$objc_id($j)]
	    } else {
	       set objc_ei($j) "-i"
	    }
	 }
	 set obj [read_objc table [expr [exprGet $obj.id]-$nchild]]
      } else {
	 array set objc_id {}
      }

      if {[keylget table TSTfd] != ""} {# we have a TST file
	 set npeak [exprGet $obj.test->npeak<$c>]
	 set schema [schemaGetFromType TEST_INFO]
	 regexp {\[([^]]+)\]} [keylget schema objc_peak_row] foo npeak_saved
      
         if {$npeak > $npeak_saved} {
	    set npeak $npeak_saved
	 }

	 loop j 0 $npeak {
	    set colc [expr [exprGet $obj.test->peak_col<$c><$j>]+$c0]
	    set rowc [expr [exprGet $obj.test->peak_row<$c><$j>]+$r0]
	    set id [expr [exprGet $obj.id] + $j]
	    if {[exprGet $obj.parent] == -1} {
	       incr id
	    }
	    if {[exprGet $obj.flags2<$c>] & $OBJECT2(DEBLEND_NOPEAK)} {
	       set ptype *
	    } elseif {$j == 0} {
	       set ptype x
	    } else {
	       set ptype +
	    }
	    lappend peaks [list $id $ptype \
			       [expr $rowc-$row0+$ir*($nrow+$gutter)] \
			       [expr $colc-$col0+$ic*($ncol+$gutter)] -e]
	 }

         set npeak [exprGet $obj.test->objc_npeak]
         if {[info exists npeak_saved] && $npeak > $npeak_saved} {
	    if {$i == $color0} {
	       echo "Object [exprGet $obj.id] has $npeak peaks;"\
		   "only $npeak_saved are preserved in TEST_INFO file"
	    }
	    set npeak $npeak_saved
	 }

	 #
	 # And now for OBJC->peaks, drawn in yellow. If available, we
	 # use $fieldparams to transform from the canonical band to this one
	 #
	 set npeak [exprGet $obj.test->objc_npeak]
	 if {$npeak > $npeak_saved} {
	    if {$i == $color0} {
	       echo "Object [exprGet $obj.id] has $npeak peaks;"\
		   "only $npeak_saved are preserved in TEST_INFO file"
	    }
	    set npeak $npeak_saved
	 }
	 
	 loop j 0 $npeak {
	    set objc_colc($j) \
		[expr [exprGet $obj.test->objc_peak_col<$j>] + $dcol + $c0]
	    set objc_rowc($j) \
		[expr [exprGet $obj.test->objc_peak_row<$j>] + $drow + $r0]
	    set objc_id($j) [expr -($j + 1)]
	    set objc_ptype($j) "+"
	    set objc_ei($j) "-i"
	 }
      }
      #
      # Show the objc peaks
      loop j 0 [array size objc_id] {
	 set colc $objc_colc($j)
	 set rowc $objc_rowc($j)
	 set id $objc_id($j)
	 set ptype $objc_ptype($j)
	 set ei $objc_ei($j)

	 lappend peaks [list $id $ptype \
			    [expr $rowc-$row0+$ir*($nrow+$gutter)] \
			    [expr $colc-$col0+$ic*($ncol+$gutter)] $ei]
      }

      handleDel $om
   }

   handleDel $aimage

   return $peaks
}

proc show_peaks {{_table table} {obj ""} {r0 0} {c0 0} {col all} {ids -1}} {
   upvar $_table table
   
   show_peaks_list [set_peaks $_table $obj $r0 $c0 $col] 0 0 $ids
}

proc show_peaks_list {peaks {r0 0} {c0 0} {ids -1}} {
   saoDrawCache
   foreach p $peaks {
      set rowc [expr $r0+[lindex $p 2]]
      set colc [expr $c0+[lindex $p 3]]
      draw_cross [lindex $p 1] $rowc $colc [lindex $p 4]
      if $ids {
	 set id [lindex $p 0]
	 if {$ids > 0 || $id > 0} {
	    eval saoDrawText $rowc [expr $colc + 4] "\{$id\}" [lindex $p 4]
	 }
      }
   }
   saoDrawCache -off
}

proc get_peak_id {r c peaks} {
   set d2min 1e10
   set pid 0
   foreach p $peaks {
      set id [lindex $p 0]
      if {$id > 0} {
	 continue;
      }
      
      set dr [expr $r - [lindex $p 2]]; set dc [expr $c - [lindex $p 3]]
      set d2 [expr $dr*$dr + $dc*$dc]
      if {$d2 < $d2min} {
	 set d2min $d2
	 set pid $id
	 if {$d2 < 1} {			# close enough
	    break;
	 }
      }
   }

   return $pid
}

proc mouse_get_peak_id {reg r c f} {
   global peaks
   echo [get_peak_id $r $c $peaks]
}

###############################################################################
#
# Create a coloured image of an object's family
#
proc family2ppm {args} {
   global OBJECT1

   set catchErr 0
   set opts [list \
		 [list [info level 0] \
		      "Create a colour image of an object's family"] \
		 [list <run> STRING 0 run "Run of interest"] \
		 [list <camCol> INTEGER 0 camCol "Column of interest"] \
		 [list <field> INTEGER 0 field "Open table for this field"]\
		 [list <rowc> STRING "" rowc "Desired rowc (or ID)"] \
		 [list <colc> DOUBLE "" colc "Desired colc"] \
		 [list -rerun STRING "" rerun \
		      "Desired reprocessing number (default: youngest)"] \
		 [list -filters STRING "irg" filters \
		      "3 filters to use in constructing output file: RGB"] \
		 [list -binned INTEGER 1 binfac \
		      "How much to bin PPM file"] \
		 [list -family INTEGER 1 family \
		      "Show object's family"] \
		 [list -parent INTEGER 1 parent \
		      "Show object's parent (always true if -family is true)"] \
		 [list -scalebar DOUBLE 5.0 scalebar \
		      "Draw a scalebar this many arcsec long"] \
		 [list -catch CONSTANT 1 catchErr "Catch any errors"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if [catch {
      global objcIo table
      set_run $run $camCol -rerun $rerun
      set table [openit $field]
      if {$rowc == "ID"} {
	 set objcIo [read_objc table $colc]
      } else {
	 set objcIo [read_objc table [find_objc table $rowc $colc "!bright"]]
      }

      if {$family || $parent} {
	 set parent [exprGet $objcIo.parent];
	 if {$parent < 0} {
	    echo "Object has no parent; displaying object"
	 }
	 
	 set objcIo [read_parent table $objcIo]
	 if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
	    set objcIo [read_objc table "next"]
	 }
      }

      if $family {
	 set family ""
      } else {
	 set family "-nochildren"
      }
      
      if 0 {
	 if [catch { mtv_blend } msg] {
	    mtv_objc; show_peaks
	 }
      }
      #
      # Be nice to people who write e.g. "gri"
      #
      if [regexp {^([ugriz])([ugriz])([ugriz])$} $filters {} r g b] {
	 if [regexp {^(ugr|ugi|ugz|gri|grz|riz)$} $filters] {
	    set tmp $b; set b $r; set r $tmp
	    set filters "${r}${g}${b}"
	 }
      }
      
      set file [eval mtv_blend_ppm table -1 -remove 0 -nodisplay \
		    -filters "$filters" -bin $binfac $family \
		    -scalebar $scalebar]
   } msg] {
      if $catchErr {
	 return ""
      } else {
	 return -errorinfo $msg -code error "Failed to create ppm file"
      }
   }

   return $file
}

#################################################################
#
# calculate cumulative flux from an objcIo in band $c (e.g. 2);
# the flux within radial index i is $aperMean($i)
#
# If $err is true, return the error in the cumulative flux
#
proc calc_cumulative {objc c _aperMean {err 0}} {
   upvar $_aperMean aperMean

   if [info exists aperMean] { unset aperMean }

   set pi [expr acos(-1)]
   set rads [profileRadii]
   if {[lindex $rads 0] == 0.0} {
      initProfileExtract; set rads [profileRadii]
   }
   #
   # N.b. if $err, `mean' means variance throughout
   #
   set cmean 0; set old_area 0
   set aperMean(0) $cmean
   loop i 0 [expr [llength $rads]-1] {
      set area [expr $pi*pow([lindex $rads $i],2)]
      if $err {
	 if {$i < [exprGet $objc.nprof<$c>]} {
	    set cmean [expr $cmean + \
			   pow(($area-$old_area)*[exprGet $objc.profErr<$c><$i>],2)]
	 }
	 if {$cmean < 0} {
	    set val "NaN"
	 } else {
	    set val [expr sqrt($cmean)]
	 }
	 set aperMean([expr $i+1]) $val
      } else {
	 if {$i < [exprGet $objc.nprof<$c>]} {
	    set cmean [expr $cmean + \
			   ($area-$old_area)*[exprGet $objc.profMean<$c><$i>]]
	 }
	 set aperMean([expr $i+1]) $cmean
      }
       
      set old_area $area
   }
}

###############################################################################

ftclHelpDefine phOutputs p_objc \
    "
Usage: p_objc \[table\] \[n\] \[col\] \[elems\] \[file\]

Print the <n>th object from a <table> opened with objfileOpen (default:
table).  If <col> is valid name, it's the desired colour; otherwise
print all colours (default).

If <elems> is omitted, print some default list of elements, if present it's
a list of elements of the OBJC_IO to print

If file is provided, it's the name of a file to print to (default: -, which
means standard output). If you prepend a +, the output is appended to the file.

E.g.
      p_objc table 10 all \"rowc colc ra dec psfMag fiberCounts\" +foo.dat

Some special \"elements\" are permitted; when reading fpObjc files fields
containing \"Mag\" are calculated from the corresponding \"Counts\" fields,
ra/dec/l/b from the rowc/colc, and EB-V from l/b; you can ask for typeColour
(e.g. psfColour; typos are allowed) and get u-g, g-r, etc. 
Positions rowcc/colcc are like rowc/colc but in the r' reference frame.
vel/velErr are the velocity in the plane of the sky. dsky (or deltasky)
gives the excess sky at the position of the object, due to siblings etc.

Velocities are quoted in mpixels/field ~ 0.436 mas/min
"

proc p_objc {{_table table} {n -1} {col "all"} {elems ""} {file "-"}} {
   upvar $_table table
   global env fieldparams objcIo OBJECT1 OBJECT2

   if {$file == "-"} {
      set fd stdout
   } else {
      if [regexp {^\+(.*)} $file foo file] {
	 set mode "a"
      } else {
	 set mode "w"
      }
      set fd [open $file $mode]
   }

   set filterlist [keylget table filters]
   set c [lsearch $filterlist $col]
   
   if {$c < 0} {
      set color0 0
      set ncolor [keylget table ncolor]
   } else {
      set color0 $c
      set ncolor [expr $c+1]
   }

   if { $elems == ""} {
      set elems [list \
		     flags \
		     flags2 \
		     rowc \
		     rowcErr \
		     colc \
		     colcErr \
		     psfCounts \
		     psfCountsErr \
		     psfMag \
		     psfMagErr \
		     psfColour \
		     fiberCounts \
		     fiberCountsErr \
		     fiberSB \
		     petroRad \
		     petroCounts \
		     petroMag \
		     petroMagErr \
		    ]
   }

   #
   # What type of file?
   #
   if {[handleType $objcIo] == "OBJC_IO"} {
      set is_fpObjc 1
   } else {
      set is_fpObjc 0
   }
   #
   # Did they say Mag when the field in a tsObj file's called Counts?
   #
   if !$is_fpObjc {
      set _elems $elems; unset elems
      foreach el $_elems {
	 regsub {Mag} [lindex $el 0] Counts name
	 lappend elems [lreplace $el 0 0 $name]
      }
   }

   if {$n > 0} {
      catch {catObjDel $objcIo -deep};		# it may not exist
   }
   set objcIo [objcIoRead table $n]
   #
   # Find the longest field name; we may be able to fit in 80 columns
   #
   set wid 10
   foreach el $elems {
      set len [string length $el]
      if {$len > $wid} { set wid $len }
   }
   incr wid;				# allow for :
   #
   # Start printing it
   #
   set rcf [get_run_camCol_field table $objcIo]
   set field [lindex $rcf 2]

   set objc_type [exprGet -enum (OBJ_TYPE)$objcIo.objc_type]
   puts $fd [format "Object %d:%d (%.0f,%.0f) ($objc_type, 0x%08x 0x%08x)" \
		 $field [exprGet $objcIo.id] \
		 [exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc] \
		 [exprGet $objcIo.objc_flags] [exprGet $objcIo.objc_flags2]]

   set sch [schemaGetFull $objcIo]

   foreach el $elems {
      if {[regexp {^[-=.]$} $el]} {
	 set ll ""
	 loop j 0 [expr $wid + ($ncolor - $color0)*(12 + 1)] {
	    append ll $el
	 }
	 puts $fd $ll

	 continue;
      } elseif {[set i [lsearch -regexp $sch "^$el"]] >= 0} {
	 set sch_elem [lindex $sch $i]
      } elseif {[regexp \
		     {(col|row)cc$|SB$|^(ap[0-9]+|model)(Counts|Mag)|Colou?rs?$|^(a|vel)(Err)?$}\
		     $el]} {
	 ;				# OK
      } elseif {$is_fpObjc && ([regexp {^d(elta)?sky$} $el] ||
			       [regexp {Mag(Err)?$} $el] ||
			       [regexp {^(alpha|delta)$} $el] ||
			       [regexp {^(ra|dec)(Err)?$} $el] ||
			       [regexp {^astar(Err)?$} $el] ||
			       [regexp {^(l|b)$} $el] ||
		               [regexp {^fracDeV} $el]
			       )} {
	 ;				# OK too
      } elseif {$el == "EB-V"} {
	 
	 if ![info exists env(DUST_DIR)] {
	    echo "You must \"setup dust\" before I can tell you E_{B-V}"
	    continue;
	 } else {
	    ;				# also OK too
	 }
      } elseif {[info commands $el] != ""} {
	 ;
      } else {
	 continue;
      }
      
      puts -nonewline $fd [format "%-${wid}s " "$el:"]

      if [regexp {^(deV|exp|model)Counts(Err)?} $el {} type err] {
	 set el counts_${type}$err
	 set sch_elem [lindex $sch $i]
      }

      loop i $color0 $ncolor {
	 set f [lindex $filterlist $i]
	 if [regexp {(.*)Mag(Err)?$} $el foo pre err] {
	    set val [get_mag_from_counts $objcIo $i $pre $err]

	    if {$val != "*"} {
	       set val [format %.2f $val]
	    }
	 } elseif [regexp {^(colc|rowc)Err$} $el] {
	    global astrom_floor
	    if ![info exists astrom_floor] {
	       set astrom_floor 0.0
	    }
	    set val [exprGet $objcIo.$el<$i>]
	    set val [expr $val < 0 ? $val : pow($val,2) - pow($astrom_floor,2)]
	    set val [format "%.3f" [expr ($val < 0) ? $val : sqrt($val)]]
	    #set val [exprGet $objcIo.$el<$i>]
	 } elseif [regexp {^(colc|rowc)c$} $el] {
	    if ![info exists fieldparams] {
	       puts -nonewline \
		   "I need a fieldparams to transform between bands"
	       break
	    }

	    if ![info exists pos] {
	       set pos [get_canonical_positions]
	    }
	    if {$el == "rowcc"} {
	       set val [lindex [keylget pos $f] 0]
	    } else {
	       set val [lindex [keylget pos $f] 2]
	    }
	    set val [format %.2f $val]
	    if {[exprGet $objcIo.flags<$i>] & $OBJECT1(CANONICAL_CENTER)} {
	       set val "($val)"
	    }
	 } elseif [regexp {^fracDeV$} $el] {
	    set val [format "%.3f" [exprGet $objcIo.fracPSF<$i>]]
	 } elseif [regexp {^flags2?} $el] {
	    puts -nonewline $fd [format "  0x%08x " [exprGet $objcIo.$el<$i>]]
	    continue
	 } elseif [regexp {^astar(Err)?$} $el] {#asteroid colour a^*
	    if {$i == 0} {
	       set astarErr [regexp {^astarErr$} $el]
	       
	       if $astarErr {
		  set err "Err"
	       } else {
		  set err ""
	       }
	       
	       set g [get_mag_from_counts $objcIo 1 psf $err]
	       set r [get_mag_from_counts $objcIo 2 psf $err]
	       set i [get_mag_from_counts $objcIo 3 psf $err]
	       
	       set c0 0.89; set c1 0.45; set c2 -0.57;	    # Ivezic et al AJ 2001 122 p2758
	       
	       if $astarErr {
		  set val [expr sqrt($c0*$c0*($g*$g + $r*$r) + $c1*$c1*($r*$r + $i*$i))]
	       } else {
		  set val [expr $c0*($g - $r) + $c1*($r - $i) + $c2]
	       }
	       set val [format %.2f $val]
	    } else {
	       set val ""
	    }
	 } elseif [regexp {^(l|b|EB-V|ra|dec|alpha|delta)(Err)?|neff$} $el] {
	    if $is_fpObjc {
	       if ![info exists fieldparams] {
		  puts -nonewline \
		      "I need a fieldparams to find $el"
		  break;
	       }

	       if [regexp {^(ra|dec)$} $el] {
		  set fmt "-format"
	       } else {
		  set fmt ""
	       }
	       eval get_ra_dec_from_objcIo $objcIo $i ra dec raErr decErr $fmt
	    } else {
	       if {$i == 0} {		# only one value is available
		  set ra [exprGet $objcIo.ra]
		  set dec [exprGet $objcIo.dec]
	       } else {
		  set val ""; continue
	       }
	    }

	    if {$el == "ra" || $el == "dec"} {
	       set val [set $el];
	    } elseif {$el == "l" || $el == "b" || $el == "EB-V"} {
	       set vals [eqToGal $ra $dec]
	       set l [keylget vals gLong]
	       set b [keylget vals gLat]
	       if {$el == "EB-V"} {
		  set val [format "%.3f" [atDustGetval $l $b -map Ebv]]
	       } else {
		  set val [format "%.4f" [set $el]]
	       }
	    } elseif [regexp {alpha$} $el] {
	       set val [format %.4f $ra]
	    } elseif [regexp {delta$} $el] {
	       set val [format %.4f $dec]
	    } elseif [regexp {Err$} $el] {
	       set val [format "%5.3f" [expr [set $el]*(60*60)]]
	    } elseif {$el == "neff"} {
	       set val [format "%5.3f" [neff $fieldparams $i]]
	    } else {
	       error "I don't know how to print $el"
	    }
	 } elseif {[regexp {^(.*)SB$} $el {} type]} {
	    if ![info exists fieldparams] {
	       set val "*"
	       continue;
	    }

	    set scale 0.400;		# plate scale
	    if {$type == "fiber" || $type == "fibre"} {
	       set val [exprGet $objcIo.fiberCounts<$i>]
	       set R [expr 1.5/$scale]
	    } elseif {$type == "petro90" || $type == "petroR90"} {
	       set val [expr 0.9*[exprGet $objcIo.petroCounts<$i>]]
	       set R [exprGet $objcIo.petroR90<$i>]
	    } elseif {$type == "petro50" || $type == "petroR50"} {
	       set val [expr 0.5*[exprGet $objcIo.petroCounts<$i>]]
	       set R [exprGet $objcIo.petroR50<$i>]
	    } elseif {$type == "petro"} {
	       loop c $color0 $ncolor {
		  if {[exprGet $objcIo.flags2<$c>] &
		      $OBJECT2(CANONICAL_BAND)} {
		     break;
		  }
	       }
	       if {$c == $ncolor} {
		  if {$i == $color0} {	# only complain once
		     echo "I cannot find the canonical band"
		     set c 0
		     set val 0
		  }
	       }
	       
	       set val [exprGet $objcIo.petroCounts<$i>]
	       set R [expr 2*[exprGet $objcIo.petroRad<$c>]]; # 2:XXX
	    } else {
	       echo "Unknown type of surface brightness: $type"
	       set val "*"
	       continue
	    }
	    
	    if {$val <= 0} {
	       set val -99.99
	    } else {
	       set val [format "%.2f" \
			    [expr 20 - 2.5*log10($val/[exprGet $fieldparams.frame<$i>.flux20]/(3.14159*pow($R*$scale,2)))]]
	    }
	 } elseif [regexp {^(rowv|colv|vel|a)(Err)?$} $el] {
	    set to_deg_day [expr ((54.5/24)*(1361.0/2048)/0.400)];

	    foreach v "rowv rowvErr colv colvErr" {
	       set $v [exprGet $objcIo.$v]
	    }
	    set vel [expr sqrt(pow($rowv,2) + pow($colv,2))]
	    if {$rowvErr >= 0 && $colvErr >= 0 && $vel != 0} {
	       set velErr [expr sqrt(pow($rowv*$rowvErr,2) + \
					 pow($colv*$colvErr,2))/$vel]
	    } else {
	       set velErr -9999
	    }

	    if [regexp {^a} $el] {
	       if {$i > 0} {
		  continue;
	       }

	       set alpha 0.986;	# v = alpha/(R + sqrt(R)); Jewitt 1999
	       set vel [expr $vel/$to_deg_day]
	       set velErr [expr $velErr/$to_deg_day]
	       if {$vel == 0} {
		  set val "??"
	       } else {
		  set a [expr pow(0.5*(-1 + sqrt(1 + 4*$alpha/$vel)),2)]

		  if {$el == "a"} {
		     set val $a
		  } else {
		     set val \
			 [expr $alpha/pow($vel,2)/(1 + 1/(2*sqrt($a)))*$velErr]
		  } 
		  set val [format %.2f $val]
	       }
	    } else {
	       if {$i > 1} {
		  continue;
	       }

	       if [regexp {^vel} $el] {
		  set val [set $el]
	       } else {
		  set val [exprGet $objcIo.$el]
	       }
	       if {![regexp {Err} $el] || $val >= 0} {
		  if {$i == 0} {		# mpix/frame
		     set val [expr [softBiasGet]*$val]
		     set val [format %.2f $val]
		  } else {			# deg/day
		     set val [expr $val/$to_deg_day]
		     set val [format %.3f $val]
		  }
	       }
	    }

	 } elseif [regexp {^ap([0-9]+)Counts(Err)?$} $el foo n] {
	    if {$n == ""} {
	       set n [exprGet $objcIo.nprof<$i>
	    }

	    set err [regexp {Err$} $el]
	    if [regexp {Counts} $el] {
	       calc_cumulative $objcIo $i aperMean $err
	       set val [format %.2f $aperMean($n)]
	    } else {
	       set val [format %.2f [get_mag_from_counts $objcIo $i $el $err]]
	    }
	 } elseif [regexp {^([^C]+)Colou?rs?$} $el foo type] {
	    if {$i == 0} {
	       set val " "
	    } else {
	       if $is_fpObjc {
		  set val0 [get_mag_from_counts $objcIo [expr $i-1] $type]
		  set val1 [get_mag_from_counts $objcIo $i $type]
	       } else {
		  if [regexp {deV|exp|model} $type] {
		     set counts counts_$type
		  } else {
		     set counts ${type}Counts
		  }
		  set val0 [exprGet $objcIo.$counts<[expr $i-1]>]
		  set val1 [exprGet $objcIo.$counts<$i>]
	       }
	       
	       if {$val0 == "*" || $val1 == "*"} {
		  set val "*"
	       } else {
		  set val [format %.2f [expr $val0 - $val1]]
	       }
	    }

	    set val [format "%-11s" $val];# left justify
	 } elseif [regexp {^d(elta)?sky$} $el] {
	    if ![info exists fieldparams] {
	       puts -nonewline \
		   "I need a fieldparams to find excess sky"
	       break;
	    }
	    set r [expr int([exprGet $objcIo.rowc<$i>])]
	    set c [expr int([exprGet $objcIo.colc<$i>])]
	    set val \
		[expr [exprGet $objcIo.sky<$i>] - \
		     [binregionInterpolate *$fieldparams.frame<$i>.sky $r $c]]

	    set val [format %.2f $val]
	 } elseif {[regexp {^(nchild)$} $el]} {	# value's an integer
	    set val [exprGet $objcIo.$el]
	 } elseif {[info commands $el] != ""} {
	    set val [$el $objcIo $i]
	 } else {
	    if {[lindex $sch_elem 3] == "NULL"} {
	       if {$i > 0} {
		  continue;
	       }
	       set val [exprGet $objcIo.$el]
	    } else {
	       set val [exprGet $objcIo.$el<$i>]
	    }

	    if [regexp {\(enum\)} $val] {
	       set val [lindex [join $val] 1]
	    } else {
	       set val [format %.2f $val]
	    }
	 }
	 if {$val != ""} {
	    puts -nonewline $fd [format "%12s " $val]
	 }
      }
      puts $fd ""
   }

   if {$fd != "stdout"} {
      close $fd
   }
}

#
# Return a magnitude from an object's counts
#
proc get_mag_from_counts {objcIo c type {err ""}} {
   global fieldparams

   set b 1;				# asinh softening. XXX

   if [regsub {^test->} $type "" type] {
      set objcIo (*$objcIo.test)
   }
   
   if [regexp {ap([0-9]+)(Counts|Mag)?} $type foo n] {
      calc_cumulative $objcIo $c aperMean
      set val $aperMean($n)
   } elseif {[regexp {^cmodel(Counts|Mag)?} $type foo n]} {
      set frac [exprGet $objcIo.fracPSF<$c>]
      set val [expr $frac*[exprGet $objcIo.counts_deV<$c>] + \
		   (1 - $frac)*[exprGet $objcIo.counts_exp<$c>]]
   } else {
      if [regexp {^(deV|exp|model)} $type] {
	 set counts counts_${type}
      } else {
	 set counts ${type}Counts
      }
      
      set val [exprGet $objcIo.$counts<$c>]
   }
   
   if {$err != ""} {
      if [regexp {ap([0-9]+)(Counts|Mag)?} $type foo n] {
	 calc_cumulative $objcIo $c aperMeanErr 1
	 set valErr $aperMeanErr($n)
      } elseif {[regexp {^cmodel} $type]} {
	 set frac [exprGet $objcIo.fracPSF<$c>]
	 set valErr \
	     [expr sqrt(pow($frac*[exprGet $objcIo.counts_deVErr<$c>],2) + \
		      pow((1 - $frac)*[exprGet $objcIo.counts_expErr<$c>],2))]
      } else {
	 set valErr [exprGet $objcIo.${counts}Err<$c>]
      }
      
      set val [expr sqrt(pow(2.5/log(10)*$valErr, 2)/\
			     (4*pow($b,2) + pow($val,2)))]
   } else {
      if ![info exists fieldparams] {
	 set val "*"
      } else {
	 set mu0 [expr 20 + \
		      2.5*log10([exprGet $fieldparams.frame<$c>.flux20])\
		      - 2.5*log10($b)]
	 set val [expr $mu0 - 2.5/log(10)*[asinh [expr $val/(2.0*$b)]]]
      }
   }

   return $val
}


#
# Get the ra/dec for an objcIo; uses astrometric information from
# the global fieldparams, and the node/inclination from the table
# if not provided
#
proc get_ra_dec_from_objcIo {args} {
   set format 0;			# Format ra/dec nicely?

   set opts [list \
		 [list [info level 0] "Return an object's ra/dec"] \
		 [list <obj> STRING "" obj \
 "The OBJC_IO in question; may be \"\" if <band> is negative"] \
		 [list <band> INTEGER 0 band \
 "The desired band.
 If negative, {ra,dec}(Err)? are taken to be the object's rowc/colc"] \
		 [list <ra> STRING "" _ra "Return ra in this variable"] \
		 [list <dec> STRING "" _dec "Return dec in this variable"] \
		 [list {[raErr]} STRING "" _raErr \
		      "Return ra error in this variable"] \
		 [list {[decErr]} STRING "" _decErr \
		      "Return dec error in this variable"] \
		 [list {[node]} STRING "" node "Node of great circle"] \
		 [list {[incl]} STRING "" incl "Inclination of great circle"] \
		 [list -format CONSTANT 1 format "Format ra/dec nicely?"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if [regexp {^[0-9]+(\.[0-9]*)?$} $_ra] {
      set ra $_ra
   } else {
      upvar $_ra ra
   }

   if [regexp {^[0-9]+(\.[0-9]*)?$} $_dec] {
      set dec $_dec
   } else {
      upvar $_dec dec
   }

   if {$_raErr != ""} {
      upvar $_raErr raErr
   }
   if {$_decErr != ""} {
      upvar $_decErr decErr
   }
   global fieldparams table

   if {$obj == "" || $band < 0} {
      set band 2

      if {$obj == ""} {
	 global objcIo
	 if ![info exists objcIo] {
	    set objcIo [objcIoNew]
	 }
	 set obj $objcIo
      }

      set rowc $ra
      set colc $dec
      if [info exists raErr] {
	 set rowcErr $raErr
      } else {
	 set rowcErr 0
      }
      if [info exists decErr] {
	 set colcErr $decErr
      } else {
	 set colcErr 0
      }

      set f "r"
   } else {
      set rowc [exprGet $obj.rowc<$band>]
      set colc [exprGet $obj.colc<$band>]
      set rowcErr [exprGet $obj.rowcErr<$band>]
      set colcErr [exprGet $obj.colcErr<$band>]
      set f [format %c [exprGet $fieldparams.frame<$band>.filter]]
   }

   if {$node == ""} {
      keylget table node node
   }
   if {$incl == ""} {
      keylget table incl incl
   }

   if {$node == "" || $incl == ""} {
      echo "I don't know your node/inclination"
      set ra "*"; set dec "*"; set raErr "*"; set decErr "*"
      return
   }
   
   set gc [transApply *$fieldparams.frame<$band>.toGCC $f \
		 $rowc $rowcErr $colc $colcErr]
   
   set eq [GCToEq [keylget gc mu] [keylget gc nu] -node $node -incl $incl]
   set ra [keylget eq ra]; set dec [keylget eq dec]

   set eq [GCToEq [expr [keylget gc mu] + [keylget gc muErr]] \
	       [expr [keylget gc nu] + [keylget gc nuErr]] \
	       -node $node -incl $incl]
   set raErr [expr [keylget eq ra] - $ra]
   set decErr [expr [keylget eq dec] - $dec]

   if $format {
      set hr [expr int($ra)/15]
      set fmin [expr 60*($ra/15 - $hr)]; set min [expr int($fmin)]
      set sec [expr 60*($fmin - $min)]
      set ra [format "%02d:%02d:%05.2f" $hr $min $sec]

      if {$dec < 0} {
	 set sign "-"; set dec [expr -$dec]
      } else {
	 set sign ""
      }
      set deg [expr int($dec)]
      set fmin [expr 60*($dec - $deg)]; set min [expr int($fmin)]
      set sec [expr 60*($fmin - $min)]
      set dec [format "$sign%02d:%02d:%05.2f" $deg $min $sec]

      set raErr [format "%.2f" [expr 3600*$raErr]]
      set decErr [format "%.2f" [expr 3600*$decErr]]
   }

   return "$ra $dec $raErr $decErr"
}

ftclHelpDefine phOutputs show_obj1 \
    "
Usage: show_obj1 \[-s sao#\] \[-x #\] <obj1> \[<flags>|options\]
Overlay some information about an OBJECT1 <obj1> on its image,
already displayed on saoimage sao# (default: 0).

The flags are the logical OR of:
   0x1         Pan the image to the object's centre		pan
   0x2         Draw object's centre				center
   0x4         Draw the 1-sigma error box for the object	errors
   0x8         Erase screen before drawing			erase
  0x10         Draw Measure object's annuli			annuli
  0x20         Draw Measure object's sectors			sectors
  0x40         Number the annuli				number
  0x80         Draw the object's Petrosian radius (in red)	petroRad
 0x100         Draw the Petrosian R50 and R90 radii (in red)	petroRn
 0x200         Draw the object's isophotal ellipse		iso_ellipse
 0x400         Draw the r_e isophote for exp model		re_exp
 0x800         Draw the r_e isophote for deV model (in red)	re_deV
0x1000         Redraw all objects' IDs if available		ids

As an alternative you can specify a list of names (e.g. \"pan center\")

If -x \# is specified, scale up all measured radii by a factor \#

You probably want to say \"saoLabel off\" when using this command.
"

proc show_obj1 {args} {
   set fac 1;				# how much to scale radii
   set sao "";				# which sao to use
   while {[regexp {^-.+} [lindex $args 0] flag] == 1} {
      switch -regexp -- $flag {
	 {^-s$} {
	    if {[llength $args] < 2} {
	       error "Please specify an fsao \# with -s"
	    }
	    set args [lreplace $args 0 0]
	    set sao "-s [lindex $args 0]"
	 }
	 {^-x$} {
	    if {[llength $args] < 2} {
	       error "Please specify a scale factor with -x"
	    }
	    set args [lreplace $args 0 0]
	    set fac [lindex $args 0]
	 }
	 default {
	    echo show_obj1: unknown flag: $flag
	 }
      }
      set args [lreplace $args 0 0]
   }

   if {[llength $args] <= 1} {
      error "show_obj1: please specify the object1 and desired flags"
   }
   set obj1 [lindex $args 0]

   set ops [list pan center errors erase annuli sectors number petroRad \
		 petroRn iso_ellipse re_exp re_deV ids]
   foreach o $ops { set $o 0 } 

   if [regexp {^(0x)?[0-9a-f]+$} $args] {
      set i -1
      foreach type [list $ops] {
	 set $type [expr $args&(1<<[incr i])]
      }
   } else {
      foreach o $args {
	 regsub {^(r)_(exp|deV)$} $o {re_\2} o;# make canonical
	 set $o 1
      }
   }
   
   set obj1 "($obj1)";			# in case it is an expression
   if $erase {
      eval saoReset [lindex $sao 1]
   }
   eval saoLabel off [lindex $sao 1]

   if $ids {
      eval refresh_ids [lindex $sao 1]
   }

   set rowc [exprGet $obj1.rowc]; set colc [exprGet $obj1.colc];

   if $pan {
      eval saoPan $sao [expr int($rowc + 0.5)] [expr int($colc + 0.5)]
   }

   if $center {
      draw_cross + $rowc $colc $sao
   }

   if {$errors} {
      set dr [exprGet $obj1.rowcErr]; set dc [exprGet $obj1.colcErr];
      eval saoDrawPolygon $sao \
	  [expr $rowc-$dr] [expr $colc-$dc] \
	  [expr $rowc+$dr] [expr $colc-$dc] \
	  [expr $rowc+$dr] [expr $colc+$dc] \
	  [expr $rowc-$dr] [expr $colc+$dc] \
	  [expr $rowc-$dr] [expr $colc-$dc]
   }

   if {$annuli} {
      show_annuli $rowc $colc [exprGet $obj1.nprof] $sectors $number $sao
   }
   
   set petroList ""
   if $petroRad {
      lappend petroList "petroRad"
   }
   if $petroRn {
      lappend petroList "petroR50"; lappend petroList "petroR90"
   }
   if {$petroList != ""} {
      foreach r $petroList {
	 eval saoDrawCircle -e $sao \
	     $rowc $colc [expr $fac*[exprGet $obj1.$r]]
      }
   }

   if $re_exp {
      set ab [exprGet $obj1.ab_exp]
      set re [exprGet $obj1.r_exp]
      set phi [exprGet $obj1.phi_exp]

      eval saoDrawEllipse $sao \
	  $rowc $colc [expr $fac*sqrt($ab)*$re] [expr $fac*$re/sqrt($ab)] $phi
      
   }

   if $re_deV {
      set ab [exprGet $obj1.ab_deV]
      set re [exprGet $obj1.r_deV]
      set phi [exprGet $obj1.phi_deV]

      eval saoDrawEllipse -e $sao \
	  $rowc $colc [expr $fac*sqrt($ab)*$re] [expr $fac*$re/sqrt($ab)] $phi
      
   }

   if $iso_ellipse {
      eval saoDrawEllipse $sao \
	  [exprGet $obj1.iso_rowc] [exprGet $obj1.iso_colc] \
	  [expr $fac*[exprGet $obj1.iso_a]] [expr $fac*[exprGet $obj1.iso_b]] \
	  [exprGet $obj1.iso_phi]
   }
}

ftclHelpDefine phOutputs show_objc \
    "
Usage: show_objc \[-children\] \[-s sao\] \[-x num \] \[objc\] \[colour\] \[flags\]

Overlay some information about the <colour>th OBJECT1 in <objc> on
saoimage <sao>.

See show_obj1 for a description of the flags, and the -x option.

If you omit some of the arguments, the proc will try to guess what the
provided ones mean. The default object is given by the global \$objcIo,
which is set by e.g. mtv_blend; the default colour is 2; and the default
value of flags is a rather useless 0x0.
This means that a simple
      show_objc 0x183
will show the \"current\" object, as set by a p_objc, mtv_blend, or
similar command.
"

proc show_objc {args} {
   global objcIo

   set children 0; set sao ""; set scale ""
   set color -1
   while {[regexp {^-.+} [lindex $args 0] flag] == 1} {
      switch -regexp -- $flag {
	 {^-ch} {
	    set children 1
	 }
	 {^-band$} {
	    if {[llength $args] < 2} {
	       error "Please specify a filter number with -band"
	    }
	    set args [lreplace $args 0 0]
	    set color [lindex $args 0]
	 }
	 {^-h$} {
	    echo {show_objc [-children] [-band n] [-s n] [-x expand factor]}
	    echo
	    echo [help show_obj1]
	    return
	 }
	 {^-s$} {
	    if {[llength $args] < 2} {
	       error "Please specify an fsao \# with -s"
	    }
	    set args [lreplace $args 0 0]
	    set sao "-s [lindex $args 0]"
	 }
	 {^-x$} {
	    if {[llength $args] < 2} {
	       error "Please specify a scale factor with -x"
	    }
	    set args [lreplace $args 0 0]
	    set scale "-x [lindex $args 0]"
	 }
	 default {
	    echo show_objc: unknown flag: $flag
	 }
      }
      set args [lreplace $args 0 0]
   }
   #
   # Handle rest of arguments
   #
   if [info exists objcIo] {		# defaults
      set objc $objcIo;
   }
   set opts 0

   if {$args != "" && [regexp {^h[0-9]+$} [lindex $args 0]]} {# an object
      set objc ([lindex $args 0])
      set args [lreplace $args 0 0]
   }

   if {$args != "" && [regexp {^[0-4]$} [lindex $args 0] color]} {# a colour
      set color [lindex $args 0]
      set args [lreplace $args 0 0]
   }
   
   if {![info exists objc]} {
      error "You must specify an object as no default one exists"
   }
   if {$color < 0} {			# unknown
      if {[exprGet $objc.ncolor] > 2} {
	 set color 2
      } else {
	 set color [expr [exprGet $objc.ncolor]-1]
      }
   }
   #
   # OK, check arguments
   #
   if {$color < 0 || $color >= [exprGet $objc.ncolor]} {
      error "Illegal colour index $color for OBJC $objc"
   }

   if {[handleType $objc] == "OBJC_IO" || [handleType $objc] == "TSOBJ"} {
      set objc_io 1
      set objc [objcNewFromObjcIo $objc]
      if $children {
	 echo "warning: -children flag is ignored for OBJC_IOs"
      }
   }

   if $children {
      handleDel [objcDescendentNext $objc];# returns handle to parent
      while {[set child [objcDescendentNext NULL]] != ""} {
	 eval show_obj1 $sao $scale *$child.color<$color> $args
	 handleDel $child
      }
   } else {
      eval show_obj1 $sao $scale *$objc.color<$color> $args
   }

   if [info exists objc_io] {
      objcDel $objc
   }

   return ""
}

ftclHelpDefine objc objcNewFromObjcIo \
    "
Usage: objcNewFromObjcIo <objcIo>
Make an OBJC from an OBJC_IO.

N.b. Parent objects are not linked to their children, and atlas images
are not filled out
"

proc objcNewFromObjcIo {objcIo} {
   set ncolor [exprGet $objcIo.ncolor]
   
   set els [schemaGetFromType OBJC_IO]
   if {[handleType $objcIo] == "TSOBJ"} {
      set is_tsobj 1
   } else {
      set is_tsobj 0
   }

   set objc [objcNew $ncolor]
   loop c 0 $ncolor {
      set obj1 [object1New]
      handleSetFromHandle $objc.color<$c> &$obj1

      foreach el $els {
	 set name [lindex $el 0]; set type [lindex $el 1]
	 if {$c == 0 &&
	     [regexp {^(id|objc_(flags|rowc(Err)?|colc(Err)?))} $name]} {
	    set $objc.$name [exprGet $objcIo.$name]
	 } elseif [regexp {\[5\]$} $type] {
	    set _name $name
	    if $is_tsobj {
	       #regsub {^Q} $_name q _name
	       #regsub {^U} $_name u _name
	       regsub {_lnL} $_name _LnL _name
	    }
	    if {[catch { set val [exprGet $objcIo.$_name<$c>] }] ||
		[regexp {^NaN} $val]} {
	       set val -9999
	    }
	    regsub {\(enum\) *} $val "" val

	    if [catch {
	       eval handleSet $obj1.$name $val
	    } msg] {
	       puts -nonewline $msg
	    }
	 }
      }

      handleDel $obj1
   }

   return $objc
}

ftclHelpDefine phOutputs classify_obj1 \
    "
Usage: classify_obj1 <obj1> \[header?\]
Print the classification info from OBJECT1 <obj1>: L flux  I0 re b/a phi;
if header? is true, print a header (the default)

See also classify_objc which prints this info for the global \$objcIo
"

proc classify_obj1 {obj1 {header 1}} {
   if $header {
      echo " type       L         flux    r_e    a/b   phi"
   }
   
   foreach type [list star deV exp] {
      if {$type == "star"} {
	 set flux [exprGet ($obj1).psfCounts]
      } else {
	 set flux [exprGet ($obj1).counts_$type]
      }

      puts -nonewline [format " %-5s %6.6f %10.0f" \
			   $type [exprGet ($obj1).${type}_L] $flux]

      if {$type == "star"} {
	 puts -nonewline [format "                    %7.3f" \
			      [exprGet ($obj1).texture]]
      } else {
	 puts -nonewline [format " %6.2f %6.2f %5.1f" \
			      [exprGet ($obj1).r_$type] \
			      [exprGet ($obj1).ab_$type] \
			      [exprGet ($obj1).phi_$type]]
      }
      echo ""
   }
}

proc show_classify {args} {
   echo "show_classify has been renamed classify_objc; please use it instead"

   uplevel 1 [eval list classify_objc $args]
}

ftclHelpDefine phOutputs classify_objc \
    "
Usage: classify_objc \[table\] \[n\]
Print the classification info from the global OBJC_IO objcIo; table is
the name of the usual table (cf. objcIoRead). If n is >= 0, first read
that object; if it's \"++\", read the next object.

If not otherwise available, proc classify_objc_callback is given a
chance to append information to the classification string:

   proc classify_objc_callback {objcIo _class} {
      upvar \$_class class
   }

See also classify_obj1 which prints this information for an OBJECT1
"

proc classify_objc {{_table table} {n -1} {fd "stdout"}} {
   upvar $_table table
   global objcIo OBJECT1 OBJECT2

   if {$n == "++"} {			# increment n when we are done
      set incr 1; set n -1;
   }
   #
   # Do we support e.g. star_lnL?  (New in v5_3)
   #
   set schema [schemaGetFromType [keylget table fileSchema]]
   if [keylget schema star_lnL foo] {
      set L "lnL"
   } else {
      set L "L"
   }
   #
   # Provide names for HST MDS classification, that may be in TEST_INFO file
   #
   array set mds_type [list \
			   1 "galaxy" \
			   2 "galaxy" \
			   3 "star" \
			   4 "disk" \
			   5 "bulge" \
			   6 "disk+bulge" \
			   ]

   global OBJ_TYPE
   foreach el [array names OBJ_TYPE] {
      set iOBJ_TYPE($OBJ_TYPE($el)) $el
   }
   
   if {$n >= 0} {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $n]
   } else {
      set gal_class ""
      if {[exprGet -enum $objcIo.objc_type] == "OBJ_GALAXY"} {
	 set fac 1.0;			# decision deV v. ?? v. exp
	 append gal_class " ("
	 loop c 0 [keylget table ncolor] {
	    set deV_L [exprGet $objcIo.deV_$L<$c>]
	    set exp_L [exprGet $objcIo.exp_$L<$c>]
	    if [regexp {e-315} "$deV_L $exp_L"] {# pre-v5_3
	       set L "L"
	       set deV_L [exprGet $objcIo.deV_$L<$c>]
	       set exp_L [exprGet $objcIo.exp_$L<$c>]
	    }
	    if {$deV_L > $fac*$exp_L} {
	       append gal_class "deV "
	    } elseif {$exp_L > $fac*$deV_L} {
	       append gal_class "Exp "
	    } else {
	       append gal_class "??? "
	    }
	 }
	 set gal_class "[string trimright $gal_class])"
      }

      set true_id 0; set true_type 0
      if {[keylget table TSTfd] != ""} {
	 set true_type [exprGet $objcIo.test->true_type]
	 set true_id [exprGet $objcIo.test->true_id]
      } elseif {[keylget table truth_table ttable]} {
	 if {![regexp {^h[0-9]+$} $ttable] ||
	     [catch { set type [handleType $ttable] }]} {
	    set vals [lookup_truthtable \
		       [exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc]]
	    set true_id [lindex $vals 0]
	    set true_type [lindex $vals 1]
	 } else {
	    if {$type == "CHAIN"} {
	       set ch \
		   [chainSearch $ttable "{iseq == [exprGet $objcIo.firstId]}"]
	       set nch [chainSize $ch]
	       if {$nch > 0} {
		  if {$nch > 1} {
		     echo "Found $nch matches for iseq == $hst_id; using first"
		  }
		  set h [chainElementGetByPos $ch 0]
		  
		  set true_id [exprGet $h.iseq]
		  set true_type [exprGet $h.iclass]
		  
		  handleDel $h
	       }
	       chainDel $ch
	    } else {
	       echo "$truthfile is not a CHAIN"
	    }
	 }
      }

      if {$true_type == 0} {
	 set true_type [classify_objc_callback $objcIo gal_class]
      }
      
      if {$true_type > 0} {
	 catch {
	    append gal_class \
		"   HST: $mds_type($true_type) $true_id $true_type"
	 }
      }

      puts $fd [format "Object ID: %3d  Centre: (%.1f, %.1f)  Type: %s%s" \
		[exprGet $objcIo.id] \
		[exprGet $objcIo.objc_rowc] [exprGet $objcIo.objc_colc] \
		$iOBJ_TYPE([exprGet (int)$objcIo.objc_type]) $gal_class]
   }
   
   puts -nonewline $fd " type        [format %3s $L]        flux    r_e    a/b   phi    f(deV)   R50/R90"
   if {[keylget table TSTfd] != ""} {
      puts -nonewline $fd "    chi^2  nu chi^2/nu"
   }
   puts $fd ""

   if {[handleType $objcIo] == "OBJC_IO"} {
      set flux_fmt %9.0f
   } else {
      set flux_fmt %9.2f
   }

   set classify(star) star; set classify(deV) galaxy; set classify(exp) galaxy
   loop c 0 [keylget table ncolor] {
      regexp {{.*obj_(.*)}} \
	  [string tolower [exprGet (OBJ_TYPE)$objcIo.type<$c>]] foo ptype

      
      set best_type "unknown"
      foreach type [list star deV exp] {
	 set tL [exprGet $objcIo.${type}_$L<$c>]
	 if {$best_type == "unknown" || $tL > $best_L} {
	    set best_type $type
	    set best_L $tL
	 }
      }
      
      foreach type [list star deV exp] {
	 if {$ptype == $classify($type)} {
	    if {[exprGet $objcIo.type<$c>] == [exprGet $objcIo.objc_type]} {
	       set ctype "+"
	    } else {
	       set ctype "-"
	    }
	 } else {
	    set ctype " "
	 }

	 if {$type == "star"} {
	    set flux [exprGet $objcIo.psfCounts<$c>]
	 } else {
	    set flux [exprGet $objcIo.counts_$type<$c>]
	 }

	 set extra ""
	 if {$type == $best_type} {
	    set extra "*"
	 }

	 if {$type == "star"} {
	    set _c " "
	    if {[exprGet $objcIo.flags2<$c>] & $OBJECT2(MAYBE_CR)} {
	       set _c "\#"
	    }
	    if {[exprGet $objcIo.flags<$c>] & $OBJECT1(CR)} {
	       set _c "@"
	    }
	    append extra $_c
	 }

	 puts -nonewline $fd [format "%s%-5s %9.3g%-2s $flux_fmt" \
			      $ctype $type [exprGet $objcIo.${type}_$L<$c>] \
			      $extra $flux]
      
	 if {$type == "star"} {
	    set R90 [exprGet $objcIo.petroR90<$c>]
	    set R50 [exprGet $objcIo.petroR50<$c>]
	    if {$R90 == 0.0} {
	       set C "??"
	    } else {
	       set C [format "%4.2f" [expr $R50/$R90]]
	    }

	    puts -nonewline $fd [format "                      %7.3f %7s" \
				 [exprGet $objcIo.fracPSF<$c>] $C]
	 } else {
	    puts -nonewline $fd [format " %6.2f %6.2f %5.1f" \
				 [exprGet $objcIo.r_$type<$c>] \
				 [exprGet $objcIo.ab_$type<$c>] \
				 [exprGet $objcIo.phi_$type<$c>]]
	 }
	 if {[keylget table TSTfd] != ""} {
	    set chisq [exprGet $objcIo.test->chisq_$type<$c>]
	    set nu [exprGet $objcIo.test->nu_$type<$c>]
	    if {$type != "star"} {
	       puts -nonewline $fd "                 "
	    }
	    if {$nu > 0} {		# we measured a profile
	       puts -nonewline $fd [format "  %7.1f %3d %7.2f" \
				    $chisq $nu [expr $chisq/$nu]]
	    }
	 }
	 puts $fd ""
      }
   }

   if [info exists incr] {
      set n [expr [exprGet $objcIo.id] + 1]
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $n]
   }

   return ""
}

proc classify_objc_callback {objcIo _class} {
   upvar \$_class class

   return 0
}

###############################################################################

ftclHelpDefine phOutputs objtable2chain \
    "
Usage: objtable2chain table

Read the contents of an OBJC_IO table into a CHAIN"

proc objtable2chain {table} {

   set chain [chainNew OBJC_IO]
   
   set OBJfd [keylget table OBJfd]
   set ncolor [keylget table ncolor]
   set nrow [keylget table OBJnrow]

   # Go to beginning of file
   fitsBinTblRowSeek $OBJfd 0 0

   loop i 0 $nrow {
      set objcIo [objcIoNew]
      fitsBinTblRowRead $OBJfd $objcIo
      handleSet $objcIo.ncolor $ncolor
      chainElementAddByPos $chain $objcIo TAIL AFTER
      handleDel $objcIo
   }
   return $chain
}

###############################################################################
#
# Dump information from TEST_INFOs to a file; exactly what is dumped
# will change with the contents of the TEST_INFO
#
ftclHelpDefine phOutputs dump_test_info_list \
    "
Usage: dump_test_info_list \[file\] \[table\] \[select\] \[n1\] \[n2\]

Dump to a file at least part of all the TEST_INFO structs for all objects
in <table> (or all in <n1>..<n2> inclusive if specified). If <select> is
provided, it's the name of a procedure used to select objects; for details
say \"help select\". As a special case, if <select> is \"leaf\" only objects
without children (a category which includes children) are selected.

If no table is supplied, it defaults to \"table\".
"

proc dump_test_info_list {{file "-"} {_table table} {select ""} {n1 1} {n2 -1}} {
   upvar $_table table
   global objcIo OBJECT1

   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }
   set field [keylget table field]
   set ncolor [keylget table ncolor]

   if {$file == "-"} {
      set fd stdout
   } else {
      if [regexp {^\+(.*)} $file foo file] {
	 set mode "a"
      } else {
	 set mode "w"
      }
      set fd [open $file $mode]
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   loop i $n1 [expr $n2+1] {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $i]
      
      if {"$select" != "" && ![eval $select $objcIo]} {
	 continue;
      }
      #
      # Here's the output
      #
      loop c 0 $ncolor {
	 set line "$field:[exprGet $objcIo.id] $c"
	 append line [format " %d" [exprGet $objcIo.flags<$c>]]
	 append line [format " %11.2f" [exprGet $objcIo.psfCounts<$c>]]
	 append line [format " %8.2f" [exprGet $objcIo.psfCountsErr<$c>]]

	 foreach t "star deV exp" {
	    append line [format " %6.2f %d " \
			     [exprGet $objcIo.test->chisq_$t<$c>] \
			     [exprGet $objcIo.test->nu_$t<$c>]]
	 }
	 
	 if [expr [exprGet $objcIo.flags<$c>] & \
		 ($OBJECT1(BADSKY) | $OBJECT1(NOPROFILE))] {
	    set nr 0
	 } else {
	    set nr [exprGet $objcIo.test->nPetroRatio<$c>]
	 }
	 append line [format "  %2d" $nr]
	 loop j 0 $nr {
	    append line " [format %.3f [exprGet $objcIo.test->petroRatio<$c><$j>]]"
	 }

	 puts $fd $line
      }
   }

   if {$fd != "stdout"} {
      close $fd
   }
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

proc show_detected_objects {args} {
   set opts [list \
		 [list [info level 0] ""] \
		 [list {[table]} STRING "table" _table "Name of table as returned by openit"] \
		 [list -options STRING "center error" options "Flags to pass to show_objc"] \
	         [list -select STRING "" select "Selection proc (or expression)"] \
		 [list -filter STRING r band "Index or name of band to use"] \
		 [list -n1 INTEGER 1 n1 "Starting row"] \
		 [list -n2 INTEGER -1 n2 "Ending row"] \
		 [list -sao INTEGER 2 sao "Desired saoimage"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_table table
   global table objcIo OBJECT1

   if [catch { set ID [exprGet $objcIo.id] }] {
      set ID 1;				# so we can return to the same object
   }

   set i [lsearch [keylget table filters] $band]
   if {$i >= 0} {
      set band $i
   }

   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   loop i $n1 [expr $n2+1] {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $i]
      
      if {"$select" != "" && ![eval $select $objcIo]} {
	 continue;
      }

      eval show_objc -band $band -s $sao $options
   }

   read_objc table $ID
}

###############################################################################

ftclHelpDefine phOutputs p_profs \
    "
Usage: p_profs {file}

Print the radial profiles from the last object read or displayed with
\[p,mtv\]_objc; if <file> is provided, print them to <file> (if <file> is disk,
use a filename of \"run-camCol-field:id.dat\"

The columns are number, radius, then profMed, profMean, profErr in all bands.
"

proc p_profs {{file ""}} {
   global objcIo

   if {$file == ""} {
      set fd stdout
   } else {
      if {$file == "disk"} {
	 global table
	 set file [format "%d-%d-%d:%d.dat" \
		       [keylget table run] [keylget table camCol] \
		       [keylget table field] [exprGet $objcIo.id]]
      }
      if [regsub {^\+} $file "" file] {
	 set mode "a"
      } else {
	 set mode "w"
      }
      set fd [open $file $mode]
      puts -nonewline $fd "\#"
   }

   set radii [profileRadii]
   if {[lindex $radii 0] == 0} {
      initProfileExtract; set radii [profileRadii]
   }

   if {[handleType $objcIo] == "OBJC_IO"} {
      set prof_fmt %.4g
   } else {
      set prof_fmt %8.3f
   }

   puts $fd "Object [exprGet $objcIo.id]  Ncolor [exprGet $objcIo.ncolor]"
   set nprof 0
   loop c 0 [exprGet $objcIo.ncolor] {
      set n [exprGet $objcIo.nprof<$c>]
      if {$n > $nprof} {
	 set nprof $n
      }
   }
   loop i 0 $nprof {
      if [catch {
	 puts -nonewline $fd [format "%-2d %-7.2f  " \
				  [expr $i+1] [lindex $radii $i]]
	 loop c 0 [exprGet $objcIo.ncolor] {
	    set err [exprGet $objcIo.profErr<$c><$i>]
	    if {$err < 0} {
	       set err "*"; set mean "*"
	    } else {
	       set err [format "%.2g" $err]
	       set mean [format $prof_fmt [exprGet $objcIo.profMean<$c><$i>]]
	    }
	    
	    puts -nonewline $fd [format "  %9s %7s" $mean $err]
	 }
	 puts $fd ""
      } msg] {
	 echo $msg
	 break;
      }
   }
   if {$fd != "stdout"} {
      close $fd
   }
}

##############################################################################

lappend help_procs reconstruct_frame

proc reconstruct_frame {args} {
   set is_table 0;			# is <file> actually a table?
   set ids_at_peaks 1;			# draw object IDs at their peaks,
   ;					# not at {row,col} min
   
   set opts [list \
		 [list [info level 0] "
 Usage: reconstruct_frame objfile hdu aifile col {rmins} {cmins} {reg}

 Paste all of the atlas images for the colour <col> from object file <file>
 and atlas image <aifile> into a region, and return it. If the optional
 arguments <_rmins> and <_cmins> are provided, return a list of (rmin,cmin)
 indexed by object id (which can be superimposed on the image using the
 command show_ids rmins cmins).  If you wish, <file> may be the name of the
 list returned by e.g. an objfileOpen command.

 If <reg> is provided, it's taken to be a region to add the atlas images to;
 if it's \"noreg\", no region will be populated (but rmins/cmins will be set).

 See also regUnbin to expand a binned region for use as a background
 for reconstruct_frame

 If select is provided, it's the name of a procedure taking one or two
 arguments, a handle to an OBJC_IO and an optional band (default: 2);
 only if it returns true will the object be displayed. For details say
 \"help select\".
 "] \
		 [list <file> STRING "" file "fpObjc/tsObj file to open"] \
		 [list <hdu> INTEGER 0 hdu "Desired HDU"] \
		 [list <aifile> STRING "" aifile "fpAtlas file to open"] \
		 [list <col> STRING "" col "Desired colour"] \
		 [list {[rmins]} STRING "" _rmins \
		      "Array to return rmin values"] \
		 [list {[cmins]} STRING "" _cmins \
		      "Array to return cmin values"] \
		 [list {[reg]} STRING "" reg \
		      "Region to add atlas images into"] \
		 [list {[select]} STRING "" select "Selection function"] \
		 [list {[tstfile]} STRING "" tstfile \
		      "fpObjc*TST.fit file to open"] \
		 [list -isTable CONSTANT 1 is_table \
		      "<file> is actually the name of a list, as returned by e.g. objfileOpen"] \
		 [list -id INTEGER 0 id "Only insert object with this ID"] \
		 [list -margin INTEGER 25 margin \
		      "Insert margin of this many pixels around object"] \
		 [list -sky DOUBLE 0 sky "Constant to add to atlas images"] \
		 [list -label_peaks CONSTANT 1 ids_at_peaks \
		      "Draw object IDs at peak, not corner of bounding box"] \
		 [list -label_corners CONSTANT 0 ids_at_peaks \
		      "Draw object IDs at corner of bounding box, not peak"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$_rmins != ""} {
      upvar $_rmins rmins $_cmins cmins
      global _objc_type
   }
   global OBJECT1 table
   
   set hdr [hdrReadAsFits [hdrNew] $aifile]
   set nrow [hdrGetAsInt $hdr "NROW"]
   set ncol [hdrGetAsInt $hdr "NCOL"]
   set soft_bias [hdrGetAsInt $hdr "SOFTBIAS"]
   set filters [hdrGetAsAscii $hdr "FILTERS"]
   hdrFreeAll $hdr; hdrDel $hdr

   if $is_table {
      upvar $file table;		# upfile is really `table'
   } else {
      global openit
      
      regexp {^0*([0-9]+)$} $openit(run) {} run
      if {$openit(reprocess) > 0} {	# not a -pr 
	 set_run -root $openit(root_dir) -rerun $openit(reprocess) $run $openit(camCol)
      }
      set table [openit $openit(field)]
      
      if [catch {			# try openit; it gets e.g. node/incl right
	 global openit

	 regexp {^0*([0-9]+)$} $openit(run) {} run
         if {$openit(reprocess) > 0} {	# not a -pr 
	    set_run -root $openit(root_dir) -rerun $openit(reprocess) $run $openit(camCol)
	 }
	 set table [openit $openit(field)]
      }] {
	 set table [objfileOpen $file $aifile $hdu $tstfile]
	 index_table table -identity
      }
   }
      
   set ncolor [keylget table ncolor]
   set fd [keylget table ATLfd]
   set nobj [keylget table OBJnrow]
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]

   set c [lsearch $filters $col]

   if {$c < 0 || $c >= $ncolor} {
      set c 0
      echo "Illegal colour: $col. Using [lindex $filters $c]"
   }

   if {$reg == ""} {
      global rand
      
      set reg [regNew $nrow $ncol]
      if {[set rand [lindex [handleListFromType RANDOM] 0]] == ""} {
	 set rand [phRandomNew 100000:2]
      }
      regIntNoiseAdd $reg $rand $soft_bias 2
   }
   
   if {$reg == "noreg"} {
      set reg ""
   } else {
      hdrInsWithInt $reg.hdr "NFILTER" $ncolor
   }
   
   if {$reg != ""} {
      set tmp [regNew [exprGet $reg.nrow] [exprGet $reg.ncol]]
      regIntSetVal $reg $soft_bias;	# XXX
   }

   global objcIo
   catch { unset rmins cmins }
   loop n 1 [expr $nobj+1] {
      set objcIo [objcIoRead table $n]

      if {"$select" != "" && ![eval $select $objcIo]} {
	 catObjDel $objcIo -deep; unset objcIo
	 continue
      }
      # The following test is wrong. XXX RHL
      if {!($select == "" &&
	    [expr [exprGet $objcIo.flags<$c>] & $OBJECT1(CHILD)]) &&
	  [exprGet $objcIo.aimage->npix] > 0} {
	 if {$reg != ""} {
	    if 0 {			# XXX
	       regIntSetVal $tmp $soft_bias
	       regSetFromAtlasImage *$objcIo.aimage $c $tmp -sky $sky
	       regIntLincom $reg $tmp -1000 1 1
	    } else {
	       regSetFromAtlasImage *$objcIo.aimage $c $reg -sky $sky
	    }
	 }
	 
	 set id [exprGet $objcIo.id]
	 if $ids_at_peaks {
	    set rmins($id) [exprGet $objcIo.objc_rowc]
	    set cmins($id) [exprGet $objcIo.objc_colc]
	 } else {
	    set rmins($id) [exprGet $objcIo.aimage->master_mask->rmin]
	    set cmins($id) [exprGet $objcIo.aimage->master_mask->cmax]
	 }
	 set _objc_type($rmins($id):$cmins($id)) \
	     [exprGet -enum (OBJ_TYPE)$objcIo.objc_type]
      }
      
      catObjDel $objcIo -deep; unset objcIo
   }
   if [info exists objcIo] {
      unset objcIo
   }
   if [info exists tmp] {
      regDel $tmp
   }

   if !$is_table {
      index_table table $file $hdu
   }

   return $reg
}

##############################################################################

ftclHelpDefine phOutputs regUnbin \
    "
Usage: regUnbin region rand

binned-file is region containing a binned image that photo puts out, and
rand is a RANDOM (e.g. created by \"set rand \[phRandomNew 100000\]\"). This
procedure then expands the binned data to its original size, dithering as
needed.

See also reconstruct_frame to superimpose atlas images.
"

proc regUnbin {reg rand} {
   set breg [binregionNew]
   handleSetFromHandle $breg.reg &$reg
   handleSet $breg.bin_row [hdrGetAsInt $reg.hdr "BINFAC"]
   handleSet $breg.bin_col [hdrGetAsInt $reg.hdr "BINFAC"]
   handleSet $breg.rsize [hdrGetAsInt $reg.hdr "RSIZE"]
   handleSet $breg.csize [hdrGetAsInt $reg.hdr "CSIZE"]

   if [catch {[hdrGetAsInt $reg.hdr "SOFTBIAS"]}] {
      set soft_bias [softBiasGet];		# keyword was missing prior to
      					# Guy Fawkes day, 1998
   }

   set shifted [hdrGetAsInt $reg.hdr "SHIFTED"]
   handleSet $breg.shift $shifted
   set scaled [expr 1<<$shifted]

   set expanded [binregionUnbin $breg $rand -bkgd [expr $soft_bias/$scaled]];
   regIntConstAdd $expanded \
       [expr int($soft_bias*($scaled-1)/$scaled.)]

   handleSet $breg.reg 0x0
   binregionDel $breg;

   return $expanded
}

##############################################################################

ftclHelpDefine phOutputs show_ids \
    "
Usage: show_ids rmins cmins \[fsao\]

Given the rmins and cmins arrays that are optionally set by reconstruct_frame,
label all of the detected objects

See also refresh_ids"

proc show_ids {_rmins _cmins {sao 1}} {
   upvar $_rmins rmins  $_cmins cmins
   global _objc_type

   saoDrawCache
   foreach i [array names rmins] {
      if {[info exists _objc_type($rmins($i):$cmins($i))] &&
	  $_objc_type($rmins($i):$cmins($i)) == "OBJ_STAR"} {
	 set ei "-i";			# yellow
      } else {
	 set ei "-e";			# red
      }
      saoDrawText -s $sao $ei $rmins($i) $cmins($i) $i
   }
   saoDrawCache -off
}

ftclHelpDefine phOutputs refresh_ids \
    "
Usage: show_ids \[fsao\]

Given the rmins and cmins arrays that are optionally set by reconstruct_frame,
label all of the atlas images"

##############################################################################

ftclHelpDefine phOutputs get_mask \
    "
Usage: get_mask file \[mask\] \[planes\]

Read planes <planes> of the spanmask in <file> into mask <sm> which is
returned; if <sm> is \"\", it'll be created. If the list of planes is
given as \"all\" (the default), read them all. Return the spanmask"

proc get_mask {file {sm ""} args} {
   uncompress_file $file

   global MASK_TYPE IMASK_TYPE
#
# interpret $args
#
   if {$args == "" || $args == "all"} {
      loop p 0 [array size MASK_TYPE] {
	 lappend planes $p
      }
   } else {
      foreach a $args {
	 if [info exists MASK_TYPE($a)] {
	    set a $MASK_TYPE($a)
	 } 
	 lappend planes $a
      }
   }
   set planes [lsort -integer $planes]
#
# read the header and see if it's an LBL bitmask
#
   set hdr [hdrNew]
   if [catch {hdrReadAsFits $hdr $file} msg] {
      hdrDel $hdr
      echo $msg
      return -code error -errorinfo $msg
   }   

   set is_lblmask 0; set hdu 0
   if [catch {set is_lblmask [hdrGetAsInt $hdr "LBLMASK"]}] {
      set nrow [hdrGetAsInt $hdr "MASKROWS"]
      set ncol [hdrGetAsInt $hdr "MASKCOLS"]
   } else {
      assert {$is_lblmask == 1}
   }
   hdrFreeAll $hdr; hdrDel $hdr
#
   set reg [regReadAsFits [regNew] -keep $file]
   if {[exprGet $reg.nrow] == 0 && [exprGet $reg.ncol] == 0} {	# a real SPANMASK (not MASK) file
      regDel $reg
   } elseif $is_lblmask {
      foreach p $planes {
	 set sm [reg2spanmask $reg -type $IMASK_TYPE($p) -level [expr 1<<$p] -spanMask $sm]
      }
      regDel $reg

      return $sm
   }
#
# Set up the SCHEMATRANS, open the file, and make the SPANMASK
#
   set fd [fitsBinTblOpen $file r]

   if {$sm == ""} {
      set sm [spanmaskNew -nrow $nrow -ncol $ncol]
   } else {
      set sm "(*(SPANMASK *)&($sm))"
      if {[exprGet $sm.nrow] != $nrow || [exprGet $sm.ncol] != $ncol} {
	 verb_echo 1 [concat "get_mask: expected ${nrow}x${ncol} SPANMASK; " \
			  "saw [exprGet $sm.nrow]x[exprGet $sm.ncol]"]
      }
   }
#
# and actually skip/read the data
#
   if ![fitsBinSchemaTransIsDefined "MASK"] {
      declare_schematrans 5
   }

   loop p 0 [array size MASK_TYPE] {
      if {[lsearch $planes $p] == -1} {
	 if {$p > [lindex $planes [expr [llength $planes]-1]]} {
	    break;
	 }
	 fitsBinTblHdrRead $fd "NULL";		# skip unwanted tables
	 fitsBinTblEnd $fd
	 continue;
      }

      set nobj [fitsBinTblHdrRead -quiet $fd "OBJMASK"]
      set chain [handleBindFromHandle [handleNew] *$sm.masks<$p>]
      loop i 0 $nobj {
	 set om [objmaskNew]
	 fitsBinTblRowRead $fd $om
	 chainElementAddByPos $chain $om
	 handleDel $om
      }
      handleDel $chain
      fitsBinTblEnd $fd
   }
      
   fitsBinTblClose $fd

   return $sm
}

###############################################################################

ftclHelpDefine phOutputs flags \
    "
Usage: flags <number>
or     flags <type> <number>
or     flags <handle>
or     flags <table:number>
or     flags \[<table>\] \[<number>\]

Given some way of divining the value of a flags field, return a list
of set bits as ASCII strings.

The first form, \"flags <number>\" is simple; the number's taken to be an
OBJECT1_FLAGS value; \"flags <type> <number>\" is similar, except that the
flags are taken to be of type <type>.

If you say \"flags <handle>\", the first field in that object's schema to
match \".*flag.*\" is analysed.

If you say \"flags table:number\" or \"flags table number\", the object
analysed is taken to be an OBJC_IO, as read back from a photo output file.

E.g.:
	flags 0x1234
	flags STAR1_FLAGS 0x1234
	flags \$star1
	set table \[openit 3\]
	flags table:123
	flags table 123

If you want to print rather than manipulate the result, try pflags
"

proc flags {{obj table:} {n ""}} {
   set flags_type OBJECT1_FLAGS
   set type UNKNOWN

   if {$obj == "table"} {		# allow this for consistency
      set obj table:
   }

   if {[regexp {([a-z][a-z0-9]*):([0-9]*)} $obj foo tab which]} {
      if {$n == "" || $n >= 0} {
	 set which $n
      }
      global $tab
      set obj [objcIoRead $tab $which]
   } else {
      if {$n != ""} {
	 set flags_type $obj
	 set obj $n
	 set n -1
      }
   }

   if [regexp {^-?[0-9]} $obj] {		# assume it's an OBJECT1_FLAGS value
      set flags $obj
   } else {
      set type [handleType $obj]
      foreach e [schemaGetFromType $type] {
	 set ename [lindex $e 0]
	 set etype [lindex $e 1]
	 if [regexp {.*flag.*} $ename] {
	    set flags [exprGet (int)(($obj).$ename)]
	    if {$type == "OBJC" || $type == "OBJC_IO" || $type == "TSOBJ"} {
	       set flags_type OBJECT1_FLAGS
	    } else {
	       set flags_type ${type}_FLAGS
	    }

	    break
	 }
      }

      if {![info exists flags]} {
	 error "Cannot find a flags field in $obj"
      }
   }
   
   set flags_obj [genericNew $flags_type]

   set ret ""
   if ![regexp {^([^_]*)_} $flags_type foo base] {
      set base OBJECT1
   }
   foreach f [schemaGetFromType $flags_type] {
      if [regexp {SAVE_MASK} $f] {
	 continue;
      }
      handleSet $flags_obj [lindex $f 0]
      if {$flags & [exprGet (int)$flags_obj]} {
	 regexp ${base}_(.*) [exprGet -enum $flags_obj] foo all
	 lappend ret $all
      }
   }

   genericDel $flags_obj

   switch $type {
      "OBJC_IO" { set ncolor [exprGet $obj.ncolor] }
      "TSOBJ" { set ncolor 5 }
      "default" { set ncolor -1 }
   }
   if {$ncolor > 0} {
      set ret [list [format "%-6s  %s" $type [join $ret]]]
      loop i 0 $ncolor {
	 lappend ret "[format %-6d $i]  [flags [exprGet $obj.flags<$i>]]"
      }
   }

   return $ret
}

ftclHelpDefine phOutputs pflags \
    "
Print the output of a flags command (q.v.)"

proc pflags {{obj table:} {n -1}} {
   foreach f [flags $obj $n] {
      echo $f
   }
}

ftclHelpDefine phOutputs pflags2 \
    "
Print the OBJECT2 flags associated with an object"

proc pflags2 {{obj table:} {n -1}} {
   if {$obj == "table"} {		# allow this for consistency
      set obj table:
   }

   if {[regexp {([a-z][a-z0-9]*):([0-9]*)} $obj foo tab which]} {
      if {$n >= 0 || $n == ""} {
	 set which $n
      }
      global $tab
      set obj [objcIoRead $tab $which]
   }

   echo [format "OBJC  %s" [flags OBJECT2_FLAGS [exprGet $obj.objc_flags2]]]
   loop i 0 [exprGet $obj.ncolor] {
      echo "$i     [flags OBJECT2_FLAGS [exprGet $obj.flags2<$i>]]"
   }
}

###############################################################################
#
# Write a range of atlas images to fits files
#
ftclHelpDefine phOutputs write_atlas_images_as_fits \
    "
Usage: write_atlas_images_as_fits table \[dir\] \[filefmt\] \[n1\] \[n2\]
Write all the atlas images defined in <table> to file dir/filefmt; if n1
and n2 are specified, only write that range of object ids.

The filefmt is translated into a filename via the command
   format \$filefmt run filtername camCol field object-id
and has default value \"AI-%06d-%s%d-%04d-%04d.fits\";
<dir>'s default value is \".\"

See also the standalone (or astrotools-linkable) code in photo/readAtlasImages.
"

proc write_atlas_images_as_fits {_table {dir ""} {filefmt ""} {n1 1} {n2 -1}} {
   upvar $_table table
   global objcIo

   if {$filefmt == ""} {
      set filefmt "AI-%06d-%s%d-%04d-%04d.fits"
   }
   if {$dir != "" && ![regexp {/$} $dir]} { set dir "$dir/" }
   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }

   set filters [keylget table filters]
   set camCol [keylget table camCol]
   set field [keylget table field]
   set ncolor [keylget table ncolor]
   set run [keylget table run]

   loop i $n1 [expr $n2+1] {
      catch {catObjDel $objcIo -deep};		# it may not exist
      set objcIo [objcIoRead table $i]

      loop c 0 $ncolor {
	 if {[exprGet $objcIo.aimage->npix] <= 0} {
	    continue;
	 }

	 get_aisize $objcIo.aimage nrow ncol r0 c0 $c
	 set reg [regNew $nrow $ncol]; regIntSetVal $reg [softBiasGet]
	 regSetFromAtlasImage *$objcIo.aimage $c $reg -row0 $r0 -col0 $c0

	 set col [lindex $filters $c]
	 regWriteAsFits $reg [format "${dir}$filefmt" \
				  $run $col $camCol $field $i]
	 regDel $reg
      }
   }
}

###############################################################################
#
# Transform the object's positions measured in the different bands
# to the canonical band
#
proc get_canonical_positions {{_table table} {n -1}} {
   upvar $_table table

   global fieldparams objcIo openit

   if ![info exists fieldparams] {
      error "I need a fieldparams to transform between bands"
   }

   if {$n > 0} {
      catch {catObjDel $objcIo -deep};		# it may not exist
   }
   set objcIo [objcIoRead table $n]

   set rb [keylget table ref_band]
   set ans [list "id [exprGet $objcIo.id]"]

   set filterlist [split [exprGet $fieldparams.filters] {}]
   set rf [lindex $filterlist $rb]

   loop c 0 [keylget table ncolor] {
      set f [lindex [keylget table filters] $c]

      if $openit(is_fpObjc) {
	 set val [exprGet $objcIo.psfCounts<$c>]
	 set flux20 [exprGet $fieldparams.frame<$c>.flux20]
	 
	 if {$val <= 0} {
	    set mags($f) -99.99
	    set magErrs($f) 100
	 } else {
	    set valErr [exprGet $objcIo.psfCountsErr<$c>]
	    set mags($f) [expr 20 - 2.5*log10($val/$flux20)]
	    set magErrs($f) [expr 2.5*$valErr/($val*log(10))]
	 }
      } else {
	 set mags($f) [exprGet $objcIo.psfCounts<$c>]
	 set magErrs($f) [exprGet $objcIo.psfCountsErr<$c>]
      }
   }

   loop c 0 [keylget table ncolor] {
      set f [lindex [keylget table filters] $c]
      
      set rowc [exprGet $objcIo.rowc<$c>]
      set colc [exprGet $objcIo.colc<$c>]
      set dd [offsetDo $fieldparams $rowc $colc \
		  $c [exprGet $fieldparams.ref_band_index] \
		  $filterlist mags -magErr magErrs]
      set drow [lindex $dd 0]
      set drowErr [lindex $dd 1]
      set dcol [lindex $dd 2]
      set dcolErr [lindex $dd 3]

      set rowcErr [exprGet $objcIo.rowcErr<$c>]
      set colcErr [exprGet $objcIo.colcErr<$c>]

      lappend ans [format "$f {%.3f %.3f  %.3f %.3f}" \
		       [expr $rowc + $drow] \
		       [expr $rowcErr < 0 ? $rowcErr : \
			    sqrt(pow($rowcErr,2) + pow($drowErr,2))] \
		       [expr $colc + $dcol] \
		       [expr $colcErr < 0 ? $colcErr : \
			    sqrt(pow($colcErr,2) + pow($dcolErr,2))]]
   }

   return $ans
}

proc print_canonical_positions {{file "-"} {_table table} {n1 1} {n2 -1}} {
   upvar $_table table

   if {$n2 == -1} {
      set n2 [keylget table OBJnrow]
   }
   set filters [keylget table filters]
   
   if {$file == "-"} {
      set fd stdout
   } else {
      set fd [open $file "w"]
   }

   loop i $n1 [expr $n2+1] {
      read_objc table $i
      set pos [get_canonical_positions]

      set vals "[keylget pos id] "
      foreach f $filters {
	 append vals "   $f [keylget pos $f]"
      }
      puts $fd $vals
   }

   if {$fd != "stdout"} {
      close $fd
   }
}
###############################################################################
#
# Now routines to write a set of binary tables containing selected objects
#
# Open output Fits Binary Table for measured objects
#
ftclHelpDefine phOutputs open_mytables \
    "
Given a list <table> returned from e.g. the openit command and a <str>,
open photo output tables fpObjc-<str>.fit, fpAtlas-<str>.fit; if the 
optional argument <write_test_info> is true, write the fpObjc-<str>-TST.fit
file too.

You can add an object to the resulting tables using write_mytables, and
close them with close_mytables. For example, to copy all objects with
OBJECT1_MOVED set to a private set of binary tables, I could say:

   set_run run col; set table \[openit field\]
   set fds \[open_mytables -tsObj table some_string\]
   loop i 1 \[expr \[keylget table OBJnrow\]+1\] {
      read_objc table \$i
      if {\[exprGet \$objcIo.objc_flags\] & OBJECT1(MOVED)} {
	 write_mytables fds \$objcIo
      }
   }
   close_mytables fds

   set mytable \[openit some_string\]
   read_objc mytable 12
"

proc open_mytables {args} {
   set is_fpObjc 1; set writeAI 1
   set write_run_id 1
   set opts [list \
		 [list open_mytables "Open a private fpObjc/tsObj file"] \
		 [list <table> STRING "table" _table \
		      "name of list returned by e.g. objfileOpen or openit"] \
		 [list <name> STRING "" file_str \
		 "Identifying string for output files"] \
		 [list -tsObj CONSTANT 0 is_fpObjc \
		 "write tsObj rather than fpObjc files"] \
		 [list -dir STRING "." outdir "Directory to write files in"] \
		 [list -noAI CONSTANT 0 writeAI "don't write atlas image files"] \
		 [list -test_info INTEGER 0 write_test_info \
		 "write TEST_INFO files"] \
		 [list -noRun_id CONSTANT 0 write_run_id \
		      "Don't Write run/column/field to table"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   upvar $_table table

   if !$is_fpObjc {
      if {[lsearch [typesList] TSOBJ] < 0} {# we don't know about TSOBJs
         error "Please read a tsObj file before trying to write one"
      }
   }

   set filters [keylget table filters]
   declare_schematrans [llength $filters]

   set hdr [hdrNew]
   hdrInsWithInt $hdr "CAMCOL" -1
   hdrInsWithAscii $hdr "FILTERS" $filters
   hdrInsWithInt $hdr "REF_BAND"  [keylget table ref_band]
   hdrInsWithInt $hdr "RUN"  [keylget table run]
   hdrInsWithAscii $hdr "RERUN"  [keylget table rerun]
   if $is_fpObjc {
       hdrInsWithInt $hdr "FIELD" -1
       hdrInsWithAscii $hdr "VERSION" [keylget table version]
   } else {
       hdrInsWithInt $hdr "FIELD0" -1
       hdrInsWithInt $hdr "NFIELDS" 1
       hdrInsWithAscii $hdr "PHOT_VER" [keylget table version]
   }
   
   open_photo_tables $outdir $file_str $hdr 1 1 [softBiasGet] \
       file_objcIo OBJfd file_aimage ATLfd $write_test_info TSTfd $is_fpObjc \
       $writeAI

   hdrDel $hdr

   return [list \
	       [list OBJfd $OBJfd] \
	       [list ATLfd $ATLfd] \
	       [list TSTfd $TSTfd] \
	       [list write_run_id $write_run_id] \
	       ]
}

ftclHelpDefine phOutputs write_mytables \
    "
Write an object to a private set of binary tables:
  write_mytables <fds> <object> \[children\] \[table\]
where <fds> is a list returned by open_mytables (q.v.). If children is
specified, the object's children will be read and written to the table;
note that this will modify the global objcIo. If writing children is desired,
one must provide the table (returned by, e.g., openit) containing the parent.

Note that -children writes the objcIo's children; -family first identifies
the parent, and then writes the whole family

If -callback is given, the specified proc is called _in_the_caller's_scope_
with the object as the sole argument, just before it's written to the table.
"
proc write_mytables {args} {
   global table OBJECT1 OBJECT2

   set children 0; set family 0
   set opts [list \
		 [list write_mytables \
		      "Write an object (or family) to a private table"] \
		 [list <fds> STRING 0 _fds \
		      "name of list returned by open_mytables"] \
		 [list <objcIo> STRING "" objcIo \
		      "Object to write"] \
		 [list {[table]} STRING "" _table \
		      "Name of table (as returned by openit)"] \
		 [list -children CONSTANT 1 children \
		      "Write objcIo's children to the table"] \
		 [list -family CONSTANT 1 family \
		      "Write objcIo's entire family to the table"] \
		 [list -id INTEGER -1 primary_id \
		      "set OBJECT2(SPARE1) bit for this object; see sel_usr1"] \
		 [list -callback STRING "" write_mytables_callback \
		      "Call this proc with object as argument before writing entry; return false to skip object"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if $family {
      set children 1;			# -family => -children
   }

   upvar $_fds fds
   if {$children != 0 && $_table == ""} {  
	error "a table must be given to write children " 
   }
   if {$_table != "" } { upvar $_table table }

   if $family {
      if {$primary_id == -1} {
	 set primary_id [exprGet $objcIo.id]
      }

      set objcIo [read_parent table $objcIo]
   }

   set write_run_id [keylget fds write_run_id]
   if $write_run_id {
      #
      # put run, field, etc. into some of the TSOBJ rosat fields
      #
      global alias_inmem alias_ondisk
      if ![info exists alias_ondisk] {
	 set_alias_ondisk
      }
   
      if {[keylget table fileSchema] == "TSOBJ"} {
	 set is_fpObjc 0
      } else {
	 set is_fpObjc 1
      }

      loop i 0 [llength $alias_inmem] {
	 set alias [lindex $alias_ondisk $i]
	 set inmem [lindex $alias_inmem $i]
	 
	 set $alias [exprGet $objcIo.$alias]

	 if $is_fpObjc {
	    set val [keylget table $inmem]
	 } else {
	    set val [exprGet $objcIo.$inmem]
	 }
      
	 handleSet $objcIo.$alias $val
      }
   }

   if {[exprGet $objcIo.id] == $primary_id} {
      handleSet $objcIo.objc_flags2 $OBJECT2(SPARE1)
   }

   if {$write_mytables_callback != ""} {
      set write_it [uplevel 1 [list $write_mytables_callback $objcIo]]
      if !$write_it {
	 return
      }
   }

   fitsBinTblRowWrite [keylget fds OBJfd] $objcIo
   if {[keylget fds ATLfd] != ""} {
       fitsBinTblRowWrite [keylget fds ATLfd] *$objcIo.aimage
   }
   if {[keylget fds TSTfd] != ""} {
      fitsBinTblRowWrite [keylget fds TSTfd] *$objcIo.test
   }

   if !$children {
      return $objcIo
   }

   loop i 0 [exprGet $objcIo.nchild] {
      set obj [read_objc table next]

      if {[exprGet $obj.id] == $primary_id} {
	 handleSet $obj.objc_flags2 $OBJECT2(SPARE1)
      }

      if {$write_mytables_callback != ""} {
	 uplevel 1 [list $write_mytables_callback $obj]
      }

      if $write_run_id {
	 loop j 0 [llength $alias_inmem] {
	    set alias [lindex $alias_ondisk $j]
	    set inmem [lindex $alias_inmem $j]
	    
	    set $alias [exprGet $objcIo.$alias]
	    
	    if $is_fpObjc {
	       set val [keylget table $inmem]
	    } else {
	       set val [exprGet $objcIo.$inmem]
	    }
	    
	    handleSet $objcIo.$alias $val
	 }
      }
      
      fitsBinTblRowWrite [keylget fds OBJfd] $obj
      if {[keylget fds ATLfd] != ""} {
         fitsBinTblRowWrite [keylget fds ATLfd] *$obj.aimage
      }
      if {[keylget fds TSTfd] != ""} {
	 fitsBinTblRowWrite [keylget fds TSTfd] *$obj.test
      }
   }

   if {$write_run_id && $is_fpObjc} {
      #
      # restore those ROSAT fields
      #
      loop i 0 [llength $alias_inmem] {
	 set alias [lindex $alias_ondisk $i]
	 handleSet $objcIo.$alias [set $alias]
      }
   }
}

ftclHelpDefine phOutputs close_mytables \
    "
Close a private set of binary tables:
  close_mytables <fds>
where <fds> is a list returned by open_mytables (q.v.)
"

proc close_mytables {_fds} {
   upvar $_fds fds

   fitsBinTblClose [keylget fds OBJfd]
   if {[keylget fds ATLfd] != ""} {
   	fitsBinTblClose [keylget fds ATLfd]
   }
   if {[keylget fds TSTfd] != ""} {
      fitsBinTblClose [keylget fds TSTfd]
   }

   unset fds
}

proc sel_usr1 {obj {band -1}} {
   global OBJECT2

   set flgs2 [exprGet $obj.objc_flags2]

   return [expr $flgs2 & $OBJECT2(SPARE1)]
}
#
# Define sel_procedures for all bits in OBJECT1 and OBJECT2
#
loop i 1 3 {
   if {$i == 1} {
      set flgs flags
   } else {
      set flgs flags2
   }

   foreach name [array names OBJECT$i] {
      set args "{obj {band 2}}"
      set body \
	  [concat \
	       "global OBJECT$i;" \
	       "if {\$band < 0} {" \
	       "   return \[expr \[exprGet \$obj.objc_$flgs\] & \$OBJECT${i}($name)\]" \
	       "} else {" \
	       "   return \[expr \[exprGet \$obj.$flgs\<\$band\>\] & \$OBJECT${i}($name)\]" \
	       "}" \
	       ]
      eval proc sel_$name $args "{" $body "}"
   }
   unset name; unset args; unset body
}
#
# Select all BRIGHT objects
#
ftclHelpDefine phOutputs sel_bright \
    "Select all BRIGHT objects; usually used as -select !sel_bright"

proc sel_bright {obj {band 2}} {
   global OBJECT1 OBJECT2

   set flags [exprGet $obj.objc_flags]

   if {$flags & $OBJECT1(BRIGHT)} {
      return 1;
   }

   return 0
}

ftclHelpDefine phOutputs sel_edge \
    "Select all EDGE objects; usually used as -select !edge"

proc sel_edge {obj {band 2}} {
   global OBJECT1 OBJECT2

   set flags [exprGet $obj.objc_flags]

   if {$flags & $OBJECT1(EDGE)} {
      return 1;
   }

   return 0
}

ftclHelpDefine phOutputs sel_good \
    "Select all `good' objects; usually used as -select good"

proc sel_good {obj {band -1}} {
   global OBJECT1 OBJECT2

   if {$band < 0} {
      set flags [exprGet $obj.objc_flags]
      set flags2 [exprGet $obj.objc_flags2]
   } else {
      set flags [exprGet $obj.flags<$band>]
      set flags2 [exprGet $obj.flags2<$band>]
   }

   if {($flags & $OBJECT1(BLENDED)) && !($flags & $OBJECT1(NODEBLEND))} {
      return 0;
   }
   
   if {($flags & ($OBJECT1(BRIGHT)|$OBJECT1(EDGE))) ||
       ($flags2 & ($OBJECT2(SATUR_CENTER)))} {
      return 0;
   }

   return 1
}

ftclHelpDefine phOutputs sel_star \
    "Select all stars; usually used as -select star"

proc sel_star {obj {band 2}} {
   if {[exprGet -enum $obj.objc_type] == "OBJ_STAR"} {
      return 1
   } else {
      return 0
   }
}

ftclHelpDefine phOutputs sel_galaxy \
    "Select all galaxies; usually used as -select galaxy"

proc sel_galaxy {obj {band 2}} {
   if {[exprGet -enum $obj.objc_type] == "OBJ_GALAXY"} {
      return 1
   } else {
      return 0
   }
}

###############################################################################
#
# Utilities for manipulating tsObj files
#
proc tsObjOpen {file {hdu 1}} {
   if {[lsearch [typesList] TSOBJ] < 0} {# we don't know about TSOBJs
      set tbl [tblColNew]
      fitsRead $tbl $file -hdu $hdu
      typeNewFromTbl_photo $tbl TSOBJ
      handleDelFromType $tbl
   }
   #
   # declare TSOBJ to photo's FITS binary table code
   #
   if {[lsearch [schemaGetFromType TSOBJ] [list rerun INT]] < 0} {# OLD; XXX
      set trans [schemaTransNewAll TSOBJ {ncolor aimage test run field camCol}]
   } else {
      set trans [schemaTransNewAll TSOBJ {ncolor aimage test}]
   }
   fitsBinDeclareSchemaTrans $trans TSOBJ
   handleDel $trans

   set TSOBJfd [fitsBinTblOpen $file r]
   loop h 1 $hdu {
      fitsBinTblHdrRead $TSOBJfd ""
   }

   return $TSOBJfd
}

#
# Delete an OBJC_IO or TSOBJ
#
proc catObjDel {obj {deep ""}} {
   if {[handleType $obj] == "OBJC_IO"} {
      eval objcIoDel $obj $deep
   } else {
      if [regexp {^-d.*} $deep] {
	 atlasImageDel *$obj.aimage $deep; handleSet $obj.aimage 0
	 testInfoDel *$obj.test $deep; handleSet $obj.test 0
      }
      genericDel $obj
   }
}

#
# Modify the names in a TBLCOL definition and define it as a type. This
# is a copy of Dervish's typeNewFromTbl, with the names changed to agree
# with those in OBJC_IOs
#
proc typeNewFromTbl_photo {tbl schemaName} {
   set saw_rerun 0;			# was there a rerun field in schema?XXX

   set structDef " INT ncolor;"
   set info [tblInfoGet $tbl]
   set nfield [keylget info FLDCNT]
   for {set i 0} {$i < $nfield} {incr i} {
      set info [tblFldInfoGet $tbl -col $i]
      set type [keylget info TYPE]
      set name [keylget info TTYPE]
      set dim  [lrange [keylget info DIM] 1 end]
      if {$type == "TBLHEAPDSC"} then {
	 set type [keylget info HEAPTYPE]
      }
      if {$type == "STR"} then {
	 set type char
      }
      if {$name == "run"} { set saw_rerun 1 };# XXX
      if {$dim != ""} then {
	 set ndim [llength $dim]
	 for {set j 0} {$j < $ndim} {incr j} {
	    set thedim [lindex $dim $j]
	    set name $name\<$thedim\>
	 }
      }

      regsub {^nProf} $name "nprof" name
      regsub {^colcerr} $name "colcErr" name
      if 1 {				# fix up old tsObj files
	 regsub {(.)counts} $name "\\1Counts" name
	 regsub {dev} $name "deV" name
	 regsub {err} $name "Err" name
	 regsub {_l} $name "_L" name
	 regsub {m(ean|ed)} $name "M\\1" name
      }
      if 1 {
	  if {$name == "priority"} {
	      set type "UINT"
	  }
      }

      append structDef " $type $name;"
   }

   append structDef " ATLAS_IMAGE *aimage;"
   append structDef " TEST_INFO *test;"

   if !$saw_rerun {			# XXX
      #
      # add the run/column/field information that's usually only in the header
      #
      append structDef " int run;"
      append structDef " int camCol;"
      append structDef " int field;"
   }

   eval typedef struct {$structDef} $schemaName
}

###############################################################################
#
# Photo truth table utilities
#
proc open_truthtable {args} {
   set opts [list \
		 [list [info level 0] "Open a photo truth table"] \
		 {<file> STRING "" file "Truth table file"} \
		 {{[format]} STRING "%g %g %g  %g %g" format \
		      "Format to read lines in truth table.
 If a number, equivalent to that many %gs; if unknown count columns in the file"} \
		 {{[cols]} STRING "" cols \
		      "Which columns to read from file (default: 1..n)"} \
		 {-binning INTEGER 2 binning "Binning to use in row/column"} \
		 {-sao INTEGER 0 fsao \
	       "Display objects in table on this saoimage; don't open table"} \
		 {-drow DOUBLE 0 drow \
		      "Add drow to all row positions shown with -sao"} \
		 {-dcol DOUBLE 0 dcol \
		      "Add dcol to all col positions shown with -sao"} \
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![file exists $file] {
      error "$file doesn't exist"
   }

   if {$fsao > 0} {
      saoDrawCache
      set fd [open $file r]
      while {[gets $fd line] != -1} {
	 if [regexp {^ *\#} $line] {
	    continue;
	 }

	 if {[lindex $line 4] == 3} {# star
	    set ei "-i"
	 } else {
	    set ei "-e"
	 }

	 set rowc [expr [lindex $line 0] + $drow]
	 set colc [expr [lindex $line 1] + $dcol]

	 draw_cross + $rowc $colc "$ei -s $fsao"
      }
      saoDrawCache -off
      close $fd
      return;
   }
   #
   # if format is `unknown' count number of columns in the file
   #
   if {$format == "unknown"} {
      set fd [open $file r]
      while {[gets $fd line] != -1} {
	 if [regexp {^ *\#} $line] {
	    continue;
	 }

	 set format [llength $line]
	 break;
      }
	 
      close $fd
   }
   #
   # If the format is a number, generate a format statement
   #
   if [regexp {^[0-9]+$} $format] {
      set n $format; unset format
      loop i 0 $n {
	 lappend format "%g"
      }
   }

   if {$cols == ""} {			# assume cols should be 1 2 ... n
      set i 0
      foreach el $format {
	 append cols "[incr i] "
      }
   }

   if [catch {
      infofileRead $file $format $cols $binning
   } msg] {
      echo [string trim $msg]
      infofileFini
   }
}

proc close_truthtable {} {
   infofileFini
}

proc lookup_truthtable {rowc colc {tol 10}} {
   if [catch {
      infofileMatch $rowc $colc $tol
   } info] {
      if {$info != "phInfofileMatch returns NULL"} {
	 echo [string trim $info]
      }

      loop i 0 10 {
	 lappend result 0
      }
   } else {
      loop i 0 10 {
	 lappend result [exprGet $info.props<$i>]
      }
      handleDel $info
   }

   return $result
}

###############################################################################
#
# Useful while debugging/developing
#
if {![info exists env(PHOTODATA_DIR)]} {
   set env(PHOTODATA_DIR) ""
}

#
# Find the highest reprocessing number less than max
#
proc find_reprocess {run root_dir reprocess {max 9999}} {
   if [regexp {^[0-9]+$} $reprocess] {
      return $reprocess
   }

   if [catch {
      set dirs [glob $root_dir/$run/\[0-9\]*]
   }] {
      return $reprocess;		# glob failed to match anything
   }

   regsub -all "$root_dir/$run/" $dirs "" dirs
   foreach num [lsort -integer -decreasing $dirs] {
      if {$num <= $max} {
	 return $num
      }
   }

   return $reprocess
}

#
# Try and find a directory named dirname, starting at root_dir
#
proc find_objc_dir {run root_dir reprocess dirname {camCol ""}} {
   if {$reprocess == "?"} {
      if [file isdirectory "$root_dir/$run/photo"] {#no rerun
	 set reprocess ""
      } elseif {[catch {				# glob may fail
	 set dirs [glob $root_dir/$run/\[0-9\]*]
	 regsub -all "$root_dir/$run/" $dirs "" dirs
	 regsub -all {66[0-9]} $dirs "" dirs
	 set newest [lsort -integer -decreasing [split $dirs]]
	 if {$newest != ""} {
	    set reprocess $newest
	 }
      } msg]} {
	 echo "RHL $msg:"; traceback
      }
   }
   foreach file [list \
		     $root_dir/$run/$reprocess/$dirname/$camCol \
		     $root_dir/$run/$reprocess/$dirname \
		     $root_dir/$run/$dirname/$camCol \
		     $root_dir/$run/$dirname \
		     $root_dir/$dirname \
		     $root_dir/$run/$reprocess \
		     $root_dir/$run \
		     $root_dir \
		    ] {
      if [file isdirectory $file] {
	 return $file
      }
   }

   return ""
}

if ![info exists data_root] {
   set data_root "/dev/null"
   foreach d "/sdss/data /u/dss/data /data/dp30.a/data" {
      if {[file exists $d] || [file exists $d]} {# give automounter a chance 
	 set data_root $d
	 break;
      }
   }
}

proc set_run {args} {
   global data_root;			# default root of data directories
   global env openit field

   global allow_old_psField;		# backwards compatibility
   if [info exists allow_old_psField] {
      unset allow_old_psField
   }
   
   set is_fpObjc 1;			# read fpObjc files
   set mjd -1;				# optional at FNAL
   set testbed 0;			# we're looking at a testbed run
   
   set opts [list \
		 [list set_run "Tell photo where to find output files\n"] \
		 [list {[run]} STRING 0 run "Run of interest"] \
		 [list {[col]} INTEGER 0 col "Column of interest"] \
		 [list {[field]} INTEGER -1 _field "Open table for this field"]\
		 [list -field INTEGER -1 ffield "Open table for this field"] \
		 [list -rootdir STRING "$data_root" root_dir \
		      "Root for SDSS data/input directories"] \
		 [list -aidir STRING "" ai_dir \
		      "Root for atlas image directories (overrides -root)"] \
		 [list -asdir STRING "" as_dir \
		      "Root for astrom output directories (overrides -root)"] \
		 [list -objdir STRING "" objc_dir \
		      "Root for frames output directories (overrides -root)"] \
		 [list -psdir STRING "" ps_dir \
		      "Root for psp output directories (overrides -root)"] \
		 [list -tsObj CONSTANT 0 is_fpObjc \
		      "Read tsObj rather than fpObjc files"] \
		 [list -reprocessing STRING "" reprocess \
		      "Desired reprocessing number (default: youngest)"] \
		 [list -rerun STRING "" rerun \
		      "Desired reprocessing number (default: youngest)"] \
		 [list -mjd INTEGER $mjd mjd "MJD of interest (obsolete)"] \
		 [list -pr STRING "" pr \
		      "PR number of test case to be read (or \"runs\")"] \
		 [list -testbed CONSTANT 1 testbed \
		      "Look for outputs in \$data_root/testbed"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$ffield >= 0 && $_field >= 0} {
      error "You may not specify \[field\] and -field"
   }
   if {$_field >= 0} { set ffield $_field }
   
   if {$rerun != ""} {
      if {$reprocess != "" && $rerun != $reprocess} {
 	 error "Make up your mind; -rerun and -reprocess values are different"
      }
      set reprocess $rerun
   }

   if {$rerun == "" && $testbed} {
      set rerun 666
   }
   #
   # OK, the arguments are safely parsed; to work
   #
   # The prs directory is a special case
   #
   if {$pr != ""} {
      if {$run == 0 && $col == 0} {# try to guess
	 if [file exists "$root_dir/prs/$pr/photo"] {
	    set photodir "$root_dir/prs/$pr/photo"
	 } else {			# try prs/<pr>/<run>/<rerun>
	    set photodir [lindex [glob "$root_dir/prs/$pr/\[0-9\]*"] 0]

	    if {$rerun != ""} {
	       append photodir /$rerun
	    } elseif ![catch { set dirs [glob $photodir/\[0-9\]*] }] {
	       switch [llength $dirs] {
		  0 {
		     ;			# OK, no reruns
		  }
		  1 {
		     set photodir [lindex $dirs 0]
		  }
		  default {
		     regsub -all "$photodir/" $dirs {} reruns
		     error "Please choose a rerun; your options are: $reruns"
		  }
	       }
	    }
	    append photodir /photo
	 }

	 set fpPlans ""
	 if [catch {
	    set fpPlans [glob "$photodir/fpPlan-?.par"]
	 }] {
	    set fpPlans [glob "$photodir/fpPlan.par"]
	 }
	 set fpPlan [lindex $fpPlans 0]

	 if ![file exists $fpPlan] {
	    error "I cannot guess your run/camCol/field; please specify them"
	 }

	 param2Chain $fpPlan vals

	 foreach el "run rerun camCol startField" {
	    set $el [keylget vals $el]
	 }

	 set col $camCol
	 set ffield $startField
      }

      set_run $run $col -rerun $rerun
      array set _openit [array get openit]

      set_run -root $root_dir/prs/$pr $run $col -rerun $rerun
      #
      # Use Standard Places if not in the prs
      #
      foreach el [array names openit] {
	 if ![regexp {/} $openit($el)] {
	    continue
	 }
	 
	 if ![file isdirectory $openit($el)] {
	    set openit($el) $_openit($el)
	 }
      }

      if {$ffield >= 0} {
	 set field $ffield;		# set global
	 uplevel \#0 [list eval set table \[openit $field\]]
      }

      return ""
   } elseif $testbed {
      append root_dir "/testbed"
   }
   #
   # A regular run
   #
   regsub {^~/} $root_dir $env(HOME)/ root_dir
   if ![file isdirectory $root_dir] {
      error "$root_dir is not a directory"
   }

   if {$mjd >= 0 && [file isdirectory $root_dir/$mjd]} {
      set root_dir $root_dir/$mjd
   }

   foreach d "ai as objc ps" {
      regsub {^~/} [set ${d}_dir] $env(HOME)/ ${d}_dir
      if {[set ${d}_dir] == ""} {
	 set ${d}_dir $root_dir
      }
   }

   if {$reprocess == ""} {
      set reprocess "?"
   }
   set reprocess [find_reprocess $run $root_dir $reprocess]
   set openit(reprocess) $reprocess 
   #
   # Assume SDSS directory convention:
   #
   set openit(root_dir) $root_dir
   set openit(dir) [find_objc_dir $run $root_dir $reprocess objcs]
   if {$openit(dir) == ""} {
      set openit(dir) [find_objc_dir $run $objc_dir $reprocess objcs]
   }
   set openit(aidir) [find_objc_dir $run $ai_dir $reprocess "objcs"]
   set openit(asdir) [find_objc_dir $run $as_dir $reprocess "astrom"]
   set openit(fangdir) [find_objc_dir $run $as_dir $reprocess "fangs"]
   set openit(logdir) [find_objc_dir $run $root_dir $reprocess "logs"]
   set openit(photo) [find_objc_dir $run $root_dir $reprocess "photo"]
   set openit(psdir) [find_objc_dir $run $ps_dir $reprocess "objcs"]
   set openit(corrdir) [find_objc_dir $run $root_dir $reprocess "corr"]
   set openit(fielddir) [find_objc_dir $run $root_dir $reprocess "fields"]
   set openit(gangdir) [find_objc_dir $run $root_dir $reprocess "gangs"]
   set openit(fangdir) [find_objc_dir $run $root_dir $reprocess "fangs"]
   set openit(sscdir) [find_objc_dir $run $root_dir $reprocess "ssc"]
   set openit(nfcalibdir) [find_objc_dir $run $root_dir $reprocess "nfcalib"]
   #
   # See if there's anything relevant in the fpPlan file
   #
   set planfile $openit(photo)/fpPlan-$col.par
   if ![file exists $planfile] {
      set planfile $openit(photo)/fpPlan.par
   }
   process_plan fpplan [read_planfile $planfile]
   if {[info exists fpplan] && [keylget fpplan psDir foo]} {
      set openit(psdir) $foo
   }
   #
   # Back to heuristics
   #
   if $is_fpObjc {
      set openit(objdir) [find_objc_dir $run $objc_dir $reprocess objcs]
   } else {
      set openit(objdir) \
	  [find_objc_dir $run $objc_dir $reprocess calibChunks]
   }

   foreach dir "corrdir dir objdir" {	# maybe things aren't quite canonical?
       if ![file isdirectory $openit($dir)] {
	   set openit($dir) $objc_dir
       }
   }

   regexp {^([0-9]*)} $run foo run_no
   set openit(run) [format %06d $run_no]
   set openit(col) $col
   
   foreach d "aidir corrdir dir fangdir fielddir gangdir objdir psdir" {
      if [file exists $openit($d)/$openit(col)] {
	 set openit($d) $openit($d)/$openit(col)
      }
   }
   set openit(is_fpObjc) $is_fpObjc
   set openit(hdu) -1

   set openit(camCol) $openit(col);	# an alias

   if {$ffield >= 0} {
      set field $ffield
      uplevel \#0 [list eval set table \[openit $field\]]
   }

   return "$openit(asdir):$openit(dir):$openit(run):$openit(col)"
}

#
# backwards compatibility
#
proc mjd {mjd args} {
   eval set_run -mjd $mjd $args
}

###############################################################################
#
ftclHelpDefine phOutputs openit \
    "
Open a field, with paths, run, etc. as specified using set_run

If the requested field is not an integer, simply open files with names
fpObjc-\$field.fits etc.

You can disable the reading of fpAtlas files with -noAI.

If -index is specified, generate an index which maps object IDs to row numbers;
this is used by read_objc when a given object ID is requested. This is
only needed if the table being read is `sparse'; normal photo output tables
have object ID == row number

The truthtable can either be a regular photo truthtable, or a chain of
parameters (typically read from an MDS catalog):
          set mdsChain \[param2Chain u26x.par hdr\]
"
	   
proc openit {args} {
   global calibs env fieldparams field openit table

   set index_table 0; set next_hdu 0; set noAI 0; set noTST 0
   set opts [list \
		 [list openit "Open a fpObjc/tsObj and fpAtlas files"] \
		 [list <field> STRING "" field \
		      "desired field (or other file identifier)"] \
		 [list -noAI CONSTANT 1 noAI "don't read atlas image files"] \
		 [list -noTST CONSTANT 1 noTST "don't read TST files"] \
		 [list -nextHdu CONSTANT 1 next_hdu \
		      "read the next HDU in a tsObj file"] \
		 [list -index CONSTANT 1 index_table \
		      "generate an index mapping object ID to row"] \
		 [list -dir STRING "" objdir \
		      "Directory to use instead of \$openit(objdir)"] \
		 [list -truth STRING "" truthfile \
		      "A photo truthtable file for this field (actually a format, with arguments run camCol field)"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$objdir == ""} {
      if ![info exists openit] {
	 error "Please use -dir to specify a directory (maybe .) or\n\t set_run to specify a run and column"
      }
      
      set objdir $openit(objdir)
      set aidir $openit(aidir)
   }

   global displayed_band
   set displayed_band "";		# we don't know which band is displayed

   regsub {^~/} $objdir $env(HOME)/ objdir

   if [info exists table] {
      catch {
	 if !$index_table {
	    index_table table -delete
	 }
	 objfileClose table
      }
   }
   
   if [info exists calibs] {
      calib1byframeDel $calibs; unset calibs
   }
   #
   # Open truthtable if provided
   #
   if {$truthfile != ""} {
      if {[regexp {^h[0-9]+$} $truthfile] &&
	  ![catch { set type [handleType $truthfile] }]} {
	 if {$type != "CHAIN"} {
	    error "$truthfile is not a CHAIN"
	 }
      } elseif {[catch {
	 regexp {^0*([0-9]+)$} $openit(run) {} r
	 set truthfile [format $truthfile $r $openit(camCol) $field]
	 open_mds_truthtable $truthfile
      }]} {
	 open_truthtable $truthfile
      }
   }
   #
   # If the field is non-numeric, it's an ID for a summary table
   #
   if ![regexp {^[0-9]+$} $field] {
      if [info exists openit(is_fpObjc)] {
	 set is_fpObjc $openit(is_fpObjc)
      } else {
	 set is_fpObjc 0
      }
      
      if $is_fpObjc {
	 set objc_prefix_list "fpObjc tsObj"
      } else {
	 set objc_prefix_list "tsObj fpObjc"
      }

      foreach objc_prefix $objc_prefix_list {
	 set file $objdir/$objc_prefix-$field.fit
	 if [file exists $file] {
	    break
	 }
	 unset file
      }
      if ![info exists file] {
	 error "I cannot find $objdir/{[join $objc_prefix_list {,}]}-$field.fit"
      }

      set aifile $objdir/fpAtlas-$field.fit
      set tstfile $objdir/fpObjc-$field-TST.fit
      if $noAI {
	 set aifile ""
      }
      if $noTST {
	 set tstfile ""
      }
      
      set table [objfileOpen $file $aifile 1 $tstfile]
      keylset table is_summary_file 1

      keylset table truth_table $truthfile
      keylset table many_fields 1
      keylset new_table rerun ""

      if ![info exists fieldparams] {
	 set filterlist [keylget table filters]
	 set fieldparams [fieldparamsNew $filterlist]
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
	 loop i 0 [llength $filterlist] {
	    handleSet $fieldparams.frame<$i>.flux20 \
	     $flux20_nominal([lindex $filterlist $i])
	 }
      }

      if $index_table {
	 index_table table $file
      } elseif !$is_fpObjc {
	 keylset table index quiet
      }
      
      return $table
   }
   #
   # Not a summary table; proceed
   #
   if ![info exists openit] {
      error "Please use set_run to specify a run and column"
   }
   
   set openit(field) $field
   foreach el "col dir asdir fangdir objdir psdir reprocess run" {
      if ![info exists $el] {
	 set $el [set openit($el)]
      }
   }

   set hdu [get_file_names $dir $objdir $run $col $field $reprocess \
		file aifile tstfile]
   if $next_hdu {
       if $openit(is_fpObjc) {
	   error "You can only use -nextHdu with tsObj files"
       }
       if {$openit(hdu) == -1} {
	   error "No table is open; you cannot use -nextHdu"
       }
       if {$hdu != $openit(hdu) + 1} {
	   set next_hdu 0
       }
   }

   set OBJfd ""
   if [info exists table] {
      if $next_hdu {
	 set OBJfd [keylget table OBJfd]
      } else {
	 if {$openit(hdu) >= 0} {
	    objfileClose table
	 }
      }
   }

   if $noAI {
      set aifile ""
   }
   if $noTST {
      set tstfile ""
   }
   if [catch {
      set new_table [objfileOpen $file $aifile $hdu $tstfile $OBJfd]
   } msg] {
      if [info exists table] {
	 unset table
      }
      regexp {^0*([0-9]+)$} $openit(run) {} run
      return -errorinfo $msg -code error "Failed to open field $run-$openit(camCol)-$field"
   }
   set openit(hdu) $hdu
   keylset new_table rerun $openit(reprocess)

   if {$truthfile != ""} {
      keylset new_table truth_table $truthfile
   }

   set filters [keylget new_table filters]
   set ref_color [lindex $filters [keylget new_table ref_band]]

   if [catch {
       set file $psdir/psField-$run-$col-[format %04d $field].fit

       set hdr [hdrReadAsFits [hdrNew] $file]
       if [catch {
       keylset new_table pspVersion [hdrGetAsAscii $hdr "VERSION"]
       } msg] {
	  echo "XXX $msg ($file); faking version string"
	  keylset new_table pspVersion "(unknown)"	  
       }
       hdrDel $hdr
      
       set calibs [offsetsRead $field $ref_color $file $filters]
      
       set asTransfile $asdir/asTrans-$run.fit
       if ![file exists $asTransfile] {
	  set asTransfile $asdir/asTrans-$run-$col.fit;# look for -col form
	  if ![file exists $asTransfile] {
	     set asTransfile $asdir/$col/asTrans-$run.fit
	  }
	  if ![file exists $asTransfile] {
	     set asTransfile $asdir/$col/asTrans-$run-$col.fit
	  }
       }
       
       set filterlist [keylget new_table filters]
       if [catch {
	  set real_field [exprGet $calibs.field]; # use the field that's in the psField file in
	  				# case we're faking things

	  astrom_read $real_field $real_field $col astroms $filterlist \
	      $asTransfile node incl
       } msg] {
	  echo Failed to read TRANS from $asTransfile: $msg

	  set node 0; set incl 0
	  foreach f $filterlist {
	     set astroms($f) [chainNew "TRANS"]
	     set trans [transNew]
	     chainElementAddByPos $astroms($f) $trans 0
	     handleDel $trans
	  }
       }
      
      keylset new_table incl $incl
      keylset new_table node $node
      
      if ![info exists fieldparams] {
	 set fieldparams [fieldparamsNew $filterlist]

	 #set bin [expr 2048/$ncol];	# XXX
	 set bin 1;			# XXX
	 loop i 0 [llength $filterlist] {
	    foreach rc "row col" {
	       handleSet $fieldparams.frame<$i>.${rc}Binning $bin
	    }
	 }
      }
      
      if 1 {				# Frames
	 global is_coadd
	 
	 set fieldstat [read_fieldstat $dir $run $col $field]
	 
	 loop i 0 [llength $filterlist] {
	    set sky($i) [exprGet $fieldstat.sky_frames<$i>]
	    handleSet $fieldparams.frame<$i>.global_sky $sky($i)
	    
	    foreach t "deV exp deV_model exp_model" {
	       handleSetFromHandle \
		   $fieldparams.frame<$i>.${t}_ap_correction \
		   $fieldstat.[string tolower ${t}_ap_correction]<$i>
	       handleSetFromHandle \
		   $fieldparams.frame<$i>.${t}_ap_correctionErr \
		   $fieldstat.[string tolower ${t}_ap_correctionErr]<$i>
	    }
	 }

	 fieldstatDel $fieldstat
      } else {				# PSP
	 loop i 0 [llength $filterlist] {
	    set sky($i) [exprGet $calibs.calib<$i>->sky]
	 }
      }

      loop i 0 [llength $filterlist] {
	 set f [lindex $filterlist $i]
	 
	 set trans [chainElementRemByPos $astroms($f) 0]
	 handleSetFromHandle $fieldparams.frame<$i>.toGCC &$trans
	 
	 assert {[chainSize $astroms($f)] == 0}
	 handleDel $trans; chainDel $astroms($f)
	 
	 handleSetFromHandle $fieldparams.frame<$i>.flux20 \
	     $calibs.calib<$i>->flux20

	 if {[info exists is_coadd] && $is_coadd} {
	    if {$i == 0} {
	       echo "Coadd: fixing flux20 by 1e3"
	    }
	    handleSet $fieldparams.frame<$i>.flux20 [expr 1e3*[exprGet $fieldparams.frame<$i>.flux20]]
	 }
	 
	 set skyreg [binregionNewFromConst $sky($i)]
	 handleSetFromHandle $fieldparams.frame<$i>.sky &$skyreg
	 handleDel $skyreg; unset skyreg

	 handleSetFromHandle $fieldparams.frame<$i>.psf $calibs.calib<$i>->psf
	 handleSet $calibs.calib<$i>->psf 0
      }
      handleSet $fieldparams.ref_band_index [keylget new_table ref_band]
   } msg] {
      echo Failed to set fieldparams: $msg
   }
   # Get flux20 values from nfcalib if possible
   if [catch {
      get_flux20_from_nfcalib flux20 $openit(nfcalibdir) $openit(run) $openit(camCol) $field
   } msg] {
      echo "Unable to read flux20: $msg" 
   } else {
      loop i 0 [llength $filterlist] {
	 handleSet $fieldparams.frame<$i>.flux20 $flux20([lindex $filterlist $i])
      }
   }

   if $index_table {
      index_table new_table $file $hdu
   }

   return $new_table
}


proc closeit {args} {
   global calibs table openit

   set opts [list \
		 [list [info level 0] "Cleanup after openit"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if [info exists table] {
      index_table table -delete
      objfileClose table
   }

   if [info exists calibs] {
      calib1byframeDel $calibs; unset calibs
   }
}

#
# Read flux20 from an nfcalibration, if possible.
#
# The flux20 is returned as an array, indexed by filtername
#
proc get_flux20_from_nfcalib {_flux20 nfcalibdir run camCol field} {
   #echo RHL in get_flux20_from_nfcalib
   upvar $_flux20 flux20

   set file "$nfcalibdir/calibPhotom-$run-$camCol.fits"
   set nfcalibs [fits2Schema $file NFCALIB]
   
   set first [chainElementGetByPos $nfcalibs 0]
   set field0 [exprGet $first.field]
   handleDel $first

   set calib [chainElementGetByPos $nfcalibs [expr $field - $field0]]
   set i 0
   foreach f "u g r i z" {
      set flux20($f) [expr 10/[exprGet $calib.nmgypercount<$i>]]
      incr i
   }
   handleDel $calib

   chainDestroy $nfcalibs genericDel
}

#
# Fix the flux20 to allow for fpC_scale
#
proc fix_flux20 {args} {
   global fieldparams

   set set_value 0
   set opts [list \
		 [list [info level 0] \
 "Fix flux20 to allow for the inputs being scaled by fpC_scale"] \
		 [list <scale> DOUBLE 1.0 scale "Desired scale factor"] \
		 [list -set_value CONSTANT 1 set_value "Set the values to scale"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   loop i 0 [exprGet $fieldparams.ncolor] {
      handleSet $fieldparams.frame<$i>.flux20 \
	  [expr $set_value ? $scale : $scale*[exprGet $fieldparams.frame<$i>.flux20]]
   }
}

#
# Return the name of a global array that maps object IDs to row numbers
#
proc index_table {_table file {hdu 1}} {
   upvar $_table table
   global _table_index

   if [info exists _table_index] {
      unset _table_index
   }

   if [regexp {^-del} $file] {		# delete index
      if [keylget table index index] {
	 keyldel table index
      }
      return
   } elseif [regexp {^-iden} $file] {	# identity map
      loop i 0 [keylget table OBJnrow] {
	 set _table_index([expr $i+1]) $i
      }
   } else {
      set ch [fits2Schema $file FOO -hdu $hdu]
      if {[chainSize $ch] == 0} {
	 ;
      } else {
	 set id [vFromChain $ch id]
	 
	 loop i 0 [vectorExprGet dimen($id)] {
	    set _table_index([vectorExprGet $id<$i>]) $i
	 }
	 
	 vectorExprDel $id
      }
      
      chainDestroy $ch genericDel
   }

   keylset table index _table_index
}

proc get_file_names {dir objdir run col field reprocess \
			 _file _aifile {_tstfile ""}} {
   global openit
   
   upvar $_file file  $_aifile aifile
   if {$_tstfile != ""} {
      upvar $_tstfile tstfile
   }

   if $openit(is_fpObjc) {
      set file $objdir/fpObjc-$run-$col-[format %04d $field].fit
      set hdu 1
   } else {
      set hdr [hdrNew]

      if [catch {
	 set files \
	     [glob $objdir/tsObj-$run-$col-$reprocess-[format %04d $field].fit]
      }] {				# no exact match
	 if [catch {
	    set files [glob $objdir/tsObj-$run-$col-$reprocess-????.fit]
	 }] {
	    set files [glob $objdir/tsObj-$run-$col-\[0-9\]*-????.fit]
	 }
      }
      foreach file $files {
	 regexp {([0-9][0-9][0-9][0-9])\.fit$} $file foo n
	 regsub {^0*} $n "" n
	 if {$n > $field} {		# cannot be the right one
	    continue
	 } elseif {$n == $field} {	# exact match
	    set field0 $field
	    set nfields 1
	 } else {
	    hdrReadAsFits $hdr $file
	    set field0 [hdrGetAsInt $hdr "FIELD0"]
	    set nfields [hdrGetAsInt $hdr "NFIELDS"]
	 }

	 set h [expr $field - $field0 + 1]

	 if {$h >= 1 && $h <= $nfields} {
	    if {$reprocess == "?"} {	# look for youngest
	       if ![info exists mtime] {
		  set mtime [file mtime $file]
	       } else {
		  set mtime1 [file mtime $file]
		  if {$mtime1 < $mtime} {# older than the one we saw before
		     continue
		  }
		  set mtime $mtime1
	       }
	       set tsObjFile $file; set hdu $h
	    } else {
	       set tsObjFile $file; set hdu $h
	       break
	    }
	 }
      }
      hdrDel $hdr

      if ![info exists tsObjFile] {
	 error "Cannot find a tsObj file for field $field in $objdir"
      }
      set file $tsObjFile
   }

   set aifile fpAtlas-$run-$col-[format %04d $field].fit
   set tstfile fpObjc-$run-$col-[format %04d $field]-TST.fit
   foreach f "aifile tstfile" {
      foreach d "objdir dir" {
	 set fullname [set $d]/[set $f]
	 if [file exists $fullname] {
	    set $f $fullname
	    break
	 }
      }
   }

   return $hdu
}

#
# Reconstruct colour $c of field $field; rm and cm may be used as
# "show_ids $rm $cm" if defined. $bkgd is the background image to add
# the atlas images too; if it's "binned" the binned output image will
# be dithered and used; if it's "binnedSky" the 4x4 binned sky will be
# used as the image's background (along with noise); if it's "brightSky"
# the local sky subtracted before faint objects are found will be reinstated
#
# If the bkgd is "noreg", no region will be set (but cm/rm will be)
#
# If mask is true, read the mask too
#
# If select is provided, it's the name of a procedure taking one or two
# arguments, a handle to an OBJC_IO and an optional band (default: 2);
# only if it returns true will the object be displayed. For details say
# \"help select\".
#
proc recon {c field {_rm ""} {_cm ""} {mask 0} {bkgd ""} {select ""} {ids_at_peaks 0}} {
   if {$_cm != "" && $_cm != ""} {
      upvar $_rm rm  $_cm cm
   }
   global openit table

   set sky 0;				# value to add to all atlas images

   if {$bkgd == "none"} {
      set reg "none"
   } elseif {$bkgd == "binned" || $bkgd == "binnedSky" ||
	     $bkgd == "brightSky" || $select == "binned"} {
      if {[phRandomIsInitialised]} {
	 set rand [lindex [handleListFromType RANDOM] 0]
      } else {
	 upvar rand rand
	 set rand [phRandomNew -type 2 1000000]
	 echo Setting \$rand
      }
      
      if {$select == "binned"} {
	 set reg [unbin $c $openit(field) $rand -binned]
      } else {
	 set reg [unbin $c $openit(field) $rand]
      }

      if {$bkgd == "binnedSky" || $bkgd == "brightSky"} {
	 #
	 # add sky and some noise (neglect the 1/sqrt(12) added by dithering)
	 #
	 set file \
	     fpFieldStat-$openit(run)-$openit(col)-[format %04d $openit(field)].fit
	 
	 set hdr [hdrReadAsFits [hdrNew] $openit(dir)/$file]
	 set filters [hdrGetAsAscii $hdr "FILTERS"]; hdrDel $hdr
	 
	 set chain [fits2Schema $openit(dir)/$file _FIELDSTAT]
	 set fieldstat [chainElementGetByPos $chain 0]
	 
	 set if [lsearch $filters $c]
	 set sky [exprGet $fieldstat.sky_frames<$if>]
	 set sigpix [expr sqrt(1-1/16.0)*[exprGet $fieldstat.sigpix<$if>]]
	 handleDel $fieldstat; chainDestroy $chain genericDel

      	 if {$bkgd == "brightSky"} {
	    set sky 0;			# we'll add it in later
	 }
	 regIntGaussianAdd $reg $rand $sky $sigpix
      }
   } else {
      set reg $bkgd
   }
   #
   # Is $select actually a logical expression?
   #
   if {$select != "binned"} {
      set select [expand_logical_select $select]
   
      global openit table
      
      if ![info exists openit] {
	 error "Please use set_run to specify a run and column"
      }
      
      if {$field == "table"} {
	 set isTable 1
	 set field $openit(field)
      } else {
	 set isTable 0
      }
      
      set hdu [get_file_names $openit(dir) $openit(objdir) \
		   $openit(run) $openit(col) $field \
		   $openit(reprocess) file aifile tstfile]
      
      if {$reg == "none"} {
	 set hdr [hdrReadAsFits [hdrNew] $aifile]
	 set nrow [hdrGetAsInt $hdr "NROW"]
	 set ncol [hdrGetAsInt $hdr "NCOL"]
	 hdrFreeAll $hdr; hdrDel $hdr

	 set reg [regNew $nrow $ncol]; regIntSetVal $reg 10
      }

      if $isTable {
	 set reg [reconstruct_frame -isTable table -sky $sky \
		      $hdu $aifile $c rm cm $reg $select $tstfile]
      } else {
	 set reg [reconstruct_frame $file -sky $sky \
		      $hdu $aifile $c rm cm $reg $select $tstfile]
      }
   }

   if {$bkgd == "brightSky"} {
      set sky [recon_sky -noscale $c $field bin_row bin_col shift]
      regMultiplyWithDbl $sky -1

      if {$select == "binned"} {
	 set bin_row [expr $bin_row/4]; set bin_col [expr $bin_col/4]
	 set shift   [expr $shift - 2]
      }
      
      set skyreg [binregionNew]
      handleSetFromHandle $skyreg.reg &$sky; handleDel $sky
      foreach v "bin_row bin_col shift" {
	 handleSet $skyreg.$v [set $v]
      }
      
      skySubtract $reg $reg $skyreg $rand
      
      binregionDel $skyreg
   }

   if {$select == "binned"} {
      return $reg
   }
      
   set filters [keylget table filters]
   if {[lsearch $filters $c] < 0} {	# reconstruct_frame already complained
      set c [lindex $filters 0]
   }
   
   if $mask {
      declare_schematrans [hdrGetAsInt $reg.hdr "NFILTER"]
      
      read_mask $c $field $reg
   }
   
   return $reg
}

#
# Read a mask from an fpM file, and maybe bind it to a region
#
proc read_mask {c field {reg ""}} {
   global openit

   set file [get_mask_filename "" $openit(run) $c $openit(col) $field]
   set dirlist [list $openit(objdir) $openit(dir) $openit(corrdir)]

   foreach dir $dirlist {
      if [uncompress_file $dir/$file] {
	 set mask [get_mask $dir/$file]
	 break;
      }
   }
   if ![info exists mask] {
      echo "File $file is in none of $dirlist"
      return
   }

   if {$reg == ""} {
      return $mask
   } else {
      handleSetFromHandle $reg.mask &$mask
      handleDel $mask

      return $reg
   }
}

proc unbin {args} {
   global openit

   set binned 0

   set opts [list \
		 [list [info level 0] "Unbin an fpBIN frame"] \
		 [list <col> STRING "" col "Desired column"] \
		 [list <field> INTEGER 0 field "Desired field"] \
		 [list <rand> STRING "" rand "Handle to a PHRANDOM"] \
		 [list -binned CONSTANT 1 binned "Leave region 4x4 binned"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![info exists openit] {
      error "Please use set_run to specify a run and column"
   }

   set reg [regReadAsFits [regNew] \
     $openit(dir)/fpBIN-$openit(run)-$col$openit(col)-[format %04d $field].fit.R]

   if $binned {
      return $reg
   }
   
   set expanded [regUnbin $reg $rand]
   regDel $reg

   return $expanded
}

#
# Read the sky image that was subtracted
#
proc recon_sky {args} {
   global field openit fieldparams
   
   if ![info exists field] { set field 0 }

   set count_objects 0;			# just count OBJECT_MODELs?
   set noscale 0;			# Remove the sky's scale factor?
   set skyError 0;			# read the skyErr (not sky) HDU?
   set objectModels 0;			# read the object models (not sky) HDU?
   set starModels 0;			# bright star models; not supported

   set opts [list \
		 [list [info level 0] "Reconstruct the sky level for a field

 (assumes that you've used set_run to specify run/camCol)"] \
		 [list <filter> STRING ""     c "Name of desired filter"] \
		 [list {[field]} INTEGER $field f "Desired field"] \
		 [list -noscale CONSTANT 1 noscale "Don't unscale the sky"] \
		 [list -shift INTEGER 0 dshift \
		      "Scale up sky level by this many bits"] \
		 [list {[bin_row]} STRING "" _bin_row \
		      "Variable to return BIN_ROW from header"] \
		 [list {[bin_col]} STRING "" _bin_col \
		      "Variable to return BIN_COL from header"] \
		 [list {[shift]} STRING "" _shift \
		      "Variable to return SHIFT from header"] \
		 [list -error CONSTANT 1 skyError "Read skyError, not sky"]\
		 [list -objectModels CONSTANT 1 objectModels \
		      "Read OBJECT_MODELs, not sky"]\
		 [list -nobjectModels CONSTANT 1 count_objects \
		      "Just print the number of objects"]\
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if $count_objects {
      set objectModels 1
   }
   if {$skyError && $starModels && $objectModels} {
      error "Please choose between -error and -objectModels"
   }

   set file $openit(dir)/fpBIN-$openit(run)-$c$openit(col)-[format %04d $f].fit

   if $objectModels {
      global env
      set hdu 4;				# OBJECT_MODELs HDU

      initFitobj "$env(PHOTO_DIR)/lib/rgalprof.dat"      

      set objectModels [fits2Schema $file OBJECT_MODEL -existingType -hdu $hdu]
      if $count_objects {
	 set nobj [chainSize $objectModels]
	 chainDestroy $objectModels genericDel

	 return $nobj
      }
      
      set ic [lsearch [split [exprGet $fieldparams.filters] {}] $c]
      #set psf [handleBindFromHandle [handleNew] $fieldparams.frame<$ic>.psf]
      set psf [dgpsfNew -sigmax1 1]; #handleSet $psf.sigma [exprGet $psf.sigmax1]
      set reg [regNew 1489 2048]
      regClear $reg

      loop i 0 [chainSize $objectModels] {
	 set om [chainElementGetByPos $objectModels $i]
	 set model [objectModel_to_region $om $psf]
		    
	 if {$model == ""} {
	    continue
	 }
	 #
	 # and add that model into $out
	 #
	 regIntLincom $reg $model 0 1 1 -intersect
      }
      dgpsfDel $psf

      chainDestroy $objectModels genericDel

      return $reg
   } elseif $starModels {
      set hdu 3
   } elseif $skyError {
      set hdu 2;				# Sky Error
   } else {
      set hdu 1;				# Sky
   } 

   if $noscale {
      set reg [regNew -type S32]
   } else {
      set reg [regNew -type FL32]
   }

   if {$hdu < 3} {			# sky
      if [catch {
	 set reg [regReadAsFits -keeptype -hdu $hdu $reg $file]
      } msg] {
	 error "Failed to read $file: $msg"
      }
      
      foreach v "bin_row bin_col shift" {
	 if {[set _$v] != ""} {
	    upvar [set _$v] $v
	    set $v [hdrGetAsInt $reg.hdr [string toupper $v]]
	 }
      }
      
      if !$noscale {
	 set shift [hdrGetAsInt $reg.hdr SHIFT]
	 regMultiplyWithDbl $reg [expr 1.0/(1 << ($shift - $dshift))]
      }
   }

   return $reg
}

proc recon_sky_fields {args} {
   global data_root

   set fake_flux20 0;			# use canonical flux20 values
   set normalise 0;			# normalise median of each chip?
   set noscale 0;			# don't scale the sky level to DN?
   set overlay 0;			# overlay display with field boundaries
   set skyOffsets 0;			# Correct for chip-to-chip sky offsets
   set skyError 0;			# read the skyErr (not sky) HDU?
   set testbed 0;			# we're looking at a testbed run
   set opts [list \
		 [list [info level 0] ""] \
		 [list <run> STRING 0 run "Run of interest"] \
		 [list <col> INTEGER 0 col0 "(First) column of interest"] \
		 [list <filter> STRING ""     c "Name of desired filter"] \
		 [list <field0> INTEGER 0 f0 "Desired starting field"] \
		 [list {[field1]} INTEGER 0 f1 "Desired ending field (default: field0)"] \
		 [list -col1 INTEGER 0 col1 \
		      "Desired ending column"] \
		 [list -testbed CONSTANT 1 testbed \
		      "Look for outputs in \$data_root/testbed"] \
		 [list -rootdir STRING "$data_root" root_dir \
		      "Root for SDSS data/input directories"] \
		 [list -rerun STRING "" rerun \
		      "Desired reprocessing number (default: youngest)"] \
		 [list -gutter INTEGER 3 gutter \
		      "Width of gutter between columns"] \
		 [list -fake_flux20 CONSTANT 1 fake_flux20 \
		      "Use default values for flux20"] \
		 [list -sao INTEGER 0 sao \
		      "Display region on this sao"] \
		 [list -noscale CONSTANT 1 noscale \
		      "Don't scale the sky level back to DN"] \
		 [list -overlay CONSTANT 1 overlay \
		      "Overlay sao display with field boundaries"] \
		 [list -skyOffsets CONSTANT 1 skyOffsets \
		      "Correct flux20 values for known chip-to-chip sky offsets"] \
		 [list -shift INTEGER 0 dshift \
		      "Scale up sky level by this many bits"] \
		 [list -normalise CONSTANT 1 normalise \
		      "Fix each chip to have same median value"] \
		 [list -error CONSTANT 1 skyError "Read skyError, not sky"]\
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if $testbed {
      append root_dir "/testbed"
   }
   if {$overlay && $sao == 0} {
      set sao 1
   }

   if $noscale {
      set noscale_cmd "-noscale"
   } else {
      set noscale_cmd "-shift $dshift"
   }

   if $skyError {
      set skyError "-error"
   } else {
      set skyError ""
   }

   if {$f1 == 0} {
      set f1 $f0
   }
   if {$col1 == 0} {
      set col1 $col0
   }

   set nfield  [expr $f1 - $f0 + 1]
   set ncamCol [expr $col1 - $col0 + 1]

   set_run -root $root_dir -rerun $rerun $run $col0
   
   set reg [eval recon_sky $noscale_cmd $c $f0 $skyError]
   set nrow [exprGet $reg.nrow]; set ncol [exprGet $reg.ncol]
   #
   # The sky regions used to have an extra row at the top/right
   #
   if {$nrow == 17} {
      incr nrow -1; incr ncol -1;
   }
   #
   # Drop the last row; it's approximately the frame overlap
   #
   incr nrow -1;		
   
   if $noscale {
      set type S32
   } else {
      set type FL32
   }
   set sky [regNew -type $type [expr $nfield*$nrow] \
		[expr $ncamCol*$ncol + ($ncamCol - 1)*$gutter]]
   regIntSetVal $sky [expr int(0.9*[regPixGet $reg 0 0])]

   regDel $reg
   #
   # Prepare to fake the flux20 values
   #
   set flux20(1) [list 1357.25 3300.25 2190.35 1505.28 319.559]
   set flux20(2) [list 1309.22 2773.16 2161.56 1286.26 312.701]
   set flux20(3) [list 1353.19 2888.34 2271.66 1607.31 277.037]
   set flux20(4) [list 1336.89 2865.81 2139.08 1568.84 269.30]
   set flux20(5) [list 1453.39 2631.91 2159.19 1500.22 436.774]
   set flux20(6) [list 897.046 2755.53 2053.80 1469.94 293.597]
   #
   # Do the work
   #
   loop camCol $col0 [expr $col1 + 1] {
      set scol0 [expr ($camCol - $col0)*($ncol + $gutter)]

      set_run -root $root_dir -rerun $rerun $run $camCol

      loop f $f0 [expr $f1 + 1] {
	 if {$f == $f0} {		# set flux20
	    if $fake_flux20 {
	       set flux20($camCol) \
		   [lindex $flux20($camCol) [lsearch "u g r i z" $c]]
	       if {$c == "r"} {
		  array set fiddle "1 1.0 2 0.98 3 0.98 4 1.00 5 1.00 6 1.00"
		  set flux20($camCol) [expr $fiddle($camCol)*$flux20($camCol)]
	       }
	    } else {
	       global table fieldparams
	       set table [openit $f]
	       set ifilter [lsearch [keylget table filters] $c]
	       set flux20($camCol) \
		   [exprGet $fieldparams.frame<$ifilter>.flux20]
	       if $skyOffsets {
		  set flux20($camCol) \
		      [expr $flux20($camCol)*(1 + [SkyOffset $camCol $c])]
	       }
	    }
	    if {$camCol == $col0} {
	       set fac 1.0
	    } else {
	       set fac [expr $flux20($col0)/$flux20($camCol)]
	    }
	 }

	 if [catch {
	    set reg [eval recon_sky $noscale_cmd $c $f $skyError]
	 } msg] {
	    echo $msg
	    set reg [regNew $nrow $ncol]; regClear $reg
	 }

	 if {$fac != 1.0} {
	    regMultiplyWithDbl $reg $fac
	 }

	 set skreg [subRegNew $sky $nrow $ncol [expr ($f - $f0)*$nrow] $scol0]
	 if [catch {
	    set treg [subRegNew $reg $nrow $ncol 1 0]
	 }] {
	    echo Failed to recon sky for camCol $camCol, field $f
	    set treg [regNew -type S32 $nrow $ncol];
	    regIntSetVal $treg 10
	 }
	 regIntCopy $skreg $treg;
	 regDel $skreg; regDel $treg; regDel $reg
      }
   }
   #
   # Fix median of each chip?
   #
   if $normalise {
      set med [regNew 1 [exprGet $sky.ncol]]
      regMedianFindByColumn $sky $med

      set mmean 0
      loop camCol $col0 [expr $col1 + 1] {
	 set scol0 [expr ($camCol - $col0)*($ncol + $gutter)]

	 set smed($camCol) [subRegNew $med 1 $ncol 0 $scol0]
	 set vals [regStatsFind $smed($camCol)]
	 set mean($camCol) [keylget vals mean]
	 set mmean [expr $mmean + $mean($camCol)]
      }
      set mmean [expr $mmean/($col1 - $col0 + 1)]

      regSetWithDbl $med $mmean
      loop camCol $col0 [expr $col1 + 1] {
	 regSetWithDbl $smed($camCol) $mean($camCol)
	 regDel $smed($camCol)
      }

      regDivByRow $sky $med -scale $mmean
      
      regDel $med
   }

   if {$sao > 0} {
      display_region $sky 0 0 $sao
      if $overlay {
	 saoReset $sao
	 overlay_recon_sky_fields -sao $sao $ncamCol $f0 $f1 -height $nrow
      }
   }
   
   return $sky
}

###############################################################################
#
# Create a ppm file for an object at the given position
#
proc make_family_ppm {args} {
   global openit objcIo OBJECT1 table

   set id 0;				# "rowc" is id; colc is omitted
   set radec 0;				# coords are ra/dec

   set opts [list \
		 [list [info level 0] \
 "Create a ppm file for an object at the given position.  Assumes that you've
 run set_run; e.g.

   set_run 756 3 129; set file \[make_family_ppm -radec 135.1223 -0.1449\]
 "] \
		 [list <rowc> DOUBLE "" rowc "Desired rowc, ra, or id"] \
		 [list {[colc]} STRING "" colc "Desired colc or dec"] \
		 [list -ppmArgs STRING "-remove 0 -nodisplay" ppmArgs \
		 "Arguments to mtv_blend_ppm. E.g. \"\" to display ppm file"] \
		 [list -id CONSTANT 1 id \
		      "First argument is ID"] \
		 [list -radec CONSTANT 1 radec \
		      "Position is ra/dec not rowc/colc"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![info exists openit] {
      error "Please run set_run <run> <camCol> <field> and try again"
   }

   if $id {
      set id $rowc
      if {$colc != ""} {
	 error "Please don't specify a colc with an object ID"
      }
      if $radec {
	 error "PLease don't specify an ra/dec as well as an object ID"
      }
   }

   if $radec {
      set radec "ra"
   } else {
      set radec ""
   }

   if !$id {
      set id [eval find_objc table $rowc $colc $radec]
   }
   
   read_objc table $id
   if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
      read_objc table "next"
   } else {
      read_parent table $objcIo
   }

   return [eval mtv_blend_ppm table [exprGet $objcIo.id] $ppmArgs]
}

###############################################################################
#
# Zeljko code from photoQA
#
# return canonical sky offsets for a given chip
#
proc SkyOffset {column filter} {

# determined on 03/05/01 as medians for 28 runs later than 7xx (starting with 994) 
# the values are in good agreement with 1-sigma clipped means
# the mean uncertainty is ~0.002 in gri, ~0.003 in z, and ~0.004 in u band

     #Column 1
       set skyOff(1,u)   -0.004
       set skyOff(1,g)   -0.005
       set skyOff(1,r)   -0.004
       set skyOff(1,i)   -0.009
       set skyOff(1,z)   -0.021
     #Column 2
       set skyOff(2,u)   -0.001
       set skyOff(2,g)    0.008; # 0.002
       set skyOff(2,r)   -0.003
       set skyOff(2,i)    0.015
       set skyOff(2,z)    0.047
     #Column 3
       set skyOff(3,u)   -0.011
       set skyOff(3,g)   -0.015; # 0.002
       set skyOff(3,r)    0.021
       set skyOff(3,i)    0.001
       set skyOff(3,z)    0.018
     #Column 4
       set skyOff(4,u)   -0.018
       set skyOff(4,g)    0.004; # 0.000
       set skyOff(4,r)   -0.004
       set skyOff(4,i)    0.000
       set skyOff(4,z)    0.013
     #Column 5
       set skyOff(5,u)    0.024
       set skyOff(5,g)    0.004; # -0.002
       set skyOff(5,r)    0.000
       set skyOff(5,i)    0.001
       set skyOff(5,z)   -0.021
     #Column 6
       set skyOff(6,u)    0.049
       set skyOff(6,g)   -0.0155; # 0.004
       set skyOff(6,r)    0.003
       set skyOff(6,i)   -0.006
       set skyOff(6,z)   -0.032

     return $skyOff($column,$filter)
}

###############################################################################
#
# Overlay a recon_sky_fields plot with field boundaries/numbers
#
proc overlay_recon_sky_fields {args} {
   set const 0;

   set opts [list \
		 [list [info level 0] \
 "Overlay a recon_sky_fields plot with field boundaries/numbers"] \
		 [list <ncol> INTEGER 0 ncol "Number of camera columns"] \
		 [list <field0> INTEGER 0 f0 "Desired starting field"] \
		 [list {[field1]} INTEGER 0 f1 "Desired ending field (default: field0)"] \
		 [list -gutter INTEGER 3 gutter \
		      "Width of gutter between columns"] \
		 [list -sao INTEGER 1 sao \
		      "Which saoimage to use"] \
		 [list -height INTEGER 0 h \
		      "Height of a frame (after trimming)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$f1 == 0} {
      set f1 $f0
   }

   if {$h == 0} {
      set h [expr 13 - 2];		# we trim the first and last row
   }
   set w 16
   set ncol [expr $ncol*$w + ($ncol - 1)*$gutter]
   
   saoDrawCache
   loop i 0 [expr $f1 - $f0 + 1] {
      saoDrawPolygon -s $sao \
	  [expr $i*$h] 0 \
	  [expr ($i+1)*$h] 0 \
	  [expr ($i+1)*$h] [expr $ncol-1] \
	  [expr $i*$h] [expr $ncol-1]
      saoDrawText -s $sao [expr $i*$h + 1] -3 [expr $f0 + $i]
   }
   saoDrawCache -off
}

###############################################################################
#
# Reconstruct a frame from its atlas images, and display it.
#
# Can be used as e.g.:
#   hdf; set field 3
#   incr field; show_recon $field r
#   incr field; show_recon $field r
#   incr field; show_recon $field r
#   incr field; show_recon $field r
#
# See also next_frame
#
proc show_recon {field c {fsao 2} {mask 0}} {
   while {[lsearch [saoGetRegion] $fsao] < 0} {
      set reg [regNew 1 1]; saoDisplay $reg -lowerleft; regDel $reg
   }
   
   set reg [display_region [recon $c $field rm cm $mask] 0 $mask $fsao]

   regDel $reg

   if {[lsearch [saoGetRegion] $fsao] < 0} {
      set fsao [lindex [saoGetRegion] 0]
   }

   saoReset $fsao;
   show_ids rm cm $fsao;

   global sao_is_ds9
   if !$sao_is_ds9 {
      saoZoom -s $fsao 0.5
   }
}

#
# Increment the field number, and display that field
#
proc next_field {args} {
   global openit table

   set no_ids 0;			# don't show IDs?
   set ids 0;				# Force IDs even if -fpC
   set ids_at_peaks 1;			# draw object IDs at their peaks,
   ;					# not at {row,col} min
   set fpBIN 0;				# simply return fpBIN file
   set fpC 0;				# simply display the fpC file
   set idR 0;				# simply display the idR file
   set show_peaks 0;			# Show all peaks
   set opts [list \
		 [list next_field \
 "display the next field. The region is saved in the global \$frame_reg"] \
		 [list {[bkgd]} STRING "" bkgd "\
 Region to use a background;
   \"\" for blank,
   \"binned\" to use the fpBIN file
   \"binnedSky\" to use the fpBIN file, with a constant sky and noise added
   \"brightSky\" to use the fpBIN file, along with the local sky that we
                 subtracted, and with noise added\
 "] \
		 [list {[mask]} INTEGER 1 mask "Display the region's mask"] \
		 [list {[filter]} STRING "r" filter \
		      "Name of desired filter"] \
		 [list {[fsao]} INTEGER "2" fsao \
		      "Number of saoimage to use"] \
		 [list -selection_function STRING "" sel \
		      "Proc to select desired objects; see \"help select\""] \
		 [list -field STRING "" ffield "Field to display (or `table')"] \
		 [list -IDs CONSTANT 1 ids \
		      "Show object IDs (default if not -fpC)"] \
		 [list -noIDs CONSTANT 1 no_ids "Don't show object IDs"] \
		 [list -fpBIN CONSTANT 1 fpBIN "Simply return the fpBIN file (but a bkgd of brightSky is honoured)"] \
		 [list -fpC CONSTANT 1 fpC "Simply display the fpC file"] \
		 [list -idR CONSTANT 1 idR "Simply display the idR file"] \
		 [list -label_peaks CONSTANT 1 ids_at_peaks \
		      "Draw object IDs at peak, not corner of bounding box"] \
		 [list -label_corners CONSTANT 0 ids_at_peaks \
		      "Draw object IDs at corner of bounding box, not peak"] \
		 [list -peaks CONSTANT 1 show_peaks \
		      "Show peaks of all detected objects"] \
		 ]
   #
   # Add flag arguments which are equivalent to all the optional arguments;
   # the corresponding values are set to "" or -1, and the variables are
   # given a leading underscore
   #
   foreach el $opts {
      if [regexp {^\[([^]]*)\]$} [lindex $el 0] foo which] {
	 set type [lindex $el 1]
	 set val [lindex $el 2]
	 set var [lindex $el 3]
	 if {$type == "STRING"} {
	    set val ""
	 } else {
	    set val -1
	 }
	 lappend aliases $var
	 lappend opts [lreplace [lreplace $el 2 3 $val _$var] 0 0 "-$which"]
      }
   }

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # Deal with any -options that were used
   #
   foreach alias $aliases {		# use the arguments if provided
      if {[set _$alias] != "" && [set _$alias] >= 0} {
	 set $alias [set _$alias]
      }
   }
   if $fpBIN {
      if {$sel != ""} {
         error "Please don't specify -fpBIN and -select"
      }
      set sel "binned"
   }
   if {$fpC || $idR} {
      if $fpBIN {
         error "Please don't specify -fpBIN and -fpC/idR"
      }
      if {$sel != ""} {
         error "Please don't specify -fpC/idR and -select"
      }
      if !$ids {
	 set no_ids 1
      }
   }
   
   global field id_row id_col frame_reg

   if {$ffield == ""} {		# not from argument list
      if ![info exists field] {
	 error "Please choose a starting field, e.g. set_run run camCol field"
      }
      set ffield [incr field]
   }
   set field $ffield

   if {$field == "table"} {
      set field [keylget table field]
   }

   if [info exists frame_reg] {
      regDel $frame_reg; unset frame_reg
   }

   if $fpC {
      global data_root

      regsub {^0*} $openit(run) {} run

      if [info exists data_root] {
	 set corrdir_real [find_objc_dir $run $data_root "?" "corr" $openit(camCol)]; # i.e. not in data/prs
      }

      if ![info exists corrdir_real] {
	 set corrdir_real "/dev/null"; # fpC certainly won't exist
      }


      set fpCfile [format fpC-$openit(run)-${filter}$openit(col)-%04d.fit $field]
      if {![file exists "$openit(corrdir)/$fpCfile"] &&
	  ![file exists "$openit(corrdir)/$fpCfile.gz"] &&
	  ![file exists "$openit(corrdir)/$fpCfile.Z"] &&
	  ![file exists "$corrdir_real/$fpCfile.Z"]
       } {
	 echo "$fpCfile doesn't exist; running do-frames -fpC"
	 parray openit
	 do-frames -fpC -rerun $openit(reprocess) $run $openit(col) $field -v 2
      }

      set openit(corrdir) [find_objc_dir $run $openit(root_dir) $openit(reprocess) "corr" $openit(col)]
      if {![file exists $openit(corrdir)]} {
	 global data_root
	 if [info exists data_root] {
	    set corrdir [find_objc_dir $run $data_root "?" "corr" $openit(camCol)]
	    set openit(corrdir) $corrdir
	 }
	 
      }
      #cmdtrace 1
      declare_schematrans 5
      #cmdtrace 15

      set got_fpC 0
      foreach dir [list $openit(corrdir) $corrdir_real] {
	 catch {
	    if {[glob "$dir/$fpCfile*"] != ""} {
	       set frame_reg [regReadAsFits [regNew] "$dir/$fpCfile"]
	       set fpCfile "$dir/$fpCfile"
	       set got_fpC 1
	    }
	 }
      }

      if {!$got_fpC} {
	 error "Unable to find $fpCfile"
      }
			 
      if $mask {
	 read_mask $filter $field $frame_reg
      }
      if !$no_ids {
	 recon $filter $field id_row id_col 0 "noreg" $sel $ids_at_peaks
      }
   } elseif $idR {
      set frame_reg [regReadAsFits [regNew] \
			 [format $openit(fielddir)/idR-$openit(run)-${filter}$openit(col)-%04d.fit $field]]
   } else {
      set frame_reg [recon $filter $field id_row id_col $mask $bkgd $sel]
   }

   if {$fsao > 0} {
      display_region $frame_reg 0 $mask $fsao
      
      if {[lsearch [saoGetRegion] $fsao] < 0} {
	 set fsao [lindex [saoGetRegion] 0]
      }
      
      saoReset $fsao;
      
      if !$no_ids {
	 show_ids id_row id_col $fsao;
	 global sao_is_ds9
	 if !$sao_is_ds9 {
	    saoZoom -s $fsao 0.5;
	 }
      }

      set displayed_band [lsearch [keylget table filters] $filter]
   }

   if $show_peaks {
      echo "-peaks is not yet implemented"
   }

   return $frame_reg
}

proc astar {o {band 2}} {
   
}

proc this_field {args} {
   global frame_reg
   
   if [regexp {^-h} [lindex $args end]] {
      echo "This field is equivalent to next_field -field table args"
      return ""
   }
   eval next_field -field table $args

   return $frame_reg
}

proc next_frame {args} {
   global field frame
   
   set field $frame
   eval next_field $args
   set frame $field
}

proc refresh_ids {{fsao 2}} {
   global id_row id_col
   if [info exists id_row] {
      show_ids id_row id_col $fsao
   }
}

###############################################################################
#
# Reconstruct the PSF for a given field; assumes that you've run set_run
#
proc reconOnePSF {args} {
   set show_basis 0;			# return the eigen images?
   set opts [list \
		 [list [info level 0] "\
 Reconstitute and return a given field's PSF from the psField file.
 Assumes that you've run set_run."] \
		 [list <field> INTEGER 0 field "Desired field"] \
		 [list -filter STRING "r" filter \
		      "Name of desired filter"] \
		 [list {-nrow} INTEGER 5 npsf_r \
		      "Number of rows of PSFs to generate"] \
		 [list {-ncol} INTEGER 7 npsf_c \
		      "Number of columns of PSFs to generate"] \
		 [list {-trim} INTEGER 10 trim \
		 "Number of rows/cols to trim from each side of PSF regions"] \
		 [list {-gutter} INTEGER 3 gutter \
		      "Gutter between PSFs in mosaic"] \
		 [list {-rowc} DOUBLE -1 rowc \
		      "Row-center of desired PSF"] \
		 [list {-colc} DOUBLE -1 colc \
		      "Column-center of desired PSF"] \
		 [list {-model} CONSTANT 1 model \
		      "Reconstitute PSFs used for model fitting"] \
		 [list -type STRING "psf" model_type \
	     "Return a model of this type, not just a PSF. E.g. {deV re ab}"] \
		 [list -sigma DOUBLE 0.0 sigma \
		      "Smoothing sigma for centroiding (used with -model)"] \
		 [list -show_basis CONSTANT 1 show_basis \
		      "Return the KL-basis functions for each image"] \
		 ]

   set model 0
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   global env openit

   if ![info exists openit] {
      error "Please use set_run to specify a run and column"
   }

   get_psfBasis psfBasis $field $filter filters

   if $show_basis {
      return [mosaic_basis $psfBasis($filter) $trim]
   }

   if {($rowc >= 0 || $colc >= 0) && ($npsf_r == 5 && $npsf_c == 7)} {
      set npsf_r 0; set npsf_c 0
   }

   if $model {
      initProfileExtract
      
      initFitobj "$env(PHOTO_DIR)/lib/rgalprof.dat"
      initCellFitobj "$env(PHOTO_DIR)/lib/cellprof.dat" [llength $filters]
      set model_band [lsearch $filters $filter]
   } else {
      set model_band -1
   }

   if [catch {
      set reg [reconstruct_psf $psfBasis($filter) $rowc $colc \
		   -nrow $npsf_r -ncol $npsf_c \
		   -model $model_band -type $model_type -sigma $sigma \
		   -trim $trim -gutter $gutter]
   } msg] {
      error "RHL psfBasisDel $psfBasis($filter)"
      psfBasisDel $psfBasis($filter)
      return -code error -errorinfo $msg
   }

   return $reg   
}

proc reconPSF {args} {
   set linear_mosaic 0
   set show_basis 0;			# show the eigen images?
   set opts [list \
		 [list [info level 0] "\
 Reconstitute and return a range of fields' PSFs from psField files.
 Assumes that you've run set_run."] \
		 [list <field0> INTEGER 0 field0 "Desired starting field"] \
		 [list {[field1]} INTEGER 0 field1 \
		      "Desired ending field (default: <field0>)"] \
		 [list -filter STRING "r" filter \
		      "Name of desired filter"] \
		 [list {-nrow} INTEGER 5 npsf_r \
		      "Number of rows of PSFs to generate"] \
		 [list {-ncol} INTEGER 7 npsf_c \
		      "Number of columns of PSFs to generate"] \
		 [list {-trim} INTEGER 10 trim \
		 "Number of rows/cols to trim from each side of PSF regions"] \
		 [list {-gutter} INTEGER 3 sgutter \
		      "Gutter between PSFs in sub-mosaics"] \
		 [list -sao INTEGER 0 sao "Display (and label) region"] \
		 [list -model CONSTANT 1 model \
		      "Return PSFs as used by model fitting code"] \
		 [list -type STRING "psf" model_type \
	     "Return a model of this type, not just a PSF. E.g. {deV re ab}"] \
		 [list -sigma DOUBLE 0.0 sigma \
		      "Smoothing sigma for centroiding (used with -model)"] \
		 [list -linear_mosaic CONSTANT 1 linear_mosaic \
		      "Arrange all mosaics in a row"] \
		 [list -show_basis CONSTANT 1 show_basis \
		      "Show the KL-basis functions for each image"] \
		 ]
   set model 0
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   global openit

   if ![info exists openit] {
      error "Please use set_run to specify a run and column"
   }

   if {$field1 == 0} { set field1 $field0 }

   if $show_basis {
      if $model {
	 error "You may not specify both -model and -show_basis"
      }
      set show_basis "-show_basis"
   } else {
      set show_basis ""
   }

   if $model {
      if {$show_basis != ""} {
	 error "You may not specify both -model and -show_basis"
      }
      set model "-model -type {$model_type} -sigma $sigma"
   } else {
      set model ""
   }

   loop field $field0 [expr $field1 + 1] {
      set gutter 3
      set psf [eval reconOnePSF $field -filter $filter $model $show_basis \
		   -nrow $npsf_r -ncol $npsf_c -trim $trim -gutter $sgutter]

      if {$field == $field0} {
	 #
	 # We need to create the mosaic
	 #
	 set nrow [exprGet $psf.nrow]; set ncol [exprGet $psf.ncol]

	 if $linear_mosaic {
	    if {$show_basis != ""} {
	       set nr 1
	       set nc [expr $field1 - $field0 + 1]
	    } else {
	       set nr [expr $field1 - $field0 + 1]
	       set nc 1
	    }
	 } else {
	    get_mosaic_geometry [expr $field1 - $field0 + 1] nr nc $nrow $ncol
	 }

	 set reg [regNew [expr $nr*$nrow + ($nr - 1)*$gutter] \
		      [expr $nc*$ncol + ($nc - 1)*$gutter]]
	 regIntSetVal $reg 1100
      }

      get_mosaic_index $nr $nc [expr $field - $field0] ir ic

      set sreg [subRegNew $reg $nrow $ncol \
		    [expr $ir*($nrow+$gutter)] [expr $ic*($ncol+$gutter)]]
      regIntCopy $sreg $psf
      regDel $sreg; regDel $psf
   }
   #
   # Display (and label) region?
   #
   if {$sao > 0} {
      display_region $reg 0 0 $sao; saoReset $sao
      
      loop field $field0 [expr $field1 + 1] {
	 get_mosaic_index $nr $nc [expr $field - $field0] ir ic
	 saoDrawText -s $sao \
	     [expr $ir*($nrow+$gutter)] [expr $ic*($ncol+$gutter)] $field
      }
   }
   
   return $reg   
}

###############################################################################
#
# Reconstitute a set of fpC files; assumes that you've run set_run
#
lappend help_procs makeFpC

proc makeFpC {args} {
   global frame_reg openit

   set opts [list \
		 [list [info level 0] \
 "Reconstitute a given field from fpAtlas/fpBIN/fpM files. Assumes that
 you've run set_run. By default, displays region
 "] \
		 [list <dir> STRING "" dir "Directory to write files to"] \
		 [list <field> INTEGER 0 field "Desired field"] \
		 [list -filters STRING "u g r i z" filters \
		      "Filters to reconstruct"] \
		 [list -bkgd STRING "" bkgd "How to generate the background"] \
		 [list -fsao INTEGER 2 fsao "saoimage to use (none if == 0)"] \
		 [list -selection_function STRING "" sel \
		      "Proc to select desired objects; see \"help select\""] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   
   if [catch {
      set calibs [offsetsRead $field [lindex $filters 0] \
		      $openit(psdir)/psField-$openit(run)-$openit(col)-[format %04d $field].fit $filters]
   } msg] {
      echo makeFpC: $msg
      echo Setting sky level to 0
      foreach f $filters {
	 set sky($f) 0
      }
   } else {
      set i 0
      foreach f $filters {
	 set sky($f) [exprGet $calibs.calib<$i>->sky]; incr i
      }
      calib1byframeDel $calibs
   }

   foreach f $filters {
      echo $field $f
      
      next_field -bkgd $bkgd -field $field -filter $f -fsao $fsao -select $sel
      regIntLincom $frame_reg "" $sky($f) 1 0
      regWriteAsFits $frame_reg \
	  [format $dir/fpC-$openit(run)-${f}$openit(col)-%04d.fit $field]
   }
}

###############################################################################
#
# Make summary tables from a plPlugMapM file
#
proc plPlugMap2tsObj {args} {
   global data_root

   set is_fpObjc 1; set writeAI 1; set ignore_rerun 0
   set write_children 0
   set opts [list \
		 [list plPlugMap2tsObj \
 "Write a private fpObjc/tsObj and fpAtlas image table containing all the
 objects in a plPlugMapM.par file
 "] \
		 [list <plPlugMap> STRING "" plug_file \
		 "The plPlugMapM.par file for the plate"] \
		 [list <name> STRING "" file_str \
		 "Identifying string for output files"] \
		 [list -dir STRING "." outdir "Directory to write files in"] \
		 [list -ignoreRerun CONSTANT 1 ignore_rerun "ignore the rerun number in the plPlugMap file"] \
		 [list -noAI CONSTANT 0 writeAI "don't write atlas image files"] \
		 [list -children CONSTANT 1 write_children \
		      "Write objcIo's entire family to the table"] \
		 [list -runs STRING "" allowed_runs \
		      "Only process objcts from these runs"] \
		 [list -test_info INTEGER 0 write_test_info \
		 "write TEST_INFO files"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # Read the plugmap file
   #
   set ch [param2Chain $plug_file PLUGMAPOBJ]

   loop i 0 [chainSize $ch] {
      set obj [chainElementGetByPos $ch $i]

      if {[exprGet -enum $obj.holeType] != "OBJECT"} {
	 continue;
      }

      set run   [exprGet $obj.objId<0>]
      set rerun [exprGet $obj.objId<1>]
      set col   [exprGet $obj.objId<2>]
      set field [exprGet $obj.objId<3>]
      set id    [exprGet $obj.objId<4>]

      lappend ids($run,$rerun,$col,$field) $id
   }
   chainDestroy $ch genericDel
   #
   # Prepare to write output table
   #
   if $writeAI {
      set noAi ""
   } else {
      set noAi "-noAI"
   }
   if $write_test_info {
      set test_info "-test_info"
   } else {
      set test_info ""
   }
   if $write_children {
      set children "-children"
   } else {
      set children ""
   }

   set set_rerun "";			# specify rerun number
   #
   # Actually do the work
   #
   foreach el [array names ids] {
      set tmp [split $el ","]
      set run   [lindex $tmp 0]
      set rerun [lindex $tmp 1]
      set col   [lindex $tmp 2]
      set field [lindex $tmp 3]

      if !$ignore_rerun {
	 set set_rerun "-reprocessing_number $rerun"
      }

      if {$allowed_runs != "" && [lsearch $allowed_runs $run] < 0} {
	 continue;
      }

      echo [format "%-3d %d %d %3d:  %s" $run $rerun $col $field $ids($el)]
      eval set_run -rootdir $data_root $set_rerun -tsObj $run $col
      set table [openit $run]
      if ![info exists my_table] {
	 set my_table [eval open_mytables table $file_str -tsObj -dir $outdir \
			   $noAi $test_info]
      }
      
      foreach id $ids($el) {
	 set objcIo [read_objc table $id]
	 eval write_mytables $children my_table $objcIo table
      }

      objfileClose table
   }
   close_mytables my_table

   return $file_str
}


###############################################################################
#
# Make tsObj files for a subset of user-defined objects
#
#
# A proc to select a set of objects from tsObj files according to 
# a user specified selection criteria (defined in the input proc "select") 
# and write out only these objects into another tsObj file with name
# tsObj-$name.fit. For an example see proc get_bright_stars below
#
proc myObj2tsObj {args} {
    global objcIo
    global table
    global ignore_primary

    set is_fpObjc 1; set writeAI 1;
    set verbose 0
    set family 0
    set write_children 0
    
    set opts [list \
		 [list myObj2tsObj \
 "Write a private fpObjc/tsObj and fpAtlas image table containing all the
 objects that pass selection cuts defined by <select>. For example,
   define name \"RHL\"
   loop c 1 7 {
      myObj2tsObj -tsObj -rerun 54 756 100 200 sel_good \$name -myTable \$name
   }
   close_mytables \$name

   openit -dir . \$name; mtv_list
 "] \
		 [list <run> INTEGER "" run \
		 "Run number"] \
		 [list <col> INTEGER "" col \
		 "Column number"] \
		 [list <startField> INTEGER "" startField \
		 "Starting field number"] \
		 [list <endField> INTEGER "" endField \
		 "Ending field number"] \
		 [list <select> STRING "" select \
		 "Proc to select objects"] \
		 [list <name> STRING "" file_str \
		 "Identifying string for output files"] \
		 [list -rerun STRING "" rerun \
		      "Desired reprocessing number (default: youngest)"] \
		 [list -tsObj CONSTANT 0 is_fpObjc \
		      "write tsObj rather than fpObjc files"] \
		 [list -dir STRING "." outdir "Directory to write files in"] \
		 [list -noAI CONSTANT 0 writeAI "don't write atlas image files"] \
		 [list -children CONSTANT 1 write_children \
		      "Write objcIo's entire family to the table"] \
		 [list -family CONSTANT 1 family \
		      "Write objcIo's entire family to the table"] \
		 [list -test_info INTEGER 0 write_test_info \
		 "write TEST_INFO files"] \
		  [list -myTable STRING "" _my_table \
 "Name of (possibly pre-existing) table variable; will be appended to and not
 closed.  Use \"close_mytables tableVar\" when done."] \
		  [list -callback STRING "" callback \
		      "Call this proc with object as argument before writing entry; return false to skip object"] \
		  [list -verbose CONSTANT 1 verbose "Be chatty"] \
		 ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return;
   }

   #
   # parameters controlling writing output table
   #
   if $is_fpObjc {
      set tsObj ""
   } else {
      set tsObj "-tsObj"
   }
   if $writeAI {
       set noAi ""
   } else {
       set noAi "-noAI"
   }
   if $write_test_info {
       set test_info "-test_info"
   } else {
       set test_info ""
   }
   if $write_children {
       set children "-children"
   } else {
       set children ""
   }
   if $family {
      set family "-family"
   } else {
      set family ""
   }
   if {$callback != ""} {
      set callback "-callback $callback"
   }

   # open my table to write; note that we read field startField only to get 
   # a template for table
   eval set_run -rerun \"$rerun\" $tsObj $run $col
   set table [openit $startField]

   if {$_my_table != ""} {
      upvar $_my_table my_table
   }
    
   if ![info exists my_table] {
       set my_table [eval open_mytables table $file_str $tsObj -dir $outdir \
	       $noAi $test_info]
   }
   objfileClose table
   
   # set pointers to the data
   eval set_run -rerun \"$rerun\" $tsObj $run $col

   # initialize all counting variables to zero
   set ntotall 0
   set nprimaryall 0
   set nselall 0
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]
   
   # start fields loop here
   loop field $startField [expr $endField + 1] {
      if [catch { set table [openit $field] } msg] {
	 echo $msg
	 continue
      }

       # total number of objects in this field
       set ntot 0  
       # number of primary objects in this field
       set nprimary 0
       # number of objects selected from this field
       set nsel 0  
       
       loop i 1 [expr [keylget table OBJnrow]+1] {
	   
	   read_objc table $i
	   incr ntot

	   if $is_fpObjc {
	      set rowc [exprGet $objcIo.objc_rowc]
	      if {$rowc > 64 && $rowc <= 1361 - 64} {
		 set primary 1
	      } else {
		 set primary 0
	      }
	   } else {
	      set primary [expr ([exprGet $objcIo.status] & 8192) != 0]
	   }

	   if {[info exist ignore_primary] && $ignore_primary} {
	      ;
	   } else {
	      # check if the object is primary
	      if {$primary != 1} {
		 if $verbose {
		    echo "   dropping a non-primary source"
		 }
		 continue;
	      }
	   }
	   incr nprimary
	   
           # this is a primary object, does it pass user selection? 
	   if {"$select" != "" && ![eval $select $objcIo]} {
	       continue;
	   }

	   # if we're writing families, have we already written it?
	   if {$family != ""} {
	      set pid [exprGet $objcIo.parent]
	      if {$pid > 0} {
		 set pid "$run,$col,$field,$pid"
		 if [info exists written($pid)] {
		    continue
		 }
		 set written($pid) 1
	      }
	   }

	   # passed selection, write it out
	   eval write_mytables $children $family $callback \
	       my_table $objcIo table
	   incr nsel
       }
       
       objfileClose table
       
       if {[info exist ignore_primary] && $ignore_primary} {
           echo "Run $run Col $col Field $field Total $ntot Selected $nsel"
       } else {
           echo "Run $run Col $col Field $field Total $ntot Primary $nprimary Selected $nsel "
       }
       # Add counts to total        
       set ntotall [expr $ntotall + $ntot]
       set nprimaryall [expr $nprimaryall + $nprimary]
       set nselall [expr $nselall + $nsel]
   }

   # Finally, close my master output my_table

   if {$_my_table == ""} {
      close_mytables my_table
   }
    
   if $verbose {
      if {[info exist ignore_primary] && $ignore_primary} {
	 echo "Run $run Col $col startField $startField endField $endField \
	 Total $ntotall Selected $nselall"
      } else {
	 echo "Run $run Col $col startField $startField endField $endField \
	    Total $ntotall Primary $nprimaryall Selected $nselall"
      }
      if $is_fpObjc {
	 set base "fpObjc"
      } else {
	 set base "tsObj"
      }
      echo "Your data is in $base-$file_str.fit"
   }

   return [list $ntotall $nprimaryall $nselall]
}



# A simple proc to select objects with magnitude between m1 and m2 in
# specified band. Allowed magnitude types are {psf, fiber, petro, exp,
# deV, model}. Band is specified by index (u=0, g=1, ... z=4).
# mag1, mag2, band and magnitude type are passed as elements of array
# sel_maglim and have to be set before calling this proc (actually, band
# defaults to 2):
#    array set sel_maglim [list mag1 17 mag2 19 type deV]
#
# Can be used with e.g. myObj2tsObj
proc sel_maglim {obj {b -1}} {
   global band sel_maglim
   global objcIo table

   if ![info exists sel_maglim(type)] {
      set sel_maglim(type) "psf"
      echo "[lindex [info level 0] 0]: setting type to $sel_maglim(type)"
   }

   if {![info exists sel_maglim(mag1)] || ![info exists sel_maglim(mag2)]} {
      error [concat "[lindex [info level 0] 0]: Please set magnitude limits sel_maglim(mag1/2)" \
		 "\n" {e.g. array set sel_maglim [list mag1 17 mag2 19 type deV]}]
   }

   if {$b < 0} {
      if ![info exists sel_maglim(band)] {
	 set sel_maglim(band) 2
      }
      set b $sel_maglim(band)
   }

   set type $sel_maglim(type);		# unpack for convenience
   if {$type == "psf" || $type == "fiber" || $type == "petro"} {
      set field ${type}Counts
   } elseif {$type == "exp" || $type == "deV" || $type == "model"} {
      set field counts_${type}
   } else {
      set field $type
   }
   
   if {[keylget table fileSchema] == "TSOBJ"} {
      set counts [exprGet $obj.$field<$b>]
      set reddening [exprGet $obj.reddening<$b>]
      set mag [expr $counts - $reddening]
   } else {
      set mag [get_mag_from_counts $objcIo $b $type]
   }
   
   if {$mag >= $sel_maglim(mag1) && $mag <= $sel_maglim(mag2)} {
      return 1
   } else {
      return 0
   }
}

# A simple proc to select objects with color m(f1)-m(f2) between c1 and c2.
# and magnitude in fmag band in the range m1 and m2. m1, m2, f1 and f2 can 
# be any of {u g r i z}. Allowed magnitude types are {psf, fiber, petro, 
# exp, deV, model}. m1, m2, f1, f2, c1, c2, band and magnitude type are 
# passed as globals and have to be set before calling this proc.
# Can be used with e.g. myObj2tsObj
proc sel_colormag {obj {b -1}} {
   global m1 m2 f1 f2 fmag c1 c2 type band ferr cerr 

   if {$type == "psf" || $type == "fiber" || $type == "petro"} {
      set field ${type}Counts
   } elseif {$type == "exp" || $type == "deV" || $type == "model"} {
      set field counts_${type}
   } else {
      set field $type
   }
   
   set filterlist {u g r i z}  
   set band [lsearch $filterlist $fmag]
   # check magnitude
   if {![sel_maglim $obj]} {
      return 0
   }
   # check color
   set index1 [lsearch $filterlist $f1]
   set index2 [lsearch $filterlist $f2]
   set fieldErr ${field}Err
   set counts1 [exprGet $obj.$field<$index1>]
   set counts2 [exprGet $obj.$field<$index2>]
   if {[catch {
      set countsErr1 [exprGet $obj.$fieldErr<$index1>]
      set countsErr2 [exprGet $obj.$fieldErr<$index2>]
      set magErr [exprGet $obj.$fieldErr<$band>]
   }]} {
      set countsErr1 0.0
      set countsErr2 0.0
      set magErr 0.0
      
   }
   set reddening1 [exprGet $obj.reddening<$index1>]
   set reddening2 [exprGet $obj.reddening<$index2>]
   set color [expr $counts1 - $reddening1 - $counts2 + $reddening2]
   set colorErr [expr sqrt($countsErr1*$countsErr1+$countsErr2*$countsErr2)] 
   
   if {$color >= $c1 && $color <= $c2 && $colorErr < $cerr && $magErr < $ferr} {
      return 1
   } else {
      return 0
   }
   
}

# an example for using myObj2tsObj proc: it selects objects brighter than
# 16th mag in r (psf mags) by using aux proc sel_maglim.

proc get_bright_stars {data_dir run startField endField {output_dir .}} {

   global band m1 m2 type data_root

     set data_root $data_dir  

     set band 2
     set m1 -10
     set m2  16
     set type psf

     # loop over camera columns
     loop col 1 7 {
        echo "working on column $col"
	set out_name bright_stars_run${run}_col${col}_${startField}_$endField 
        echo [myObj2tsObj $run $col $startField $endField sel_maglim $out_name \
                    -dir $output_dir] 
        echo ""
     }  

}


# an example for using myObj2tsObj proc: it selects all objects from 
# a specifed run with 14.5 < z < 20 and i-z>1.5 by default, or anything
# else by specifying parameters through list $params 
proc get_my_sources {data_dir {runs ""} {camCols "all"} {field1 -1} {field2 -1} \
			  {output_dir .} {log_file red_sources.log} {params {}}} {
   global m1 m2 f1 f2 fmag c1 c2 type band data_root ignore_primary openit ferr cerr 
     set data_root $data_dir  

     if {$runs == ""} {
         cd $data_root
         set runs [glob \[1-9\]*]
     } 

     if {$params == {}} {
        ### default parameters (extremely i-z red sources)
        # brighter than z=21, fainter than 14 to avoid saturated stars
        set fmag z
        set m1 14.0; set m2 21.0
        # and i-z > 1.5
        set f1 i; set f2 z
        set c1 1.5; set c2 20
        # max magnitude error 
        set ferr 1.0
        # max color error 
        set cerr 2.0
        # psf magnitude
        set type psf
        # output file name
        set name red_sources
     } else {
        ### user selected parameters
        # e.g.
        # set params [list  z 14 21 i z 1.5 20 1.0 2.0 psf red_sources]
	set fmag [lindex $params 0]
        set m1 [lindex $params 1]; set m2 [lindex $params 2]
        set f1 [lindex $params 3]; set f2 [lindex $params 4]
        set c1 [lindex $params 5]; set c2 [lindex $params 6]
        set ferr [lindex $params 7]; set cerr [lindex $params 8]
        set type [lindex $params 9]
        set name [lindex $params 10]
     }
     # we want all objects, not only primary
     set ignore_primary 1

     # log file
     set outf [open $output_dir/$log_file w]
     puts $outf "# Searched for sources with $m1 < m($fmag) < $m2 (type=$type)"
     puts $outf "# and $c1 < $f1-$f2 < $c2"  

     # loop over runs 
     foreach run $runs {
        echo ""
        echo "*** Run $run ***"
        echo " column     Ntotal   Nselected"
        puts $outf "#"
        puts $outf "#    *** Run $run ***"     
        puts $outf "#  column      Ntotal  Nselected"     
        set Ntotal 0; set Nselected 0 
        # loop over camera columns
        if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}
        foreach col $camCols {
           if {$field1 < 0 || $field2 < 0} {
              # get tsObj dir
              set_run $run $col -tsObj
              set tsObjdir $openit(objdir)
              # find the field range
              set fields [find_field_range $tsObjdir]
              set startField [lindex $fields 0]
              set endField [lindex $fields 1] 
           } else {
              set startField $field1 
              set endField $field2
           }          
  	   set out_name name_run${run}_col${col}_${startField}_$endField 
           set list [myObj2tsObj $run $col $startField $endField sel_colormag $out_name \
                    -dir $output_dir]
	   set Ntot [lindex $list 0]
	   set Nsel [lindex $list 2]
	   echo "     $col          $Ntot        $Nsel"
           puts $outf "     $col          $Ntot        $Nsel"     
	   incr Ntotal $Ntot; incr Nselected $Nsel
        }  
        echo "    total       $Ntotal          $Nselected"
        puts $outf "    total       $Ntotal        $Nselected"     
   }

   close $outf
}



###############################################################################
#
# Draw photo's summary plots; the working routines are fpstats and pspstats
#
proc plot_stats {args} {
   global openit

   set opts [list \
		 [list [info level 0] ""] \
		 [list {[start]} INTEGER -1 start "First field to plot"] \
		 [list {[end]} INTEGER -1 end "Last field to plot"] \
		 [list -type STRING "FP" which "FP (frames) or PSP (psp)"] \
		 [list -plan STRING "" planfile \
		      "psPlan file to use (default {fp,ps}Plan-camCol.par or {fp,ps}Plan.par)"] \
		 [list -filterlist STRING "all" filterlist \
		      "List of filters to plot (or \"summary\")"] \
		 [list -dev STRING "/XWINDOW" dev "Device to plot to"] \
		 [list -plots INTEGER 0x0 desired_plots \
		      "Plots to make; e.g. 0x5 for 1st and 3rd (0 => all)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$which == "FP"} {
      if {$planfile == ""} {
	 set planfile fpPlan
      }
   } elseif {$which == "PSP"} {
      if {$planfile == ""} {
	 set planfile psPlan
      }
   } else {
      error "Unknown plot type: $which"
   }

   if ![info exists openit] {
      error "Please use set_run and try again"
   }
   set camCol $openit(col)
   set outputDir $openit(dir)
   set run $openit(run)
   if {$run != "0"} {
      regsub {^[0]*} $run "" run
   }
   
   regsub {\.par$} $planfile {} planfile
   
   if {![file exists $openit(photo)/$planfile.par] &&
       [file exists $openit(photo)/$planfile-$camCol.par]} {
      append planfile "-$camCol"
   }
      
   param2Chain $openit(photo)/$planfile.par fpplan

   if {($start <= 0 || $end <= 0) && ![info exists fpplan]} {
      error "You must either read a plan file or specify starting/ending fields"
   }
   if {$start <= 0} {
      set start [keylget fpplan startField]
   }
   if {$end <= 0} {
      set end [keylget fpplan endField]
   }

   if {$which == "FP"} {
      plot_fpstats $dev $start $end $outputDir $run $camCol $filterlist
   } else {
      plot_pspstats $dev $start $end $outputDir $run $camCol $filterlist \
	  -plots $desired_plots
   }
}

###############################################################################
#
# Calculate effective areas of the PSF
#
proc neff {args} {
   set opts [list \
		 [list [info level 0] "Calculate n_eff given a FIELDPARAMS (pixels)"] \
		 [list <fieldparams> STRING "" fieldparams "The fieldparams in question"] \
		 [list <band> INTEGER "" band "Desired band"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set sigma1 [exprGet $fieldparams.frame<$band>.psf->sigma1_2G]
   set sigma2 [exprGet $fieldparams.frame<$band>.psf->sigma2_2G]
   set b [exprGet $fieldparams.frame<$band>.psf->b_2G]
   set pi [expr 4*atan2(1,1)]

   set neff [expr 4*$pi*pow(pow($sigma1,2) + $b*pow($sigma2,2), 2)/ \
		 (pow($sigma1,2) + 4*$b*pow($sigma1,2)*pow($sigma2,2)/(pow($sigma1,2) + pow($sigma2,2)) + \
		      pow($b,2)*pow($sigma2,2))]

   return $neff
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

proc fpstats {args} {
   set opts [list \
		 [list [info level 0] "Make the frames plots"] \
		 [list {[start]} INTEGER -1 start "First field"] \
		 [list {[end]} INTEGER -1 end "Last field"] \
		 [list {[filters]} STRING "all" filterlist "Filters to plot, or \"summary\" (or \"all\")"] \
		 [list -device STRING "/XWINDOW" dev "Device to print to"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   plot_stats -type FP $start $end -filterlist $filterlist -dev $dev
}

proc pspstats {args} {
   set opts [list \
		 [list [info level 0] "Make the PSP plots"] \
		 [list {[start]} INTEGER -1 start "First field"] \
		 [list {[end]} INTEGER -1 end "Last field"] \
		 [list {[filters]} STRING "all" filterlist "Filters to plot"] \
		 [list -device STRING "/XWINDOW" dev "Device to print to"] \
		 [list -plots INTEGER 0x0 desired_plots \
		      "Plots to make; e.g. 0x5 for 1st and 3rd (0 => all)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   plot_stats -type PSP $start $end $filterlist $dev -plots $desired_plots
}

###############################################################################
#
# Prepare to run photo yourself, starting with the photo's outputs
#
proc make_photo_inputs {args} {
   global data_root frame_reg openit

   set opts [list \
		 [list make_photo_inputs \
      "Prepare files needed to (re)run photo, starting with photo's outputs"] \
		 [list <dir> STRING "" basename \
    "Root name of directory tree to write files to (\$run will be appended)"] \
		 [list <field> INTEGER "" field "The desired field"] \
		 [list {[filters]} STRING "u g r i z" filterlist \
		      "filters to process"] \
		 [list -planFile STRING "" planfile "Write a planfile here"] \
		 [list -old_psField CONSTANT "" old_psField \
		      "The psField files are in an old format"] \
		 [list -display INTEGER 0 display "value of display"] \
		]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   foreach v "old_psField" {
      set $v [info exists $v]
   }

   if ![info exists openit] {
      error "Please use set_run to specify the run/column of interest"
   }
   
   set run $openit(run); regsub {^0*} $run "" run
   set col $openit(col)

   if ![regexp "$run\$" $basename] {
      append basename "/$run"
   }
   
   foreach dir "photo fields/$col objcs/$col" {
      if ![file isdir $basename/$dir] {
	 mkdir -path $basename/$dir
      }
   }
   set params $basename/photo/fpParam.par
   if ![file exists $params] {
      close [open $params w]
   }

   set psField psField-$openit(run)-$col-[format %04d $field].fit
   if ![file exists $basename/objcs/$col/$psField] {
      exec cp $openit(psdir)/$psField $basename/objcs/$col
   }
   
   foreach b $filterlist {
      set fpM [get_mask_filename "" $openit(run) $b $openit(col) $field]
      if ![file exists $basename/fields/$col/$fpM] {
	 exec cp $openit(psdir)/$fpM $basename/fields/$col
      }
   }

   foreach b $filterlist {
      set fpC [format %s/fpC-%06d-${b}$col-%04d.fit \
		   "$basename/fields/$col" $run $field]
      if ![file exists $fpC] {
	 echo $b
	 next_field -field $field -filter $b -bkgd binned
	 regWriteAsFits $frame_reg $fpC
      }
   }

   if ![info exist planfile] {
      return;
   }

   if ![regexp {^/} $planfile] {
      set planfile $basename/photo/$planfile
   }

   set fd [open $planfile w]
   puts $fd "filterlist		$filterlist"
   puts $fd ""
   puts $fd "run			$run"
   puts $fd "camCol			$col"
   puts $fd ""
   puts $fd "baseDir			$basename"
   puts $fd ""
   puts $fd "transFileDir		$data_root/\$run/astrom"
   puts $fd "transFile		asTrans-[format %06d $run].fit"
   puts $fd ""
   puts $fd "biasDir			\$baseDir/photo/calib"
   puts $fd "imageDir		\$baseDir/fields/\$camCol"
   puts $fd "fpCDir			\$baseDir/objcs/\$camCol"
   puts $fd "psDir			\$baseDir/objcs/\$camCol"
   puts $fd ""

   set configDir $data_root/$run/logs
   puts $fd "configDir		$data_root/\$run/logs"
   foreach t "Config ECalib BC Camera" {
      if [catch {
	 set file [lindex [glob $configDir/op$t-*.par] 0]
      } msg] {
	 echo "Failed to find op$t-*.par; not including in plan"
	 continue
      }
      puts $fd "ccd$t		[file tail $file]"
   }
   
   puts $fd ""
   puts $fd "parametersDir		\$baseDir/photo"
   puts $fd ""
   puts $fd "outputDir		\$baseDir/objcs/\$camCol"
   puts $fd ""
   puts $fd "#"
   puts $fd "# Local changes"
   puts $fd "#"
   puts $fd "write_test_info	0"
   puts $fd "trace_memory		mem.trace.$col"
   puts $fd "read_fpC_files 1	# read fpC, not idR, files"
   puts $fd "write_fpC_files 0	# don't write fpC files"
   puts $fd ""
   puts $fd "startField		$field"
   puts $fd "endField		$field"
   puts $fd ""
   puts $fd "display		$display"

   if $old_psField {
      puts $fd "allow_old_psField     1"
   }
      
   close $fd
}

###############################################################################
#
# Run fitCellChisq for an OBJC_IO
#
proc modelFitChisq {args} {
   global env fieldparams table

   set deV 0; set exp 0;
   set sky_noise_only 0;

   set opts [list \
		 [list [info level 0] ""] \
		 [list <objcIo> STRING "" objcIo "The object to fit"] \
		 [list <filter> STRING "" filter "Desired filter (u, g, ...)"]\
		 [list <psf> STRING "" _psfBasis \
		      "array of PSF_BASIS indexed by filter"] \
		 [list -deV CONSTANT 1 deV "Make deV (not exp) models"] \
		 [list -exp CONSTANT 1 exp \
		      "Make exp (not deV) models (default)"] \
		 [list -totflux DOUBLE 0 totflux "Total flux"] \
		 [list -aratio0 DOUBLE 0 aratio0 "Starting a/b"] \
		 [list -aratio1 DOUBLE 1.0 aratio1 "Ending a/b"] \
		 [list -daratio DOUBLE 0.01 daratio "Stepsize in a/b"] \
		 [list -rsize0 DOUBLE 0 rsize0 "Starting r_e"] \
		 [list -rsize1 DOUBLE 5 rsize1 "Ending r_e"] \
		 [list -drsize DOUBLE 0.01 drsize "Stepsize in r_e"] \
		 [list -sky_noise_only CONSTANT 1 sky_noise_only \
		      "Only consider sky noise in chi^2"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   upvar $_psfBasis psfBasis

   set naratio [expr int(($aratio1 - $aratio0)/$daratio) + 1]
   set nrsize [expr int(($rsize1 - $rsize0)/$drsize) + 1]

   if {$naratio <= 0} {
      error "Number of steps in a/b = $naratio"
   }
   if {$nrsize <= 0} {
      error "Number of steps in r_e = $nrsize"
   }

   if {$deV && $exp} {
      error "You mayn't specify both -deV and -exp"
   }

   initProfileExtract
   initFitobj $env(PHOTO_DIR)/lib/rgalprof.dat
   initCellFitobj $env(PHOTO_DIR)/lib/cellprof.dat 5

   set flags ""
   if $deV {
      lappend flags "-deV"
   }
   if $sky_noise_only {
      lappend flags "-sky_noise_only"
   }

   set band [lsearch [keylget table filters] $filter]
   if {$band < 0} {
      error "Filter $filter is invalid"
   }

   if {$totflux == 0} {
      set totflux [exprGet $objcIo.petroCounts<$band>]
   }

   if {[exprGet (int)$fieldparams.frame<$band>.data] == 0} {
      global frame_reg

      set n [exprGet $objcIo.id];	# we'll have to re-read it

      if [info exists frame_reg] {
	 regDel $frame_reg
      }
      set frame_reg [recon $filter table "" "" 0 "binned"]

      handleSetFromHandle $fieldparams.frame<$band>.data &$frame_reg

      read_objc table $n
   }
   #
   # Setup the PSF
   #
   set reg [reconstruct_psf $psfBasis($filter) \
		[exprGet $objcIo.rowc<$band>] [exprGet $objcIo.colc<$band>]]
   set psf [fitPsfFromReg $reg 0 -sigma 1]
   handleSetFromHandle $fieldparams.frame<$band>.psf &$psf
   
   fitCellColorSet $band $psf.coeffs

   regDel $reg; handleDel $psf

   eval fitCellChisq $flags $objcIo $band $fieldparams \
       $aratio0 $aratio1 $naratio $rsize0 $rsize1 $nrsize
}

###############################################################################
#
# Initialise model fitting code
#
proc initModels {args} {
   global env

   set opts [list \
		 [list [info level 0] "Initialise model fitting code"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   initProfileExtract
   initFitobj $env(PHOTO_DIR)/lib/rgalprof.dat
   initCellFitobj $env(PHOTO_DIR)/lib/cellprof.dat 5
   if {[llength [handleListFromType RANDOM]] == 0} {
      global rand
      set rand [phRandomNew 10000000:3]
   }
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

proc testCellprof {args} {
   global env

   set opts [list \
		 [list [info level 0] ""] \
		 [list <field> INTEGER -1 field "Field"] \
		 [list <n> INTEGER 1 n "Object ID"] \
		 [list {[filter]} STRING "r" filter "Desired filter"] \
		 [list -cellprof STRING "$env(PHOTO_DIR)/lib/cellprof.dat"\
		      cellprof "Cellprof.dat file to use"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   global table openit objcIo filterlist

   if ![info exists openit] {
      error "Please use set_run to specify a run and column"
   }
   set table [openit $field]
   get_psfBasis psfBasis $field

   if {[lindex [profileRadii] end] == 0} {		# init
      initProfileExtract
      
      initFitobj "$env(PHOTO_DIR)/lib/rgalprof.dat"
      initCellFitobj $cellprof [llength [keylget table filters]]
   }

   set objcIo [read_objc table $n]
   set objc [objcNewFromObjcIo $objcIo]

   set band [lsearch [keylget table filters] $filter]
   #handleSet $objc.color<$band>->rowc 0.5
   #handleSet $objc.color<$band>->colc 0.5
   handleSet $objc.color<$band>->phi_deV 90
   handleSet $objc.color<$band>->counts_deV 300000
   set reg [fittedModelMake -deV -cellprof -nocorrect $psfBasis($filter) $objc $band]
   objcDel $objc

   return $reg
}

#
# This must be after any procs for which you need help
#
set_ftclHelp phOutputs help_procs;	# ftclHelpDefine's on all listed procs
