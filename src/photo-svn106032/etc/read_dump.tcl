#
# RHL specials and more
#
# TCL procedures to manipulate FOOs
#
# Initialise a FOO
#
proc fooInit { foo name i l f } {
	foreach el "name i l f" {
		memberSet $foo $el [set $el]
	}
	return $foo
}
proc testDumpWrite { file } {
	#
	# create a set of regions
	#
	set r0 [regNew -type S16 -mask 20 10]
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
verboseSet 0
proc test_dump {} {
	#
	# we haven't read a dump yet, so let's write one...
	#
	set test_file_name "tst_dump.dat"
	testDumpWrite $test_file_name
	#
	# and read it back. The list of handles is in $d1
	#
	set d1 [dumpRead $test_file_name]
	set $test_file_name {}
	dumpPrint $d1
}
#verboseSet 2
#test_dump

proc listNamePrint { list } {
	proc foo {elem} {
		set members [membersGet $elem]
		echo [keylget members name]
	} 
	listApply foo $list
	rename foo {}
}
#
# Words to manipulate malloced C arrays. The first two procs are private
#
# given an array return the <ind>'th element
# The pointer is by <handle>->ptr, and may be thought of as of type void **ptr;
# this word returns a (void *), ptr[i]
#
set sizeof_ptr 4

proc p_handleArrElemGet { hndl ind } {
   global sizeof_ptr
   set hndl2 [handleBindNew [expr [handlePtr $hndl]+$sizeof_ptr*$ind] PTR]
   set val [handleDeref $hndl2]
   handleDel $hndl2
   return $val
}
#
# given a handle and index (as for p_handleArrElemGet), set the pointed to
# address to val (an integer): *(int *)ptr[i] = val
#
proc p_handleArrElemSet { hndl ind val } {
   global sizeof_ptr
   set hndl2 [handleBindNew [expr [handlePtr $hndl]+$sizeof_ptr*$ind] PTR]
   handleDerefAndSet $hndl2 $val
   handleDel $hndl2
   return $val
}
#
# The rest of these handle array words are public. They manipulate a TCL
# type that I call a handleArray, which is a keyed list with elements:
#	handle		The real handle
#	dimen		The dimension
#	type		The type; for an array of FOOs this is FOO*
#
# If you don't want to deal with this type there is a word handleArrAsTCLArr
# that converts one to a regular TCL array of handles
#
# Return a handleArray from a normal TCL handle; the array is 
# the element handle->member and has dimension <dimen>
#
# For example, given a type
#	typedef {
#	   int n;
#	   BAR **bar;
#	} FOO;
#
# if $foo is a handle to a FOO, and you know that bar is an array of length n,
# you could say
#	set bar [handleArrNewFromHandle $foo bar [memberGet $h0 n]]
#
proc handleArrNewFromHandle { handle member dimen } {
   set type [schemaMemberGet [handleType $handle] $member]
   if {[regexp {(.*)\*$} $type foo type] != 1} {
      error "$member isn't a pointer type"
   }
   list \
     "dimen $dimen" \
     "handle [handleBindNew [memberGet $handle $member] PTR]" \
     "type $type"
}
#
# make a handleArr using shMalloc
#
proc handleArrNew { type dimen } {
   global sizeof_ptr

   if {[regexp {(.*)\*$} $type] != 1} {
      error "$type isn't a pointer type"
   }
   list \
     "dimen $dimen" \
     "handle [shMalloc [expr $sizeof_ptr*$dimen]]" \
     "type $type"
}
#
# Return the handle from a handleArr
# e.g.
#	handleArrHandleGet mag
#
proc handleArrHandleGet { arrHandle } {
   upvar $arrHandle hndl
   keylget hndl handle
}
#
# Given a handleArray that hasn't had its members initialised, call a
# proc <elemNew> to set each of them.
#
# For example, after the last example
#	handleArrElemsNew $bar barNew
#
proc handleArrElemsNew { arrHandle elemNew } {
   upvar $arrHandle hndl
   for {set i 0} {$i < [keylget hndl dimen]} {incr i 1} {
      p_handleArrElemSet \
	             [keylget hndl handle] $i [handlePtr [set hh [$elemNew]]]
      handleDel $hh
   }
}
#
# Free all elements of a handleArray. The word elemDel is called for each
# element, then the array is deleted too
# For example,
#	handleArrDel foo "fooDel $handle"
# (but if you try it the rest of the example won't work)
#
proc handleArrDel { arrHandle elemDel } {
   upvar $arrHandle hndl
   for {set i 0} {$i < [keylget hndl dimen]} {incr i 1} {
      set handle [handleArrElemGet hndl $i]
      eval $elemDel
#     p_handleArrElemSet [keylget hndl handle] $i 0
   }
#  handleDel $handle
   shFree [keylget hndl handle]
   unset hndl
}
#
#
# Delete only the pointer array !!THIS COULD ABANDON MALLOCED SPACE!!
#
proc handleArrDelOnlyPtrs { arrHandle } {
   upvar $arrHandle hndl
   shFree [keylget hndl handle]
   unset hndl
}
#
# Return a handle to an element of an handleArray, for example
#	p [handleArrElemGet bar 2]
#
proc handleArrElemGet { arrHandle i } {
   upvar $arrHandle hndl
   if { $i < 0 || $i >= [keylget hndl dimen] } {
      set max [expr [keylget hndl dimen]-1]
      error "Array index $i out of bounds for $arrHandle: range \[0--$max\]"
   }
   set type [keylget hndl type]
   if {[regexp {(.*)\*$} $type foo type] != 1} {
      error "$arrHandle isn't an array of pointers"
   }
   handleBindNew [p_handleArrElemGet [keylget hndl handle] $i] $type
}
#
# Set an element of a handleArray to <val>, for example
#	handleArrElemSet mag 1 0
#
proc handleArrElemSet { arrHandle i val } {
   upvar $arrHandle hndl
   if { $i < 0 || $i >= [keylget hndl dimen] } {
      set max [expr [keylget hndl dimen]-1]
      error "Array index $i out of bounds for $arrHandle: range \[0--$max\]"
   }
   set type [keylget hndl type]
   if {[regexp {(.*)\*$} $type foo type] != 1} {
      error "$handle isn't an array of pointers"
   }
   p_handleArrElemSet [keylget hndl handle] $i $val
   return $val
}
#
# Set an element of a handleArray to a handle, for example
#	handleArrElemSetFromHandle mag 1 [magNew]
#
proc handleArrElemSetFromHandle { arrHandle i handle } {
   upvar $arrHandle hndl
   handleArrElemSet hndl $i [handlePtr $handle]
}
#
#
# Convert a handleArray to a regular TCL array, for example
#	handleArrAsTCLArr bar bar_t
#	p $bar_t(2)
#
proc tclArrSetFromHandleArr { arrHandle name } {
   upvar $arrHandle hndl
   upvar $name arr
   set dimen [keylget hndl dimen]
   for {set i 0} {$i < $dimen} {incr i 1} {
      set arr($i) [handleArrElemGet hndl $i]
   }
}
#
#
# Convert a TCL array to a handle array, given a TCL list of the array
#	elements.  There must be at least one element in the list
proc handleArrNewFromTclArr { tclArr listOfElements } {
  upvar $tclArr tclArrName
  set dimen [llength $listOfElements]
# Check to see if the request makes sense
  if ($dimen==0) {
    return "The listOfElements must have at least one element"
  }
  set type [handleType $tclArrName([lindex $listOfElements 0])]
  loop element 1 $dimen {
    if ([string compare $type \
      [handleType $tclArrName([lindex $listOfElements $element])]]) {
      return "The elements must all be pointers to objects of the same type"
    }
  }
  set newHandleArr [handleArrNew $type* $dimen]
  loop index 0 $dimen {
    handleArrElemSetFromHandle newHandleArr $index \
      $tclArrName([lindex $listOfElements $index])
  }
  return $newHandleArr
}

proc listDel2 { list proc } {
        set members [membersGet $list]
        if { [keylget members first] == 0 && [keylget members last] == 0 } {
                listDelIfEmpty $list
                return
        }
        set nextelem [iterInit $list]
        while { 1 } {
                set members [membersGet $nextelem]
                set type [typeGet [keylget members type]]
                set next [keylget members next]
		set elem [handleDup $nextelem]
                handleBind $nextelem $next $type
                listRem $list $elem
                $proc $elem
                if { $next == 0 } {
                        break;
                }
        }
        handleDel $nextelem

        memberSet $list first 0
        memberSet $list last 0
        listDelIfEmpty $list
}
