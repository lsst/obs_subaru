#
# Non-comprehensive tests for region utility commands
#
proc testRegUtils {} {
   set nr 55
   set nc 40
   set reg1 [regNew $nr $nc]
   set reg2 [regNew $nr $nc]
   set reg3 [regNew $nr $nc]
   set reg4 [regNew $nr $nc]
   set err "testRegUtils error: "

   #
   # Test regSetWithDbl (and regPixGet)
   #
   regSetWithDbl $reg1 1
   set rval [regPixGet $reg1 0 0]
   if {$rval != 1} {echo TEST-ERR: $err regSetWithDbl - tried to set to 1, got $rval}
   set rval [regPixGet $reg1 20 20]
   if {$rval != 1} {echo TEST-ERR: $err regSetWithDbl - tried to set to 1, got $rval}

   regSetWithDbl $reg2 2
   set rval [regPixGet $reg2 0 0]
   if {$rval != 2} {echo TEST-ERR: $err regSetWithDbl - tried to set to 2, got $rval}
   set rval [regPixGet $reg2 20 20]
   if {$rval != 2} {echo TEST-ERR: $err regSetWithDbl - tried to set to 2, got $rval}

   #
   # Test regAdd
   #
   regSetWithDbl $reg3 0
   regAdd $reg1 $reg2 -regOut $reg3
   loop rr 0 [expr {$nr - 1}] {
      loop rc 0 [expr {$nc - 1}] {
         set sum [regPixGet $reg3 $rr $rc]
         if {$sum != 3} {echo TEST-ERR: $err regAdd - expected sum of 3, got $sum}
      }
   }

   #
   # Test regSubtract
   #
   regSetWithDbl $reg3 10
   regSubtract $reg1 $reg2 -regOut $reg3
   loop rr 0 [expr {$nr - 1}] {
      loop rc 0 [expr {$nc - 1}] {
         set sum [regPixGet $reg3 $rr $rc]
         if {$sum != 0} {echo TEST-ERR: $err regSubract - expected  0, got $sum}
      }
   }
   regSetWithDbl $reg3 10
   regSubtract $reg2 $reg1 -regOut $reg3
   loop rr 0 [expr {$nr - 1}] {
      loop rc 0 [expr {$nc - 1}] {
         set sum [regPixGet $reg3 $rr $rc]
         if {$sum != 1} {echo TEST-ERR: $err regSubract - expected  1, got $sum}
      }
   }

   #
   # Test regPixCopy, regColFlip and regComp
   #
   regPixCopy $reg3 $reg4
   set rval [regComp $reg3 $reg4]
   if {$rval != "same"} {
      echo TEST-ERR: $err regPixCopy - copied region is different from original : $rval
   }
   regPixSetWithDbl $reg4 10 10 5
   set rval [regComp $reg3 $reg4]
   if {[lindex $rval 0] != "row 10"} {
      echo TEST-ERR: $err regComp - region comparison result is not as expected
      echo TEST-ERR: Got: $rval, when expecting \{row 10\} \{col 0\}
   }
   regColFlip $reg4
   set rval [regComp $reg3 $reg4]
   if {[lindex $rval 0] != "row 10"} {
      echo TEST-ERR: $err regComp - region comparison result is not as expected
      echo TEST-ERR: Got: $rval, when expecting \{row 10\} \{col 0\}
   }

   regDel $reg1
   regDel $reg2
   regDel $reg3
   regDel $reg4
}

proc tst_subtraction {} {
# Test subtraction - a more comprehensive test.
# For all types of regions, set a pixels to the numbers in list.
# set another region of the same type to 1, and subtract the "one"
# region from the number regio into a third
# region of the same type. Make sure we get the answers predicted by 
# the "ans" array.

set ntestpix 13
set types {S8 U8 S16 U16 FL32 S32 U32}
set nums    {65537 65536 65535  257 256 255 1  0 -1  -127 -128 -32767 -32768}

set ans(S8)   {126   126     126 126 126 126 0 -1 -2 -128 -128   -128    -128}
set ans(U8)   {254   254     254 254 254 254 0  0  0    0    0      0       0}
set ans(S16)  {32766 32766 32766 256 255 254 0 -1 -2 -128 -129 -32768  -32768}
set ans(U16)  {65534 65534 65534 256 255 254 0  0  0    0    0      0       0}
set ans(FL32) {65536 65535 65534 256 255 254 0 -1 -2 -128 -129 -32768  -32769}
set ans(S32)  {65536 65535 65534 256 255 254 0 -1 -2 -128 -129 -32768  -32769}
set ans(U32)  {65536 65535 65534 256 255 254 0  0  0    0    0      0       0}

foreach t $types {
   set regin [regNew 1 $ntestpix -type $t]
   set c 0
   foreach n $nums { regPixSetWithDbl $regin  0 $c $n ; incr c }
   set regone [regNew 1 $ntestpix -type $t] ; regSetWithDbl $regone 1
   set regout [regNew 1 $ntestpix -type $t]
   regSubtract $regin $regone -regOut $regout

   set c 0
   set regcmp [regNew 1 $ntestpix -type $t]
   foreach n $ans($t) {regPixSetWithDbl $regcmp 0 $c $n ; incr c }
   set cmp [regComp $regcmp $regout]
   if {$cmp != "same"} { 
      echo TEST-ERR: subtracting regions of type $t - $cmp
   }
   regDel $regin     
   regDel $regone    
   regDel $regout     
   regDel $regcmp
}
}

