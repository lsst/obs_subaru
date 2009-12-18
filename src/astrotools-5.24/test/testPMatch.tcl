# 
#   This function contains TCL regression tests for the routines
#   in tclPMatch.c (and atPMatch.c): matching lists of objects
#   and figuring out transformations between the coordinate systems.
#

#############################################################################
# create a CHAIN we can use as a test, by creating a temporary 
# ASCII parameter file and then using 'param2Chain'.
#
# Note that we allow the "x", "y" and "mag" values to have any
# one of the types 
#                  int, float, double
#
# 
proc make_chain_1 { } {

  set temp_file "/tmp/tst_match.tcl" 
  set temp_id [open $temp_file w]
  
  puts $temp_id "typedef struct {" 
  puts $temp_id "  int id;"
  puts $temp_id "  double xpos;"
  puts $temp_id "  int    ypos;"
  puts $temp_id "  float  mag;"
  puts $temp_id "} CATSTAR;"
  puts $temp_id " " 
  puts $temp_id "CATSTAR   1    2.0   20    12.0  "
  puts $temp_id "CATSTAR   2    4.0    6    12.9  "
  puts $temp_id "CATSTAR   3    7.0   37    12.8  "
  puts $temp_id "CATSTAR   4   11.0    8    12.3  "
  puts $temp_id "CATSTAR   5   16.0   49    12.4  "
  puts $temp_id "CATSTAR   6   22.0   10    12.5  "
  puts $temp_id "CATSTAR   7   29.0   51    12.6  "
  puts $temp_id "CATSTAR   8   37.0   12    12.7  "
  puts $temp_id "CATSTAR   9   48.0   63    12.2  "
  puts $temp_id "CATSTAR  10   58.0   14    12.1  "
  puts $temp_id "CATSTAR  11   69.0   75    12.0  "

  close $temp_id


  # now read it into a CHAIN
  set ch [param2Chain $temp_file hdr]

  # remove the temporary file
  exec /bin/rm $temp_file

  # and return the CHAIN
  return $ch
}


###########################################################################
# given a chain of objects, multiply all the coordinate values by 2.0
#
proc magnify_chain { ch xname yname magnification } {

  set size [chainSize $ch]
  loop i 0 $size {
    set el [chainElementGetByPos $ch $i] 
    set x [exprGet $el.$xname]
    set y [exprGet $el.$yname]
    set x [expr $x*$magnification]
    set y [expr $y*$magnification]
    handleSet $el.$xname $x
    handleSet $el.$yname $y

    handleDel $el
  }

}


###########################################################################
# given a chain of objects, fiddle with it a bit so that it's not quite
# the same (so matches won't be perfect).
#
proc modify_chain { ch xname yname } {

  # size of typical movement of original coordinate
  set offset 2.0

  set size [chainSize $ch]
  loop i 0 $size {
    set el [chainElementGetByPos $ch $i] 
    set x [exprGet $el.$xname]
    set y [exprGet $el.$yname]
    set x [expr $x+$offset*[randGet]]
    set y [expr $y+$offset*[randGet]]
    handleSet $el.$xname $x
    handleSet $el.$yname $y

    handleDel $el
  }

}


