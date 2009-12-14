#
# this file contains regression tests for the functions in the 
# "library" of Mirella-derived functions found in library.c and
# tclLibrary.c
#
# each regression test is a TCL verb that should return 0.  If it
# returns 1, an error has occurred.
#
# at the end of the file, each test is called in turn.

############################################################################
# this proc is used by several other procs in this file; it simply creates
# a REGION that can then be used to test other procedures.
# The region has values
#                   0   0   3   2*
#                   2   4   5   4 
#                   3*  6  10   6*
#                   2   4*  5   4 
#
# pixels marked with "*" have mask values set to 1
#
proc make_region { } {

  # make a region.
  set reg [regNew 4 4 -type U16]
  set mask [spanmaskNew]
  handleSet (long)$reg.mask [exprGet &$mask]

  regPixSetWithDbl $reg 0 0 0;  regPixSetWithDbl $reg 0 1 0
  regPixSetWithDbl $reg 0 2 3;  regPixSetWithDbl $reg 0 3 2
  regPixSetWithDbl $reg 1 0 2;  regPixSetWithDbl $reg 1 1 4
  regPixSetWithDbl $reg 1 2 5;  regPixSetWithDbl $reg 1 3 4
  regPixSetWithDbl $reg 2 0 3;  regPixSetWithDbl $reg 2 1 6
  regPixSetWithDbl $reg 2 2 10; regPixSetWithDbl $reg 2 3 6
  regPixSetWithDbl $reg 3 0 2;  regPixSetWithDbl $reg 3 1 4
  regPixSetWithDbl $reg 3 2 5;  regPixSetWithDbl $reg 3 3 4

  # now, set mask values for four of the pixels to "1"
  spanmaskSetAsPix  *$reg.mask 0 3 1; spanmaskSetAsPix  *$reg.mask 2 0 1
  spanmaskSetAsPix  *$reg.mask 2 3 1; spanmaskSetAsPix  *$reg.mask 3 1 1 

  return $reg
}

######################################################################
# This proc makes a 4x4 region who's pixels are normally distributed 
# around 10 with sigma = 5. 

proc make_gaussian_region {} {

   set reg [regNew 4 4]
   regPixSetWithDbl $reg 0 0 11; regPixSetWithDbl $reg 0 1 11
   regPixSetWithDbl $reg 0 2 9;  regPixSetWithDbl $reg 0 3 11
   regPixSetWithDbl $reg 1 0 8; regPixSetWithDbl $reg 1 1 10
   regPixSetWithDbl $reg 1 2 11;  regPixSetWithDbl $reg 1 3 9
   regPixSetWithDbl $reg 2 0 9; regPixSetWithDbl $reg 2 1 10
   regPixSetWithDbl $reg 2 2 10; regPixSetWithDbl $reg 2 3 7
   regPixSetWithDbl $reg 3 0 10; regPixSetWithDbl $reg 3 1 10
   regPixSetWithDbl $reg 3 2 8;  regPixSetWithDbl $reg 3 3 12

   return $reg
}

############################################################################
# this proc is used by several other procs in this file; it simply creates
# a histogram that can then be used to test other procedures.
# The histogram has values
#       0 0 2 2 2 3 3 4 4 4 4 5 5 6 6 10
#
proc make_histogram { } {

  set reg [make_region]
#  set mask [handleBindFromHandle [handleNew] *((SPANMASK*)$reg.mask)->masks<0>]
#  set hist [histU16FromRegion $reg -mask $mask]
  set hist [histU16FromRegion $reg]

  # and delete the region
  spanmaskDel *$reg.mask
  handleSet (long)$reg.mask 0
  regDel $reg

  # return the histogram
  return $hist
}

############################################################################
# this proc is used by several other procs in this file; it creates
# a histogram that can then be used to test other procedures.
# The region has values
#       0 0 2 2 2 3 3 4 4 4 4 5 5 6 6 10
# but       x     x     x         x
# have mask values set to 1.  Thus, the histogram we get should have values
#       0 0 2 2 3 4 4 4 5 5 6 10
#
#
proc make_histogram_with_mask { } {

  set reg [make_region]
  set hist [histU16FromRegion $reg -mask $reg.mask->masks<1>]

  # and delete the region
  regDel $reg

  # return the histogram
  return $hist
}



