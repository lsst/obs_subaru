#
# This is an example of how to write a dump file from TCL, incidently
# building a linked list as part of the dump. It is discussed in the
# file diskio.feature.html
#
#
# Initialise a FOO
#
proc fooInit { foo name i l f } {
	foreach el "name i l f" {
		handleSet $foo.$el [set $el]
	}
	return $foo
}
#
# Write a test dump file
#
proc testDumpWrite { file } {
	#
	# create a set of regions
	#
	set r0 [regNew -type=S16 -mask 20 10]
	regSetWithDbl $r0 10
	regPixSetWithDbl $r0 5 8 100
	set r1 [subRegNew $r0 10 5 2 4]
	set r2 [subRegNew $r0 8  5 1 4]
	#
	# dump various things to a file; first these [sub]regions,
	# then some primitive types, then a FOO and a list of FOOs
	# if FOOs are available
	#
	dumpOpen $file w

	dumpHandleWrite $r0
	dumpHandleWrite $r1
	dumpHandleWrite $r2
	dumpValueWrite 3.14159 FLOAT
	dumpValueWrite "For the Snark was a Boojum, you see" STR
	dumpValueWrite 32767 INT
	catch {					# we may not be FOO capable
		set foo [fooInit [fooNew] "Hello World" 10 1000 3.14159]
		dumpHandleWrite $foo; handleDel $foo

		set list [listNew FOO]
		foreach n {"Yr Wyfdda" "Carnedd Llewellyn" "Pen Yr Oleu Wen"} {
			set foo [fooInit [fooNew] $n 0 1 2.71828]
			listAdd $list $foo
			handleDel $foo
		}
		dumpHandleWrite $list; handleDel $list
	}

	dumpClose
}
#
# write a dump
#
set test_file_name "tst_dump.dat"
testDumpWrite $test_file_name
#
# and read it back. The list of handles is in $d1
#
set d1 [dumpRead $test_file_name]
set $test_file_name {}
dumpPrint $d1
