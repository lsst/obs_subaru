proc pra {handle} {
	set regInfo [regInfoGet $handle]
	set regNrow [keylget regInfo nrow]
	set regNcol [keylget regInfo ncol]
	set sum 0
	loop icol 0 $regNcol {
	  loop irow 0 $regNrow {
	     set sum [expr {$sum + [regPixGet $handle $irow $icol]} ]
	  }
	}			
	expr { $sum / ($regNrow * $regNcol) }
}