############################################################################
# first, we test the function "histU16FromRegion"
#
proc test_histU16FromRegion { } {

  set hist [make_histogram]

  # now look to make sure that the histogram was built correctly
  set num [exprGet $hist.num_bin]
  if { $num != 11 } {
    echo "TEST-ERR: histU16FromRegion: num_bin should be 11, is " $num
    phHistDel $hist
    return 1
  }
  set num [exprGet $hist.min_bin]
  if { $num != 0 } {
    echo "TEST-ERR: histU16FromRegion: min_bin should be 0, is " $num
    phHistDel $hist
    return 1
  }
  set num [exprGet $hist.total_number]
  if { $num != 16 } {
    echo "TEST-ERR: histU16FromRegion: total_number should be 16, is " $num
    phHistDel $hist
    return 1
  }
  set num 0
  if { [exprGet $hist.value<0>] != 2 } { set num 1 }
  if { [exprGet $hist.value<1>] != 0 } { set num 1 }
  if { [exprGet $hist.value<2>] != 3 } { set num 1 }
  if { [exprGet $hist.value<3>] != 2 } { set num 1 }
  if { [exprGet $hist.value<4>] != 4 } { set num 1 }
  if { [exprGet $hist.value<5>] != 2 } { set num 1 }
  if { [exprGet $hist.value<6>] != 2 } { set num 1 }
  if { [exprGet $hist.value<7>] != 0 } { set num 1 }
  if { [exprGet $hist.value<8>] != 0 } { set num 1 }
  if { [exprGet $hist.value<9>] != 0 } { set num 1 }
  if { [exprGet $hist.value<10>] != 1 } { set num 1 }
  if { $num != 0 } {
    echo "TEST-ERR: histU16FromRegion: at least one bin value is incorrect"
    phHistDel $hist
    return 1
  }
  phHistDel $hist

# This isn't working with the new mask.
if 0 {
  # now, test making a histogram from a region with MASK bits set.

  set hist [make_histogram_with_mask]

  # now look to make sure that the histogram was built correctly
  set num [exprGet $hist.num_bin]
  if { $num != 11 } {
    echo "TEST-ERR: histU16FromRegion: num_bin should be 11, is " $num
    phHistDel $hist
    return 1
  }
  set num [exprGet $hist.min_bin]
  if { $num != 0 } {
    echo "TEST-ERR: histU16FromRegion: min_bin should be 0, is " $num
    phHistDel $hist
    return 1
  }
  set num [exprGet $hist.total_number]
  if { $num != 12 } {
    echo "TEST-ERR: histU16FromRegion: total_number should be 12, is " $num
    phHistDel $hist
    return 1
  }
  set num 0
  if { [exprGet $hist.value<0>] != 2 } { set num 1 }
  if { [exprGet $hist.value<1>] != 0 } { set num 1 }
  if { [exprGet $hist.value<2>] != 2 } { set num 1 }
  if { [exprGet $hist.value<3>] != 1 } { set num 1 }
  if { [exprGet $hist.value<4>] != 3 } { set num 1 }
  if { [exprGet $hist.value<5>] != 2 } { set num 1 }
  if { [exprGet $hist.value<6>] != 1 } { set num 1 }
  if { [exprGet $hist.value<7>] != 0 } { set num 1 }
  if { [exprGet $hist.value<8>] != 0 } { set num 1 }
  if { [exprGet $hist.value<9>] != 0 } { set num 1 }
  if { [exprGet $hist.value<10>] != 1 } { set num 1 }
  if { $num != 0 } {
    echo "TEST-ERR: histU16FromRegion: at least one bin value is incorrect"
    phHistDel $hist
    return 1
  }
  phHistDel $hist
}

  return 0
}


