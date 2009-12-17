# handleBindNew		Return a new handle, bound to a pointer
# handleDup		Return a copy of a handle
# handlePtr		Return the pointer packaged in a handle
# handleType		Return the type of a handle
###############################################################################
#
# Functions to manipulate handles
#
# Return a new handle, bound to <addr>, type <type>
#
proc handleBindNew { addr type } {
	handleBind [handleNew] $addr $type
}
#
# Return a copy of a handle
#
proc handleDup { handle } {
	eval handleBind [handleNew] [handleGet $handle]
}
#
# Return a handle's ptr or type
proc handlePtr { handle } {
	lindex [handleGet $handle] 0
}
proc handleType { handle } {
	lindex [handleGet $handle] 1
}

###############################################################################
#
# regListNames lists the names of existing regions.
# Chris Stoughton  23-Jun-93
#
#
ftclHelpDefine shRegion regListNames \
	"USAGE: regListNames \n       returns a list of existing region handles"

# List out handles that are attached to the type of structure entered by the
# user.  (ie.  all REGION handles)
proc handleListFromType { type } {
   set hlist {}
   set uType [string toupper $type]
   set handles [handleList]
   set hlen [llength $handles]
   if {$hlen != 0} {
      loop i 0 $hlen {
         set aHandle [lindex $handles $i]
         if { [lindex $aHandle 1] == $uType } {
            lappend hlist [lindex $aHandle 0]
         }
      }
   }
   return $hlist
}

# Change regListNames so it does not print out error messages for regions that
#  do not exist.  Also remove the limitation that it only looks at the first
#  100 handles.  EFB
#
proc regListNames {} {
   handleListFromType REGION
}

