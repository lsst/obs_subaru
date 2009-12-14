#
# Non-comprehensive tests for utils code
#
proc testRegUtils {} {
   set ret 0
#
# Test regIntSetVal, regIntConstAdd, and regClear for U16
#
   set reg [regNew -type U16 10 10]
   regIntSetVal $reg 10
   if {[exprGet ((short**)$reg.rows_u16)<5><5>] != 10} {
      echo TEST-ERR: in testRegUtils: setting unsigned short region
      set ret 1
   }
   regClear $reg
   if {[exprGet ((short**)$reg.rows)<5><5>] != 0} {
      echo TEST-ERR: in testRegUtils: clearing unsigned short region
      set ret 1
   }
   regIntConstAdd $reg 123
   if {[exprGet ((short**)$reg.rows)<5><5>] != 123} {
      echo TEST-ERR: in testRegUtils: setting unsigned short region to a const
      set ret 1
   }
   regDel $reg
#
# Test regIntSetVal and regClear for S16
#
   set reg [regNew -type S16 10 10]
   regIntSetVal $reg 10
   if {[exprGet ((short**)$reg.rows_s16)<5><5>] != 10} {
      echo TEST-ERR: in testRegUtils: setting signed short region
      set ret 1
   }
   regClear $reg
   if {[exprGet ((short**)$reg.rows_s16)<5><5>] != 0} {
      echo TEST-ERR: in testRegUtils: clearing signed short region
      set ret 1
   }
   regIntConstAdd $reg 123
   if {[exprGet ((short**)$reg.rows_s16)<5><5>] != 123} {
      echo TEST-ERR: in testRegUtils: setting signed short region to a const
      set ret 1
   }
   regDel $reg
#
# Test regStatsFromQuartiles; only {mean,sigma,min,max} so far
#
   set reg [regNew 500 500]
   regIntSetVal $reg 200
   set tmp [subRegNew $reg 500 250 0 0]
   regIntSetVal $tmp 400
   regDel $tmp
   regStatsFromQuartiles $reg -mean mean -sigma sig
   if { $mean != 300 || $sig != 100 } {
      echo mean = $mean, sigma = $sig
      echo TEST-ERR: in testRegUtils: finding mean and sigma for a region
      set ret 1
   }
   regPixSetWithDbl $reg 5 5 10000
   regPixSetWithDbl $reg 20 20 123
   regStatsFromQuartiles -coarse 1 $reg -min min -max max
   if { $min != 123 || $max != 10000 } {
      echo min = $min, max = $max
      echo TEST-ERR: in testRegUtils: finding min and max for a region
      set ret 1
   }
   regDel $reg

   return $ret
}

