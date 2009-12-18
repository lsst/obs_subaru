proc testRaDecToField {} {

    if {[tst_radectofield]!=0} {
	error "Trouble in tst_radectofield"
	return 1
    }
    return 0
}

proc tst_radectofield {} {
    set list [raDecToField 23.4921 -0.059167 94 0]
    if {$list > -1} {
	set camcol [keylget list CAMCOL]        
	set field [keylget list FIELD]  
	set rowc [keylget list ROW]
	set colc [keylget list COL]
	if {$camcol != 3 || $field != 325 || abs($rowc-602.38278) > 0.001 || abs($colc-1305.0588757) > 0.01} {
	    #Test failed
	    echo "$camcol $field $rowc $colc"
	    echo "testRaDecToField failed"
	    return 1
	} else {
	    #Test passed, don't really need to tell user test passed.
	    #echo "testRaDecToField passed"
	    return 0
	    
	}
    } else {
	#Test failed, but don't abort, just give a warning
	if {$list == -2} {
	    echo "Warning: trouble in tst_radectofield"
	    echo "test failed because SDSS_DATA not defined, continuing..."
	    return 0
	} elseif {$list == -1} {
	    echo "Warning: Trouble in tst_radectofield"
	    echo "asTrans file may not exist on this machine, continuing..."
	    return 0
	}
    }
}

