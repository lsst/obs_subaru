#
# historySave		Save history
# historyRestore	Restore history
#
# verbs to save and restore history
#
# Save and restore history to a file given by the dervish variable
# DERVISH_HISTFILE.
#
proc getHistoryFile {} {
	if { [catch { set histFile $env(DERVISH_HISTFILE) }] } {
		global DERVISH_HISTFILE
		if { [info exists DERVISH_HISTFILE] == 1 } {
			set histFile $DERVISH_HISTFILE
		} else {
			set histFile "/dev/null"
		}
	}
	return $histFile
}
#
# Save all of your history commands to [getHistoryFile]
#
proc historySave {} {
	set fd [open [getHistoryFile] w]
	set old_hline {}
	foreach h [split [history] "\n"] {
		regexp {^[ ]*[0-9]+[ ]*(.*)} $h foo hline
		if { $hline != $old_hline } {
			puts $fd $hline
			set old_hline $hline
		}
	}
	close $fd
}
#
# Restore the history from a previous session, saved in [getHistoryFile]
#
proc historyRestore {} {
	set fd [open [getHistoryFile] r]
	while { [gets $fd line] != -1 } {
		history add $line
	}
	close $fd
}
#
# Redefine exit to save history
# Added code to propagate the exit status to the UNIX shell.
# - vijay
catch {				# exit.old may already exist
   rename exit exit.old
}
proc exit {{status 0}} {
   history change ""
   if [catch {historySave} msg] {
	echo $msg
   }
   exit.old $status
}

