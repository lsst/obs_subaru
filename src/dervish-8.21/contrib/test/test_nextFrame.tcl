#
# Test code for nextFrameProc.  If all goes well, nothing will get printed
# to stdout.
#
proc nextFrameTest {} {
    loop num 1 4 {
	set reg [regNew 100 100]
	hdrInsertLine $reg.hdr 0 [hdrMakeLineWithAscii NUMBER $num] 
	regSetWithDbl $reg [expr $num*20]
	loop i 0 100 {regPixSetWithDbl $reg 0 $i [expr $num+$i]}
	loop i 0 100 {regPixSetWithDbl $reg 99 $i [expr 99+$num-$i]}
	regWriteAsFits $reg testNextFrame-$num.fits
	regDel $reg
    }
#
    set state [nextFrameInit . testNextFrame- 1 3 .fits 10 110 0]
    set region [keylget state frameHandle]
    set state [nextFrameGet $state]
    set i 1
    while { ![nextEOF $state]} {
        set pixvalue [regPixGet $region 105 0]
	set i [incr i]
        if {$pixvalue != [expr $i*20]} {
	    error "Frame pixel value is not what was expected"
	    break
	}
	set state [nextFrameGet $state]
	if {$i > 3} {
	    error "Frame processing has gone beyond end of sequence"
	    break
	}
    }
    loop num 1 4 {
	exec rm testNextFrame-$num.fits
    }
}

#
# Run the test
#
nextFrameTest
#echo Test complete
