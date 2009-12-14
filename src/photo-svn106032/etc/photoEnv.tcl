# This file sets up the photo environment in tcl into the array photoEnv().  
#   This defines the photo
#   environment variables needed to run the pipelines.  This must be done
#   in tcl so that they can be modified from within the tcl script.

if {[info exists env(PHOTOPLAN_DIR)]}  {
   set defDirs(source) [envscan \$PHOTO_DIR]/etc
   set defDirs(regReadAsFits) [envscan \$PHOTOPLAN_DIR]/input
   set defDirs(regWriteAsFits) [envscan \$PHOTOPLAN_DIR]/output

   set photoEnv(FRAMES_TCL_SCRIPTS) .
   set photoEnv(FRAMES_PLAN) [envscan \$PHOTOPLAN_DIR]
   
   set photoEnv(PS_TCL_SCRIPTS) .
   set photoEnv(PS_PLAN) [envscan \$PHOTOPLAN_DIR]
}
