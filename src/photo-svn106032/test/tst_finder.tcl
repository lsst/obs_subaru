#
# Basic test of Finder
#
source [envscan \$PHOTO_DIR]/etc/photo_procs.tcl
source [envscan \$PHOTO_DIR]/test/tst_procs.tcl
#
# Procs to make regions for object-finder testing
#
# Each proc is responsible for setting $reg and $levels, and returning
# the number of objects that should be found.
#
# If there is only one level, the number of objects is that found by regFinder
#
# If there are two (or more), regFinder is run, and the first object found
# is taken to the one of interest. objectsFindInObjmask is then run on the
# pixels in this first object, at a threshold of levels.vec<1>
#
proc make_reg0 {_reg _levels} {
   upvar $_reg reg  $_levels levels

   set reg [add_mask [regNew 10 10]]
   regClear $reg

   set levels [vectorExprEval 10,10]

   return 0
}

proc make_reg1 {_reg _levels} {
   upvar $_reg reg  $_levels levels

   set reg [add_mask [regNew 10 10]]
   regClear $reg

   add_to_reg $reg 6 3   "100   0   0   0 100"
   add_to_reg $reg 5 3   "  0   0 100   0   0"
   add_to_reg $reg 4 3   "100   0   0   0 100"

   set levels [vectorExprEval 10,10]

   return 5
}

proc make_reg2 {_reg _levels} {
   upvar $_reg reg  $_levels levels

   set reg [add_mask [regNew 10 10]]
   regClear $reg

   add_to_reg $reg 6 4   "100   0 100"
   add_to_reg $reg 5 4   "  0 100   0"
   add_to_reg $reg 4 4   "100   0 100"

   set levels [vectorExprEval 10,10]

   return 1
}

proc make_reg3 {_reg _levels} {
   upvar $_reg reg  $_levels levels

   set reg [add_mask [regNew 10 10]]
   regClear $reg

   add_to_reg $reg 2 8   "100 100"
   add_to_reg $reg 1 8   "100 100"
   add_to_reg $reg 0 8   "100 100"

   set levels [vectorExprEval 10,10]

   return 1
}

proc make_reg4 {_reg _levels} {
   upvar $_reg reg  $_levels levels

   set reg [add_mask [regNew 10 10]]
   regClear $reg

   add_to_reg $reg 5 4   " 10  10 100"
   add_to_reg $reg 4 4   " 10  10  10"
   add_to_reg $reg 3 4   " 100 10  10"

   set levels [vectorExprEval "{5 20}"]

   return 2
}

proc make_reg5 {_reg _levels} {
   upvar $_reg reg  $_levels levels

   set reg [add_mask [regNew 20 20]]
   regClear $reg

   add_to_reg $reg 7 4   "  0   0   0 100"
   add_to_reg $reg 6 4   "  0   1   1"
   add_to_reg $reg 5 4   " 10  10 100"
   add_to_reg $reg 4 4   " 10 100  10"
   add_to_reg $reg 3 4   " 100 10  10"

   set levels [vectorExprEval "{5 20}"]

   return 1
}

#
# Here's the proc to actually carry out the tests of regFinder
#
proc test_finder {test} {
   global fparams
   
   set ret 0
    
   set nobjs0 [eval make_$test reg levels]
   set objs [regFinder $reg $levels $fparams]

   if {[exprGet $levels.dimen] > 1} {	# carry out objmask test too
      set obj [chainElementRemByPos $objs HEAD]
      chainDestroy $objs objectDel

      set tmp $levels
      set levels [vectorExprEval [exprGet $levels.vec<1>]]
      vectorExprDel $tmp
      set objs [objectsFindInObjmask $reg *$obj.sv<0> $levels]
      objectDel $obj
   }

   set nobjs [chainSize $objs]
   if { $nobjs != $nobjs0 } {
      echo nobjs = $nobjs
      echo TEST-ERR: test $test: wrong number of objects ($nobjs not $nobjs0)
      incr ret
   }
   
   chainDestroy $objs objectDel
   vectorExprDel $levels
   spanmaskDel *$reg.mask
   handleSet (long)$reg.mask 0
   regDel $reg

   return $ret
}

#
# Set fieldparams
#
set soft_param_file [envscan \$PHOTO_DIR]/test/tst_params
inipar $soft_param_file

set fieldparams [fieldparamsNew "r"]
set fparams [handleBindFromHandle [handleNew] $fieldparams.frame<0>]

set soft_bias [softBiasGet]
handleSet $fparams.bkgd 0
handleSet $fparams.gain [getpari gain]
handleSet $fparams.dark_variance [getpari dark_variance]

#
# Run all the tests
#
set failed 0

set tests [list 0 1 2 3 4 5]
#set tests [list 1]
foreach test $tests {
   incr failed [test_finder reg$test]
}

handleDel $fparams
fieldparamsDel $fieldparams

if $failed {
   error "Failed $failed tests"
}
