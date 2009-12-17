#
# Initialise a FOO
#
proc fooInit {foo name i l f } {
	foreach el "name i l f" {
		memberSet $foo $el [set $el]
	}
	return $foo
}
