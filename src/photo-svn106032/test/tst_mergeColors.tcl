#
# Test the Measure Objects module
#
source [envscan \$PHOTO_DIR]/etc/photoStartup.tcl
source [envscan \$PHOTO_DIR]/test/tst_procs.tcl

set soft_param_file [envscan \$PHOTO_DIR]/test/tst_params
inipar $soft_param_file
#
# create regions, using the given sky, and add data values for objects
#    "object" on top; then merge them and see if we get the answer right
#
# n.b. The region for test "foo" is created by proc make_region_foo
#
#
# A single colour, with two objects that overlap
#
proc make_region_1 {sky _reg _vals _grow} {
   upvar $_reg regarray  $_vals vals  $_grow grow

   set filters [list r]

   set regarray(r) [regNew 40 40]

   set reg $regarray(r)
   global soft_bias; regIntSetVal $reg [expr $sky+$soft_bias]
   
   add_to_reg $reg 20 22 "100 100"
   
   add_to_reg $reg 20 20 " 10  10  10"
   add_to_reg $reg 19 20 "100  10  10"
   add_to_reg $reg 18 20 " 10  10  10"
   
   add_to_reg $reg 30 20 " 10  10  10"
   add_to_reg $reg 29 20 "100 100 100"
   add_to_reg $reg 28 20 " 10  10  10"
   #                 cmin cmax rmin rmax
   set vals(0) [list 20   23   18   20]
   set vals(1) [list 20   22   28   30]
   
   return $filters
}

#
# A single colour, with 2 objects that don't overlap and a bridge
# that joins them after they've been grown
#
proc make_region_2 { sky _reg _vals _grow} {
   upvar $_reg regarray  $_vals vals  $_grow grow

   set filters [list r]
   foreach f $filters {
      set regarray($f) [regNew 40 40]

      set reg $regarray($f)
      global soft_bias; regIntSetVal $reg [expr $sky+$soft_bias]

      add_to_reg $reg 20 20 " 10  10  10"
      add_to_reg $reg 19 20 " 10  10  10"
      add_to_reg $reg 18 20 " 10  10  10"
      
      add_to_reg $reg 20 29 " 10  10  10"
      add_to_reg $reg 19 29 " 10  10  10"
      add_to_reg $reg 18 29 " 10  10  10"

      add_to_reg $reg 21 24 "  0  10  10   0";# the bridge
      add_to_reg $reg 20 24 " 10  10  10  10"
      #                 cmin cmax rmin rmax
      set vals(0) [list 19   32   17   22]
   }
   
   set grow 1
   return $filters
}

#
# Two colours, with an object in g that overlaps one in r
#
proc make_region_3 { sky _reg _vals _grow} {
   upvar $_reg regarray  $_vals vals  $_grow grow

   set filters [list g r]
   foreach f $filters {
      set regarray($f) [regNew 40 40]

      set reg $regarray($f)
      global soft_bias; regIntSetVal $reg [expr $sky+$soft_bias]

      if {$f == "r"} {
	 add_to_reg $reg 20 22 "100 100"
      } elseif {$f == "g"} {
	 add_to_reg $reg 20 20 " 10  10  10"
	 add_to_reg $reg 19 20 "100  10  10"
	 add_to_reg $reg 18 20 " 10  10  10"
      
	 add_to_reg $reg 30 20 " 10  10  10"
	 add_to_reg $reg 29 20 "100 101 100"
	 add_to_reg $reg 28 20 " 10  10  10"
      }
      #                 cmin cmax rmin rmax
      set vals(0) [list 20   23   18   20]
      set vals(1) [list 20   22   28   30]
   }
   
   return $filters
}

