proc testWcs { } {
    global cfn bfn rv

    set run 125
    set rerun 7
    set camcol 1
    set field 274
    set filter r
    set dataRoot [envscan \$ASTROTOOLS_DIR/test/wcs]

    set cfn $dataRoot/$run/$rerun/corr/$camcol/fpC-[format %06d $run]-$filter$camcol-[format %04d $field]

    set bfn $dataRoot/$run/$rerun/objcs/$camcol/fpBIN-[format %06d $run]-$filter$camcol-[format %04d $field]

    if {0} {
    echo Reconstitute the input corrected frame
    exec gunzip -q $cfn.header
    set hdr [hdrReadAsFits [hdrNew] $cfn.header]
    set reg [regNew 1489 2048 -type U16]
    regSetWithDbl $reg 100
    regFluctuateAsSqrt $reg
    hdrCopy $hdr $reg.hdr
    regWriteAsFits $reg $cfn.fit
    exec sdsscompress -f $cfn.fit


    echo Reconstitute the input BIN frame
    exec gunzip -q $bfn.header
    set hdr [hdrReadAsFits [hdrNew] $bfn.header]
    set reg [regNew 372 512 -type U16]
    regSetWithDbl $reg 100
    regFluctuateAsSqrt $reg
    hdrCopy $hdr $reg.hdr
    regWriteAsFits $reg $bfn.fit
    exec sdsscompress -f $bfn.fit
    }
    echo Delete the bin frame $bfn
    exec rm -f $bfn.fit
    exec rm -f $bfn.fit.gz

    echo Make the BIN image with WCS
    atFixfpBHdr $run $rerun $camcol $field $filter $dataRoot $dataRoot

    echo Unzip the BIN frame
    exec gunzip $bfn.fit.gz

    echo Read in the bin header
    set bh [hdrReadAsFits [hdrNew] $bfn.fit]




    echo Delete the corrected frame $cfn
    exec rm -f $cfn.fit
    exec rm -f $cfn.fit.gz

    echo Make the corrected frame with WCS
    atFixfpCHdr $run $rerun $camcol $field $filter $dataRoot $dataRoot

    echo Unzip the corrected frame
    exec gunzip $cfn.fit.gz

    echo Read in the corrected header
    set ch [hdrReadAsFits [hdrNew] $cfn.fit]

    echo Ra,Dec, of lower left hand corner from SDSS Trans structure
    set r 0
    set c 0
    echo r=$r
    echo c=$c
    set rv(1) [sdssTransTest $run $rerun $camcol $field $filter $dataRoot $r $c]
    echo $rv(1)

    echo Ra,Dec, of upper right corner from SDSS Trans structure
    set r 1488
    set c 2048
    echo r=$r
    echo c=$c
    set rv(2) [sdssTransTest $run $rerun $camcol $field $filter $dataRoot $r $c]
    echo $rv(2)

    echo Ra,Dec of lower left corner from WCS header information corrected
    set r 0.5
    set c 0.5
    echo r=$r
    echo c=$c
    set rv(3) [wcsTransTest $ch $r $c]
    echo $rv(3)

    echo Ra,Dec of upper right corner from WCS header information corrected
    set r 1488.5
    set c 2048.5
    echo r=$r
    echo c=$c
    set rv(4) [wcsTransTest $ch $r $c]
    echo $rv(4)

    echo Ra,Dec of lower left corner from WCS header information bin
    set r 0.5
    set c 0.5
    echo r=$r
    echo c=$c
    set rv(5) [wcsTransTest $bh $r $c]
    echo $rv(5)

    echo Ra,Dec of upper right corner from WCS header information bin
    set r 372.5
    set c 512.5
    echo r=$r
    echo c=$c
    set rv(6) [wcsTransTest $bh $r $c]
    echo $rv(6)

    set nbad [testWcsSeparations]

    exec sdsscompress -f $bfn.fit
    exec sdsscompress -f $cfn.fit
    return $nbad
}

proc testWcsSeparations { } {
    global rv
    set nbad 0
    foreach pair "{1 3} {1 5} {2 4} {2 6}" {
	set i [lindex $pair 0]
	set j [lindex $pair 1]
	set rai  [keylget rv($i) raDeg]
	set deci [keylget rv($i) decDeg]
	set raj  [keylget rv($j) raDeg]
	set decj [keylget rv($j) decDeg]
	set sep [expr 3600*[sepGet $rai $deci $raj $decj]]
	echo i=$i j=$j sep=$sep arcsec
	if {$sep > 0.01} {incr nbad}
    }
    return $nbad
}

proc wcsTransTest {hdr row col} {
    global at_deg2Rad
    foreach par "CRPIX1 CRPIX2 CRVAL1 CRVAL2 CD1_1 CD1_2 CD2_1 CD2_2" {
	set $par [hdrGetAsDbl $hdr $par]
    }
    set cosdec [cos $at_deg2Rad*$CRVAL1]

    set drow [expr $row-$CRPIX2]
    set dcol [expr $col-$CRPIX1]

    set ra  [expr $CRVAL2 + ($dcol*$CD2_1 + $drow*$CD2_2)/$cosdec]
    set dec [expr $CRVAL1 +  $dcol*$CD1_1 + $drow*$CD1_2]

    keylset retval raDeg $ra
    keylset retval decDeg $dec
    keylset retval ra [degToHMS $ra]
    keylset retval dec [degToDMS $dec]
    return $retval
}

proc sdssTransTest {run reRun camcol field filter inParentDir row col} {
    # find asTrans file for the corrected frame; does it exist?...
    set camrow(r) 1
    set camrow(i) 2
    set camrow(u) 3
    set camrow(z) 4
    set camrow(g) 5

    set astromDir      $inParentDir/$run/$reRun/astrom
    if { ![file isdirectory $astromDir] } {
	echo "Error:  $astromDir is NOT a directory!"
	echo "        Exiting!!!"
	return 1
    }
    set astromName     asTrans-[format %06d $run].fit
    set fullAstromName $astromDir/$astromName
    if { ![file exists $fullAstromName] } {
	echo "Error:  $fullAstromName does NOT exist!"
	echo "        Exiting!!!"
	return 1
    }

    set ccd $camrow($filter)$camcol
    set stuff      [atReadTrans $fullAstromName $ccd]
    set transChain [keylget stuff chain]
    set node       [keylget stuff node]
    set incl       [keylget stuff incl]
    set smallChain [chainSearch $transChain "{ id == $field }"]
    if { [chainSize $smallChain] != 1 } {
	echo "Error:  Problem extracting TRANS information!"
	echo "        Exiting!!!"
	return 1
    }
    set trans      [chainElementGetByPos $smallChain HEAD]

    set munu [rc2munu $trans $row $col]
    set muref [keylget munu mu]
    set nuref [keylget munu nu]
    set radec [GCToEq $muref $nuref -node $node -inclination $incl]
    keylset retval raDeg [keylget radec ra]
    keylset retval decDeg [keylget radec dec]
    set ra [degToHMS [keylget radec ra]]
    set dec [degToDMS [keylget radec dec]]
    keylset retval ra $ra
    keylset retval dec $dec
    return $retval
}