proc tst_division {} {
# Test division -- a more comprehensive test
# For all types of regions, set a pixels to the numbers in list.
# set another region of the same type to 10 into a third
# region of the sam type. Make sure we get the answers predicted by 
# the "Ans" array.

set ntestpix 7
set types {S8 U8 S16 U16 FL32 S32 U32}
set divisors {1   10 100 1000 10000 100000 1000000}

set ans(S8)   {0   1  10   13    13     13      13}
set ans(U8)   {0   1  10   26    26     26      26}
set ans(S16)  {0   1  10  100  1000   3277    3277}
set ans(U16)  {0   1  10  100  1000   6554    6554}
set ans(FL32) {0.1 1  10  100  1000  10000  100000}
set ans(S32)  {0   1  10  100  1000  10000  100000}
set ans(U32)  {0   1  10  100  1000  10000  100000}

foreach t $types {
   set regin [regNew 1 $ntestpix -type $t]
   set c 0
   foreach d $divisors { regPixSetWithDbl $regin  0 $c $d ; incr c }
   set regten [regNew 1 $ntestpix -type $t] ; regSetWithDbl $regten 10
   set regout [regNew 1 $ntestpix -type $t]
   regDivide $regin $regten -regOut $regout

   set c 0
   set regcmp [regNew 1 $ntestpix -type $t]
   foreach n $ans($t) {regPixSetWithDbl $regcmp 0 $c $n ; incr c }
   set cmp [regComp $regcmp $regout]
   if {$cmp != "same"} { 
      echo TEST-ERR: dividing regions of type $t - $cmp
      echo  $regin     
      echo  $regten    
      echo  $regout     
      echo  $regcmp
      return
   }
   regDel  $regin     
   regDel  $regten    
   regDel  $regout     
   regDel  $regcmp
}
}

proc tst_regSetWithDbl {} {
   set types {S8 U8 S16 U16 FL32 S32 U32}
   set val(S8)   {-129    -128 -0.6 0.5   127   128}
   set ans(S8)   {-128    -128   -1   1   127   127}

   set val(U8)   { -1   0     0.4  0.6  255   256}
   set ans(U8)   {  0   0       0    1  255   255}

   set val(S16)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S16)  {-32768 -32768   -1   1  32767 32767}

   set val(U16)  {-1    0      0.4  0.6   65535 65536}
   set ans(U16)  { 0    0        0    1   65535 65535}

   set val(FL32) { -0.1   0.0   0.1}
   set ans(FL32) { -0.1   0.0   0.1}

   set val(S32)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S32)  {-32769 -32768   -1   1  32767 32768}

   set val(U32)  {-1    0      0.4  0.6   65535 65536}
   set ans(U32)  { 0    0        0    1   65535 65536}

   foreach t $types {
      set reg [regNew 10 10 -type $t]
      set i 0
      foreach v $val($t) {
         regSetWithDbl $reg $v
         set a [lindex $ans($t) $i] ; incr i
         set r [regPixGet $reg 0 0]
         if {$a != $r} {echo TEST-ERR: from type $t set $v read $r, expect $a}
      }	
      regDel $reg

   }
}