#############################################################################
# try using "afTriMatch" on a given pair of chains 
#
# This might be useful in debugging, or for a comparison of the
# two methods of matching items.
#
#
proc try_tri_match { chainA chainB xname yname } {

  # create an AF with "x" values and "y" values for the first chain
  set sizeA [chainSize $chainA]
  set afx_A [afNew]
  afDefine $afx_A -nValue $sizeA
  set afy_A [afNew]
  afDefine $afy_A -nValue $sizeA
  loop i 0 $sizeA {
    set el [chainElementGetByPos $chainA $i]
    afSet $afx_A $i [exprGet $el.$xname] -error 1
    afSet $afy_A $i [exprGet $el.$yname] -error 1
    handleDel $el
  }

  # create an AF with "x" values and "y" values for the SECOND chain
  set sizeB [chainSize $chainB]
  set afx_B [afNew]
  afDefine $afx_B -nValue $sizeB
  set afy_B [afNew]
  afDefine $afy_B -nValue $sizeB
  loop i 0 $sizeB {
    set el [chainElementGetByPos $chainB $i]
    afSet $afx_B $i [exprGet $el.$xname] -error 1
    afSet $afy_B $i [exprGet $el.$yname] -error 1
    handleDel $el
  }

  # okay, ready to call "afTriMatch"
  set trans \
      [afTriMatch $afx_A $afy_A $afx_B $afy_B -ne $sizeA -nm $sizeB -idb 1]
  echo [exprGet $trans]

  return 0
}



#############################################################################
# compare the two given numbers -- if they differ by more than the given
# "delta", then return 1. Otherwise, return 0.
#
proc not_same { num1 num2 delta } {

  if { [fabs [expr $num1-$num2]] > $delta } {
    return 1
  }
  return 0
}


#############################################################################
# check to see if two chains, one of which has coords which are exact
# multiples of the others, yield very-close-to-perfect
# TRANS coefficients:
#
#       a = 0.0, b = magnification, c = 0.0
#       d = 0.0, e = 0.0,           f = magnification
#
proc test_magnified { ch1 ch2 magnification } {

  set failed 0

  if { $magnification == 0.0 } {
    echo "test_magnified can't be given magnification = 0.0"
    return 1
  }
  set scale [expr 1.0/$magnification]

  if { [catch { set trans [atFindTrans $ch1 xpos ypos mag $ch2 xpos ypos mag \
                               -scale $scale] } ] == 1 } {
    return 1
  }

  if { [not_same [exprGet $trans.a] 0.0 0.001]  || \
       [not_same [exprGet $trans.b] $magnification 0.001]  || \
       [not_same [exprGet $trans.c] 0.0 0.001]  || \
       [not_same [exprGet $trans.d] 0.0 0.001]  || \
       [not_same [exprGet $trans.e] 0.0 0.001]  || \
       [not_same [exprGet $trans.f] $magnification 0.001] || \
       [not_same [exprGet $trans.dRow0] 0.0 0.001]  || \
       [not_same [exprGet $trans.dRow1] 0.0 0.001]  || \
       [not_same [exprGet $trans.dRow2] 0.0 0.001]  || \
       [not_same [exprGet $trans.dRow3] 0.0 0.001]  || \
       [not_same [exprGet $trans.dCol0] 0.0 0.001]  || \
       [not_same [exprGet $trans.dCol1] 0.0 0.001]  || \
       [not_same [exprGet $trans.dCol2] 0.0 0.001]  || \
       [not_same [exprGet $trans.dCol3] 0.0 0.001]  || \
       [not_same [exprGet $trans.csRow] 0.0 0.001]  || \
       [not_same [exprGet $trans.csCol] 0.0 0.001]  || \
       [not_same [exprGet $trans.ccRow] 0.0 0.001]  || \
       [not_same [exprGet $trans.ccCol] 0.0 0.001]  || \
       [not_same [exprGet $trans.riCut] 0.0 0.001] } {
    set failed 1
  }
  transDel $trans

  return $failed
}



#############################################################################
# check to see if two IDENTICAL chains yield very-close-to-perfect
# TRANS coefficients:
#
#       a = 0.0, b = 1.0, c = 0.0
#       d = 0.0, e = 0.0, f = 1.0
#
proc test_identical { ch1 ch2 } {

  set failed 0

  set trans [atFindTrans $ch1 xpos ypos mag $ch2 xpos ypos mag]

  if { [not_same [exprGet $trans.a] 0.0 0.001]  || \
       [not_same [exprGet $trans.b] 1.0 0.001]  || \
       [not_same [exprGet $trans.c] 0.0 0.001]  || \
       [not_same [exprGet $trans.d] 0.0 0.001]  || \
       [not_same [exprGet $trans.e] 0.0 0.001]  || \
       [not_same [exprGet $trans.f] 1.0 0.001] } {
    set failed 1
  }
  transDel $trans

  return $failed
}


