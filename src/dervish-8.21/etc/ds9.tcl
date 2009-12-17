#
# Provide some ds9 equivalents of sao commands, and (if you run the command
# saoIsDs9) make sao* commands really use ds9; This is done for you if
# DS9_DIR is set and FSAO_DIR isn't.  My DERVISH_USER says:
#    if [info exists env(DS9_DIR)] { saoIsDs9 -ds9 }
# to switch even if fsaoimage is setup
if ![info exists sao_exclude] {
   set sao_exclude 0;			# make dspDraw* -e set "exclude"
   					#  property, as in saoimage.
}

if ![info exists sao_is_ds9] {
   set sao_is_ds9 0
}

proc saoDrawCache {args} {
   set opts [list \
		 [list [info level 0] \
		      "Dummy equivalent of ds9DrawCache"] \
		 [list -off CONSTANT 0 cache_on \
		      "Turn on caching Draw commands"] \
		 [list -flush CONSTANT 1 flush \
		      "Flush all cached Draw commands"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
}

proc saoIsDs9 {args} {
   global sao_is_ds9
   
   set really_sao 0;			# Make sao commands talk to fsao again?
   set really_ds9 0;			# Make sao commands talk to ds9?

   set opts [list \
		 [list [info level 0] \
 "Make sao* commands actually talk to ds9; sets \$sao_is_ds9 to 1"] \
		 [list -ds9 CONSTANT 1 really_ds9 \
		      "Make sao commands really talk to ds9 (default)"] \
		 [list -sao CONSTANT 1 really_sao \
		      "Make sao commands really talk to fsaoimage again"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # -ds9 is the default
   #
   if {$really_ds9 + $really_sao == 0} {
      set really_ds9 1
   }

   #
   # save sao versions and provide ds9 equivalents
   #
   set cmds [info commands sao*]
   
   if $really_sao {
      set sao_is_ds9 0

      foreach cmd $cmds {
	 if {[info commands $cmd.sao] != ""} {
	    rename $cmd {}
	    rename $cmd.sao $cmd
	 }
      }
   } else {
      set sao_is_ds9 1
      
      foreach cmd $cmds {
	 if {[info commands $cmd.sao] != ""} {
	    continue;			# we've already processed this command
	 }
	 
	 regsub {^sao} $cmd "ds9" ds9cmd
	 
	 if {[info commands $ds9cmd] == ""} {
	    continue;			# no ds9 equivalent
	 }
	 
	 if {[info commands $cmd.sao] == ""} {
	    rename $cmd $cmd.sao
	 } else {
	    rename $cmd {}
	 }
	 alias $cmd $ds9cmd
      }
   }
}

if {[info exists env(DS9_DIR)] && ![info exists env(FSAO_IMAGE)]} {
   saoIsDs9 -ds9
}

###############################################################################
#
# Provide dummy versions of some sao commands
#
proc ds9Label {args} {
   set opts [list \
		 [list [info level 0] "Not implemented"] \
		 [list <onoff> STRING "" onoff "Turn labels on/off"] \
		 [list {[fsao]} STRING "" frame "Frame to use"] \
		]
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
}


proc ds9MaskColorSet {args} {
   set opts [list \
		 [list [info level 0] "Not implemented"] \
		 [list <colors> STRING "" colors "List of colors for mask"] \
		]
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
}

proc ds9MaskGlyphSet {args} {
   set opts [list \
		 [list [info level 0] "Not implemented"] \
		 [list <glyph> STRING "" colors "???"] \
		 [list -s STRING "" frame "Frame to use"] \
		]
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
}

###############################################################################
#
# Here are wrappers of ds9 primitives to look more like the corresponding
# sao commands
#
proc ds9GetRegion {args} {
   set opts [list \
		 [list [info level 0] "List available frames"] \
		 [list -frame STRING "all" frame "Only consider this frame"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   
   set frames [xpaGet "frame all"]

   if {$frame == "all"} {
      return $frames
   } else {
      if {[lsearch $frames $frame] < 0} {
	 return ""
      } else {
	 return $frame
      }
   }
}

if ![info exists ds9Frame] {
   set ds9Frame ""
}

proc ds9FrameSet {args} {
   global ds9Frame
   set opts [list \
		 [list [info level 0] ""] \
		 [list <frame> STRING "" frame "The desired frame"] \
		 [list -ntry INTEGER 2 ntry \
		      "How many times to check to see if the frame exists"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$frame == $ds9Frame} {
      return
   } else {
      ds9DrawCache -flush
      set ds9Frame $frame
   }
   
   if {$frame != ""} {
      catch {
	 xpaSet "frame $frame"
      }

      loop i 0 $ntry {
	 if {[set msg [ds9GetRegion -frame $frame]] != ""} {
	    break
	 }

	 sleep 1
      }

      if {$i == $ntry} {
	 error "Failed to find frame $frame -- perhaps ds9 isn't running?"
      }
   }
}

proc ds9Display {args} {
   set histeq 0; set linear 0; set log 0; set sqrt 0
   set lleft 0

   set opts [list \
		 [list [info level 0] "Display a region using ds9"] \
		 [list <reg> STRING "" reg "Region to display"] \
		 [list {[frame]} STRING "" frame "Which frame to use"] \
		 [list -lowerleft CONSTANT 1 lleft \
		      "Put origin in lower left (ignored)"]\
		 [list -geometry STRING "" geometry \
		      "Where to put ds9 window (ignored)"]\
		 [list -histeq CONSTANT 1 histeq "Use histogram equalisation"]\
		 [list -linear CONSTANT 1 linear \
		  "Use a linear scale (default)"]\
		 [list -log CONSTANT 1 log "Use a logarithmic scale"]\
		 [list -sqrt CONSTANT 1 sqrt "Use a sqrt scale"]\
		]			
   #
   # Did they omit the frame argument?
   #
   if {[llength $args] >= 2 && ![regexp {^[0-9]*$} [lindex $args 1]]} {
      set frames [ds9GetRegion -frame all]
      if {$frames == ""} {
	 set frames [list -1]
      }

      global next_ds9_frame;		# next ds9 frame to use; unused by this photo
      if ![info exists next_ds9_frame] {
	 set next_ds9_frame -1
      }
      incr next_ds9_frame
      set args [linsert $args [expr [llength $args] == 2 ? 1: 2] $next_ds9_frame]
   }
   #
   # If there are no fsao options this is easy
   #
   if {[llength $args] == 2} {
      if {[shTclParseArg $args $opts [info level 0]] == 0} {
	 return 0
      }

      ds9FrameSet $frame
      ds9DisplayPrim $reg;

      return [xpaGet "frame"]
   }
   #
   # Parse fsao options
   #
   set reg [lindex $args 0]; set frame [lindex $args 1]
   set saoargs [string trimleft [string trimright [lindex $args 2]]]
   if {$saoargs != ""} {
      set args [concat  [lrange $args 0 1] [split $saoargs]]
   }

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 0
   }

   ds9FrameSet $frame

   if {$linear + $histeq + $log + $sqrt == 0} {
      set linear 1
   }
   
   if $linear {
      set scale "linear"
   }
   if $log {
      set scale "log"
   }
   if $histeq {
      set scale "histequ"
   }
   if $sqrt {
      set scale "sqrt"
   }
   
   if [catch {
      xpaSet "iconify no; raise; scale $scale"
   } msg] {
      echo "Caught xpa error $msg"
   }
   
   ds9FrameSet $frame
   xpaSet "mask clear"
   ds9DisplayPrim $reg

   return [xpaGet "frame"]
}

###############################################################################
#
# Display a mask
#
proc ds9MaskDisplay {args} {
   global saoMaskColors
   set reverse 0
   set opts [list \
		 [list [info level 0] "Display a mask in the current colour"] \
		 [list <mask> STRING "" mask "Handle to mask"] \
		 [list -p STRING "" tcl_proc "Tcl proc to run (not implemented)"] \
		 [list -t STRING "" tcl_script "Tcl script to run (not implemented)"] \
		 [list -s STRING "" frame "Frame to use"] \
		 [list -reverse CONSTANT 1 reverse "Put up bit planes in reverse order"] \
		]
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![info exists saoMaskColors] {
      set saoMaskColors [list black white red green blue cyan magenta yellow]
   }

   ds9FrameSet $frame
   xpaSet "mask yes"

   set ds9Mask [maskNew [exprGet $mask.nrow]  [exprGet $mask.ncol]]

   set i0 0; set i1 [llength $saoMaskColors]; set di 1
   if $reverse {
      set i0 [expr $i1 - 1]; set i1 -1; set di -1
   }
   loop i $i0 $i1 $di {
      maskCopy $mask $ds9Mask
      if {[maskAndEqualsValue $ds9Mask [expr 1<<$i]] > 0} {
	 xpaSet [format "mask color %s" [lindex $saoMaskColors $i]]
	 ds9DisplayPrim -mask $ds9Mask
      }
   }

   maskDel $ds9Mask

   return [xpaGet "frame"]
}

###############################################################################

proc ds9Center {args} {
   set raise 1;				# raise ds9 to top of stacking order

   set opts [list \
		 [list [info level 0] "Center the specified image"] \
		 [list -noraise CONSTANT 0 raise \
		      "Raise ds9 to top of window stack"] \
		 [list -s STRING "" frame "Frame to use"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   ds9FrameSet $frame

   if $raise {
      xpaSet "raise"
   }

   xpaSet "frame center"
}

proc ds9Pan {args} {
   set raise 1;				# raise ds9 to top of stacking order

   set opts [list \
		 [list [info level 0] \
 "Pan to the specified position (n.b. SDSS convention, so centre of first
 pixel is (0.5, 0.5))"] \
		 [list <rowc> DOUBLE 1 rowc "Row-position to pan too"] \
		 [list <colc> DOUBLE 1 colc "Column-position to pan too"] \
		 [list -noraise CONSTANT 0 raise \
		      "Raise ds9 to top of window stack"] \
		 [list -s STRING "" frame "Frame to use"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   ds9FrameSet $frame

   if $raise {
      xpaSet "raise"
   }

   xpaSet "pan to [expr $colc + 0.5] [expr $rowc + 0.5]"
}

proc ds9Zoom {args} {
   set reset 0;				# reset zoom to 1
   set raise 1;				# raise ds9 to top of stacking order

   set opts [list \
		 [list [info level 0] "Zoom the current ds9 frame"] \
		 [list {[magnification]} DOUBLE 1 zoom "Zoom factor"] \
		 [list -reset CONSTANT 1 reset "Reset the zoom to 1"] \
		 [list -noraise CONSTANT 0 raise \
		      "Raise ds9 to top of window stack"] \
		 [list -s STRING "" frame "Frame to use"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   ds9FrameSet $frame

   if $raise {
      xpaSet "raise"
   }

   if $reset {
      xpaSet "zoom to fit"
   }

   if {$zoom != 1} {
      xpaSet "zoom $zoom"
   }
}

###############################################################################
#
# Reset display; i.e. delete text/circle/etc
#
proc ds9Reset {args} {
   set opts [list \
		 [list [info level 0] "Remove all glyphs from display"] \
		 [list {[frame]} STRING "" _frame "Frame to use"] \
		 [list -s STRING "" frame "Frame to use"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$_frame == ""} {
      if {$frame == ""} {
	 set frame 1
      }
   } else {
      if {$frame == ""} {
	 set frame $_frame
      } else {
	 error "Please don't specify <frame> and -s <frame>"
      }
   }

   ds9FrameSet $frame
   xpaSet "regions deleteall"
   ds9DrawCache -clear
}

###############################################################################
#
# Now for various "ds9Draw" commands
#
if ![info exists ds9DrawRegionsList] {
   set ds9DrawRegionsList ""; set ds9DrawRegionsListLen 0
} 
if ![info exists ds9DrawRegionsCacheOn] {
   set ds9DrawRegionsCacheOn 0
}

proc ds9DrawCache {args} {
   global ds9DrawRegionsList ds9DrawRegionsListLen ds9DrawRegionsCacheOn

   return

   set cache_on $ds9DrawRegionsCacheOn;	# turn cache on
   set clear 0;				# reset cache, clearing pending ops
   set flush 0;				# flush cache

   set opts [list \
		 [list [info level 0] \
		      "Turn on (or off) caching ds9Draw commands"] \
		 [list -clear CONSTANT 1 clear \
		      "Discard all cached Draw commands"] \
		 [list -flush CONSTANT 1 flush \
		      "Flush all cached Draw commands"] \
		 [list -off CONSTANT 0 cache_on \
		      "Turn off caching Draw commands"] \
		 [list -on CONSTANT 1 cache_on \
		      "Turn on caching Draw commands (default)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if $clear {
      set ds9DrawRegionsList ""; set ds9DrawRegionsListLen 0
   }

   if {$flush || !$cache_on} {
      if {$ds9DrawRegionsList != ""} {
	 xpaSet "regions $ds9DrawRegionsList"
      }
      set ds9DrawRegionsList ""; set ds9DrawRegionsListLen 0
   }

   set ds9DrawRegionsCacheOn $cache_on
}

proc xpaSet_with_cache {val} {
   global ds9DrawRegionsCacheOn ds9DrawRegionsList ds9DrawRegionsListLen

   if $ds9DrawRegionsCacheOn {
      if {$ds9DrawRegionsList != ""} {
	 set val "$val"
      }
      append val "\n"
      
      append ds9DrawRegionsList $val
      incr ds9DrawRegionsListLen [string length $val]
      
      if {$ds9DrawRegionsListLen > 9000} {
	 ds9DrawCache -flush
      }
   } else {
      xpaSet "regions" "$val"
   }
}

proc ds9DrawCircle {args} {
   set sao_e 0; set sao_i 0
   set colors [list black white red green blue cyan magenta yellow]

   set opts [list \
		 [list [info level 0] "Draw a circle at specified position"] \
		 [list <rowc> DOUBLE 1 rowc "Row-position to pan too"] \
		 [list <colc> DOUBLE 1 colc "Column-position to pan too"] \
		 [list <radius> DOUBLE 1 radius "Desired radius"] \
		 [list -color STRING "" color "Desired colour ($colors)"] \
		 [list -e CONSTANT 1 sao_e "Draw in red"] \
		 [list -i CONSTANT 1 sao_i "Draw in yellow"] \
		 [list -s STRING "" frame "Frame to use"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   ds9FrameSet $frame

   set ei "+"
   if $sao_e {
      global sao_exclude
      if $sao_exclude {
	 set ei "-"
      }
   
      if {$color == ""} {
	 set color "red"
      }
   }
   if $sao_i {
      if {$color == ""} {
	 set color "yellow"
      }
   }

   if {$color != ""} {
      if {[lsearch $colors $color] < 0} {
	 error "Invalid colour: $color"
      }
      set color " \# color=$color"
   }

   xpaSet_with_cache \
       "${ei}circle [expr $colc +0.5] [expr $rowc + .5] $radius $color"
}

###############################################################################

proc ds9DrawText {args} {
   set sao_e 0; set sao_i 0
   set colors [list black white red green blue cyan magenta yellow]

   set opts [list \
		 [list [info level 0] "Draw a circle at specified position"] \
		 [list <rowc> DOUBLE 1 rowc "Row-position to pan too"] \
		 [list <colc> DOUBLE 1 colc "Column-position to pan too"] \
		 [list <text> STRING 1 text \
		      "Desired text (quoted with \"{\"})"] \
		 [list -color STRING "" color "Desired colour ($colors)"] \
		 [list -e CONSTANT 1 sao_e "Draw in red"] \
		 [list -i CONSTANT 1 sao_i "Draw in yellow"] \
		 [list -s STRING "" frame "Frame to use"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   ds9FrameSet $frame

   set ei "+"
   if $sao_e {
      global sao_exclude
      if $sao_exclude {
	 set ei "-"
      }
   
      if {$color == ""} {
	 set color "red"
      }
   }
   if $sao_i {
      if {$color == ""} {
	 set color "yellow"
      }
   }

   if {$color != ""} {
      if {[lsearch $colors $color] < 0} {
	 error "Invalid colour: $color"
      }
      set color " \# color=$color"
   }

   if ![regexp {^[""]} $text] {
      set text "\"$text\""
   }

   xpaSet_with_cache \
       "${ei}text [expr $colc + 0.5] [expr $rowc + 0.5] $text $color"
}

###############################################################################

proc ds9DrawPolygon {args} {
   set sao_e 0; set sao_i 0
   set colors [list black white red green blue cyan magenta yellow]

   set opts [list \
		 [list [info level 0] "Draw a circle at specified position"] \
		 [list -color STRING "" color "Desired colour ($colors)"] \
		 [list -e CONSTANT 1 sao_e "Draw in red"] \
		 [list -i CONSTANT 1 sao_i "Draw in yellow"] \
		 [list -s STRING "" frame "Frame to use"] \
		]

   set optargs ""
   set coords ""
   set is_val 0;			# is this the value of an argument?
   foreach el $args {
      if $is_val {
	 lappend optargs $el
	 set is_val 0
      } elseif {[regexp -- {^-[^0-9.]} $el]} {	# an option
	 lappend optargs $el
	 if [regexp -- {^-(color|s)$} $el] {
	    set is_val 1;		# next argument is options value
	 }
      } else {
	 lappend coords $el
      }
   }

   if {[shTclParseArg $optargs $opts [info level 0]] == 0} {
      return 
   }

   ds9FrameSet $frame

   set ei "+"
   if $sao_e {
      global sao_exclude
      if $sao_exclude {
	 set ei "-"
      }
      if {$color == ""} {
	 set color "red"
      }
   }
   if $sao_i {
      if {$color == ""} {
	 set color "yellow"
      }
   }

   if {$color != ""} {
      if {[lsearch $colors $color] < 0} {
	 error "Invalid colour: $color"
      }
      set color " \# color=$color"
   }
   #
   # Switch row, col and convert to SDSS convention on origin
   #
   set clist "("
   loop i 0 [llength $coords] 2 {
      append clist "[format %.3f [expr [lindex $coords [expr $i + 1]] + 0.5]],"
      append clist "[format %.3f [expr [lindex $coords $i] + 0.5]],"
   }
   regsub {,$} $clist ")" clist

   set ei ""
   xpaSet_with_cache "${ei}polygon$clist $color"
}