proc tst_regAdd {} {
   set types {S8 U8 S16 U16 FL32 S32 U32}
   set val(S8)   {-129    -128 -0.6 0.5   127   128}
   set ans(S8)   {-127    -127    0   2   127   127}

   set val(U8)   { -1   0     0.4  0.6  255   256}
   set ans(U8)   {  1   1       1    2  255   255}

   set val(S16)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S16)  {-32767 -32767    0   2  32767 32767}

   set val(U16)  {-1    0      0.4  0.6   65535 65536}
   set ans(U16)  { 1    1        1    2   65535 65535}

   set val(FL32) { -0.1   0.0   0.1}
   set ans(FL32) {  0.9   1.0   1.1}

   set val(S32)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S32)  {-32768 -32767    0   2  32768 32769}

   set val(U32)  {-1    0      0.4  0.6   65535 65536}
   set ans(U32)  { 1    1        1    2   65536 65537}

   foreach t $types {

      set regout [regNew 16 16 -type $t]
      set regval [regNew 16 16 -type $t]
      set regone [regNew 16 16 -type $t]
      regSetWithDbl $regone 1 

      set i 0
      foreach v $val($t) {
         regSetWithDbl $regval $v
         set a [lindex $ans($t) $i] ; incr i
	 regAdd $regval $regone -regOut $regout
         set r [regPixGet $regout 0 0]
         if {$a != $r} {echo TEST-ERR: from type $t set $v+1  read $r, expect $a}
      }	
      regDel $regout
      regDel $regval
      regDel $regone

   }
}

proc tst_regAddWithDbl {} {
   set types {S8 U8 S16 U16 FL32 S32 U32}

   set val(S8)   {-129    -128 -0.6 0.5   127   128}
   set ans(S8)   {-128    -128   -1   1   127   127}

   set val(U8)   { -1   0     0.4  0.6  255   256}
   set ans(U8)   {  0   0       0    1  255   255}

   set val(S16)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S16)  {-32768 -32768   -1   1  32767 32767}

   set val(U16)  {-1    0      0.4  0.6   65535 65536}
   set ans(U16)  { 0    0        0    1   65535 65535}

   set val(FL32) { -0.1   0.0   0.1}
   set ans(FL32) { -0.1   0.0   0.1}

   set val(S32)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S32)  {-32769 -32768   -1   1  32767 32768}

   set val(U32)  {-1    0      0.4  0.6   65535 65536}
   set ans(U32)  { 0    0        0    1   65535 65536}

   foreach t $types {

      set reg [regNew 16 16 -type $t] 

      set i 0
      foreach v $val($t) {
         regSetWithDbl $reg 0
         regAddWithDbl $reg $v
         set r [regPixGet $reg 0 0]
	 set a [lindex $ans($t) $i] ; incr i
         if {$a != $r} {echo TEST-ERR: from type $t added $v+0 read $r, expected $a}
      }	
      regDel $reg
   }
}

#
# test region coping by copying to float, and seeing if you
# get back the original frame.
#
proc tst_regPixCopy {} {
   set ntypes 7
   set types {  S8  U8    S16    U16    FL32     S32    U32}
   set tmax  { 127 255  32767  65535  100000  100000 100000}
   set tmin  {-128   0 -32768      0 -100000 -100000      0}

   loop i 0 7 {
     set t   [lindex $types $i]
     set max [lindex $tmax  $i]
     set min [lindex $tmin  $i]
     set reg [regNew 16 16 -type $t] 

     regSetWithDbl $reg $max
     set regto  [regNewFromReg $reg -type FL32]
     set regfro [regNewFromReg $regto -type FL32]
     set cmp [regComp $reg $regto]
     if {$cmp != "same" } {
          echo TEST-ERR: could not transform $t MAX $cmp
     }
     regDel $regto 
     regDel $regfro


     regSetWithDbl $reg $min
     set regto  [regNewFromReg $reg -type FL32]
     set regfro [regNewFromReg $regto -type FL32]
     set cmp [regComp $reg $regto]
     if {$cmp != "same" } {
          echo TEST-ERR: could not transform $t MIN $cmp
     }

     regDel $regto 
     regDel $regfro
     regDel $reg

   }
}


proc tst_regAddRow {} {
   set types {S8 U8 S16 U16 FL32 S32 U32}
   set val(S8)   {-129    -128 -0.6 0.5   127   128}
   set ans(S8)   {-127    -127    0   2   127   127}

   set val(U8)   { -1   0     0.4  0.6  255   256}
   set ans(U8)   {  1   1       1    2  255   255}

   set val(S16)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S16)  {-32767 -32767    0   2  32767 32767}

   set val(U16)  {-1    0      0.4  0.6   65535 65536}
   set ans(U16)  { 1    1        1    2   65535 65535}

   set val(FL32) { -0.1   0.0   0.1}
   set ans(FL32) {  0.9   1.0   1.1}

   set val(S32)  {-32769 -32768 -0.6 0.6  32767 32768}
   set ans(S32)  {-32768 -32767    0   2  32768 32769}

   set val(U32)  {-1    0      0.4  0.6   65535 65536}
   set ans(U32)  { 1    1        1    2   65536 65537}

   foreach t $types {

      set regout [regNew 16 16 -type $t]
      set regval [regNew 16 16 -type $t]
      set regone [regNew  1 16 -type $t]
      regSetWithDbl $regone 1 

      set i 0
      foreach v $val($t) {
         regSetWithDbl $regval $v
         set a [lindex $ans($t) $i] ; incr i
	 regAddRow $regval $regone -regOut $regout
         set r [regPixGet $regout 1 1]
         if {$a != $r} {echo TEST-ERR: regAddRow, type $t set $v+1  read $r, expect $a}
      }	
      regDel $regout
      regDel $regval
      regDel $regone

   }
}