#############################################################################
# check to see if two ALMOST-IDENTICAL chains yield sort-of-perfect 
# TRANS coefficients: perfect would be
#
#       a = 0.0, b = 1.0, c = 0.0
#       d = 0.0, e = 0.0, f = 1.0
#
# but, because we've modified the positions of objects slightly
# in ch2, allow 
#      the offsets "a" and "d" to be as much as 0.0 +/- 4.0
#      the coeffs  "b" and "f" to be as much as 1.0 +/- 0.05
#      the coeffs  "e" and "c" to be as much as 0.0 +/- 0.05
#
# Note that since we've made the objects no longer match up exactly,
# we have to increase the radius in triangle-space which denotes a
# matched pair of objects.  The default 0.002 yields only 2 or 3
# matched triangles, and we need many more than that to succeed.
#
proc test_almost_identical { ch1 ch2 } {

  set failed 0

  set trans [atFindTrans $ch1 xpos ypos mag $ch2 xpos ypos mag -radius 0.02]

  if { [not_same [exprGet $trans.a] 0.0 4.00 ]  || \
       [not_same [exprGet $trans.b] 1.0 0.05 ]  || \
       [not_same [exprGet $trans.c] 0.0 0.05 ]  || \
       [not_same [exprGet $trans.d] 0.0 4.00 ]  || \
       [not_same [exprGet $trans.e] 0.0 0.05 ]  || \
       [not_same [exprGet $trans.f] 1.0 0.05 ] } {
    set failed 1
  }
  transDel $trans

  return $failed
}
   

#############################################################################
# check to see if the "atApplyTrans" verb works.
#
# We make a copy of a CHAIN, then create a new TRANS structure that
# has coeffs
#
#       a = 1.0, b = 1.0, c = 0.0
#       d = 0.0, e = 0.0, f = 1.0
#
# When we apply this TRANS to the copy, we expect the coordinates
# to be 
#
#          x' = x + 1
#          y' = y'
#
proc test_apply_trans { } {

  set failed 0

  set xname xpos
  set yname ypos

  set ch1 [make_chain_1]
  set ch2 [make_chain_1]

  set trans [transNew]
  handleSet $trans.a 1.0; handleSet $trans.b 1.0; handleSet $trans.c 0.0
  handleSet $trans.d 0.0; handleSet $trans.e 0.0; handleSet $trans.f 1.0
  atApplyTrans $ch2 $xname $yname $trans

  set size [chainSize $ch1]
  loop i 0 $size {
    set el  [chainElementGetByPos $ch1 $i]
    set el2 [chainElementGetByPos $ch2 $i]

    set x  [exprGet $el.$xname]
    set x2 [exprGet $el2.$xname]
    set y  [exprGet $el.$yname]
    set y2 [exprGet $el2.$yname]

    if { [not_same $x [expr $x2-1] 0.0 ] || \
         [not_same $y $y2          0.0 ] } {
      set failed 1
    }

    handleDel $el
    handleDel $el2
  }

  transDel $trans
  chainDestroy $ch1 genericDel
  chainDestroy $ch2 genericDel

  return $failed
}