#
# Now some misc tests that don't belong anywhere else
#
proc testMiscUtils {} {
   set failed 0

   set nrow 200; set ncol 180
   set reg [regNew $nrow $ncol]
   set psf [dgpsfNew -sigmax1 1 -sigmax2 3 -b 0.111]
   #
   # objmaskFromRect
   #
   set om [objmaskFromRect 100 120 130 140]

   #display_objmask $om $reg

   objmaskDel $om
   #
   # objmaskFromCircle
   #
   set I0 9000
   set rowc1 30.5; set colc1 40.5
   set rowc 100.5; set colc 120.5; set r 3

   regIntSetVal $reg 1000
   set om [objmaskFromCircle [expr int($rowc)] [expr int($colc)] $r]

   dgpsfAddToReg $reg $psf $rowc1 $colc1 -a $I0
   dgpsfAddInObjmask $reg $om $psf $rowc $colc -a $I0

   #display_objmask $om $reg

   if {[regPixGet $reg [expr int($rowc1)] [expr int($colc1)]] != $I0 + 1000} {
      echo "TEST-ERR: failed to add PSF (no mask)"
      incr failed
   }
   if {[regPixGet $reg [expr int($rowc)] [expr int($colc)]] != $I0 + 1000} {
      echo "TEST-ERR: failed to add PSF"
      incr failed
   }
   if {[regPixGet $reg [expr int($rowc)-1] [expr int($colc)-3]] != 1000} {
      echo "TEST-ERR: added PSF outside mask at ([expr int($rowc)-1],[expr int($colc)-3])"
      incr failed
   }

   objmaskDel $om
   #
   # objmaskFromCircle test 2: subtract a PSF. Also use -outer this time
   #
   regIntSetVal $reg 1000
   set om [objmaskFromCircle [expr int($rowc)] [expr int($colc)] $r]

   dgpsfAddToReg $reg $psf $rowc $colc -a $I0
   set row [expr int($rowc)-1]; set col [expr int($colc)-3]; set oval [regPixGet $reg $row $col]
   
   dgpsfAddInObjmask -outer 20 $reg $om $psf $rowc $colc -a -$I0

   #display_objmask $om $reg

   if {[regPixGet $reg [expr int($rowc)] [expr int($colc)]] != 1000} {
      echo "TEST-ERR: failed to subtract PSF"
      incr failed
   }
   if {[regPixGet $reg $row $col] != $oval} {
      echo "TEST-ERR: subtracted PSF outside mask (using subregions) at ($row,$col)"
      incr failed
   }

   objmaskDel $om
   #
   # objmaskFromCircle test 3: subtract a PSF using sub-regions
   #
   regIntSetVal $reg 1000
   set sreg [subRegNew $reg [expr $nrow - 20] [expr $ncol - 20] 10 10]
   set om [objmaskFromCircle [expr int($rowc)] [expr int($colc)] $r]

   dgpsfAddToReg $reg $psf $rowc1 $colc1 -a $I0

   dgpsfAddToReg $sreg $psf $rowc $colc -a $I0
   dgpsfAddInObjmask $sreg $om $psf $rowc $colc -a -$I0

   #display_objmask $om $reg

   if {[regPixGet $reg [expr int($rowc1)] [expr int($colc1)]] != $I0 + 1000} {
      echo "TEST-ERR: failed to add PSF (no mask, using subregions)"
      incr failed
   }
   if {[regPixGet $reg [expr int($rowc)] [expr int($colc)]] != 1000} {
      echo "TEST-ERR: failed to subtract PSF (using subregions) at ([expr int($rowc)],[expr int($colc)])"
      incr failed
   }
   if {[regPixGet $reg [expr int($rowc)-1] [expr int($colc)-3]] != 1570} {
      echo "TEST-ERR: subtracted PSF outside mask (using subregions) at ([expr int($rowc)-1],[expr int($colc)-3])"
      incr failed
   }

   objmaskDel $om; regDel $sreg
   #
   # Clean up
   #
   regDel $reg; dgpsfDel $psf
   #
   # Test the region-to-vectors code
   #
   set reg [regNew 400 200]; regIntSetVal $reg 1000
   set sreg [subRegNew $reg 400 50 0 100];
   regIntSetVal $sreg 2000; regDel $sreg
   
   #mtv $reg 
   echo Collapse onto a row
   set vec [vectorGetFromRegion $reg 10 390 20 180 0 -nbin 3 -median]
   if {[vectorExprGet $vec<79>] != 1000 || [vectorExprGet $vec<80>] != 2000 ||
       [vectorExprGet $vec<129>] != 2000 || [vectorExprGet $vec<130>] !=1000} {
      echo "TEST-ERR: failed to collapse vector onto a row"
      echo [vectorExprGet $vec<79>] != 1000 || \
	  [vectorExprGet $vec<80>] != 2000 || \
	  [vectorExprGet $vec<129>] != 2000 || \
	  [vectorExprGet $vec<130>] !=1000
      incr failed
   }
   #vectorExprPrint $vec
   vectorExprDel $vec

   regIntSetVal $reg 1000
   
   echo Collapse onto a column
   set vec [vectorGetFromRegion $reg 10 390 20 180 1 -nbin 3 -median]
   if {[vectorExprGet $vec<0>] != 1000} {
      echo "TEST-ERR: failed to collapse vector onto a column"
      echo [vectorExprGet $vec<0>] != 1000
      incr failed
   }
   #vectorExprPrint $vec
   vectorExprDel $vec

   echo Collapse onto a column
   set vecs [vectorGetFromRegion $reg 10 390 20 180 1 -nbin 3 -median -quart]
   if {[vectorExprGet [lindex $vecs 1]<0>] != 1000} {
      echo "TEST-ERR: failed to collapse vector onto a column"
      echo [vectorExprGet $vec<0>] != 1000
      incr failed
   }
   #vectorExprPrint $vec
   foreach vec $vecs {
      vectorExprDel $vec
   }
   #
   # Clean up
   #
   regDel $reg

   return $failed
}

#
# Run all the tests
#
set rand [phRandomNew 100000 -seed 12345 -type 2]

set failed 0
incr failed [testRegUtils]
incr failed [testMiscUtils]

global interactive
if {[catch {set interactive}]} {
   exit $failed;
}
