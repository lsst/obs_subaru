#
# This is the driver programme for contributed tests
#
echo "Running tests of contributed software:"
source template.tcl
foreach f [glob test/*.tcl] {
	echo File: [string range $f [expr 1+[string first "/" $f ]] end]
	if {[catch {source $f} msg]} {
		echo "Failed: $msg"
	}
}
echo All tests completed
