###############################################################################
#                                                                             #
#  TCL Functions for dervish                                                    #
#                                                                             #
#  This files describes two sets of tcl functions                             #
#      a) functions to simplify access to dump files                          #
#      b) functions to access schema of handle expression                     #
#                                                                             #
# dumpRead <file>       Read an entire dump file, returning a list of handles #
# dumpPrint <list>	Print the contents of the list produced by dumpRead   #
#                                                                             #
# schemaPrint <handle>	Print the schema for a handle                         #
# schemaPrintFromType <type>                                                  #
#                       Print the schema for a type                           #
# schemaMemberPrint <type> <member>                                           #
#                       Print the schema for the member of a type             #
# schemaGetFromType <type>                                                    #
# handleListFromType <type>                                                   #
#                       List out handles that are attached to the type of     #
#                       structure entered by the user.                        #
#                       (ie.  all REGION handles)                             #
# regListNames          List all REGION handles.                              #
# defDirs <arg>                                                               #
#                                                                             #
# Private functions:                                                          #
#                                                                             #
# clfill <high> <low> <ngaps>                                                 #
#                                                                             #
# Also in this file:                                                          #
#      mouseDefine POINT_CL_L, POINT_CL_M and  POINT_CL_R                     #
#      history keep 100                                                       #
#      Create help for:                                                       #
#            regPixGetStatus regPixGetArray regPixGetArrayFsao                #
#            getHistoryFile historySave historyRestore handleBindNew          #
#            handleDup handlePtr handleType handleArrayDel                    #
#            dumpRead dumpPrint schemaPrint schemaPrintFromType               #
#            schemaMemberGet quit defDirs		                      #
#            handleListFromType						      #
###############################################################################

