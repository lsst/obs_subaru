#Some very simple chain tests.

proc chainTest {} {
   set chain [chainNew TRANSTEST]
   set reg [regNew 512 512]
   loop i 0 100 {
	set hndl [genericNew TRANSTEST]
	handleSet $hndl.ib<7> $i
	handleSetFromHandle $hndl.ra &$reg
	chainElementAddByPos $chain $hndl
	}
   echo Chain is $chain
   
   echo Test chainSearch:
   set test [chainSearch $chain "{ib<7> < 20}"]
   if {[chainSize $test] == 20} {echo "   " Passed} else {echo "   " Failed}
   chainDel $test

   echo Test chainSearch:
   set test [chainSearch $chain "{ib<7> & 1} {ib<7> & 8} {ib<7> !& 4}"]
   if {[chainSize $test] == 12} {echo "   " Passed} else {echo "   " Failed}
   chainDel $test

   echo Test chainSearch:
   set test [chainSearch $chain "{ib<7> !& 1}"]
   if {[chainSize $test] == 50} {echo "   " Passed} else {echo "   " Failed}
   chainDel $test

   echo Test chainFSort:
   chainFSort $chain ib<7> -reverse
   set hndl [chainElementGetByPos $chain 3]
   if {[exprGet $hndl.ib<7>] == 96} {echo "   " Passed} else {echo "   " Failed}
   handleDel $hndl

   echo Test chainSelect:
   chainFSort $chain ib<7>
   set test [chainSelect $chain "{97 <= ib<7> < 2}"]
   if {[chainSize $test] == 5} {echo "   " Passed} else {echo "   " Failed}
   chainDel $test

   echo Test chainSelect again:
   set test [chainSelect $chain "{105 <= ib<7> < -4}"]
   if {[chainSize $test] == 0} {echo "   " Passed} else {echo "   " Failed}
   chainDel $test

   echo Test chainSet:
   chainSet $chain ia 20
   set hndl [chainElementGetByPos $chain 3]
   if {[exprGet $hndl.ia] == 20} {echo "   " Passed} else {echo "   " Failed}
   handleDel $hndl
   
   echo Test chainScale:
   chainScale $chain ia -2 1.5
   set hndl [chainElementGetByPos $chain 3]
   if {[exprGet $hndl.ia] == 28} {echo "   " Passed} else {echo "   " Failed}
   handleDel $hndl
   
  echo Test chainSetFromHandle:
   chainSetFromHandle $chain ia $reg.nrow
   set hndl [chainElementGetByPos $chain 3]
   if {[exprGet $hndl.ia] == 512} {echo "   " Passed} else {echo "   " Failed}
   handleDel $hndl
   
   echo Test chainTransZXY:
   chainTransZXY $chain ib<8> ra->nrow ib<7> 3 -1 7
   set hndl [chainElementGetByPos $chain 3]
   set nrow [exprGet $hndl.ra->nrow]
   set ib [exprGet $hndl.ib<7>]
   set result [expr 3 + (-1)*$nrow + 7*$ib]
   if {[exprGet $hndl.ib<8>] == $result} {echo "   " Passed} else {echo "   " Failed}
   handleDel $hndl

   echo Test chainTransXY:
   set x 22; set y 44
   chainSet $chain fb<0> $x
   chainSet $chain fb<1> $y
   chainTransXY $chain fb<0> fb<1> 2 -4 3 .2 -.3 7
   set hndl [chainElementGetByPos $chain 3]
   set tx [format %.2lf [exprGet $hndl.fb<0>]]
   set ty [format %.2lf [exprGet $hndl.fb<1>]]
   set cx [expr 2 + (-4)*$x + 3*$y]
   set cy [expr .2 + (-.3)*$x + 7*$y]
   if {$tx == $cx && $ty == $cy} {echo "   " Passed} else {echo "   " Failed}
   handleDel $hndl

   echo Test chainExtract:
   set test [chainExtract $chain 55 10]
   set hndl [chainElementGetByPos $test 3]
   if {[chainSize $test] == 10 && [exprGet $hndl.ib<7>] == 58} \
	{echo "   " Passed} else {echo "   " Failed}
   handleDel $hndl
   chainDel $test
   regDel $reg
   genericChainDel $chain
   }

chainTest
