global cmd file

set shPipeInArgs {
  {shPipeIn  "Set up pipes to allow data to be sent from Dervish to Unix.\nReturns a file handle\n"}
  {<command> STRING "" cmd  "unix command whose output is piped to Dervish\nmay contain options/args if command is enclosed in \""}
  {-file     STRING "" file "the name of the file <command> will use as input"}
}

set shPipeOutArgs {
  {shPipeOut  "Set up pipes to allow data to be sent from Unix to Dervish.\nReturns a file handle\n"}
  {<command> STRING "" cmd  "unix command whose output is piped to Dervish\nmay contain options/args if command is enclosed in \""}
  {-file     STRING "" file "the name of the file to store the output of the <command> in"}
}

ftclHelpDefine shUtils shPipeOut [shTclGetHelpInfo shPipeOut $shPipeOutArgs]
ftclHelpDefine shUtils shPipeIn  [shTclGetHelpInfo shPipeIn  $shPipeInArgs]
 
proc shPipeIn {args} {
#     Parse the users command according to the parse table
#     The tcl variable cmd and file will be set
   upvar #0 shPipeInArgs formal_list 
   if {[shTclParseArg $args $formal_list shPipeIn] == 0} {
      return 
   }
   # Get the read and write pipes
   set pipes [pipe]
   set rp [lindex $pipes 0]
   set wp [lindex $pipes 1]

   # cmd may contain a command and arguments.  we separate these into a cmd and arglist
   if {[llength $cmd] == 1} {
         set argList ""
   } else {
      set tmpCmd [lindex $cmd 0]
      set argList [lreplace $cmd -1 -1]
      set cmd $tmpCmd
   }
   # fork off the child
   if {[set childPid [fork]] == 0} {
      # this is the child
      # assign the write end of the pipe to stdout
      close stdout
      dup $wp stdout

      # close the pipe in the child
      close $rp
      close $wp

      # if the file option was specified, open it and assign it to stdin
      # as that is where the output goes to.
      if {$file != ""} {
         set f [open $file r]
         dup $f stdin
         close $f
      }

      # Now we become the command.  We should never return from here
      execl $cmd $argList
   } else {
      # This is still the parent, return the read pipe end so the user can use
      # it.
      close $wp
      return $rp
   }
}

proc shPipeOut {args} {
#     Parse the users command according to the parse table
#     The tcl variable cmd and file will be set
   upvar #0 shPipeInArgs formal_list
   if {[shTclParseArg $args $formal_list shPipeOut] == 0} {
      return 
   }
   # Get the read and write pipes
   set pipes [pipe] 
   set rp [lindex $pipes 0]
   set wp [lindex $pipes 1]

   # cmd may contain a command and arguments.  we separate these into a cmd and arglist
   if {[llength $cmd] == 1} {
         set argList ""
   } else {
      set tmpCmd [lindex $cmd 0]
      set argList [lreplace $cmd -1 -1]
      set cmd $tmpCmd
   }

   # fork off the child
   if {[set childPid [fork]] == 0} {

      # this is the child
      close stdin

      # assign the read end of the pipe to stdin
      dup $rp stdin

      # close the pipe in the child
      close $rp
      close $wp

      # if the file option was specified, open it and assign it to stdout
      # as that is where the output goes to.
      if {$file != ""} {
         set f [open $file w]
         dup $f stdout
         close $f
      }

      # Now we become the command.  We should never return from here
      execl $cmd $argList
   } else {
      # This is still the parent, return the write pipe end so the user can use
      # it.
      close $rp
      return $wp
   }
}