proc tst_regSubRow {} {

# Test subtraction - a more comprehensive test.
# For all types of regions, set a pixels to the numbers in list.
# set another region of the same type to 1, and subtract the "one"
# region from the number regio into a third
# region of the same type. Make sure we get the answers predicted by 
# the "ans" array.

set ntestpix 13
set types {S8 U8 S16 U16 FL32 S32 U32}
set nums    {65537 65536 65535  257 256 255 1  0 -1  -127 -128 -32767 -32768}

set ans(S8)   {126   126     126 126 126 126 0 -1 -2 -128 -128   -128    -128}
set ans(U8)   {254   254     254 254 254 254 0  0  0    0    0      0       0}
set ans(S16)  {32766 32766 32766 256 255 254 0 -1 -2 -128 -129 -32768  -32768}
set ans(U16)  {65534 65534 65534 256 255 254 0  0  0    0    0      0       0}
set ans(FL32) {65536 65535 65534 256 255 254 0 -1 -2 -128 -129 -32768  -32769}
set ans(S32)  {65536 65535 65534 256 255 254 0 -1 -2 -128 -129 -32768  -32769}
set ans(U32)  {65536 65535 65534 256 255 254 0  0  0    0    0      0       0}

foreach t $types {
   set regin [regNew 1 $ntestpix -type $t]
   set c 0
   foreach n $nums { regPixSetWithDbl $regin  0 $c $n ; incr c }
   set regone [regNew 1 $ntestpix -type $t] ; regSetWithDbl $regone 1
   set regout [regNew 1 $ntestpix -type $t]
   regSubRow $regin $regone -regOut $regout

   set c 0
   set regcmp [regNew 1 $ntestpix -type $t]
   foreach n $ans($t) {regPixSetWithDbl $regcmp 0 $c $n ; incr c }
   set cmp [regComp $regcmp $regout]
   if {$cmp != "same"} { 
      echo TEST-ERR: regSubRow : type $t - $cmp $regout $regcmp
      return
   }
   regDel $regin     
   regDel $regone    
   regDel $regout     
   regDel $regcmp
}
}

proc tst_regSubCol {} {

# Test subtraction - a more comprehensive test.
# For all types of regions, set a pixels to the numbers in list.
# set another region of the same type to 1, and subtract the "one"
# region from the number regio into a third
# region of the same type. Make sure we get the answers predicted by 
# the "ans" array.

set ntestpix 13
set types {S8 U8 S16 U16 FL32 S32 U32}
set nums    {65537 65536 65535  257 256 255 1  0 -1  -127 -128 -32767 -32768}

set ans(S8)   {126   126     126 126 126 126 0 -1 -2 -128 -128   -128    -128}
set ans(U8)   {254   254     254 254 254 254 0  0  0    0    0      0       0}
set ans(S16)  {32766 32766 32766 256 255 254 0 -1 -2 -128 -129 -32768  -32768}
set ans(U16)  {65534 65534 65534 256 255 254 0  0  0    0    0      0       0}
set ans(FL32) {65536 65535 65534 256 255 254 0 -1 -2 -128 -129 -32768  -32769}
set ans(S32)  {65536 65535 65534 256 255 254 0 -1 -2 -128 -129 -32768  -32769}
set ans(U32)  {65536 65535 65534 256 255 254 0  0  0    0    0      0       0}

foreach t $types {
   set regin [regNew $ntestpix 1 -type $t]
   set r 0
   foreach n $nums { regPixSetWithDbl $regin  $r 0 $n ; incr r }
   set regone [regNew $ntestpix 1 -type $t] ; regSetWithDbl $regone 1
   set regout [regNew $ntestpix 1 -type $t]
   regSubCol $regin $regone -regOut $regout

   set r 0
   set regcmp [regNew $ntestpix 1 -type $t]
   foreach n $ans($t) {regPixSetWithDbl $regcmp $r 0 $n ; incr r }
   set cmp [regComp $regcmp $regout]
   if {$cmp != "same"} { 
      echo TEST-ERR: regSubCol : type $t - $cmp $regout $regcmp
      return
   }
   regDel $regin     
   regDel $regone    
   regDel $regout     
   regDel $regcmp
}
}

