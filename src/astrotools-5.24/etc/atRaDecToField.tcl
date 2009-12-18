
#
# Brute force search for matching field
#
#	-DOES- work off the equator
#
proc raDecToField {ra dec run {verbose 1} {rerun ""} } {
	global atNode atIncl ra_field0 ra_nfields 
	global trans

	set rowSize 1361
	set colSize 2048
	set colList "1 2 3 4 5 6"

    set compname [exec hostname]
#note the below changed to match the data model because $SDSS_DATA is not defined
    set sdssData /sdss/data
#    set sdssData [envscan \$SDSS_DATA]
    if {$sdssData == "" } {
		echo "raDecToField: SDSS_DATA environment variable not defined."
		return -2
    }
    set transfile $sdssData/$run/$rerun/astrom/asTrans-[format %06d $run].fit

    if {![file exists $transfile]} {
		error "radectofield: $transfile file not found."
		echo Code is hardwired to work on SDSS cluster.
		echo Must hand edit to find 'asTrans-$run.fit'.
    }


	if {![info exists atNnode($run)] || ![info exists atIncl($run)] ||
	   ![info exists ra_field0($run)] || ![info exists ra_nfields($run)]} {
		set trhdr [hdrReadAsFits [hdrNew] $transfile]
		set atNnode($run) [hdrGetAsDbl $trhdr NODE]
 		set atIncl($run) [hdrGetAsDbl $trhdr INCL]
		set ra_field0($run) [hdrGetAsInt $trhdr FIELD0]
		set ra_nfields($run) [hdrGetAsInt $trhdr NFIELDS]
		hdrDel $trhdr
	}

	set munu [eqToGC $ra $dec -node $atNnode($run)  -inclination $atIncl($run)]
	set mu [keylget munu mu]
	set nu [keylget munu nu]

	set camcol 1
	if {![info exists trans($run,$camcol)] } {
		set hdu [expr ($camcol-1)*5+1]
	  	set trans($run,$camcol) [fits2Schema $transfile TRANS -hdu $hdu]
	}
	set i 0
	set fld [chainElementGetByPos $trans($run,$camcol) $i]
	set rowcol [transInverseApply $fld r $mu 0 $nu 0]
	set row [keylget rowcol row]
	set deltaFields [int $row/$rowSize]
	loop i [expr $deltaFields-3] [expr $deltaFields+3+1] {
		if {$i < 0 } { continue }
		if {$i >  [chainSize $trans($run,$camcol)]  } { continue }
		foreach camcol $colList {
	   		set match($camcol,field) ""
	   		set match($camcol,row) ""
	   		set match($camcol,col) ""
	   		if {![info exists trans($run,$camcol)] } {
				set hdu [expr ($camcol-1)*5+1]
	  			set trans($run,$camcol) [fits2Schema $transfile TRANS -hdu $hdu]
	   		}
			set fld [chainElementGetByPos $trans($run,$camcol) $i]
			set rowcol [transInverseApply $fld r $mu 0 $nu 0]
			set row [keylget rowcol row]
			set col [keylget rowcol col]
			if {$row >= 0 && $row <= $rowSize && \
				$col >= 0 && $col <= $colSize } {
				lappend match($camcol,field) [expr $i+$ra_field0($run)]
				lappend match($camcol,row) $row
				lappend match($camcol,col) $col
				break 
			}
		}

		foreach camcol $colList {
			if {[llength $match($camcol,field)] > 0 } { 
				if {[info exists match(coord)] } {
					error "radectofield: too many matches" 
				}
				set match(coord) "$camcol $match($camcol,field) \
					$match($camcol,row) $match($camcol,col)"
			}
		}
	}

		
	if {[info exists match(coord)] } {
		return $match(coord)
	} else {
		if {$verbose > 0} {
	    	echo "Object not in this run"
		}
		return -1
	}
}


