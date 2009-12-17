echo Begin tcl_dumpIO.tcl ---------------------------

proc testCleanUp {region chain index} {
	echo ...cleaning
	regDel $region
	loop i 0 $index {
	  set pt [chainElementRemByPos $chain 0]
	  ptDel $pt
	}
	chainDel $chain
}

proc testDumpWrite {} {
	echo CREATE THE DUMP FILE
	exec rm -f test.dump
        global index

	echo ...chain
	set chain [chainNew PT]
	loop i 0 $index {
	  set pt [ptNew]
	  ptDefine $pt $i $i $i
	  chainElementAddByPos $chain $pt
	}
	echo ...region
	set region [regNew 100 100]
	regSetWithDbl $region 123
	
	echo ...dumpOpen
	dumpOpen test.dump w
        dumpValueWrite  11:22:33 STR
	dumpHandleWrite $chain
	dumpHandleWrite $region
        dumpValueWrite 1234567890 LONG
	echo ...dumpClose
	dumpClose
	testCleanUp $region $chain $index
	return 0
}

proc testDumpRead {} {
        global index

	dumpOpen test.dump r
        set sn1 [memSerialNumber]
        set value [dumpHandleRead]
	set chain  [dumpHandleRead]
	set region [dumpHandleRead]
        set l [dumpHandleRead]
        set sn2 [memSerialNumber]

	echo
	echo The next command emits a spurious error message.
	dumpClose
	echo That was the spurious error message.
	echo

	if {[exprGet $value] != "11:22:33"} {
		error "tst_dumpIO.tcl: VALUE dump not read"
		return 1 }

	if {[chainSize $chain] != 10} {
		error "tst_dumpIO.tcl: CHAIN dump not read-sizes do not match"
		return 1}

	if {[regPixGet $region 55 66] != 123} { 
		error "tst_dumpIO.tcl: REGION dump not read" 
		return 1 }

	if {[exprGet $l] != 1234567890} {
	  	error "tst_dumpIO.tcl: LONG dump not read"
		return 1 }

	testCleanUp $region $chain $index
	handleDel $value
	handleDel $l
	memFreeBlocks $sn1 $sn2

	echo tst_dumpIO.tcl:  SUCCESS
	return 0
}

proc testDumpRemove {} {
	exec rm -f test.dump
	return 0
}

set index 10

echo WRITE A DUMP FILE
if {[testDumpWrite] != 0} {
	echo tst_dumpIO.tcl:  ERROR executing testDumpWrite
	return 1
}

echo READ THE DUMP FILE
if {[testDumpRead] != 0} {
        echo tst_dumpIO.tcl:  ERROR executing testDumpRead
        return 1
}

echo REMOVE THE DUMP FILE
if {[testDumpRemove] != 0} {
	echo tst_dumpIO.tcl:  ERROR executing testDumpRemove
	return 1
}

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_dumpIO.tcl
    echo TEST-ERR: $rtn
    error "Error in tst_dumpIO"
    return 1
}

return 0