proc tst_regMultWithRow {} {
   set types {S8 U8 S16 U16 FL32 S32 U32}
   set val(S8)   {-129    -128 -0.6 0 0.5  10  127    128}
   set ans(S8)   { 127     127    1 0  -1 -10  -127   -127}

   set val(U8)   { -1   0     0.4  0.6  10 255   256}
   set ans(U8)   {  0   0       0    2  20 255   255}

   set val(S16)  {-32769 -32768 -0.6 0.6   10  32767  32768}
   set ans(S16)  { 32767  32767    1  -1  -10 -32767 -32767}

   set val(U16)  {-1    0      0.4  0.6   10 65535 65536}
   set ans(U16)  { 0    0        0    2   20 65535 65535}

   set val(FL32) { -0.1   0.0    0.1   10}
   set ans(FL32) {  0.1   0.0   -0.1  -10}

   set val(S32)  {-32769 -32768 -0.6 0.6   10  32767  32768}
   set ans(S32)  { 32769  32768    1  -1  -10 -32767 -32768}

   set val(U32)  {-1    0      0.4  0.6  10   65535   65536}
   set ans(U32)  { 0    0        0    2  20  131070  131072}

   foreach t $types {

      set regout [regNew 16 16 -type $t]
      set regval [regNew 16 16 -type $t]
      set regone [regNew  1 16 -type $t]
      regSetWithDbl $regone -1
      if {"$t" == "U8"}  {regSetWithDbl $regone 2}
      if {"$t" == "U16"} {regSetWithDbl $regone 2}
      if {"$t" == "U32"} {regSetWithDbl $regone 2}

      set i 0
      foreach v $val($t) {
         regSetWithDbl $regval $v
         set a [lindex $ans($t) $i] ; incr i
	 regMultWithRow $regval $regone -regOut $regout
         set r [regPixGet $regout 1 1]
         if {$a != $r} {echo TEST-ERR: regMultWithRow, type $t  read $r, expect $a}
      }	
      regDel $regout
      regDel $regval
      regDel $regone

   }
}


proc tst_regDivByRow {} {
   set types {S8 U8 S16 U16 FL32 S32 U32}
   set val(S8)   {-129    -128 -0.6 0 0.5  10  127    128}
   set ans(S8)   { 127     127    1 0  -1 -10  -127   -127}

   set val(U8)   { -1   0     0.4  0.6  10 255   256}
   set ans(U8)   {  0   0       0    1   5 128   128}

   set val(S16)  {-32769 -32768 -0.6 0.6   10  32767  32768}
   set ans(S16)  { 32767  32767    1  -1  -10 -32767 -32767}

   set val(U16)  {-1    0      0.4  0.6   10 65535 65536}
   set ans(U16)  { 0    0        0    1    5 32768 32768}

   set val(FL32) { -0.1   0.0    0.1   10}
   set ans(FL32) {  0.1   0.0   -0.1  -10}

   set val(S32)  {-32769 -32768 -0.6 0.6   10  32767  32768}
   set ans(S32)  { 32769  32768    1  -1  -10 -32767 -32768}

   set val(U32)  {-1    0      0.4  0.6  10   65535  131072  1000000}
   set ans(U32)  { 0    0        0    1   5   32768   65536   500000}

   foreach t $types {

      set regout [regNew 16 16 -type $t]
      set regval [regNew 16 16 -type $t]
      set regone [regNew  1 16 -type $t]
      regSetWithDbl $regone -1
      if {"$t" == "U8"}  {regSetWithDbl $regone 2}
      if {"$t" == "U16"} {regSetWithDbl $regone 2}
      if {"$t" == "U32"} {regSetWithDbl $regone 2}

      set i 0
      foreach v $val($t) {
         regSetWithDbl $regval $v
         set a [lindex $ans($t) $i] ; incr i
	 regDivByRow $regval $regone -regOut $regout
         set r [regPixGet $regout 1 1]
         if {$a != $r} {echo TEST-ERR: regDivByRow, type $t  read $r, expect $a}
      }	
      regDel $regout
      regDel $regval
      regDel $regone

   }
}

tst_regDivByRow
testRegUtils
tst_subtraction
tst_division
tst_regSetWithDbl
tst_regAdd
tst_regAddWithDbl
tst_regPixCopy
tst_regAddRow
tst_regSubRow
tst_regMultWithRow


# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_util.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_util"
    return 1
}

return