#
# 2 objects in g that don't overlap, with a bridge in r that joins them
#
proc make_region_4 { sky _reg _vals _grow} {
   upvar $_reg regarray  $_vals vals  $_grow grow

   set filters [list g r]
   foreach f $filters {
      set regarray($f) [regNew 40 40]

      set reg $regarray($f)
      global soft_bias; regIntSetVal $reg [expr $sky+$soft_bias]

      if {$f == "r"} {
	 add_to_reg $reg 21 22 "  0 100 100   0"
	 add_to_reg $reg 20 22 "100 100 100 100"
      } elseif {$f == "g"} {
	 add_to_reg $reg 20 20 " 10  10  10"
	 add_to_reg $reg 19 20 "100  10  10"
	 add_to_reg $reg 18 20 " 10  10  10"

	 add_to_reg $reg 20 25 " 10  10  10"
	 add_to_reg $reg 19 25 " 10  10 100"
	 add_to_reg $reg 18 25 " 10  10  10"
      }
      #                 cmin cmax rmin rmax
      set vals(0) [list 20   27   18   21]
   }
   
   return $filters
}

#
# An object in i (whose coordinate system is offset from the reference)
#
proc make_region_5 { sky _reg _vals _grow} {
   upvar $_reg regarray  $_vals vals  $_grow grow

   set filters [list i]
   foreach f $filters {
      set regarray($f) [regNew 40 40]

      set reg $regarray($f)
      global soft_bias; regIntSetVal $reg [expr $sky+$soft_bias]

      add_to_reg $reg 25 30 " 10  10  10"
      add_to_reg $reg 24 30 "100  10  10"
      add_to_reg $reg 23 30 " 10  10  10"
      #                 cmin cmax rmin rmax
      set vals(0) [list 30   32   23   25]
   }
   
   return $filters
}

#
# An object in r and i (whose coordinate systems are offset)
#
proc make_region_6 { sky _reg _vals _grow} {
   upvar $_reg regarray  $_vals vals  $_grow grow

   set filters [list r i]
   foreach f $filters {
      set regarray($f) [regNew 40 40]

      set reg $regarray($f)
      global soft_bias; regIntSetVal $reg [expr $sky+$soft_bias]

      if {$f == "r"} {
	 add_to_reg $reg 20 20 " 10  10  10"
	 add_to_reg $reg 19 20 " 10  10 100"
	 add_to_reg $reg 18 20 " 10  10  10"
      } elseif {$f == "i"} {
	 add_to_reg $reg 25 30 " 10  10  10"
	 add_to_reg $reg 24 30 "100  10  10"
	 add_to_reg $reg 23 30 " 10  10  10"
      }
      #                 cmin cmax rmin rmax
      set vals(0) [list 20   22   18   20]
   }
   
   return $filters
}

################################################################
#
# given a region, and a sky value, run the object finder to find all
# pixels above sky, and use this to generate an OBJC, which is then returned
#
proc find_objects {reg fparams sky level grow} {
   global soft_bias OBJECT1
   
   set lev [vectorExprEval ($sky+$level+$soft_bias)]
   set bsm [binregionNewFromConst $sky];
   set objects [regFinder $reg $lev $fparams]
   vectorExprDel $lev

   if $grow {
      set grown [chainNew OBJECT]
      loop i 0 [chainSize $objects] {
	 set obj [chainElementGetByPos $objects $i]
	 set new [objectGrow $obj 0 $reg $grow]
	 chainElementAddByPos $grown $new
	 handleDel $obj; handleDel $new
      }
      objectChainDel $objects
      set objects $grown
   }
#
# Convert OBJECT chain to OBJECT1s
#
   set obj1_chain [objectToObject1ChainConvert $objects 0 $reg $bsm]
   object1ChainFlagsSet $obj1_chain $OBJECT1(BINNED1)

   objectChainDel $objects
   binregionDel $bsm

   return $obj1_chain
}

#
# Set parameters and run code. Note that the i band is offset from the
# g and r
#
set filterlist [list g r i]
set nfilter [llength $filterlist]
set sky 100
if {![info exists verbose]} {
   set verbose 0
}

foreach filter $filterlist {
  set toGCC($filter) [transNew]
}
if {[lsearch $filterlist "i"] >= 0} {
   handleSet $toGCC(i).a -5
   handleSet $toGCC(i).d -10
}

#####
# create CALIB1BYFRAME structure
set calibs [calib1byframeNew $filterlist]
loop i 0 $nfilter {
   set cal1 [handleBindFromHandle [handleNew] *$calibs.calib<$i>]
   handleSet $cal1.sky $sky
   handleSet $cal1.node 0.0
   handleSet $cal1.incl 0.0
   
   handleSetFromHandle $calibs.calib<$i> &$cal1
   handleDel $cal1
}

