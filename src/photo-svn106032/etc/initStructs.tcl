# Inits structs in the photometric pipeline

proc magInit { handle passBand mag magErr } {
	foreach el "passBand mag magErr" {
	  if {[string compare [set $el] #]} {memberSet $handle $el [set $el]}
	}
	return $handle
}

proc fluxinstInit { handle flux fluxErr radius } {
	foreach el "flux fluxErr radius" {
	  if {[string compare [set $el] #]} {memberSet $handle $el [set $el]}
	}
	return $handle
}

proc frameinfoInit { num airmass mjd } {
   set handle [frameinfoNew]
   handleSet $handle.frameNum $num
   handleSet $handle.airmass $airmass
   handleSet $handle.mjd $mjd
   
   return $handle

}
	

proc extinctionInit { filterlist maglist magerrlist mjd } {
	set ncolors  [llength $maglist]
	set handle [extinctionNew $ncolors]
	handleSet $handle.mjd $mjd

	loop i 0 $ncolors {
		handleSet $handle.k<$i>->passBand<0> \
			[lindex $filterlist $i]
		handleSet $handle.k<$i>->mag \
			[lindex $maglist $i]
		handleSet $handle.k<$i>->magErr\
			[lindex $magerrlist $i]
		}
	return $handle
}

proc star1Init { handle id row rowErr col colErr ra raErr dec decErr skyLevel psfParams psfCounts apCounts quality } {
	foreach el "id row rowErr col colErr ra raErr dec decErr skyLevel quality" {
	  if {[string compare [set $el] #]} {memberSet $handle $el [set $el]}
	}
	foreach el "psfParams psfCounts apCounts" {
	  if {[string compare [set $el] #]} \
	    {memberSet $handle $el [handlePtr [set $el]]}
	}
	return $handle
}

proc starcInit {id ra raErr dec decErr filterlist maglist magerrlist quality} {
	set handle [starcNew [llength $filterlist]]
	foreach el "id ra raErr dec decErr quality" {
	  if {[string compare [set $el] #]} {memberSet $handle $el [set $el]}
	}
        handleSet $handle.ncolors [llength $filterlist]
	starcFilterSet $handle $filterlist
        loop index 0 [llength $filterlist] {
	  if {[string compare [lindex $maglist $index] #]} {
	    handleSet $handle.mag<$index>->mag [lindex $maglist $index]
	    handleSet $handle.mag<$index>->magErr [lindex $magerrlist $index]
	  }
	}
	return $handle
}

proc calib1bytimeInit {mag20list mag20errlist filterlist mjd } {

	set handle [calib1bytimeNew $filterlist]
	handleSet $handle.mjd $mjd

	loop index 0 [llength $filterlist] {
	  handleSet $handle.calib<$index>->filter<0> \
		[lindex $filterlist $index]
	  handleSet $handle.calib<$index>->mag20 \
		[lindex $mag20list $index]
	  handleSet $handle.calib<$index>->mag20Err \
		[lindex $mag20errlist $index]
	}
	return $handle
}

proc calib1bytimeInit2 {mag20list mag20errlist filterlist mjd } {

        echo "This proc assumes that you've entered NULL for any"
        echo "bands with no calib information. You need dummy"
        echo "variables for those bands in the maglists"

	set handle [calib1bytimeNew $filterlist]
	handleSet $handle.mjd $mjd

	loop index 0 [llength $filterlist] {
          set filter [lindex $filterlist $index]
          if {$filter == "NULL"} {
            set null [handleBindNew 0x0 UNKNOWN]
            set old [handleBindNew [exprGet $handle.calib<$index>] CALIB1]
            handleSetFromHandle $handle.calib<$index> &$null
            echo [handleGet $old]
            calib1Del $old
            echo "NULL" $index
          } else {
	    handleSet $handle.calib<$index>->filter<0> $filter
	    handleSet $handle.calib<$index>->mag20 \
		[lindex $mag20list $index]
  	    handleSet $handle.calib<$index>->mag20Err \
		[lindex $mag20errlist $index]
          }
	}
	return $handle
}


proc star1starcmergeInit { handle star1 starc starcMagPos flux20 } {
	foreach el "star1 starc" {
	  if {[string compare [set $el] #]} \
	    {memberSet $handle $el [handlePtr [set $el]]}
	}
	foreach el "starcMagPos flux20" {
	  if {[string compare [set $el] #]} {memberSet $handle $el [set $el]}
	}
	return $handle
}

proc starcInit2 { id ra raErr dec decErr mjd \
	maglist magerrlist filterlist  } {
#  echo "This proc assumes that you've entered NULL for any"
#  echo "bands with no calib information. You need dummy"
#  echo "variables for those bands in the maglists"


  set ncolors  [llength $maglist]
  set handle [starcNew $ncolors]

  handleSet $handle.id $id
  handleSet $handle.ra $ra
  handleSet $handle.raErr $raErr
  handleSet $handle.dec $dec
  handleSet $handle.decErr $decErr
  handleSet $handle.mjd $mjd

  loop index 0 [llength $filterlist] {
    set filter [lindex $filterlist $index]
    if {$filter == "NULL"} {
      set null [handleBindNew 0x0 UNKNOWN]
      set old [handleBindNew [exprGet $handle.mag<$index>] MAG]
      handleSetFromHandle $handle.mag<$index> &$null
#      echo [handleGet $old]
      magDel $old
#      echo "NULL" $index
    } else {
      handleSet $handle.mag<$index>->passBand<0> $filter
      handleSet $handle.mag<$index>->mag \
          [lindex $maglist $index]
      handleSet $handle.mag<$index>->magErr \
    	[lindex $magerrlist $index]
      }
    }
  return $handle
}
	



proc calib1byframeInit {frame filterlist ncolor} {
  set calib1byframe [calib1byframeNew $filterlist]
  handleSet $calib1byframe.frameNum $frame
  loop i 0 $ncolor {
    set cal [calib1New]
    handleSet $cal.filter<0> [lindex $filterlist $i]
    handleSetFromHandle $cal.psf &[dgpsfNew]
    handleSet $cal.psf->sigmax1 1.1
    handleSet $cal.psf->sigmay1 1.1
    handleSet $cal.psf->sigmax2 3.9
    handleSet $cal.psf->sigmay2 3.9
    handleSet $cal.psf->b 0.15
    handleSetFromHandle $calib1byframe.calib<$i> &$cal
  }
  return $calib1byframe
}

proc totransInit {frame filter a b c d e f} {
  set totrans [totransNew]
  handleSet $totrans.frame $frame
  loop i 0 [clength $filter] {
    handleSet $totrans.filter<$i> [string index $filter $i]
  }
  set trans [transInit $a $b $c $d $e $f]
  handleSetFromHandle $totrans.trans &$trans
  handleDel $trans
  return $totrans
}

proc transInit {a b c d e f} {
  set trans [transNew]
  handleSet $trans.a $a
  handleSet $trans.b $b
  handleSet $trans.c $c
  handleSet $trans.d $d
  handleSet $trans.e $e
  handleSet $trans.f $f
  return $trans
}

