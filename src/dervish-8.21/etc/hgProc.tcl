ftclHelpDefine eagphot test_hg \
"USAGE: test_hg

Simple test of histogramming.  Create a region with gaussian noise around
1000 electrons, and put the values in a histogram."

proc test_hg {} {
	echo Create the region
    	set region [regNew 500 500]
	regSetWithDbl $region 1000
	seedSet 111119
	regFluctuateAsSqrt $region -gain=1.0
echo Create and fill the histogram.
	set hg [hgNew]
	hgDefine $hg -min=800 -max=1200 -nbin=100 "-name=Sky Histogram" 
	hgReg $hg $region
	hgPrint $hg
echo Plot the histogram.
	set pgstate [pgstateNew]
	pgstateSet $pgstate 
	pgstateOpen $pgstate
	hgPlot $pgstate $hg
	hgMean $hg
	hgSigma $hg
	pgstateClose $pgstate
	return 0
}

proc test_hg2 {} {
	set af [afNew]
	echo af is $af
	afDefine $af "-name=test2"
	afSet $af 1 4 
	afSet $af 2 4 
	afSet $af 3 4 
	afSet $af 4 4 
	afSet $af 5 4 
	afSet $af 6 4 
	afSet $af 7 4
	afSet $af 1 1
	set hg [hgFromAf $af]
	hgPrint $hg
}
 
proc test_af {} {
	set pgstate [pgstateNew]
	set afx [afNew]
	afDefine $afx "-name=This is the X Name"
	afSet $afx 0 0 -error=0.5
	afSet $afx 1 1 -error=1
	afSet $afx 2 2 -error=2
	afSet $afx 3 3 -error=3
	afSet $afx 4 4 -error=4

	set afy [afNew]
	afDefine $afy "-name=This is the Y Name"
	afSet $afy 0 0 -error=0.5
	afSet $afy 1 8 -error=3
	afSet $afy 2 7 -error=.5
	afSet $afy 3 6 -error=.5
	afSet $afy 4 5 -error=.5

	pgstateOpen $pgstate
	echo pgstate = $pgstate
	echo afx = $afx
	echo afy = $afy
	afPlot $pgstate $afy $afx
#	pgstateClose $pgstate

	return $pgstate
}

proc test_cur { {npt 10} } {
	global ptList pgstate
	echo Make ptList
	set ptList [listNew PT]	
	echo ptlist $ptList
	loop i 0 $npt {
		set object [ptNew]
		set xpos [randGet -min=10 -max=60]
		set ypos [randGet -min=40 -max=90]
		set radius [expr  5*($xpos/$ypos+[randGet]) ]
		ptDefine $object $xpos $ypos $radius
		listAdd $ptList $object
	}
	echo Make and plot afs
	set afRow [afFromList $ptList row]
	set afCol [afFromList $ptList col]
	echo afRow afCol $afRow $afCol
	set pgstate [pgstateNew]
	echo pgstate $pgstate
	pgstateOpen $pgstate
	afPlot $pgstate $afRow $afCol

	return $ptList
}

proc test_tl { {npt 100} } {
	global ptList pgstate
	seedSet 1111119

	echo Make and SAODisplay region
	set region [regNew 100 100]
	regSetWithDbl $region 1000
	regFluctuateAsSqrt $region
	saoDisplay $region
	saoLabel off

	echo Make ptList
	set ptList [listNew PT]	
	echo ptlist $ptList
	loop i 0 $npt {
		set object [ptNew]
		set xpos [randGet -min=10 -max=60]
		set ypos [randGet -min=40 -max=90]
		set radius [expr  5*($xpos/$ypos+[randGet]) ]
		ptDefine $object $xpos $ypos $radius
		listAdd $ptList $object
	}
	echo Make and plot afs
	set afRow [afFromList $ptList row]
	set afCol [afFromList $ptList col]
	set afRadius [afFromList $ptList radius]
	echo afRow afCol $afRow $afCol $afRadius
	set hgRadius [hgFromAf $afRadius -min=0 -max=10 -nbin=10]


	set pgstate [pgstateNew]
	echo pgstate $pgstate
	pgstateOpen $pgstate
#	afPlot $pgstate $afRow $afCol
	hgPlot $pgstate $hgRadius
	return $ptList
}

proc test_avg {} {
	set af [afNew]
	afDefine $af -nValue=10
	loop i 0 10 {
		afSet $af $i 100
	}
	afSet $af 1 99
	afPrint $af
	afSigmaClip $af
}



