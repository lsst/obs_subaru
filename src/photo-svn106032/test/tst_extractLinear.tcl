#
# Test the extraction code; in particular test that it doesn't fail to
# test that pixels being examined lie within the region
#
# Unless we get a SEGV, this code will report no errors
#
proc do_line { reg r0 c0 r1 c1 {from_profile 0}} {
   set st [profileExtractLinear $reg $r0 $c0 $r1 $c1 10 0 0]
   set ncol [exprGet $reg.ncol]
   set nrow [exprGet $reg.nrow]
   if $from_profile {
      set om [objmaskSetFromProfileLinear $ncol $nrow $st 1000]
   } else {
      set sm [spanmaskNew]

      set om [objmaskNew]
      set chain [handleBindFromHandle [handleNew] *$sm.masks<0>]
      chainElementAddByPos $chain $om HEAD AFTER
      objmaskSetLinear $om $ncol $nrow $r0 $c0 $r1 $c1 1 1
      handleDel $om
      
      set om [objmaskNew]
      set chain [handleBindFromHandle [handleNew] *$sm.masks<1>]
      chainElementAddByPos $chain $om HEAD AFTER
      objmaskSetLinear $om $ncol $nrow $r0 $c0 $r1 $c1 10 20
      handleDel $om

      spanmaskDel $sm
   }
   handleDel $st
}
#
# See if we are running interactively
#
global interactive
if {[catch {set interactive}]} {
   set interactive 0
}

initProfileExtract
set tmp [regNew 1482 2048]
set mtmp [maskNew [exprGet $tmp.nrow] [exprGet $tmp.ncol] -name input]
handleSetFromHandle $tmp.mask &$mtmp
handleDel $mtmp

foreach from_profile { 1 0 } {
   maskClear *$tmp.mask
   do_line $tmp 800 -700 800 4300 $from_profile
   do_line $tmp -700 800 4300 800 $from_profile
   do_line $tmp 800 700 800 1300 $from_profile
   do_line $tmp 700 800 1300 800 $from_profile

   do_line $tmp 1160 1120 1120 100 $from_profile
   do_line $tmp 2160 1120 5120 -10 $from_profile
   do_line $tmp 3160 1120 5120 -10 $from_profile
   do_line $tmp 3160 1120 5120 -1010 $from_profile
   do_line $tmp 3160 4120 5120 -1010 $from_profile
   do_line $tmp 2160 1120 -5120 -10 $from_profile
   do_line $tmp 2160 1120 -5120 2010 $from_profile
   do_line $tmp 2160 -3020 -5120 2010 $from_profile
   do_line $tmp 560 2120 520 -10 $from_profile
   do_line $tmp 560 2120 820 -10 $from_profile
   do_line $tmp 560 2120 2820 -10 $from_profile
   do_line $tmp 560 2120 -400 -10 $from_profile
   do_line $tmp -10 400 1000 -10 $from_profile
   do_line $tmp 1200 -100 3000 300 $from_profile
   do_line $tmp 1120 1160 100 1120 $from_profile

   do_line $tmp 1120 2160 -10 5120 $from_profile
   do_line $tmp 1120 3160 -10 5120 $from_profile
   do_line $tmp 1120 3160 -1010 5120 $from_profile
   do_line $tmp 4120 3160 -1010 5120 $from_profile
   do_line $tmp 1120 2160 -10 -5120 $from_profile
   do_line $tmp 1120 2160 2010 -5120 $from_profile
   do_line $tmp -3020 2160 2010 -5120 $from_profile
   do_line $tmp 2120 560 -10 520 $from_profile
   do_line $tmp 2120 560 -10 820 $from_profile
   do_line $tmp 2120 560 -10 2820 $from_profile
   do_line $tmp 2120 560 -10 -400 $from_profile
   do_line $tmp 400 -10 -10 1000 $from_profile
   do_line $tmp -100 1200 300 3000 $from_profile
}
if $interactive {
#   mtv $tmp
}
regDel $tmp

if {!$interactive} {
   exit 0
}
