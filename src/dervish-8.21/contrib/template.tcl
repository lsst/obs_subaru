proc template {} {
	echo Hello World
}

proc contrib_add {num1 num2} {
   set ans [expr $num1+$num2]

   return $ans
}

proc contrib_subtract {num1 num2} {	# subtract $num2 from $num1
	contrib_add $num2 [expr -$num1]
}

proc contrib_multiply {num1 num2} {	# only works if $num1 is a +ve integer
	if {[catch { expr $num1%1 } ]} {
		error "contrib_multiply: $num1 is not an integer"
	}
	if {$num1 < 0} {
		set num1 [expr -$num1]
		set num2 [expr -$num2]
	}
	set ans 0
	for {set i 0} {$i < $num1} {set i [expr $i+1]} {
		set ans [contrib_add  $ans $num2]
	}
	return $ans
}