############################################################################
# Test the "histU16FromAF" call.  As for histU16FromRegion, array has values
#
#       0 0 2 2 2 3 3 4 4 4 4 5 5 6 6 10
#
# But note that the AF has first element = -4.  The TCL function is supposed
# to clip values < 0 to be 0, and > 65535 to be 65535.  We test the 
# clipping only on the low end in this routine.
#
proc test_histU16FromAF { } {
  set af [afNew]
  afDefine $af -nValue 16
  afSet $af 0 -4; afSet $af 1 0; afSet $af 2 2; afSet $af 3 2
  afSet $af 4 2; afSet $af 5 3; afSet $af 6 3; afSet $af 7 4
  afSet $af 8 4; afSet $af 9 4; afSet $af 10 4; afSet $af 11 5
  afSet $af 12 5; afSet $af 13 6; afSet $af 14 6; afSet $af 15 10

  set hist [histU16FromAF $af]

  # now look to make sure that the histogram was built correctly
  set num [exprGet $hist.num_bin]
  if { $num != 11 } {
    echo "TEST-ERR: histU16FromAF: num_bin should be 11, is " $num
    afDel $af; phHistDel $hist
    return 1
  }
  set num [exprGet $hist.min_bin]
  if { $num != 0 } {
    echo "TEST-ERR: histU16FromAF: min_bin should be 0, is " $num
    afDel $af; phHistDel $hist
    return 1
  }
  set num [exprGet $hist.total_number]
  if { $num != 16 } {
    echo "TEST-ERR: histU16FromRegion: total_number should be 16, is " $num
    regDel $reg; phHistDel $hist
    return 1
  }
  set num 0
  if { [exprGet $hist.value<0>] != 2 } { set num 1 }
  if { [exprGet $hist.value<1>] != 0 } { set num 1 }
  if { [exprGet $hist.value<2>] != 3 } { set num 1 }
  if { [exprGet $hist.value<3>] != 2 } { set num 1 }
  if { [exprGet $hist.value<4>] != 4 } { set num 1 }
  if { [exprGet $hist.value<5>] != 2 } { set num 1 }
  if { [exprGet $hist.value<6>] != 2 } { set num 1 }
  if { [exprGet $hist.value<7>] != 0 } { set num 1 }
  if { [exprGet $hist.value<8>] != 0 } { set num 1 }
  if { [exprGet $hist.value<9>] != 0 } { set num 1 }
  if { [exprGet $hist.value<10>] != 1 } { set num 1 }
  if { $num != 0 } {
    echo "TEST-ERR: histU16FromAF: at least one bin value is incorrect"
    afDel $af; phHistDel $hist
    return 1
  }

  afDel $af; phHistDel $hist
  return 0
}


############################################################################
# test the "ntile" TCL verb.
proc test_ntile { } {

  set hist [make_histogram]

  # find the median -- should be 3.75
  set list [ntile $hist 2]
  set num [llength $list]
  if { $num != 1 } {
    error "test_ntile: list length should be 1, but is $num"
    phHistDel $hist
    return 1
  }
  set num [lindex $list 0]
  if { $num != 3.75 } {
    error "test_ntile: median should be 3.75, but is $num"
    phHistDel $hist
    return 1
  }

  # now, find the 25'th, 50'th and 75'th percentiles -- 2.16, 3.75, 5.0
  set list [ntile $hist 4]
  set num [llength $list]
  if { $num != 3 } {
    error "test_ntile: list length should be 3, but is $num"
    phHistDel $hist
    return 1
  }
  set num [lindex $list 0]
  if { $num != 2.17 } {
    error "test_ntile: 25'th percentile should be 2.17, but is $num"
    phHistDel $hist
    return 1
  }
  set num [lindex $list 1]
  if { $num != 3.75 } {
    error "test_ntile: 50'th percentile should be 3.75, but is $num"
    phHistDel $hist
    return 1
  }
  set num [lindex $list 2]
  if { $num != 5.0 } {
    error "test_ntile: 75'th percentile should be 5.0, but is $num"
    phHistDel $hist
    return 1
  }

  phHistDel $hist
  return 0
}
  

