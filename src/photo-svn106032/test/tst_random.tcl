#
# Test the random number generator
#
proc tst_randomNew {nval} {
   global rand; set rand [phRandomNew $nval -seed [random 100000]]
   return 0
}

proc tst_random {nval} {
   set ret 0

   set sig [expr 2.0*sqrt(1.0/12)];		# expected standard deviation
   
   set sum 0; set sum2 0
   loop i 0 $nval {
      set r [expr [phRandom].0/(1<<31)]
      set sum [expr $sum+$r]
      set sum2 [expr $sum2+$r*$r]
   }
   set sum [expr $sum/$nval]
   set sum2 [expr sqrt($sum2/$nval-$sum*$sum)]
   
   if {abs($sum) > 3*$sig/sqrt($nval)} {
      echo "TEST-ERR: Mean is more than 3 sigma wrong"
      set ret 1
   }
   if {abs($sum2 - $sig) > 3*$sig/sqrt(2*$nval)} {
      echo "TEST-ERR: Standard deviation is more than 3 sigma wrong"
      echo "TEST-ERR: ($sum2 v. $sig)"
      set ret 1
   }
   
   return $ret
}
#
# Run those tests
#
set failed 0

echo Running tst_randomNew
if {[tst_randomNew 2000]} {
   echo "TEST-ERR: tst_random failed"
   set failed 1
}
echo Running tst_random
if {[tst_random 1000]} {
   echo "TEST-ERR: tst_random failed"
   set failed 1
}
phRandomDel $rand

global interactive
if {[catch {set interactive}]} {
   exit $failed
}
