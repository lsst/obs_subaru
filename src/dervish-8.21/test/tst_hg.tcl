ftclHelpDefine eagphot test_hg \
"USAGE: test_hg

Simple test of histogramming.  Create a region with gaussian noise around
1000 electrons, and put the values in a histogram."

proc test_hg {} {
echo Create the region
	set region [regNew 500 500]
	regSetWithDbl $region 1000
	seedSet 111119
	regFluctuateAsSqrt $region -gain 1.0
echo Create and fill the histogram.
	set hg [hgNew]
	hgDefine $hg -min 800 -max 1200 -nbin 100 -name "Sky Histogram" 
	hgReg $hg $region
	hgPrint $hg
echo Plot the histogram.
	set pgstate [pgstateNew]
	pgstateSet $pgstate -device /PS
	pgstateOpen $pgstate
	hgPlot $pgstate $hg
	hgMean $hg
	hgSigma $hg
        pgstateClose $pgstate

        regDel $region
        hgDel  $hg
        pgstateDel $pgstate
	return 0
}

 

#
# Test code for hg verbs
#


if {[test_hg] != 0} {
	error "Error in test_hg"
}

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_hg.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_hg"
    return 1
}

return 0
exit 0