#############################################################################
# check to see if the "atMatchChains" verb works.
#
# We make a simple CHAIN, then a copy of it.  Then we modify 
# the last element of the copy, so that it doesn't match
# the original any more.  We call "matchChains", and check 
# to see that 
#
#       - the two "matched" output chains have N-1 elements
#       - the two "unmatched" output chains have 1 element
#
proc test_match_chains { } {

  set failed 0

  set xname xpos
  set yname ypos

  set ch1 [make_chain_1]
  set ch2 [make_chain_1]

  # now fiddle with the last element on ch2
  set size [chainSize $ch2]
  set el [chainElementGetByPos $ch2 [expr $size-1]]
  handleSet $el.$xname 999
  handleSet $el.$yname 999
  handleDel $el

  # now match up the chains
  atMatchChains $ch1 $xname $yname $ch2 $xname $yname 0.1 \
                     chainJ chainK chainL chainM

  # make sure that each of J,K has $size-1 elements
  set size1 [expr $size-1]
  set size2 [chainSize $chainJ]
  if { $size2 != $size1 } {
    echo "test_match_chains: chainJ should have $size1, has $size2"
    set failed 1
  }
  set size2 [chainSize $chainK]
  if { $size2 != $size1 } {
    echo "test_match_chains: chainK should have $size1, has $size2"
    set failed 1
  }
  # check that each of L,M has 1 element
  set size2 [chainSize $chainL]
  if { $size2 != 1 } {
    echo "test_match_chains: chainL should have 1, has $size2"
    set failed 1
  }
  set size2 [chainSize $chainM]
  if { $size2 != 1 } {
    echo "test_match_chains: chainM should have 1, has $size2"
    set failed 1
  }


  # clean up
  chainDel $chainJ; chainDel $chainK; chainDel $chainL; chainDel $chainM
  chainDestroy $ch1 genericDel
  chainDestroy $ch2 genericDel

  return $failed
}






#############################################################################
# print out a CHAIN of x, y, and mag values
# useful for debugging.
#
proc print_chain { ch } {

  loop i 0 [chainSize $ch] {
    set el [chainElementGetByPos $ch $i]
    set x [exprGet $el.xpos]
    set y [exprGet $el.ypos]
    set mag [exprGet $el.mag]
    echo [format "%4d %7.1f %7.1f %6.2f\n" $i $x $y $mag]
    handleDel $el
  }
}


###########################################################################
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
  
  return 0
}



#############################################################################
# run all the tests in this file, and call the "error" function if any fail.
# If this isn't an interactive session, return the status code.
#

proc testPMatch {} {

  # first, use two chains that are EXACTLY the same
  set ch1 [make_chain_1]
  set ch2 [make_chain_1]

  # Test the identical chains
  if { [set n [ test_identical $ch1 $ch2 ]] != 0 } {
    echo "TEST-ERR: matching identical sets of points fails"
    return 1
  }

  # Test a pair of chains which are exactly the same, but one of which
  #   has all coords twice as big as the other
  set ch3 [make_chain_1]
  magnify_chain $ch3 xpos ypos 2.0
  if { [set n [test_magnified $ch1 $ch3 2.0 ]] != 0 } {
    echo "TEST-ERR: matching magnified sets of points fails"
    return 1
  }
  # this version _ought_ to fail
  echo "IGNORE THE FOLLOWING TWO ERROR MESSAGES: THEY ARE EXPECTED!"
  if { [set n [test_magnified $ch1 $ch3 1.0 ]] != 1 } {
    echo "TEST-ERR: matching magnified sets of points fails to fail"
    return 1
  }
   
  # next, try two chains that are slightly different
  modify_chain $ch2 xpos ypos
  if { [set n [ test_almost_identical $ch1 $ch2 ]] != 0 } {
    echo "TEST-ERR: matching almost-identical sets of points fails"
    return 1
  }

  # now, check to see that we can apply a TRANS correctly to items on a CHAIN
  if { [set n [ test_apply_trans  ]] != 0 } {
    echo "TEST-ERR: application of TRANS to a CHAIN fails"
    return 1
  }
  
  # now it's time to try matching the items on two chains
  if { [set n [ test_match_chains ]] != 0 } {
    echo "TEST-ERR: the atMatchChains verb fails"
    return 1
  }


  # get rid of temporary variables
  chainDestroy $ch1 genericDel
  chainDestroy $ch2 genericDel
  chainDestroy $ch3 genericDel

  return 0

}