#
# Now dumps and schema
#
# read an entire dump file from <file>, returning a list of handles
#
# dumpRead reads <file> and returns a list; the first element in the
# file name and dump time; successive elements are handles to the
# contents of the file
#
proc dumpRead { file } {
	dumpOpen $file r
	lappend list "$file: Created [dumpDateGet]"
	catch {			# catch error at end of file
		while {[set h [dumpHandleRead]] != ""} {
			lappend list $h
		}
	} err
	if { [regexp {can't get next type} "$err"] == "0"} { 	# a real error
		error "$err"
	}
	dumpClose
	return $list
}
#
# Print the contents of the list returned by dumpRead
#
proc dumpPrint { arr } {
	echo [lindex $arr 0]
	foreach h [lrange $arr 1 [expr [llength $arr]-1]] {
		echo [format "%-3s" $h] [handleType $h]
	}
}
#
# print a type's schema
#
proc schemaPrint {handle} {
	schemaPrintFromType [handleType $handle]
}
proc schemaPrintFromType {type} {
	foreach f [schemaGetFromType $type] { echo $f }
}
proc schemaMemberGet {type elem} {
	set members [schemaGetFromType $type]
	keylget members $elem
}

proc clfill {high low ngaps} {
  global clist
  set range [expr $high-$low]
  set step [expr $range/$ngaps]
  loop i 0 $ngaps {
	lappend clist [expr $low+[expr $i*$step]]
  }
}


###############################################################################
#
# Define initial functions for three of the mouse commands
#
# Print out array of pixels about the pixel value returned from the mouse click
mouseDefine POINT_CL_L "regPixPrintArrayFsao 5 5"
mouseDefine POINT_CL_M "xsectionPlot C"
mouseDefine POINT_CL_R "xsectionPlot R"

###############################################################################
#
# Set TCL history count to 100 events
#
history keep 100

proc defDirs {arg} {
   global defDirs
   set prog [lindex $arg 0]  
   set dir [set defDirs($prog)]
   case $prog in {
       
       reg*AsFits {set file [lindex $arg 2]}
       source     {set file [lindex $arg 1]}
   }

   set file [lappend dir $file]
   set file [join $file /]

   case $prog in {
       reg*AsFits {set arg [lreplace $arg 2 2 $file]}
       source     {set arg [lreplace $arg 1 1 $file]}
   }
   
   uplevel 1 eval $arg
}
#
#  setup help for procedures in dervishStartup.tcl
#
#  

set SEP "\n\t\t\t"

set hh "Get the mean, high and lo pixel values and stdev $SEP\
in a box with the upper left, height and width $SEP\
defined. USAGE: regPixGetStatus row_width $SEP\
col_width region row_center col_center\n"
ftclHelpDefine shRegion regPixGetStatus $hh

set hh "Print an array of pixel values surrounding $SEP\
the input row and column number $SEP\
USAGE: regPixPrintArray rowoff coloff reg row col\n"
ftclHelpDefine shRegion regPixPrintArray $hh

set hh "Return an array of pixel values surrounding $SEP\
the input row and column number $SEP\
USAGE: regPixGetArray rowoff coloff reg row col\n"
ftclHelpDefine shRegion regPixGetArray $hh

set hh "Use mouse in FSAO to get an array of pixel values surrounding $SEP\
the input row and column number $SEP\
USAGE: regPixGetArrayFsao rowoff coloff reg row col fsao\n"
ftclHelpDefine shRegion regPixGetArrayFsao $hh

set hh "Use mouse in FSAO to print an array of pixel values surrounding $SEP\
the input row and column number $SEP\
USAGE: regPixPrintArrayFsao rowoff coloff reg row col fsao\n"
ftclHelpDefine shRegion regPixPrintArrayFsao $hh

set hh "Return the history file name given $SEP\
by the environment. No argument needed.\n"
ftclHelpDefine shHistory getHistoryFile $hh

set hh  "Save all of your history commands to the $SEP\
history file. No args needed.\n"
ftclHelpDefine shHistory historySave $hh

set hh  "Load the history file form a previous session. \
$SEP No args needed.\n"
ftclHelpDefine shHistory historyRestore $hh

set hh "Return a new handle, bound to <addr> and <type> $SEP \
USAGE: handleBindNew addr type\n"
ftclHelpDefine shHandle handleBindNew $hh

set hh "Return a copy of the handle. USAGE: handleDup handle\n"
ftclHelpDefine shHandle handleDup $hh

set hh "Return a handle's pointer. USAGE: handlePtr handle\n"
ftclHelpDefine shHandle handlePtr $hh

set hh "Return a handle's type. USAGE: handleType handle\n"
ftclHelpDefine shHandle handleType $hh

set hh "Read dump file <file> and return a list of $SEP\
handles. USAGE: dumpRead file\n"
ftclHelpDefine shDiskio dumpRead $hh

set hh "Print the contents of the list returned $SEP\
by dumpRead. USAGE: dumpPrint array \n"
ftclHelpDefine shDiskio dumpPrint $hh

set hh "Print a types' schema. USAGE: schemaPrint handle\n"
ftclHelpDefine shSchema schemaPrint $hh

set hh "Print schema by given type. $SEP\
USAGE: schemaPrintFromType type\n"
ftclHelpDefine shSchema schemaPrintFromType $hh

set hh "Get an elem of schema of given type. $SEP \
USAGE: schemaMemberGet type elem\n"
ftclHelpDefine shSchema schemaMemberGet $hh

set hh "Exit dervish. \n"
ftclHelpDefine shUtils quit $hh

set hh "Execute commands for files in default $SEP\
directories defined by set defDirs(cmd_name) dir_name $SEP\
So far only reg*AsFits and source are implemented. $SEP\
USAGE: defDirs \"cmd_string\" "
ftclHelpDefine shUtils defDirs $hh

set hh "Usage: handleListFromType <type>\n$SEP\
List out handles that are attached to the type of $SEP\
structure entered by the user. (ie.all REGION handles).\n"
ftclHelpDefine shHandle handleListFromType $hh

unset SEP
unset hh

#end of help text for the procedures
