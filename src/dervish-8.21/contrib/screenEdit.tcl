#Display columns of information on a vt100 screen.
#Must have loaded vt100.tcl as well
#Scrolling region is diddled
#At present, assume 24 lines in the display.  Gotta find out how to get
#info on the display
#Now add editing!

# lineShow will display one line of one field on the screen.
#	param = text
#	x = starting screen row (1 indexed)
#	col = starting column within text (0 indexed)
#	line = index number within array (0 indexed)
#	start = starting screen column (nominal) (1 indexed)
#	startcol = left shift of starting screen column for use with ctrl-a
#	width = max. field with in characters
#	flag = 1 if reverse
#	edit = 1 if editable.
#
proc lineShow {param x col line start startcol width flag edit} {
   set COL 80
#startcol is an adjust factor - apply here
   set start [expr $start-$startcol+$col]
   set width [expr $width-$col]
   if {$start < 1} then return
#Adjust width so we don't go off the end of the screen
   set wid [min $width [expr $COL-$start+1]]
   if {$wid <= 0} then return
   if {$flag != ""} then vtBold
   if {$edit == 0 || $line == 0} then {
	set data [string range $param $col [expr $col+$wid-1]]
   } else {
	set data [string range $param $col [expr $col+$wid-2]]`
	}
   set data [format "%-${wid}s" $data]
   vtMovexy $x $start
   puts stdout $data nonewline
   vtNormal
   return
   }


#
#Display the portion of the page that is visible in the window
#
proc pageShow {dstart startcol} {
#Inherit all variables from up 1 level
   upvar 1 npage npage
   upvar 1 indxmax indxmax
   upvar 1 msg msg
   upvar 1 nfield nfield
   upvar 1 start start
   upvar 1 width width
   upvar 1 edit edit
   set COL 80
   loop i 0 $nfield {
	upvar 1 param${i} param${i}
	}
   set jmax [min [expr $dstart+$npage] $indxmax]
   loop j $dstart $jmax {
	set flag ""
	if {$j == 0} then {set flag 1} else {
	   if {[info exists msg($j)]} then {
		if {$msg($j) !=  ""} then {set flag 1}
		}
	   }
	set x [expr $j-$dstart+1]
	vtMovexy $x 0
	vtClearToLineEnd
	loop i 0 $nfield {
	   set param [set param${i}($j)]
	   lineShow $param $x 0 $j $start($i) $startcol $width($i) \
		$flag $edit($i)
	   }
	vtNormal
	}
#Screen line number following last line in array
   set linemax [expr $indxmax-$dstart+1]
#If this line is on screen, fill with ----
   if {$linemax <= $npage} {
	loop i 0 $nfield {
	   vtMovexy [expr $linemax] $start($i)
	   loop j 0 [min $width($i) [expr $COL-$start($i)+1]] {
		puts stdout - nonewline
		}
	   }
	}
#Clear out bottom.
   loop j [expr $jmax-$dstart+1] $npage {
	vtMovexy [expr $j+1] 1
	vtClearToLineEnd
	}
   flush stdout
   }

proc screenEdit {inmsg beginline \
   {inparam0 ""} {ncol0 ""} {inparam1 ""} {ncol1 ""} \
   {inparam2 ""} {ncol2 ""} {inparam3 ""} {ncol3 ""} \
   {inparam4 ""} {ncol4 ""} {inparam5 ""} {ncol5 ""} \
   {inparam6 ""} {ncol6 ""} {inparam7 ""} {ncol7 ""} \
   {inparam8 ""} {ncol8 ""} {inparam9 ""} {ncol9 ""}}\
   {

#npage is number of lines to be displayed in the main table.
   set npage 20
   set nfield 0
#Starting screen column of the 1st input field
   set start(0) 1
#Are all fields editable?
   set editAll 1	
#How many fields?
   loop i 0 10 {
	if {[set inparam$i] == ""} then break
	upvar 1 [set inparam$i] param${i}_in
	set width($i) [expr int(abs([set ncol$i]))]
	set size($i) [array size param${i}_in]
#Is the field editable?
	if {[set ncol$i] > 0} then {set edit($i) 1} else {set edit($i) 0}
	if {$edit($i) == 0} then {set editAll 0} 
#Starting column of next field
	incr nfield
	set start($nfield) [expr $start($i)+$width($i)]
#Store all input in temporary internal arrays
	loop j 0 $size($i) {
	   set param${i}($j) [set param${i}_in($j)]
	   }
	}
#Check for consistency
   set indxmax $size(0)
   if {$nfield > 1} then {
	loop i 1 $nfield {
	   if {$size($i) != $size(0)} then {
		error "Incompatible input array sizes"
		}
	   }
	}
#Clear screen, update, and then save cursor.  This forces cursor to the bottom
#of the screen before updating.
   vtSaveCursor
   screen
#Input messages and flags
   upvar 1 $inmsg msg_in
#Initialize the msg array
   loop i 0 $indxmax {
	set msg($i) ""
	}
#Copy msg_in to internal storage
   if {[info exists msg_in(0)]} then {
	foreach index [array names msg_in] {
	   set msg($index) $msg_in($index)
	   }
   } else {
	set msg(0) $inmsg
	}

#Top line - format with ---.
   loop i 0 $nfield {
	while {[string length [set param${i}(0)]] < $width($i)} {
	   append param${i}(0) -
	   }
	}
#Insert values.  indxmax is the array size.  Indices in array go from 0 to 
# indxmax-1
   set line [min [max $beginline 1] [expr $indxmax-1]]
#Starting array index
   set dstart [expr ($line/$npage)*$npage]
#First page display
   pageShow $dstart 0
#Messages.
   set message $msg(0)
   vtMovexy [expr $npage+1] 1
   puts stdout $message nonewline
   flush stdout
#Big input loop
#Define control characters and other
   set CTRL_A [format %c 1]
   set CTRL_B [format %c 2]
   set CTRL_C [format %c 3]
   set CTRL_D [format %c 4]
   set CTRL_E [format %c 5]
   set CTRL_F [format %c 6]
   set CTRL_H [format %c 8]
   set CTRL_I [format %c 9]
   set CTRL_J [format %c 10]
   set CTRL_K [format %c 11]
   set CTRL_L [format %c 12]
   set CTRL_N [format %c 14]
   set CTRL_O [format %c 15]
   set CTRL_P [format %c 16]
   set CTRL_R [format %c 18]
   set CTRL_U [format %c 21]
   set CTRL_W [format %c 23]
   set CTRL_X [format %c 24]
   set CTRL_Z [format %c 26]
   set ESC [format %c 27]
#First, set tty to be in raw mode.
   exec stty raw -echo
#Cursor position
   set field 0
#Find first editable field
   loop i 0 $nfield {
	if {[set ncol$i] <= 0} then continue
	set field $i
	break
	}
#Cursor column within the field
   set col 0
   set startcol 0
   set startcolOld 0
#
############################### Main Loop ###################################
#
   while {1} {
	set maxline [expr $indxmax-1]
#	if {$maxline == 0} then break
#Adjust line if necessary
	set line [max 1 [min $indxmax $line]]
#Check to see if we page up or down
	set dstartNew [expr ($line/$npage)*$npage]
	if {$dstartNew != $dstart || $startcolOld != $startcol} then {
		pageShow $dstartNew $startcol
		}
	set dstart $dstartNew
	set startcolOld $startcol
#Move cursor - adjust col if necessary
	if {$line <= $maxline} then {
	   set col [max 0 [min [expr $width($field)-1] $col \
		 [string length [set param${field}($line)]]]]
	   }
	set x [expr $line+1-$dstart]
	set y [expr $start($field)+$col-$startcol]
	vtMovexy $x $y
#Ok, read next character
	set char [read stdin 1]
#Decide what to do.
	if {[info exists msg($line)]} then {
		set flag $msg($line)} else {set flag ""}
#Printable character
	if {[ctype print $char]} then {
	   if {$line > $maxline} then continue
	   if {$col >= [expr $width($field)-1]} then continue
	   if {$edit($field) != 1} then continue
	   set temp [set param${field}($line)]
	   set out [string range $temp 0 [expr $col-1]]${char}[string range \
		$temp $col end]
	   set param${field}($line) $out
	   lineShow [set param${field}($line)] $x $col $line $start($field) \
		$startcol $width($field) $flag $edit($field)
	   incr col
	   continue
	   }
#Delete key
	if {$char == [format %c 127]} then {
	   if {$line > $maxline} then continue
	   if {$col == 0} then continue
	   set temp [set param${field}($line)]
	   set out [string range $temp 0 [expr $col-2]][string range \
		$temp $col end]
	   set param${field}($line) $out
	   incr col -1
	   lineShow [set param${field}($line)] $x $col $line $start($field) \
		$startcol $width($field) $flag $edit($field)
	   continue
	   }
#Carriage return
	if {$char == "\r"} then {
	   set col 0
	   incr line
	   continue
	   }
#Ctrl-A
	if {$char == $CTRL_A} then {
	   if {$startcol == 0} then {
		set startcol [expr $start($field)-1]
	   } else {
		set startcol 0
		}
	   continue
	   }
#Ctrl-B
	if {$char == $CTRL_B} then {set CTRL B; break}
#Ctrl-C
	if {$char == $CTRL_C} then {set CTRL C; break}
#Ctrl-D
	if {$char == $CTRL_D} then {
	   incr line $npage
	   continue
	   }
#Ctrl-E
	if {$char == $CTRL_E} then {
	   set col $width($field)
	   continue
	   }
#Ctrl-F
	if {$char == $CTRL_F} then {
	   set field [expr int(fmod($field+1,$nfield))]
	   set col 0
	   set startcol 0
	   continue
	   }
#Ctrl-H
	if {$char == $CTRL_H} then {set CTRL H; break}
#Ctrl-I - Insert line at cursor position
	if {$char == $CTRL_I} then {
	   if {$editAll == 0} then continue
	   loop j $line $indxmax {
		set jold [expr $indxmax-$j+$line-1]
		set jnew [expr $jold+1]
		loop i 0 $nfield {
		   set param${i}($jnew) [set param${i}($jold)]
		   }
		set msg($jnew) $msg($jold)
		}
	   loop i 0 $nfield {
		set param${i}($line) ""
		}
	   set msg($line) ""
	   incr indxmax
	   set dstart -999
	   set col 0
	   continue
	   }
#Ctrl-J
	if {$char == $CTRL_J} then {
	   if {$line > $maxline} then continue
	   if {![info exists msg($line)]} then {
		set msg($line) 1
	   } else {
		if {$msg($line) == ""} then {
		   set msg($line) 1
		} else {
		   set msg($line) ""
		   }
		}
	   loop k 0 $nfield {
	      lineShow [set param${k}($line)] $x 0 $line $start($k) \
		$startcol $width($k) $msg($line) $edit($field)
		}
	   continue
	   }
#Ctrl-K
	if {$char == $CTRL_K} then {
	   if {$line > $maxline} then continue
	   set param${field}($line) ""
	   lineShow [set param${field}($line)] $x 0 $line $start($field) \
		$startcol $width($field) $flag $edit($field)
	   continue
	   }
#Ctrl-L
	if {$char == $CTRL_L} then {set CTRL L; break}
#Ctrl-N
	if {$char == $CTRL_N} then {set CTRL N; break}
#Ctrl-O - Delete line at cursor position
	if {$char == $CTRL_O} then {
	   if {$line > $maxline} then continue
	   if {$editAll == 0} then continue
	   loop j [expr $line+1] $indxmax {
		set jnew [expr $j-1]
		set jold $j
		loop i 0 $nfield {
		   set param${i}($jnew) [set param${i}($jold)]
		   }
		set msg($jnew) $msg($jold)
		}
	   loop i 0 $nfield {
		unset param${i}($maxline)
		}
	   unset msg($maxline)
	   incr indxmax -1
	   set dstart -999
	   set col 0
	   continue
	   }
#Ctrl-P
	if {$char == $CTRL_P} then {set CTRL P; break}
#Ctrl-R
	if {$char == $CTRL_R} then {
	   set dstart -999
	   continue
	   }
#Ctrl-U
	if {$char == $CTRL_U} then {
	   incr line -$npage
	   continue
	   }
#Ctrl-W
	if {$char == $CTRL_W} then {set CTRL W; break}
#Ctrl-X
	if {$char == $CTRL_X} then {set CTRL X; break}
#Ctrl-Z
	if {$char == $CTRL_Z} then {set CTRL Z; break}
#Escape key - look for arrow codes
	if {$char == $ESC} then {
	   set char2 [read stdin 1]
	   set char3 [read stdin 1]
	   if {$char2 != "\["} then continue
	   if {$char3 == "A"} then {
		incr line -1
		continue
		}
	   if {$char3 == "B"} then {
		incr line
		continue
		}
	   if {$char3 == "C"} then {
		if {$line > $maxline} then continue
		incr col
		continue
		}
	   if {$char3 == "D"} then {
		if {$line > $maxline} then continue
		incr col -1
		continue
		}
	   }
	}
   exec stty cooked echo
   vtRestoreCursor
   flush stdout
#Restuff arrays
   if  {![info exists CTRL]} then {set CTRL Z}
   if {$CTRL == "C"} then {error abort}
   if {$CTRL != "B"} then {
	loop i 0 $nfield {
#Is the field editable?
	   if {$edit($i) == 0} then continue
#Unset existing arrays in case size has changed - save the 0th index, however,
#since the internally stored one has been mucked up.
	   set save [set param${i}_in(0)]
	   unset param${i}_in
	   set param${i}_in(0) $save
#Restore all input from temporary internal arrays
	   loop j 1 $indxmax {
		set param${i}_in($j) [set param${i}($j)]
		}
	   }
	if {[info exists msg_in(0)]} then {
	   set save $msg_in(0)
	   unset msg_in
	   set msg_in(0) $save
	   loop j 1 $indxmax {
		if {[info exists msg($j)]} then {
		   set msg_in($j) $msg($j)
		} else {
		   set msg_in($j) ""
		   }
		}
	   }
	}
   if {$line >= $indxmax} then {set line 0}
   return "$CTRL $line"
   }