#
# Do the real work
#
set retval 0;				# success until proven a failure

initMergeColors $nfilter

set fieldparams [fieldparamsNew $filterlist]
handleSet $fieldparams.ref_band_index 0
set soft_bias [softBiasGet]
loop i 0 $nfilter {
   handleSet $fieldparams.frame<$i>.bkgd 0
   handleSetFromHandle $fieldparams.frame<$i>.gain \
       &[binregionNewFromConst [getpari gain] -shift $PHOTO_CONSTS(MAX_U16)]
handleSetFromHandle $fieldparams.frame<$i>.dark_variance \
    &[binregionNewFromConst [getpari dark_variance] -shift $PHOTO_CONSTS(MAX_U16)]
   handleSet $fieldparams.frame<$i>.colBinning 1
}

if ![info exists tests] {
   set tests [list 1 2 3 4 5 6]
}
foreach test $tests {
   echo "Frame $test:"
#
# create data
#
   catch {unset vals reg}
   set grow 0;				# how much to grow objects by
   eval "set filters \[make_region_$test $sky reg vals grow\]"
   set nfilters [llength $filters]
   handleSet $fieldparams.ncolor $nfilters

   foreach f $filterlist {
      set i [lsearch $filters $f]
      if {$i < 0} {
	 continue;
      }

      add_mask $reg($f)

      if 0 {
	 mtv $reg($f)
      }

      set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<$i>]
      handleSetFromHandle $fparams.data &$reg($f)
      handleSetFromHandle $fparams.toGCC &$toGCC($f)
      
      set objects($f) [chainNew "OBJECT1"]
      foreach lev {10 100} {
	 set chain [find_objects $reg($f) $fparams $sky $lev $grow]
	 if $verbose {
	    echo "There are [chainSize $chain] $f objects at $lev"
	 }
	 chainJoin $objects($f) $chain
      }
      if $verbose {
	 echo "There are [chainSize $objects($f)] $f objects/levels in total"
      }

      handleDel $fparams
   }

   set merged [mergeColors $filters objects $fieldparams]

   set nmerged [chainSize $merged]
   if {[array size vals] != $nmerged} {
      echo "TEST-ERR: Expected [array size vals] not $nmerged" \
	  "elements on merged list for test $test"
      set nmerged [array size vals]
      incr retval
   }
   loop i 0 $nmerged {
      set objc [chainElementGetByPos $merged $i]

      set mm [handleBindFromHandle [handleNew] *$objc.aimage->master_mask]
      if {[exprGet $mm.cmin] != [lindex $vals($i) 0] || \
	      [exprGet $mm.cmax] != [lindex $vals($i) 1] || \
	      [exprGet $mm.rmin] != [lindex $vals($i) 2] || \
	      [exprGet $mm.rmax] != [lindex $vals($i) 3]} {
	 echo "TEST-ERR: Incorrect bounding box for object $i of test $test"
	 echo "Found cmin cmax rmin rmax =" \
	     "[exprGet $mm.cmin]" \
	     "[exprGet $mm.cmax]" \
	     "[exprGet $mm.rmin]" \
	     "[exprGet $mm.rmax]" \
	     " Should be " \
	     "[lindex $vals($i) 0]" \
	     "[lindex $vals($i) 1]" \
	     "[lindex $vals($i) 2]" \
	     "[lindex $vals($i) 3]"
	 incr retval
      }

      handleDel $mm; handleDel $objc
   }
#
# cleanup
#
   objcChainDel $merged
   foreach f $filters {
      chainDel $objects($f);
      spanmaskDel *$reg($f).mask
      handleSet (long)$reg($f).mask 0
      regDel $reg($f)
   }
}

#####
# clean up
finiMergeColors
set filterlist [array names toGCC]; set nfilter [llength $filterlist]
loop i 0 $nfilter {
   transDel $toGCC([lindex $filterlist $i])
   handleSet $fieldparams.frame<$i>.toGCC 0
}
handleSet $fieldparams.ncolor $nfilter
fieldparamsDel $fieldparams

unset reg

calib1byframeDel $calibs

if $retval {
   error "Failed $retval tests"
}
