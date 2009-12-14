#
# Create a file of C to initialise photo's FITS I/O machines
#
source ../etc/frames_pipeline_procs.tcl
declare_schematrans 5

set file read_atlas_gen.c

fitsBinTblMachineDump header $file
fitsBinTblMachineDump ATLAS_IMAGE $file
fitsBinTblMachineDump OBJMASK $file
fitsBinTblMachineDump trailer $file
