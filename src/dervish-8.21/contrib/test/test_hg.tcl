#
# Test code for hg verbs
#
echo Create and plot a histogram from a region with noise around 1000.
if {[test_hg] != 0} {
	error "Error in test_hg"
}

echo Make up and plot a few points.
if {[test_af] != 0} {
	error "Error in test_af"
}