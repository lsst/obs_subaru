# Standard global variables
#
set INT_DIR $env(DERVISH_DIR)/doc/int
lappend auto_path $env(DERVISH_DIR)/contrib $env(DERVISH_DIR)/etc
# if {[catch {
# 	source $env(FTCL_DIR)/src/library/init.tcl
# 	lappend auto_path "$env(FTCL_DIR)/src/library"
#      }]} {
# 	echo \n\$FTCL_DIR/src/library is missing or defective
# }
# 
