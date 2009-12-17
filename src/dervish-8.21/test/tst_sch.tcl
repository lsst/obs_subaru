# dervish/test/tst_sch.tcl
# simple test of fits table reading and writing
# Chris S.

proc sch_write {} {
    # Make a dummy list
    set chain [chainNew PT]
    loop i 0 10 {
	set pt [ptNew]
	handleSet $pt.row $i
	handleSet $pt.col [expr $i*$i]
	handleSet $pt.radius [expr $i*$i*$i]
	chainElementAddByPos $chain $pt
	handleDel $pt
    }

    # Make the xsch 
    set xsch [schemaTransNew]
#    schemaTransEntryAdd $xsch ignore junk type int
#    schemaTransEntryAdd $xsch ignore junk prev int
#    schemaTransEntryAdd $xsch ignore junk next int
    schemaTransEntryAdd $xsch name row row float
    schemaTransEntryAdd $xsch name col col float
    schemaTransEntryAdd $xsch name radius radius float
    schemaTransEntryAdd $xsch name id id int

    # Make the tbl
    set tbl [schemaToTbl $chain $xsch -schemaName PT]
    
    # Write it to a file
    fitsWrite $tbl ptList.fits -ascii -pdu MINIMAL

    # Clean up
    set cursor [chainCursorNew $chain]
    while {[set elem [chainWalk $chain $cursor]] != ""}  {
           ptDel $elem
    }
    chainCursorDel $chain $cursor
    chainDel $chain
    tblColDel $tbl
    schemaTransDel $xsch
}

proc sch_read {} {
    
    # Make the xsch 
    set xsch [schemaTransNew]
    schemaTransEntryAdd $xsch name row row float
    schemaTransEntryAdd $xsch name col col float
    schemaTransEntryAdd $xsch name radius radius float

    # Make the tbl
    set tbl [handleNewFromType TBLCOL]

    # Read it in
    fitsRead $tbl ptList.fits -hdu 1 -ascii

    # Make it a chain
    set chain [chainNew PT]
    tblToSchema $tbl $chain $xsch -schemaName PT

    tblColDel $tbl
    schemaTransDel $xsch

    return $chain
}

proc sch_test {} {
    sch_write
    set chain [sch_read]
    if { [chainSize $chain] != 10 } {
	error "chain read back is wrong size"
	return 1
    }
    set obj2 [chainElementGetByPos $chain 9]
    set radius [exprGet $obj2.radius]
    if { $radius != 729 } {
	error "last element on chain is incorrect"
	return 1
    }

    # Clean up
    set cursor [chainCursorNew $chain]
    while {[set elem [chainWalk $chain $cursor]] != ""}  {
           ptDel $elem
    }
    chainCursorDel $chain $cursor
    chainDel $chain    

    return 0
}

if { [sch_test] != 0 } {
    error "Error in sch_test"
}

exec rm ptList.fits

# Test for any memory leaks
set rtn [shMemTest]
if { $rtn != "" } {
    echo TEST-ERR: Memory leaks in tst_sch.tcl
    echo TEST-ERR: $rtn
    error "Error in sch_test"
    return 1
}

