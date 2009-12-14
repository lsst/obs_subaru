#
# Utilities to interface SX output ascii tables with photo
#
proc sx2tbl {args} {
   global env OBJECT1 OBJECT2 table

   set has_pos 0;			# specify objects by (rowc, colc)
   set has_id 0;			# specify objects by ID
   set writeAI 1;			# write atlas images?
   set write_run_id 1;			# write run/column/field to table?
   set write_test_info 0;		# write test_info file?

   set children 0

   set opts [list \
		 [list [info level 0] \
 "Write a private fpObjc/tsObj file based on the results of an SX query

The objc_flags2 USR1 bit will be set for objects satisfying the query;
this is useful when the -family flag is specified.
"] \
		 [list <sxFile> STRING "" sxFile \
		      "Name of .tbl file returned by SX (~/... is allowed)"] \
		 [list <name> STRING "" file_str \
		 "Identifying string for output files"] \
		 [list -id CONSTANT 1 has_id \
		      "The SX file contains object IDs (default)"] \
		 [list -pos CONSTANT 1 has_pos \
		      "The SX file contains object rowc, colc"] \
		 [list -noAI CONSTANT 0 writeAI \
		      "don't write atlas image files"] \
		 [list -test_info CONSTANT 1 write_test_info \
		      "write TEST_INFO files"] \
		 [list -children CONSTANT 1 children \
		      "Write objcIo's entire family to the table"] \
		 [list -family CONSTANT 1 children \
		      "Write objcIo's entire family to the table"] \
		 [list -noRun_id CONSTANT 0 write_run_id \
		      "Don't Write run/column/field to table"] \
		 [list -n1 INTEGER 0 n1 "first row to write to file"]\
		 [list -n2 INTEGER 0 n2 "last row to write to file"]\
		]
   if {[shTclParseArg $args $opts open_mytables] == 0} {
      return 
   }

   if {$has_id + $has_pos == 0} {
      set has_id 1
   }
   if $children {
      set children "-children"
   } else {
      set children ""
   }

   set fd [open $sxFile "r"]

   set field_id ""
   set i 0
   while {[gets $fd line] >= 0} {
      if [regexp {^[ 	]*\#} $line] {
	 continue;
      }

      incr i
      if {$n1 > 0 && $i < $n1} {
	 continue
      }
      if {$n2 > 0 && $i > $n2} {
	 break;
      }
      
      set run [lindex $line 0]
      set camCol [lindex $line 1]
      set field [lindex $line 2]

      if {$field_id != "$run:$camCol:$field"} {
	 if [info exists table] {
	    objfileClose table
	 }

	 set_run -ts $run $camCol
	 if [catch {
	    set table [openit $field]
	 } msg] {
	    echo Couldn't open $run-$camCol-$field ($msg)
	    continue;
	 }

	 set field_id $run:$camCol:$field
      }

      regsub {~} $file_str $env(HOME) file_str

      if ![info exists fds] {
	 set fds [open_mytables -tsObj table $file_str]
      }
      if {$has_id && !$has_pos} {
	 set id [lindex $line 3]
      } else {
	 if $has_id {
	    set rowc [lindex $line 4]
	    set colc [lindex $line 5]
	 } else {
	    set rowc [lindex $line 3]
	    set colc [lindex $line 4]
	 }

	 if [catch {
	    set id [find_objc table $rowc $colc sel_not_bright]
	 } msg] {
	    echo "Failed to find object at ($rowc, $colc): $msg"
	    continue
	 }
      }
	 
      puts -nonewline "$run $camCol $field $id\r"
      echo "";				# XXX

      if [catch {
	 set objcIo [read_objc table $id]
      } msg] {
	 echo Couldn't read $run-$camCol-$field:$id ($msg)
	 continue;
      }

      if {$children != ""} {
	 set objcIo [read_parent table $objcIo]
	 if {[exprGet $objcIo.objc_flags] & $OBJECT1(BRIGHT)} {
	    set objcIo [read_objc table next]
	 }
      }

      if [catch {
	 eval write_mytables fds $objcIo table $children -id $id
      } msg] {
	 echo Couldn't write $run-$camCol-$field:$id ($msg)
	 continue;
      }
   }
   echo ""

   catch {objfileClose table}
   
   close $fd
   
   if [info exists fds] {
      close_mytables fds
   }
}

#
# Select non-BRIGHT objects
#
proc sel_not_bright {o {band 2}} {
   global OBJECT1

   if {[exprGet $o.objc_flags] & $OBJECT1(BRIGHT)} {
      return 0
   }
   return 1
}

#
# Select objects that the SX returned, as opposed to their families
#
proc sel_sx {o {band 2}} {
   global OBJECT2

   if {[exprGet $o.objc_flags2] & $OBJECT2(USR1)} {
      return 1
   }
   return 0
}
