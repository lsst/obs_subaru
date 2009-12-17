# TCL procedures that parse proc arguments - see ftclParseArg.c
#  the indentation and prefix parameters should match those in 
#  tclParseArgv.c
#
proc shTclGetHelpInfo {command formal_table} {
   ftclGetHelp $command $formal_table 10 "USAGE: " "\n" "\n"
}