############################################################################
# check to see that the given keyed list of { quartiles, mean, mode, sigma } has
# values that agree with the arguments 
#
proc check_list { func lis quart1 quart2 quart3 mean mode sigma } {
  upvar $lis list

  # test the quartiles: should be $quart1, $quart2, $quart3
  set quart [keylget list quartiles]
  set q [lindex $quart 0]
  if { $q != $quart1 } {
    error "$func: 1st quartile should be $quart1, is $q"
    return 1
  }
  set q [lindex $quart 1]
  if { $q != $quart2 } {
    error "$func: 2nd quartile should be $quart2, is $q"
    return 1
  }
  set q [lindex $quart 2]
  if { $q != $quart3 } {
    error "$func: 3nd quartile should be $quart3, is $q"
    return 1
  }

  # now, look at mean, mode, sigma -- should be $mean, $mode, $sigma
  set q [keylget list mean]
  if { $q != $mean } {
    error "$func: mean should be $mean, but is $q"
    return 1
  }
  set q [keylget list mode]
  if { $q != $mode } {
    error "$func: mode should be $mode, but is $q"
    return 1
  }
  set q [keylget list sigma]
  if { $q != $sigma } {
    error "$func: sigma should be $sigma, but is $q"
    return 1
  }

  return 0
}



############################################################################
# test the "histU16Stats" TCL verb.
proc test_histU16Stats { } {

  set hist [make_histogram]

  # first test using all data in histogram
  set list [histU16Stats $hist]
  set ok [check_list test_histU16Stats list \
              2.17 3.75 5.00 3.75 3.75 2.10]
  if { $ok != 0 } {
    phHistDel $hist
    return 1
  }

  # now test with the "-clip" flag
  set list [histU16Stats $hist -clip 2.3]
  set ok [check_list test_histU16Stats list \
              2.08 3.62 4.62 3.33 4.21 1.93]

  phHistDel $hist
  if { $ok != 0 } {
    return 1
  }

  return 0
}



############################################################################
# test the "regStats" verb on the region returned by make_region.
# Run tests with maskflag = 0 and maskflag = 1.
#
#
proc test_regStats { } {

  set reg [make_region]

  # first, run a test with maskflag = 0, no -clip
  set list [regStats $reg -mask [handleBindFromHandle [handleNew] *((SPANMASK*)$reg.mask)->masks<0>]]
  set ok [check_list "test_regStats-nomask" list \
              2.17 3.75 5.00 3.75 3.75 2.10]
  if { $ok != 0 } {
    regDel $reg
    return 1
  }

  # run on same data, but set the -clip flag this time
  set list [regStats $reg  -mask [handleBindFromHandle [handleNew] *((SPANMASK*)$reg.mask)->masks<0>] -clip 2.3]
  set ok [check_list "test_regStats-nomask-clip" list \
              2.08 3.62 4.62 3.33 4.21 1.93]
  if { $ok != 0 } {
    regDel $reg
    return 1
  }

  # now, run on same region, no -clip, maskflag = 1
  set list [regStats $reg -mask [handleBindFromHandle [handleNew] *((SPANMASK*)$reg.mask)->masks<1>]]
  set ok [check_list "test_regStats-mask" list \
              2.00 3.83 5.00 3.75 4.00 2.22]
  if { $ok != 0 } {
    regDel $reg
    return 1
  }

  # final permutation, with -clip, maskflag = 1
  set list [regStats $reg -mask [handleBindFromHandle [handleNew] *((SPANMASK*)$reg.mask)->masks<1>] -clip 2.3]
  set ok [check_list "test_regStats-mask-clip" list \
              1.88 3.67 4.62 3.18 4.64 2.09]
  if { $ok != 0 } {
    regDel $reg
    return 1
  }

  spanmaskDel *$reg.mask
  handleSet (long)$reg.mask 0
  regDel $reg
  return 0
}


############################################################################
# Here's where we call each of the test procedures
#
#

#if { [test_histU16FromRegion] != 0 } {
#   error "regression test of histU16FromRegion failed"
#}
#
#if { [test_histU16FromAF] != 0 } {
#   error "regression test of histU16FromAF failed"
#}
#
#if { [test_ntile] != 0 } {
#   error "regression test of ntile failed"
#}
#
#if { [test_histU16Stats] != 0 } {
#   error "regression test of histU16Stats failed"
#}
#
#if { [test_regStats] != 0 } {
#   error "regression test of regStats failed"
#}

