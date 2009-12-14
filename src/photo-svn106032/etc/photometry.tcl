###################################################################
# This file contains Tcl procedures for analyzing SDSS photometry.
# These tools can match 2 runs, or match objects from the field
# overlaps in a single run, and then perform statistical analysis
# on photometric differences. The two main steps are:

# 1) Match objects positionally and dump photometry to a *.par file. 
# 2) Read *.par file and plot/report various statistics.


### The following types of matches are available:

# A) do_overlaps dumps data for frame-to-frame overlaps to 
#    *.par files, analyzes matches, produces plots and a log file
#    which lists all problematic fields
#    e.g. photo> do_overlaps $run $rerun $outputDir
#
#    log file is named do_overlaps_r${run}_rr$rerun.log 
#    (for investigating problematic single fields use proc
#     plot_overlap_field)

# B) do_matches performs identical analysis to do_overlaps (both call
#    plot_overlaps) except that here data comes from 2 SDSS runs 
#    rather than from frame overlaps in a single run as above.
#
# C) do_photoQA takes photometric data from a single run and analyzes
#    various quantities (e.g. mpsf-mmod, the positions of the stellar
#    locus, etc.) 

### N.B. all tunable parameters are defined in proc get_parameter ###











 ##########################################################
 # This proc defines tunable parameters used in analysis, #
 # this is the only place where they need to be adjusted. #
 ##########################################################
proc get_parameter {name {fp ""}} {

   ### these parameters affect the sources dumped to files 
     # these flags must be on
     set param(flagsON) {}
     # these flags must be off
     set param(flagsOFF) [list BRIGHT SATUR BLENDED]
     #set param(flagsOFF) [list BRIGHT SATUR]
     #set param(flagsOFF) [list BRIGHT]
     # only sources of this type are considered
     #set param(obj_type) STAR
     #set param(obj_type) GALAXY
     set param(obj_type) all
     # box half-size in arcsec for matched sources
     set param(match_size) 0.5
     set param(match_size) 1.0

   ### these parameters affect only the calculation of statistics
     # maximum magnitude for a matched object to be used
     set param(mmax) 20.5
     # maximum error for a matched object to be used
     set param(emax) 0.3
     set param(emax) 0.3
     # max. magnitude difference for a source to be considered
     set param(mdifmax) 0.8
     # minimum number of sources in an overlap to consider it
     # statistically significant 
     set param(Nmin) 5

   ### these parameters affect only the warnings
     # max. deviation for the median of the psf magnitude difference
     set param(psf_med_max) 0.05 
     # max. deviation for the sigma of the psf magnitude difference
     set param(psf_sig_max) 0.1 
     # max. deviation for the sigma of psf chi distribution
     set param(psf_chi_max) 2.0
     # max. deviation for the median of the model magnitude difference
     set param(mod_med_max) 0.1 
     # max. deviation for the sigma of the model magnitude difference
     set param(mod_sig_max) 0.2 
     # max. deviation for the sigma of model chi distribution
     set param(mod_chi_max) 3.0
       

   ### do work
     if {$fp != ""} { 
        foreach n [array names param] {
           puts $fp "# $n = $param($n)"
        }
     }

     if {[info exist param($name)]} {
         return $param($name)
     } else {
         error "get_parameter: $name is not defined"
     }

}





###################################################################
# User wrappers:

### For analyzing frame overlaps within a run
proc do_overlaps {run {rerun 0} {outputDir .} {device /VCPS} \
                  {dump 1} {camCols "all"} {f1 -1} {f2 -1} } {

    # take care of defaults
    if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}

    # open log file   
    set fd [open do_overlaps_r${run}_rr$rerun.log "w"]
    puts $fd "# *** LOG FILE from do_overlaps ***"
    puts $fd \
         "# match of run $run rerun $rerun cols $camCols "
    set aux [get_parameter mmax $fd]
    set Nbad ""

    # dump files
    if {$dump} {
        # matching size in arcsec
        set ms [get_parameter match_size]
        # these flags must be on
        set flagsON [get_parameter flagsON]
        # these flags must be off
        set flagsOFF [get_parameter flagsOFF]
        # we want only this object type
        set obj_type [get_parameter obj_type]
        # all set, do work
        set output_file $outputDir/overlaps-run$run 
        if {[match_overlaps $run $rerun $camCols $f1 $f2 $output_file \
             $ms "default" $flagsON $flagsOFF $obj_type] != 0} {
            error "do_overlaps: could not dump match files."
    }
    }

    # analyze and plot overlaps-run$run_delmag_$type.ps
    set badList {}
    foreach camCol $camCols {   
        set match_file $outputDir/overlaps-run$run-col$camCol.par
        # overlaps-run$run_delmag_psfCounts.ps 
        set psf_list [plot_overlaps $match_file $device "psfCounts" $f1 $f2]        
        # overlaps-run$run_delmag_counts_model.ps 
        set mod_list [plot_overlaps $match_file $device "counts_model" $f1 $f2]
        # write report to log file
        lappend badList [write_log $fd $camCol $psf_list $mod_list] 
    }

    # write summary to log file
    puts $fd "#"; puts $fd "# ****** SUMMARY ******"
    puts $fd "#  Column   psf:  u    g    r    i    z     mod: u    g    r    i    z"
    foreach el $badList {
        set index [lsearch $badList $el]
        set col [lindex $camCols $index]
        puts $fd "      $col        $el"       
    }     
   
    close $fd

    return $badList

}




### For analyzing 2 runs' overlaps
proc do_matches {run1 rerun1 {camCols "all"} {f1 -1} {f2 -1} run2 rerun2 \
             {outputDir .} {device /VCPS} {dump 1} {orphans 0}} {

    # take care of defaults
    if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}

    # open log file   
    set fd [open do_matches_r${run1}_rr${rerun1}_r${run2}_rr${rerun2}.log "w"]
    puts $fd "# *** LOG FILE from do_matches ***"
    puts $fd \
         "# match of run $run1 rerun $rerun1 cols $camCols with run $run2 rerun $rerun2"
    set aux [get_parameter mmax $fd]
    set Nbad ""

    # dump files
    if {$dump} {
        # matching size in arcsec
        set ms [get_parameter match_size]
        # these flags must be on
        set flagsON [get_parameter flagsON]
        # these flags must be off
        set flagsOFF [get_parameter flagsOFF]
        # we want only this object type
        set obj_type [get_parameter obj_type]
        # all set, do work
        set outfile $outputDir/match
        set logfile $outputDir/match_log
        if {[match_runs $run1 $rerun1 $run2 $rerun2 $camCols $f1 $f2  \
          $outfile $ms $flagsON $flagsOFF $obj_type $logfile $orphans] != 0} {
            error "do_matches: could not dump match files."
    }
    }
                 
    # analyze and plot overlaps-run$run_delmag_$type.ps
    # field window half-width
    set hw 0
    set badList {}
    foreach camCol $camCols {   
        set match_files [glob $outputDir/match-run$run1-col${camCol}VSrun$run2-col?.par]
        foreach match_file $match_files {
           # overlaps-run$run_delmag_psfCounts.ps 
           set psf_list [plot_overlaps $match_file $device "psfCounts" $f1 $f2 $hw]        
           # overlaps-run$run_delmag_counts_model.ps 
           set mod_list [plot_overlaps $match_file $device "counts_model" $f1 $f2 $hw]
           # write report to log file
           lappend badList [write_log $fd $camCol $psf_list $mod_list] 
       }
    }

    # write summary to log file
    puts $fd "#"; puts $fd "# ****** SUMMARY ******"
    puts $fd "#  Column   psf:  u    g    r    i    z     mod: u    g    r    i    z"
    foreach el $badList {
        set index [lsearch $badList $el]
        set col [lindex $camCols $index]
        puts $fd "      $col        $el"       
    }     
   
    close $fd

    return $badList

}





### For analyzing photometry for a specified run
proc do_photoQA {run {rerun 0} {outputDir .} {device /VCPS} \
                 {dump 1} {camCols "all"} {f1 -1} {f2 -1} } {

global mem

    # for memory reporting
    set startMem [memNextSerialNumber]
    set mem [open do_photoQA.$run.mem.trace w]

    # take care of defaults
    if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}

    # open log file   
    set fd [open do_photoQA_r${run}_rr$rerun.log "w"]
    puts $fd "# *** LOG FILE from do_photoQA ***"
    puts $fd \
         "# run $run rerun $rerun cols $camCols "
    set aux [get_parameter mmax $fd]
    set Nbad ""

    # dump files
    if {$dump} {
        # these flags must be on
        set flagsON [get_parameter flagsON]
        # these flags must be off
        set flagsOFF [get_parameter flagsOFF]
        # we want only this object type
        set obj_type [get_parameter obj_type]
        set no_doubles 1
        # all set, do work
        foreach col $camCols {
             write_mem mem "dumping col $col"
             set outfile $outputDir/photometry-run$run-col$col 
             set log_file $outputDir/dump_tsObj-run$run-col$col.log     
             dump_tsObj $outfile $run $rerun $col $f1 $f2 $flagsON $flagsOFF \
                        $obj_type 30 $no_doubles $log_file 
        }
    }

    # analyze and plot
    set half_win 0
  set half_win 1
    foreach col $camCols {   
        write_mem mem "analyzing col $col"
        set dump_file $outputDir/photometry-run$run-col$col.par
        photometryQA $dump_file $device $half_win
    }

    # check for memory leaks
    assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}
 
    return 0

}












######################################################################

# for writing report
proc write_log {log_file camCol psf_list model_list} {
    
    set filterlist {u g r i z}

    puts $log_file "# *** Results for camera column $camCol ***"

    foreach type {psf model} {
       puts $log_file "#"
       puts $log_file "#     = $type magnitudes ="
    
       # loop over filters 
       loop i 0 5 {
          set f [lindex $filterlist $i]
          set Nbad($f) 0
      if {$type == "psf"} { 
              set list_aux [lindex $psf_list $i]
          } else {
              set list_aux [lindex $model_list $i]
          }
          if {$list_aux == {}} {
             puts $log_file "#       All fields OK for filter $f"
          } else {
             puts $log_file "#       - filter $f -"           
             puts $log_file "#     field   Ndata    med     sig     chi"
             foreach el $list_aux {
               incr Nbad($f)
               set field [format "%4d" [lindex $el 0]]
               set Ndata [format "%4d" [lindex $el 1]]
               set med   [format "%6.3f" [lindex $el 2]]
               set sig   [format "%6.3f" [lindex $el 3]]
               set chi   [format "%6.3f" [lindex $el 4]]
               puts $log_file "      $field    $Ndata   $med  $sig  $chi"
             }
          } 
          set Nbad($f) [format "%4d" $Nbad($f)]     
       }
       set bad($type) "$Nbad(u) $Nbad(g) $Nbad(r) $Nbad(i) $Nbad(z)"
   }

   return "$bad(psf)       $bad(model)"
    
}

# this proc finds overlap of 2 arbitrary runs
# e.g. 
# photo> match_runs 745 20 756 20 all -1 -1 Match 1.0 {} {BRIGHT} all match_2runs 1 30 {} {BRIGHT}
proc match_runs {run1 rerun1 run2 rerun2 {camCols "all"} {startField1 -1} {endField1 -1}  \
         {outfile ""} {match_size 1} {flagsON {}} {flagsOFF {}} {obj_type "all" } \
         {log_file_root "match_2runs"} {orphans 0} {rmax 30} {orphON {}} {orphOFF {}} } {

global openit

     # take care of defaults
     if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}

     # loop over camera columns
     foreach camCol $camCols {   
         echo; echo "working on camera column $camCol from run $run1"   
         # first find the overlapping fields
         set overlap [find_run_overlap $run1 $camCol $run2 $startField1 $endField1]
         set nfields [lindex $overlap 2]
         if {$nfields < 1} {
             echo "There is no overlap between run $run1, column $camCol and run $run2."
             continue
         } else {
             set startMatch1 [lindex $overlap 0]
             set startMatch2 [lindex $overlap 1]
             echo " Run $run1, column $camCol and run $run2 overlap for $nfields fields."
             echo " startMatch1 = $startMatch1, startMatch2 = $startMatch2" 
         }
         set one_run 0
         set_run $run1 $camCol -tsObj
         set tsObjdir1 $openit(objdir)
         foreach col2 [lindex $overlap 3] {
              echo "        matching to run $run2, col $col2"
              set_run $run2 $col2 -tsObj
              set tsObjdir2 $openit(objdir)
              set log_file $log_file_root-run$run1-col${camCol}VSrun$run2-col$col2.log
              if {$outfile != ""} {
                  set out $outfile-run$run1-col${camCol}VSrun$run2-col$col2
              } else {
                  set out match-run$run1-col${camCol}VSrun$run2-col$col2
              }
              match_tsObj $run1 $rerun1 $camCol $startMatch1 $run2 $rerun2 $col2 $startMatch2 \
                          $nfields $tsObjdir1 $tsObjdir2 $out $match_size $one_run \
                          $flagsON $flagsOFF $obj_type $log_file $orphans $rmax $orphON $orphOFF
         } 
     }
     
     return 0

}




# wrapper around match_runs to get the depths in each specified band
proc match_bands {run1 rerun1 run2 rerun2 {bands {u g r i z}} {camCols "all"}   \
          {startField1 -1} {endField1 -1} {outfile "depth"}} {

global fband  

       foreach band $bands { 
           set fband $band
           set outf Filter-$fband-$outfile 
           # sources must be detected in this band => BINNED1 must be on
           # and we don't care for BRIGHT sources and parents
           set flagsON {BINNED1}
           set flagsOFF {BRIGHT BLENDED SATUR}
           match_runs $run1 $rerun1 $run2 $rerun2 $camCols $startField1 $endField1 \
           $outf 1 $flagsON $flagsOFF "all" $outf 1 30 $flagsON $flagsOFF
       }


}



# this proc finds fields overlap by calling match_tsObj for each 
# camera column 
proc match_overlaps {run rerun {camCols "all"} field1 field2 \
                 outfile {match_size 1} {tsObjdir "default"} \
                     {flagsON {}} {flagsOFF {}} {obj_type "all" }\
                     {log_file_root "match_overlaps"}} {
global openit

     # take care of defaults
     if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}
     if {$field1 < 0 || $field2 < 0} {
         # we want all fields
         set all_fields 1
     } else {
         set all_fields 0  
         set nfields [expr $field2 - $field1 + 1]
     }
     set call_set_run $all_fields 
     set tsObjdir_def 0
     if {$tsObjdir == "default"} { 
         set call_set_run 1
         set tsObjdir_def 1
     } 

     # loop over camera columns
     foreach camCol $camCols {   
         echo; echo "working on camera column $camCol"   
         if {$call_set_run} { 
             set_run $run $camCol -tsObj
         }
         if {$tsObjdir_def} { 
             set tsObjdir $openit(objdir)
         }
         if {$all_fields} { 
             set fields [find_field_range $tsObjdir]
             set field1 [lindex $fields 0]
             set field2 [lindex $fields 1] 
             set nfields [expr $field2 - $field1]            
         }
         set log_file $log_file_root-run$run-col$camCol
         match_tsObj $run $rerun $camCol $field1 $run $rerun \
                     $camCol [expr $field1 + 1] $nfields $tsObjdir \
                     $tsObjdir $outfile-col$camCol $match_size 1 \
                     $flagsON $flagsOFF $obj_type $log_file
     }

     return 0
         
}


# find overlapping fields between run1, column1, fields ranging
# from startField1 to endField1 (can be all by setting -1) and run2
# return a list (start1 start2 nfields matchCol2) where start1
# is the first overlapping field in run1, start2 is the first 
# overlapping field in run2, and the nfields is the overlap length.
# matchCol2 is a list of up to 2 elements containing matched
# columns from run2 
# N.B. assumes that runs are parallel so that fields from 
#      run1/column1can match at most 2 from the other run
proc find_run_overlap {run1 column1 run2 {startField1 -1} {endField1 -1}} {

global data_root openit


    # make sure data dir is kosher
    if {![info exist data_root] || ![file exist $data_root]} {
        set data_root [get_data_root ""]
    }
    if {![info exist data_root]} {
        error "Specified data_root does NOT exist: $data_root"
    }

    set rerun [get_highest_dir $data_root/$run1]
    if {$rerun < 0} {
       error "Directory $data_root/$run doesn't contain any reruns!?"
    }  
    
    # take care of the defaults 
    if {$startField1 == -1 || $endField1 == -1} {
        set_run $run1 $column1 -rerun $rerun
        set asTransDir $openit(asdir)
        set fields [find_field_range_asTrans $asTransDir]
        if {$startField1 == -1} {set startField1 [lindex $fields 0]}
        if {$endField1 == -1} {set endField1 [lindex $fields 1]}
        assert { $endField1 >= $startField1}
    }         

    ### first assume that run 2 starts first 
    # search for the lower left corner of run 1 in run2
    set aux [pix2pix $run1 $column1 $startField1 0 0 $run2]
    if {$aux == "-1"} {
        # try to find the lower right corner
        set aux [pix2pix $run1 $column1 $startField1 0 2048 $run2]
        if {$aux == "-1"} {
           # wrong assumption: run 1 starts first 
           set startOK 0
           # find the first field from run1, column1 observed in run2
           while {$startOK == "0" && $startField1 < $endField1} {
               incr startField1  
               if {[pix2pix $run1 $column1 $startField1 0 2048 $run2] != -1} {
                   set startOK R
                   set aux [pix2pix $run1 $column1 $startField1 0 2048 $run2]
               } elseif {[pix2pix $run1 $column1 $startField1 0 0 $run2] != -1} {
                   set startOK L
                   set aux [pix2pix $run1 $column1 $startField1 0 0 $run2]
               }
       }
        } else {
           # found run 2 on the north (right, col=2048) side of run 1
           set startOK R
        }
    } else {
        # found run 2 on the south (left, col=0) side of run 1
        set startOK L
    }

    # if established that run 2 starts first, then copy needed fields
    if {$startOK != "0"} {
        set matchStart1 $startField1
        set matchCol2 [lindex $aux 1]    
        # offset of 1 field to conform to the requirement from match_tsObj
        # that the lower edge of frame2 must be within frame1
        set matchStart2 [expr [lindex $aux 2] + 1]
        # check whether there is a 2-column overlap
        # makes assumption that each column has the same number of fields)
        if {$startOK == "L"} {            
        if {[lindex $aux 4] > 1768 && $matchCol2 < 6} {
                set matchCol2 [list $matchCol2 [expr $matchCol2+1]]      
            }
        } else {
        if {[lindex $aux 4] < 280 && $matchCol2 > 1} {
                set matchCol2 [list $matchCol2 [expr $matchCol2-1]]      
            }
        }
    } else {
        # there is no overlap
        return [list 0 0 0 0]
    }

    ### now find the overlap end
    set endOK 0
    # first assume that run 2 goes further than run 1
    set aux [pix2pix $run1 $column1 $endField1 0 2048 $run2]
    if {$aux == "-1"} {set aux [pix2pix $run1 $column1 $endField1 0 0 $run2]}
    if {$aux != "-1"} {
        # correct assumption
        set nfields [expr $endField1 - $startField1 + 1]
    } else {
        # incorrect assumption, run 1 goes further
        # what is the last field in run2?  

        set_run $run2 [lindex $matchCol2 0] -rerun $rerun
        set asTranDdir $openit(asdir)
        set fields [find_field_range_asTrans $asTransDir]
        set endField2 [lindex $fields 1]
        # where is that field in run1?
        set aux [pix2pix $run2 [lindex $matchCol2 0] $endField2 0 2048 $run1]
        if {$aux == "-1"} {set aux \
            [pix2pix $run2 [lindex $matchCol2 0] $endField2 0 0 $run1]}
        set nfields [expr [lindex $aux 2] - $startField1 + 1]     
    } 
 
    return [list $matchStart1 $matchStart2 $nfields $matchCol2]
}



##################################################################
# procs for studying the data depth

# given a file, read a vector of data from column C and 
# from lines ranging from L1 to L2 (all by default)
# C is assumed to be an integer.
# this proc can be also used to read vectors from par files, 
# instead of using param2Chain which (slowly) reads everything
# and thus uses a lot of memory
# when reading par file specify the record name, e.g. set C dec.
# if the structure contains a list, e.g. PHOTOMETRY's record 
# psfCounts[5], then make C a list, e.g.
# set C {psfCounts 2} 
# to select psfCounts<2>, i.e. mpsf(r)
proc file2Vector {file C {L1 -1} {L2 -1} {sanity 1}} {

    # find the number of lines in this file and do sanity check
    if {$sanity} { 
       set Nlines [lindex [exec wc -l $file] 0]
    }

    # what kind of data we want
    if [catch {expr $C * 1}] {
        set parfile 1
        set record [lindex $C 0]
        if {[llength $C] > 1} {
            set partype "vector"
            set band [lindex $C 1]
        } else {
            set partype "scalar"
        }       
    } else {
        set parfile 0
        set Nrecord [expr $C - 1]
    }

    # open file
    set inf [open $file r]

    # want to start from the beginning
    if {$L1 < 0} {
        if {$parfile} {
           set bDef 1
           set L1 0
           set structOK 0
           set Fstruct 1
           set b "\} "
           set e ";"
           set d "typedef struct \{"
           while {!$structOK} {
              incr L1 
              set line [gets $inf]
              if {$line == $d} {
                  set Fstruct 1
              }
              set lB [string range $line 0 1]
              set nC [expr [string length $line]-1]
              set lE [string range $line $nC $nC] 
              if {$lB == $b && $lE == $e && $Fstruct} {
                 set structOK 1
                 set structName [string range $line 2 [expr $nC-1]] 
                 set structList [schemaGetFromType $structName]
                 loop i 0 [llength $structList] {
                     set aux [lindex [lindex $structList $i] 0]
                     if {$aux == $record} {
                         set Nrecord [expr $i + 1]
                     }
                 }
              }
           }
           if {!$structOK} {
               error "could not find a structure definition"
       }
        } else {
           set L1 0
        }
    } else {
        set bDef 0
        loop i 1 $L1 {
           set line [gets $inf]
        }
    }
    
    # how many lines?
    if {$L2 < 0} { 
        set eDef 1
        set L2 [expr $Nlines - $L1]      
    } else {
        set eDef 0
    }

    # make vector
    set Ndata [expr $L2 - $L1 + 1]
    set vector [vectorExprNew $Ndata]

    # loop over input file
    set Necho 1000000
    set Nline 0; set Nvec 0
    #echo "  reading lines $L1-$L2..."
    loop i $L1 [expr $L2 + 1] {
       #echo line $i ($L1-$L2)
       set line [gets $inf]
       # process line
       if {![eof $inf]} {  
           if {[string range $line 0 0] == "#" || $line == ""} {  
               continue
           }
           incr Nline   
           if {[expr $Nline/$Necho*$Necho] == $Nline} {echo $Nline}
           if [catch { 
               # extract values
               set record [lindex $line $Nrecord]
               if {$parfile && $partype == "vector"} {
                   set record [lindex $record $band]
               } 
               handleSet $vector.vec<[expr $i - $L1]> $record
               incr Nvec 
           } msg] {
               #echo "  bad line $i: $msg"
               #echo $line
           }
       }
    }
    close $inf
 
    echo "Read $Nline lines, $Nvec values are good"

    return [list $Nvec $vector]

}


# given a text file, read a vector of data from (1-indexed) column C 
# and from lines ranging from L1 to L2 (all by default)
# N.B. similar to file2Vector, except it doesn't support par files
proc Tfile2Vector {file C {L1 -1} {L2 -1}} {

    assert { $C > 0 }
    set Nrecord [expr $C-1]

    # want to start from the beginning?
    if {$L1 < 0} {
       set L1 1
    }
    
    # all lines?
    if {$L2 < 0} { 
        set L2 [lindex [exec wc -l $file] 0]
    }

    # make vector
    set Ndata  [expr $L2 - $L1 + 1]
    set vector [vectorExprNew $Ndata]

    # open file
    set inf [open $file r]

    # loop over input file
    set Necho 1000000
    set Nline 0; set Nvec 0
    set iLine 0 
    while {![eof $inf] && $iLine <= $L2} {
       incr iLine
       set line [gets $inf]
       # process line
       if {![eof $inf] && $iLine >= $L1} {  
           if {[string range $line 0 0] == "#" || $line == ""} {  
               continue
           }
           incr Nline   
           if {[expr $Nline/$Necho*$Necho] == $Nline} {echo $Nline}
           if [catch { 
               # extract values
               set record [lindex $line $Nrecord]
               handleSet $vector.vec<$Nvec> $record
               incr Nvec
           } msg] {
               echo "  bad line $iLine: $msg"
               echo $line
           }
       }
    }
    close $inf

    return $vector

}





# given a vector of decimal numbers, produce a histogram 
# of N-th digits
proc NdigitHist {vector Ndigit device} {

               
       set factor [expr pow(10.0,$Ndigit)]
       set b1 [vectorExprEval int($vector*0.1*$factor)]     
       set b2 [vectorExprEval $b1*10/$factor]     
       set b3 [vectorExprEval $vector-$b2]             
       set b4 [vectorExprEval $factor*$b3]       
       plot_histogram $device $b4 10

       vListDel [list $b1 $b2 $b3 $b4]
}


# get a histogram of values in vector. The range can be specified
# as list minmax = {min max}, and also a vector mask.
proc vecToHist {vector nbin {minmax ""} {mask ""} {normalize 0}} {

        set hg [hgNew]
        if {$minmax != "" } {
        set min [lindex $minmax 0]
        set max [lindex $minmax 1]
            hgDefine $hg -min $min -max $max -nbin $nbin 
            if {$mask == ""} {           
               set mask1 [vectorExprEval "$vector < $max ? 1 : 0"]
            } else {
               set mask1 [vectorExprEval "$vector < $max ? $mask : 0"]
            }
            set mask2 [vectorExprEval "$vector > $min ? $mask1 : 0"]
            vectorExprDel $mask1
        } else {
            hgDefine $hg -nbin $nbin 
            if {$mask != ""} {           
                set mask2 $mask
            } 
        }

        # fill the histogram
        if {$mask == "" && $minmax == "" } { 
             hgFillFromV $hg $vector
        } else {          
             hgFillFromV $hg $vector -vMask $mask2
             vectorExprDel $mask2
        }

        # in requested, normalized the histogram by the total number of objects
        if {$normalize} {
            set Ntotal [exprGet $hg.entries]
            loop i 0 [exprGet $hg.nbin] {
                handleSet $hg.contents<$i> [expr 1.0*[exprGet $hg.contents<$i>]/$Ntotal]
            } 
        }

    return $hg
}



# 
# e.g. plot_completeness Filter-r-depth-run745-col1VSrun756-col1 r "chip 1r, 30 fields" /VCPS
# PAGE 1
# panel 1: numbers of matched sources and orphans from each run
# panel 2: completeness for each run
# panel 3: numbers of SS, GG, (diff. types) 
# panel 4: photometric errors as a function of magnitude, separately for all and for SS only
# PAGE 2
# panel 1: completeness for each run for all sources (same as panel 2 above)
# panel 2: completeness for each run for STARS only (using type(filter))
proc plot_completeness {fileroot filter title device {fMin 0} {fMax 1000}} {

    set filters {u g r i z}
    set band [lsearch $filters $filter]
    if {$band < 0} {return "Filter $filter does not exist"}
    set Nbins 120; set mmin 14.0; set mmax 25.0 
    set NbinsErr 24.0

    ### read data ###

    # model mags for epoch1 for matched sources
    set Cm [list counts_model $band]
    set matches [lindex [file2Vector $fileroot.par $Cm] 1]
    # model mags for orphans
    set orph1 [lindex [file2Vector $fileroot-orphans1.par $Cm] 1]
    set orph2 [lindex [file2Vector $fileroot-orphans2.par $Cm] 1]

    # N.B. this is not objc_type, it is type(band)
    echo "   getting histograms [exec date]"
    set Ctype objc_type
    set mtypeA [lindex [file2Vector $fileroot.par $Ctype] 1]
    set Ctype objc_typeB
    set mtypeB [lindex [file2Vector $fileroot.par $Ctype] 1]
    set Ctype objc_type
    set orph1Type [lindex [file2Vector $fileroot-orphans1.par $Ctype] 1]
    set orph2Type [lindex [file2Vector $fileroot-orphans2.par $Ctype] 1]

    # flags2 to get rid of moving objects
    set Mflags [lindex [file2Vector $fileroot.par objc_flags] 1]
    set Mflags2 [lindex [file2Vector $fileroot.par objc_flags2] 1]
    set O1flags [lindex [file2Vector $fileroot-orphans1.par objc_flags] 1]
    set O1flags2 [lindex [file2Vector $fileroot-orphans1.par objc_flags2] 1]
    set O2flags [lindex [file2Vector $fileroot-orphans2.par objc_flags] 1]
    set O2flags2 [lindex [file2Vector $fileroot-orphans2.par objc_flags2] 1]

    # fields 
    set Mfield  [lindex [file2Vector $fileroot.par field] 1]
    set MfieldB [lindex [file2Vector $fileroot.par fieldB] 1]
    set O1field [lindex [file2Vector $fileroot-orphans1.par field] 1]
    set O2field [lindex [file2Vector $fileroot-orphans2.par field] 1]
    set fieldMin [vExtreme $Mfield min]
    set fieldMax [vExtreme $Mfield max]
    set fieldMinB [vExtreme $MfieldB min]
    set fieldMaxB [vExtreme $MfieldB max]
    set fieldOffset [expr $fieldMaxB - $fieldMax]
    if {$fMin < $fieldMin} {
        set fMin $fieldMin
    }
    if {$fMax > $fieldMax} {
        set fMax $fieldMax
    }
    set fMinB [expr $fMin + $fieldOffset]
    set fMaxB [expr $fMax + $fieldOffset]

    echo " ==== The field range is $fieldMin-$fieldMax (offset = $fieldOffset) ===="
    # limit for counts
    set Nfields [expr $fieldMax-$fieldMin+1]
    if {$Nfields < 100} {
       set Nmax 3.5
    } elseif {$Nfields < 300} {
       set Nmax 4.0
    } else {
       set Nmax 4.5  
    }
   
    set MfieldMask  [vectorExprEval "$Mfield >= $fMin && $Mfield <= $fMax ? 1 : 0"]
    set O1fieldMask  [vectorExprEval "$O1field >= $fMin && $O1field <= $fMax ? 1 : 0"]
    set O2fieldMask  [vectorExprEval "$O2field >= $fMinB && $O2field <= $fMaxB ? 1 : 0"]


    ### process data ###

    # masks for (not) moving objects
    set MMOmask  [flagsMask $Mflags2 {DEBLENDED_AS_MOVING} {}]
    set O1MOmask [flagsMask $O1flags2 {DEBLENDED_AS_MOVING} {}]
    set O2MOmask [flagsMask $O2flags2 {DEBLENDED_AS_MOVING} {}]
    set MnotMOmask  [vectorExprEval "$MMOmask == 0 ? $MfieldMask : 0"]
    set O1notMOmask [vectorExprEval "$O1MOmask == 0 ? $O1fieldMask : 0"]
    set O2notMOmask [vectorExprEval "$O2MOmask == 0 ? $O2fieldMask : 0"]
    set SS [vectorExprEval "$mtypeA == 6 && $mtypeB == 6 && !$MMOmask ? $MfieldMask : 0"]
    set GG [vectorExprEval "$mtypeA == 3 && $mtypeB == 3 && !$MMOmask ? $MfieldMask : 0"]
    set NS [vectorExprEval "$mtypeA != $mtypeB && !$MMOmask ? $MfieldMask : 0"]    

    # orphaned stars
    set SSo1 [vectorExprEval "$orph1Type == 6 && !$O1MOmask ? $O1fieldMask : 0"]
    set SSo2 [vectorExprEval "$orph2Type == 6 && !$O2MOmask ? $O2fieldMask : 0"]

    ### get magnitude histograms
    set HSS [vecToHist $matches $Nbins [list $mmin $mmax] $SS]
    set HGG [vecToHist $matches $Nbins [list $mmin $mmax] $GG]
    set HNS [vecToHist $matches $Nbins [list $mmin $mmax] $NS]
    # fractions for S/G separation
    set totCorrect [hgOper $HSS + $HGG]
    set totalAll   [hgOper $totCorrect + $HNS]
    set HSSfrac [hgOper $HSS / $totalAll] 
    set HGGfrac [hgOper $HGG / $totalAll] 
    set HNSfrac [hgOper $HNS / $totalAll] 
    handleSet $HSSfrac.xLabel "mmod($filter)"
    handleSet $HSSfrac.yLabel "fraction"  
    handleSet $HGGfrac.xLabel ""; handleSet $HGGfrac.yLabel ""
    handleSet $HNSfrac.xLabel ""; handleSet $HNSfrac.yLabel ""
    handleSet $HSSfrac.name ""; handleSet $HGGfrac.name ""; handleSet $HNSfrac.name ""
 

    # logs for S/G counts plot
    set LHSS [hgLog10 $HSS]; set LHGG [hgLog10 $HGG] 
    set LHNS [hgLog10 $HNS];
    # logs for matches/orphans counts plot
    set Hm [vecToHist $matches $Nbins [list $mmin $mmax] $MnotMOmask]   
    set Ho1 [vecToHist $orph1 $Nbins [list $mmin $mmax] $O1notMOmask]   
    set Ho2 [vecToHist $orph2 $Nbins [list $mmin $mmax] $O2notMOmask] 
    set moHm  [vecToHist $matches $Nbins [list $mmin $mmax] $MMOmask]   
    set moHo1 [vecToHist $orph1 $Nbins [list $mmin $mmax] $O1MOmask]   
    set moHo2 [vecToHist $orph2 $Nbins [list $mmin $mmax] $O2MOmask] 
    # fraction of moving objects relative to the total number of matches/orphans
    set totalM  [hgOper $Hm + $moHm]
    set totalO1 [hgOper $Ho1 + $moHo1]
    set totalO2 [hgOper $Ho2 + $moHo2]
    set moHMfrac  [hgOper $moHm  / $totalM] 
    set moHo1frac [hgOper $moHo1 / $totalO1] 
    set moHo2frac [hgOper $moHo2 / $totalO2]
    hgDel $totalM; hgDel $totalO1; hgDel $totalO2; 

    handleSet $moHMfrac.xLabel "mmod($filter)"
    handleSet $moHMfrac.yLabel "fraction"  
    handleSet $moHo1frac.xLabel ""; handleSet $moHo1frac.yLabel ""
    handleSet $moHo2frac.xLabel ""; handleSet $moHo2frac.yLabel ""
    handleSet $moHMfrac.name ""
    handleSet $moHo1frac.name ""
    handleSet $moHo2frac.name ""

    handleSet $LHSS.xLabel "mmod($filter)"; handleSet $LHSS.yLabel "log10 (N)" 
    handleSet $Hm.xLabel "mmod($filter)"; handleSet $Hm.yLabel  "N matches" 
    handleSet $Ho1.xLabel "mmod($filter)"; handleSet $Ho1.yLabel "N orphans 1"  
    handleSet $Ho2.xLabel "mmod($filter)"; handleSet $Ho2.yLabel "N orphans 2" 


    ## get aux. histograms
    echo "   aux. histograms [exec date]"
    set tot1 [hgOper $Ho1 + $Hm]
    set c1 [hgOper $Hm / $tot1]
    set tot2 [hgOper $Ho2 + $Hm]
    set c2 [hgOper $Hm / $tot2]
    handleSet $c1.xLabel "mmod($filter)"; handleSet $c1.yLabel "completeness"  
    handleSet $c2.xLabel "mmod($filter)"; handleSet $c2.yLabel ""  
    handleSet $c1.name ""; handleSet $c2.name "";

    # stars separately
    set Ho1S [vecToHist $orph1 $Nbins [list $mmin $mmax] $SSo1]  
    set Ho2S [vecToHist $orph2 $Nbins [list $mmin $mmax] $SSo2] 
    set tot1S [hgOper $Ho1S + $HSS]
    set c1S [hgOper $HSS / $tot1S]
    set tot2S [hgOper $Ho2S + $HSS]
    set c2S [hgOper $HSS / $tot2S]
    handleSet $c1S.xLabel "mmod($filter)"; handleSet $c1S.yLabel "completeness"  
    handleSet $c2S.xLabel "mmod($filter)"; handleSet $c2S.yLabel ""  
    handleSet $c1S.name ""; handleSet $c2S.name "";

    # temp
    if {1} {
      set f1 0.96
      set f2 0.90
      set f3 0.25 
      # echo $c1S $c2S
      # return
      if {[catch {
        set stats1 [list $c1S [ChistStats $c1S $f1 2] [ChistStats $c1S $f2 2] [ChistStats $c1S $f3 2]]
        set stats2 [list $c2S [ChistStats $c2S $f1 2] [ChistStats $c2S $f2 2] [ChistStats $c2S $f3 2]]
        set outList [list $stats1 $stats2]
        echo $outList
	   }]} {
            echo "****** FAILED *******"
            set outList "dummy"
           }
    } else {
       set outList "dummy"
    } 

    # log10 
    set Lm [hgLog10 $Hm]     
    set Lo1 [hgLog10 $Ho1]     
    set Lo2 [hgLog10 $Ho2]     
    handleSet $Lm.xLabel "mmod($filter)"; handleSet $Lm.yLabel "log10 (N)" 
    handleSet $Lo1.xLabel "mmod($filter)"; handleSet $Lo1.yLabel ""
    handleSet $Lo2.xLabel "mmod($filter)"; handleSet $Lo2.yLabel "" 
    handleSet $Lm.name ""; handleSet $Lo1.name ""; handleSet $Lo2.name ""; 
    set LmMO [hgLog10 $moHm]     
    set Lo1MO [hgLog10 $moHo1]     
    set Lo2MO [hgLog10 $moHo2]     
    handleSet $LmMO.xLabel "mmod($filter)"; handleSet $LmMO.yLabel "log10 (N)" 
    handleSet $Lo1MO.xLabel ""; handleSet $Lo1MO.yLabel ""
    handleSet $Lo2MO.xLabel ""; handleSet $Lo2MO.yLabel "" 
 

    echo "   all set, going to plot [exec date]"



    ### plot
    if [catch {

       ## page 1
       set pg [pgstateNew]
       set dev [devicename $device depth_1col.ps]
       pgstateSet $pg -device $dev -lineWidth 3 \
          -xfract 0.3 -yfract 0.4 -nxwindow 2 -nywindow 4
       pgstateOpen $pg
       set icDef [exprGet $pg.icLine]

       # panel 1
       hgPlot $pg $Lm -xmin $mmin -xmax $mmax -ymin 0  -ymax $Nmax 
       titlePlot $title 40 
       pgstateSet $pg -isNewplot 0 -icLine 2
       hgPlot $pg $Lo1 -xmin $mmin -xmax $mmax -ymin 0  -ymax $Nmax 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $Lo2 -xmin $mmin -xmax $mmax -ymin 0  -ymax $Nmax 

       # panel 2
       pgstateSet $pg -isNewplot 1 -icLine $icDef
       hgPlot $pg $moHMfrac -xmin $mmin -xmax $mmax -ymin 0 -ymax 1.1 
       titlePlot "frac. moving obj." 40  
       pgstateSet $pg -isNewplot 0 -icLine 2
       hgPlot $pg $moHo1frac -xmin $mmin -xmax $mmax 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $moHo2frac -xmin $mmin -xmax $mmax 
       plot_line $pg 10.0 1.0 30.0 1.0

       # panel 3
       pgstateSet $pg -isNewplot 1 -icLine 2
       hgPlot $pg $c1 -xmin $mmin -xmax $mmax 
       titlePlot "All; red: run 1, blue: run 2" 40 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $c2 -xmin $mmin -xmax $mmax
       plot_line $pg 10.0 1.0 30.0 1.0
       ## The black book completeness limits in ugriz
       set BBlimits [list 22.3 23.3 23.1 22.3 20.8]

       # panel 4
       pgstateSet $pg -isNewplot 1 -icLine 2
       hgPlot $pg $c1S -xmin $mmin -xmax $mmax 
       titlePlot "STARS only; 1:red, 2:blue"  40 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $c2S -xmin $mmin -xmax $mmax
       plot_line $pg 10.0 1.0 30.0 1.0

       # panel 5
       pgstateSet $pg -isNewplot 1 -icLine $icDef
       hgPlot $pg $LHSS -xmin $mmin -xmax $mmax -ymin 0 -ymax $Nmax
       titlePlot "black: SS, blue: GG, red: diff." 40 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $LHGG -xmin $mmin -xmax $mmax  -ymin 0 -ymax $Nmax
       pgstateSet $pg -isNewplot 0 -icLine 2
       hgPlot $pg $LHNS -xmin $mmin -xmax $mmax  -ymin 0 -ymax $Nmax


       # panel 6
       pgstateSet $pg -isNewplot 1 -icLine $icDef
       hgPlot $pg $HSSfrac -xmin $mmin -xmax $mmax
       titlePlot "black: SS, blue: GG, red: diff." 40 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $HGGfrac -xmin $mmin -xmax $mmax 
       pgstateSet $pg -isNewplot 0 -icLine 2
       hgPlot $pg $HNSfrac -xmin $mmin -xmax $mmax 
       plot_line $pg 10.0 1.0 30.0 1.0


       endplot $pg

    } msg] {
       echo "$msg"
    }
 
    echo "  done plotting [exec date]"
    # clean
    vListDel [list $matches $orph1 $orph2 $SS $GG $NS]
    vListDel [list $orph1Type $orph2Type $SSo1 $SSo2 $mtypeA $mtypeB] 
    vListDel [list $Mflags $Mflags2 $O1flags $O1flags2 $O2flags $O2flags2]
    vListDel [list $MMOmask $O1MOmask $O2MOmask]
    vListDel [list $MnotMOmask $O1notMOmask $O2notMOmask]
    vListDel [list $Mfield $MfieldB $O1field $O2field $MfieldMask $O1fieldMask $O2fieldMask]  
 
    hgDel $HSS; hgDel $HGG; hgDel $HNS
    hgDel $HSSfrac; hgDel $HGGfrac; hgDel $HNSfrac
    hgDel $LHSS; hgDel $LHGG; hgDel $LHNS 
    hgDel $Hm; hgDel $Ho1; hgDel $Ho2; hgDel $Ho1S; hgDel $Ho2S; 
    hgDel $moHm; hgDel $moHo1; hgDel $moHo2; 
    hgDel $tot1; hgDel $tot2; hgDel $tot1S; hgDel $tot2S
    hgDel $c1; hgDel $c2
    # hgDel $c1S; hgDel $c2S
    hgDel $Lm; hgDel $Lo1; hgDel $Lo2   
    hgDel $LmMO; hgDel $Lo1MO; hgDel $Lo2MO   

  return $outList

}




# given a handle to a histogram, and assuming it's monotonic,
# determine the bin value for which the histogram for the first 
# time becomes larger (type = 1) than $val (or larger for type=2).
# start from binMin 
proc ChistStats {h val type {binMin 17}} {

    set bin [exprGet $h.binPosition<0>]
    set N [exprGet $h.nbin]
    set ok 0; set i 0 

    while {$i < $N && !$ok} {
        incr i
        set bin [exprGet $h.binPosition<$i>]
        if {$bin >= $binMin} {
	  set v [exprGet $h.contents<$i>]
          if {($type == 1 && $v > $val) || $v < $val} {
	     set xL [exprGet $h.binPosition<[expr $i-1]>]
	     set xR [exprGet $h.binPosition<$i>]
             set yL [exprGet $h.contents<[expr $i-1]>]
             set bin [expr $xL + ($xR-$xL)*($val-$yL)/($v-$yL)]
             set ok 1  
          }
        } 
    }

   return $bin
}

# 
# e.g. plot_depth_1col Filter-r-depth-run745-col1VSrun756-col1 r "chip 1r, 30 fields" /VCPS
# PAGE 1
# panel 1: numbers of matched sources and orphans from each run
# panel 2: completeness for each run
# panel 3: numbers of SS, GG, (diff. types) 
# panel 4: photometric errors as a function of magnitude, separately for all and for SS only
# PAGE 2
# panel 1: completeness for each run for all sources (same as panel 2 above)
# panel 2: completeness for each run for STARS only (using type(filter))
proc plot_depth_1col {fileroot filter title device} {

    set filters {u g r i z}
    set band [lsearch $filters $filter]
    if {$band < 0} {return "Filter $filter does not exist"}
    set Nbins 120; set mmin 14.0; set mmax 26.0 
    set NbinsErr 24.0

    ### form vectors 
    # model mags for epoch1 for matched sources
    set Cm [list counts_model $band]
    set matches [lindex [file2Vector $fileroot.par $Cm] 1]
    # model mags for orphans
    set orph1 [lindex [file2Vector $fileroot-orphans1.par $Cm] 1]
    set orph2 [lindex [file2Vector $fileroot-orphans2.par $Cm] 1]

    # flags2 to get rid of moving objects
    set Mflags [lindex [file2Vector $fileroot.par objc_flags] 1]
    set Mflags2 [lindex [file2Vector $fileroot.par objc_flags2] 1]
    set O1flags [lindex [file2Vector $fileroot-orphans1.par objc_flags] 1]
    set O1flags2 [lindex [file2Vector $fileroot-orphans1.par objc_flags2] 1]
    set O2flags [lindex [file2Vector $fileroot-orphans2.par objc_flags] 1]
    set O2flags2 [lindex [file2Vector $fileroot-orphans2.par objc_flags2] 1]

    # masks for moving objects
    set MMOmask [flagsMask $Mflags2 {DEBLENDED_AS_MOVING} {}]
    set O1MOmask [flagsMask $O1flags2 {DEBLENDED_AS_MOVING} {}]
    set O2MOmask [flagsMask $O2flags2 {DEBLENDED_AS_MOVING} {}]

    # N.B. this is not objc_type, it is type(band)
    echo "   getting histograms [exec date]"
    set Ctype objc_type
    set mtypeA [lindex [file2Vector $fileroot.par $Ctype] 1]
    set Ctype objc_typeB
    set mtypeB [lindex [file2Vector $fileroot.par $Ctype] 1]
    set SS [vectorExprEval "$mtypeA == 6 && $mtypeB == 6 ? 1 : 0"]
    set GG [vectorExprEval "$mtypeA == 3 && $mtypeB == 3 ? 1 : 0"]
    set NS [vectorExprEval "$mtypeA != $mtypeB ? 1 : 0"]    
    set HSS [vecToHist $matches $Nbins [list $mmin $mmax] $SS]
    set HGG [vecToHist $matches $Nbins [list $mmin $mmax] $GG]
    set HNS [vecToHist $matches $Nbins [list $mmin $mmax] $NS]
    set LHSS [hgLog10 $HSS]; set LHGG [hgLog10 $HGG] 
    set LHNS [hgLog10 $HNS];
    handleSet $LHSS.xLabel "mmod($filter)"; handleSet $LHSS.yLabel "log10 (N)" 


    # first epoch model photometry for matched sources
    echo "   2 epoch photometry [exec date]"
    set C [list counts_model $band]
    set model [lindex [file2Vector $fileroot.par $C] 1]
    set C [list counts_modelErr $band]
    set err [lindex [file2Vector $fileroot.par $C] 1]
    # second epoch model photometry for matched sources
    set CB [list counts_modelB $band]
    set modelB [lindex [file2Vector $fileroot.par $CB] 1]
    set CB [list counts_modelErrB $band]
    set errB [lindex [file2Vector $fileroot.par $CB] 1]

    # mean mags and the difference
    set mmags [vectorExprEval 0.5*($model+$modelB)]
    set merr [vectorExprEval 0.0001+sqrt($err*$err+$errB*$errB)]
    set mdiff [vectorExprEval $model-$modelB]
    set chi2 [vectorExprEval $mdiff/$merr]    
    # get magnitude bins
    set magbins [vectorExprNew $Nbins]
    vNameSet $magbins "mmod($filter)"
    # aux mask
    set qmask [vectorExprEval "1 == 1 ? 1 : 0"]
    set binwidth [expr ($mmax-$mmin)/$NbinsErr]
    set Halfwidth [expr 0.5*$binwidth]
    handleSet $magbins.vec<0> [expr $mmin + $Halfwidth]
    loop i 1 $Nbins {
       set j [expr $i-1]
       handleSet $magbins.vec<$i> [expr [exprGet $magbins.vec<$j>] + $binwidth]
    }
    #set errorstats [binned_stats $mmags $mdiff $qmask $magbins $Halfwidth 3 0.0]  
    echo "   binned stats [exec date]"
    set errorstatsStars [binned_stats $mmags $mdiff $SS $magbins $Halfwidth 3 0.0]  
    set chi2statsStars [binned_stats $mmags $chi2 $SS $magbins $Halfwidth 3 0.0]  
    set errorstatsGal [binned_stats $mmags $mdiff $GG $magbins $Halfwidth 3 0.0]  
    set chi2statsGal [binned_stats $mmags $chi2 $GG $magbins $Halfwidth 3 0.0]  


    ### get magnitude histograms
    set Hm [vecToHist $matches $Nbins [list $mmin $mmax]]   
    set Ho1 [vecToHist $orph1 $Nbins [list $mmin $mmax]]   
    set Ho2 [vecToHist $orph2 $Nbins [list $mmin $mmax]] 
    handleSet $Hm.xLabel "mmod($filter)"; handleSet $Hm.yLabel  "N matches" 
    handleSet $Ho1.xLabel "mmod($filter)"; handleSet $Ho1.yLabel "N orphans 1"  
    handleSet $Ho2.xLabel "mmod($filter)"; handleSet $Ho2.yLabel "N orphans 2" 
    # orphaned stars
    set Ctype objc_type
    set orph1Type [lindex [file2Vector $fileroot-orphans1.par $Ctype] 1]
    set orph2Type [lindex [file2Vector $fileroot-orphans2.par $Ctype] 1]
    set SSo1 [vectorExprEval "$orph1Type == 6 ? 1 : 0"]
    set SSo2 [vectorExprEval "$orph2Type == 6 ? 1 : 0"]

    ## get aux. histograms
    echo "   aux. histograms [exec date]"
    set tot1 [hgOper $Ho1 + $Hm]
    set c1 [hgOper $Hm / $tot1]
    set tot2 [hgOper $Ho2 + $Hm]
    set c2 [hgOper $Hm / $tot2]
    handleSet $c1.xLabel "mmod($filter)"; handleSet $c1.yLabel "completeness"  
    handleSet $c2.xLabel "mmod($filter)"; handleSet $c2.yLabel ""  
    handleSet $c1.name ""; handleSet $c2.name "";
    # stars separately
    set Ho1S [vecToHist $orph1 $Nbins [list $mmin $mmax] $SSo1]  
    set Ho2S [vecToHist $orph2 $Nbins [list $mmin $mmax] $SSo2] 
    set tot1S [hgOper $Ho1S + $HSS]
    set c1S [hgOper $HSS / $tot1S]
    set tot2S [hgOper $Ho2S + $HSS]
    set c2S [hgOper $HSS / $tot2S]
    handleSet $c1S.xLabel "mmod($filter)"; handleSet $c1S.yLabel "completeness"  
    handleSet $c2S.xLabel "mmod($filter)"; handleSet $c2S.yLabel ""  
    handleSet $c1S.name ""; handleSet $c2S.name "";

    # log10 
    set Lm [hgLog10 $Hm]     
    set Lo1 [hgLog10 $Ho1]     
    set Lo2 [hgLog10 $Ho2]     
    handleSet $Lm.xLabel "mmod($filter)"; handleSet $Lm.yLabel "log10 (N)" 
    handleSet $Lo1.xLabel "mmod($filter)"; handleSet $Lo1.yLabel ""
    handleSet $Lo2.xLabel "mmod($filter)"; handleSet $Lo2.yLabel "" 
    handleSet $Lm.name ""; handleSet $Lo1.name ""; handleSet $Lo2.name ""; 
    echo "   all set, going to plot [exec date]"

    ### plot
    if [catch {

       ## page 1
       set pg [pgstateNew]
       set dev [devicename $device depth_1col.ps]
       pgstateSet $pg -device $dev -lineWidth 3 \
          -xfract 0.3 -yfract 0.4 -nxwindow 2 -nywindow 4
       pgstateOpen $pg
       set icDef [exprGet $pg.icLine]

       # panel 1
       hgPlot $pg $Lm -xmin $mmin -xmax $mmax 
       titlePlot $title 40 
       pgstateSet $pg -isNewplot 0 -icLine 2
       hgPlot $pg $Lo1 -xmin $mmin -xmax $mmax 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $Lo2 -xmin $mmin -xmax $mmax 

       # panel 2
       pgstateSet $pg -isNewplot 1 -icLine $icDef
       hgPlot $pg $LHSS -xmin $mmin -xmax $mmax -ymin 0 -ymax 4.5  
       titlePlot "black: SS, blue: GG, red: diff." 40 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $LHGG -xmin $mmin -xmax $mmax 
       pgstateSet $pg -isNewplot 0 -icLine 2
       hgPlot $pg $LHNS -xmin $mmin -xmax $mmax 

       # panel 3
       pgstateSet $pg -isNewplot 1 -icLine 2
       hgPlot $pg $c1 -xmin $mmin -xmax $mmax 
       titlePlot "All; red: run 1, blue: run 2" 40 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $c2 -xmin $mmin -xmax $mmax
       plot_line $pg 10.0 1.0 30.0 1.0
       ## The black book completeness limits in ugriz
       set BBlimits [list 22.3 23.3 23.1 22.3 20.8]
       set BBlimit [lindex $BBlimits $band]
     #  plot_line $pg $BBlimit -0.05 $BBlimit 1.05 3 3

       # panel 4
       pgstateSet $pg -isNewplot 1 -icLine 2
       hgPlot $pg $c1S -xmin $mmin -xmax $mmax 
       titlePlot "STARS only; 1:red, 2:blue"  40 
       pgstateSet $pg -isNewplot 0 -icLine 4
       hgPlot $pg $c2S -xmin $mmin -xmax $mmax
       plot_line $pg 10.0 1.0 30.0 1.0
     #  plot_line $pg $BBlimit -0.05 $BBlimit 1.05 3 3

     if {0} {
       # panel 5
       pgstateSet $pg -isNewplot 1 -icLine $icDef -isLine 1
       set vector [lindex $errorstatsStars 1]
       handleSet $vector.name "\[mag\]"      
       vPlot $pg $magbins $vector -xmin $mmin -xmax $mmax -ymin -0.25 -ymax 0.25 
       pgstateSet $pg -isNewplot 1 -icLine $icDef -isLine 1
       titlePlot "black: diff; red, blue: sigma" 40 
       set vector [lindex $errorstatsStars 2]
       set vectorAux [vectorExprEval $vector/1.41]
       pgstateSet $pg -isNewplot 0 -icLine 2 
       vPlot $pg $magbins $vectorAux -xmin $mmin -xmax $mmax -ymin -0.25 -ymax 0.25       vListDel [list $vectorAux]      
       set vector [lindex $errorstatsGal 2]
       set vectorAux [vectorExprEval $vector/1.41]
       pgstateSet $pg -isNewplot 0 -icLine 4 
       vPlot $pg $magbins $vectorAux -xmin $mmin -xmax $mmax -ymin -0.25 -ymax 0.25 
       vListDel [list $vectorAux]      

       plot_line $pg 10.0 0.0 30.0 0.0 1 $icDef
       plot_line $pg 14.0 0.05 26.0 0.05 1 3

       # panel 6
       pgstateSet $pg -isNewplot 1 -icLine 2 -isLine 1
       set vector [lindex $chi2statsStars 2]
       vPlot $pg $magbins $vector -xmin $mmin -xmax $mmax -ymin 0.0 -ymax 3.0
       titlePlot "chi2; red: stars. blue: gal." 40 
       pgstateSet $pg -isNewplot 0 -icLine 4 
       set vector [lindex $chi2statsGal 2]
       vPlot $pg $magbins $vector -xmin $mmin -xmax $mmax -ymin 0.0 -ymax 3.0
       plot_line $pg 10.0 1.0 30.0 1.0 1 $icDef
     }

       endplot $pg

    } msg] {
       echo "$msg"
    }
 
    echo "  done plotting [exec date]"
    # clean
    vListDel [list $matches $model $modelB $mmags $mdiff $magbins $orph1 $orph2 $SS $GG $NS]
    vListDel [list $orph1Type $orph2Type $SSo1 $SSo2 $mtypeA $mtypeB $err $errB $merr $chi2] 
    vListDel [list $Mflags $Mflags2 $O1flags $O1flags2 $O2flags $O2flags2]
    vListDel [list $MMOmask $O1MOmask $O2MOmask]
  
    #vListDel $errorstats
    vListDel $errorstatsStars 
    vListDel $chi2statsStars
    vListDel $errorstatsGal
    vListDel $chi2statsGal
    hgDel $HSS; hgDel $HGG; hgDel $HNS 
    hgDel $LHSS; hgDel $LHGG; hgDel $LHNS 
    hgDel $Hm; hgDel $Ho1; hgDel $Ho2; hgDel $Ho1S; hgDel $Ho2S; 
    hgDel $tot1; hgDel $tot2; hgDel $tot1S; hgDel $tot2S
    hgDel $c1; hgDel $c2; hgDel $c1S; hgDel $c2S
    hgDel $Lm; hgDel $Lo1; hgDel $Lo2   

}



# plots errors and chi2 for psf magnitudes for stars
# e.g. plot_photoErrors Filter-r-depth-run745-col1VSrun756-col1 r "chip 1r, 30 fields" /VCPS
proc plot_photoErrors {fileroot filter title device} {

    set filters {u g r i z}
    set band [lsearch $filters $filter]
    if {$band < 0} {return "Filter $filter does not exist"}
    set Nbins 120; set mmin 14.5; set mmax 23.0 
    set NbinsErr 17.0

    ### form vectors 
    # N.B. this is not objc_type, it is type(band)
    set Ctype objc_type
    set mtypeA [lindex [file2Vector $fileroot.par $Ctype] 1]
    set Ctype objc_typeB
    set mtypeB [lindex [file2Vector $fileroot.par $Ctype] 1]
    set SS [vectorExprEval "$mtypeA == 6 && $mtypeB == 6 ? 1 : 0"]
    set GG [vectorExprEval "$mtypeA == 3 && $mtypeB == 3 ? 1 : 0"]

    # first epoch psf photometry for matched sources
    set C [list psfCounts $band]
    set psf [lindex [file2Vector $fileroot.par $C] 1]
    set C [list psfCountsErr $band]
    set err [lindex [file2Vector $fileroot.par $C] 1]
    # second epoch psf photometry for matched sources
    set CB [list psfCountsB $band]
    set psfB [lindex [file2Vector $fileroot.par $CB] 1]
    set CB [list psfCountsErrB $band]
    set errB [lindex [file2Vector $fileroot.par $CB] 1]
    # mean mags and the difference
    set mmags [vectorExprEval 0.5*($psf+$psfB)]
    set merr [vectorExprEval 0.0001+sqrt($err*$err+$errB*$errB)]
    set mdiff [vectorExprEval $psf-$psfB]
    set chi2 [vectorExprEval $mdiff/$merr]    
    # get magnitude bins
    set magbins [vectorExprNew $Nbins]
    vNameSet $magbins "mpsf($filter)"
    # aux mask
    set binwidth [expr ($mmax-$mmin)/$NbinsErr]
    set Halfwidth [expr 0.5*$binwidth]
    handleSet $magbins.vec<0> [expr $mmin + $Halfwidth]
    loop i 1 $Nbins {
       set j [expr $i-1]
       handleSet $magbins.vec<$i> [expr [exprGet $magbins.vec<$j>] + $binwidth]
    }

    set errorstatsStars [binned_stats $mmags $mdiff $SS $magbins $Halfwidth 3 0.0]  
    set chi2statsStars [binned_stats $mmags $chi2 $SS $magbins $Halfwidth 3 0.0] 


    ### plot
    if [catch {

       ## page 1
       set pg [pgstateNew]
       set dev [devicename $device mpsf_errors.ps]
       pgstateSet $pg -device $dev -lineWidth 3 -isLine 1 \
          -xfract 0.3 -yfract 0.35 -nxwindow 1 -nywindow 4
       pgstateOpen $pg
       set icDef [exprGet $pg.icLine]

       # N stars
       set vector [lindex $errorstatsStars 0]
       vNameSet $vector "N stars / bin"
       vPlot $pg $magbins $vector -xmin $mmin -xmax $mmax -ymin 0
       titlePlot $title 40 

       # error
       set vector [lindex $errorstatsStars 2]
       set vectorAux [vectorExprEval $vector/1.41]
       vNameSet $vectorAux "error (mag)"
       vPlot $pg $magbins $vectorAux -xmin $mmin -xmax $mmax -ymin 0.0 -ymax 0.3
       vListDel [list $vectorAux]      
       plot_line $pg 15.0 0.05 23.0 0.05
       plot_line $pg 10.0 0.0 30.0 0.0

       # chi2
       pgstateSet $pg -isNewplot 1
       set vector [lindex $chi2statsStars 2]
       vNameSet $vector "chi2"
       vPlot $pg $magbins $vector -xmin $mmin -xmax $mmax -ymin 0.0
       plot_line $pg 10.0 1.0 30.0 1.0

       endplot $pg

    } msg] {
       echo "$msg"
    }

    # clean
    vListDel [list $psf $psfB $mmags $mdiff $magbins $SS $GG]
    vListDel [list $mtypeA $mtypeB] 
    vListDel $chi2statsStars; vListDel $errorstatsStars; 


}



# given a histogram, return a copy of it with log contents  
proc hgLog10 {hg} {

    set hgLog [hgNew]
    hgDefine $hgLog -nbin [exprGet $hg.nbin]
    foreach t {id name xLabel yLabel minimum maximum} {
    handleSet $hgLog.$t [exprGet $hg.$t]
    }
    loop i 0 [exprGet $hg.nbin] { 
    handleSet $hgLog.binPosition<$i> [exprGet $hg.binPosition<$i>]
        set aux [expr 1.0 + [exprGet $hg.contents<$i>]]
        handleSet $hgLog.contents<$i> [expr log10($aux)]
    } 
  
    return $hgLog

}


###################################################################
# primitive  procs:

# structure which holds photometry and other info for a matched source
typedef struct {
    int matchID;
    int run;
    int camCol;
    int field;
    int id;
    float objc_rowc;
    float objc_colc;
    double ra;
    double dec;
    int objc_type;
    int objc_flags;
    int objc_flags2;
    float psfCounts[5];
    float psfCountsErr[5];
    float counts_model[5];
    float counts_modelErr[5];
    float reddening[5];
    int runB;
    int camColB;
    int fieldB;
    int idB;
    float objc_rowcB;
    float objc_colcB;
    double raB;
    double decB;
    int objc_typeB;
    int objc_flagsB;
    int objc_flags2B;
    float psfCountsB[5];
    float psfCountsErrB[5];
    float counts_modelB[5];
    float counts_modelErrB[5];
    float reddeningB[5];
} PHOTOMATCH;

# structure which holds photometry for a selected source
typedef struct {
    int localID;
    int run;
    int camCol;
    int field;
    int id;
    int parent;
    int nchild;
    float objc_rowc;
    float objc_colc;
    double ra;
    double dec;
    int objc_type;
    int objc_flags;
    int objc_flags2;
    int type[5];
    float psfCounts[5];
    float psfCountsErr[5];
    float petroCounts[5];
    float petroCountsErr[5];
    float petroR50[5];
    float petroR50Err[5];
    float counts_model[5];
    float counts_modelErr[5];
    float star_L[5];
    float exp_L[5];
    float deV_L[5];
    float reddening[5];
    float rowv; 
    float rowvErr; 
    float colv; 
    float colvErr; 
    int propermotionmatch;
    float propermotiondelta;
    float propermotion;
    float propermotionangle;
    float usnoBlue;
    float usnoRed;
    int firstMatch;
    int rosatMatch;
    int primTarget;
    int secTarget;
} PHOTOMETRY;


### Match specified tsObj files and dump PHOTOMATCHs to a *.par file
# Matching is done positionally within a square of a side 2*match_size,
# with match_size given in seconds
# When one_run = 0 only objects with 0 <= row <= 1361 from the first
# run are considered, and the preceeding field from the second run
# is also matched. For one_run = 1, all objects from the first run are
# considered, and only one field from the second run is matched (with a
# field number larger for one, of course).
# Matched objects must have set on all the flags specified in the list 
# flagsON, while all the flags specified in the list flagsOFF must be
# off. Also, object type must be equal to obj_type (3=galaxy, 6=stars,
# both forms allowed, or when ignoring objects' type set it to either
# -1 or "all"
# If orphans != 0 then all unmatched sources will be dumped to a par
# files with structure PHOTOMETRY $outfile-orphans1.par and 
# $outfile-orphans2.par, for each run respectively.
#
# N.B. If runs are offset in field (row) direction the lower left corner
# (row=0,col=0) of startfield2 for run2 must be within the boundaries of 
# run1's field specified by startfield1
# This runs with the speed of ~5 sec/frame, not optimized
# to get valid obj. flags execute "parray OBJECT1", for obj.types 
# execute "parray OBJ_TYPE"
proc match_tsObj {run1 rerun1 col1 startfield1 run2 rerun2 col2 startfield2 nfields \
                  tsObjdir1 tsObjdir2 outfile  match_size {one_run 0} {flags_ON {}} \
                  {flags_OFF {}} {obj_type "all" } {log_file match_tsObj.log} \
                  {orphans 0} {rmax 30} {orphON {}} {orphOFF {}} } {
 
global OBJECT1
global OBJ_TYPE

    # for  memory reporting
    set startMem [memNextSerialNumber]

    # start log file
    set logf [match_log_file $log_file $run1 $rerun1 $col1 $startfield1 \
              $run2 $rerun2 $col2 $startfield2 $nfields $tsObjdir1 \
              $tsObjdir2 $outfile $match_size $flags_ON $flags_OFF $obj_type]

    # start output file
    # first some aux information
    set outf [open $outfile.par "w"]
    if {$one_run} { 
       puts $outf \
       "# Frame-to-frame overlap sources from run $run1, rerun $rerun1"
    } else {
       puts $outf "# Matched sources for $nfields fields from"
       puts $outf "# run1 = $run1, rerun1 $rerun1, starting field $startfield1"
       puts $outf "# run2 = $run2, rerun2 $rerun2, starting field $startfield2"
    }
    puts $outf "#"
    close $outf
    # write out PHOTOMATCH structure definition
    set hdr ""
    set chain [chainNew PHOTOMATCH]
    chain2Param $outfile.par $chain $hdr -append
    chainDestroy $chain genericDel
    # are we dumping orphans?
    if {$orphans} {
        set chain [chainNew PHOTOMETRY]
        set of1 $outfile-orphans1.par
        set of2 $outfile-orphans2.par
        chain2Param $of1 $chain $hdr -append
        chain2Param $of2 $chain $hdr -append
        chainDestroy $chain genericDel
    }

    # coordinates in tsObj files are in degrees
    set match_size [expr $match_size/3600.0]

    # translate flags and obj_type
    set flagsON [translate_flags $flags_ON]   
    set flagsOFF [translate_flags $flags_OFF]   
    if {$obj_type == "STAR" || $obj_type == "GALAXY"} {
       set obj_type [expr $OBJ_TYPE($obj_type)]
    }
    
    echo ""; echo " -------------- MATCHING $nfields fields  --------------" 
    if {$orphans} {
        echo " field        N1       N2      Nmatch    Ngood     Norph1    Norph2" 
        puts $logf "# field       N1       N2      Nmatch    Ngood     Norph1    Norph2" 
    } else {
        echo " field        N1       N2      Nmatch    Ngood" 
        puts $logf "# field       N1       N2      Nmatch    Ngood" 
    }


    # for total counts
    set N1tot 0
    set N2tot 0
    set NmatchTot 0
    set NgoodTot 0
   
    # there is no "old" chain2 of TSOBJs yet, so make an empty one
    if {!$one_run} {set chn2old [chainNew TSOBJ]}
    
    loop i 0 $nfields {
     
        if {[expr $i/100*100] == $i} {
            write_mem mem "run1 $run1 column1 $col1, field $i"
        }

        # read chain for epoch 1
        set tsObj [get_tsObj_name $tsObjdir1 $run1 $rerun1 $col1 $startfield1 $i]
        set chn1 [tsObj2chain $tsObj]
    set chn1_size [chainSize $chn1]
        # read chain for epoch 2
        set tsObj [get_tsObj_name $tsObjdir2 $run2 $rerun2 $col2 $startfield2 $i]
        set chn2 [tsObj2chain $tsObj]
    set chn2_size [chainSize $chn2]

        # add sources from the previous field in run2 
        # (n.b. this does away with chn2old)
        if {!$one_run} {chainJoin $chn2 $chn2old}

 
        ### positional matching ######
        set match_condition "{lambda lambda $match_size} {eta eta $match_size}"
    set matchchn [chainMatch $chn1 $chn2 $match_condition]
    set Nmatch [chainSize $matchchn]
        ##############################

    # This loops over all of the matched elements and dumps them to the 
        # output file. The return value is the number of good elements.
        set outf $outfile.par
        set Ngood [dump_good_matches $matchchn $NgoodTot $one_run $obj_type \
                                     $flagsON $flagsOFF $logf $outf] 
        # are we dumping orphans?
        if {$orphans} {
            # clean chains to speed it up
        set clean1 [clean_tsObj_chain $chn1 r 14.0 $rmax g r -10 10 10 10 $orphON $orphOFF]  
        set clean2 [clean_tsObj_chain $chn2 r 14.0 $rmax g r -10 10 10 10 $orphON $orphOFF]  
            set Norph [dump_orphans $matchchn $clean1 $clean2 $match_size $of1 $of2 $logf \
                $one_run $obj_type]
            set Norph1 [lindex $Norph 0]; set Norph2 [lindex $Norph 1]
            chainDel $clean1; chainDel $clean2
        }
 

        # some statistics and counting
        set Nmatch [format "%8d" $Nmatch]
        set Ngood [format "%8d" $Ngood]
        set chn1_size [format "%8d" $chn1_size]  
        set chn2_size [format "%8d" $chn2_size]  
        if {$orphans} {
            set text " [format "%4d" $i]   $chn1_size $chn2_size   $Nmatch $Ngood         $Norph1       $Norph2"
        } else {
            set text " [format "%4d" $i]   $chn1_size $chn2_size   $Nmatch $Ngood"
        }
        echo $text
        puts $logf $text 
        incr N1tot $chn1_size 
        incr N2tot $chn2_size 
        incr NmatchTot $Nmatch
        incr NgoodTot $Ngood
        
        if {!$one_run} {
           # copy epoch 2 elements for matching with the next field
       set chn2old [chainNew TSOBJ]
       loop i 0 $chn2_size {
          set elem [chainElementRemByPos $chn2 0]
          chainElementAddByPos $chn2old $elem TAIL AFTER
       }
        }

        # clean   
        genericChainDestroy $matchchn
        genericChainDestroy $chn1
        genericChainDestroy $chn2
    }
    if {!$one_run} {genericChainDestroy $chn2old} 


    set NmatchTot [format "%8d" $NmatchTot]
    set NgoodTot [format "%8d" $NgoodTot]   
    set N1tot [format "%8d" $N1tot]   
    set N2tot [format "%8d" $N2tot]   
    set text " Total: $N1tot $N2tot   $NmatchTot $NgoodTot"
    echo $text
    echo " ------------------------------------------------"
    puts $logf $text

    puts $logf "# finished on [exec date]"
    close $logf

    # check for memory leaks
    # assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}

}

proc translate_flags {flags} {

global OBJECT1 OBJECT2 

    set aux 0
    foreach flag $flags {
        if {[catch {set aux [expr $aux | $OBJECT1($flag)]}]} {
            catch {set aux [expr $aux | $OBJECT2($flag)]}
        }
    }

    return $aux

} 


### start log file to keep track of time, number of sources, etc.
proc match_log_file {log_file run1 rerun1 col1 startfield1 run2 rerun2 \
                     col2 startfield2 nfields tsObjdir1 tsObjdir2 \
                     outfile match_size flagsON flagsOFF obj_type} {
    
    set fd [open $log_file "w"]

    puts $fd "# *** LOG FILE from match_tsObj ***"
    puts $fd \
         "# match of run $run1 rerun $rerun1 col $col1 starting field $startfield1 vs. "
    puts $fd "#          run $run2 rerun $rerun2 col $col2 starting field $startfield2\
              for $nfields fields"
    puts $fd "# other specs: matching box half size (arcsec): $match_size"
    puts $fd "# flagsON: $flagsON"   
    puts $fd "# flagsOFF: $flagsOFF"   
    puts $fd "# requested Photo object type: $obj_type"
    puts $fd "# ------------------------------------------------------------------------"
    puts $fd "# started on [exec date]"
    puts $fd "#"

    return $fd
}


### For specified tsObj files dump PHOTOMETRYs to a *.par file
# User can select maximum mod(r) magnitude (rmodMax)
# When no_doubles = 1 only objects with 64 <= row <= 1425 are considered.
# Selected objects must have set on all the flags specified in the list 
# flagsON, while all the flags specified in the list flagsOFF must be
# off. Also, object type must be equal to obj_type (3=galaxy, 6=stars,
# STAR, GALAXY form also allowed, or when ignoring objects' type set 
# it to either -1 or "all"
# This runs with the speed of ~20 sec/frame, not optimized
# N.B. to get valid obj. flags execute "parray OBJECT1", for obj.types 
# execute "parray OBJ_TYPE"
proc dump_tsObj {outfile run rerun col {startfield -1} {endfield -1} \
                 {flags_ON {}} {flags_OFF {}} {obj_type "all" } \
		 {rmodMax 30} {no_doubles 1} {log_file dump_tsObj.log} } {
 
global OBJECT1 OBJECT2
global OBJ_TYPE
global openit data_root
global kbo

    # for  memory reporting
    set startMem [memNextSerialNumber]

    # get tsObjdir 
    set_run $run $col -tsObj -rerun $rerun -rootdir $data_root 
    set tsObjdir $openit(objdir)

    if {$startfield < 0} {
        set startfield [lindex [find_field_range $tsObjdir] 0]
    }
    if {$endfield < 0} {
        set endfield [lindex [find_field_range $tsObjdir] 1]
    }
    set nfields [expr $endfield - $startfield +1]
       
    # start log file
    set logf [dump_log_file $log_file $run $rerun $col $startfield \
              $endfield $tsObjdir $outfile $flags_ON $flags_OFF $obj_type $rmodMax]


    # start output file
    # first some aux information
    set outf [open $outfile.par "w"]
    puts $outf "# Dumped sources from run $run, rerun $rerun"
    puts $outf "#"
    close $outf
    # write out PHOTOMETRY structure definition
    set hdr ""
    set chain [chainNew PHOTOMETRY]
    chain2Param $outfile.par $chain $hdr -append
    chainDestroy $chain genericDel

    # split flags according to OBJECT1 vs. OBJECT2 flags
    # all possible flags
    set OBJ1flags [array names OBJECT1]
    set OBJ2flags [array names OBJECT2]
    # get subsets of flagsON and flagsOFF that correspond to OBJ1 and OBJ2
    set flags_ON1 {}
    set flags_ON2 {}
    set flags_OFF1 {}
    set flags_OFF2 {}

    foreach f $flags_ON {
	if {[lsearch $OBJ1flags $f] >= 0} {
           lappend flags_ON1 $f
        }
	if {[lsearch $OBJ2flags $f] >= 0} {
           lappend flags_ON2 $f
        }        
    }
    foreach f $flags_OFF {
	if {[lsearch $OBJ1flags $f] >= 0} {
           lappend flags_OFF1 $f
        }
	if {[lsearch $OBJ2flags $f] >= 0} {
           lappend flags_OFF2 $f
        }        
    }

    # translate flags and obj_type
    set flagsOFF1 [translate_flags $flags_OFF1]
    set flagsON1 [translate_flags $flags_ON1]
    set flagsOFF2 [translate_flags $flags_OFF2]
    set flagsON2 [translate_flags $flags_ON2]
    if {$obj_type == "STAR" || $obj_type == "GALAXY"} {
       set obj_type [expr $OBJ_TYPE($obj_type)]
    }

    echo ""; echo "  --  DUMPING $nfields fields for col. $col  --"
    echo " field        N    Nbright     Ngood" 
    puts $logf "# field        N     Nbright     Ngood" 
  
    # for counts
    set Ntot 0; set Nbright 0; set NgoodTot 0
      

    ### loop over all fields 
    loop i 0 $nfields {
     
        if {[expr $i/100*100] == $i} {
            write_mem mem "run $run column $col, field $i"
        }

        # read chain from this tsObj file
        set tsObj [get_tsObj_name $tsObjdir $run $rerun $col $startfield $i]
        if {[file exists $tsObj]} {
             set chain [tsObj2chain $tsObj]
        } else {
             set text " ** File $tsObj does NOT exist !?!?"
             echo $text
             puts $logf $text 
             continue     
        }

        # This loops over all objects and dumps the "good" ones
        # to the output file. The return value is the number of good elements.
        set outf $outfile.par
        set chainBright [chainSearch $chain "{counts_model<2> <= $rmodMax}"]

        ### block for KBOs 
	if {[info exist kbo] && $kbo} {
            # first select sources without POSS match within 30 arcsec
	    set orph1 [chainSearch $chainBright "{propermotionmatch == 0}"]
            # and then w/o match within 3 arcsec
	    set aux1 [chainSearch $chainBright "{propermotionmatch == 1}"]
	    set orph2 [chainSearch $aux1 "{propermotiondelta > 3.0}"]
            chainDel $aux1
            # concatenate chains
            chainDel $chainBright
            set chainBright $orph1
            chainJoin $chainBright $orph2
        } 
        catch {
           set Ngood [dump_good_sources $chainBright $NgoodTot $obj_type \
                              $flagsON1 $flagsOFF1 $flagsON2 $flagsOFF2 $no_doubles $logf $outf] 
	}
        # some statistics and counting
        set Ngood [format "%8d" $Ngood]
        set chn_size [format "%8d" [chainSize $chain]]  
        set chn_sizeBright [format "%8d" [chainSize $chainBright]]  
        set text " [format "%4d" $i]   $chn_size  $chn_sizeBright  $Ngood"
        echo $text
        puts $logf $text 
        incr Ntot $chn_size 
        incr Nbright $chn_sizeBright 
        incr NgoodTot $Ngood
        
        # clean   
        chainDel $chainBright
        genericChainDestroy $chain
    }

    set NgoodTot [format "%8d" $NgoodTot]
    set Nbright [format "%8d" $Nbright]  
    set Ntot [format "%8d" $Ntot]   
    set text " Total: $Ntot  $Nbright  $NgoodTot"
    echo $text
    echo " ------------------------------------------------"
    puts $logf $text

    puts $logf "# finished on [exec date]"
    close $logf

    # check for memory leaks
    assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}

}




### For a list of tsObj files dump PHOTOMETRYs to a *.par file
# User can select maximum mod(r) magnitude (rmodMax)
# When no_doubles = 1 only objects with 64 <= row <= 1125 are considered.
# Selected objects must have set on all the flags specified in the list 
# flagsON, while all the flags specified in the list flagsOFF must be
# off. Also, object type must be equal to obj_type (3=galaxy, 6=stars,
# STAR, GALAXY form also allowed, or when ignoring objects' type set 
# it to either -1 or "all"
# N.B. to get valid obj. flags execute "parray OBJECT1", for obj.types 
# execute "parray OBJ_TYPE"
proc dump_tsObjFileList {outfile tsObjFileList \
                 {flags_ON {}} {flags_OFF {}} {obj_type "all" } \
		 {rmodMax 30} {no_doubles 1} {log_file dump_tsObj.log} } {
 
global OBJECT1 OBJECT2
global OBJ_TYPE
global openit data_root

    # for  memory reporting
    set startMem [memNextSerialNumber]
    
    foreach type {run rerun col startfield endfield tsObjdir nfields} { 
       set $type "dummy"
    }    
    # start log file
    set logf [dump_log_file $log_file $run $rerun $col $startfield \
              $endfield $tsObjdir $outfile $flags_ON $flags_OFF $obj_type $rmodMax]


    # start output file
    # first some aux information
    set outf [open $outfile.par "w"]
    puts $outf "# Dumped sources from run $run, rerun $rerun"
    puts $outf "#"
    close $outf
    # write out PHOTOMETRY structure definition
    set hdr ""
    set chain [chainNew PHOTOMETRY]
    chain2Param $outfile.par $chain $hdr -append
    chainDestroy $chain genericDel

    # split flags according to OBJECT1 vs. OBJECT2 flags
    # all possible flags
    set OBJ1flags [array names OBJECT1]
    set OBJ2flags [array names OBJECT2]
    # get subsets of flagsON and flagsOFF that correspond to OBJ1 and OBJ2
    set flags_ON1 {}
    set flags_ON2 {}
    set flags_OFF1 {}
    set flags_OFF2 {}

    foreach f $flags_ON {
	if {[lsearch $OBJ1flags $f] >= 0} {
           lappend flags_ON1 $f
        }
	if {[lsearch $OBJ2flags $f] >= 0} {
           lappend flags_ON2 $f
        }        
    }
    foreach f $flags_OFF {
	if {[lsearch $OBJ1flags $f] >= 0} {
           lappend flags_OFF1 $f
        }
	if {[lsearch $OBJ2flags $f] >= 0} {
           lappend flags_OFF2 $f
        }        
    }

    # translate flags and obj_type
    set flagsOFF1 [translate_flags $flags_OFF1]
    set flagsON1 [translate_flags $flags_ON1]
    set flagsOFF2 [translate_flags $flags_OFF2]
    set flagsON2 [translate_flags $flags_ON2]
    if {$obj_type == "STAR" || $obj_type == "GALAXY"} {
       set obj_type [expr $OBJ_TYPE($obj_type)]
    }

    echo ""; echo "  --  DUMPING from tsObj files  --"
    echo "       N    Nbright     Ngood" 
    puts $logf "#      N     Nbright     Ngood" 
  
    # for counts
    set Ntot 0; set Nbright 0; set NgoodTot 0
      

    ### loop over all fields 
    foreach tsObj $tsObjFileList {
     
        if {1} {
           write_mem mem "file $tsObj"
        }

        # read chain from this tsObj file
        if {[file exists $tsObj]} {
             set chain [tsObj2chain $tsObj]
        } else {
             set text " ** File $tsObj does NOT exist !?!?"
             echo $text
             puts $logf $text 
             continue     
        }

        # This loops over all objects and dumps the "good" ones
        # to the output file. The return value is the number of good elements.
        set outf $outfile.par
        set chainBright [chainSearch $chain "{counts_model<2> <= $rmodMax}"]
        catch {
           set Ngood [dump_good_sources $chainBright $NgoodTot $obj_type \
                              $flagsON1 $flagsOFF1 $flagsON2 $flagsOFF2 $no_doubles $logf $outf] 
	}
        # some statistics and counting
        set Ngood [format "%8d" $Ngood]
        set chn_size [format "%8d" [chainSize $chain]]  
        set chn_sizeBright [format "%8d" [chainSize $chainBright]]  
        set text " $chn_size  $chn_sizeBright  $Ngood  $tsObj"
        echo $text
        puts $logf $text 
        incr Ntot $chn_size 
        incr Nbright $chn_sizeBright 
        incr NgoodTot $Ngood
        
        # clean   
        chainDel $chainBright
        genericChainDestroy $chain
    }

    set NgoodTot [format "%8d" $NgoodTot]
    set Nbright [format "%8d" $Nbright]  
    set Ntot [format "%8d" $Ntot]   
    set text " Total: $Ntot  $Nbright  $NgoodTot"
    echo $text
    echo " ------------------------------------------------"
    puts $logf $text

    puts $logf "# finished on [exec date]"
    close $logf

    # check for memory leaks
    assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}

}




# similar to dump_tsObj, but it operates on a single tsObj file (e.g. obtained
# by matching procs)
proc tsObj2par {tsObjfile outroot {obj_type "all"}  \
            {flags_OFF {BRIGHT SATUR BLENDED EDGE}} {flags_ON {}} } { 

global OBJECT1 OBJ_TYPE

      # dump PHOTOMETRY structure definition to output par file
      set outf [open $outroot.par "w"]
      puts $outf "# Dumped sources from $tsObjfile"
      puts $outf "#"
      close $outf
      set hdr ""
      set chain [chainNew PHOTOMETRY]
      chain2Param $outroot.par $chain $hdr -append
      chainDestroy $chain genericDel
  
      # translate flags and objc_type
      set flagsON 0
      foreach flag $flags_ON {
         set flagsON [expr $flagsON | $OBJECT1($flag)] 
      }
      set flagsOFF 0
      foreach flag $flags_OFF {
         set flagsOFF [expr $flagsOFF | $OBJECT1($flag)] 
      } 
      if {$obj_type == "STAR" || $obj_type == "GALAXY"} {
         set obj_type [expr $OBJ_TYPE($obj_type)]
      }

      # read objects and dump
      set chain [tsObj2chain $tsObjfile]
      # we want only primary elements
      set prim [get_primaries $chain]        
      set logf [open $outroot.log "w"]
      # dumping...
      set Ngood [dump_good_sources $prim 0 $obj_type $flagsON $flagsOFF \
                                    1 $logf $outroot.par] 

      # clean & close
      chainDel $prim
      chainDestroy $chain genericDel      
      close $logf

      echo "dumped $Ngood sources to file $outroot.par"

}



### start log file to keep track of time, number of sources, etc.
proc dump_log_file {log_file run rerun col startfield endfield \
			tsObjdir outfile flagsON flagsOFF obj_type {rmodMax 30}} {
    
    set fd [open $log_file "w"]

    puts $fd "# *** LOG FILE from dump_tsObj ***"
    puts $fd "# dump of run $run, rerun $rerun, col $col, starting fields\
                $startfield-$endfield"
    puts $fd "# other specs:" 
    puts $fd "# mod(r) <= $rmodMax"
    puts $fd "# flagsON: $flagsON"   
    puts $fd "# flagsOFF: $flagsOFF"   
    puts $fd "# requested Photo object type: $obj_type"
    puts $fd "# ------------------------------------------------------------------------"
    puts $fd "# started on [exec date]"
    puts $fd "#"

    return $fd
}


# return the full path to a tsObj file for given specificators 
# if rerun < 0, sniff around given directory and find proper rerun
proc get_tsObj_name {tsObjdir run rerun col startfield field0} {
    
    # if rerun < 0 find the rerun by looking at the files in tsObjdir
    if {$rerun < 0} {
        set testfile [file tail [lindex [glob $tsObjdir/tsObj*fit*] 0]]
        set rerun [string index $testfile [expr [string length $testfile] - 10]]
    }  

    set field [expr $startfield + $field0]
    set filename tsObj-[format %06d $run]-$col-$rerun-[format %04d $field].fit
    
    return $tsObjdir/$filename

}

proc dump_good_matches {matchchn id0 one_run obj_type flagsON  \
                        flagsOFF logfile outfile} { 


     # This loops over all of the matched elements, checks that 
     # the flags are set/not set as requested, and dumps good ones 
     # to the output file
     set good_chain [chainNew PHOTOMATCH]
     set cursor [chainCursorNew $matchchn]
     set Ngood 0

     # which fields do we want copied over? 
     # as defined in PHOTOMATCH
     set Pfieldsaux [schemaGetFromType PHOTOMATCH]
     set Pfields {}
     foreach el $Pfieldsaux {   
        set el [lindex $el 0]
        lappend Pfields $el
     }
     # from TSOBJ
     set Tfieldsaux [schemaGetFromType TSOBJ]
     set Tfields {}
     foreach el $Tfieldsaux {   
        set el [lindex $el 0]
        lappend Tfields $el
     }
     # this could be done in a more general way...
     set chromatic [list psfCounts psfCountsErr counts_model  \
                         counts_modelErr reddening]

     while {[set match [chainWalk $matchchn $cursor]] != ""} { 
        if [good_match $match $one_run $obj_type $flagsON $flagsOFF] {   
            # this is a good match, copy needed fields to output structure
            set good_match [genericNew PHOTOMATCH]
            incr Ngood
            # first set overall counter
            handleSet $good_match.matchID [expr $id0 + $Ngood] 
            # copy over all the fields that exist
            foreach el $Pfields {
                # is this B epoch?
                set n [expr [string length $el]-1]
                if {[string range $el $n $n] == "B"} {
                    set record [string range $el 0 [expr $n-1]] 
                    set epoch 2
                } else {
                    set record $el
                    set epoch 1
                }
                if {[lsearch $Tfields $record] >= 0} {
                    # this field exists, treat specially chromatic fields
                    if {[lsearch $chromatic $record] >= 0} {
                       loop j 0 5 {
                          set rec "$record<$j>"
                          set aux [readSTRUCT TSOBJ $match $epoch $rec]
                          handleSet $good_match.$el<$j> [format "%6.3f" $aux]
                       }
            } else {
                       set aux [readSTRUCT TSOBJ $match $epoch $record]
                       handleSet $good_match.$el $aux
                    }
                }
            } 
            # all set, add it to the chain
       chainElementAddByPos $good_chain $good_match
    }
     }
       
     # dump the chain of good matches to the output file
     set Ngood [chainSize $good_chain]
     chain2Param $outfile $good_chain "" -append -notypedump

     # clean up   
     chainCursorDel $matchchn $cursor
     chainDestroy $good_chain genericDel
   
     return $Ngood

}

      

# given a matched chain obtained by matching chains chain1 and chain2,
# dump structure PHOTOMETRY for all unmatched sources (orphans) which
# satisfy (obj_type, flagsON, flagsOFF) conditions to files file1 and 
# file2, respectively.
proc dump_orphans {matchchn chain1 chain2 msize file1 file2 logf \
                   one_run obj_type} { 

     # lambda-eta limits
     set lambdaMin 360.0; set lambdaMax -360.0
     set etaMin 360.0; set etaMax -360.0
     set cursor [chainCursorNew $matchchn]
     while {[set match [chainWalk $matchchn $cursor]] != ""} { 
         set aux [readSTRUCT TSOBJ $match 1 lambda]
         if {$aux > $lambdaMax} {set lambdaMax $aux}
         if {$aux < $lambdaMin} {set lambdaMin $aux}
         set aux [readSTRUCT TSOBJ $match 1 eta]
         if {$aux > $etaMax} {set etaMax $aux}
         if {$aux < $etaMin} {set etaMin $aux}
     }
     chainCursorDel $matchchn $cursor

     set cond "{$lambdaMin <= lambda <= $lambdaMax} {$etaMin <= eta <= $etaMax}"
     set select1 [chainSearch $chain1 $cond]
     set select2 [chainSearch $chain2 $cond]

     # chains select1 and select2 contain sources from the overlap area
     # now go through each chain and find all good orphans
     set orphans1 [get_orphans $matchchn 1 $select1 $msize $one_run $obj_type] 
     set orphans2 [get_orphans $matchchn 2 $select2 $msize $one_run $obj_type] 
      
     # dump good orphans to output files
     chain2Param $file1 $orphans1 "" -append -notypedump
     chain2Param $file2 $orphans2 "" -append -notypedump

     set Norph1 [chainSize $orphans1]; set Norph2 [chainSize $orphans2] 

     chainDel $select1; chainDel $select2
     genericChainDestroy $orphans1; genericChainDestroy $orphans2; 

     return [list $Norph1 $Norph2]

}



# aux. structure which holds (ra, dec) for a matched source
typedef struct {
    double lambda;
    double eta;
} auxPos;


## given a chain of matches and one of the matched chains, find all orphans which
## satisfy (obj_type, flagsON and flagsOFF)
proc get_orphans {matches epoch chain msize one_run obj_type} { 

global fband  

     set filters {u g r i z}

     # first get a chain of (RA, Dec) for all matches
     set matchPos [chainNew auxPos]
     loop i 0 [chainSize $matches] {
        set match [chainElementGetByPos $matches $i]
    set new [genericNew auxPos]
        handleSet $new.lambda [readSTRUCT TSOBJ $match $epoch lambda]
        handleSet $new.eta [readSTRUCT TSOBJ $match $epoch eta]
        chainElementAddByPos $matchPos $new TAIL AFTER
     }     

     # which fields do we want copied over? 
     # as defined in PHOTOMETRY
     set Pfieldsaux [schemaGetFromType PHOTOMETRY]
     set Pfields {}
     foreach el $Pfieldsaux {   
        set el [lindex $el 0]
        lappend Pfields $el
     }
     # from TSOBJ
     set Tfieldsaux [schemaGetFromType TSOBJ]
     set Tfields {}
     foreach el $Tfieldsaux {   
        set el [lindex $el 0]
        lappend Tfields $el
     }
     # this could be done in a more general way...
     set chromatic [list psfCounts psfCountsErr counts_model counts_modelErr petroCounts \
                    petroCountsErr petroR50 petroR50Err reddening type star_L exp_L deV_L]

     set orphans [chainNew PHOTOMETRY]

     set Ngood 0
     set Nall 0
     set cursor [chainCursorNew $chain]
     while {[set source [chainWalk $chain $cursor]] != ""} { 
        incr Nall
        if [good_orphan $matchPos $epoch $source $msize $one_run $obj_type] {  
            # this is a good orphan, copy needed fields to output structure
            set orphan [genericNew PHOTOMETRY]
            incr Ngood
            # fake overall counter (offset is simply 1000*field)
            handleSet $orphan.localID [expr $Ngood + 1000*[readSTRUCT TSOBJ $source 0 field]]
            # copy over all the fields that exist 
            foreach el $Pfields {
                # is this B epoch?
                set n [expr [string length $el]-1]
                if {[string range $el $n $n] == "B"} {
                    set record [string range $el 0 [expr $n-1]] 
                    set epoch 2
                } else {
                    set record $el
                    set epoch 1
                }
                if {[lsearch $Tfields $record] >= 0} {
                    # this field exists, treat specially chromatic fields
                    if {[lsearch $chromatic $record] >= 0} {
                       loop j 0 5 {
                          set rec "$record<$j>"
                          set aux [readSTRUCT TSOBJ $source 0 $rec]
              if {[catch {set aux [format "%6.3f" $aux]}]} {
                              set aux "-9.999"
                          }
                          handleSet $orphan.$el<$j> $aux
                       }
            } else {
                       set aux [readSTRUCT TSOBJ $source 0 $record]
                       handleSet $orphan.$el $aux
                       # special case for a single band search
               if {[info exist fband]} {
                           set iband [lsearch $filters $fband]
                           if {$record == "objc_type" && $iband > -1} {
                               set aux [readSTRUCT TSOBJ $source 0 type<$iband>]
                               handleSet $orphan.$el $aux
                           }
                           if {$record == "objc_flags" && $iband > -1} {
                               set aux [readSTRUCT TSOBJ $source 0 flags<$iband>]
                               handleSet $orphan.$el $aux
                           }
               }
                    }
                }
            } 
            # all set, add it to the chain
        chainElementAddByPos $orphans $orphan
    }
     }
     # clean up   
     chainCursorDel $chain $cursor
     chainDestroy $matchPos genericDel
      
     return $orphans

}


# if this source does not exist in the chain of matched sources, and
# if it passes conditions (obj_type, flagsON, flagsOFF), then return
# 1, 0 otherwise.
proc good_orphan {matches epoch source msize one_run obj_type} {

    # coordinates for this source
    set lambda0 [readSTRUCT TSOBJ $source 0 lambda]
    set eta0 [readSTRUCT TSOBJ $source 0 eta]
    # bounding box around it 
    set lMin [expr $lambda0 - $msize]; set lMax [expr $lambda0 + $msize]; 
    set eMin [expr $eta0 - $msize]; set eMax [expr $eta0 + $msize]; 
    # search for sources in the bounding box
    set cond "{lambda > $lMin} {lambda < $lMax} {eta > $eMin} {eta < $eMax}"
    set box [chainSearch $matches $cond]

    # if found matched sources in the box, return immediately
    if {[chainSize $box] > 0} {
       chainDel $box
       return 0
    }

    ### OK, this is an orphan, but is it a good orphan?
    if {!$one_run} {   
       # unless doing one-run overlaps, avoid edge objects
       set row [readSTRUCT TSOBJ $source 0 objc_rowc]
       if {$row < 64 || $row > 1424} {return 0}
    }

    if {$obj_type != "all" && $obj_type != -1} {
        set type [readSTRUCT TSOBJ $source 0 objc_type]
        if {$type != $obj_type} {
            return 0
        }
    }

    chainDel $box
    return 1
}




## a good match must have set on all the flags specified in the list 
# flagsON, while all the flags specified in the list flagsOFF must be
# off.
proc good_match {match one_run obj_type flagsON flagsOFF} {

# band for enforcing flags
global fband

    set filters {u g r i z}   

    if {!$one_run} {   
       # unless doing one-run overlaps, avoid edge objects
       set row1 [readSTRUCT TSOBJ $match 1 objc_rowc]
       set row2 [readSTRUCT TSOBJ $match 2 objc_rowc]
       if {$row1 < 64 || $row1 > 1424 || $row2 < 0} {return 0}
    }

    if {$obj_type != "all" && $obj_type != -1} {
        if {[info exist fband] && [lsearch $filters $fband] > -1} {
        set iband [lsearch $filters $fband]
            set Rtype type<$iband>
        } else {
            set Rtype objc_type
        }
        set type1 [readSTRUCT TSOBJ $match 1 $Rtype]
        set type2 [readSTRUCT TSOBJ $match 2 $Rtype]
        if {$type1 != $obj_type || $type2 != $obj_type} {
            return 0
        }
    }

    if {$flagsON != 0 || $flagsOFF != 0} {
        if {[info exist fband] && [lsearch $filters $fband] > -1} {
        set iband [lsearch $filters $fband]
            set Rflags flags<$iband>
            set Rflags2 flags2<$iband>
        } else {
            set Rflags objc_flags
            set Rflags2 objc_flags2
        }
        set flags1 [readSTRUCT TSOBJ $match 1 $Rflags]
        set flags2 [readSTRUCT TSOBJ $match 1 $Rflags2]
        set flags [expr ($flags1 | $flags2)]
        if {![are_flagsOK $flags $flagsON $flagsOFF]} {
            return 0
        }
        set flags1 [readSTRUCT TSOBJ $match 2 $Rflags]
        set flags2 [readSTRUCT TSOBJ $match 2 $Rflags2]
        set flags [expr ($flags1 | $flags2)]
        if {![are_flagsOK $flags $flagsON $flagsOFF]} {
            return 0
        } 
    }
    return 1

}


proc dump_good_sources {chain id0 obj_type fON1 fOFF1 fON2 fOFF2 no_doubles logfile outf} { 

global fband  

     set filters {u g r i z}

     # This loops over all of the elements, checks that the flags are 
     # set/not set as requested, and dumps good ones to the output file
     set good_chain [chainNew PHOTOMETRY]
     set cursor [chainCursorNew $chain]
     set Ngood 0

     # which fields do we want copied over? 
     # as defined in PHOTOMETRY
     set Pfieldsaux [schemaGetFromType PHOTOMETRY]
     set Pfields {}
     foreach el $Pfieldsaux {   
        set el [lindex $el 0]
        lappend Pfields $el
     }
     # from TSOBJ
     set Tfieldsaux [schemaGetFromType TSOBJ]
     set Tfields {}
     foreach el $Tfieldsaux {   
        set el [lindex $el 0]
        lappend Tfields $el
     }
     # this could be done in a more general way...
     set chromatic [list type psfCounts psfCountsErr petroCounts petroCountsErr \
          petroR50 petroR50Err counts_model counts_modelErr star_L exp_L deV_L reddening]

     while {[set source [chainWalk $chain $cursor]] != ""} { 
        if [good_source $source $obj_type $fON1 $fOFF1 $fON2 $fOFF2 $no_doubles] { 
            # this is a good match, copy needed fields to output structure
            set good_source [genericNew PHOTOMETRY]
            incr Ngood
            # first set overall counter
            handleSet $good_source.localID [expr $id0 + $Ngood] 
            # copy over all the fields that exist
            foreach el $Pfields {
                set record $el
                if {[lsearch $Tfields $record] >= 0} {
                    # this field exists, treat specially chromatic fields
                    if {[lsearch $chromatic $record] >= 0} {
                       loop j 0 5 {
                          set rec "$record<$j>"
                          set aux [readSTRUCT TSOBJ $source 0 $rec]
                          handleSet $good_source.$el<$j> [format "%6.3f" $aux]
                       }
                    } else {
                       set aux [readSTRUCT TSOBJ $source 0 $record]
                       handleSet $good_source.$el $aux
                    }
                    # special case for a single band search
                    if {[info exist fband]} {
                        set iband [lsearch $filters $fband]
                        if {$iband > -1} {
                           echo dump_good_sources: band $fband, record = $record
                        }
                        if {$record == "objc_type" && $iband > -1} {
                           set aux [readSTRUCT TSOBJ $source 0 type<$iband>]
                           handleSet $orphan.$el $aux
                        }
                        if {$record == "objc_flags" && $iband > -1} {
                           set aux [readSTRUCT TSOBJ $source 0 flags<$iband>]
                           handleSet $orphan.$el $aux
                        }
                    }
                }
            } 
            # all set, add it to the chain
            chainElementAddByPos $good_chain $good_source
        }
     }
       
     # dump the chain of good matches to the output file
     set Ngood [chainSize $good_chain]
     chain2Param $outf $good_chain "" -append -notypedump

     # clean up   
     chainCursorDel $chain $cursor
     chainDestroy $good_chain genericDel
   
     return $Ngood

}




## a good source must have set on all the flags specified in the list 
# flagsON, while all the flags specified in the list flagsOFF must be
# off. If $no_doubles then only sources which satisfy 64 <= rowc <= 1425
# are selected
proc good_source {source obj_type flagsON1 flagsOFF1 flagsON2 flagsOFF2 no_doubles} {

    if {$no_doubles} {   
       set rowc [readSTRUCT TSOBJ $source 0 objc_rowc]
       if {$rowc < 64 || $rowc > 1425} {return 0}
    }

    if {$obj_type != "all" && $obj_type != -1} {
        set type [readSTRUCT TSOBJ $source 0 objc_type]
        if {$type != $obj_type} {
            return 0
        }
    }

    if {$flagsON1 != 0 || $flagsOFF1 != 0} {
        set flags [readSTRUCT TSOBJ $source 0 objc_flags]
        if {![are_flagsOK $flags $flagsON1 $flagsOFF1]} {
            return 0
        } 
    }

    if {$flagsON2 != 0 || $flagsOFF2 != 0} {
        set flags [readSTRUCT TSOBJ $source 0 objc_flags2]
        if {![are_flagsOK $flags $flagsON2 $flagsOFF2]} {
            return 0
        } 
    }

    return 1

}

# given a chain of TSOBJ, search for and return only the elements
# which satisfy flagsON and flagsOFF, and selection function sel_func
proc chainSearchByFlags {chain flagsON flagsOFF {sel_func ""} {epoch 0}} {

# band for enforcing flags
global fband

    set filters {u g r i z}

    set chainType [exprGet $chain.type]
    set new [chainNew $chainType]
    set flagsOFF [translate_flags $flagsOFF]
    set flagsON [translate_flags $flagsON]


    if {[info exist fband] && [lsearch $filters $fband] > -1} {
        set iband [lsearch $filters $fband]
        set Rflags flags<$iband>
        set Rflags2 flags2<$iband>
    } else {
        if {$epoch == 2} { 
           set Rflags objc_flagsB
           set Rflags2 objc_flags2B
        } else {
           set Rflags objc_flags
           set Rflags2 objc_flags2
        }
    }

    loop i 0 [chainSize $chain] {
    set el [chainElementGetByPos $chain $i]
        set flags1 [readSTRUCT $chainType $el 0 $Rflags]
        set flags2 [readSTRUCT $chainType $el 0 $Rflags2]
        set flags [expr ($flags1 | $flags2)]
        if {[are_flagsOK $flags $flagsON $flagsOFF]} {
       if {$sel_func != "" && ![eval $sel_func $el]} {
           continue;
       }            
           chainElementAddByPos $new $el TAIL AFTER
        }      
    }

    return $new
}



proc doglob {pattern} {

   if {[catch {set a [glob $pattern]}]} {
      set reply ""
   } else {
      set reply $a
   }

   return $reply
}




#####################################################################
#  Plotting procs 

# given a dump file with PHOTOMETRY data, perform various statistical
# analysis and produce plots and summary files. Sources are binned 
# in fields with the bin width of 2*half_win
proc photometryQA {dump_file device {half_win 0} {fmin -1} {fmax -1} \
           {chain ""} {mag_type "psfCounts"} } {

global mem

     set filterlist {u g r i z} 
     set colors {ug gr ri iz} 

     # only sources brighter than this are considered
     set mmax [get_parameter mmax]
     # only sources with errors less than this are considered
     set emax [get_parameter emax]
     # a given overlap must have at least this many sources
     set Nmin [get_parameter Nmin]
     # maximum values for triggering reporting
     if {$mag_type == "psfCounts"} {
         set type psf
     } else { 
         # all other magnitudes are treated as model mags
         set type mod
     } 
     set med_max [get_parameter ${type}_med_max]
     set sig_max [get_parameter ${type}_sig_max]
     set chi_max [get_parameter ${type}_chi_max]

     # read chain from input file
     set readChain 0
     if {$chain == ""} {
        set chain [param2Chain $dump_file ""]
        set readChain 1
        write_mem mem "photometryQA: read chain" 
     }
     set Ndata [chainSize $chain]
     echo "Processing $Ndata sources"

     set fields [vFromChain $chain field]
     loop i 0 5 {
        set f [lindex $filterlist $i]
        echo "   getting photometry for filter $f"
        set mpsf($f) [vFromChain $chain psfCounts<$i>]
        set epsf($f) [vFromChain $chain psfCountsErr<$i>]
        set mmod($f) [vFromChain $chain counts_model<$i>]
        set emod($f) [vFromChain $chain counts_modelErr<$i>]
        set reddening($f) [vFromChain $chain reddening<$i>]
        set delmag($f) [vectorExprEval $mpsf($f)-$mmod($f)]
    set mpsfR($f) [vectorExprEval $mpsf($f)-$reddening($f)] 
     }
     if {$readChain} {genericChainDestroy $chain}
 
     # colors
     set C(ug) [vectorExprEval $mpsf(u)-$mpsf(g)]
     set C(gr) [vectorExprEval $mpsf(g)-$mpsf(r)]
     set C(ri) [vectorExprEval $mpsf(r)-$mpsf(i)]
     set C(iz) [vectorExprEval $mpsf(i)-$mpsf(z)]
     # reddening corrections
     set CR(ug) [vectorExprEval $C(ug)-($reddening(u)-$reddening(g))]
     set CR(gr) [vectorExprEval $C(gr)-($reddening(g)-$reddening(r))]
     set CR(ri) [vectorExprEval $C(ri)-($reddening(r)-$reddening(i))]
     set CR(iz) [vectorExprEval $C(iz)-($reddening(i)-$reddening(z))]

     # field range and field bins
     if {$fmin < 0} {
        set fmin [expr int([vExtreme $fields min])]
     }
     if {$fmax < 0} {
        set fmax [expr int([vExtreme $fields max])]
     }
     
     if {$half_win > 0} {
         set Nbin [expr ($fmax - $fmin)/(2*$half_win)]
         set w [expr 2*$half_win]
     } else {
         set Nbin [expr $fmax - $fmin + 1]
         set half_win 0
         set w 1 
     } 
     set fbinned [vectorExprNew $Nbin]
     vNameSet $fbinned "field"
     set ww [expr 1+2*$half_win]

     handleSet $fbinned.vec<0> [expr $fmin + $half_win]
     loop i 1 $Nbin {
         set j [expr $i-1]
         handleSet $fbinned.vec<$i> [expr [exprGet $fbinned.vec<$j>] + $w]
     }
     # mask takes care of the magnitude/error limits (r band)
     set me_mask [get_magerr_mask $mpsf(r) $epsf(r) 14.0 $mmax $emax]
     set Ngood [vNnonzero $me_mask]
     echo "   all set for plotting; selected $Ngood good sources out of $Ndata"

     write_mem mem "photometryQA: going to analyze"
    ##### all set, now can plot any quantity for any subsample ####


    ### low-z QSOs ###
     echo "   * working on low-z QSOs *"
     # counts of low-z QSOs are too low for a field-by-field statistics, so 
     # 29.09 fields = 4.4 degrees length = 1 deg2 area (29 fields = 0.9979 deg2)
     set QSOhwin 14; 
     set QSOww [expr 1+2*$QSOhwin]
     # QSO selection (with reddening correction)
     set ugMin -0.3; set ugMax 0.6; set grMin -0.2; set grMax 0.6; set uMax 21.0
     set cond "$CR(ug) > $ugMin && $CR(ug) < $ugMax && $CR(gr) > $grMin && $CR(gr) < $grMax && \
               $mpsfR(u) < $uMax ? $me_mask : 0"  
     set qmask [vectorExprEval $cond]
     echo "      there are [vNnonzero $qmask] low-z QSOs, doing statistics..."


     foreach color $colors {
        write_mem mem "photometryQA: QSO binned_stats in color $color"
        set qso($color) [binned_stats $fields $C($color) $qmask $fbinned $QSOhwin 3 -99.0]
        set qsoR($color) [binned_stats $fields $CR($color) $qmask $fbinned $QSOhwin 3 -99.0]
        set N($color) [lindex $qso($color) 0]
        set med($color) [lindex $qso($color) 1]
        set mask($color) [lindex $qso($color) 3]
        vNameSet $N($color) "N"
        vNameSet $med($color) "median"
        # reddening corrected median
        set medR($color) [lindex $qsoR($color) 1]
        set sigR($color) [lindex $qsoR($color) 2]
        vNameSet $sigR($color) "sigma"
        vNameSet $medR($color) ""
     } 

     echo "      statistics finished, plotting..."
     set cmin(ug) -0.1; set cmax(ug) 0.5
     set cmin(gr) -0.1; set cmax(gr) 0.5
     set cmin(ri) -0.1; set cmax(ri) 0.5
     set cmin(iz) -0.1; set cmax(iz) 0.5
     set smax 0.25
     foreach color $colors {
        if [catch {
           # plot 
           set ext "qso_$color"
           set t1 "median $color color for QSOs (thick: dereddened)" 
           set t2 "sigma for $color distribution" 
           set L1 "window = $QSOww fields, mpsf(r) < $mmax, err < $emax" 
           set L2 "QSOs: $ugMin < u-g < $ugMax, $grMin < g-r < $grMax, u < $uMax"    
           plot_sample_color $device $dump_file $ext $fbinned $fmin $fmax $cmin($color) $cmax($color) \
                $smax $N($color) $med($color) $medR($color) $sigR($color) $mask($color) $t1 $t2 $L1 $L2  
        } msg] {
         echo "$msg"
        }
     }


    ### halo stars ###
     echo "   * working on halo stars *" 
     # halo star selection (with reddening correction)
     set grMin 0.2; set grMax 0.6; set rMin 18.5; set rMax $mmax
     set cond "$CR(gr) > $grMin && $CR(gr) < $grMax && $mpsfR(r) > $rMin && \
               $mpsfR(r) < $rMax ? $me_mask : 0"  
     set hmask [vectorExprEval $cond]
     echo "      there are [vNnonzero $hmask] halo stars, doing statistics..."

     foreach color $colors {
        write_mem mem "photometryQA: halo binned_stats in color $color"
        set halo($color) [binned_stats $fields $C($color) $hmask $fbinned $half_win 3 -99.0]
        set haloR($color) [binned_stats $fields $CR($color) $hmask $fbinned $half_win 3 -99.0]
        set N($color) [lindex $halo($color) 0]
        set med($color) [lindex $halo($color) 1]
        set mask($color) [lindex $halo($color) 3]
        vNameSet $N($color) "N"
        vNameSet $med($color) "median"
        # reddening corrected median
        set medR($color) [lindex $haloR($color) 1]
        set sigR($color) [lindex $haloR($color) 2]
        vNameSet $sigR($color) "sigma"
        vNameSet $medR($color) ""
     } 

     echo "      statistics finished, plotting..."
     set cmin(ug) 0.8; set cmax(ug) 1.2
     set cmin(gr) 0.21; set cmax(gr) 0.56
     set cmin(ri) 0.0; set cmax(ri) 0.3
     set cmin(iz) 0.0; set cmax(iz) 0.3
     set smax 0.19
     foreach color $colors {
        if [catch {
           # plot 
           set ext "halo_$color"
           set t1 "median $color color for halo stars (thick: dereddened)" 
           set t2 "sigma for $color distribution" 
           set L1 "window = $ww fields, mpsf(r) < $mmax, err < $emax" 
           set L2 "halo stars: $ugMin < u-g < $ugMax, $grMin < g-r < $grMax, u < $uMax"    
           plot_sample_color $device $dump_file $ext $fbinned $fmin $fmax $cmin($color) $cmax($color) \
                $smax $N($color) $med($color) $medR($color) $sigR($color) $mask($color) $t1 $t2 $L1 $L2  
        } msg] {
         echo "$msg"
        }
     }

    ### disk stars ###
     echo "   * working on disk stars *"
     # disk star selection (with reddening correction)
     set grMin 1.2; set grMax 1.7; set riMin 0.8; set riMax 1.1
     set cond "$CR(gr) > $grMin && $CR(gr) < $grMax && \
               $CR(ri) > $riMin && $CR(ri) < $riMax ? $me_mask : 0"  
     set dmask [vectorExprEval $cond]
     echo "      there are [vNnonzero $dmask] disk stars, doing statistics..."

     set Dcolors {gr}
     foreach color $Dcolors {
        write_mem mem "photometryQA: disk binned_stats in color $color"
        set disk($color) [binned_stats $fields $C($color) $dmask $fbinned $half_win 3 -99.0]
        set diskR($color) [binned_stats $fields $CR($color) $dmask $fbinned $half_win 3 -99.0]
        set N($color) [lindex $disk($color) 0]
        set med($color) [lindex $disk($color) 1]
        set mask($color) [lindex $disk($color) 3]
        vNameSet $N($color) "N"
        vNameSet $med($color) "median"
        # reddening corrected median
        set medR($color) [lindex $diskR($color) 1]
        set sigR($color) [lindex $diskR($color) 2]
        vNameSet $sigR($color) "sigma"
        vNameSet $medR($color) ""
     } 

     echo "      statistics finished, plotting..."
     set cmin(gr) 1.11; set cmax(gr) 1.69
     set smax 0.12
     foreach color $Dcolors {
        if [catch {
           # plot 
           set ext "disk_$color"
           set t1 "median $color color for disk stars (thick: dereddened)" 
           set t2 "sigma for $color distribution" 
           set L1 "window = $ww fields, mpsf(r) < $mmax, err < $emax" 
           set L2 "disk stars: $ugMin < u-g < $ugMax, $grMin < g-r < $grMax, u < $uMax"    
           plot_sample_color $device $dump_file $ext $fbinned $fmin $fmax $cmin($color) $cmax($color) \
                $smax $N($color) $med($color) $medR($color) $sigR($color) $mask($color) $t1 $t2 $L1 $L2  
        } msg] {
         echo "$msg"
        }
     }


    ### g-r comparison for QSOs, halo-QSOs, disk - QSOs
     echo "   * comparing median g-r colors for QSOs and (halo and disk) stars *"
     set grQSO  [lindex $qsoR(gr) 1]; set maskQSO [lindex $qsoR(gr) 3]
     set grHalo [lindex $haloR(gr) 1]; set maskHalo [lindex $haloR(gr) 3]
     set grDisk [lindex $diskR(gr) 1]; set maskDisk [lindex $diskR(gr) 3]
     # g-r color differences
     set HaloQSO [vectorExprEval $grHalo-$grQSO]
     set DiskQSO [vectorExprEval $grDisk-$grQSO]
     vNameSet $grQSO "g-r"
     vNameSet $HaloQSO "g-r"
     vNameSet $DiskQSO "g-r"
     # plot 3 panel figure
     if [catch {
        set pg [pgstateNew]
        set PGdevice [devicename $device ${dump_file}_compare_gr.ps] 
        pgstateSet $pg -device $PGdevice -lineWidth 2 -isLine 1 \
            -xfract 0.4 -yfract 0.45 -nxwindow 1 -nywindow 4 
        pgstateOpen $pg 

        # QSO
        vPlot $pg $fbinned $grQSO -xmin $fmin -xmax $fmax -ymin -0.1  -ymax 0.5
        set mean_med [vPlotStats $grQSO $maskQSO "mean" 0.05 -0.1]
        titlePlot "Low-z QSOs, window = $QSOww fields" 20
        pgstateSet $pg -icLine 2 -isNewplot 0
        plot_line $pg $fmin $mean_med $fmax $mean_med 1 5
        # Halo-QSOs         
        pgstateSet $pg -icLine 0 -isNewplot 1
        vPlot $pg $fbinned $HaloQSO -xmin $fmin -xmax $fmax -ymin 0.0  -ymax 0.4
        set mean_med [vPlotStats $HaloQSO $maskHalo "mean" 0.05 -0.1]
        titlePlot "Halo-QSO, halo window = $w fields" 20
        pgstateSet $pg -icLine 2 -isNewplot 0
        plot_line $pg $fmin $mean_med $fmax $mean_med 1 5
        # Disk-QSOs         
        pgstateSet $pg -icLine 0 -isNewplot 1
        vPlot $pg $fbinned $DiskQSO -xmin $fmin -xmax $fmax -ymin 1.0  -ymax 1.4
        set mean_med [vPlotStats $DiskQSO $maskDisk "mean" 0.05 -0.1]
        titlePlot "Disk-QSO, disk window = $w fields" 20
        pgstateSet $pg -icLine 2 -isNewplot 0
        plot_line $pg $fmin $mean_med $fmax $mean_med 1 5

        endplot $pg
     } msg] {
         echo "$msg"
     }

     # clean colors etc.
     vectorExprDel $qmask; vectorExprDel $hmask; vectorExprDel $dmask; 
     vectorExprDel $HaloQSO; vectorExprDel $DiskQSO 


   ### 2) the mpsf-mmod scatter for STARS only
     # stars == mpsf-mmod < 0.2 in both g and r
     echo " the mpsf-mmod scatter"  
     set diff_min -0.03
     set diff_max 0.20
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set PGdevice [devicename $device ${dump_file}_psf_mod.ps] 
         pgstateSet $pg -device $PGdevice -lineWidth 2 -isLine 1 \
          -xfract 0.2 -yfract 0.45 -nxwindow 1 -nywindow 7 
         pgstateOpen $pg 
 
         # loop over filters 
         foreach f $filterlist {
            # mask takes care of the magnitude/error limits
            set psfmask [get_magerr_mask $mpsf($f) $epsf($f) 14.0 $mmax $emax]
            set modmask [get_magerr_mask $mmod($f) $emod($f) 14.0 $mmax $emax]
            set fmask [vectorExprEval $psfmask*$modmask]
            # get statistics
            set qmask [vectorExprEval "$delmag($f) < $emax ? $fmask : 0" ]
            set psfmod [binned_stats $fields $delmag($f) $qmask $fbinned $half_win 3 -99.0]
            vectorExprDel $psfmask; vectorExprDel $modmask
            vectorExprDel $qmask; vectorExprDel $fmask
            # unpack
            set psfmod_N [lindex $psfmod 0]
            set psfmod_median [lindex $psfmod 1]
            set psfmod_sig [lindex $psfmod 2]
            set psfmod_mask [lindex $psfmod 3]
            # plot sigma  
            vNameSet $psfmod_sig "filter $f"
            pgstateSet $pg -icLine 2 -lineWidth 3
            vPlot $pg $fbinned $psfmod_sig -xmin $fmin -xmax $fmax -ymin $diff_min -ymax $diff_max
            plot_line $pg -10 0.0 2000 0.0 1 5
            set meanscatter($f) [vPlotStats $psfmod_sig $psfmod_mask "mean" 0.05 -0.1] 
        if {[lsearch $filterlist $f] == 0} {  
               titlePlot "file $dump_file" 20
            }
            pgstateSet $pg -icLine 1 -isNewplot 0 -lineWidth 1
            vPlot $pg $fbinned $psfmod_median -xmin $fmin -xmax $fmax -ymin $diff_min -ymax $diff_max
            pgstateSet $pg -icLine 0 -isNewplot 1 -lineWidth 2
            # clean
            vListDel $psfmod 
     }
      
         plot_info "window = $ww fields, mpsf-mmod < $emax, mag < $mmax, err < $emax" \
                    0.0 -1.0 -0.95         
         plot_info "** m(psf)-m(mod) **  thin: median, thick: sigma" 0.0 -1.0 -1.2

         endplot $pg
     } msg] {
         echo "$msg"
     }



     ### clean
     vectorExprDel $fields; vectorExprDel $fbinned; vectorExprDel $me_mask
     loop i 0 5 {
        set f [lindex $filterlist $i]
    vListDel [list $mpsf($f) $epsf($f) $mpsfR($f) $mmod($f) $emod($f) \
                       $delmag($f) $reddening($f)]
     }
     foreach color {ug gr ri iz} {
        vListDel [list $C($color) $CR($color)]
        vListDel $qso($color); vListDel $qsoR($color);               
        vListDel $halo($color); vListDel $haloR($color);               
     }
     foreach color $Dcolors {
        vListDel $disk($color); vListDel $diskR($color);               
     }

}



# a wrapper for QA plots
proc plot_sample_color {device dump_file ext fbinned fmin fmax cmin cmax smax  \
                        vN vMed vMedR vSig vMask title1 title2 line1 line2} { 

     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set PGdevice [devicename $device ${dump_file}_$ext.ps] 
         pgstateSet $pg -device $PGdevice -lineWidth 2 -isLine 1 \
          -xfract 0.4 -yfract 0.45 -nxwindow 1 -nywindow 4 
         pgstateOpen $pg 

         # N sources
         vPlot $pg $fbinned $vN -xmin $fmin -xmax $fmax -ymin 0
         set mean_N [vPlotStats $vN $vMask "mean" 0.05 -0.1] 
         plot_line $pg $fmin $mean_N $fmax $mean_N 1 5
         titlePlot "file: $dump_file" 20

         # median color
         pgstateSet $pg -lineWidth 1
         vPlot $pg $fbinned $vMed -xmin $fmin -xmax $fmax -ymin $cmin  -ymax $cmax
         pgstateSet $pg -lineWidth 2
         set mean_med [vPlotStats $vMedR $vMask "mean" 0.05 -0.1]
         titlePlot $title1  20
         plot_line $pg $fmin $mean_med $fmax $mean_med 1 5
         pgstateSet $pg -icLine 2 -isNewplot 0 -lineWidth 3
         vPlot $pg $fbinned $vMedR -xmin $fmin -xmax $fmax -ymin $cmin -ymax $cmax
         pgstateSet $pg -icLine 0 -isNewplot 1 -lineWidth 2

         # scatter in color
         vPlot $pg $fbinned $vSig -xmin $fmin -xmax $fmax -ymin 0.0  -ymax $smax
         set mean_sig [vPlotStats $vSig $vMask "mean" 0.05 -0.1] 
         titlePlot $title2  20
         plot_line $pg $fmin $mean_sig $fmax $mean_sig 1 5
         pgstateSet $pg -icLine 2 -isNewplot 0 -lineWidth 3
         vPlot $pg $fbinned $vSig
         pgstateSet $pg -icLine 0 -isNewplot 1 -lineWidth 2

         # info at the bottom
         plot_info $line1 0.0 -1.0 -0.7
         plot_info $line2 0.0 -1.0 -0.9
        
         endplot $pg
     } msg] {
         echo "$msg"
     }
 }



# return a mask vector with value 1 when 
# mmin < mag < mmax, and err < emax
proc get_magerr_mask {mag err mmin mmax emax} {

     set aux1 [vectorExprEval "$mag > $mmin ? 1 : 0"]
     set aux2 [vectorExprEval "$mag < $mmax ? $aux1 : 0"]
     set mask [vectorExprEval "$err < $emax ? $aux2 : 0"]
     vectorExprDel $aux1
     vectorExprDel $aux2
     
     return $mask
}   

proc vListDel {list} {

   foreach v $list {
       vectorExprDel $v
   }

}


proc plot_overlaps {match_file device {mag_type psfCounts} \
          {f_min -1} {f_max -1} {half_window 0} {chain ""}} {

     # only sources brighter than this are considered
     set mmax [get_parameter mmax]
     # only sources with errors less than this are considered
     set emax [get_parameter emax]
     # a given overlap must have at least this many sources
     set Nmin [get_parameter Nmin]
     # maximum values for triggering reporting
     if {$mag_type == "psfCounts"} {
         set type psf
     } else { 
         set type mod
     }
     set med_max [get_parameter ${type}_med_max]
     set sig_max [get_parameter ${type}_sig_max]
     set chi_max [get_parameter ${type}_chi_max]
     # clip on maximum delta(m)
     set mdifmax [get_parameter mdifmax]
     set filterlist {u g r i z} 

     # read chain from input file
     set read_chain 0
     if {$chain == ""} {
         set chain [param2Chain $match_file ""]    
         set read_chain 1
     } 

     # make vectors
     set fields [vFromChain $chain field]
     if {$f_min < 0} {
        set f_min [expr int([vExtreme $fields min])]
     }
     if {$f_max < 0} {
        set f_max [expr int([vExtreme $fields max])]
     }

     set return_list {}
     loop i 0 5 { 
         set f [lindex $filterlist $i]
         set m1 [vFromChain $chain ${mag_type}<$i>]
         set e1 [vFromChain $chain ${mag_type}Err<$i>]
         set m2 [vFromChain $chain ${mag_type}B<$i>]
         set e2 [vFromChain $chain ${mag_type}ErrB<$i>]
         # calculate statistics
         set stats($f) [mdiff_stats $fields $f_min $f_max $m1 $e1 $m2 $e2 \
                                    $mmax $emax $Nmin $mdifmax $half_window]
         # find all problematic fields
         set problem_list [analyze_stats $stats($f) $med_max $sig_max $chi_max]
         lappend return_list $problem_list 
         # clean
         vectorExprDel $m1; vectorExprDel $m2
         vectorExprDel $e1; vectorExprDel $e2
     }
     vectorExprDel $fields
     if {$read_chain} {genericChainDestroy $chain}

     # plot
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set device [devicename $device ${match_file}_delmag_$mag_type.ps] 
         pgstateSet $pg -device $device -lineWidth 1 -isLine 1 \
          -xfract 0.4 -yfract 0.35 -nxwindow 4 -nywindow 6 
         pgstateOpen $pg

         # plotting for each band
         loop i 0 5 { 
            set f [lindex $filterlist $i]
            set fields [lindex $stats($f) 0]
            set Ndata  [lindex $stats($f) 1]
            set median [lindex $stats($f) 2]
            set sigma  [lindex $stats($f) 3]
            set chi    [lindex $stats($f) 4]
            set mask   [lindex $stats($f) 5]

            # Ndata
            pgstateSet $pg -icLine 2
            vPlot $pg $fields $Ndata -xmin $f_min -xmax $f_max -ymin 0 
            plot_info "filter $f" 90.0 
        if {$i == 2} {ylabel "N stars"}
        if {$i == 4} {xlabel "field"}
            # title
        if {$i == 0} {
                titlePlot "mag < $mmax, err < $emax" 20
        }
            # minimum number of stars
            plot_line $pg $f_min $Nmin $f_max $Nmin 1 5
 
            # median
            pgstateSet $pg -icLine 2
            vPlot $pg $fields $median -xmin $f_min -xmax $f_max \
                      -ymin -0.15 -ymax 0.15
            vPlotStats $median $mask "mean" 0.15 -0.3 
            plot_line $pg $f_min 0 $f_max 0 1 1
        if {$i == 4} {xlabel "field"}
            if {$i == 2} {ylabel "median for the distribution (m1-m2)"}
            # title
        if {$i == 0} {
                titlePlot "mag. type = $mag_type" 20
        }
            pgstateSet $pg -isNewplot 1 
            # sigma
            pgstateSet $pg -icLine 2
            vPlot $pg $fields $sigma -xmin $f_min -xmax $f_max \
                      -ymin 0 -ymax 0.29
            set mask_aux [vectorExprEval "$sigma > 0 ? $mask : 0"]            
            vPlotStats $sigma $mask_aux "mean" 0.15 -0.3 
            vectorExprDel $mask_aux          
            plot_line $pg $f_min 0 $f_max 0 1 1 
            if {$i == 4} {xlabel "field"}
            if {$i == 2} {ylabel "sigma for the distribution (m1-m2)"}
            # title
        if {$i == 0} {
                set file [file tail $match_file]
                titlePlot "   file: $file" 20
        }
            pgstateSet $pg -isNewplot 1 
            # chi
            pgstateSet $pg -icLine 2
            vPlot $pg $fields $chi -xmin $f_min -xmax $f_max \
                      -ymin 0 -ymax 3.5
            vPlotStats $chi $mask "mean" 0.15 -0.3 
            plot_line $pg $f_min 0 $f_max 0 1 1 
            if {$i == 4} {xlabel "field"}
            set str "sigma for the distribution (m1-m2)/sqrt(err1**2+err2**2)"
            if {$i == 2} {ylabel $str}
            pgstateSet $pg -isNewplot 1 

            # clean
            vectorExprDel $fields
            vectorExprDel $Ndata
            vectorExprDel $median
            vectorExprDel $sigma
            vectorExprDel $chi           
            vectorExprDel $mask          
         }  
         endplot $pg
     } msg] {
         echo "$msg"
     }
     
     return $return_list
     
}

#ts> matchedPairChainDel $chain
#
#The stats chain can be destroyed with:
#
#ts> taGenericChainDestroy $stats
#


# using chains of PHOTOMATCHs, produce an html file with summary tables
# and plots with detailed information on photometric and astrometric 
# consistency between two runs
# matches is a list of chains with PHOTOMATCH structure 
# names is a list of names to go with these chains, e.g. "col. 3 vs. col. 4"
# title is a summary description e.g. "Run 745 vs. run 756"
# htmlfile is name for output html file 
# outDir is desired output directory for html and PS files 
proc do_match_stats {matches names title htmlfile {outDir .}} {
  
    #assert { [llength $matches] == [llength $names] }
    set filters {u g r i z} 

    ### open html file
    set outf [open $htmlfile w]         
    puts $outf "<title>Match Statistics for $title</title>"
    puts $outf "<BODY bgcolor=\"EEEEFF\">"
    puts $outf "<hr>"
    puts $outf "<H1><font color=\"blue\">Match Statistics for $title</font></H1>"
    puts $outf "<hr>"
    puts $outf "QA produced on [exec date]"
    puts $outf "<br> Note that entries in the first column are linked to PS plots<hr> "    

    set opdb 1      

    ### produce plots and get back the lists with statistics
    set Nmatches [llength $matches]
    loop i 0 $Nmatches {
       set chains [lindex $matches $i]
       set name($i) [lindex $names $i]
       if {!$opdb} {
          ## select stars
          set chain $chains
          set stars [chainSearch $chain "{objc_type == 6} {objc_typeB == 6}"] 
       } else {
          set stars [lindex $chains 0]
          set galaxies [lindex $chains 1]
       }
       # stars, psf mags
       set file $outDir/match_stats_stars_psf_$i.ps
       set filenameSpsf($i) $file 
       set header "$title, $name($i) stars"
       set listSpsf($i) [plot_match_stats_photo /VCPS $stars psfCounts $header $file]
       # stars, mod mags
       set file $outDir/match_stats_stars_mod_$i.ps 
       set filenameSmod($i) $file 
       set header "$name($i) stars"
       set listSmod($i) [plot_match_stats_photo /VCPS $stars counts_model $header $file]
       
       if {$opdb} {
         set chain [chainJoin $stars $galaxies]   
       }

       # all, psf mags
       set file $outDir/match_stats_all_psf_$i.ps
       set filenameApsf($i) $file 
       set header "$name($i) all"
       set listApsf($i) [plot_match_stats_photo /VCPS $chain psfCounts $header $file]
       # all, mod mags
       set file $outDir/match_stats_all_mod_$i.ps
       set filenameAmod($i) $file 
       set header "$name($i) all"
       set listAmod($i) [plot_match_stats_photo /VCPS $chain counts_model $header $file]
       # astrometry
       set file $outDir/match_stats_astro_$i.ps
       set filenameA($i) $file 
       set header "$name($i)"
       set listA($i) [plot_match_stats_astro /VCPS $chain counts_model $header $file]
    }   

    ### make tables and write to $outf

    ## calibration offsets    
    set header "Photometric Calibration Offsets"
    set MaxOffsets {0.05 0.03 0.03 0.03 0.04}
    make_html_table $outf $header $MaxOffsets $Nmatches name filenameSpsf listSpsf 0 0

    ## Photometric accuracy for bright stars (psf mag)     
    set header "Photom. Acc. for Stars (psf(r)<19)"
    set MaxOffsets {0.1 0.05 0.05 0.05 0.07}
    make_html_table $outf $header $MaxOffsets $Nmatches name filenameSpsf listSpsf 1

    ## Photometric accuracy for bright stars (mod mag)     
    set header "Photom. Acc. for Stars (mod(r)<19)"
    set MaxOffsets {0.1 0.05 0.05 0.05 0.07}
    make_html_table $outf $header $MaxOffsets $Nmatches name filenameSmod listSmod 1

    ## Chi for all objects (psf mag) 
    set header "delmag/err (all, psf mag)"
    set MaxOffsets {2.5 2.5 2.5 2.5 2.5}
    make_html_table $outf $header $MaxOffsets $Nmatches name filenameApsf listApsf 2

    ## Chi for all objects (model mag) 
    set header "delmag/err (all, mod mag)"
    set MaxOffsets {2.5 2.5 2.5 2.5 2.5}
    make_html_table $outf $header $MaxOffsets $Nmatches name filenameAmod listAmod 2


    ## Astrometry table 
    set warning 0
    puts $outf "<P>"
    puts $outf "<table border cellspacing=0 cellpadding=6>"
    puts $outf "<tr> <td colspan=0 rowspan=1></td>"
    puts $outf "<th colspan=7 align=center><font color=\"blue\"> \
                Astrometry </font></th> </tr>"
    puts $outf "<tr><th>Quantity:</th><th>RAoff</th><th>RAsigS</th> \
               <th>RAsigG</th><th>Decoff</th><th>DecsigS</th><th>DecsigG</th></tr>"
    set index [list 0 1 2 3 4 5 6]
    loop i 0 $Nmatches {     
       set MaxOffset 0.2
       set line "<tr> <th> <A HREF=file://$filenameA($i)> $name($i) </th>"
       loop j 0 6 {
      if {$j == 6} {set MaxOffset 90.0}
          set offset [lindex $listA($i) [lindex $index $j]]
          if {[expr abs($offset)] > $MaxOffset} {
             set warning 1
             set line "$line <th><font color=\"red\"> ${offset}* </font> </th>"
          } else {
             set line "$line <th> $offset </th>"
          }
       }
       set line "$line </tr>"
       puts $outf $line
    }
    puts $outf "</table>" 
    if {$warning} {
        puts $outf "<br> <font color=\"red\"> *Red entries are triggered by \
                    entry > $MaxOffset</font>"
    }
    puts $outf "<br> The entries are listed in arcsec for sources with mmod(r) \<20."
    puts $outf "<br> The astrometric offsets are calculated for stars"
    #puts $outf "<br> The last column lists the number of galaxies with 19 \< mmod(r) \<20"
    #puts $outf "<br> as a percentage of the number of galaxies with mmod(r)\<20."

  
    puts $outf "<hr>"
    close $outf

}



proc make_html_table {outf title MaxOffsets Nmatches _name _filename _list listentry \
                     {print_mean 1}} {

upvar $_filename filename $_name name $_list list

    set warning 0
    set filters {u g r i z}
    puts $outf "<P>"
    puts $outf "<table border cellspacing=0 cellpadding=5>"
    puts $outf "<tr> <td colspan=0 rowspan=1></td>"
    puts $outf "<th colspan=6 align=center><font color=\"blue\"> \
                $title </font></th> </tr>"
    puts $outf "<tr><th>band:</th><th>u</th><th>g</th><th>r</th><th>i</th><th>z</th></tr>"
    loop i 0 $Nmatches {     
       set line "<tr> <th> <A HREF=file://$filename($i)> $name($i) </th>"
       foreach f $filters {
          set ifilter [lsearch $filters $f]
      if {$i == 0} {
              set mean($f) 0
      }
      if {$listentry < 0} {
             set offset [lindex $list($i) [expr abs(1+$listentry)]]
             set MaxOffset [lindex $MaxOffsets 0]
      } else {
             set offset [lindex [lindex $list($i) $ifilter] $listentry]
             set MaxOffset [lindex $MaxOffsets $ifilter]
          }
          set mean($f) [expr $mean($f) + 1.0*$offset/$Nmatches]
          if {[expr abs($offset)] > $MaxOffset} {
             set warning 1
             set line "$line <th><font color=\"red\"> ${offset}* </font> </th>"
          } else {
             set line "$line <th> $offset </th>"
          }
       }
       set line "$line </tr>"
       puts $outf $line
    }
    if {$print_mean} {
       set line "<tr> <th> mean value </th>"
       foreach f $filters {
          set ifilter [lsearch $filters $f]
          set offset [format "%4.2f" $mean($f)]
          if {[expr abs($offset)] > [lindex $MaxOffsets $ifilter]} {
             set warning 1
             set line "$line <th><font color=\"red\"> ${offset}* </font> </th>"
          } else {
             set line "$line <th> $offset </th>"
          }          
       }
       set line "$line </tr>"
       puts $outf $line
    } 
    puts $outf "</table>" 
    if {$warning} {
    if {$listentry < 0} {
            puts $outf "<br> <font color=\"red\"> *Red entries are triggered by \
                    entry > $MaxOffset </font>"
        } else {
            puts $outf "<br> <font color=\"red\"> *Red entries are triggered by \
                    entry > ($MaxOffsets) in ($filters) </font>"
        }
    }
   
}


# plot various statistics for a chain of PHOTOMATCHs and return a list
# {stats(u) stats(g) ... stats(z)} 
# where each stats is a list of 
# {offset sigma sigmachi}
proc plot_match_stats_photo {device chain {mag_type psfCounts} {title ""} \
                            {filename ""}} {

     set Ndata [chainSize $chain]
     if {$mag_type == "psfCounts"} {
         set type psf
         set typename psfCounts
     } else { 
         set type mod
         set typename counts_model
     }
     set filterlist {u g r i z} 
     set Nbins 60; set mmin 14.0; set mmax 26.0 
     set NbinsErr 24.0
     set allstats {}

     set opdb 1

     loop i 0 5 { 
         set f [lindex $filterlist $i]
         # photometry
         if {!$opdb} {
           set reddcorr [vFromChain $chain reddening<$i>]
           set m1 [vFromChain $chain ${mag_type}<$i>]
           set e1 [vFromChain $chain ${mag_type}Err<$i>]
           set m2 [vFromChain $chain ${mag_type}B<$i>]
           set e2 [vFromChain $chain ${mag_type}ErrB<$i>]
         } else {
           set m1 [vFromChain $chain obj1->detection<$i>.$typename.val]
           set e1 [vFromChain $chain obj1->detection<$i>.$typename.err]
           set m2 [vFromChain $chain obj2->detection<$i>.$typename.val]
           set e2 [vFromChain $chain obj2->detection<$i>.$typename.err]
           set reddcorr1 [vFromChain $chain obj1->reddening<$i>]
           set reddcorr2 [vFromChain $chain obj2->reddening<$i>]
           set reddcorr [vectorExprEval 0.5*($reddcorr1+$reddcorr2)]
           set redd_diff [vectorExprEval $reddcorr2-$reddcorr1)]
       if {[vExtreme $redd_diff min] < -0.01 || \
                    [vExtreme $redd_diff max] >  0.01} {
               error "Reddening differs for more than 0.01 mag!?!"  
       }
           vectorExprDel $reddcorr1; vectorExprDel $reddcorr2
           vectorExprDel $redd_diff
         }
         set meanmag($f) [vectorExprEval 0.5*($m1+$m2)-$reddcorr]
         set magdif($f) [vectorExprEval $m1-$m2]
         set err($f) [vectorExprEval 0.001+sqrt($e1*$e1+$e2*$e2)]
         set goodmagcond "$m1 > 14 && $m2 > 14 && $m1 < 25 && $m2 < 25 && $err($f) < 5"
         set goodmag($f) [vectorExprEval "$goodmagcond ? 1 : 0"] 
         vectorExprDel $m1; vectorExprDel $m2
         vectorExprDel $e1; vectorExprDel $e2
         vectorExprDel $reddcorr

         ## bright sources
         set mbright 19.0
         set mask($f) [vectorExprEval "$meanmag($f) < $mbright ? $goodmag($f) : 0"]
         # mean offset
         set gooderrcond "$magdif($f) < 0.4 && $magdif($f)" 
         set mask2 [vectorExprEval "$gooderrcond ? $mask($f) : 0"]
         if [catch {
                    set offset($f) [format "%4.2f" [vMean $magdif($f) -vMask $mask2]]
               }] {
             set offset($f) 0.0
         }
         set corrdiff [vectorExprEval $magdif($f)-$offset($f)]
         if [catch {
                    set sigma($f) [format "%4.2f" [vSigma $corrdiff -vMask $mask2]] 
               }] {
             set sigma($f) 0.0
         }            
         vectorExprDel $corrdiff; vectorExprDel $mask2
         set chi($f) [vectorExprEval ($magdif($f)-$offset($f))/$err($f)]
     }

     # plot
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         if {$filename == ""} {
             set filename $type-mag-errors.ps
         }
         set deviceA [devicename $device $filename] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -isLine 0 \
           -xfract 0.55 -yfract 0.4 -nxwindow 4 -nywindow 8 -symb 1
         set icLineDef [exprGet $pg.icLine]
         pgstateOpen $pg

         # plotting for each band
         loop i 0 5 { 
            set f [lindex $filterlist $i]
        set stats {}

            # mag difference
            pgstateSet $pg -icMark 4
            vPlot $pg $meanmag($f) $magdif($f) -vectorMask $goodmag($f) \
                      -xmin 14 -xmax 23 -ymin -0.32 -ymax 0.32
            pgstateSet $pg -icMark 6
            pgstateSet $pg -isNewplot 0
            vPlot $pg $meanmag($f) $magdif($f) -vectorMask $mask($f) \
                      -xmin 14 -xmax 23 -ymin -0.32 -ymax 0.32

            lappend stats $offset($f)
            lappend stats $sigma($f)
            plot_info "$offset($f)" 90.0 0.2 0.3
            plot_line $pg 10 $offset($f) 30 $offset($f) 1 2
            plot_line $pg 10 0.0 30 0.0 1 1 
            plot_info "$f" 0.0 0.15 0.0

        if {$i == 2} {ylabel "$type mag(1) - $type mag(2)"}
        if {$i == 4} {xlabel "mean $type mag"}
            # title
        if {$i == 0} {
                titlePlot "$title ($type, N = $Ndata)" 20
        }
            if {$i == 4} {
                plot_info "The magenta points are bright sources selected by mag<$mbright" \
                           0.0 -1.2 -1.5
                plot_info "The values in the 1st column are mean offsets (bright)" \
                           0.0 -1.2 -1.85
                plot_info "The values in the 2nd column are mean rms scatter (bright)" \
                           0.0 -1.2 -2.2
                plot_info "The values in the 4th column are mean chi scatter (all)" \
                           0.0 -1.2 -2.55
            } 
            pgstateSet $pg -isNewplot 1

            # mag. diff. histogram
            set hist [vecToHist $magdif($f) 50 {-0.25 0.25} $mask($f)]
            if {$i == 4} {handleSet $hist.xLabel "mag. diff."}
            handleSet $hist.yLabel "N" 
            pgstateSet $pg -icLine 6
            hgPlot $pg $hist -xmin -0.3 -xmax 0.3
            pgstateSet $pg -icLine $icLineDef
            hgDel $hist
            plot_info "$sigma($f)" 90.0 0.2 0.3
    

            ## chi difference 
            # all sources
            pgstateSet $pg -icMark 4
            vPlot $pg $meanmag($f) $chi($f) -vectorMask $goodmag($f) \
                      -xmin 14 -xmax 23 -ymin -2.5 -ymax 2.5 
            pgstateSet $pg -icMark $icLineDef
            # all sources
            pgstateSet $pg -isNewplot 0
            pgstateSet $pg -icMark 6
            vPlot $pg $meanmag($f) $chi($f) -vectorMask $mask($f) \
                      -xmin 14 -xmax 23 -ymin -2.5 -ymax 2.5 
            plot_line $pg 10 0.0 30 0.0 1 1 
        if {$i == 2} {ylabel "chi = magdiff/sqrt(err(1)**2+err(2)**2)"}
        if {$i == 4} {xlabel "mean $type mag"}
            # width for bright sources
            set sigchi [format "%4.2f" [vSigma $chi($f) -vMask $mask($f)]] 
            plot_info "bw: $sigchi" 90.0 0.2 0.0
            pgstateSet $pg -icMark $icLineDef -isNewplot 1


            ## chi histogram
            # all sources
            set hist [vecToHist $chi($f) 50 {-2.5 2.5} $goodmag($f)]
            if {$i == 4} {handleSet $hist.xLabel "chi"}
            handleSet $hist.yLabel "N" 
            pgstateSet $pg -icLine 4
            hgPlot $pg $hist -xmin -2.5 -xmax 2.5
            hgDel $hist
            set sigchi [format "%4.2f" [vSigma $chi($f) -vMask $goodmag($f)]] 
            lappend stats $sigchi
            plot_info "all: $sigchi" 90.0 0.2 0.0
            pgstateSet $pg -icLine $icLineDef

            # clean
            vectorExprDel $mask($f)
            vectorExprDel $meanmag($f)
            vectorExprDel $magdif($f)
            vectorExprDel $err($f)
            vectorExprDel $chi($f)   
         
            lappend allstats $stats        
         }  
         endplot $pg
     } msg] {
         echo "$msg"
     }

     return $allstats
     
}


# plot various statistics for a chain of PHOTOMATCHs
# and return a list of 
# {RAoffset RAstarSigma RAgalaxySigma Decoffset DecstarSigma DecgalaxySigma fracGal}
# where fracGal is the number of galaxies with 19 < mmod(r) < 20 as a fraction of 
# the total number of galaxies brighter than mmod(r) = 20 
proc plot_match_stats_astro {device chain {mag_type psfCounts} {title ""} \
                            {filename "plot_match_stats_astro"}} {

     set Ndata [chainSize $chain]
     if {$mag_type == "psfCounts"} {
         set type psf
         set typename psfCounts
     } else { 
         set type mod
         set typename counts_model
     }
     set filterlist {u g r i z} 
     set Nbins 60; set mmin 14.0; set mmax 26.0 
     set NbinsErr 24.0; set f r; set i 2 

     set opdb 1
   
        
         # get data     
         if {!$opdb} {
            set reddcorr [vFromChain $chain reddening<$i>]
            set m1 [vFromChain $chain ${mag_type}<$i>]
            set m2 [vFromChain $chain ${mag_type}B<$i>]
            set mtypeA [vFromChain $chain objc_type]
            set mtypeB [vFromChain $chain objc_typeB]
            set ra [vFromChain $chain ra]; set dec [vFromChain $chain dec]; 
            set raB [vFromChain $chain raB]; set decB [vFromChain $chain decB]; 
         } else {
            set reddcorr [vFromChain $chain obj1->reddening<$i>]
            set m1 [vFromChain $chain obj1->detection<$i>.$typename.val]
            set m2 [vFromChain $chain obj2->detection<$i>.$typename.val]
            set mtypeA [vFromChain $chain obj1->objc_type]
            set mtypeB [vFromChain $chain obj2->objc_type]
            set ra [vFromChain $chain obj1->ra]; set dec [vFromChain $chain obj1->dec]; 
            set raB [vFromChain $chain obj2->ra]; set decB [vFromChain $chain obj2->dec]; 
          } 

         set meanmag($f) [vectorExprEval 0.5*($m1+$m2)-$reddcorr]

         # other
         if {$i == 2} {
            # S/G separation
            set SS [vectorExprEval "$mtypeA == 6 && $mtypeB == 6 ? 1 : 0"]
            set GG [vectorExprEval "$mtypeA == 3 && $mtypeB == 3 ? 1 : 0"]
            set NS [vectorExprEval "$mtypeA + $mtypeB == 9 ? 1 : 0"]   
            set bSS [vectorExprEval "$meanmag($f) < 20 ? $SS : 0"] 
            set bGG [vectorExprEval "$meanmag($f) < 20 ? $GG : 0"] 
            set bGG1 [vectorExprEval "$meanmag($f) > 19 ? $bGG : 0"] 
        if {0} {
               set nGG [vNnonzero $bGG]
               set nGG1 [vNnonzero $bGG1]
            } else {
               set nGG 1
               set nGG1 1
            }
            set fracG [format "%4.1f" [expr 100.0*$nGG1/$nGG]]
            set HSS [vecToHist $meanmag($f) $Nbins [list $mmin $mmax] $SS]
            set HGG [vecToHist $meanmag($f) $Nbins [list $mmin $mmax] $GG]
            set HNS [vecToHist $meanmag($f) $Nbins [list $mmin $mmax] $NS]
            set LHSS [hgLog10 $HSS]; set LHGG [hgLog10 $HGG]; set LHNS [hgLog10 $HNS];
            handleSet $LHGG.xLabel "mmod($f)"; handleSet $LHGG.yLabel "log10 (N)" 
            # ra & dec differences (XXX this should be eta and lambda!!!)
            set delra [vectorExprEval 3600*($ra-$raB)]
            set deldec [vectorExprEval 3600*($dec-$decB)]
            set raHS [vecToHist $delra 50 [list -0.5 0.5] $bSS]
            set raHG [vecToHist $delra 50 [list -0.5 0.5] $bGG]
            set decHS [vecToHist $deldec 50 [list -0.5 0.5] $bSS]
            set decHG [vecToHist $deldec 50 [list -0.5 0.5] $bGG]
            handleSet $raHS.xLabel "RA1 - RA2 (arcsec)"
            handleSet $decHS.xLabel "Dec1 - Dec2 (arcsec)"
            handleSet $raHS.yLabel "N"
            handleSet $decHS.yLabel "N"
            set medianSra  [format "%4.2f" [vMedian $delra -vMask $bSS]] 
            set sigmaSra [format "%4.2f" [vSigma $delra -vMask $bSS]] 
            set sigmaGra [format "%4.2f" [vSigma $delra -vMask $bGG]] 
            set medianSdec  [format "%4.2f" [vMedian $deldec -vMask $bSS]] 
            set sigmaSdec [format "%4.2f" [vSigma $deldec -vMask $bSS]] 
            set sigmaGdec [format "%4.2f" [vSigma $deldec -vMask $bGG]] 
            # CHILDREN differences
         if {!$opdb} {
        set children1 [chainSearchByFlags $chain CHILD {} "" 1]
        set children2 [chainSearchByFlags $chain CHILD {} "" 2]
        set children12 [chainSearchByFlags $children1 CHILD {} "" 2]
        foreach c [list $children1 $children2 $children12] {
                set reddccorr [vFromChain $c reddening<$i>]
                set mc1 [vFromChain $c ${mag_type}<$i>]
                set mc2 [vFromChain $c ${mag_type}B<$i>]
                set meancmag [vectorExprEval 0.5*($mc1+$mc2)-$reddccorr]
                set Hc($c) [vecToHist $meancmag 11 [list 14.0 25.0]]
                set LHc($c) [hgLog10 $Hc($c)]
                vectorExprDel $mc1; vectorExprDel $mc2
                vectorExprDel $meancmag; vectorExprDel $reddccorr
            }
     }
            vectorExprDel $ra; vectorExprDel $raB; vectorExprDel $delra 
            vectorExprDel $dec; vectorExprDel $decB; vectorExprDel $deldec
            vectorExprDel $bSS; vectorExprDel $bGG    
            vectorExprDel $bGG1     
            vectorExprDel $mtypeA; vectorExprDel $mtypeB 
         }
       
         # clean
         vectorExprDel $m1; vectorExprDel $m2
         vectorExprDel $meanmag($f); vectorExprDel $reddcorr
   
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         if {$filename == ""} {
             set filename astroETC.ps
         }
         set deviceA [devicename $device $filename] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -isLine 1 \
           -xfract 0.35 -yfract 0.35 -nxwindow 2 -nywindow 3 
         set icLineDef [exprGet $pg.icLine]
         pgstateOpen $pg

         # panel 1
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raHS -xmin -0.5 -xmax 0.5
         titlePlot "$title ($Ndata)" 20
         pgstateSet $pg -isNewplot 0 -icLine 4
         hgPlot $pg $raHG -xmin -0.5 -xmax 0.5
         plot_info "med S: $medianSra" 90.0 0.1 -0.2
         plot_info "sig S: $sigmaSra" 90.0 0.1 0.5
         plot_info "sig G: $sigmaGra" 90.0 0.2 0.05

         # panel 2
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decHS -xmin -0.5 -xmax 0.5
         titlePlot "black: S, blue: G, mod(r)<20" 40 
         pgstateSet $pg -isNewplot 0 -icLine 4
         hgPlot $pg $decHG -xmin -0.5 -xmax 0.5
         plot_info "med S: $medianSdec" 90.0 0.1 -0.2
         plot_info "sig S: $sigmaSdec" 90.0 0.1 0.5
         plot_info "sig G: $sigmaGdec" 90.0 0.2 0.05

         # panel 3
         pgstateSet $pg -isNewplot 1 -icLine 4
         hgPlot $pg $LHGG -xmin $mmin -xmax $mmax 
         titlePlot "black: SS, blue: GG, red: diff." 40 
         pgstateSet $pg -isNewplot 0 -icLine $icLineDef
         hgPlot $pg $LHSS -xmin $mmin -xmax $mmax 
         pgstateSet $pg -isNewplot 0 -icLine 2
         hgPlot $pg $LHNS -xmin $mmin -xmax $mmax 
         #plot_info "G(19-20)/G(<20): $fracG%" 90.0 0.1 -0.1

         plot_info "The top two panels: astrometric accuracy" \
                    0.0 -1.0 -0.5
         plot_info "The bottom left panel: repeatability of S/G separation" \
                    0.0 -1.0 -0.65
         if {!$opdb} {
            plot_info "The bottom right panel: repeatability of deblending" \
                    0.0 -1.0 -0.80 
         }

    
       if {!$opdb} {
         # panel 4
         handleSet $LHc($children2).xLabel "mmod($f)"
         handleSet $LHc($children2).yLabel "log10 (N)" 
         pgstateSet $pg -isNewplot 1 -icLine 4
         hgPlot $pg $LHc($children2) -xmin $mmin -xmax $mmax 
         titlePlot "black: c1, blue: c2, red: c12" 40 
         pgstateSet $pg -isNewplot 0 -icLine $icLineDef
         hgPlot $pg $LHc($children1) -xmin $mmin -xmax $mmax 
         pgstateSet $pg -isNewplot 0 -icLine 2
         hgPlot $pg $LHc($children12) -xmin $mmin -xmax $mmax 
         plot_info "CHILDREN COUNTS" 90.0 0.1 0.0
 
     foreach c [list $children1 $children2 $children12] {
            hgDel $Hc($c) 
            hgDel $LHc($c)
            chainDel $c 
         }
       }
       
       

         endplot $pg
     } msg] {
         echo "$msg"
     }

     vListDel [list $SS $GG $NS]
     hgDel $HSS; hgDel $HGG; hgDel $HNS 
     hgDel $LHSS; hgDel $LHGG; hgDel $LHNS 
     hgDel $raHS; hgDel $raHG; hgDel $decHS; hgDel $decHG;

  return [list $medianSra $sigmaSra $sigmaGra $medianSdec $sigmaSdec $sigmaGdec $fracG]
     
}



# plot various QA diagrams for sources with field' such that 
# field-half_win <= field' <= field+half_win
proc plot_overlap_field {match_file device field filter \
                         {half_win 0} {chain ""} {print 1}} {

     # only sources brighter than this are considered
     set mmax [get_parameter mmax]
     # only sources with errors less than this are considered
     set emax [get_parameter emax]

     set filterlist {u g r i z} 
     set ifilter [lsearch $filterlist $filter]

     # read chain from input file and make vectors
     set readChain 0
     if {$chain == ""} {
        set chain [param2Chain $match_file ""]
        set readChain 1
     }
     set fields [vFromChain $chain field]
     set mIDs [vFromChain $chain matchID]
     set type(0) "psfCounts"
     set type(1) "counts_model"
     loop i 0 2 {
        set mag_type $type($i)
        set m1($i) [vFromChain $chain ${mag_type}<$ifilter>]
        set e1($i) [vFromChain $chain ${mag_type}Err<$ifilter>]
        set m2($i) [vFromChain $chain ${mag_type}B<$ifilter>]
        set e2($i) [vFromChain $chain ${mag_type}ErrB<$ifilter>]
        set mmean($i) [vectorExprEval 0.5*($m1($i)+$m2($i))]
        set mdiff($i) [vectorExprEval $m2($i)-$m1($i)]
        set emean($i) [vectorExprEval sqrt($e1($i)*$e1($i)+$e2($i)*$e2($i)+0.0001)]
     }
     set Ndata [chainSize $chain]
     if {$readChain} {genericChainDestroy $chain}

     # difference between the two magnitude types
     set delm1 [vectorExprEval $m1(0)-$m1(1)]
     set delm2 [vectorExprEval $m2(0)-$m2(1)]

     # find this field(s) and take care of the magnitude/error limit
     set fmin [expr $field - $half_win]
     set fmax [expr $field + $half_win]
     set mask_aux1 [vectorExprEval "$fields >= $fmin && $fields <= $fmax ? 1 : 0"]
     echo "There [vNnonzero $mask_aux1] sources in this field's overlap"
     set mask_aux2 [vectorExprEval "$mmean(0) < $mmax ? $mask_aux1 : 0"]
     echo "[vNnonzero $mask_aux2] pass magnitude cut"
     set mask [vectorExprEval "$emean(0) < $emax ? $mask_aux2 : 0"]
     vectorExprDel $mask_aux1; vectorExprDel $mask_aux2; 
     echo "and [vNnonzero $mask] pass error cut"

     # print a table with all info
     if {$print} {
        echo "    match   mmpsf  dmpsf  mmmod  dmmod dpsfmod1 dpsfmod2"   
        loop i 0 $Ndata {
       if {[exprGet $mask.vec<$i>] == 1} {
              set mID [format "%7d" [exprGet $mIDs.vec<$i>]]
              loop k 0 2 {
                 set mm($k) [format "%6.3f" [exprGet $mmean($k).vec<$i>]]
                 set md($k) [format "%6.3f" [exprGet $mdiff($k).vec<$i>]]
              }
              set dm1 [format "%6.3f" [exprGet $delm1.vec<$i>]]
              set dm2 [format "%6.3f" [exprGet $delm2.vec<$i>]]
              echo "  $mID  $mm(0) $md(0) $mm(1) $md(1)  $dm1   $dm2"
           }   
        } 
     }

     # plot
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set device [devicename $device ${match_file}_frame${field}$filter.ps] 
         pgstateSet $pg -device $device -lineWidth 1 -symb 0 -icError 1 \
          -xfract 0.4 -yfract 0.35 -nxwindow 2 -nywindow 2 
         pgstateOpen $pg
         titlePlot "field ${field} filter $filter from file ${match_file}" 20        
 
         # delm vs. mean   
         loop i 0 2 {
            set ymin($i) [vExtreme $mdiff($i) -vMask $mask min]
        if {$ymin($i) > -0.02} {set ymin($i) -0.02} 
            set ymax($i) [vExtreme $mdiff($i) -vMask $mask max]
        if {$ymax($i) < 0.02} {set ymax($i) 0.02} 
            vPlot $pg $mmean($i) $mdiff($i) -vectorMask $mask \
                      -vectorXErr $emean($i) -vectorYErr $emean($i) \
                      -xmin 14 -xmax $mmax -ymin $ymin($i) -ymax $ymax($i) 
            # labels and title          
        xlabel "mean $type($i) mag."                   
        ylabel "$type($i) mag. diff."
            plot_line $pg 10 0.0 30 0.0 1 1 
         }

         # delmag_model vs. delmag_psf
         vPlot $pg $mdiff(0) $mdiff(1) -vectorMask $mask \
                      -vectorXErr $emean(0) -vectorYErr $emean(1) \
                      -xmin $ymin(0) -xmax $ymax(0) -ymin $ymin(1) -ymax $ymax(1)
     xlabel "psf magnitude difference"                   
     ylabel "model magnitude difference"
         plot_line $pg $ymin(0) 0.0 $ymax(0) 0.0 1 1 
         plot_line $pg 0.0 -1 0.0 1 1 1 

         # (psf-model)2 - (psf-model)1
         set i 3
         set ymin($i) [vExtreme $delm2 -vMask $mask min]
     if {$ymin($i) > -0.02} {set ymin($i) -0.02} 
         set ymax($i) [vExtreme $delm2 -vMask $mask max]
     if {$ymax($i) < 0.02} {set ymax($i) 0.02} 
         set i 4
         set ymin($i) [vExtreme $delm1 -vMask $mask min]
     if {$ymin($i) > -0.02} {set ymin($i) -0.02} 
         set ymax($i) [vExtreme $delm1 -vMask $mask max]
     if {$ymax($i) < 0.02} {set ymax($i) 0.02} 

         vPlot $pg $delm1 $delm2 -vectorMask $mask \
               -xmin $ymin(4) -xmax $ymax(4) -ymin $ymin(3) -ymax $ymax(3) 
     xlabel "(psf - model)1"                   
     ylabel "(psf - model)2"
         plot_line $pg -1 0.0 1 0.0 1 1 
         plot_line $pg 0.0 -1 0.0 1 1 1 
         plot_line $pg -0.1 -0.1 1.6 1.6 1 1 
         
         endplot $pg
     } msg] {
         echo "$msg"
     }



     # clean
     vectorExprDel $fields; vectorExprDel $mask
     vectorExprDel $delm1; vectorExprDel $delm2
     loop i 0 2 {
        vectorExprDel $m1($i); vectorExprDel $m2($i)
        vectorExprDel $e1($i); vectorExprDel $e2($i)
        vectorExprDel $mmean($i)
        vectorExprDel $mdiff($i)
        vectorExprDel $emean($i)
     }

     return 0
     
}




proc plot_line {pg x1 y1 x2 y2 {lw 1} {icL 1}} {

     set x [vectorExprNew 2]
     set y [vectorExprNew 2]
     handleSet $x.vec<0> $x1; handleSet $x.vec<1> $x2; 
     handleSet $y.vec<0> $y1; handleSet $y.vec<1> $y2; 

     set preds [list isNewplot isLine lineWidth icLine]
     foreach v $preds {
        set pred($v) [exprGet $pg.$v] 
     }

     pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth $lw -icLine $icL
     vPlot $pg $x $y

     foreach v $preds {
         pgstateSet $pg -$v $pred($v)      
     }

     vectorExprDel $x 
     vectorExprDel $y

} 

# aux proc for plot_overlaps: print mean or rms value for array y 
# only values with mask=1 are considered
proc vPlotStats {y mask {type "mean"} {xscale 0.15} {yscale 0.0}} {


    set q xxxxxx
    if {$type == "mean"} {
       if {$mask != ""} {
          catch {set q [format "%4.3f" [vMean $y -vMask $mask]]}
       } else {
          catch {set q [format "%4.3f" [vMean $y]]}
       }
       set qstr "<>=$q"
    } else {
       if {$mask != ""} {
          catch {set q [format "%4.3f" [vSigma $y -vMask $mask]]}
       } else {
          catch {set q [format "%4.3f" [vSigma $y]]}
       }
       set qstr "sig=$q"
    }

    plot_info $qstr 90.0 $xscale $yscale
   
    return $q

}


proc plot_info {q {angle 0.0} {xscale 0.15} {yscale 0.0}} {

    set win [lindex [pgQwin] 0] 
    set xmin [lindex $win 0]
    set xmax [lindex $win 1]
    set ymin [lindex $win 2]
    set ymax [lindex $win 3]
    set x [expr $xmax+$xscale*($xmax-$xmin)]
    set y [expr $ymin+$yscale*($ymax-$ymin)]
     
    pgPtext $x $y $angle 0.0 $q

}


# given two-epoch photometry for a set of sources, and an array of 
# fields, farray, for each field in the range f1-f2 determine median 
# m1-m2, rms for m1-m2, and chi = <(delm-delm_mean)/sigma>, where
# sigma = sqrt(e1^2 + e2^2). 
# use only data with the mean magnitude brighter than mmax and
# sigma < emax. Clip by limiting m1-m2 to mdifmax.
# When there is less than Nmin good sources for
# a given field, median, sigma and aux are set to 0.
# One can use a box-car average over many fields by specifying half_win > 0.
proc mdiff_stats {farray f1 f2 m1 e1 m2 e2 mmax emax Nmin mdifmax {half_win 0}} {

     set Nfields [expr int($f2 - $f1 + 1)]
     set fields [vectorExprNew $Nfields]
     set Ndata [vectorExprNew $Nfields]
     set median [vectorExprNew $Nfields]
     set sigma [vectorExprNew $Nfields]
     set chi [vectorExprNew $Nfields]
     set fmask [vectorExprNew $Nfields]
     if {$half_win < 0} {set half_win 0}
 
     # aux vectors
     set mmean [vectorExprEval 0.5*($m1+$m2)]
     set mdiff [vectorExprEval $m2-$m1]
     set emean [vectorExprEval sqrt($e1*$e1+$e2*$e2+0.0001)]
     # master mask takes care of the magnitude/error limits
     set mmask_aux1 [vectorExprEval "$mmean < $mmax ? 1 : 0"]
     set mmask_aux2 [vectorExprEval "$mdiff < $mdifmax ? $mmask_aux1 : 0"]
     set mmask [vectorExprEval "$emean < $emax ? $mmask_aux2 : 0"]
     vectorExprDel $mmask_aux1
     vectorExprDel $mmask_aux2

     # loop over each field
     loop f $f1 [expr $f2 + 1] {
        set f0 [expr $f - $f1]
        handleSet $fields.vec<$f0> $f
        # set mask for this field
        set fmin [expr $f - $half_win]
        set fmax [expr $f + $half_win]
        set mask [vectorExprEval "$farray >= $fmin && $farray <= $fmax ? $mmask : 0"]
        # calculate all quantities
        handleSet $Ndata.vec<$f0> [vNnonzero $mask]
        # there must be at least Nmin sources
        if {[exprGet $Ndata.vec<$f0>] < $Nmin} {
           handleSet $fmask.vec<$f0> 0
           handleSet $median.vec<$f0> 0.0
           handleSet $sigma.vec<$f0> 0.0
           handleSet $chi.vec<$f0> 0.0 
        } else {  
           handleSet $fmask.vec<$f0> 1
           set mean_aux 0
           catch {
              set median_aux [vMedian $mdiff -vMask $mask]
           }
           handleSet $median.vec<$f0> $median_aux
           set sigma_aux 0
           catch {
              set sigma_aux [vSigma $mdiff -vMask $mask]
           }
           handleSet $sigma.vec<$f0> $sigma_aux 
           set ichi [vectorExprEval ($mdiff-$median_aux)/$emean] 
           set chi_aux 0.0
           catch {
             set chi_aux [vSigma $ichi -vMask $mask]
           } 
           handleSet $chi.vec<$f0> $chi_aux
           vectorExprDel $ichi 
        }
        vectorExprDel $mask
     }

     # clean
     vectorExprDel $mmean  
     vectorExprDel $mdiff 
     vectorExprDel $emean
     vectorExprDel $mmask
  
     return [list $fields $Ndata $median $sigma $chi $fmask]
}


# Given 2 vectors for a set of sources, q1 and q2, a vector of bin 
# centers for the first array, bins, and their halfwidth, binHw, for 
# each bin(i) find all sources that satisfy 
# bin(i)-Hw < q1 < bin(i)+Hw, determine their median and sigma.
# Only sources that have qmask = 1 should be considered. 
# If there isn't at least Nmin sources in a given bin, set median
# and sigma to qdefault and for such bins set mask to 0.
# Return these new vectors, and the number of sources in each bin,
# as a list {N median sigma mask}.
proc binned_stats {q1 q2 qmask bins binHw Nmin qdefault} {

# global mem

     set Nbins [exprGet $bins.dimen]
     set Nbin [vectorExprNew $Nbins]
     set median [vectorExprNew $Nbins]
     set sigma [vectorExprNew $Nbins]
     set mask [vectorExprNew $Nbins]

     # loop over bins
     loop i 0 $Nbins {
        set q1min [expr [exprGet $bins.vec<$i>] - $binHw]
        set q1max [expr [exprGet $bins.vec<$i>] + $binHw]
        set binmask [vectorExprEval "$q1 >= $q1min && $q1 <= $q1max ? $qmask : 0"]
        # do we pass Nmin condition?
        set N [vNnonzero $binmask]      
        if {$N < $Nmin} {
           set med $qdefault; set sig $qdefault; set m 0;
        } else {
           # calculate median and sigma
           # write_mem mem "binned_stats: med bin $i" 
           set med [vMedian $q2 -vMask $binmask]
           # write_mem mem "binned_stats: sig bin $i" 
           set sig [vSigma $q2 -vMask $binmask]
           set m 1

        }
        # copy this bin's values to output vectors
        handleSet $Nbin.vec<$i> $N
        handleSet $median.vec<$i> $med
        handleSet $sigma.vec<$i> $sig 
        handleSet $mask.vec<$i> $m 
        vectorExprDel $binmask
     }

     return [list $Nbin $median $sigma $mask]

}


# return the number of elements in a vector which are != 0
proc vNnonzero {vector} {

    set N 0
    loop i 0 [exprGet $vector.dimen] {
       if {[exprGet $vector.vec<$i>] != 0} {incr N}
    }
    return $N

}

# return the number of elements in a vector which satisfy
# a <= vector <= b
proc vNinrange {vector a b} {

    set N 0
    loop i 0 [exprGet $vector.dimen] {
       set v [exprGet $vector.vec<$i>]
       if {$a <= $v && $v <= $b} {incr N}
    }
    return $N

}


# this proc analyzes output list from mdiff_stats
# it returns a list of lists {field Ndata median sigma chi} 
# for all fields which satisfy the following
# 1) 
proc analyze_stats {stats med_max sig_max chi_max} {

     # parameters:
     # max. deviation for the median of the magnitude difference
     set max(med) $med_max
     # max. deviation for the sigma of the magnitude difference
     set max(sig) $sig_max
     # max. deviation for the sigma of chi distribution
     set max(chi) $chi_max

     # unpack stats list         
     set fields [lindex $stats 0]
     set v(Ndata) [lindex $stats 1]
     set v(med) [lindex $stats 2]
     set v(sig) [lindex $stats 3]
     set v(chi) [lindex $stats 4]

     set problem_list {}
     loop i 0 [exprGet $fields.dimen] {
        set fieldOK 1
        foreach t {med sig chi} {
           set q [expr abs([exprGet $v($t).vec<$i>])]
           if {$q > $max($t)} {set fieldOK 0} 
        }
        if {!$fieldOK} {
           set aux [exprGet $fields.vec<$i>]
           foreach t {Ndata med sig chi} {
              set aux "$aux [exprGet $v($t).vec<$i>]"
           }
           lappend problem_list $aux
        }
     }

     return $problem_list
}







#############
### aux procs

# print a PHOTOMATCH entry 
proc fetch_id {file id {type PHOTOMATCH} {mag_type psf}} {
    
    catch {set line [exec fgrep "$type $id " $file]}

    if {![info exist line] || $line == ""} {
        echo " * Can't find source $id in file $file *"
    } else {
        show_$type $line $mag_type 
    }
    
}

proc show_PHOTOMATCH {data {mag_type psf}} {
    
    set idn [lindex $data 1]
    set r1 [lindex $data 2]
    set c1 [lindex $data 3]
    set f1 [lindex $data 4]
    set id1 [format "%4d" [lindex $data 5]]
    set row1 [format "%4d" [expr int([lindex $data 6])]]
    set col1 [format "%4d" [expr int([lindex $data 7])]]
    set ra1 [lindex $data 8]
    set dec1 [lindex $data 9]
    set objc_type1 [lindex $data 10]
    set r2 [lindex $data 18]
    set c2 [lindex $data 19]
    set f2 [lindex $data 20]
    set id2 [format "%4d" [lindex $data 21]]
    set row2 [format "%4d" [expr int([lindex $data 22])]]
    set col2 [format "%4d" [expr int([lindex $data 23])]]
    set ra2 [lindex $data 24]
    set dec2 [lindex $data 25]
    set objc_type2 [lindex $data 26]
    set oflag_1 [lindex $data 11]   
    set oflag_2 [lindex $data 27]  

    if {$mag_type == "psf"} {
        set mag(1) [lindex $data 13]
        set err(1) [lindex $data 14]
        set mag(2) [lindex $data 29]
        set err(2) [lindex $data 30]
   } else {
        set mag_type "model"
        set mag(1) [lindex $data 15]
        set err(1) [lindex $data 16]
        set mag(2) [lindex $data 31]
        set err(2) [lindex $data 32]
    }

    foreach e {1 2} {
       foreach T {m s} {
      if {$T == "m"} {
              set aux $mag($e)
      } else {
              set aux $err($e)
      }
          set ${T}u$e [lindex $aux 0]
          set ${T}g$e [lindex $aux 1]
          set ${T}r$e [lindex $aux 2]
          set ${T}i$e [lindex $aux 3]
          set ${T}z$e [lindex $aux 4]
       }
    }

    set delu [format %5.3f [expr $mu2 - $mu1]]
    set delg [format %5.3f [expr $mg2 - $mg1]]
    set delr [format %5.3f [expr $mr2 - $mr1]]
    set deli [format %5.3f [expr $mi2 - $mi1]]
    set delz [format %5.3f [expr $mz2 - $mz1]]

    # shift in position (for finding moving objects)
    set delRA [expr 3600.0*($ra2-$ra1)] 
    set delDec [expr 3600.0*($dec2-$dec1)] 
    # change in position (arcsec) 
    set delPosition [expr sqrt($delRA*$delRA + $delDec*$delDec)] 
     

    echo "-------------------------------------------------------------------------------------"  
    echo "OBJECT MATCH ID: $idn"
    echo " Run1 Column1 Frame1 id1  row1  col1      RA1       Dec1"
    echo " $r1    $c1      $f1  $id1  $row1  $col1  [format %3.7f $ra1] [format %3.7f $dec1]"
    echo ""
    echo " Run2 Column2 Frame2 id2  row2  col2      RA2       Dec2"
    echo " $r2    $c2      $f2  $id2  $row2  $col2  [format %3.7f $ra2] [format %3.7f $dec2]"
    echo ""
    echo "$mag_type magnitudes:"
    echo "1:     U                G                R                I                Z"
    echo "[format %5.3f $mu1] +- [format %5.4f $su1] [format %5.3f $mg1] +- [format %5.4f $sg1] [format %5.3f $mr1] +- [format %5.4f $sr1] [format %5.3f $mi1] +- [format %5.4f $si1] [format %5.3f $mz1] +- [format %5.4f $sz1]"
    set ug [format "%5.2f" [expr $mu1 - $mg1]]
    set gr [format "%5.2f" [expr $mg1 - $mr1]]
    set ri [format "%5.2f" [expr $mr1 - $mi1]]
    set iz [format "%5.2f" [expr $mi1 - $mz1]]
    echo " colors:  u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
    echo ""
    echo "2:     U                G                R                I                Z"
    echo "[format %5.3f $mu2] +- [format %5.4f $su2] [format %5.3f $mg2] +- [format %5.4f $sg2] [format %5.3f $mr2] +- [format %5.4f $sr2] [format %5.3f $mi2] +- [format %5.4f $si2] [format %5.3f $mz2] +- [format %5.4f $sz2]"
    set ug [format "%5.2f" [expr $mu2 - $mg2]]
    set gr [format "%5.2f" [expr $mg2 - $mr2]]
    set ri [format "%5.2f" [expr $mr2 - $mi2]]
    set iz [format "%5.2f" [expr $mi2 - $mz2]]
    echo " colors:  u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
  
    
    echo ""
    echo "Delta  U                G                R                I                Z "
    echo "     $delu            $delg            $delr            $deli            $delz"          
    echo " "
    echo " moved for [format "%6.3f" $delPosition] arcsec"
    echo " "

    echo ""; echo " objc_type(1) = $objc_type1"; echo " flags(1) = [flags $oflag_1]"  
    echo ""; echo " objc_type(2) = $objc_type2"; echo " flags(2) = [flags $oflag_2]"  

    echo "-------------------------------------------------------------------------------------"  
   
    
}

proc show_SDSS2MASS {data {mag_type psf}} {

       set id [lindex $data 5]
       set RA [format "%9.5f" [lindex $data 8]]
       set Dec [format "%9.5f" [lindex $data 9]]
       set flags [flags [lindex $data 10]]
       set objc_type [obj_type_translate [lindex $data 12]]
       echo ""; echo "Source at RA=$RA, Dec=$Dec, ID=$id, OBJC_TYPE=$objc_type"
       set row [format "%4d" [expr int([lindex $data 6])]]
       set col [format "%4d" [expr int([lindex $data 7])]]
       echo "          pixel coordinates: row = $row, col = $col"
       echo "          filter      mpsf          mmodel            type"
       loop i 0 5 {
          set f [lindex {u g r i z} $i]
      set ind1 [expr 18 + 2*$i]
      set mp($f) [format "%4.2f" [lindex $data [expr $ind1 + 0]]]
      set ep($f) [format "%4.2f" [lindex $data [expr $ind1 + 1]]]
      set mm($f) [format "%4.2f" [lindex $data [expr $ind1 + 10]]]
      set em($f) [format "%4.2f" [lindex $data [expr $ind1 + 11]]]
          set obj_type [obj_type_translate [lindex $data [expr 13 +$i]]]
          echo "             $f    $mp($f) +- $ep($f)   $mm($f) +- $em($f)      $obj_type"
       }
       set ug [format "%4.2f" [expr $mp(u)-$mp(g)]]
       set gr [format "%4.2f" [expr $mp(g)-$mp(r)]]
       set ri [format "%4.2f" [expr $mp(r)-$mp(i)]]
       set iz [format "%4.2f" [expr $mp(i)-$mp(z)]]
       echo "            psf: u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
       set ug [format "%4.2f" [expr $mm(u)-$mm(g)]]
       set gr [format "%4.2f" [expr $mm(g)-$mm(r)]]
       set ri [format "%4.2f" [expr $mm(r)-$mm(i)]]
       set iz [format "%4.2f" [expr $mm(i)-$mm(z)]]
       echo "            mod: u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
       echo "         flags = $flags"

       set J [format "%4.2f" [lindex $data 65]]
       set Jerr [format "%4.2f" [lindex $data 66]]
       set H [format "%4.2f" [lindex $data 67]]
       set Herr [format "%4.2f" [lindex $data 68]]
       set K [format "%4.2f" [lindex $data 69]]
       set Kerr [format "%4.2f" [lindex $data 70]]
       set JH [format "%4.2f" [expr $J-$H]]
       set JHerr [format "%4.2f" [expr sqrt($Jerr*$Jerr+$Herr*$Herr)]]
       set HK [format "%4.2f" [expr $H-$K]]
       set HKerr [format "%4.2f" [expr sqrt($Kerr*$Kerr+$Herr*$Herr)]]
       set JK [format "%4.2f" [expr $J-$K]]
       set JKerr [format "%4.2f" [expr sqrt($Kerr*$Kerr+$Jerr*$Jerr)]]
       set rd_flg [lindex $data 71]
       set bl_flg [lindex $data 72]
       set cc_flg [lindex $data 73]
       set extd_flg [lindex $data 74]
       set mp_flg [lindex $data 75]

       echo ""; echo "  2MASS: J = $J+-$Jerr  H = $H+-$Herr  K = $K+-$Kerr"
       echo "         J-H = $JH+-$JHerr H-K = $HK+-$HKerr J-K = $JK+-$JKerr" 
       echo "         rd_flg = $rd_flg  bl_flg = $bl_flg  cc_flg = $cc_flg"
       echo "         extd_flg = $extd_flg  mp_flg = $mp_flg"
       echo "         z-J = [format "%4.2f" [expr $mp(z)-$J]]"
    
}   


# given an input file with either PHOTOMETRY or PHOTOMATCH 
# structure, select all sources that do NOT have any
# of the flags in flagsOFF1 set on in epoch1, AND do NOT have 
# any of the flags in flagsOFF2 set on in epoch3, AND that 
# have ALL of the flags from flagsON1 set in epoch1, AND that 
# have ALL of the flags from flagsON2 set in epoch2. Also, 
# select only sources of type obj_type in BOTH epochs, unless
# obj_type = all
# type_bands is a list of a number and a list of bands in which 
# to enforce the type requirement (when obj_type != all). 
# the number shows the minimum number of bands which must have
# correct type, e.g. set type_bands {2 {g r i}} specifies that
# a source must have the proper type in at least 2 of the 3 
# listed bands.
# N.B: type_bands works only with PHOTOMETRY and SDSS2MASS
# If type_bands is an empty list use objc_type.
## EXCEPTION: when BLENDED is part of flagsOFF it is honored
## ONLY IF flag NODEBLEND is false
proc flag_search {infile outfile {flagsOFF1 {}} {flagsOFF2 {}} \
                  {flagsON1 {}}  {flagsON2 {}} {obj_type all} \
          {type_bands ""} {Nlinemax 100000000} } {

global OBJ_TYPE
global OBJECT1 OBJECT2

global verbose 
     set verbose 2

       set filterlist {u g r i z}

       # open files
       set inf [open $infile r]
       set outf [open $outfile w]         
       puts $outf "### ECHOed FROM $infile:"
       puts $outf "# search with flagsOFF1 = $flagsOFF1, flagsOFF2 = $flagsOFF2"
       puts $outf "# search with flagsON1 = $flagsON1, flagsON2 = $flagsON2"
       # to avoid using param2Chain, copy PHOTOMATCH structure (and any
       # other comments) from input file to the output file 
       set structOK 0
       set headerOK 0
       set Nheader 0
       while {$headerOK != 1} {
          set line [gets $inf]
          if {[eof $inf]} {  
              error "Reached the end of file without finding PHOTOMATCH data"
          }
          puts $outf $line
          set aux [lindex $line 1]
      if {$aux == "PHOTOMATCH;" || $aux == "PHOTOMETRY;" || $aux == "DOUBLEFIRST;" || $aux == "SDSSFIRST;" || $aux == "SDSSDOUBLEFIRST;" \
                   || $aux == "SDSS2MASSXSC;" || $aux == "SDSS2MASS;" || $aux == "SDSS2MASSFIRST;" || $aux == "SDSSVAR;"} {
             set structOK 1
      }
          set aux [lindex $line 0]
      if {$aux == "PHOTOMATCH" || $aux == "PHOTOMETRY" || $aux == "DOUBLEFIRST"  || $aux == "SDSSFIRST" || $aux == "SDSSDOUBLEFIRST" \
                   || $aux == "SDSS2MASSXSC" || $aux == "SDSS2MASS" || $aux == "SDSS2MASSFIRST" || $aux == "SDSSVAR"} {
             set headerOK $structOK
             set struct $aux
      }
          incr Nheader
       }
       echo " read $Nheader header lines"


       ### search input file
       set Nline 0
       set NflagOFF1 0
       set NflagON1 0
       set NflagOFF2 0
       set NflagON2 0
       set Nobj 0
       set Ngood 0

       # translate flags and obj_type
       set flagsOFF1 [translate_flags $flagsOFF1]
       set flagsOFF2 [translate_flags $flagsOFF2]
       set flagsON1 [translate_flags $flagsON1]
       set flagsON2 [translate_flags $flagsON2]
       set NbandsMin 0
       if {$obj_type == "STAR" || $obj_type == "GALAXY"} {
          set obj_type [expr $OBJ_TYPE($obj_type)]
       }
       if {$obj_type != "all"} { 
          if {$type_bands != ""} {
              set NbandsMin [lindex $type_bands 0]
              set TypeBands [lindex $type_bands 1]
          } 
       }

    
       set Necho 50000
       # loop over input file
       while {![eof $inf] && $Nline < $Nlinemax} {
          set line [gets $inf]
          # process line
          if {![eof $inf]} {  
         if {$line == ""} {continue}
             if {[string range $line 0 0] == "#"} {  
                 puts $outf $line
                 continue
             }
             incr Nline   
             if {[expr $Nline/$Necho*$Necho] == $Nline} {echo $Nline}
             # extract values
             if [catch { 
             if {$struct == "PHOTOMATCH"} {
                   set otype1 [lindex $line 10] 
                   set oflag_1 [lindex $line 11]   
                   set otype2 [lindex $line 26] 
                   set oflag_2 [lindex $line 27]  
                } elseif {$struct == "PHOTOMETRY"} { 
                   set otype1 [lindex $line 10] 
                   set oflag_1 [lindex $line 11]   
                   set otype2 [lindex $line 10] 
                   set oflag_2 [lindex $line 12] 
                   if {$NbandsMin > 0} { 
                       set Nbands 0
                       set o_types [lindex $line 13] 
                       foreach band $TypeBands {
                           set iband [lsearch $filterlist $band]
                           set o_type [lindex $o_types $iband]
                           if {$o_type == $obj_type} {
                               incr Nbands
                           } 
                       }
                   }
                } elseif {$struct == "SDSS2MASSXSC" || $struct == "SDSS2MASS" || $struct == "SDSSFIRST" || $struct == "SDSSDOUBLEFIRST"} { 
                   set otype1 [lindex $line 12] 
                   set oflag_1 [lindex $line 10]   
                   set otype2 [lindex $line 12] 
                   set oflag_2 [lindex $line 10]       
                   if {$NbandsMin > 0} { 
                       set Nbands 0
                       foreach band $TypeBands {
                           set iband [lsearch $filterlist $band]
                           set o_type [lindex $line [expr 13+$iband]] 
                           if {$o_type == $obj_type} {
                               incr Nbands
                           } 
                       }
                   }
		 } elseif {$struct == "SDSSVAR"} {
                   set otype1 [lindex $line 13] 
                   set oflag_1 [lindex $line 11]   
                   set otype2 [lindex $line 13] 
                   set oflag_2 [lindex $line 11]  
		 } else {
                   set otype1 [lindex $line 10] 
                   set oflag_1 [lindex $line 11]   
                   set otype2 [lindex $line 35] 
                   set oflag_2 [lindex $line 33]  
                }
         }] {
                echo "bad line for Nline = $Nline:"
                echo "$line"
                continue
             }

         if [catch {  
             set sourceOK 1
             # given a source and its types and flags (oflag_1 and oflag_2)
             # if any of these is ON this source is bad
             if {![are_flagsOK $oflag_1 {} $flagsOFF1]} {
                 set sourceOK 0
                 incr NflagOFF1 1
             }
             if {![are_flagsOK $oflag_2 {} $flagsOFF2]} {
                 set sourceOK 0
                 incr NflagOFF2 1
             }
             # all of these must be ON for this source to be good, i.e.
             # if any of them is OFF the source is bad
             if {![are_flagsOK $oflag_1 $flagsON1 {}]} {
                 set sourceOK 0
                 incr NflagON1 1
             }
             if {![are_flagsOK $oflag_2 $flagsON2 {}]} {
                 set sourceOK 0
                 incr NflagON2 1
             }
             # object type     
             if {$obj_type != "all"} {
                if {$NbandsMin == 0} {
                   if {$otype1 != $obj_type || $otype2 != $obj_type} {
                      set sourceOK 0
                      incr Nobj 1
                   }
                } else {
                   if {$Nbands < $NbandsMin} {
                      set sourceOK 0
                      incr Nobj 1
                   }
                }
             }
             # if good dump it to output file
             if {$sourceOK} {
               incr Ngood
               puts $outf $line              
             }
           } msg] {
              echo "bad analysis for Nline = $Nline: $msg"
              #echo "$line"
              #echo "oflag_1 = $oflag_1, oflag_2 = $oflag_2"
              continue
           }
          }
      }

      close $inf
      close $outf

      verb_echo 1 " Searched $Nline sources and found:"
      verb_echo 1 " BAD:  $NflagOFF1 have required OFF1 flags on" 
      verb_echo 1 "       $NflagOFF2 have required OFF2 flags on" 
      verb_echo 1 "       $NflagON1 have required ON1 flags off"
      verb_echo 1 "       $NflagON2 have required ON2 flags off"
      verb_echo 1 "       $Nobj are not proper type ($obj_type)"
      verb_echo 1 " GOOD: selected $Ngood sources with good flags"


}



# given an input file print flags for all sources
proc flags_from_file {infile {Nlinemax 100000000}} {

       # open file
       set inf [open $infile r]
       set structOK 0
       set headerOK 0
       while {$headerOK != 1} {
          set line [gets $inf]
          if {[eof $inf]} {  
              error "Reached the end of file without finding PHOTOMATCH data"
          }
          set aux [lindex $line 1]
      if {$aux == "PHOTOMATCH;" || $aux == "PHOTOMETRY;" || $aux == "SDSS2MASS;"} {
             set structOK 1
      }
          set aux [lindex $line 0]
      if {$aux == "PHOTOMATCH" || $aux == "PHOTOMETRY" || $aux == "SDSS2MASS"} {
             set headerOK $structOK
             set StructType $aux
      }
       }


       set Nline 0  
       # loop over input file
       while {![eof $inf] && $Nline < $Nlinemax} {
          set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#"} {  
                 continue
             }
             incr Nline   
             # extract values
             if [catch { 
               set id [lindex $line 0]
               set id "$id [lindex $line 1]"              
               set otype1 [lindex $line 10] 
               set oflag_1 [lindex $line 11]   
               set otype2 [lindex $line 26] 
               set oflag_2 [lindex $line 27] 
           if {$StructType == "SDSS2MASS"} {
               set otype1 [lindex $line 12] 
               set oflag_1 [lindex $line 10]   
               set otype2 [lindex $line 12] 
               set oflag_2 [lindex $line 10] 
           } 
               echo "source $id: t(1) = $otype1, flags(1) = [flags $oflag_1]"  
               echo "                t(2) = $otype2, flags(2) = [flags $oflag_2]"
               echo ""  
         }] {
                echo "bad line for Nline = $Nline:"
                echo "$line"
                continue
             }
          }
      }

      close $inf

      echo "processed $Nline sources"
}





# this proc searches *.par files with 2-epoch photometry (PHOTOMATCH
# structure) obtained by proc match_tsObj. The search can be 
# based on variability, color boxes, magnitude range, and positional
# change. 
# Conditions for variability are specified as 
# - Nsigma: minimum required variation expressed in units of errors 
# - weights: flag specifying bands in which to enforce m2-m1 > Nsigma*err
#      bands are ordered as {u g r i z}, to enforce the above condition
#      set flag to 1, to skip it set to 0
# - minSum: minimum number of bands with the above condition satisfied
#      useful for specifying "at least x (any) bands" queries
#      n.b. when less than the sum of weights has no effect
# - mag_range: magnitude range for selected sources given as list
#      {band m_min m_max}, band is specified by its name
# - min_ampl: minimum magnitude change in a specified band, if negative
#             then interpreted as a maximum magnitude change
# - color_ranges: required color ranges, specified as list {col_min 
#      col_max ...}, where colors are ordered as u-g, g-r, r-i, i-z
# - bb: if != 0 require that a source is bluer when brighter
#       for color use color given by bb (i.e 1-4, ordered as above),
#       and for magnitude in the same band as in min_ampl
# - minDelPosition: if > 0 select only sources that moved for more than 
#                   that many arcsec between the runs, if < 0 select
#                   select only sources that moved less than 
#                   abs(minDelPosition) arcsec 
# - Nlinemax: only for testing purposes, max. no. of line to process
#
# E.g.
# RR Lyrae search:
#   selectPHOTOMATCH $infile $infile_RRLyrae \
#                    5 0 {0 1 1 0 0} {r 0.15} {r 14.0 23.0}  \
#                    {-10 10 -0.1 0.4 -10 10 -10 10} 2
# aux:
# set wRR {0 1 1 0 0}; set mag_range {r 14.0 23.0}
# set col_rangeRR  {-10 10 -0.1 0.4 -10 10 -10 10}
# selectPHOTOMATCH $infile $infile.RRLyrae 5 0 $wRR {r 0.15} $mag_range $col_rangeRR 2

# general search - 3 sigma in at least 2 bands
#   selectPHOTOMATCH $infile $infile.general \
#                    3 2 {0 0 0 0 0} {r 0.0} {r 14.0 23.0} 

# u-flare stars - 5 sigma in u, delta(u) > 0.5 and 14 < u < 23
#   selectPHOTOMATCH $infile $infile.uflare \
#                    5 0 {1 0 0 0 0} {u 0.5} {u 14.0 23.0}
#                    
# RA search:
# selectPHOTOMATCH $inf $outf 0 0 {0 0 0 0 0} {r 0.0} {r 14.0 24.0} 
#                  {-10 10 -10 10 -10 10 -10 10} 0 none $RAMin $RAmax 
proc selectPHOTOMATCH {infile outfile Nsigma minSum {weights {0 0 0 0 0}} \
                      {min_ampl {r 0.0}} {mag_range {r 0 100}}   \
                      {color_ranges {-10 10 -10 10 -10 10 -10 10}} \
                      {bb 0} {minDelPosition none} {RAmin 0.0} {RAmax 360.0} \
                      {Nlinemax 10000000} } {

  global verbose

       set verbose 1
       set Necho 10000

       ### here choose magnitude type: either "psf" or "model"
       set magType "psf"

       ### sanity check on input 
       set bands {u g r i z} 
       set colors {"u-g" "g-r" "r-i" "i-z"}
       if {$Nsigma < 0} {
           echo "Nsigma is < 0, => interpreted as delta(mag) < Nsigma*err"
       }
       # weights
       set sum_allw 0
       loop j 0 5 {
          set w($j) [lindex $weights $j]
          if {$w($j) != 0 && $w($j) != 1} {
              error "all weights should be either 0 or 1, and not $w($j)" 
          }
          if {$w($j)} {incr sum_allw}
       }
       if {$minSum > 5} {
           error "how can minSum be $minSum, there are only 5 bands!?"
       }
       # magnitude range 
       set mag_band [lindex $mag_range 0]
       set mag_index [lsearch {u g r i z} $mag_band]
       if {$mag_index < 0} {
           error "cannot recognize magnitude band $mag_band"
       }
       set mmin [lindex $mag_range 1]
       set mmax [lindex $mag_range 2]
       # minimum amplitude
       set ampl_band [lindex $min_ampl 0]
       set ampl_index [lsearch {u g r i z} $ampl_band]
       if {$ampl_index < 0} {
           error "cannot recognize amplitude band $mag_band"
       }
       set ampl_min [lindex $min_ampl 1]
       # color range
       loop i 0 4 {
          set col_min($i) [lindex $color_ranges [expr 2*$i]]
          set col_max($i) [lindex $color_ranges [expr 2*$i+1]]
       }
       # bluer-brighter
       if {$bb} {
           if {$bb < 0 || $bb > 4} {
               error "bluer-brighter condition can be only 0-4"    
           }
           set bb_index [expr $bb - 1]           
           set bb_color [lindex $colors $bb_index]   
       }
       # change in position
       if {$minDelPosition != "none"} {
           if {$minDelPosition > 0} {
               echo "selecting sources that moved for more than $minDelPosition arcsec"
           } else {
               echo "selecting sources with positions consistent to within \
                     [expr abs($minDelPosition)] arcsec"    
               if {$minDelPosition > -0.001} {
                    error "1 mas is too much to ask for (too little, actually)"
           }                
           } 
       } else {
            echo "no requirement on the positions" 
            set minDelPosition -100000
       }

       # open files

       set inf [open $infile r]
       set outf [open $outfile w] 

       # to avoid using param2Chain, copy PHOTOMATCH structure (and any
       # other comments) from input file to the output file 
       set structOK 0
       set headerOK 0
       set Nheader 0
       while {$headerOK != 1} {
          set line [gets $inf]
          if {[eof $inf]} {  
              error "Reached the end of file without finding PHOTOMATCH data"
          }
          puts $outf $line
      if {[lindex $line 1] == "PHOTOMATCH;"} {
             set structOK 1
      }
      if {[lindex $line 0] == "PHOTOMATCH"} {
             set headerOK $structOK
      }
          incr Nheader
       }
       echo " read $Nheader header lines"
 
       puts $outf "### ECHOed FROM $infile:"

       ### search input file
       set Nline 0
       set Nbadmag 0
       set Nbadcolor 0
       set Nnonvariable 0
       set Nsmallampl 0
       set NnotBB 0 
       set Ngood 0
          
       # loop over input file
       while {![eof $inf] && $Nline < $Nlinemax} {
          set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#"} {  
                 puts $outf $line
                 continue
             }
             incr Nline   
             if {[expr $Nline/$Necho*$Necho] == $Nline} {echo $Nline}
         if [catch { 
             # extract values
             set matchID [lindex $line 1]   
             set run1 [lindex $line 2]   
             set col1 [lindex $line 3]   
             set field1 [lindex $line 4] 
             set id1 [lindex $line 5]   
             set ra1 [format "%11.7f" [lindex $line 8]]   
             set dec1 [format "%11.7f" [lindex $line 9]]
             set run2 [lindex $line 18]   
             set col2 [lindex $line 19]   
             set field2 [lindex $line 20]   
             set id2 [lindex $line 21]   
             set ra2 [format "%11.7f" [lindex $line 24]]   
             set dec2 [format "%11.7f" [lindex $line 25]]
             # mean position
             set meanRA  [expr 0.5*($ra1 + $ra2)] 
             set meanDec [expr 0.5*($dec1 + $dec2)] 
             set goodRA 1
             if {$RAmin <= $RAmax} {
                 if {$meanRA < $RAmin || $meanRA > $RAmax} {
                     set goodRA 0
                     continue
                 }
             } else {
                 if {$meanRA > $RAmin || $meanRA < $RAmax} {
                     set goodRA 0
                     continue
                 }
             }           
             # for finding moving objects
             set delRA [expr 3600.0*($ra2-$ra1)] 
             set delDec [expr 3600.0*($dec2-$dec1)] 
             # change in position (arcsec) 
             set delPosition [expr sqrt($delRA*$delRA + $delDec*$delDec)] 
             # PHOTOMATCH structure carries the following 5-element lists:
             #   index     quantity
             #    13      psfCounts[5];
             #    14      psfCountsErr[5];
             #    15      counts_model[5];
             #    16      counts_modelErr[5];
             #    17      reddening[5];
             # here we choose psf mags
             if {$magType == "psf"} {
                 set mags1 [lindex $line 13]
                 set errs1 [lindex $line 14]
                 set mags2 [lindex $line 29]
                 set errs2 [lindex $line 30]
             } else {
                 set mags1 [lindex $line 15]
                 set errs1 [lindex $line 16]
                 set mags2 [lindex $line 31]
                 set errs2 [lindex $line 32]
             }
             # reddening
             set reddening1 [lindex $line 17] 
             set reddening2 [lindex $line 33] 
             loop j 0 5 {
                # reddening correction
        set rc1 [lindex $reddening1 $j]
        set rc2 [lindex $reddening2 $j]
                if {[expr abs($rc1-$rc2)] > 0.02} {
                   error "Reddening for match $matchID differs for more than 0.02!?"
                } 
                # magnitudes and errors
                set maux1 [lindex $mags1 $j]
                set eaux1 [lindex $errs1 $j]
                set maux2 [lindex $mags2 $j]
                set eaux2 [lindex $errs2 $j]
                set m1($j) [format "%5.2f" [expr $maux1 - $rc1]] 
                set e1($j) [format "%5.2f" $eaux1] 
                set m2($j) [format "%5.2f" [expr $maux2 - $rc2]] 
                set e2($j) [format "%5.2f" $eaux2] 
                # aux quantities
                set mean_mag($j) [expr 0.5*($m1($j) + $m2($j))]
                set delm($j) [expr $m2($j) - $m1($j)]
                set err($j) [expr 0.001 + sqrt($e1($j)*$e1($j) + $e2($j)*$e2($j))] 
        if {$m1($j) < $m2($j)} {
                    set min_mag($j) $m1($j)
                    set max_mag($j) $m2($j)
            } else { 
                    set min_mag($j) $m2($j)
                    set max_mag($j) $m1($j)
                }
             }  
     } msg] {
         if {$goodRA} {
                echo "bad line for Nline = $Nline:"
                #echo "$line"
                echo $msg
                #error
             }
             continue
         }
             loop j 0 4 {
                   set mean_col($j) [expr $mean_mag($j) - $mean_mag([expr $j+1])]
                   set color1($j) [expr $m1($j) - $m1([expr $j+1])]
                   set color2($j) [expr $m2($j) - $m2([expr $j+1])]
             }

            ### check required conditions
            set sumx 0
            set sumwx 0 
            set good 1
            loop j 0 5 { 
               # is this band changing for more than Nsigma?
           if {$Nsigma < 0} {
                  if {[expr abs($delm($j))] < [expr abs($Nsigma) * $err($j)]} {
                      set x($j) 1
                  } else {
                      set x($j) 0 
                  }
               } else {
                  if {[expr abs($delm($j))] >= [expr $Nsigma * $err($j)]} {
                      set x($j) 1
                  } else {
                      set x($j) 0 
                  } 
           }
               # change in any band
               incr sumx $x($j)
               # change in required bands 
               incr sumwx [expr $w($j)*$x($j)]
            }
            # check magnitude range
            if {$min_mag($mag_index) < $mmin || $max_mag($mag_index) > $mmax} {
               set good 0
               incr Nbadmag 
               verb_echo 3 "source $Nline has bad mag: \
                     $mmin - $mmax ($min_mag($mag_index)-$max_mag($mag_index))"
            }            
            # check color ranges
            set goodC 1
            loop j 0 4 { 
               if {$color1($j) < $col_min($j) || $color1($j) > $col_max($j) ||
                   $color2($j) < $col_min($j) || $color2($j) > $col_max($j)} {
          if {$goodC} {
                      incr Nbadcolor
                      set goodC 0
                  }  
                  set good 0
                  verb_echo 3 "source $Nline has bad [lindex $colors $j]: \ 
                        $color1($j)-$color2($j) ($col_min($j) -- $col_max($j))" 
               }
            }
            # variability condition  
            if {$sumx < $minSum || $sumwx < $sum_allw} { 
               set good 0
               incr Nnonvariable
               verb_echo 3 "source $Nline is not variable: \
                     sumx = $sumx ($minSum), sumwx = $sumwx ($sum_allw)" 
            }
            # amplitude condition 
            set ampl [format "%4.2f" [expr abs($delm($ampl_index))]]  
            if {$ampl_min >= 0.0} {
               if {$ampl < $ampl_min} {
                  set good 0
                  incr Nsmallampl  
                  verb_echo 3 "source $Nline doesn't vary enough: ampl = $ampl ($ampl_min)"
               }
        } else {
               if {$ampl > [expr abs($ampl_min)]} {
                  set good 0
                  incr Nsmallampl  
                  verb_echo 3 "source $Nline varies too much: ampl = $ampl ($ampl_min)"
               } 
        }   
            # amplitude condition 
            # n.b. colors are indexed 0-3 and bb is assumed to be 1-4
            if {$bb} {
               set bb_cond [expr $delm($ampl_index) * ($color2($bb_index)-$color1($bb_index))]  
               if {$bb_cond < 0} {
                  set good 0 
                  incr NnotBB
                  verb_echo 3 "source $Nline is redder when brighter"
               }
            } 
            # dump to output file
            if {$good} {
                if {$minDelPosition > 0} {
                   # selecting moving objects
                   if {$delPosition > $minDelPosition} {
                      incr Ngood
                      puts $outf $line
                      verb_echo 2 " ** source $Nline is GOOD (so far n=$Ngood) **"
                   } else {
                      verb_echo 5 " Did not move (<$minDelPosition): delPos = $delPosition" 
                   }
            } else {
                   if {$minDelPosition <= 0} {
                     # selecting objects with consistent positions
                     if {$delPosition < [expr abs($minDelPosition)]} {
                        incr Ngood
                        puts $outf $line
                        verb_echo 2 " ** source $Nline is GOOD (so far n=$Ngood) **"
                     } else {
                        verb_echo 5 " Moved too much: delPos = $delPosition" 
                     }
           }
                }
            }
         }; # end of processing this line
      }; # end of searching input file


      verb_echo 1 "Searched file $infile with:"
      verb_echo 1 "   Nsigma = $Nsigma, minSum = $minSum"
      verb_echo 1 "   weights = $w(0) $w(1) $w(2) $w(3) $w(4)"
      verb_echo 1 "   magnitude range = $mmin - $mmax in $mag_band band"
      verb_echo 1 "   minimum amplitude = $ampl_min in $ampl_band band"
      verb_echo 1 "   color range = $col_min(0) -- $col_max(0), $col_min(1) -- $col_max(1), \
               $col_min(2) -- $col_max(2), $col_min(3) -- $col_max(3), "
      if {$bb} {
         verb_echo 1 "   also require bluer-brighter condition for $ampl_band vs. $bb_color"
      }      
      if {$minDelPosition > 0} {
         verb_echo 1 \
          "   ** Selecting moving sources with delPosition > $minDelPosition arcsec **"

      }
      verb_echo 1 ""
      verb_echo 1 " Found $Nline sources:"
      verb_echo 1 "       [expr $Nline - $Nbadmag] have good magnitude range"
      verb_echo 1 "       [expr $Nline - $Nbadcolor] have good color ranges"
      verb_echo 1 "       [expr $Nline - $Nnonvariable] satisfy variability condition"
      verb_echo 1 "       [expr $Nline - $Nsmallampl] satisfy amplitude condition"
      if {$bb} {
         verb_echo 1 "       [expr $Nline - $NnotBB] satisfy bluer-brighter condition"
      }
      verb_echo 1 "   and $Ngood selected sources are written to file $outfile"
      verb_echo 1 ""

      puts $outf "# Searched file $infile with:"
      puts $outf "#   Nsigma = $Nsigma, minSum = $minSum"
      puts $outf "#   weights = $w(0) $w(1) $w(2) $w(3) $w(4)"
      puts $outf "#   magnitude range = $mmin - $mmax in $mag_band band"
      puts $outf "#   minimum amplitude = $ampl_min in $ampl_band band"
      puts $outf "#   color range = $col_min(0) -- $col_max(0), $col_min(1) -- $col_max(1), \
               $col_min(2) -- $col_max(2), $col_min(3) -- $col_max(3), "
      puts $outf "#"
      puts $outf "# Results:"
      puts $outf "# Found $Nline sources:"
      puts $outf "#       [expr $Nline - $Nbadmag] have good magnitude range"
      puts $outf "#       [expr $Nline - $Nbadcolor] have good color ranges"
      puts $outf "#       [expr $Nline - $Nnonvariable] satisfy variability condition"
      puts $outf "#       [expr $Nline - $Nsmallampl] satisfy amplitude condition"
      if {$bb} {
         puts $outf "#       [expr $Nline - $NnotBB] satisfy bluer-brighter condition"
         puts $outf "#   also require bluer-brighter condition for $ampl_band vs. $bb_color"
      }
      if {$minDelPosition > 0} {
         puts $outf \
          "#  ** Selecting moving sources with delPosition > $minDelPosition arcsec **"

      }     
      puts $outf "#   and $Ngood selected sources are written to file $outfile"
      puts $outf "#"


      close $inf
      close $outf

}





# this proc searches *.par files with PHOTOMETRY structure
# obtained by proc dump_tsObj. The search can be on RA range, magnitude 
# range, and color boxes. 
# Conditions are specified as 
# - mag_range: magnitude range for selected sources given as list
#      {band m_min m_max}, band is specified by its name
# - color_ranges: required color ranges, specified as list {col_min 
#      col_max ...}, where colors are ordered as u-g, g-r, r-i, i-z
# - RA range: RAmin and RAmax (wraps around 360)
# - Nlinemax: only for testing purposes, max. no. of line to process
#
# E.g.
# search for L dwarfs:
#   selectPHOTOMETRY $infile $infile.Ldwarfs {z 14.0 21.0} \
#                    {-10 10 -10 10 -10 10 -10 10} 
# RA search:
# selectPHOTOMETRY $inf $outf {r 14.0 24.0}  \
#                   {-10 10 -10 10 -10 10 -10 10} $RAMin $RAmax 
#
proc selectPHOTOMETRY {infile outfile {mag_range {r 0 100}}   \
                      {color_ranges {-10 10 -10 10 -10 10 -10 10}} \
                      {RAmin 0.0} {RAmax 360.0} \
                      {Nlinemax 10000000} } {

  global verbose

       set verbose 1
       set Necho 10000

       ### here chose magnitude type: either "psf" or "model"
       set magType "psf"

       set bands {u g r i z} 
       set colors {"u-g" "g-r" "r-i" "i-z"}

       ### sanity check on input 

       # magnitude range 
       set mag_band [lindex $mag_range 0]
       set mag_index [lsearch {u g r i z} $mag_band]
       if {$mag_index < 0} {
           error "cannot recognize magnitude band $mag_band"
       }
       set mmin [lindex $mag_range 1]
       set mmax [lindex $mag_range 2]
       # color range
       loop i 0 4 {
          set col_min($i) [lindex $color_ranges [expr 2*$i]]
          set col_max($i) [lindex $color_ranges [expr 2*$i+1]]
       }

       ### open files
       set inf [open $infile r]
       set outf [open $outfile w] 

       # to avoid using param2Chain, copy PHOTOMETRY structure (and any
       # other comments) from input file to the output file 
       set structOK 0
       set headerOK 0
       set Nheader 0
       while {$headerOK != 1} {
          set line [gets $inf]
          if {[eof $inf]} {  
              error "Reached the end of file without finding PHOTOMETRY data"
          }
      if {[lindex $line 1] == "PHOTOMETRY;"} {
             set structOK 1
      }
      if {[lindex $line 0] == "PHOTOMETRY"} {
             set headerOK $structOK
      }
          incr Nheader
      if {!$headerOK} {puts $outf $line}

       }
       echo " read $Nheader header lines"
 
       puts $outf "### ECHOed FROM $infile:"

       ### search input file
       set Nline 0
       set Nbadmag 0
       set Nbadcolor 0
       set Ngood 0
          
       # loop over input file
       while {![eof $inf] && $Nline < $Nlinemax} {
      if {$headerOK} {
             set headerOK 0
          } else {
             set line [gets $inf]
          }
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#"} {  
                 puts $outf $line
                 continue
             }
             incr Nline   
             if {[expr $Nline/$Necho*$Necho] == $Nline} {echo $Nline}
           if [catch { 
             # extract values
             set matchID [lindex $line 1]   
             set run1 [lindex $line 2]   
             set col1 [lindex $line 3]   
             set field1 [lindex $line 4] 
             set id1 [lindex $line 5]   
             set ra1 [format "%11.7f" [lindex $line 8]]   
             set dec1 [format "%11.7f" [lindex $line 9]]
             # mean position
             set meanRA  $ra1
             set meanDec $dec1
             set goodRA 1
             if {$RAmin <= $RAmax} {
                 if {$meanRA < $RAmin || $meanRA > $RAmax} {
                     set goodRA 0
                     continue
                 }
             } else {
                 if {$meanRA > $RAmin || $meanRA < $RAmax} {
                     set goodRA 0
                     continue
                 }
             }           
             # PHOTOMATCH structure carries the following 5-element lists:
             #   index     quantity
             #    14      psfCounts[5];
             #    15      psfCountsErr[5];
             #    16      petroCounts[5];
             #    17      petroCountsErr[5];
             #    20      counts_model[5];
             #    21      counts_modelErr[5];
             #    25      reddening[5];
             # here we choose psf mags
             if {$magType == "psf"} {
                 set mags1 [lindex $line 14]
                 set errs1 [lindex $line 15]
             } else {
                 set mags1 [lindex $line 20]
                 set errs1 [lindex $line 21]
             }
             # reddening
             set reddening1 [lindex $line 25] 
             loop j 0 5 {
                # reddening correction
        set rc1 [lindex $reddening1 $j]
                # magnitudes and errors
                set maux1 [lindex $mags1 $j]
                set eaux1 [lindex $errs1 $j]
                set m1($j) [format "%5.2f" [expr $maux1 - $rc1]] 
                set e1($j) [format "%5.2f" $eaux1] 
                # aux quantities
                set mean_mag($j) $m1($j)
                set err($j) [expr 0.001 + $e1($j)] 
             }  
     } msg] {
         if {$goodRA} {
                echo "bad line for Nline = $Nline:"
                #echo "$line"
                echo $msg
                #error
             }
             continue
         }
             loop j 0 4 {
                set mean_col($j) [expr $mean_mag($j) - $mean_mag([expr $j+1])]
             }

            ### check required conditions
            set good 1
            # check magnitude range
            if {$mean_mag($mag_index) < $mmin || $mean_mag($mag_index) > $mmax} {
               set good 0
               incr Nbadmag 
               verb_echo 3 "source $Nline has bad mag: $mean_mag($mag_index) not \
                     between $mmin and $mmax"
            }            
            # check color ranges
            set goodC 1
            loop j 0 4 { 
               if {$mean_col($j) < $col_min($j) || $mean_col($j) > $col_max($j)} {
          if {$goodC} {
                      incr Nbadcolor
                      set goodC 0
                  }  
                  set good 0
                  verb_echo 3 "source $Nline has bad [lindex $colors $j]: \ 
                        $mean_col($j) ($col_min($j) -- $col_max($j))" 
               }
            }
            # dump to output file
            if {$good} {
                incr Ngood
                puts $outf $line
                verb_echo 2 " ** source $Nline is GOOD (so far n=$Ngood) **"
            }
         }; # end of processing this line
      }; # end of searching input file


      verb_echo 1 "Searched file $infile with:"
      verb_echo 1 "   magnitude range = $mmin - $mmax in $mag_band band"
      verb_echo 1 "   color range = $col_min(0) -- $col_max(0), $col_min(1) -- $col_max(1), \
               $col_min(2) -- $col_max(2), $col_min(3) -- $col_max(3), "
      verb_echo 1 ""
      verb_echo 1 " Found $Nline sources:"
      verb_echo 1 "       [expr $Nline - $Nbadmag] have good magnitude range"
      verb_echo 1 "       [expr $Nline - $Nbadcolor] have good color ranges"
      verb_echo 1 "   and $Ngood selected sources are written to file $outfile"
      verb_echo 1 ""

      puts $outf "# Searched file $infile with:"
      puts $outf "#   magnitude range = $mmin - $mmax in $mag_band band"
      puts $outf "#   color range = $col_min(0) -- $col_max(0), $col_min(1) -- $col_max(1), \
               $col_min(2) -- $col_max(2), $col_min(3) -- $col_max(3), "
      puts $outf "#"
      puts $outf "# Results:"
      puts $outf "# Found $Nline sources:"
      puts $outf "#       [expr $Nline - $Nbadmag] have good magnitude range"
      puts $outf "#       [expr $Nline - $Nbadcolor] have good color ranges"
      puts $outf "#   and $Ngood selected sources are written to file $outfile"
      puts $outf "#"

      close $inf
      close $outf

}



## plot a chain of PHOTOMETRY structures
proc plotPHOTOMETRY {device root title chain {symbol -1}} {

     set filterlist {u g r i z}
 
     ### get data vectors
     set deredden 1
     foreach f $filterlist { 
         set band [lsearch $filterlist $f]
         if {$deredden} {
            set reddening [vFromChain $chain reddening<$band>]
            set mpsfRaw  [vFromChain $chain psfCounts<$band>]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            set mmodRaw  [vFromChain $chain counts_model<$band>]
            set mmod($f) [vectorExprEval $mmodRaw-$reddening]
            vectorExprDel $reddening
            vectorExprDel $mpsfRaw
            vectorExprDel $mmodRaw
         } else {
            set mpsf($f) [vFromChain $chain psfCounts<$band>]
            set mmodRaw  [vFromChain $chain counts_model<$band>]
         }
         set epsf($f) [vFromChain $chain psfCountsErr<$band>]
         set emod($f) [vFromChain $chain counts_modelErr<$band>]
         set petroR50($f) [vFromChain $chain petroR50<$band>]
         vNameSet $petroR50($f) "petroR50(r')";
         vNameSet $mpsf($f) "mpsf(${f}')"; vNameSet $epsf($f) ""  
         vNameSet $mmod($f) "mmod(${f}')"; vNameSet $emod($f) ""  
     }     
     set ra [vFromChain $chain ra]; vNameSet $ra "RA J2000 (deg)"   
     set dec [vFromChain $chain dec]; vNameSet $dec "Dec J2000 (deg)"   
     set objc_type [vFromChain $chain objc_type]
     set type(r) [vFromChain $chain type<2>]

     # SDSS colors (based on model magnitudes
     set ug [vectorExprEval $mmod(u)-$mmod(g)]
     set gr [vectorExprEval $mmod(g)-$mmod(r)]
     set ri [vectorExprEval $mmod(r)-$mmod(i)]
     set iz [vectorExprEval $mmod(i)-$mmod(z)]
     vNameSet $ug "u'-g'"; vNameSet $gr "g'-r'"; 
     vNameSet $ri "r'-i'"; vNameSet $iz "i'-z'"; 



     ### all read, now plot     
     # plot color-color diagrams
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # g-r vs. u-g
         vPlot $pg $ug $gr -xmin -0.5 -xmax 3.5 -ymin -0.5 -ymax 3.0 
         titlePlot $title 20      

         # r-i vs. g-r
         vPlot $pg $gr $ri -xmin -0.5 -xmax 3.0 -ymin -0.5 -ymax 3.0 

         # i-z vs. r-i
         vPlot $pg $ri $iz -xmin -0.5 -xmax 3.0 -ymin -0.5 -ymax 3.0 

         # g vs. g-r
         vPlot $pg $gr $mmod(g) -xmin -0.5 -xmax 3.0 -ymin 23.0 -ymax 14.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
 
     # plot color-magnitude diagrams
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cm_diags.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # g vs. u-g
         vPlot $pg $ug $mmod(g) -xmin -0.5 -xmax 3.5 -ymin 23.0 -ymax 14.0 
         titlePlot $title 20      

         # r vs. g-r
         vPlot $pg $gr $mmod(r) -xmin -0.5 -xmax 3.0 -ymin 23.0 -ymax 14.0 

         # i vs. r-i
         vPlot $pg $ri $mmod(i) -xmin -0.5 -xmax 3.0 -ymin 23.0 -ymax 14.0 

         # z vs. i-z
         vPlot $pg $iz $mmod(z) -xmin -0.5 -xmax 3.0 -ymin 23.0 -ymax 14.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # plot various diagrams dealing with extended sources
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-ext_diags.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         set delmag(g) [vectorExprEval $mpsf(g)-$mmod(g)]
         set delmag(r) [vectorExprEval $mpsf(r)-$mmod(r)]
         vNameSet $delmag(g) "mpsf(g')-mmod(g')"
         vNameSet $delmag(r) "mpsf(r')-mmod(r')"

         # mpsf(r)-mod(r) vs. mmod(r)
         vPlot $pg $mmod(r) $delmag(r) -xmin 14.0 -xmax 23.0 -ymin -1.4 -ymax 4.5
         titlePlot $title 20      
         # definition for classifier
         set dmC 0.14
         plot_line $pg 0.0 $dmC 30.0 $dmC 1 2

         # mpsf(r)-mod(r) vs. mpsf(r)-mod(r)
         pgstateSet $pg -isNewplot 1 -icMark 1
         vPlot $pg $delmag(g) $delmag(r) -xmin -1.5 -xmax 4.5 -ymin -1.5 -ymax 4.5
         plot_line $pg $dmC $dmC 10.0 $dmC 1 2
         plot_line $pg $dmC $dmC $dmC 10.0 1 2 

         # petro50(r) vs. mod(r)
         pgstateSet $pg -isNewplot 1 -icMark 1
         vPlot $pg $mmod(r) $petroR50(r) -xmin 14.0 -xmax 23.0 -ymin 0.0 -ymax 15.0 

         # petro50(r) vs. mpsf(r)-mod(r)
         vPlot $pg $delmag(r) $petroR50(r) -xmin -1.5 -xmax 4.5 -ymin 0.0 -ymax 15.0 

         vectorExprDel $delmag(g); vectorExprDel $delmag(r); 
         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # plot RA-Dec
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-radec.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 1 -nywindow 2
         pgstateOpen $pg

         # arbitrary limits
         vPlot $pg $ra $dec 
         titlePlot $title 20      

         endplot $pg
     } msg] {
         echo "$msg"
     }

     # clean
     vListDel [list $ug $gr $ri $iz $ra $dec $objc_type $type(r)]
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
        vListDel [list $mpsf($f) $mmod($f) $epsf($f) $emod($f) $petroR50($f)]
     }     

}    



## plot a chain of PHOTOMETRY structures
proc plotSDSSVAR {device root title chain {symbol -1}} {

     set filterlist {u g r i z}
 
     ### get data vectors
     set deredden 1
     foreach f $filterlist { 
         if {$deredden} {
            set reddening [vFromChain $chain A$f]
            set mpsfRaw  [vFromChain $chain m$f]
            set m($f) [vectorExprEval $mpsfRaw-$reddening]
            vectorExprDel $reddening
            vectorExprDel $mpsfRaw
         } else {
            set m($f) [vFromChain $chain m$f]
         }
         set dm($f) [vFromChain $chain d$f]
         set mErr($f) [vFromChain $chain ${f}Err]
         vNameSet $m($f) "mpsf(${f})"; vNameSet $mErr($f) ""  
         vNameSet $dm($f) "mpsf(${f})1-mpsf(${f})2";
     }
     set ra [vFromChain $chain ra]; vNameSet $ra "RA J2000 (deg)"   
     set dec [vFromChain $chain dec]; vNameSet $dec "Dec J2000 (deg)"   
     set objc_type [vFromChain $chain objc_type]

     # colors
     set ug [vectorExprEval $m(u)-$m(g)]
     set gr [vectorExprEval $m(g)-$m(r)]
     set ri [vectorExprEval $m(r)-$m(i)]
     set iz [vectorExprEval $m(i)-$m(z)]
     vNameSet $ug "u-g"; vNameSet $gr "g-r"; 
     vNameSet $ri "r-i"; vNameSet $iz "i-z"; 


     ### all read, now plot     
     # plot color-color diagrams
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # g-r vs. u-g
         vPlot $pg $ug $gr -xmin -0.5 -xmax 3.5 -ymin -0.5 -ymax 2.0 
         titlePlot $title 20      

         # r-i vs. g-r
         vPlot $pg $gr $ri -xmin -0.5 -xmax 2.0 -ymin -0.5 -ymax 2.0 

         # i-z vs. r-i
         vPlot $pg $ri $iz -xmin -0.5 -xmax 2.0 -ymin -0.5 -ymax 2.5 

         # r vs. g-r
         vPlot $pg $gr $m(g) -xmin -0.5 -xmax 2.0 -ymin 23.0 -ymax 14.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
 
     # plot variability diagrams #1
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-var_diags1.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # r2-r2 vs. r
         vPlot $pg $m(r) $dm(r) -xmin 15 -xmax 22 -ymin -2 -ymax 2 
         titlePlot $title 20      

         # r2-r1 vs. u-g
         vPlot $pg $ug $dm(r) -xmin -0.5 -xmax 3.5  -ymin -2 -ymax 2 

         # u2-u1 vs. r2-r1 
         vPlot $pg $dm(r) $dm(u) -xmin -2 -xmax 2 -ymin -2 -ymax 2

         # r vs. u-g
         vPlot $pg $ug $m(r) -xmin -0.5 -xmax 3.5 -ymin 22.0 -ymax 14.0 



         endplot $pg
     } msg] {
         echo "$msg"
     }
 

     # plot variability diagrams #2
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-var_diags2.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 1 -nywindow 2
         pgstateOpen $pg

         if {![info exist rMax]} {set rMax 20.0}
         set rMask [vectorExprEval "$m(r) <= $rMax ? 1 : 0"]        
         
         # g vs. u-g
         vPlot $pg $ug $m(g) -xmin -0.5 -xmax 3.0 -ymin 21.0 -ymax 14.0 


         endplot $pg
     } msg] {
         echo "$msg"
     }

 
     # plot color-magnitude diagrams
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cm_diags.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # u vs. u-g
         vPlot $pg $ug $m(u) -xmin -0.5 -xmax 3.5 -ymin 22.0 -ymax 14.0 
         titlePlot $title 20      

         # g vs. g-r
         vPlot $pg $gr $m(g) -xmin -0.5 -xmax 2.0 -ymin 22.0 -ymax 14.0 

         # r vs. r-i
         vPlot $pg $ri $m(r) -xmin -0.5 -xmax 2.0 -ymin 22.0 -ymax 14.0 

         # z vs. i-z
         vPlot $pg $iz $m(z) -xmin -0.5 -xmax 2.5 -ymin 21.0 -ymax 13.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
    
 
     
 
     # plot RA-Dec
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-radec.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 1 -nywindow 2
         pgstateOpen $pg

         # arbitrary limits
         vPlot $pg $ra $dec -xmin 340 -xmax 20 
         titlePlot $title 20      

         endplot $pg
     } msg] {
         echo "$msg"
     }

     # clean
     vListDel [list $ug $gr $ri $iz $ra $dec $objc_type]
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
        vListDel [list $m($f) $dm($f) $mErr($f)]
     }     

}    


## plot a chain of PHOTOMETRY structures
proc plotSDSSVAR2 {device root title chain {symbol -1}} {

global rMax

     set filterlist {u g r i z}
 
     ### get data vectors
     set deredden 1
     foreach f $filterlist { 
         if {$deredden} {
            set reddening [vFromChain $chain A$f]
            set mpsfRaw  [vFromChain $chain m$f]
            set m($f) [vectorExprEval $mpsfRaw-$reddening]
            vectorExprDel $reddening
            vectorExprDel $mpsfRaw
         } else {
            set m($f) [vFromChain $chain m$f]
         }
         set dm($f) [vFromChain $chain d$f]
         set mErr($f) [vFromChain $chain ${f}Err]
         vNameSet $m($f) "mpsf(${f})"; vNameSet $mErr($f) ""  
         vNameSet $dm($f) "mpsf(${f})1-mpsf(${f})2";
     }
     set ra [vFromChain $chain ra]; vNameSet $ra "RA J2000 (deg)"   
     set dec [vFromChain $chain dec]; vNameSet $dec "Dec J2000 (deg)"   
     set objc_type [vFromChain $chain objc_type]

     # colors
     set ug [vectorExprEval $m(u)-$m(g)]
     set gr [vectorExprEval $m(g)-$m(r)]
     set ri [vectorExprEval $m(r)-$m(i)]
     set iz [vectorExprEval $m(i)-$m(z)]
     vNameSet $ug "u-g"; vNameSet $gr "g-r"; 
     vNameSet $ri "r-i"; vNameSet $iz "i-z"; 


     ### all read, now plot     
     # plot color-color diagrams

     # plot variability diagrams #2
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-var_diags2.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icMark 2 \
          -xfract 0.3 -yfract 0.3 -nxwindow 1 -nywindow 3
         pgstateOpen $pg

         if {![info exist rMax]} {set rMax 20.0}
         set rMask [vectorExprEval "$m(r) <= $rMax ? 1 : 0"]
         set mags "$m(u) $m(g) $m(r) $m(i) $m(z)" 
         set QSOmask [get_astro_mask $mags QSO $rMask]
         set RRmask [get_astro_mask $mags RRLyrae $rMask]
         set UVxMask [vectorExprEval "$ug <= 0.8 ? $rMask : 0"]        

         echo N(r<$rMax) = [vNnonzero $rMask]
         echo N(r<$rMax & QSO) = [vNnonzero $QSOmask]
         echo N(r<$rMax & UVx) = [vNnonzero $UVxMask]
         echo N(r<$rMax & RRLyrae) = [vNnonzero $RRmask]



         # g vs. u-g
         pgstateSet $pg -isNewplot 1 -symb $symbol -icMark 2
         vPlot $pg $ug $m(g) -xmin -0.5 -xmax 3.0 -ymin 21.0 -ymax 14.0 -vectorMask $rMask 
         pgstateSet $pg -isNewplot 0 -symb 13 -icMark 4
         vPlot $pg $ug $m(g) -xmin -0.5 -xmax 3.0 -ymin 21.0 -ymax 14.0 -vectorMask $QSOmask
         pgstateSet $pg -isNewplot 0 -symb 13 -icMark 10
         vPlot $pg $ug $m(g) -xmin -0.5 -xmax 3.0 -ymin 21.0 -ymax 14.0 -vectorMask $RRmask
         titlePlot $title 20      

                 
         # g-r vs. u-g
         pgstateSet $pg -isNewplot 1 -symb $symbol -icMark 2
         vPlot $pg $ug $gr -xmin -0.5 -xmax 3.0 -ymin -0.5 -ymax 2.0 -vectorMask $rMask 
         pgstateSet $pg -isNewplot 0 -symb 13 -icMark 4
         vPlot $pg $ug $gr -xmin -0.5 -xmax 3.0 -ymin -0.5 -ymax 2.0 -vectorMask $QSOmask
         pgstateSet $pg -isNewplot 0 -symb 13 -icMark 10
         vPlot $pg $ug $gr -xmin -0.5 -xmax 3.0 -ymin -0.5 -ymax 2.0 -vectorMask $RRmask
         plot_line $pg -0.5 -0.3 0.5 -0.3
         plot_line $pg  0.5 -0.3 0.5 0.5
         plot_line $pg  0.5 0.5 -0.5 0.5
         plot_line $pg  -0.5 0.5 -0.5 -0.3
         plot_line $pg  1.0 -0.1 1.5 -0.1
         plot_line $pg  1.5 -0.1 1.5 0.4
         plot_line $pg  1.5 0.4 1.0 0.4
         plot_line $pg  1.0 0.4 1.0 -0.1 


 

         endplot $pg
     } msg] {
         echo "$msg"
     }

     # clean
     vListDel [list $ug $gr $ri $iz $ra $dec $objc_type]
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
        vListDel [list $m($f) $dm($f) $mErr($f)]
     }     

}    



### this is where QSOs, blue and red disk stars, and halo stars are defined ###
# set masks for QSO, Bdisk, Rdisk, Halo
# given a list of mags {u g r i z}, above type, and an optional mask
proc get_astro_mask {mags type {inmask ""}} {

     set filterlist {u g r i z}
     foreach f $filterlist {
         set ifilter [lsearch $filterlist $f]
         set $f [lindex $mags $ifilter]
     }
     if {$inmask == ""} {
         set inmask [vectorExprNew [exprGet $u.dimen]]
         vectorExprSet $inmask 1
     }

     if {$type == "QSO"} {
         # low-z quasars
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
        # set cond "$ug > -0.5 && $ug < 0.5 && $gr > -0.3 && $gr < 0.5 && $u < 20"
         set cond "$ug > -0.5 && $ug < 0.5 && $gr > -0.3 && $gr < 0.5"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug $gr]
     } elseif {$type == "Bdisk"} {
         # blue disk stars
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
         set cond "$ug > 0.6 && $ug < 1.5 && $gr > 0.1 && $gr < 0.6 && $r < 18"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug $gr]
     } elseif {$type == "Rdisk"} {
         # red disk stars
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
         set ri [vectorExprEval "$r-$i"]
         set cond "$gr > 1.1 && $ug < 1.6 && $ri > 0.8 && $ri < 1.4 && $g < 21"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug $gr $ri]
     } elseif {$type == "Halo"} {
         # halo (blue) stars
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
         set cond "$ug > 0.5 && $ug < 1.4 && $gr > 0.0 && $gr < 0.5 && $r > 20 && $r < 22 && $g < 22"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug $gr]
     } elseif {$type == "CVs"} {
         # outliers in ugr diagram
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
         set cond "$ug > 0.5 && $ug < 1.4 && $gr > 0.0 && $gr < 0.5 && $r > 20 && $r < 22 && $g < 22"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug $gr]
     } elseif {$type == "RRLyrae"} {
         # RRLyrae in gr vs. ug
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
         set ri [vectorExprEval "$r-$i"]
         set iz [vectorExprEval "$i-$z"]
         set cond "$ug > 1.0 && $ug < 1.5 && $gr > -0.1 && $gr < 0.4 && $ri > -0.2 && $ri < 0.2 && $iz > -0.2 && $iz < 0.2"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug $gr $ri $iz]
     } else {
         error "proc get_astro_type doesn't recognize type $type!?"
     }
  
   return $outmask

}


#####################################################################
#####################################################################
#  These procs should go somewhere else

proc check_mem_and_time {command} {

   # for measuring execution time
   timerStart

   # for memory reporting
   set startMem [memNextSerialNumber]
 
   # do work
   eval $command
   
   echo "time elapsed: [cpuTime]"
   echo; echo "checking for memory leaks:"
   # check for memory leaks
   if [catch {
       assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}
   }] {
       echo "memory leaking" 
   } else {
       echo "OK"
   }

   catch {assert { 1 == 2 }}

   return 0
}




# read records from a $STRUCT structure
# for type=1 or 2 assume that the element is a result of chainMatch,
# otherwise treat it as a single $STRUCT
proc readSTRUCT {STRUCT el type record} {

    if {$type == 1} {
        return [exprGet (($STRUCT*)$el.object1)->$record]
    } elseif {$type == 2} {
        return [exprGet (($STRUCT*)$el.object2)->$record]
    } else {
        return [exprGet $el.$record]
    }
}




# given a tsObj file, return a chain of objcIo from id=id1 to
# id=id2 (or Nobj, whichever is smaller). For id1 > id2, return
# all objects
proc tsObj2chain {tsObjfile {id1 1} {id2 -1} } {

      set table [objfileOpen $tsObjfile ""]

      if {$id1 > $id2} {
         set id1 1
         set id2 [keylget table OBJnrow]
      } else {
         if {$id2 > [keylget table OBJnrow]} {
             set id2 [keylget table OBJnrow]
         }
      }

      set fileSchema [keylget table fileSchema]
      set chain [chainNew $fileSchema]
      # read and add each element to the chain
      loop i $id1 [expr $id2 + 1] {
          set el [objcIoRead_stripped table $i $fileSchema]
          chainElementAddByPos $chain $el
          handleDel $el
      }

      objfileClose table 
      return $chain

} 


proc tsObjList2chain {list {stripped 0} {objc_type "all"} {rmax 22} {verbose 0}} {

    if {![file exist [lindex $list 0]]} {error "Can't find file [lindex $list 0]"}

    set chain [tsObj2chain [lindex $list 0]]
    if {$verbose>0} {
       echo "read [chainSize $chain] from [lindex $list 0]"
    }
    if {$stripped} {
       set chain2 [tsObjChain2strippedChain $chain $objc_type $rmax]
       chainDestroy $chain genericDel    
       set chain $chain2          
    }

    if {[llength $list] > 1} {
       loop i 1 [llength $list] {
          if {![file exist [lindex $list $i]]} {
              echo "SKIPPING [lindex $list $i]"
              continue 
          }
          set aux [tsObj2chain [lindex $list $i]]
          if {$verbose>0} {
             echo "read [chainSize $aux] from [lindex $list $i]"
          }
          if {$stripped} {
              set aux2 [tsObjChain2strippedChain $aux $objc_type $rmax]
              chainJoin $chain $aux2
              chainDestroy $aux genericDel              
          } else {
              chainJoin $chain $aux
          }
       }
    }

  return $chain

}

# given a tsObj chain, return a chain of stripped elements 
# to be used in flatfield analysis
proc tsObjChain2strippedChain {chain {objc_type "all"} {rmax 30} \
	             {fON1 ""} {fOFF1 ""} {fON2 ""} {fOFF2 ""}} {

       # flag selection
       if {$fON1 == ""}  {set fON1 "BINNED1"}
       if {$fOFF1 == ""} {set fOFF1 "BRIGHT SATUR BLENDED"}
       if {$fOFF2 == ""} {set fOFF2 "DEBLENDED_AS_MOVING"}

       # translate flags
       set fON1 [translate_flags $fON1]   
       set fOFF1 [translate_flags $fOFF1]   
       set fON2 [translate_flags $fON2]   
       set fOFF2 [translate_flags $fOFF2]   
       set no_doubles 1


       ## select a subset of requested objects 
       if {$objc_type == "all"} {
           set s [chainSearch $chain "{psfCounts<2> < $rmax}"]          
       } else {
           set s [chainSearch $chain "{objc_type == $objc_type} {psfCounts<2> < $rmax}"]
       }

       # given chain $s, make a stripped chain $good and copy needed data
       set good [chainNew FFPHOTOMETRY]

       # which fields do we want copied over? 
       # as defined in FFPHOTOMETRY
       set Pfieldsaux [schemaGetFromType FFPHOTOMETRY]
       set Pfields {}
       foreach el $Pfieldsaux {   
          set el [lindex $el 0]
          lappend Pfields $el
       }
       # from TSOBJ
       set Tfieldsaux [schemaGetFromType TSOBJ]
       set Tfields {}
       foreach el $Tfieldsaux {   
          set el [lindex $el 0]
          lappend Tfields $el
       }
       # this could be done in a more general way...
       set chromatic [list type psfCounts psfCountsErr petroCounts petroCountsErr \
          petroR50 petroR50Err counts_model counts_modelErr star_L exp_L deV_L reddening]


       ### loop over all elements in the input chain
       set cursor [chainCursorNew $s]
       while {[set source [chainWalk $s $cursor]] != ""} { 
         if [good_source $source $objc_type $fON1 $fOFF1 $fON2 $fOFF2 $no_doubles] { 
            # this is a good match
            set good_source [genericNew FFPHOTOMETRY]
            # copy over all the fields that exist
            foreach el $Pfields {
                set record $el
                if {[lsearch $Tfields $record] >= 0} {
                    # this field exists, treat specially chromatic fields
                    if {[lsearch $chromatic $record] >= 0} {
                       loop j 0 5 {
                          set rec "$record<$j>"
                          set aux [readSTRUCT TSOBJ $source 0 $rec]
                          handleSet $good_source.$el<$j> [format "%6.3f" $aux]
                       }
                    } else {
                       set aux [readSTRUCT TSOBJ $source 0 $record]
                       handleSet $good_source.$el $aux
                    }
                    # special case for a single band search
                    if {[info exist fband]} {
                        set iband [lsearch $filters $fband]
                        if {$iband > -1} {
                           echo dump_good_sources: band $fband, record = $record
                        }
                        if {$record == "objc_type" && $iband > -1} {
                           set aux [readSTRUCT TSOBJ $source 0 type<$iband>]
                           handleSet $orphan.$el $aux
                        }
                        if {$record == "objc_flags" && $iband > -1} {
                           set aux [readSTRUCT TSOBJ $source 0 flags<$iband>]
                           handleSet $orphan.$el $aux
                        }
                    }
                }
            } 
            # all set, add it to the chain
            chainElementAddByPos $good $good_source
         }
     }
  
     # clean up   
     chainCursorDel $s $cursor
     chainDel $s

   return $good
}



# structure which holds photometry for a selected source
typedef struct {
    int field;
    float objc_colc;
    int objc_flags;
    int objc_flags2;
    float psfCounts[5];
    float psfCountsErr[5];
    float counts_model[5];
    float counts_modelErr[5];
    float reddening[5];
} FFPHOTOMETRY;



# read tsObj files in chunks of no more than 100 fields, 
# select given objc_type and limit mod(r) to < $rMax
# objc_type = 3 for galaxies, 6 for stars, or anything else for all objects
proc tsObjLongList2chain {list {verbose 0} {objcType all} {rMax 30}} {

       loop i 0 [llength $list] {
          set aux [tsObj2chain [lindex $list $i]]
          if {$verbose>2} {
             echo "read [chainSize $aux] from [lindex $list $i]"
          }
          ## select stars brighter than r = 21 
          if {$objcType == 3} {
             set s [chainSearch $aux "{objc_type == 3} {counts_model<2> < $rMax}"]
          } elseif {$objcType == 6} {
             set s [chainSearch $aux "{objc_type == 6} {counts_model<2> < $rMax}"]
          } else {
             set s [chainSearch $aux "{counts_model<2> < $rMax}"]
          }
          set c [chainCopy $s]
          chainDel $s
          chainDestroy $aux genericDel
          if {$i > 0} {
             chainJoin $chain $c
          } else {
             set chain $c
          } 
          if {$verbose>1} {
             echo "after $i files: Ngood = [chainSize $chain]"
          }          
       }

    return $chain

}


# given a chain read from a list of either tsObj or fpObj files,
# overwrite rowc<$ifilter> by master raw defined as
# ifield*1361 + $rowc<$ifilter>
# the new field is found by testing for id=1, i.e. it is assumed
# that the portion of the chain coming from a particular field
# is properly ordered, as well as the fields
proc make_master_row {chain {rowsperframe 1361}} {

     set ifield -1
     loop i 0 [chainSize $chain] {
	 set el [chainElementGetByPos $chain $i]
         if {[exprGet $el.id] == 1} {
             incr ifield
             echo "working on field $ifield"
             set offset [expr $rowsperframe*$ifield]
         }
         set ncolor [exprGet $el.ncolor]
         loop ic 0 $ncolor {
             set new [expr $offset+[exprGet $el.rowc<$ic>]]
             handleSet $el.rowc<$ic> $new
         }  
     }

}


# given a chain read from a list of either tsObj or fpObj files,
# overwrite objc_rowc by float field defined as
# ifield + $objc_rowc/1361 
# the new field is found by testing for id=1, i.e. it is assumed
# that the portion of the chain coming from a particular field
# is properly ordered, as well as the fields
proc make_float_field {chain {fieldOffset 0} {rowsperframe 1361}} {

     set ifield -1
     set fac [expr 1.0/1361]
     loop i 0 [chainSize $chain] {
	 set el [chainElementGetByPos $chain $i]
         if {[exprGet $el.id] == 1} {
             incr ifield
             echo "working on field $ifield"
             set offset [expr $rowsperframe*$ifield]
         }
         set ncolor [exprGet $el.ncolor]
         set new [expr $ifield+$fac*[exprGet $el.objc_rowc]]
         handleSet $el.objc_rowc [expr $fieldOffset+$new]
     }

}



proc paramList2Chain {list {verbose 0}} {

    set chain [param2Chain [lindex $list 0] ""]
    if {$verbose>0} {
       echo "read [chainSize $chain] from [lindex $list 0]"
    }
    if {[llength $list] > 1} {
       loop i 1 [llength $list] {
          set aux [param2Chain [lindex $list $i] ""]
          if {$verbose>0} {
             echo "read [chainSize $aux] from [lindex $list $i]"
          }
          chainJoin $chain $aux
       }
    }

    return $chain

}


# only the guts of objcIoRead
proc objcIoRead_stripped {_table id {fileSchema TSOBJ}} {

   upvar $_table table

   # get the needed table info
   set OBJfd [keylget table OBJfd]
   set ncolor [keylget table ncolor]
   set nrow [keylget table OBJnrow]

   # we want this source
   set n [expr $id-1];  # IDs are 1-indexed   
   if {$n >= $nrow} {
      error "There are only $nrow objects in the table"
   }          
   if {$n < 0 } {
      error "Table row must be >= 0 (and object ID >=1)"
   } 

   
   # read data
   set objcIo [genericNew $fileSchema]
   fitsBinTblRowSeek $OBJfd $n 0
   fitsBinTblRowRead $OBJfd $objcIo

   if {$fileSchema == "TSOBJ"} {
      # assuming a per-field tsObj file
      regsub {^0*} [keylget table run] "" run
      handleSet $objcIo.run $run
      handleSet $objcIo.camCol [keylget table camCol]
      handleSet $objcIo.field [keylget table field]
   }
   handleSet $objcIo.ncolor $ncolor
    

   if {[exprGet $objcIo.id] != $id} {
      #echo "object ID is not the same as its row number:"
      #echo "id = $id, objcIo.id = [exprGet $objcIo.id]"
      #error 
   }

   return $objcIo
}


# given flags for an object obtained as [exprGet $objcIo.objc_flags]
# return the flag list in English
proc flag_list {flags} {
 
  return [flags [format 0x%x $flags]]
 
} 


# given a vector with flags, return a mask vector such that it is
# 1 if and only if all of the flags listed in flagsON are on, 
# and all of the flags listed in flagsOFF are off. Treat specially
# the case when BLENDED is a member of flagsOFF: honor it only if
# NODEBLEND if false.
proc flagsMask {vec {flagsON 0} {flagsOFF 0}} {

    set fON [translate_flags $flagsON]
    set fOFF [translate_flags $flagsOFF]
 
    set mask [vectorExprEval $vec]
    loop i 0 [expr [exprGet $vec.dimen]] {
	if [are_flagsOK [exprGet $vec.vec<$i>] $fON $fOFF] {
            handleSet $mask.vec<$i> 1
        } else {
            handleSet $mask.vec<$i> 0
        }
    }
   return $mask   
}



# given flags for an object obtained as [exprGet $objcIo.objc_flags]
# return 1 if and only if all of the flags listed in flagsON are on, 
# and all of the flags listed in flagsOFF are off. Treat specially
# the case when BLENDED is a member of flagsOFF: honor it only if
# NODEBLEND if false.
proc are_flagsOK {flags {flagsON 0} {flagsOFF 0}} {

global OBJECT1 OBJECT2


     # finding that any flag in flagsON is off is sufficent for 0 
     if {$flagsON != 0 && $flagsON != {}} {
        if {$flags == 0 && $flags == {}} {return 0} 
        if {[expr ($flags & $flagsON) != $flagsON]} {return 0}
     }

     # finding that any flag in flagsOFF is set is sufficent for 0
     # unless the flag is BLENDED. BLENDED can be on in case that
     # NODEBLEND is also on.
     if {$flagsOFF != 0 && $flagsOFF != {}} {
        if {$flags == 0 && $flags == {}} {return 1} 
        if {[expr ($flags & $flagsOFF) != 0]} {
          # echo "there are forbidden flags turned on"
          # there are forbidden flags turned on, before
          # declaring the source bad check whether it is BLENDED & NODEBLEND
          if {[expr ($flags & $OBJECT1(BLENDED))] && [expr ($flags & $OBJECT1(NODEBLEND))]} {
              # echo "BLENDED & NODEBLEND is set..."
              # yes, is it bad due to other flags?
              loop j 0 [llength [flags $flagsOFF]] {
                 set flag [lindex [flags $flagsOFF] $j]
                 if {$flag != "BLENDED" && $flag != "NODEBLEND"} {
                    if {[expr ($flags & [translate_flags $flag])]} {
                       # this is a bad source due to this particular flag
                       # echo "this is a bad source due to $flag "
                       return 0
                    } 
                 }
              } 
          } else {
              return 0
          }
        }
     }   

     return 1
}


# given a dir with tsObj files, get the field range
proc find_field_range {tsObjDir {filetype tsObj} {ext fit}} {
        
        # get the tsObj files list
        set fileList [doglob $tsObjDir/${filetype}*.$ext]


        set fieldstring [findFields $fileList]   
        # sanity checks on the field range
        if {$fieldstring == "MISSING!"} {
            error "There are no $filetype files in $tsObjDir"
        }
        if {[llength $fieldstring] > 1} {
            error "The $filetype file sequence is not contiguous: $fieldstring"
        }
        # unpack the values for starting and ending field 
        set iaux [string first - $fieldstring]    
        set nchar [string length $fieldstring]    
        set f1 [string range $fieldstring 0 [expr $iaux-1]]
        set f2 [string range $fieldstring [expr $iaux+1] $nchar]
 
   return [list $f1 $f2]

}


# given a dir with asTrans file, get the field range
proc find_field_range_asTrans {asTransDir} {
        
     # get the file name
     set fileList [doglob $asTransDir/asTrans-??????.fit]
     if {[llength $fileList]>1} {error "more than one asTrans file in $asTransDir"}

     set hdr [hdrReadAsFits [hdrNew] $fileList]
     set field0 [hdrGetAsInt $hdr FIELD0]
     set nfields [hdrGetAsInt $hdr NFIELDS]
     hdrDel $hdr
     set f1 $field0
     set f2 [expr $f1+$nfields-1]
 
   return [list $f1 $f2]

}

   



# taken from Gordon
proc findFields {fileList} {

    if {$fileList != ""} {
        
        set fieldList {}
        set fieldListSort {}
        set fieldstring ""

        foreach ff $fileList {
            set file [file tail $ff]

            set b [split $file -]
            set sizeb [llength $b]
            set endb [string trimright [lindex $b [expr $sizeb-1]] ".R"]
            set fieldb [string trimleft [file rootname $endb] 0]
            # we want only unique fields
            if {[lsearch $fieldList $fieldb] < 0} {
               lappend fieldList $fieldb
            }            
        }

        set fieldListSort [lsort -integer $fieldList]
        set fieldsize [llength $fieldListSort]
        set firstfield [lindex $fieldListSort 0]
        set lastfield [lindex $fieldListSort [expr $fieldsize-1]]

        # Check to see if all fields are here
        if {$fieldsize == [expr $lastfield-$firstfield+1]} {
            set fieldstring "$firstfield-$lastfield"
        } else {
            # If fields are missing need to figure out which ones
            # and set the field string accordingly            
            set j 0
            set ff 0
            set lf 0
            set ff [lindex $fieldListSort 0]
            set nfold $ff
            loop i 1 $fieldsize {
                set nf [lindex $fieldListSort $i]
                if {$nf != [expr $nfold+1]} {
                    set lf [lindex $fieldListSort [expr $i-1]]
                    if {$j == 0} {
                        append fieldstring "$ff-$lf"
                    } else {
                        append fieldstring ",$ff-$lf"
                    }
                    incr j
                    set ff $nf
                }
                if {$nf == $lastfield} {
                    if {$lastfield == [expr [lindex $fieldListSort [expr $fieldsize-1]] + 1]} {
                        set lf $lastfield
                        append fieldstring ",$ff-$lf"
                    }
                }
                set nfold $nf
            }
        }
    } else {
        set fieldstring "MISSING!"
    }
        

    

    return $fieldstring
}

proc doglob {pattern} {

   if {[catch {set a [glob $pattern]}]} {
      set reply ""
   } else {
      set reply $a
   }

   return $reply
}



#########################################################################


# given a list of fpObj files, read chain of all elements,
# select stars, and stuff fiber counts with aperture mags.
proc fpObjList2chain {list} {

    set c [tsObjList2chain $list]
    set s [chainSearch $c "{objc_type == OBJ_STAR}"] 

    make_float_field $s
    apCounts2tsObj $s 

  return $s 

}


# make images with median and sigma for psfMag-apMag
proc psfVSaperQA {device {chain ""} {tsObjList ""} {inTitle ""} \
                  {filters "u g r i z"}} {

     if {$chain == "" && $tsObjList == ""} {
         error "must give chain or tsObjList"
     }

     set facR [expr 1.0/1361]
     set facC [expr 2048.0]

     if {$chain != ""} {
         set sB $chain
     } else {
         set c [tsObjList2chain $tsObjList]  
         set s [chainSearch $c "{objc_type == 6}"]  
         set sB [chainSearch $s "{psfCounts<2> < 21} {parent < 0} {nchild == 0}"]
         loop i 0 [chainSize $sB] {
	     set el [chainElementGetByPos $sB $i]
             # float field
             set ifield [exprGet $el.field]
             set new [expr $ifield+$facR*[exprGet $el.objc_rowc]]
             handleSet $el.objc_rowc $new
             # master pixel column
             set camCol [exprGet $el.camCol]
             set colc [exprGet $el.objc_colc]
             set new [expr 1.1*($camCol-1)*$facC + $colc]
             handleSet $el.objc_colc $new
         }
         echo "computing aperture counts..."
         apCounts2tsObj $sB
     }  
     
     echo "done with aperture counts, going to plot..."
     foreach f $filters {
        if {$f == "u" || $f == "z"} {
            set psfMax 19
            set dmMax 0.05
        } else {
            set psfMax 20
            set dmMax 0.025
        }
        set dmMin [expr -1*$dmMax]
	set ifilter [lsearch "u g r i z" $f]
        set sBf [chainSearch $sB "{psfCounts<$ifilter> > 14} {psfCounts<$ifilter> < $psfMax}"]
	set fields [vFromChain $sBf field]
        set Fmin [vExtreme $fields min]
        set Fmax [vExtreme $fields max]
	set Nfields [expr $Fmax - $Fmin + 1]
        set title "$inTitle, filter $f (psf($f)<$psfMax)"
        plotap7psf $sBf $f $Fmin $Fmax $device $title $dmMin $dmMax
        chainDel $sBf
        if {$device != "/XWINDOW"} {
            catch {exec cp plotap7psf.ps plotap7psf_$f.ps}
        }
     }

     if {$chain == ""} {
         chainDel $s
         chainDel $c
         echo finished, here is your chain
         return $sB
     } else {
        echo finished, you can reuse your chain $chain
     }

}


# given a chain of elements read from fpObj files, plot 
# aperture7-mpsf in band $f vs. master field from mf1 to mf2
proc plotap7psf {chain f mf1 mf2 {device /XWINDOW} {title ""} {dmin -0.2} {dmax 0.2}} {

     set ifilter [lsearch {u g r i z} $f]

     set apCnt [vFromChain $chain fiberCounts<$ifilter>]
     set psfCnt [vFromChain $chain psfCounts<$ifilter>]
     set magDif [vectorExprEval 2.5*lg($apCnt/$psfCnt)]
     vNameSet $magDif "mpsf-maper"
     # set psfMag [vectorExprEval 27.0-2.5*lg($psfCnt)]
     set mfield [vFromChain $chain objc_rowc]
     vNameSet $mfield "field"
     set colc [vFromChain $chain objc_colc]
     vNameSet $colc "master pix column"
          
     set mask [vectorExprEval "abs($magDif) > 0 ? 1 : 0"]

     # plot variability diagrams #2
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device plotap7psf.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb 3 -icMark 2 \
          -xfract 0.3 -yfract 0.3 -nxwindow 1 -nywindow 3
         pgstateOpen $pg

         # magDif vs. field
         pgstateSet $pg -isNewplot 1 -symb 1 -icMark 2
         vPlot $pg $mfield $magDif -xmin $mf1 -xmax $mf2 -ymin $dmin -ymax $dmax -vectorMask $mask
         plot_line $pg 0 0 1000 0 1 5

         set sig [format "%5.3f" [vSigma $magDif -vMask $mask]]
         titlePlot "$title, (sig=$sig)" 20      

         # magDif vs. colc 
         pgstateSet $pg -isNewplot 1 -symb 1 -icMark 2
         vPlot $pg $colc $magDif -xmin 0 -xmax 13312 -ymin $dmin -ymax $dmax -vectorMask $mask
         plot_line $pg 0 0 13312 0 1 5
         set facC 2048.0
         loop camCol 1 7 {
	    set x1  [expr 1.1*($camCol-1)*$facC + 0]
	    set x2  [expr 1.1*($camCol-1)*$facC + 2048]
            plot_line $pg $x1 -1 $x1 1 1 4
            plot_line $pg $x2 -1 $x2 1 1 4
         }         

         endplot $pg
     } msg] {
         echo "$msg"
     }

     vListDel [list $colc $mfield $apCnt $psfCnt $magDif $mask]



}




###########################################################################################
### TOOLS FOR DETERMINING FLATFIELD VECTOR CORRECTIONS FROM STELLAR COLOR DISTRIBUTIONS ###
###########################################################################################


######################################################################
###
### the synthetic colors are defined by a linear combination Au + Bg + Cr + Di + Ez + F,
### and 14 < r < rMax, for sources !BRIGHT, !SATUR, !BLENDED
### where
### color name    A      B       C       D       E     F     rMax  err
### -------------------------------------------------------------------
###    s       -0.249  0.794  -0.555   0.0     0.0   0.234   19.0  5.0
###    w        0.0   -0.227   0.792  -0.567   0.0   0.040   20.0  2.9
###    x        0.0    0.707  -0.707   0.0     0.0  -0.983   19.0  9.6
###    y        0.0    0.0    -0.270   0.800  -0.534 0.059   19.5  3.7
### -------------------------------------------------------------------
###
### The "good" sources must also satisfy P1min < P1 < P1max
### where P1 is defined as                        P1min  P1max 
### P1(s) = 0.910*(u-g) + 0.415*(g-r) - 1.28       -0.2   0.8
### P1(w) = 0.928*(g-r) + 0.372*(r-i) - 0.425      -0.2   0.6
### P1(x) = r-i                                     0.8   1.6
### P1(y) = 0.895*(r-i) + 0.447*(i-z) - 0.600       0.1   1.2
###
### The expected error (in milimag) is err*sqrt(300/Nfield)*sqrt(Nbin/64),
### where err for each color is tabulated above 
###
### Given the values of these four synthetic colors, and a closure relation 
### of the form K1*u+K2*g+K3*r+K4*i+K5*z+K6=0 (e.g. PT white mag, t, 
### with K1=K5=0, and K2=K3=K4=1/3. i.e. Dg+Dr+Di=3*Dt, with Dt=-K6), 
### the corrections in each band are obtained by solving a matrix equation:
### 
### | -0.249  0.794  -0.555   0.0     0.0   | |Du|   | Ds | 
### |  0.0   -0.227   0.792  -0.567   0.0   | |Dg|   | Dw |  
### |  0.0    0.707  -0.707   0.0     0.0   |*|Dr| = | Dx |  
### |  0.0    0.0    -0.270   0.800  -0.534 | |Di|   | Dy |  
### |   K1     K2      K3      K4     K5    | |Dz|   | Dt |
###
### which yields (for K1=K5=0, K2=K3=K4=1/3):
###
### |u|   |-4.016  0.565  4.239  0.000  0.962|   | Ds | 
### |g|   | 0.000  0.589  1.131  0.000  1.002|   | Dw | 
### |r| = | 0.000  0.589 -0.283  0.000  1.002| * | Dx|  
### |i|   |-0.000 -1.177 -0.848  0.000  0.999|   | Dy |  
### |z|   |-0.000 -2.061 -1.128 -1.873  0.989|   | Dt | 
###
### and for e.g. K1=K2=K4=K5=0, K3=1
###
### |u|   |-4.016  0.000  4.510  0.000  0.960|   | Ds | 
### |g|   | 0.000  0.000  1.414  0.000  1.000|   | Dw |  
### |r| = | 0.000  0.000  0.000  0.000  1.000| * | Dx |  
### |i|   |-0.000 -1.764 -0.566 -0.000  0.996|   | Dy |  
### |z|   |-0.000 -2.642 -0.848 -1.873  0.987|   | Dt |   
###
######################################################################



##################################################
##### produce a next iteration of idFF files #####
##################################################


### ### MASTER PROC ###
### assuming that Yanny's PT files (m25mptcolc*par) and tsObj-based color
### histograms (PixPsf*dat files) exist in their default dir, that idFF
### files from an old iteration exist in their default dir,
### produce new iteration of flatfield files
### 
### NB PixPsf*dat files are produced on a run-by-run basis using
### proc makePrincipalColors $run $rerun to process tsObj files
### 
proc makeNewFlats {iterNo {rerun "mix2021"} {prevIterNo -1} {dataRoot ""} {idFFdir "idFFdir"} \
           {binSizePC 32} {binSizePT 64} {NtchebyPT 5} {NtchebyFF 8}} {

global data_root

         ### take care of all directories
         if {$dataRoot == ""} {
             set data_root [get_data_root ""]
         } else {
             set data_root $dataRoot
         }    
         set outputDir $data_root/$idFFdir/iteration$iterNo
         set user [exec whoami]
         if {![file exist $outputDir]} {
	     if {[catch {exec mkdir $outputDir}]} {
                echo " *** ERROR in makeNewFlats ***"
                echo " ==> directory $outputDir"
                error " ==> doesn't exist and $user can't make it!?!"
             }
         }
         set auxDir $outputDir/auxDir
         if {![file exist $auxDir]} {
	     if {[catch {exec mkdir $auxDir}]} {
                echo " *** ERROR in makeNewFlats ***"
                echo " ==> directory $outputDir"
                error " ==> doesn't exist and $user can't make it!?!"
             }
         }
         if {$prevIterNo < 0} {
             set prevIterNo [expr $iterNo-1]
         }
         assert { $prevIterNo >= 0 }

         ### this part produces binned FF corrections for each chip and season
         ### by reading data run by run according to season definitions 
         makeNewFFcorrections $rerun $auxDir $binSizePC $binSizePT $NtchebyPT

         ### and this part applies binned FF corrections to old idFF files
         ### and produces new idFF files 
         set idFFroot idFF-[format %03d $prevIterNo]
         set idFFoldRoot $data_root/$idFFdir/iteration${prevIterNo}/$idFFroot
         set idFFroot idFF-[format %03d $iterNo]
         set idFFnewRoot $outputDir/$idFFroot
         set FFcorrDir $auxDir
         set FFcorrNR BinnedFFcorr${binSizePC}_
         idFFcorrectAll $iterNo $idFFoldRoot $idFFnewRoot $FFcorrDir $FFcorrNR $NtchebyFF         

    ### be user-friendly
    echo "*** Dearest $user, your new idFF files and QA outputs are in directory ***"
    echo "$outputDir" 

  return 0

}


# e.g.
# set iterNo 1
# set idFFoldRoot idFFdir/iteration0/idFF-000
# set idFFnewRoot idFFdir/iteration1/idFF-001
# set FFcorrDir   idFFdir/iteration1/auxDir
# set FFcorrNameRoot BinnedFFcorr32_ 
proc idFFcorrectAll {iterNo idFFoldRoot idFFnewRoot FFcorrDir FFcorrNameRoot {Ntcheby 15} \
	              {seasons "all"} {camCols "all"} {filterlist "all"} {addTestTerm 0}} {

    if {$seasons == "all"} {set seasons "10 20 30 40 50 60 70 80 90"}
    if {$camCols == "all"} {
         set camColsPlot "1 4 2 5 3 6"
         set camCols "1 2 3 4 5 6"
    } else {
         set camColsPlot $camCols
    }

  set seasons 90

    if {$filterlist == "all"} {set filterlist "u g r i z"}

    # log file
    set logFile [file dirname $idFFnewRoot]/idFFcorrectAll_${iterNo}_${Ntcheby}.log
    set fd [open $logFile "w"]
    set date [exec date]
    set user [exec whoami]
    puts $fd "# *** LOG FILE from idFFcorrectAll ***"
    puts $fd "# produced by $user on $date for iterNo=$iterNo (Ntcheby=$Ntcheby)"
    puts $fd "# idFFoldRoot = $idFFoldRoot"
    puts $fd "# idFFnewRoot = $idFFnewRoot"
    puts $fd "# FFcorrDir = $FFcorrDir"
    puts $fd "# FFcorrNameRoot = $FFcorrNameRoot"
 
    foreach s $seasons {
       set sstr  [format %03d $s]
       echo "   working on season $sstr"
       set FFcorrRoot $FFcorrDir/Season${s}$FFcorrNameRoot
       puts $fd "#"
       puts $fd "# ====== Season $s ======"
       puts $fd "# filter  camCol  rms1    rmsC    maxC"
       set rmsMax 0; set rmsMaxF ""; set rmsMaxC ""
       set maxDevMax 0; set maxDevMaxF ""; set maxDevMaxC ""
       foreach f $filterlist {
          ## this is a place holder for setting a higher order fit in the u band
	  if {$f == "u"} {
             set Ntch $Ntcheby
          } else {
             set Ntch $Ntcheby
          }
          set vList ""
          foreach c $camColsPlot {
	     set idFFold ${idFFoldRoot}${sstr}-${f}${c}.fit
	     set idFFnew ${idFFnewRoot}${sstr}-${f}${c}.fit
             set FFcorrFile ${FFcorrRoot}${f}${c}.dat
	     if {[lsearch $filterlist $f] == [lsearch $camCols $c]} {
                set testTerm $addTestTerm
             } else {
                set testTerm 0
             }
             set v [idFFcorrect $idFFold $FFcorrFile $idFFnew $iterNo $Ntch $testTerm]
             set vList "$vList [list $v]"
          }   
          # produce QA plot for this season and filter 
          set psFile ${idFFnewRoot}${sstr}-${f}QA.ps
          set title "FF iteration $iterNo for season $s and filter $f (Ntcheby=$Ntch)"
          set stats($s,$f) [idFFcorrectQA $vList $camColsPlot $title $psFile] 
          foreach camCol $camCols  {
              set cc [lsearch $camCols $camCol]
              puts $fd "     $f       $camCol   [lindex $stats($s,$f) $cc]"
              if {[lindex [lindex $stats($s,$f) $cc] 1] > $rmsMax} {
                  set rmsMax [lindex [lindex $stats($s,$f) $cc] 1]
                  set rmsMaxF $f
                  set rmsMaxC $camCol
              }
              if {[lindex [lindex $stats($s,$f) $cc] 2] > $maxDevMax} {
                  set maxDevMax [lindex [lindex $stats($s,$f) $cc] 2]
                  set maxDevMaxF $f
                  set maxDevMaxC $camCol
              }
            
          }
          # clean vectors
          foreach v $vList {
             vListDel $v
          }	
          puts $fd "# -------------------------------------"
       }
       puts $fd "#  The largest rms is for chip ${rmsMaxF}$rmsMaxC: $rmsMax milimags"     
       puts $fd "#  The largest maxDev is for chip ${maxDevMaxF}$maxDevMaxC: $maxDevMax milimags"     
    } 

    ### now plot all new flatfields together (one plot per filter)
    foreach f $filterlist {
       set title "FF comparison for band $f, iteration $iterNo (Ntcheby=$Ntcheby)"
       idFFplotAll $title $f $idFFnewRoot $seasons $camCols 
    }

    close $fd
    echo "  --- your log file is $logFile ---"
}



## produce flatfield corrections in a given output dir for all runs ##
## with required files in the default dirs, using default parameters #
## the run lists were determined using proc getGoodFFrrListFromSeason
## and define the iteration 1 idFF files
proc makeNewFFcorrections {rerun {outputDir .} {binSizePC 32} {binSizePT 64} {Ntcheby 5}} {

  #### this is version 1.0
  # for season 1 excluded runs 94 and 125
  set listS(10) "{752 20} {745 20} {756 20} {1040 20} {1043 20} {1045 20} {1056 20} {1033 20} {1035 20} {1037 20} {1140 20}" 
  set listS(20) "{1331 20} {1239 20} {1241 20} {1402 20} {1412 20} {1458 20} {1453 20} {1450 20} {1478 20}  {1231 21} {1336 21} {1339 21} {1345 21} {1356 21} {1359 21} {1462 21}"
  set listS(30) "{1666 20} {1737 20} {1739 20} {1749 20} {1729 21} {1755 21} {1740 21} {1752 21}"
  set listS(40) "{1896 20} {1891 20} {1992 20} {2076 20} {2075 20} {1893 20}  {1907 21} {1895 21} {1869 21} {1894 21} {2074 21}"
  set listS(50) "{2141 20} {2126 21} {2125 21} {2137 21} {2134 21} {2131 21}"
  set listS(60) "{2248 20} {2206 20} {2207 20} {2189 20} {2259 20} {2247 20} {2188 20} {2243 20}  {2190 21}"
  set listS(70) "{2326 20} {2327 20} {2328 20} {2333 20} {2334 20} {2335 20} {2304 20} {2305 20} {2299 20} {2298 20} {2379 20} {2385 20} {2391 20} {2392 20}"
  set listS(80) "{2570 20} {2768 20} {2566 20} {2574 20} {2576 20} {2578 20} {2582 20} {3177 20} {3015 20} {3031 20} {3103 20} {3180 20} {3187 20} {3183 20} {3184 20} {3130 20} {3185 20}"
  set listS(90) "{3225 20} {3226 20} {3230 20} {3513 20} {3518 20} {3325 20} {3525 20} {3530 20} {3538 20} {3355 20} {3384 20} {3358 20} {3362 20} {3367 20} {3560 20}"
  set Ver 1.0;    # n.b. all 9 seasons: 109 runs
     
    # local dir?
    if {$outputDir == "."} {
       set outputDir [pwd]
    }

    ### and now call the work horse proc 
    set seasons {10 20 30 40 50 60 70 80 90}
 set seasons 90
    foreach s $seasons {
       echo "    ***************   WORKING ON SEASON $s *************** "
       set title "Season $s (v $Ver)"
       if {$rerun == "mix2021"} {
          # this is for mixed reruns 20/21
          set rrList $listS($s) 
       } else {
          # this is for a uniform rerun (beware of bad early runs)
          if {$s == "10"} {
              set runsToExclude "94 125"
          } else {
              set runsToExclude ""
          }
          set rrList [getGoodFFrrListForSeason $rerun $s $runsToExclude] 
       }
       getFFbinnedCorr $rrList $outputDir $s $title $binSizePC $binSizePT $Ntcheby
    }

   echo "=========== DONE WITH makeNewFFcorrections ($outputDir) ============" 
}




### wrapper for making and processing principal colors for all runs in a rerun
proc coaddPCruns {runrerunList {outputDir .} {outputRoot SeasonX} \
	          {camCols "all"} {filterlist "all"} {FFcorrRoot FFcorrPC}} {

global data_root 

     if {$filterlist == "all"} {set filterlist "u g r i z"}
     if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}
     # local dir?
     if {$outputDir == "."} {
        set outputDir [pwd]
     }
 

     ### loop over all requested camera columns
     foreach camCol $camCols {
        foreach filter $filterlist {
           echo " ===  coaddPCruns: camCol $camCol, filter $filter"
           ### read data for all runs 
           set irun 0  
           foreach rr $runrerunList {
              set run [lindex $rr 0]
              set rerun [lindex $rr 1]
 	      set fname $outputDir/${FFcorrRoot}_run${run}_${rerun}_${filter}${camCol}.dat
	      if {![file exists $fname]} {
                 error "File $fname doesn't exist!?!"
	      }
              set bin($irun) [Tfile2Vector $fname 1 8]
              if {$irun > 0} {
                 assert { [exprGet $bin($irun).dimen] == [exprGet $bin([expr $irun-1]).dimen] }
              }
              set corr($irun) [Tfile2Vector $fname 2 8]
              set corrErr($irun) [Tfile2Vector $fname 3 8]
              incr irun
           }
           set pixBins [vectorExprEval $bin(0)]

           ### and now coadd for all runs
           loop i 0 $irun {
              set w [vectorExprEval 1.0/$corrErr($i)/$corrErr($i)]
              set x $corr($i)
	      if {$i>0} {
                 set oSw $Sw
                 set oSwx $Swx
                 set oSwx2 $Swx2
                 set Sw   [vectorExprEval $Sw+$w]
                 set Swx  [vectorExprEval $Swx+$w*$x]
                 set Swx2 [vectorExprEval $Swx2+$w*$x*$x]
	         vListDel "$oSw $oSwx $oSwx2"
	      } else {
                 set Sw   [vectorExprEval $w]
                 set Swx  [vectorExprEval $w*$x]
                 set Swx2 [vectorExprEval $w*$x*$x]
              }
    	      vListDel "$w $corr($i) $corrErr($i) $bin($i)"
           } 
           ### and now get interesting quantities
           set corrTot [vectorExprEval $Swx/$Sw]
           set corrTotErr [vectorExprEval 1.0/sqrt($Sw)]
           set corrChi2 [vectorExprEval ($Swx2-$corrTot*$corrTot*$Sw)/($irun-1)]

           #### all set, now dump correction, correction error and chi2
	   set outname $outputDir/${outputRoot}_FFcorr_${filter}${camCol}.dat
           set f [open $outname w]
           set Nrows [exprGet $pixBins.dimen]
           puts $f "# binCenter (pix)  FFcorr    FFcorrErr  corr/corrErr   chi2pdf"
           puts $f "#                  -- in 1/1000 mag --"
           ## loop over all entries
           loop j 0 $Nrows {
              # bin position
              set v1 [format "%4.0f" [exprGet $pixBins.vec<$j>]]
              # correction
              set v2 [format "%7.2f" [exprGet $corrTot.vec<$j>]]
              # correction error
              set v3 [format "%7.2f" [exprGet $corrTotErr.vec<$j>]]
              # correction / correction error
              set v4 [format "%7.2f" [expr $v2/(abs($v3)+0.000001)]]
              # chi2pdf
              set v5 [format "%7.2f" [exprGet $corrChi2.vec<$j>]]
              puts $f "       $v1      $v2      $v3     $v4     $v5"
           }
           # close file
           close $f 
           ### clean
           vListDel "$Sw $Swx $Swx2 $corrTot $corrTotErr $corrChi2 $pixBins"
        }
     }


}


# a small aux proc to get a list of runs from a list of directories
proc get_runList {list Nstrip {runMin 0} {runMax 999999}} {

       set runList ""
       foreach d $list {
          loop i 0 $Nstrip {
             set d [file dirname $d]
          }
          set run [file tail $d]
          if {$run > $runMin && $run < $runMax} {
              set runList "$runList $run"
          }
       }

  return [lsort $runList]

}


# another small aux proc to get a list of runs from a list of directories
proc get_runrerunList {list Nstrip {runMin 0} {runMax 999999}} {

       set runrerunList ""
       foreach d $list {
          loop i 0 $Nstrip {
             set d [file dirname $d]
          }
          set rerun [file tail $d]
          set d [file dirname $d]
          set run [file tail $d]
          if {$run >= $runMin && $run <= $runMax} {
              set runrerunList "$runrerunList {$run $rerun}"
          }
       }

  return $runrerunList

}



### wrapper for making and processing principal colors for all runs in a rerun
proc doPCrerun {rerun {runList ""} {ignoreRunList ""} {outputDir "default"} {dataRoot ""} \
                 {NrebinColor 2} {NrebinPix 0} {OffMax 0.1} {binSize 32}} {

global data_root firstChar

         if {![info exist firstChar]} {set firstChar ""}

         if {$dataRoot == ""} {
             set data_root [get_data_root ""]
         } else {
             set data_root $dataRoot
         }

         if {![file exist $data_root]} {
             error "CANNOT FIND data_root=$data_root"
         }

         if {$outputDir == "default"} {
             set outputDir "nfcalib/ptflatmatch"
         }

         if {$runList == ""} {
             # get a list of all runs from the given rerun that have tsObj files
             set list [glob $data_root/${firstChar}*/${rerun}/calibChunks/1]
             set runList ""
             foreach d $list {
                set d [file dirname $d]
                set d [file dirname $d]
                set d [file dirname $d]
                set run [file tail $d]
                set runList "$runList $run"
             }
         }

         foreach run $runList {
	    if {[lsearch $ignoreRunList $run] < 0} {
                echo "***** processing run $run rerun $rerun *****"
                set outDir $data_root/$run/$rerun/$outputDir
                if [catch {
                    doPrincipalColors $run $rerun $outDir all 0 -1 $NrebinColor $NrebinPix $OffMax $binSize
		}] {echo "problems with doPrincipalColors for run $run rerun $rerun"}
                # and now process PT files
                if [catch {
                    processPTfiles $run $rerun $outDir
		}] {echo "problems with processPTfiles for run $run rerun $rerun"}
            } else {
                echo " ---|||| run $run ignored ||||---"
            }
         }
      
	 echo "--- done with doPCrerun for rerun $rerun ---"

}



### wrapper for making and processing principal colors
proc doPrincipalColors {run rerun {outputDir .} {camCols "all"} {fieldStart 0} {fieldEnd -1} \
			{NrebinColor 2} {NrebinPix 0} {OffMax 0.1} {binSize 32} {doModMags 0} } {
global data_root 

         # doPT=1: use t magnitude constraint from PT, doPT=0, assume delta(t)=0
         set doPT 0

         # we want all color samples
         set samples "0 1 2 3"

         # first make histograms for principal colors
         echo " *** making PCs for camCols=$camCols samples=$samples ***"
         set fields [makePrincipalColors $run $rerun $outputDir $camCols \
			 $fieldStart $fieldEnd $samples $binSize $doModMags]
         set fMin [lindex $fields 0]
         set fMax [lindex $fields 1]

         # process principal colors
         echo " *** processing PCs for camCols=$camCols samples=$samples ***"
         processPrincipalColors $run $rerun $outputDir $camCols $samples $fMin $fMax \
		     $doModMags $NrebinColor $NrebinPix $OffMax

         # make flatfield corrections, one per chip 
         echo " *** producing FF corrections ***"
         doPC2FFcorrection $run $rerun $outputDir $camCols $doPT
         if {$doModMags} {
            doPC2FFcorrection $run $rerun $outputDir $camCols $doPT ColorCorrMod FFcorrPCmod 
         }

    return " === All went well with doPrincipalColors for run $run, rerun $rerun ==="
}



# make mPT-m25 images for all runs that have BinnedPTdm* files
proc makePTm25images {rerunList {runList ""}  {ignoreRunList ""} {outputDir .} {camCols "all"} \
	              {filterlist "all"} {dataRoot ""} {binSize 32} } {

global data_root 

         set PTdir nfcalib/ptflatmatch
         set BinnedPTdmRoot BinnedPTdm$binSize

         if {$dataRoot == ""} {
             set data_root [get_data_root ""]
         } else {
             set data_root $dataRoot
         }

         if {![file exist $data_root]} {
             error "CANNOT FIND data_root=$data_root"
         }

         if {$outputDir == "."} {
             set outputDir "[pwd]"
         }

         if {$filterlist == "all"} {set filterlist "u g r i z t"}
         if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}

         # sanity check
         if {[llength $rerunList] > 1 && $runList != ""} {
            error "You can specify more than one rerun if runList is not empty."
         }

         # first make run/rerun list
         set rrList ""
         foreach rerun $rerunList {
             if {$runList == ""} {
                 set aux $rerun/${PTdir}/$BinnedPTdmRoot
                 set runs [get_runList [glob $data_root/*/${aux}*z6.dat] 4]
             } else {
                 set runs $runList
             }
             foreach run $runs {
	        if {[lsearch $ignoreRunList $run] < 0} {
                     set runstr [format %06d $run]
                     set rrList "$rrList {$runstr $run $rerun}"
                }
             }
         }
         set rrList [lsort $rrList]

         # now read all vectors
         foreach camCol $camCols {
            foreach f $filterlist {
               echo " reading data for camCol $camCol filter $f"               
               set Nrun 0
               set binList ""
               set corrList ""
               set corrErrList ""
               set chiList ""
               set goodRunList ""
               foreach rr $rrList {
                  set runstr [lindex $rr 0]
                  set run [lindex $rr 1]
                  set rerun [lindex $rr 2]
                  set dir $data_root/${run}/${rerun}/$PTdir 
		  set fileRoot $dir/${BinnedPTdmRoot}-${runstr}-
                  if {$f == "t"} {
                     set K1 0; set K2 0.3333; set K3 0.3333; set K4 0.3333; set K5 0; 
                     set PTconstraint [getPTconstraint $fileRoot $camCol $K1 $K2 $K3 $K4 $K5]
                     set bin     [lindex $PTconstraint 0]
                     set corr    [lindex $PTconstraint 1]
                     set corrErr [lindex $PTconstraint 2]
	          } else {
		     set PTfile ${fileRoot}${f}${camCol}.dat
		     if {![file exist $PTfile]} {
                         echo "CANNOT find file"
                         error $PTfile
		     } else {
                         set bin     [Tfile2Vector $PTfile 1 9]
                         set corr    [Tfile2Vector $PTfile 2 9]
                         set corrErr [Tfile2Vector $PTfile 3 9]
                     }
                  }
                  set chi [vectorExprEval $corr/(abs($corrErr)+0.001)]                  
                  # sanity check
                  if {$Nrun > 0} {
		     assert {[exprGet $bin.dimen] == [exprGet $masterBin.dimen] }
                  } else {
                     set masterBin [vectorExprEval $bin]
                  }
                  set binList "$binList $bin"
                  set corrList "$corrList $corr"
                  set corrErrList "$corrErrList $corrErr"
                  set chiList "$chiList $chi"
                  set goodRunList "$goodRunList $run"
                  incr Nrun
               }
               echo " making images for camCol $camCol filter $f"               
               ### make and store images
               set fname $outputDir/BinnedPTdm-Image-${f}${camCol}.fit
               set im1 [makeRunDMimage $fname $goodRunList $corrList -20 20] 
               regWriteAsFits $im1 $fname; regDel $im1
               set fname $outputDir/BinnedPTdmErr-Image-${f}${camCol}.fit
               set im2 [makeRunDMimage $fname $goodRunList $corrErrList -10 10] 
               regWriteAsFits $im2 $fname; regDel $im2
               set fname $outputDir/BinnedPTchi-Image-${f}${camCol}.fit
               set im3 [makeRunDMimage $fname $goodRunList $chiList -5 5]  
               regWriteAsFits $im3 $fname; regDel $im3
               # clean 
               vectorExprDel $masterBin
               vListDel $binList 
               vListDel $corrList 
               vListDel $corrErrList 
               vListDel $chiList
               echo " done with camCol $camCol filter $f"               
            }
         }

   return 
}



### a little aux proc to go with makePCimages for making mosaics
proc makePCimageMosaics {{root BinnedPC-CORRdm-Image} {inputDir .} {outputDir .} \
                        {camCols "all"} {PCcolors "all"} {magType Psf}} {


        if {$PCcolors == "all"} {set PCcolors "s w x y"}
        if {$camCols == "all"} {set camCols "3 6 2 5 1 4"}

        ### loop over all camera columns 
        set bigList ""
        foreach camCol $camCols {
           set list ""
           foreach PCcolor $PCcolors {
	      set list "$list $root-${PCcolor}${camCol}.fit"
           }
           set reg [fitlist2mosaic $list $inputDir [llength $PCcolors] 1 0 2 5] 
           set outname ${root}-mosaic-camCol$camCol.fit
           set bigList "$bigList $outname"
           regWriteAsFits $reg $outputDir/$outname
           regDel $reg       
        }

        set reg [fitlist2mosaic $bigList $outputDir 2 [expr [llength $camCols]/2] 0 2 10] 
        set outname ${root}-mosaic.fit
        regWriteAsFits $reg $outputDir/$outname
        regDel $reg
        echo "master mosaic saved as $outputDir/$outname"	
}


# make images for all runs that have ColorCorrPsf*dat (or ColorCorrMod*dat) files
proc makePCimages {rerunList {outputDir .} {camCols "all"} {PCcolors "all"} \
	           {magType Psf}} {

global data_root correctFF FFcorrDir binSizePC

        ### sanity checks
        if {![file exist $data_root]} {
            error "CANNOT FIND data_root=$data_root"
        }

        if {$outputDir == "."} {
            set outputDir "[pwd]"
        }

        if {$PCcolors == "all"} {set PCcolors "s w x y"}
        if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}

    
        ### first get all runs
        set PTdir nfcalib/ptflatmatch
        set root ColorCorr$magType
        set list ""
        foreach rerun $rerunList {
	   set list "$list [glob $data_root/*/$rerun/$PTdir/${root}_y*c6.dat]"
        } 
        set rrList [get_runrerunList $list 3]
        echo " === extracted run/rerun list, going to process all seasons ==="; echo ""
         
        if {![info exist correctFF]} {set correctFF 0} 

        if {$correctFF} {
            if {![info exist FFcorrDir]} {error "You must specify FFcorrDir since correctFF=true"} 
            if {![info exist binSizePC]} {error "You must specify binSizePC since correctFF=true"} 
            set outRoot BinnedPC-CORR
            echo "  ---- USING FF CORRECTIONS FROM $FFcorrDir"
        } else {
            set outRoot BinnedPC
        }
       
         # now make runstr/run/rerun list
         set rrrList ""
         foreach rr $rrList {
            set run [lindex $rr 0]
            set runstr [format %06d $run]
            set rrrList "$rrrList {$runstr $run [lindex $rr 1]}"
         }
         set rrList [lsort $rrrList]

         # now read all vectors
         foreach camCol $camCols {
            foreach f $PCcolors {
               echo " reading data for camCol $camCol filter $f"               
               set Nrun 0
               set binList ""
               set corrList ""
               set corrErrList ""
               set chiList ""
               set goodRunList ""
               foreach rr $rrList {
                  set runstr [lindex $rr 0]
                  set run [lindex $rr 1]
                  set rerun [lindex $rr 2]
                  set dir $data_root/${run}/${rerun}/$PTdir 
		  set Cfile $dir/${root}_${f}_run${run}_${rerun}_c${camCol}.dat
		  if {![file exist $Cfile]} {
                      echo "CANNOT find file"
                      error $Cfile
		  } else {
                      set bin     [Tfile2Vector $Cfile 1 12]
                      set corr    [Tfile2Vector $Cfile 6 12]
                      set corrErr [Tfile2Vector $Cfile 7 12]
                      if {$correctFF} {
                          ### use derived flatfield correction to correct
                          # first determine season for this run
                          set season [getFFseasonNumberFromRun $run]
                          set corrFileRoot $FFcorrDir/Season${season}BinnedFFcorr$binSizePC
                          set corr [correctByFFcorr $corr $f $camCol $corrFileRoot]
                      }
                  }
                  set chi [vectorExprEval $corr/(abs($corrErr)+0.001)]                  
                  # sanity check
                  if {$Nrun > 0} {
		     assert {[exprGet $bin.dimen] == [exprGet $masterBin.dimen] }
                  } else {
                     set masterBin [vectorExprEval $bin]
                  }
                  set binList "$binList $bin"
                  set corrList "$corrList $corr"
                  set corrErrList "$corrErrList $corrErr"
                  set chiList "$chiList $chi"
                  set goodRunList "$goodRunList $run"
                  incr Nrun
               }
               echo " making images for camCol $camCol PC $f"               
               ### make and store images
               set fname $outputDir/${outRoot}dm-Image-${f}${camCol}.fit
               set im1 [makeRunDMimage $fname $goodRunList $corrList -20 20] 
               regWriteAsFits $im1 $fname; regDel $im1
               set fname $outputDir/${outRoot}dmErr-Image-${f}${camCol}.fit
               set im2 [makeRunDMimage $fname $goodRunList $corrErrList -10 10] 
               regWriteAsFits $im2 $fname; regDel $im2
               set fname $outputDir/${outRoot}chi-${f}${camCol}.fit
               set im3 [makeRunDMimage $fname $goodRunList $chiList -5 5]  
               regWriteAsFits $im3 $fname; regDel $im3
               # clean 
               vectorExprDel $masterBin
               vListDel $binList 
               vListDel $corrList 
               vListDel $corrErrList 
               vListDel $chiList
               echo " done with camCol $camCol PC $f"               
            }
         }

   return 
}


### given a correction vector for one of the principal colors, PC, in the
### specified camCol, and assuming that binned FF correction is given on
### the same grid, and in files ${FFfile}-$filter$camCol.dat, add the FF
### correction and return the new correction vector (and delete the old one)
proc correctByFFcorr {corr PC camCol FFfile} {

     set filterlist {u g r i z}
     set PCcolors {s w x y}
     ## first read FF corrections for all five filters
     foreach f $filterlist {
	set file ${FFfile}_${f}${camCol}.dat
        if {![file exist $file]} {
           echo "correctByFFcorr: CANNOT find file"
           error $file
        } else {
           set FFcorr($f) [Tfile2Vector $file 2 8]
           # sanity check
	   assert { [exprGet $corr.dimen] == [exprGet $FFcorr($f).dimen] } 
        }
     }
     ## using FFcorrections for the 5 filters, derive corrections for the PCs
     set FFcorr(s) [vectorExprEval -0.249*$FFcorr(u)+0.794*$FFcorr(g)-0.555*$FFcorr(r)]
     set FFcorr(w) [vectorExprEval -0.227*$FFcorr(g)+0.792*$FFcorr(r)-0.567*$FFcorr(i)]
     set FFcorr(x) [vectorExprEval  0.707*$FFcorr(g)-0.707*$FFcorr(r)]
     set FFcorr(y) [vectorExprEval -0.270*$FFcorr(r)+0.800*$FFcorr(i)-0.534*$FFcorr(z)]
       
     ### and here we correct...
     set newCorr [vectorExprEval $corr-$FFcorr($PC)]

     # clean 
     foreach f $filterlist {
        vectorExprDel $FFcorr($f)
     }
     foreach c $PCcolors {
        vectorExprDel $FFcorr($c)
     }
     vectorExprDel $corr 

   return $newCorr
}


# given a list of values for the first column and a list of row vectors 
# make an image and dump it to outfile
proc makeRunDMimage {outFile firstColList rowList {pmin -20} {pmax 20} \
	             {blackVal -1000} {pixMin -1000} {pixMax 1000}} { 


     assert { [llength $firstColList] == [llength $rowList] }

     set ncol [expr [exprGet [lindex $rowList 0].dimen]+1]  
     set Nruns [llength $rowList]
     set nrow [expr 8+$Nruns]  
     set image [regNew -type FL32 $nrow $ncol]  
     regClear $image  

     # make top bar
     loop i [expr $Nruns+2] [expr $Nruns+8] {
        loop j 0 $ncol {
           set Pixel [subRegNew $image 1 1 $i $j]
           set C [expr $pmin+1.0*$j*($pmax-$pmin)/($ncol-1)]
           regAddWithDbl $Pixel $C
           regDel $Pixel
        }
     }   
     # and black boundary
     loop i [expr $Nruns+0] [expr $Nruns+5] {
        loop j 0 $ncol {
           set Pixel [subRegNew $image 1 1 $i $j]
           regAddWithDbl $Pixel $blackVal
           regDel $Pixel
        }
     }
    
     loop i 0 $Nruns {
        set firstPixel [subRegNew $image 1 1 $i 0]
        set C [lindex $firstColList $i]
        regAddWithDbl $firstPixel $C
        regDel $firstPixel
        set row [lindex $rowList $i]
        loop j 0 [expr $ncol-1] {
           set thisPix [subRegNew $image 1 1 $i [expr $j+1]]
           set C [exprGet $row.vec<$j>]
	   if {$C < $pixMin} {set C $pixMin}
	   if {$C > $pixMax} {set C $pixMax}
           regAddWithDbl $thisPix $C
           regDel $thisPix
        }
     }

   return $image    
  
}



# e.g.
# set rerun 20
# set runsAll  [get_runList [glob $data_root/*/$rerun/nfcalib/ptflatmatch/m25mptcolc*z6.par] 4]; echo
# set runsDone [get_runList [glob $data_root/*/$rerun/nfcalib/ptflatmatch/BinnedPTdm*z6.dat] 4]; echo
# doPTfiles $rerun $runsAll $runsDone
#
### wrapper for making and processing PT files for all runs in a rerun
proc doPTfiles {rerun {runList ""} {ignoreRunList ""} {camCols "all"} {filterlist "all"} \
	       {outputDir "default"} {dataRoot ""} {PTroot m25mptcolc} {binSize 32}} {

global data_root 

         if {$dataRoot == ""} {
             set data_root [get_data_root ""]
         }

         if {![file exist $data_root]} {
             error "CANNOT FIND data_root=$data_root"
         }

         if {$outputDir == "default"} {
             set outputDir "nfcalib/ptflatmatch"
         }

         if {$filterlist == "all"} {set filterlist "u g r i z"}
         if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}

         if {$runList == ""} {
             # get a list of all runs from the given rerun that have m25mptcolc files
             set list [glob $data_root/*/${rerun}/${outputDir}/m25mptcolc-*-g1.par] 
             set runList ""
             foreach d $list {
                set d [file dirname $d]
                set d [file dirname $d]
                set d [file dirname $d]
                set d [file dirname $d]
                set run [file tail $d]
                set runList "$runList $run"
             }
         }

         foreach run $runList {
	    if {[lsearch $ignoreRunList $run] < 0} {
                echo "***** processing PT residuals for run $run rerun $rerun *****"
                processPTfiles $run $rerun $outputDir $camCols $filterlist $PTroot "" $binSize 
            } else {
                echo " ---|||| run $run ignored ||||---"
            }
         }
      
	 echo "--- done with processPTfiles for rerun $rerun ---"

}


### proc to process m25mptcolc-$run-$rerun-$f$c.par files  
proc processPTfiles {run rerun {outputDir "default"} {camCols "all"} {filterlist "all"} \
	        {PTroot m25mptcolc} {dataRoot ""} {binSize 32} {binnedName BinnedPTdm} } {

      set binnedName ${binnedName}$binSize

      if {$filterlist == "all"} {set filterlist "u g r i z"}
      if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}
      if {$dataRoot == ""} {
          set data_root [get_data_root ""]
      }
      if {![file exist $data_root]} {
          error "CANNOT FIND data_root=$data_root"
      }
      if {$outputDir == "default"} {
          set outputDir "nfcalib/ptflatmatch"
      }
      set runstr [format %06d $run]
 
      ### loop over all requested chips
      set dir $data_root/${run}/${rerun}/${outputDir}
      foreach camCol $camCols {
         foreach f $filterlist {
            # echo "processPTfiles for run $run rerun $rerun: camCol $camCol, filter $f"
	    set PTfile $dir/m25mptcolc-${runstr}-${rerun}-${f}${camCol}.par
	    set outFile $dir/${binnedName}-${runstr}-${f}${camCol}.dat
            set chain [parList2Chain $PTfile]  
            processPTchain $chain $outFile $f $binSize
            chainDestroy $chain genericDel 
         }
      }
      
}


### after running processPTfiles2 for all camCols/filters produce a 
### 6-panel plot with gri bands and the t magnitudes for all 6 cols
### and overplot the Ntcheby-th order Tchebychev polynomial for
### the t magnitude (optionally plot all 5 bands
proc plotPTconstraint {Ntcheby inputDir inputNameRoot outputDir outputPSname \
	              {title ""} {dev /XWINDOW} {plotFilters "g r i"}} {

      set filterlist "u g r i z t"
      set camCols "1 4 2 5 3 6"
      set ic(u) 5 
      set ic(g) 3
      set ic(r) 13
      set ic(i) 14
      set ic(z) 12

      set dmMin -30; set dmMax 30 

      ### setup the plot
      set pg [pgstateNew]
      set device [devicename $dev "$outputDir/$outputPSname"] 
      pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
                -xfract 0.27 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
      set icLineDef [exprGet $pg.icLine]
      pgstateOpen $pg

      ### read all vectors
      foreach camCol $camCols {
         ## get data for this camCol
         foreach f $filterlist {     
            set fileRoot $inputDir/${inputNameRoot}-
            if {$f == "t"} {
               set K1 0; set K2 0.3333; set K3 0.3333; set K4 0.3333; set K5 0; 
               set PTconstraint [getPTconstraint $fileRoot $camCol $K1 $K2 $K3 $K4 $K5]
               set bin($f)     [lindex $PTconstraint 0]
               set corr($f)    [lindex $PTconstraint 1]
               set corrErr($f) [lindex $PTconstraint 2]
               ## fit the Ntcheby-th order Tchebychev polynomial 
               set xTcheby [vIndex 2048]
               set wTcheby [vectorExprEval 1.0/$corrErr($f)/$corrErr($f)]
               set yTcheby [tchebyFit $bin($f) $corr($f) $wTcheby $Ntcheby -fitx $xTcheby]
	    } else {
               set PTfile ${fileRoot}${f}${camCol}.dat
               if {![file exist $PTfile]} {
                  echo "CANNOT find file"
                  error $PTfile
	       } else {
                  set bin($f)     [Tfile2Vector $PTfile 1 9]
                  set corr($f)    [Tfile2Vector $PTfile 2 9]
                  set corrErr($f) [Tfile2Vector $PTfile 3 9]
               }
	    }
         }
         ## plot this camCol   
         # loop over desired filters 
         foreach f $plotFilters {     
            pgstateSet $pg -icLine $ic($f) -lineWidth 1
            vPlot $pg $bin($f) $corr($f) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
            pgstateSet $pg -isNewplot 0
         }
         # and now t magnitude
         set f t
         pgstateSet $pg -isNewplot 0 -isLine 0 -lineWidth 2 -icMark 4 -symb 4 
         vPlot $pg $bin($f) $corr($f)
         pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 3 -icLine 4 
         vPlot $pg $xTcheby $yTcheby 
         # to guide the eye...
         pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1 -icLine 15
         plot_line $pg -1000 0 10000 0 1 4 
         plot_line $pg -1000 -10 10000 -10 1 4 
         plot_line $pg -1000 10 10000 10 1 4 
         plot_line $pg 0 -1000 0 1000 1 2
         plot_line $pg 2048 -1000 2048 1000 1 2
         xlabel "chip column (pixel)"
         ylabel "dm (milimag)" 
         pgstateSet $pg -isNewplot 1

         # which camCol 
         pgText 100 -25 "camCol $camCol"

         ## clean
         vListDel "$xTcheby $wTcheby $yTcheby"  
         foreach f $filterlist {     
            vListDel "$bin($f) $corr($f) $corrErr($f)"
         }   
         if {$camCol == [lindex $camCols 0]} {titlePlot "$title" 20} 
         if {$camCol == [lindex $camCols 4]} {
            # info
            plot_info "The constraints from m25-mPT photometric comparison:" 0 -1.0 -0.6
            plot_info "green: g, red: r, gray: i" 0 -1.0 -0.8
            plot_info "blue circles: t, blue line: Tchebychev fit" 0 -1.0 -1.0
         }
      }

      ## end plot
      endplot $pg

}


############## a master wrapper to produce binned flatfield corrections ##################
### a wrapper to produce PT constraints and plots for all seasons, given a rerun list
### run boundaries for a given season are defined in proc getFFseasonRunRange
proc getFFseasonBinnedCorr {rerunList {seasons "all"} {outputDir "."} \
                             {binSize 32} {binSizePT 64} {Ntcheby 5}} {

global data_root

      if {$seasons == "all"} {set seasons "10 20 30 40 50 60 70 80 90"}

      ### first produce PT constraints (Season${season}BinnedPTdm$binSize*dat files)
      doPTseasons $rerunList $seasons $outputDir $binSizePT $Ntcheby

      ### now produce PC constraints (Season${season}BinnedPCdm$binSize*dat files) 
      doPCseasons $rerunList $seasons $outputDir $binSize

      ### and now combine them (Season${season}BinnedFFcorr$binSize*dat files)  
      doFFseasons $seasons $outputDir $outputDir $binSize $binSizePT $Ntcheby 
      # and make plots
      set title "Binned Flatfield Corrections" 
      plotBinnedFFCorr $title $outputDir $outputDir $binSizePC BinnedFFCorr $seasons

    echo ""
    echo "**********************************************************"
    echo "** Produced binned FF corrections for seasons $seasons"
    echo "** and stored to Season\${season}BinnedFFcorr\$binSize*dat files"
    echo "**********************************************************"

}




## aux proc to straighten out keywords for the 0th iteration idFF files
proc fixFFkeywords {{inputDir .} {outputDir .}} {

    foreach s {1 2 3 4 5 6 7 8 9} {
       set startRun [lindex [getFFseasonRunRange $s] 0]
       set endRun [lindex [getFFseasonRunRange $s] 1]
       echo "   working on season $s ($startRun-$endRun)"
       foreach c {1 2 3 4 5 6} {
	  foreach f {u g r i z} {
	     set old $inputDir/idFFx-0000${s}0-${f}${c}.fit
	     set new $outputDir/idFF-0000${s}0-${f}${c}.fit
             # read old file
             set FFreg [regNew]
             regReadAsFits $FFreg $old
             # remove all keywords that aren't needed
             set kwords "RUN FIELD FF_ID VERSION DERV_VER ASTR_VER"
             set kwords "$kwords BIAS_ID SSC_ID EXCAL_ID"
             foreach kword $kwords {
                catch {hdrDelByKeyword $FFreg.hdr $kword}
             }
             loop xx 0 99 {
                set kword KAL[format %02d $xx]_ID 
                catch {hdrDelByKeyword $FFreg.hdr $kword}
             }
             # and now add the keywords describing the 0th iteration
             hdrInsWithInt $FFreg.hdr SEASON $s "Flatfield season"
             hdrInsWithInt $FFreg.hdr FFS_SR $startRun "Flatfield season starting run"
             hdrInsWithInt $FFreg.hdr FFS_ER $endRun "Flatfield season ending run"
             hdrInsWithInt $FFreg.hdr FFITER 0 "Flatfield iteration"
             hdrInsWithInt $FFreg.hdr ITERIN -1 "Flatfield iteration used as input"
             set version "0th iteration from rerun 20/21 psFF files"
             hdrInsWithAscii $FFreg.hdr PHOT_VER $version "Photo version used"
             # write new file
             regWriteAsFits $FFreg $new
          }     
       }
    } 
}





## given a new iteration of idFF files, compare all seasons for the given filter 
proc idFFplotAll {title filter idFFnewRoot {seasons "all"} {camCols "all"} {dev /VCPS}} {

    if {$seasons == "all"} {set seasons "10 20 30 40 50 60 70 80 90"}
    if {$camCols == "all"} {set camCols "1 4 2 5 3 6"}

    ### definitions of line colors for each season
    set iSeason(10) 2 
    set iSeason(20) 3
    set iSeason(30) 4 
    set iSeason(40) 5
    set iSeason(50) 2
    set iSeason(60) 3
    set iSeason(70) 4
    set iSeason(80) 5
    set iSeason(90) 2

    # first read all data
    foreach s $seasons {
       set sstr [format %03d $s]
       foreach c $camCols {
	  set idFFfile ${idFFnewRoot}${sstr}-${filter}${c}.fit
          set idFFreg [regReadAsFits [regNew] $idFFfile]
          # get vector out of this 1xNcol region
          set Ncol [exprGet $idFFreg.ncol]
          set aux [vectorGetFromRegion $idFFreg 0 0 0 [expr $Ncol-1] 0]
          # rescale it to ~1
          set tshift [hdrGetAsInt $idFFreg.hdr TSHIFT]
          set FF($s,$c) [vectorExprEval 1.0*$aux/$tshift]
          vectorExprDel $aux
          regDel $idFFreg
       }
    }

    # and now plot
    set pix [vIndex [exprGet $FF([lindex $seasons 0],[lindex $camCols 0]).dimen]]
    ### setup this plot
    set pg [pgstateNew]
    set psFile ${idFFnewRoot}-CompareAllSeasons-${filter}.ps
    set device [devicename $dev $psFile] 
    pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
          -xfract 0.28 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
    set icLineDef [exprGet $pg.icLine]
    pgstateOpen $pg

    ### loop over camCols
     foreach c $camCols {
        # determine y axis range
        set yMin 0.95; set yMax 1.05
        foreach s $seasons {
           set min [vExtreme $FF($s,$c) min] 
           if {$min < $yMin} {set yMin $min}
           set max [vExtreme $FF($s,$c) max] 
           if {$max > $yMax} {set yMax $max}
        }

        ### and now plot
        foreach s $seasons {
           ## this seaons's color 
           set iS $iSeason($s)
           pgstateSet $pg -icLine $iS -lineWidth 1
           vPlot $pg $pix $FF($s,$c) -xmin 100 -xmax 1950 -ymin $yMin -ymax $yMax             
           pgstateSet $pg -isNewplot 0
        }

        # to guide the eye...
        pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1
        plot_line $pg 0 1 2048 1 1 14
        plot_line $pg 2048 -1000 2048 1000 1 14
        plot_line $pg 0 -1000 0 1000 1 14
        xlabel "chip column (pixel)"
        ylabel "(inverse) FF" 
        # which camCol 
        set yText [expr $yMin+0.1*($yMax-$yMin)]
        pgText 200 $yText "camCol $c"
        if {$c == [lindex $camCols 0]} {titlePlot "$title" 20} 
        if {$c == [lindex $camCols 4]} {
           # info
           plot_info "Each line shows flatfield for one season"  0 -1.0 -0.6
        }
        pgstateSet $pg -isNewplot 1
    }
             
    # end plot
    endplot $pg

    # clean 
    vectorExprDel $pix
    foreach s $seasons {
       foreach c $camCols {
	  vListDel "$FF($s,$c)"
       }
    }   
}



#### plot QA plots for flatfield correction
## given a list of vector lists returned by idFFcorrect, the corresponding
## camCol list, produce ps file with QA (/XWINDOW for psFileName =="")
proc idFFcorrectQA {vList {camCols "all"} {title ""} {psFile ""}} {

    if {$camCols == "all"} {set camCols "1 4 2 5 3 6"}       

    assert { [llength $vList] == [llength $camCols] }
 

    ### setup this plot
    set pg [pgstateNew]
    if {$psFile == ""} {
        set dev /XWINDOW
    } else  {
        set dev /VCPS
    }
    set device [devicename $dev $psFile] 
    pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
          -xfract 0.28 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
    set icLineDef [exprGet $pg.icLine]
    pgstateOpen $pg

    set statsList ""
    ### loop over camCols
    foreach camCol $camCols {
       set iCC [lsearch $camCols $camCol]
       set vecs [lindex $vList $iCC]
       ### aux names for vectors (need to be compatible with idFFcorrect)
       # P for points (coarse grid), L for lines (fine grid)
       set binP [lindex $vecs 0]
       set oldFFP [lindex $vecs 3] 
       set newFFP [lindex $vecs 4] 
       set binL [lindex $vecs 6] 
       set oldFFL [lindex $vecs 7] 
       set newFFL [lindex $vecs 8] 
       # determine y axis range
       set yMin 0.95; set yMax 1.05
       foreach k "3 4 7 8" {
           set min [vExtreme [lindex $vecs $k] min] 
           if {$min < $yMin} {set yMin $min}
           set max [vExtreme [lindex $vecs $k] max] 
           if {$max > $yMax} {set yMax $max}
       }

       ### and now plot
       ## old line
       pgstateSet $pg -icLine 2 -lineWidth 1
       vPlot $pg $binL $oldFFL -xmin 100 -xmax 1950 -ymin $yMin -ymax $yMax  
       ## old points
       #pgstateSet $pg -isNewplot 0 -isLine 0 -icMark 2 -symb 3
       #vPlot $pg $binP $oldFFP -xmin 100 -xmax 1950 -ymin $yMin -ymax $yMax  
       ## new line
       pgstateSet $pg -isNewplot 0 -icLine 4 -isLine 1 -lineWidth 3
       vPlot $pg $binL $newFFL -xmin 100 -xmax 1950 -ymin $yMin -ymax $yMax  
       ## old points
       pgstateSet $pg -isNewplot 0 -isLine 0 -icMark 4 -symb 4
       vPlot $pg $binP $newFFP -xmin 100 -xmax 1950 -ymin $yMin -ymax $yMax  
       
       # let's compute some stats (in milimags)
       set magFFaux [vectorExprEval -2.5*lg($newFFL)*1000]
       set mean [vMean $magFFaux]
       set magFF [vectorExprEval $magFFaux-$mean] 
       # rms error if flatfield vector set to 1
       set rmsUnity [format "%5.1f" [vSigma $magFF]]
       set magCorraux [vectorExprEval -2.5*lg($oldFFL/$newFFL)*1000]
       set mean [vMean $magCorraux]
       set magCorr [vectorExprEval $magCorraux-$mean]
       # rms correction
       set rmsCorr [format "%5.1f" [vSigma $magCorr]]
       # extremes for correction
       set aux [vectorExprEval abs($magCorr)]
       set maxCorr [format "%5.1f" [vExtreme $aux max]]
       vListDel "$magFF $magCorr $magFFaux $magCorraux $aux"
       set yText [expr $yMax-0.1*($yMax-$yMin)]
       pgText 30 $yText "rms=$rmsCorr, max=$maxCorr"
       set statsList "$statsList {$rmsUnity   $rmsCorr   $maxCorr}"
       # to guide the eye...
       pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1
       plot_line $pg 0 1 2048 1 1 14
       #plot_line $pg 0 1.05 2048 1.05 1 14
       #plot_line $pg 0 0.95 2048 0.95 1 14
       plot_line $pg 2048 -1000 2048 1000 1 14
       plot_line $pg 0 -1000 0 1000 1 14
       xlabel "chip column (pixel)"
       ylabel "(inverse) FF" 
       # which camCol 
       set yText [expr $yMin+0.1*($yMax-$yMin)]
       pgText 200 $yText "camCol $camCol"
       if {$camCol == [lindex $camCols 0]} {titlePlot "$title" 20} 
       if {$camCol == [lindex $camCols 4]} {
          # info
          plot_info "The red line is the old flatfield"  0 -1.0 -0.6
          plot_info "The blue line and circles is the new flatfield"  0 -1.0 -0.75
          plot_info "Symbols: the binned correction, line: Tchebyshev fit"  0 -1.0 -0.90   
          plot_info "Numbers: rms and max. dev for the FF correction (mmags)"  0 -1.0 -1.05   
       }
       pgstateSet $pg -isNewplot 1
    }
             
    # end plot
    endplot $pg

   return "$statsList"     
}



### given an input idFF file and a binned FF correction file, make new idFF file
### by forcing the final flatfield to be as smooth as a Ntcheby-th order poly
### return all relevant vectors 
### e.g
# set idFFold idFFdir/iteration0/idFF-000060-u5.fit
# set idFFnew idFFdir/iteration1/idFF-001060-u5.fit 
# set FFcorrFile DumpDir/Season6BinnedFFcorr32_u5.dat
# set FFiter 1
proc idFFcorrect {idFFold FFcorrFile idFFnew FFiter {Ntcheby 5} {testTerm 0}} {

   # fir fixing jump at 1024 for 2-amp chips
   set nSplit 1024
   set Lbuffer 100
   set Rbuffer 100

      ### first read old idFF 
      set idFF [regReadAsFits [regNew] $idFFold]
      # get vector out of this 1xNcol region
      set Ncol [exprGet $idFF.ncol]
      set aux [vectorGetFromRegion $idFF 0 0 0 [expr $Ncol-1] 0]
      # rescale it to ~1
      set tshift [hdrGetAsInt $idFF.hdr TSHIFT]
      set idFFvecOld [vectorExprEval 1.0*$aux/$tshift]
      # fix the break for 2-amp chips
      set vJump [get_jump $idFF $nSplit $Lbuffer $Rbuffer 4]
      set Jmedian [exprGet $vJump.vec<0>]
      set FFcorrectionL [format "%6.4f" [expr 2.0*$Jmedian/(1+$Jmedian)]]
      set FFcorrectionR [format "%6.4f" [expr 2.0/(1+$Jmedian)]]
      loop k 0 $Ncol {
          set val [exprGet $idFFvecOld.vec<$k>]
	  if {$k < $nSplit} {
              handleSet $idFFvecOld.vec<$k> [expr $FFcorrectionL*$val]
          } else {
              handleSet $idFFvecOld.vec<$k> [expr $FFcorrectionR*$val]
          }
      }      
      vListDel "$aux $vJump"
      # and the fine 2048 pixel grid
      set binFF [vIndex [exprGet $idFFvecOld.dimen]]
      #### fix keywords
      set PS_ID [idStamp]
      set version [photoVersion]
      # these two are kept under different names
      set IterIn   [hdrGetAsInt $idFF.hdr FFITER]
      set OldPS_ID [hdrGetAsAscii $idFF.hdr PS_ID]
      # remove old keywords
      catch {hdrDelByKeyword $idFF.hdr OLDPS_ID}
      hdrDelByKeyword $idFF.hdr FFITER
      hdrDelByKeyword $idFF.hdr PS_ID
      hdrDelByKeyword $idFF.hdr PHOT_VER
      hdrDelByKeyword $idFF.hdr ITERIN
      # and add new ones
      hdrInsWithInt $idFF.hdr FFITER $FFiter "Flatfield iteration"
      hdrInsWithInt $idFF.hdr ITERIN $IterIn "Flatfield iteration used as input"
      hdrInsWithAscii $idFF.hdr PS_ID $PS_ID 
      hdrInsWithAscii $idFF.hdr OLDPS_ID $OldPS_ID "PS_ID from input idFF file"
      hdrInsWithAscii $idFF.hdr PHOT_VER $version "Photo version used"

      ### read FF correction
      set bin [Tfile2Vector $FFcorrFile 1 8]
      set FFcorr [Tfile2Vector $FFcorrFile 2 8]
      set FFcorrErr [Tfile2Vector $FFcorrFile 3 8]
    #### JEG trick: increase the error in edge bins by a factor Ferr
      if {1} {
          set Ferr 1.0; # iteration 1A
          set Ferr 2.0; # iteration 1B
          set Ferr 4.0; # iteration 1C
          set Ferr 3.0; # iteration 1
          set new [expr $Ferr*[exprGet $FFcorrErr.vec<0>]]
          handleSet $FFcorrErr.vec<0> $new
          set Nm1 [expr [exprGet $FFcorrErr.dimen]-1]
          set new [expr $Ferr*[exprGet $FFcorrErr.vec<$Nm1>]]
          handleSet $FFcorrErr.vec<$Nm1> $new 
      }
      set FFweight [vectorExprEval 1.0/$FFcorrErr/$FFcorrErr]
      set binSize [expr 2*[exprGet $bin.vec<0>]]
      set Nbins [exprGet $bin.dimen]
      
      ### now that we know binning, get the binned version of input FF
      set oldFF [vectorExprEval 0*$bin]
      loop i 0 $Nbins {
	 set binL [expr $i * $binSize]
         set binR [expr ($i+1) * $binSize]
         # mask for this bin
         set binmask [vectorExprEval "$binFF >= $binL && $binFF < $binR ? 1 : 0"]
         set medFF [vMedian $idFFvecOld -vMask $binmask]
         handleSet $oldFF.vec<$i> $medFF 
      }

      ### now apply the binned correction to the binned old FF
      ## from milimags to factors   
      set MultFFcorr [vectorExprEval 10^(0.4*$FFcorr/1000)]
      set newFF [vectorExprEval 0*$bin]
      loop i 0 $Nbins {
         set C [exprGet $MultFFcorr.vec<$i>]
         assert { $C > 0.5 && $C < 1.5 } 
         set old [exprGet $oldFF.vec<$i>]  
         handleSet $newFF.vec<$i> [expr $C*$old]
      }
      vectorExprDel $MultFFcorr
     
      ### given the new binned flatfield, newFF, fit a Tchebyshev polynomial
      ### to newFF(bin), and evaluate it for x=bin and x=binFF 
      set newFFfit [tchebyFit $bin $newFF $FFweight $Ntcheby] 
      set idFFvecNew [tchebyFit $bin $newFF $FFweight $Ntcheby -fitx $binFF]
      vectorExprDel $FFweight

      ### transfer vector to region and dump new idFF
      loop i 0 $Ncol {
         set C [exprGet $idFFvecNew.vec<$i>]
         ### this is a fake term for testing
	 if {$testTerm} {
            set addC [expr 0.2/2048 * [exprGet $binFF.vec<$i>]]
            echo "WARNING: adding term for testing ($addC): $idFFnew"
            set C [expr $C + $addC]
         }
         assert { $C > 0.5 } 
         assert { $C < 1.5 } 
         handleSet $idFF.rows_u16<0><$i> [expr $tshift*$C]
      }      
      regWriteAsFits $idFF $idFFnew
      regDel $idFF

      # and return all vectors
      set fineList "$binFF $idFFvecOld $idFFvecNew"
      set coarseList "$bin $FFcorr $FFcorrErr $oldFF $newFF $newFFfit"  

    return "$coarseList $fineList"

}

    



### aux proc to search through data_root and find all runs from given 
### rerun and season that have both
### 1) ColorCorrPsf*dat files in nfcalib/ptflatmatch subdir 
###    (produced by doPCrerun from tsObj files)
### 2) m25mptcolc*par files in nfcalib/ptflatmatch subdir
###    (produced by Yanny from nfcalib outputs)
proc getGoodFFrrListForSeason {rerun season {runsToExclude ""}} {

global data_root 

       set runRange [getFFseasonRunRange $season]

       set root m25mptcolc
       set PTlist [glob $data_root/*/$rerun/nfcalib/ptflatmatch/${root}*z6.par]
       set PTlistOK [get_runrerunList $PTlist 3 [lindex $runRange 0] [lindex $runRange 1]]
   
       set root ColorCorrPsf_y
       set PClist [glob $data_root/*/$rerun/nfcalib/ptflatmatch/${root}*c6.dat]
       set PClistOK [get_runrerunList $PClist 3 [lindex $runRange 0] [lindex $runRange 1]]        

       # and now match the two good lists
       set list ""
       foreach run $PTlistOK {
	   if {[lsearch $PClistOK $run] > -1 && [lsearch $list $run] < 0} {
               if {[lsearch $runsToExclude [lindex $run 0]] < 0} {
                  set list "$list {$run}"
               }
           }
       }

   return $list
}



### similar to getFFseasonBinnedCorr, except that it takes an arbitrary 
# list of {run rerun}
proc getFFbinnedCorr {runrerunList {outputDir "."} {seasonName X} \
	     {title ""} {binSizePC 32} {binSizePT 64} {Ntcheby 5}} {

global data_root

         if {$outputDir == "."} {
            set outputDir [pwd]
         }

         #### PT block ####
         # make output data files
         set outname Season${seasonName}BinnedPTdm$binSizePT
         processPTfiles2 $runrerunList $outputDir $outname $binSizePT
         # make plots
	 if {$title == ""} {
            set titlePT "Data: $runrerunList, PT constraint ($binSizePT pix bins, Ntcheby=$Ntcheby)"
         } else {
            set titlePT "$title, PT constraint ($binSizePT pix bins, Ntcheby=$Ntcheby)"
         }
         plotPTconstraint $Ntcheby $outputDir $outname $outputDir $outname.ps $titlePT /VCPS
         echo "done with $title"


         #### PC block ####
         # make output data files
         echo "  **** executing PC block ****"
         set outname Season${seasonName}BinnedPCdm$binSizePC
         processPCfiles $runrerunList $outputDir $outname $binSizePC
         # make plots
	 if {$title == ""} {
            set titlePC "Data: $runrerunList, PC constraints ($binSizePC pix bins)"
         } else {
            set titlePC "$title, PC constraints ($binSizePC pix bins)"
         }
         plotPCcoadded $outputDir $outname $outputDir $outname.ps $titlePC /VCPS
         echo "  --- done with PC block ---"


         #### FF block ####
         doFFseasons $outputDir $outputDir $seasonName all $binSizePC $binSizePT $Ntcheby
         # make plots
	 if {$title == ""} {
            set titleFF "Data: $runrerunList, Binned Flatfield Corrections"
         } else {
            set titleFF "$title, Binned Flatfield Corrections"
         }
         plotBinnedFFCorr $titleFF $outputDir $outputDir $binSizePC BinnedFFCorr $seasonName
         plotBinnedFFCorrColor $titleFF $outputDir $outputDir $binSizePC BinnedFFCorrColor $seasonName

    echo ""
    echo "**********************************************************"
    echo "** Produced binned FF corrections for run/rerun list:"
    echo "** $runrerunList"
    echo "** and stored to Season${seasonName}Binned\[PT,PC,FF\]*dat files in directory"
    echo "** $outputDir"
    echo "**********************************************************"

}




### a wrapper to produce binned flatfield corrections from the binned PC and PT constraints,
### as well as QA plots
proc doFFseasons {{inputDir "."} {outputDir "."} {seasons "all"} {camCols "all"} \
                  {binSizePC 32} {binSizePT 64} {Ntcheby 5}} {

        ### definition of the PT white magnitude ###
        # n.b. these coefficients can be arbitrarily changed here 
        set K1 0
        set K2 0.333
        set K3 0.333
        set K4 0.333
        set K5 0
        # given K1...K5, get the solution matrix 
        # n.b. Alist is a list of 5 vectors (rows in matrix A^{-1}, each with 5 elements)
        set Alist [invPCmat $K1 $K2 $K3 $K4 $K5]

        if {$inputDir == "."} {
           set inputDir [pwd]
        }
        if {$outputDir == "."} {
           set outputDir [pwd]
        }

        ### defaults
        if {$seasons == "all"} {set seasons "1 2 3 4 5 6 7 8 9"}
        if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}
        set filterlist "u g r i z"
        set PCcolors "s w x y"

        ## loop over seasons 
        foreach season $seasons {
           ## loop over camCols
           foreach camCol $camCols {
              echo " == doFFseasons: processing season $season, camCol $camCol"
              ## PC constraints
              foreach PCcolor $PCcolors {
	          set fname $inputDir/Season${season}BinnedPCdm${binSizePC}-${PCcolor}${camCol}.dat
                  # read this color's data: clipped mean and its error
                  set bin($PCcolor)     [Tfile2Vector $fname 1 5]
                  set mean($PCcolor)    [Tfile2Vector $fname 2 5]
                  set meanErr($PCcolor) [Tfile2Vector $fname 3 5]
                  # make sure all bin vectors are the same
	          if {$PCcolor != [lindex $PCcolors 0]} {
                     assert { [exprGet $bin($PCcolor).dimen] == [exprGet $bin([lindex $PCcolors 0]).dimen] }
                  }
	      }
              set PCbins $bin([lindex $PCcolors 0])
              ## PT constraints
              set fileRoot "$inputDir/Season${season}BinnedPTdm${binSizePT}-"
              set PTconstraint [getPTconstraint $fileRoot $camCol $K1 $K2 $K3 $K4 $K5]
              set binPTvec [lindex $PTconstraint 0]
              set K6vec    [lindex $PTconstraint 1]
              set K6errvec [lindex $PTconstraint 2]
              ## if binPC and binPT are not the same, rebin PT constraints to the PC grid
              ## fit the Ntcheby-th order Tchebychev polynomial to the PC grid
              echo "    .... rebinning PT constraints to PC grid ...."
              set K6vecOld [vectorExprEval $K6vec]
              set K6errvecOld [vectorExprEval $K6errvec]
              # first dt
              set wdt [vectorExprEval 1.0/$K6errvec/$K6errvec]
              set K6vec [tchebyFit $binPTvec $K6vec $wdt $Ntcheby -fitx $PCbins]
              # and then dtErr (all weights equal)
              set wdtErr [vectorExprEval 1.0+0*$K6errvec]
              set K6errvec [tchebyFit $binPTvec $K6errvec $wdtErr $Ntcheby -fitx $PCbins]
              # clean
              vListDel "$K6vecOld $K6errvecOld $wdt $wdtErr"
              ### sanity check
              if {[exprGet $PCbins.dimen] != [exprGet $K6vec.dimen]} {
                  echo " doFFseasons: vectors PCbins and K6vec have different dimensions"
                  echo " PCbins.dimen = [exprGet $PCbins.dimen], K6vec.dimen = [exprGet $K6vec.dimen]" 
                  echo " for season = $season and camCol = $camCol"     
                  set pc [lindex $PCcolors 0]
                  set fname $inputDir/Season${season}BinnedPCdm${binSizePC}-${pc}${camCol}.dat      
                  echo " PC obtained from $fname"
                  error "PT obtained with ($K1 $K2 $K3 $K4 $K5) from $fileRoot"
              }
              ### now we have all PC colors and PT constraint on the same grid, solve for ugriz
              ### loop over all 5 SDSS bands 
              loop i 0 5 {
	         set band [lindex $filterlist $i]   
                 # make correction and correction error vectors 
	         set corr($band)    [vectorExprNew [exprGet $bin([lindex $PCcolors 0]).dimen]]
	         set corrErr($band) [vectorExprNew [exprGet $bin([lindex $PCcolors 0]).dimen]]
                 # this row of matrix coefficients (corresponding to this band)
                 set coeffs [lindex $Alist $i]
                 set aux [exprGet $coeffs.vec<4>]
                 ## loop over all pixel bins
	         loop j 0 [exprGet $bin([lindex $PCcolors 0]).dimen] {
                     # start with the term that couples PT constraint (note the sign change)
		     handleSet $corr($band).vec<$j> [expr -1.0*[exprGet $K6vec.vec<$j>]*$aux]
                     set corrVar [expr pow([exprGet $K6errvec.vec<$j>]*$aux,2)]
                     # and now loop over all 4 PCs
                     foreach PCcolor $PCcolors {
       	                set k [lsearch $PCcolors $PCcolor]
                        # NB: corrections are given in milimags
		        set Smean [exprGet $mean($PCcolor).vec<$j>]
		        set SmeanErr [exprGet $meanErr($PCcolor).vec<$j>]
       	                set thisCorr [expr [exprGet $coeffs.vec<$k>]*$Smean]
		        handleSet $corr($band).vec<$j> [expr $thisCorr+[exprGet $corr($band).vec<$j>]]
       	                set thisCorrVar [expr pow([exprGet $coeffs.vec<$k>]*$SmeanErr,2)]
		        set corrVar [expr $thisCorrVar+$corrVar]
                     }
                     ### get error for this band
                     handleSet $corrErr($band).vec<$j> [expr sqrt($corrVar)]
                 }
                 # all set, dump bin, correction and correction error vectors for this band
	         set outname $outputDir/Season${season}BinnedFFcorr${binSizePC}_${band}${camCol}.dat
                 doFFseasonCorrFileHeader $outname $camCol $band $inputDir
                 # dump pixBins, color correction and its error to file
                 set f [open $outname a]
                 set Nrows [exprGet $PCbins.dimen]
                 puts $f "# binCenter (pix)  FFcorr     FFcorrErr  corr/corrErr"
                 puts $f "#                  -- in 1/1000 mag --"
                 ## loop over all entries
                 loop j 0 $Nrows {
                    # bin position
		    set v1 [format "%4.0f" [exprGet $PCbins.vec<$j>]]
                    # correction
		    set v2 [format "%7.2f" [exprGet $corr($band).vec<$j>]]
		    set v3 [format "%7.2f" [exprGet $corrErr($band).vec<$j>]]
                    set v4 [format "%7.2f" [expr $v2/(abs($v3)+0.000001)]]
                    puts $f "       $v1      $v2      $v3     $v4"
                 }
                 # close file
                 close $f 
                 # clean
                 vListDel "$corr($band) $corrErr($band)"
              }

              ## clean
              foreach PCcolor $PCcolors {
                 vListDel "$bin($PCcolor) $mean($PCcolor) $meanErr($PCcolor)"
              }                                
              vListDel "$binPTvec $K6vec $K6errvec" 
           }   
        }

       # clean
       vListDel $Alist

}
          

### after running doFFseasons for all camCols in a given season, produce 
### 6-panel plots with corrections for the specified filters
proc plotBinnedFFCorr {{title ""} {inputDir "."} {outputDir "."} {binSizePC 32} \
       {outputRootPSname BinnedFFCorr} {seasons "all"} {dev /VCPS} {filterlist "all"}} {

        if {$inputDir == "."} {
           set inputDir [pwd]
        }
        if {$outputDir == "."} {
           set outputDir [pwd]
        }

        ### defaults
        if {$seasons == "all"} {set seasons "1 2 3 4 5 6 7 8 9"}
        if {$filterlist == "all"} {set filterlist "u g r i z"}
        set camCols "1 4 2 5 3 6"

        ### loop over seasons 
        foreach season $seasons {
           ### loop over filters
           foreach f $filterlist {   
              echo " == plotBinnedFFCorr: processing season $season, band $f"
              # y axis range
              set dmMin -20; set dmMax 20 
              ### setup this plot
              set pg [pgstateNew]
	      set outputPSname "$outputDir/${outputRootPSname}-season$season-${f}.ps"
              set device [devicename $dev "$outputPSname"] 
              pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
                      -xfract 0.25 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
              set icLineDef [exprGet $pg.icLine]
              pgstateOpen $pg

              ### first loop over camCols: read all vectors and get overall min/max 
              foreach camCol $camCols {
                 # first read all vectors (and get min/max)
                 set file $inputDir/Season${season}BinnedFFcorr${binSizePC}_${f}${camCol}.dat
                 if {![file exist $file]} {
                    echo "CANNOT find file"
                    error $file
	         } else {
                    set bin($camCol)     [Tfile2Vector $file 1 8]
                    set corr($camCol)    [Tfile2Vector $file 2 8]
                    set corrErr($camCol) [Tfile2Vector $file 3 8]
                 }
                 set min [vExtreme $corr($camCol) min]
                 if {$min < $dmMin} {set dmMin $min}
                 set max [vExtreme $corr($camCol) max]
                 if {$max > $dmMax} {set dmMax $max}
                 if {$dmMin < -100} {set dmMin -100}                 
                 if {$dmMax > 100} {set dmMax 100}                 
	      }
              ### second loop over camCols: plot and clean 
              foreach camCol $camCols {
                 # and now plot
                 set Yminus [vectorExprEval $corr($camCol)-$corrErr($camCol)]
                 set Yplus [vectorExprEval $corr($camCol)+$corrErr($camCol)]
                 pgstateSet $pg -icLine 2 -lineWidth 1
                 vPlot $pg $bin($camCol) $Yminus -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
                 pgstateSet $pg -isNewplot 0
                 vPlot $pg $bin($camCol) $Yplus -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
                 pgstateSet $pg -isNewplot 0 -icLine 4 -lineWidth 3
                 vPlot $pg $bin($camCol) $corr($camCol) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
                 pgstateSet $pg -isNewplot 0
                 vListDel "$bin($camCol) $corr($camCol) $corrErr($camCol) $Yplus $Yminus"
                 # to guide the eye...
                 pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1 -icLine 15
                 plot_line $pg -1000 0 10000 0 1 4 
                 plot_line $pg -1000 -10 10000 -10 1 4 
                 plot_line $pg -1000 10 10000 10 1 4 
                 plot_line $pg 0 -1000 0 1000 1 2
                 plot_line $pg 2048 -1000 2048 1000 1 2
		 if {$dmMin < -50 && $dmMax > 50} {
                    plot_line $pg -1000 -50 10000 -50 1 2
                    plot_line $pg -1000 50 10000 50 1 2
	         }
                 xlabel "chip column (pixel)"
                 ylabel "dm (milimag)" 
                 pgstateSet $pg -isNewplot 1
                 # which camCol 
                 set yText [expr $dmMin+0.1*($dmMax-$dmMin)]
                 pgText 400 $yText "camCol $camCol"
                 if {$camCol == [lindex $camCols 0]} {titlePlot "$title" 20} 
                 if {$camCol == [lindex $camCols 4]} {
                    # info
                    plot_info "The flatfield correction for season $season and filter $f" 0 -1.0 -0.6
                    plot_info "The blue line is the correction and the two red lines"  0 -1.0 -0.75
                    plot_info "    are +-1sigma envelope" 0 -1.0 -0.9
		    if {$dmMin < -50 && $dmMax > 50} {
                        plot_info "The horizonatal blue lines: +-1%, red lines: +-5%"  0 -1.0 -1.05
                    } else {
                        plot_info "The horizonatal blue lines: +-1%"  0 -1.0 -1.05
                    }
                 }
	      }
              # end plot
              endplot $pg   
         }
      }
}



### very similar to plotBinnedFFCorr, except that it plots SDSS colors
### instead of bands
proc plotBinnedFFCorrColor {{title ""} {inputDir "."} {outputDir "."} {binSizePC 32} \
       {outputRootPSname BinnedFFCorrColor} {seasons "all"} {dev /VCPS} {colorlist "all"}} {

        if {$inputDir == "."} {
           set inputDir [pwd]
        }
        if {$outputDir == "."} {
           set outputDir [pwd]
        }

        ### defaults
        if {$seasons == "all"} {set seasons "1 2 3 4 5 6 7 8 9"}
        if {$colorlist == "all"} {set colorlist "ug gr ri iz"}
        set filterlist "u g r i z"
        set camCols "1 4 2 5 3 6"
       
        # color definition
        set f1(ug) u; set f2(ug) g 
        set f1(gr) g; set f2(gr) r
        set f1(ri) r; set f2(ri) i 
        set f1(iz) i; set f2(iz) z 

        ### loop over seasons 
        foreach season $seasons {
           ### loop over colors 
           foreach color $colorlist {   
              echo " == plotBinnedFFCorrColor: processing season $season, color $color"
              # y axis range
              set dmMin -20; set dmMax 20 
              ### setup this plot
              set pg [pgstateNew]
	      set outputPSname "$outputDir/${outputRootPSname}-season$season-${color}.ps"
              set device [devicename $dev "$outputPSname"] 
              pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
                      -xfract 0.25 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
              set icLineDef [exprGet $pg.icLine]
              pgstateOpen $pg

              ### first loop over camCols: read all vectors and get overall min/max 
              foreach camCol $camCols {
                 # first read all vectors (and get min/max)
                 set fList "$f1($color) $f2($color)"
                 foreach f $fList {
                    set file $inputDir/Season${season}BinnedFFcorr${binSizePC}_${f}${camCol}.dat
                    if {![file exist $file]} {
                       echo "CANNOT find file"
                       error $file
	            } else {
                       set binF($f,$camCol)     [Tfile2Vector $file 1 8]
                       set corrF($f,$camCol)    [Tfile2Vector $file 2 8]
                       set corrErrF($f,$camCol) [Tfile2Vector $file 3 8]
                    }
		 }
                 assert { [exprGet $binF($f1($color),$camCol).dimen] == [exprGet $binF($f2($color),$camCol).dimen] }
                 set bin($camCol)  [vectorExprEval $binF($f1($color),$camCol)]
                 set corr($camCol) [vectorExprEval $corrF($f1($color),$camCol)-$corrF($f2($color),$camCol)]
                 set aux1 $corrErrF($f1($color),$camCol); set aux2 $corrErrF($f2($color),$camCol)
                 set corrErr($camCol) [vectorExprEval sqrt($aux1*$aux1+$aux2*$aux2)]
                 foreach f $fList {
                    vListDel "$binF($f,$camCol) $corrF($f,$camCol) $corrErrF($f,$camCol)"
                 }                 
                 set min [vExtreme $corr($camCol) min]
                 if {$min < $dmMin} {set dmMin $min}
                 set max [vExtreme $corr($camCol) max]
                 if {$max > $dmMax} {set dmMax $max}
                 if {$dmMin < -100} {set dmMin -100}                 
                 if {$dmMax > 100} {set dmMax 100}                 
	      }
              ### second loop over camCols: plot and clean 
              foreach camCol $camCols {
                 # and now plot
                 set Yminus [vectorExprEval $corr($camCol)-$corrErr($camCol)]
                 set Yplus [vectorExprEval $corr($camCol)+$corrErr($camCol)]
                 pgstateSet $pg -icLine 2 -lineWidth 1
                 vPlot $pg $bin($camCol) $Yminus -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
                 pgstateSet $pg -isNewplot 0
                 vPlot $pg $bin($camCol) $Yplus -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
                 pgstateSet $pg -isNewplot 0 -icLine 4 -lineWidth 3
                 vPlot $pg $bin($camCol) $corr($camCol) -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
                 pgstateSet $pg -isNewplot 0
                 vListDel "$bin($camCol) $corr($camCol) $corrErr($camCol) $Yplus $Yminus"
                 # to guide the eye...
                 pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1 -icLine 15
                 plot_line $pg -1000 0 10000 0 1 4 
                 plot_line $pg -1000 -10 10000 -10 1 4 
                 plot_line $pg -1000 10 10000 10 1 4 
                 plot_line $pg 0 -1000 0 1000 1 2
                 plot_line $pg 2048 -1000 2048 1000 1 2
		 if {$dmMin < -50 && $dmMax > 50} {
                    plot_line $pg -1000 -50 10000 -50 1 2
                    plot_line $pg -1000 50 10000 50 1 2
	         }
                 xlabel "chip column (pixel)"
                 ylabel "dm (milimag)" 
                 pgstateSet $pg -isNewplot 1
                 # which camCol 
                 set yText [expr $dmMin+0.1*($dmMax-$dmMin)]
                 pgText 400 $yText "camCol $camCol"
                 if {$camCol == [lindex $camCols 0]} {titlePlot "$title" 20} 
                 if {$camCol == [lindex $camCols 4]} {
                    # info
                    plot_info "The flatfield correction for season $season and color $color" 0 -1.0 -0.6
                    plot_info "The blue line is the correction and the two red lines"  0 -1.0 -0.75
                    plot_info "    are +-1sigma envelope" 0 -1.0 -0.9
		    if {$dmMin < -50 && $dmMax > 50} {
                        plot_info "The horizonatal blue lines: +-1%, red lines: +-5%"  0 -1.0 -1.05
                    } else {
                        plot_info "The horizonatal blue lines: +-1%"  0 -1.0 -1.05
                    }
                 }
	      }
              # end plot
              endplot $pg   
         }
      }
}




# aux proc to write header to Season?BinnedFFcorr*.dat   files
proc doFFseasonCorrFileHeader {filename camCol filter inputDir} {

       # open file
       set f [open $filename w]
       set date [exec date]
       set user [exec whoami]
       puts $f "#  Flatfield Correction vs. Chip Column Position for chip ${filter}${camCol}"
       puts $f "#  determined from data files in directory"
       puts $f "#  $inputDir"
       puts $f "#  produced using photo version [photoVersion]"
       puts $f "#  by $user on $date"
       # close file
       close $f
}


### a wrapper to produce PC constraints and plots for all seasons, given a rerun list
proc doPCseasons {rerunList {seasons "all"} {outputDir "."} {binSize 32}} {

global data_root

      if {$seasons == "all"} {set seasons "1 2 3 4 5 6 7 8 9"}

      # first get all runs
      set root ColorCorrPsf_y
      set list ""
      foreach rerun $rerunList {
	 set list "$list [glob $data_root/*/$rerun/nfcalib/ptflatmatch/${root}*c6.dat]"
      } 
      echo " === extracted run list, going to process all seasons ==="; echo ""

      # and now loop over seasons 
      foreach season $seasons {
         set runRange [getFFseasonRunRange $season]
         set rrList [get_runrerunList $list 3 [lindex $runRange 0] [lindex $runRange 1]]
         # make output data files
         set outname Season${season}BinnedPCdm$binSize
         processPCfiles $rrList $outputDir $outname $binSize
         # make plots
         set title "Season $season, PC constraints ($binSize pix bins)"
         plotPCcoadded $outputDir $outname $outputDir $outname.ps $title /VCPS
         echo "done with $title"
      }

}


# a wrapper to produce PT constraints and plots for all seasons, given a rerun list
proc doPTseasons {rerunList {seasons "all"} {outputDir "."} {binSize 32} {Ntcheby 10}} {

global data_root

      if {$seasons == "all"} {set seasons "1 2 3 4 5 6 7 8 9"}

      # first get all runs
      set root m25mptcolc
      set list ""
      foreach rerun $rerunList {
	 set list "$list [glob $data_root/*/$rerun/nfcalib/ptflatmatch/${root}*z6.par]"
      } 
      echo " === extracted run list, going to process all seasons ==="; echo ""

      # and now loop over seasons 
      foreach season $seasons {
         set runRange [getFFseasonRunRange $season]
         set rrList [get_runrerunList $list 3 [lindex $runRange 0] [lindex $runRange 1]]
         # make output data files
         set outname Season${season}BinnedPTdm$binSize
         processPTfiles2 $rrList $outputDir $outname $binSize
         # make plots
         set title "Season $season, PT constraint ($binSize pix bins, Ntcheby=$Ntcheby)"
         plotPTconstraint $Ntcheby $outputDir $outname $outputDir $outname.ps $title /VCPS
         echo "done with $title"
      }

}



### proc to process m25mptcolc-$run-$rerun-$f$c.par files; similar to processPTfiles
### but operates on a list of {run rerun} and treats all the files simultaneously
### e.g.
### sdr; set list [glob $data_root/3???/2?/nfcalib/ptflatmatch/m25mptcolc*z6.par]
### set rrList [get_runrerunList $list 3]
### processPTfiles2 $rrList . Season7BinnedPTdm64 64 default 6 r
proc processPTfiles2 {runrerunList {outputDir "."} {outputName BinnedPTdm} {binSize 32} \
            {inputDir "default"} {camCols "all"} {filterlist "all"} {PTroot m25mptcolc} {dataRoot ""}} {

      if {$filterlist == "all"} {set filterlist "u g r i z"}
      if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}
      if {$dataRoot == ""} {
          set data_root [get_data_root ""]
      }
      if {![file exist $data_root]} {
          error "CANNOT FIND data_root=$data_root"
      }
      if {$inputDir == "default"} {
          set inputDir "nfcalib/ptflatmatch"
      }
      if {$outputDir == "default"} {
          set outputDir "[pwd]"
      }


      ### loop over all requested chips 
      foreach camCol $camCols {
         foreach f $filterlist {
            echo "processPTfiles2 for camCol $camCol, filter $f"
            # first make PT file list 
            set PTfileList ""
            # loop over all requested run/rerun pairs
            foreach rr $runrerunList {
               set run [lindex $rr 0]
               set runstr [format %06d $run]
               set rerun [lindex $rr 1]
               set dir $data_root/${run}/${rerun}/$inputDir
	       set PTfile $dir/m25mptcolc-${runstr}-${rerun}-${f}${camCol}.par
               if {![file exist  $PTfile]} {
                   error "processPTfile: CANNOT FIND file $PTfile"
               }
               set PTfileList "$PTfileList $PTfile"
	    }
            set chain [parList2Chain $PTfileList]  
	    set outFile $outputDir/${outputName}-${f}${camCol}.dat
            processPTchain $chain $outFile $f $binSize $runrerunList
            chainDestroy $chain genericDel 
         }
      }

}


### given a list of {run rerun}, process all ColorCorrPsf*dat for a given season
proc processPCfiles {runrerunList {outputDir "default"} {outputName BinnedPCdm} {binSize 32} \
            {inputDir "default"} {camCols "all"} {Pcolors "all"} {PCroot ColorCorrPsf} {dataRoot ""}} {

      if {$Pcolors == "all"} {set Pcolors "s w x y"}
      if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}
      if {$dataRoot == ""} {
          set data_root [get_data_root ""]
      }
      if {![file exist $data_root]} {
          error "CANNOT FIND data_root=$data_root"
      }
      if {$inputDir == "default"} {
          set inputDir "nfcalib/ptflatmatch"
      }
      if {$outputDir == "."} {
          set outputDir "[pwd]"
      }


      ### loop over all requested chips 
      foreach camCol $camCols {
         foreach f $Pcolors {
            echo "processPCfiles for camCol $camCol, filter $f"
            # first make PC file list 
            set PCfileList ""
            # loop over all requested run/rerun pairs
            foreach rr $runrerunList {
               set run [lindex $rr 0]
               set runstr [format %06d $run]
               set rerun [lindex $rr 1]
               set dir $data_root/${run}/${rerun}/$inputDir
	       set PCfile $dir/${PCroot}_${f}_run${run}_${rerun}_c${camCol}.dat
               if {![file exist  $PCfile]} {
                   error "processPCfile: CANNOT FIND file $PCfile"
               }
               set PCfileList "$PCfileList $PCfile"
	    }
	    set outFile $outputDir/${outputName}-${f}${camCol}.dat
            processPCfileList $PCfileList $outFile w $runrerunList
         }
      }

}



### given a list of ColorCorrPsf*dat, coadd them and dump the result to 
### outFile
proc processPCfileList {PCfileList outFile {mode w} {rrList ""}} {

        set Nfile 0        
  
        ### first read all files
        foreach fname $PCfileList { 
           if {![file exists $fname]} {
               error "File $fname doesn't exist!?!"
           }
           set bin($Nfile) [Tfile2Vector $fname 1 12]
           if {$Nfile > 0} {
              assert { [exprGet $bin($Nfile).dimen] == [exprGet $bin(0).dimen] }
           }
           set corr($Nfile) [Tfile2Vector $fname 6 12]
           set corrErr($Nfile) [Tfile2Vector $fname 7 12]
           set Nbin($Nfile) [Tfile2Vector $fname 9 12]
           incr Nfile
        }
        set pixBins [vectorExprEval $bin(0)]

        ### now coadd for all files
        loop i 0 $Nfile {
           set w [vectorExprEval 1.0/$corrErr($i)/$corrErr($i)]
           set x $corr($i)
	   if {$i>0} {
               set oSw $Sw
               set oSwx $Swx
               set oSwx2 $Swx2
               set oNbinTot $NbinTot
               set Sw   [vectorExprEval $Sw+$w]
               set Swx  [vectorExprEval $Swx+$w*$x]
               set Swx2 [vectorExprEval $Swx2+$w*$x*$x]
               set NbinTot [vectorExprEval $oNbinTot+$Nbin($i)]
               vListDel "$oSw $oSwx $oSwx2 $oNbinTot"
	   } else {
               set Sw   [vectorExprEval $w]
               set Swx  [vectorExprEval $w*$x]
               set Swx2 [vectorExprEval $w*$x*$x]
               set NbinTot [vectorExprEval $Nbin(0)]
           }
    	   vListDel "$w $bin($i) $corr($i) $corrErr($i) $Nbin($i)"
        }  
 
        ### and then get interesting quantities
        set corrTot    [vectorExprEval $Swx/$Sw]
        set corrTotErr [vectorExprEval 1.0/sqrt($Sw)]
        set corrChi2   [vectorExprEval ($Swx2-$corrTot*$corrTot*$Sw)/($Nfile-1)]
        vListDel "$Sw $Swx $Swx2"

        #### all set, now dump correction, correction error and chi2
        set f [open $outFile $mode]
        set Nrows [exprGet $pixBins.dimen]
        puts $f "# output from proc processPCfileList for run/rerun list:"
        puts $f "# $rrList"
        puts $f "# binCenter (pix)   corr     corrErr    corr/corrErr   chi2pdf   Ntotal"
        puts $f "#                  -- in 1/1000 mag --"
        # loop over all entries
        loop j 0 $Nrows {
           # bin position
           set v1 [format "%4.0f" [exprGet $pixBins.vec<$j>]]
           # correction
           set v2 [format "%7.2f" [exprGet $corrTot.vec<$j>]]
           # correction error
           set v3 [format "%7.2f" [exprGet $corrTotErr.vec<$j>]]
           # correction / correction error
           set v4 [format "%7.2f" [expr $v2/(abs($v3)+0.000001)]]
           # chi2pdf
           set v5 [format "%7.2f" [exprGet $corrChi2.vec<$j>]]
           # Ntot
           set v6 [format "%7.0f" [exprGet $NbinTot.vec<$j>]]
           puts $f "       $v1      $v2      $v3     $v4     $v5   $v6"
        }
        # close file
        close $f 
        ### clean
        vListDel "$corrTot $corrTotErr $corrChi2 $pixBins $NbinTot"

}



### after running processPCfileList for all camCols/principal colors,
### produce a 6-panel plot with swxy colors for all 6 cols
proc plotPCcoadded {inputDir inputNameRoot outputDir outputPSname \
	{title ""} {dev /XWINDOW} {plotList ""}} {

      set camCols "1 4 2 5 3 6"
      set filterlist "s w x y"
      if {$plotList == ""} {set plotList $filterlist}
      set ic(s) 3
      set ic(w) 2
      set ic(x) 4
      set ic(y) 12

      set dmMin -15; set dmMax 15 

      ### setup the plot
      set pg [pgstateNew]
      set device [devicename $dev "$outputDir/$outputPSname"] 
      pgstateSet $pg -device $device -lineWidth 3 -isLine 1 \
                -xfract 0.27 -yfract 0.28 -nxwindow 2 -nywindow 4 -symb 3
      set icLineDef [exprGet $pg.icLine]
      pgstateOpen $pg

      ### read all vectors
      foreach camCol $camCols {
         ## get data for this camCol
         foreach f $filterlist {     
            set fileRoot $inputDir/${inputNameRoot}
            set PCfile ${fileRoot}-${f}${camCol}.dat
            if {![file exist $PCfile]} {
               echo "CANNOT find file"
               error $PCfile
	    } else {
               set bin($f)     [Tfile2Vector $PCfile 1 5]
               set corr($f)    [Tfile2Vector $PCfile 2 5]
               set corrErr($f) [Tfile2Vector $PCfile 3 5]
            }

         }
         ## plot this camCol   
         # loop over desired filters 
         foreach f $plotList {     
            set Yminus [vectorExprEval $corr($f)-$corrErr($f)]
            set Yplus [vectorExprEval $corr($f)+$corrErr($f)]
            pgstateSet $pg -icLine $ic($f) -lineWidth 1
            vPlot $pg $bin($f) $Yminus -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
            pgstateSet $pg -isNewplot 0
            vPlot $pg $bin($f) $Yplus -xmin 100 -xmax 1950 -ymin $dmMin -ymax $dmMax
            vListDel "$Yplus $Yminus"
            pgstateSet $pg -isNewplot 0
         }
         # to guide the eye...
         pgstateSet $pg -isNewplot 0 -isLine 1 -lineWidth 1 -icLine 15
         plot_line $pg -1000 0 10000 0 1 4 
         plot_line $pg -1000 -10 10000 -10 1 4 
         plot_line $pg -1000 10 10000 10 1 4 
         plot_line $pg 0 -1000 0 1000 1 2
         plot_line $pg 2048 -1000 2048 1000 1 2
         xlabel "chip column (pixel)"
         ylabel "dm (milimag)" 
         pgstateSet $pg -isNewplot 1

         # which camCol 
         pgText 400 -14 "camCol $camCol"

         ## clean
         foreach f $filterlist {     
            vListDel "$bin($f) $corr($f) $corrErr($f)"
         }   
         if {$camCol == [lindex $camCols 0]} {titlePlot "$title" 20} 
         if {$camCol == [lindex $camCols 4]} {
            # info
            plot_info "The constraints from the principal colors:" 0 -1.0 -0.6
            plot_info "green: s, red: w, blue: x, magenta: y" 0 -1.0 -0.8
            plot_info "The two lines per color are +-1sigma envelope" 0 -1.0 -1.0     
         }
      }

      ## end plot
      endplot $pg

}





# given a list of par files, read them and return a concatenated chain
proc parList2Chain {list} {

    set chain [param2Chain [lindex $list 0] ""]

    if {[llength $list] > 1} {
       loop i 1 [llength $list] {
          set aux [param2Chain [lindex $list $i] ""]
          chainJoin $chain $aux
       }
    }
  return $chain
}



# given a Yanny's file with m25-mPT differences, do proper statistics and 
# produce binned (in chip column) vectors of differences and their errors
proc processPTchain {PTchain outFile band {binSize 32} {rrList ""}} {
        
        # number of chip column bins
        set Nbins [expr int(2048 / $binSize)]
        # for getting sigma double the bin width
        set binSize2 [expr 2*$binSize]
        set Nbins2 [expr int(2048 / $binSize2)]

        # for binned statistics
        set magStep 0.5
        set magMin(u) 15.0
        set magMin(g) 14.0
        set magMin(r) 14.0
        set magMin(i) 14.0
        set magMin(z) 14.0
        set magMax(u) 19.0
        set magMax(g) 19.0
        set magMax(r) 19.0
        set magMax(i) 19.0
        set magMax(z) 17.0

        # for sigma clipping when computing mean
        set NsigmaClip 3.0
        # for PT hack
        set dm25mPTmax 0.25

        ## extract needed vectors 
        set reclist "colc rowc field mag25 mag25mpt mag25err magpterr color25"
        set veclist [vectorsFromChain $PTchain $reclist] 
        foreach rec $reclist {
           set irec [lsearch $reclist $rec]         
           set $rec [lindex $veclist $irec] 
        }

        ### 1) get sigma for mag25mpt as a function of magnitude
        set NmagBins [expr int(($magMax($band)-$magMin($band))/$magStep)]
        set magBinC [vectorExprNew $NmagBins]
        set dmSigma [vectorExprNew $NmagBins]
        set dmSigmaErr [vectorExprNew $NmagBins]
        ### loop over magnitude bins
        loop j 0 $NmagBins {
	   set mL [expr $magMin($band) + $j * $magStep]
           set mR [expr $magMin($band) + ($j+1) * $magStep]
           set mC [format "%5.2f" [expr $magMin($band) + ($j+0.5) * $magStep]]
           handleSet $magBinC.vec<$j> $mC
           ## mask for magnitude bin
           set magmask [vectorExprEval "$mag25 >= $mL && $mag25 < $mR ? 1 : 0"]
           ## now bin data by chip column and get sigma/sigmaErr for each bin, and then average sigma
           set sigma     [vectorExprNew $Nbins2]
           set sigmaErr  [vectorExprNew $Nbins2]
           set sigmaMask [vectorExprNew $Nbins2]
           # loop over chip column for this magnitude bin
           loop i 0 $Nbins2 {
	      set binL [expr $i * $binSize2]
              set binR [expr ($i+1) * $binSize2]
              # mask for binning
              set binmask [vectorExprEval "$colc >= $binL && $colc < $binR ? $magmask : 0"]
              set N [vectorExprGet sum($binmask)]
	      if {$N > 3} {
                  set sig [expr 1.0*[vSigma $mag25mpt -vMask $binmask]]
                  set sigErr [expr $sig/sqrt($N)]
                  handleSet $sigmaMask.vec<$i> 1
              } else {
                  if {$mC < 16} {
                     set sig 0.03
                  } else {
                     set sig 0.1
                  }
                  set sigErr 0.05
                  handleSet $sigmaMask.vec<$i> 0
              }
              handleSet $sigma.vec<$i> $sig
              handleSet $sigmaErr.vec<$i> $sigErr
              vListDel $binmask
           }  
           # get mean, properly weighted sigma
           set Ns [vectorExprGet sum($sigmaMask)]
	   if {$Ns > 2} {
               handleSet $dmSigma.vec<$j>    [vMedian $sigma -vMask $sigmaMask]
               handleSet $dmSigmaErr.vec<$j> [vSigma $sigma -vMask $sigmaMask]
           } else {
               if {$mC < 16} {
                   handleSet $dmSigma.vec<$j> 0.03
               } else {
                   handleSet $dmSigma.vec<$j> 0.1
               }
               handleSet $dmSigmaErr.vec<$j> 0.05
               # echo "CANNOT compute mag25mpt sigma (mC=$mC, Ns=$Ns)"
           }
           vListDel "$magmask $sigma $sigmaErr $sigmaMask"
        } 

        ### 2) now we have sigma(m), where m=magBinC and sigma=dmSigma
        ###  fit a third order polynomial to sigma(m) and evaluate for each mag25,
        ###  also, evaluate for m=magBinC and dump for sanity checks
        set Clist [vFitPolynom $magBinC $dmSigma 3]
        set m25mPTsigma [vEvalPolynom $mag25 $Clist]
        set sigmaFit [vEvalPolynom $magBinC $Clist]
        set outf [open ${outFile}.sanity w]
        puts $outf "# 3rd poly fit to sigma(m)"
        close $outf
        vectorsWriteToFile "$magBinC $dmSigma $sigmaFit" ${outFile}.sanity "" "" a
        vListDel "$magBinC $dmSigma $dmSigmaErr $sigmaFit"

        ### 3) for each bin of chip column determine weighted mean and its error
        ###    n.b. same as above, except there is no magnitude binning
        set binC [vectorExprNew $Nbins]
        set dmN  [vectorExprNew $Nbins]
        set dmMean  [vectorExprNew $Nbins]
        set dmMeanErr [vectorExprNew $Nbins]
        set dmMeanChi2pdf [vectorExprNew $Nbins]
        set dmMeanMask [vectorExprNew $Nbins]
        ### mask in this filter
        ### hack for gross outliers in m25-mPT
        set PTmask [vectorExprEval "$mag25mpt >= -$dm25mPTmax && $mag25mpt <= $dm25mPTmax ? 1 : 0"]
        set magmask [vectorExprEval "$mag25 >= $magMin($band) && $mag25 < $magMax($band) ? $PTmask : 0"]
        vectorExprDel $PTmask 
        # loop over all chip column bins
        loop i 0 $Nbins {
	   set binL [expr $i * $binSize]
           set binR [expr ($i+1) * $binSize]
           handleSet $binC.vec<$i> [expr ($i+0.5) * $binSize]
           # mask for binning
           set binmask [vectorExprEval "$colc >= $binL && $colc < $binR ? $magmask : 0"]
           set N [vectorExprGet sum($binmask)]
	   if {$N > 3} {
               # first get median and sigma for this subsample
               set SSmedian [vMedian $mag25mpt -vMask $binmask]
               set SSsigma  [vSigma  $mag25mpt -vMask $binmask]
               # and now add condition +-NsigmaClip sigma from the median
               set dmMinClip [expr $SSmedian - $NsigmaClip * $SSsigma]
               set dmMaxClip [expr $SSmedian + $NsigmaClip * $SSsigma]
               set mask [vectorExprEval "$mag25mpt > $dmMinClip && $mag25mpt < $dmMaxClip ? $binmask : 0"]
               # n.b. points not to be included (wrong bin or sigma clipped) excluded through mask
               set weight  [vectorExprEval $mask/$m25mPTsigma/$m25mPTsigma]
               set weightX [vectorExprEval $weight*$mag25mpt]
               ### and now we get mean offset, its error and chi2pdf
               set offset [vectorExprGet sum($weightX)/sum($weight)]
               set offsetErr [vectorExprGet 1.0/sqrt(sum($weight))]
               set chi2Vec [vectorExprEval $weight*($mag25mpt-$offset)*($mag25mpt-$offset)]
               set chi2pdf [expr sqrt([vectorExprGet sum($chi2Vec)]/($N-1))]
               handleSet $dmMeanMask.vec<$i> 1
               vListDel "$mask $weight $weightX $chi2Vec"
           } else {
              set offset 0.0
              set offsetErr 0.2
              set chi2pdf 0.0
              handleSet $dmMeanMask.vec<$i> 0
           } 
           handleSet $dmN.vec<$i> $N
           handleSet $dmMean.vec<$i> $offset
           handleSet $dmMeanErr.vec<$i> $offsetErr
           handleSet $dmMeanChi2pdf.vec<$i> $chi2pdf
           vListDel $binmask         
        }
        vListDel "$magmask $m25mPTsigma"
        vListDel $veclist 

        ### now we have binC dmMean and dmMeanErr 
        ### first get some sanity check and dump to sanity out file 
        set outf [open ${outFile}.sanity a]
        puts $outf "# sanity check on bin-by-bin statistics (in mags)"
        set dmMeanMedian [format "%4.0f" [vMedian $dmMean]]
        set dmMeanSigma [format "%6.3f" [vSigma $dmMean]]
        set dmNMedian [format "%6.3f" [vMedian $dmN]]
        set dmMeanChi2pdfMedian [format "%6.3f" [vMedian $dmMeanChi2pdf]]
        set dmMeanMin [format "%6.3f" [vExtreme $dmMean min]]
        set dmMeanMax [format "%6.3f" [vExtreme $dmMean max]]
        # treat all bins as a sample
        set weight [vectorExprEval 1.0/$dmMeanErr/$dmMeanErr]
        set weightX [vectorExprEval $weight*$dmMean]
        set meanOff [format "%6.3f" [vectorExprGet sum($weightX)/sum($weight)]]
        set chi2pdf [vectorExprEval $weight*($dmMean-$meanOff)*($dmMean-$meanOff)]
        set chi2pdfWeighted [format "%6.2f" [expr sqrt([vectorExprGet sum($chi2pdf)]/($Nbins-1))]]
        # 
        set dmOverErr [vectorExprEval $dmMean/(abs($dmMeanErr)+0.0001)]
        set dmOverErr2 [vectorExprEval $dmOverErr*$dmOverErr]
        set chiMedian [format "%6.2f" [vMedian $dmOverErr]]
        set chiSigma [format "%6.2f" [vSigma $dmOverErr]]
        set chi2pdfMean [format "%6.2f" [vMean $dmOverErr2]]
        vListDel "$dmOverErr $dmOverErr2 $weight $weightX $chi2pdf" 
        set dmMeanErrMedian [format "%6.3f" [vMedian $dmMeanErr]]
        puts $outf "# median offset: $dmMeanMedian, sigma offset: $dmMeanSigma, mean Weighted off: $meanOff"
        puts $outf "# min offset: $dmMeanMin, max offset: $dmMeanMax"
        puts $outf "# median error: $dmMeanErrMedian"
        puts $outf "# median N for a bin: $dmNMedian, median chi2pdf for a bin: $dmMeanChi2pdfMedian"
        puts $outf "# ALL bins WEIGHTED -> chi2pdf: $chi2pdfWeighted"
        puts $outf "# ALL bins NOT W. -> median chi: $chiMedian, sigma chi: $chiSigma, mean chi2pdf: $chi2pdfMean"
        puts $outf "# ---------------------------------------------"
        close $outf

        ### 4) we have the resulting offsets, now dump them to output file
        set outf [open $outFile w]
        puts $outf "# output from processPTchain for run/rerun list:"
        puts $outf "# $rrList"        
        puts $outf "#  produced using photo version [photoVersion]"
        puts $outf "#  by [exec whoami] on [exec date]"
        set Nrows [exprGet $binC.dimen]
        puts $outf "# binCenter (pix)  dmMean   dmMeanErr  dmMean/dmMeanErr  chi2pdf Nstar  OK"
        puts $outf "#                  -- in 1/1000 mag --"
        ## loop over all entries
        loop j 0 $Nrows {
           # bin position
           set v1 [format "%4.0f" [exprGet $binC.vec<$j>]]
           set v2 [format "%7.2f" [expr 1000*([exprGet $dmMean.vec<$j>])]] 
           set v3 [format "%7.2f" [expr 1000*([exprGet $dmMeanErr.vec<$j>])]]
           set v4 [format "%7.2f" [expr $v2/(abs($v3)+0.000001)]]
           set v5 [format "%7.2f" [exprGet $dmMeanChi2pdf.vec<$j>]]
           set v6 [format "%3.0f" [exprGet $dmN.vec<$j>]]
           set v7 [format "%3.0f" [exprGet $dmMeanMask.vec<$j>]] 
           puts $outf "       $v1      $v2      $v3     $v4      $v5   $v6   $v7"
        }
        # close file
        close $outf 
 
       vListDel "$binC $dmMean $dmMeanErr $dmMeanChi2pdf $dmN $dmMeanMask"

     return
}





proc invPCmat {K1 K2 K3 K4 K5 {print 0}} {

     set line(0) "-0.249  0.794  -0.555   0.0     0.0" 
     set line(1) " 0.0   -0.227   0.792  -0.567   0.0"
     set line(2) " 0.0    0.707  -0.707   0.0     0.0"
     set line(3) " 0.0    0.0    -0.270   0.800  -0.534" 
     set line(4) "$K1     $K2      $K3     $K4     $K5"
     set vecList ""
     set OvecList ""
     loop i 0 5 {
	 set vec($i) [vectorExprNew 5]
         loop j 0 5 {
	     handleSet $vec($i).vec<$j> [lindex $line($i) $j]
         }
         set vecList "$vecList $vec($i)"
         set Ovec($i) [vectorExprEval $vec($i)]
         set OvecList "$OvecList $Ovec($i)"
     } 
     
     vMatrixInvert $vecList

   # Print A, Ainverse, and sanity check
   if {$print} {
     echo ""
     loop i 0 5 {
        set L "|"
        loop j 0 5 {
	   set tL [format "%6.3f" [exprGet $Ovec($i).vec<$j>]]
	   set L "$L $tL" 
        }
        if {$i == 2} {
           set L "$L|  *  |"
        } else {
           set L "$L|     |"
        }
        loop j 0 5 {
	   set tL [format "%6.3f" [exprGet $vec($i).vec<$j>]]
	   set L "$L $tL" 
        }
        set L "$L|"
        # multiply
        set sum 0 
	loop k 0 5 {
	   set sum [expr $sum+[exprGet $Ovec($i).vec<$k>]*[exprGet $vec($k).vec<$i>]]
	}
	set sum [format "%6.3f" $sum]
        if {$i == 2} {
           set L "$L  =  |$sum|"
        } else {
           set L "$L     |$sum|"
        }
        echo $L  
     }
     echo ""
   }

   vListDel $OvecList 

  return $vecList 
}


proc testSolution {Alist Blist} {

    assert { [exprGet [lindex $Alist 0].dimen] == [llength $Blist] }

    set sumList ""
    foreach A $Alist {
       set N [exprGet $A.dimen]
       set sum 0
       loop j 0 $N {
          set B [lindex $Blist $j]
	  set sum [expr $sum + [exprGet $A.vec<$j>] * $B]
       }
       set sumList "$sumList $sum"
    }

  return $sumList

}



## process a file produced by doColorPixBins
proc processColorHist {infile {NrebinColor 0} {NrebinPix 0} {OffMax 0.1}} {

      set Nsigma 2.5
      set Niter 2

      ## open and read input file to get the number of columns
      set firstline 1
      set inf [open $infile r]
      while {![eof $inf] && $firstline} {
         set line [gets $inf]
         # process line
         if {![eof $inf] && [lindex $line 0] != "#"} { 
	    set Ncolumns [llength $line]
            set firstline 0
         }
      } 
      close $inf
    
      ## now we know the number of columns, read them all
      set color [Tfile2Vector $infile 1 8]
      set nTot  [Tfile2Vector $infile 2 8]
      set list "$color $nTot"
      loop i 0 [expr $Ncolumns-2] {
         set n($i) [Tfile2Vector $infile [expr 3+$i] 8]
         set list "$list $n($i)"
      }
    
      ## do we have to rebin in color space?
      loop i 0 $NrebinColor {
         set oldlist $list 
	 set list "[vecBin [lindex $oldlist 0] 2]"
	 loop k 1 [llength $oldlist] {
            set list "$list [vecBin [lindex $oldlist $k] 1]"
         }
         vListDel $oldlist 
      }

      ## do we have to rebin in pixel space?
      loop i 0 $NrebinPix {
         set oldlist $list 
	 set list [vectorExprEval [lindex $oldlist 0]]
	 set list "$list [vectorExprEval [lindex $oldlist 1]]"
	 set NbinNew [expr ([llength $oldlist]-2)/2]
	 loop k 0 $NbinNew {
            set v1 [lindex $oldlist [expr 2+2*$k]]
            set v2 [lindex $oldlist [expr 2+2*$k+1]]
	    set list "$list [vectorExprEval $v1+$v2]" 
         }
         vListDel $oldlist 
      }

      ## get now offset(bin) and offsetErr(bin) vectors
      set Nbin [expr [llength $list]-2] 
      set offset [vectorExprNew $Nbin]
      set offsetErr [vectorExprNew $Nbin]
      set mean [vectorExprNew $Nbin]
      set meanErr [vectorExprNew $Nbin]
      set NptsML [vectorExprNew $Nbin]
      set NptsC [vectorExprNew $Nbin]

      ## loop over bins 
      set color   [lindex $list 0]
      set probTot [lindex $list 1]
      # step in color 
      set colorStep [expr [exprGet $color.vec<1>]-[exprGet $color.vec<0>]] 
      # maximum offset in units of color step
      set OffMaxStep [expr int($OffMax/$colorStep)]
      loop k 0 $Nbin {
          # offset and offsetErr are evaluated using maximum likelihood
	  set off [getBayesOffset $probTot [lindex $list [expr $k+2]] $OffMaxStep]
          handleSet $offset.vec<$k> [expr $colorStep*[lindex $off 0]]
          handleSet $offsetErr.vec<$k> [expr $colorStep*[lindex $off 1]]
          # number of points for this bin used in ML 
          handleSet $NptsML.vec<$k> [vectorExprGet sum([lindex $list [expr $k+2]])] 
          # mean and meanErr are evaluated using standard (gaussian) expressions
          # with Niter clippings on Nsigma (both specified above) from the mean
          set stats [gaussCounts $color [lindex $list [expr $k+2]] $Nsigma $Niter]
          handleSet $mean.vec<$k> [lindex $stats 0]
	  handleSet $meanErr.vec<$k> [lindex $stats 1]
          handleSet $NptsC.vec<$k> [lindex $stats 2]
      }

    return "$offset $offsetErr $NptsML $mean $meanErr $NptsC"

}


## given a vector of counts C, and a vector of positions x, 
## determine mean x and its error by iterative clipping 
## at Nsigma*sigma, where sigma is the distribution width 
## (aka single measurement error)
## return "xmean xerr Nclipped"
proc gaussCounts {x C {Nsigma 3} {Niter 1} {verbose 0}} {
 
       ## first pass
       set Ntot [vectorExprGet sum($C)]
       if {$Ntot < 1} {
           return "0.0 999.99 $Ntot"
       }
       set xN [vectorExprEval $x*$C]
       set SxN [vectorExprGet sum($xN)]  
       set Amean [expr 1.0*$SxN/$Ntot]
       set x2N [vectorExprEval ($x-$Amean)*($x-$Amean)*$C]
       set Sx2N [vectorExprGet sum($x2N)]  
       set Asig [expr sqrt(1.0*$Sx2N/$Ntot)]
       set Aerr [expr 1.0*$Asig/sqrt($Ntot)]
       vListDel "$xN $x2N"
          
       # and now iteratively clip 
       loop k 0 $Niter {
          # clipping condition
          set cond "$x > [expr $Amean-$Nsigma*$Asig] && $x < [expr $Amean+$Nsigma*$Asig]"
          set clippedData [vectorExprEval "$cond ? $C : 0"]  
          set xN [vectorExprEval $x*$clippedData]
          set Ntot [vectorExprGet sum($clippedData)] 
          if {$Ntot < 1} {
              return "$Amean $Aerr $Ntot"
          }
          set SxN [vectorExprGet sum($xN)]  
          set Amean [expr 1.0*$SxN/$Ntot]
          set x2N [vectorExprEval ($x-$Amean)*($x-$Amean)*$clippedData]
          set Sx2N [vectorExprGet sum($x2N)]  
          set Asig [expr sqrt(1.0*$Sx2N/$Ntot)]
          vListDel "$xN $x2N"
	  if {$verbose} {
               set Aerr [expr 1.0*$Asig/sqrt($Ntot)] 
               echo "  iteration $k: mean=$Amean err=$Aerr sig=$Asig N=$Ntot"
          }
       }
  
     set Aerr [expr 1.0*$Asig/sqrt($Ntot)]
   return "$Amean $Aerr $Ntot"

}

# given a vector, return a vector of half its size defined as 
# - sum of the two adjacent elements for type = 1
# - mean value of the two adjacent elements for type = 2
proc vecBin {vec type} {

    set out [vectorExprNew [expr [exprGet $vec.dimen]/2]]
    loop i 0 [exprGet $out.dimen] {
        set v1 [exprGet $vec.vec<[expr 2*$i]>]
        set v2 [exprGet $vec.vec<[expr 2*$i+1]>]
	if {$type == 1} {
	    handleSet $out.vec<$i> [expr $v1+$v2]
        } elseif {$type == 2} {
	    handleSet $out.vec<$i> [expr 0.5*($v1+$v2)]
        } else {
	   error "proc vecBin doesn't support type=$type"
        }          
    }

  return $out
}



## given a vector describing the "true" probability distribution
## for some variable, e.g. colors (already normalized such that
## sum(p)=1, and counts defined for the same, but possibly offset, 
## variable, determine this offset by maximizing likelihood based
## on counting Poisson statistics.
## assume that the offset is limited to +-offsetMax
## return offset and its error
proc getBayesOffset {prob counts offsetMax} {

       # sanity check
       assert { [exprGet $prob.dimen] == [exprGet $counts.dimen] }

       # the number of elements
       set Nbin [exprGet $prob.dimen]
       # the number of trial offsets
       set Noff [expr 2*$offsetMax+1]
       # vector of offsets
       set offset [vectorExprNew $Noff]
       # vector to store likelihoods
       set LL [vectorExprNew $Noff]

       ## loop over all offsets
       loop i 0 $Noff {
           handleSet $offset.vec<$i> [expr $i - $offsetMax]
           ## loop over all bins to get total likelihood
           set L 0 
           loop k 0 $Nbin {
	       set Nk [exprGet $counts.vec<$k>]
               # expected counts
               set j [expr $k+[exprGet $offset.vec<$i>]]  
               set pj -1
               if {$j >= 0 && $j < $Nbin} {
                   # log == ln!
                   set pj [exprGet $prob.vec<$j>]
                   if {$pj > 0} {
                       set pkD [expr log($pj)]
                   } else {
                       set pkD 0
                   }
               } else { 
                   set pkD 0
               }
               # see Sivia (1996), section 3.1
               set L [expr $L + $Nk*$pkD]
           }
           handleSet $LL.vec<$i> $L
       }

       ## given likelihood as a function of offset, get
       ## the best value of offset and its error by fitting a
       ## second order parabola
       set Clist [vFitPolynom $offset $LL 2]   
       # y = a + b*x + c*x2
       set b [lindex $Clist 1]
       set c [lindex $Clist 2]
       if {$c >= 0} {
           echo "proc getBayesOffset: c>=0 !?! (offset=$offset, L=$LL, b=$b)"
           set bestOffset 0
           set errOffset -999.99
        } else {
           set bestOffset [expr 0.5*$b/$c]  
           set errOffset [expr 1.0/sqrt(-2*$c)]  
       }
       vListDel "$offset $LL"
   
    return "$bestOffset $errOffset"

} 
 



# given files with the principal color statistics, 
# and an additional constraint (e.g. PT-based t magnitudes),
# get flatfield corrections for each chip
proc doPC2FFcorrection {run rerun {outputDir .} {camCols "all"} {doPT 1} \
	     {root ColorCorrPsf} {outRoot FFcorrPC}}  {

        ### definition of the PT white magnitude
        ### N.B. these coefficients can be arbitrarily changed here 
        set K1 0
        set K2 0.333
        set K3 0.333
        set K4 0.333
        set K5 0

        # local dir?
        if {$outputDir == "."} {
            set outputDir [pwd]
        }
 
        # do we want all camCols?
        if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}

        # color/band names
        set PCcolors "s w x y"
        set filterlist "u g r i z"

        set runstr [format %06d $run]
          
        ### given K1...K5, get the solution matrix 
        # n.b. Alist is a list of 5 vectors (rows in matrix A^{-1}, each with 5 elements)
        set Alist [invPCmat $K1 $K2 $K3 $K4 $K5]

        ### loop over all requested camera columns
        foreach camCol $camCols {
           echo " === doPC2FFcorrection: run $run ($rerun), camCol $camCol"
           # and all samples 
           foreach PCcolor $PCcolors {
	      set fname $outputDir/${root}_${PCcolor}_run${run}_${rerun}_c${camCol}.dat
              # read this color's data: clipped mean and its error
              set bin($PCcolor) [Tfile2Vector $fname 1 12]
              set mean($PCcolor) [Tfile2Vector $fname 6 12]
              set meanErr($PCcolor) [Tfile2Vector $fname 7 12]
              # make sure all bin vectors are the same
	      if {$PCcolor != [lindex $PCcolors 0]} {
                  assert { [exprGet $bin($PCcolor).dimen] == [exprGet $bin([lindex $PCcolors 0]).dimen] }
              }
	   }
           # we have all 4 vectors with principal colors
           # now get the PT constraint
	   if {$doPT} {
              ### get PT constraint: bin vector and synthetic magnitude correction with associated error
              set fileRoot "$outputDir/BinnedPTdm-${runstr}-"
              set PTconstraint [getPTconstraint $fileRoot $camCol $K1 $K2 $K3 $K4 $K5]
              set binPT    [lindex $PTconstraint 0]
              set K6vec    [lindex $PTconstraint 1]
              set K6errvec [lindex $PTconstraint 2]
              # N.B. K6 and K6err are in milimags (as are all other corrections)
              # sanity check 
	      assert { [exprGet $binPT.dimen] == [exprGet $bin([lindex $PCcolors 0]).dimen] }
              vListDel $binPT
           } else {
              ### assume no error in t magnitudes (and its error of 10 mmags)
              set K6 0
              set K6err 10.0
              # make vector of K6 of the same length as bin($PCcolor) 
              set K6vec [vectorExprEval $K6+0*$bin([lindex $PCcolors 0])]
              set K6errvec [vectorExprEval $K6err+0*$bin([lindex $PCcolors 0])]
           }
          
           ### loop over all 5 SDSS bands 
           loop i 0 5 {
	      set band [lindex $filterlist $i]   
              # make correction and correction error vectors 
	      set corr($band) [vectorExprNew [exprGet $bin([lindex $PCcolors 0]).dimen]]
	      set corrErr($band) [vectorExprNew [exprGet $bin([lindex $PCcolors 0]).dimen]]
              # this row of matric coefficients (corresponding to this band)
              set coeffs [lindex $Alist $i]
              set aux [exprGet $coeffs.vec<4>]
              ## loop over all pixel bins
	      loop j 0 [exprGet $bin([lindex $PCcolors 0]).dimen] {
                  # start with the term that couples PT constraint
		  handleSet $corr($band).vec<$j> [expr -0.001*[exprGet $K6vec.vec<$j>]*$aux]
                  set corrVar [expr pow(0.001*[exprGet $K6errvec.vec<$j>]*$aux,2)]
                  # and now loop over all 4 PCs
                  foreach PCcolor $PCcolors {
       	             set k [lsearch $PCcolors $PCcolor]
                     # NB: corrections are given in milimags
		     set Smean [expr 0.001*[exprGet $mean($PCcolor).vec<$j>]]
		     set SmeanErr [expr 0.001*[exprGet $meanErr($PCcolor).vec<$j>]]
       	             set thisCorr [expr [exprGet $coeffs.vec<$k>]*$Smean]
		     handleSet $corr($band).vec<$j> [expr $thisCorr+[exprGet $corr($band).vec<$j>]]
       	             set thisCorrVar [expr pow([exprGet $coeffs.vec<$k>]*$SmeanErr,2)]
		     set corrVar [expr $thisCorrVar+$corrVar]
                   }
                  ### get error for this band
                  handleSet $corrErr($band).vec<$j> [expr sqrt($corrVar)]
              }
              # all set, dump bin, correction and correction error vectors for this band
	      set outname $outputDir/${outRoot}_run${run}_${rerun}_${band}${camCol}.dat
	      set Cname $outputDir/${root}"_?_run"${run}_${rerun}_c${camCol}.dat
              doFFCorrFileHeader $outname $fname $run $rerun $camCol $band
              # dump pixBins, color correction and its error to file
              set f [open $outname a]
              set pixBins $bin([lindex $PCcolors 0])
              set Nrows [exprGet $pixBins.dimen]
              puts $f "# binCenter (pix)  FFcorr     FFcorrErr  corr/corrErr"
              puts $f "#                   -- in 1/1000 mag --"
              ## loop over all entries
              loop j 0 $Nrows {
                 # bin position
		 set v1 [format "%4.0f" [exprGet $pixBins.vec<$j>]]
                 # correction
		 set v2 [format "%7.2f" [expr 1000*([exprGet $corr($band).vec<$j>])]]
		 set v3 [format "%7.2f" [expr 1000*([exprGet $corrErr($band).vec<$j>])]]
                 set v4 [format "%7.2f" [expr $v2/(abs($v3)+0.000001)]]
                 puts $f "       $v1      $v2      $v3     $v4"
              }
              # close file
              close $f 
              # clean
              vListDel "$corr($band) $corrErr($band)"
          }
          # clean
          vListDel "$K6vec $K6errvec"
          foreach PCcolor $PCcolors {
             vListDel "$bin($PCcolor) $mean($PCcolor) $meanErr($PCcolor)"
          }
        }
        # clean
        vListDel $Alist

}




# aux proc to write header to ColorCorr* files
proc doFFCorrFileHeader {filename datafile run rerun camCol band} {

       # open file
       set f [open $filename w]
       set date [exec date]
       set user [exec whoami]
       puts $f "#  Color Correction vs. Chip Column Position determined from data files"
       puts $f "#  $datafile"
       puts $f "#  For run $run rerun $rerun camCol $camCol band $band"
       puts $f "#  produced using photo version [photoVersion]"
       puts $f "#  by $user on $date"
       # close file
       close $f
}


# given BinnedPTdm*dat files, determine synthetic magnitude correction and its error,
# where the synthetic magnitude is defined as t = K1*u+K2*g+K3*r+K4*i+K5*z, as 
# a function of chip column 
proc getPTconstraint {fileRoot camCol K1 K2 K3 K4 K5 {verbose 0}} {

    # first read all 5 data files 
    if {$verbose} {
       echo "----------------------------------"
       echo "f   Mean  Med   Rms  MedErr SigRat"
    }
    foreach f {u g r i z} {
	set fname ${fileRoot}${f}${camCol}.dat
        if {![file exist $fname]} {
           echo "file $fname"
           error "DOESN'T EXIST !?!"
        }
        # read data for this color
        set bin($f) [Tfile2Vector $fname 1 7]
        set c($f) [Tfile2Vector $fname 2 7]
        set cE($f) [Tfile2Vector $fname 3 7]
        # sanity check
        if {$f != "u"} {
	   assert { [exprGet $bin($f).dimen] == [exprGet $bin(u).dimen] }
        }   
        if {$verbose} {
            set rat [vectorExprEval $c($f)/$cE($f)]
            set cMedian   [format "%5.2f" [vMedian $c($f)]]
            set cMean   [format "%5.2f" [vMean $c($f)]]
            set cSigma  [format "%5.2f" [vSigma $c($f)]]
            set cEMedian  [format "%5.2f" [vMedian $cE($f)]]
            set ratSigma  [format "%5.2f" [vSigma $rat]]
            echo "$f: $cMedian $cMean $cSigma  $cEMedian $ratSigma"
            vectorExprDel $rat
        }
    }

    # now make the requested linear combination
    set binPT [vectorExprEval $bin(u)]
    set K6 [vectorExprEval $K1*$c(u)+$K2*$c(g)+$K3*$c(r)+$K4*$c(i)+$K5*$c(z)]
    set aux1 [vectorExprEval $K1*$cE(u)*$K1*$cE(u)+$K2*$cE(g)*$K2*$cE(g)+$K3*$cE(r)*$K3*$cE(r)]
    set aux2 [vectorExprEval $K4*$cE(i)*$K4*$cE(i)+$K5*$cE(z)*$K5*$cE(z)]
    set K6err [vectorExprEval sqrt($aux1+$aux2)]
    if {$verbose} {
        echo "----------------------------------"
        set rat [vectorExprEval $K6/$K6err]
        set cMedian   [format "%5.2f" [vMedian $K6]]
        set cMean   [format "%5.2f" [vMean $K6]]
        set cSigma  [format "%5.2f" [vSigma $K6]]
        set cEMedian  [format "%5.2f" [vMedian $K6err]]
        set ratSigma  [format "%5.2f" [vSigma $rat]]
        echo "t: $cMedian $cMean $cSigma  $cEMedian $ratSigma"
        vectorExprDel $rat
        echo "----------------------------------"
    }

    # clean
    vListDel "$aux1 $aux2"
    foreach f {u g r i z} {
       vListDel "$bin($f) $c($f) $cE($f)"
    }  

  # N.B. K6 and K6err are in milimags
  return "$binPT $K6 $K6err"

}



### for specified run/rerun, camCols and field range, 
### process data files with principal color histograms binned by chip column
proc processPrincipalColors {run rerun {outputDir "default"} {camCols "all"} {samples "all"} {fMin 0} \
	            {fMax 0} {doModMags 0} {NrebinColor 2} {NrebinPix 0} {OffMax 0.1} {dataRoot ""}} {

global data_root


         if {$dataRoot == ""} {
             set data_root [get_data_root ""]
         } else {
             set data_root $dataRoot
         }

         if {![file exist $data_root]} {
             error "CANNOT FIND data_root=$data_root"
         }

         if {$outputDir == "default"} {
             set outputDir "$data_root/$run/$rerun/nfcalib/ptflatmatch"
         }

     # do we want coarser color and pixel bins? 
     # -> NrebinColor 
     # -> NrebinPix
     # OffMax: maximum plausible correction

     # do we want all camCols?
     if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}
     if {$samples == "all"} {set samples "0 1 2 3"}
     # sample names
     set sampleName(0) "s"
     set sampleName(1) "w"
     set sampleName(2) "x"
     set sampleName(3) "y"


        ### loop over all requested camera columns
        foreach camCol $camCols {
           # and all samples 
           foreach Tsample $samples {
              echo "   - working on camCol $camCol, sample $Tsample -" 
              set sName $sampleName($Tsample)
              set filename $outputDir/PixPsf_${sName}_run${run}_${rerun}_c${camCol}_${sName}.dat
              ### this is the statistics work horse #####
                set offset [processColorHist $filename $NrebinColor $NrebinPix $OffMax] 
              ###########################################
              set pixBinsAux [vIndex [exprGet [lindex $offset 0].dimen]]     
	      set pixStep [expr 2048/[exprGet $pixBinsAux.dimen]]
              set pixBins [vectorExprEval $pixStep*$pixBinsAux+$pixStep/2]  
              # output file name and header
              set outname $outputDir/ColorCorrPsf_${sName}_run${run}_${rerun}_c${camCol}.dat
              doColorCorrFileHeader $filename $outname $run $rerun $camCol $fMin $fMax $sName $sName psf
              # dump pixBins, color correction and its error to file
              set f [open $outname a]
              set Nrows [exprGet $pixBins.dimen]
              # correct by the mean correction
	      set MeanCorr [vMean [lindex $offset 0]]
	      set MeanMean [vMean [lindex $offset 3]]
	      ## get the width of chi=signal/err distribution
              set chiML [vectorExprEval ([lindex $offset 0]-$MeanCorr)/(abs([lindex $offset 1])+1.0e-6)]
	      set ChiSigML [format "%5.2f" [vSigma $chiML]]
              vectorExprDel $chiML
              set Nmean [lindex $offset 5]
              set chiMean [vectorExprEval abs(([lindex $offset 3]-$MeanMean)/abs([lindex $offset 4])+1.0e-6))]
	      set ChiSigMean [format "%5.2f" [vSigma $chiMean]]
              vectorExprDel $chiMean
              # add this line to output file 
              puts $f "# "
              puts $f "#     MeanML = $MeanCorr, ChiSigML = $ChiSigML, MeanMean = $MeanMean  ChiSigMean = $ChiSigMean"    
              puts $f "# "
              echo "      => MeanML = $MeanCorr, ChiSigML = $ChiSigML, MeanMean = $MeanMean  ChiSigMean = $ChiSigMean"
              ## loop over all entries
              puts $f "# binCenter (pix)  MLcorr     MLcorrErr      MLchi       MLN         mean       meanErr       chi      N"
              puts $f "#                   -- in 1/1000 mag --                              -- in 1/1000 mag --"
              loop j 0 $Nrows {
                  # bin position (in pixels)
		  set v1 [format "%4.0f" [exprGet $pixBins.vec<$j>]]
                  # *1000 for mag -> mmag
                  # ML entries
		  set v2 [format "%7.2f" [expr 1000*([exprGet [lindex $offset 0].vec<$j>]-$MeanCorr)]]
		  set v3 [format "%7.2f" [expr 1000*[exprGet [lindex $offset 1].vec<$j>]]]
		  set v4 [format "%7.2f" [expr $v2/[expr abs($v3)+1.0e-6]]]
		  set v5 [format "%4.0f" [exprGet [lindex $offset 2].vec<$j>]]
                  # clipped mean entries
		  set v6 [format "%7.2f" [expr 1000*([exprGet [lindex $offset 3].vec<$j>]-$MeanMean)]]
	          set v7 [format "%7.2f" [expr 1000*[exprGet [lindex $offset 4].vec<$j>]]]
		  set v8 [format "%7.2f" [expr $v6/(abs($v7)+1.0e-6)]]
	          set v9 [format "%4.0f" [exprGet [lindex $offset 5].vec<$j>]]
                  puts $f "       $v1      $v2      $v3      $v4      $v5      $v6      $v7    $v8    $v9"
              }
              # close file
              close $f 
              echo "       ... produced $outname"
              # clean
              vListDel "$pixBinsAux $pixBins"
              vListDel $offset
	      if {$doModMags} {
                 set filename $outputDir/PixMod_${sName}_run${run}_${rerun}_c${camCol}_${sName}.dat
                 set offset [processColorHist $filename $NrebinColor $NrebinPix $OffMax] 
                 set pixBinsAux [vIndex [exprGet [lindex $offset 0].dimen]]      
                 set pixBins [vectorExprEval 2048/[exprGet $pixBinsAux.dimen]*$pixBinsAux]  
                 # output file name and header
                 set outname $outputDir/ColorCorrMod_${sName}_run${run}_${rerun}_c${camCol}.dat
                 doColorCorrFileHeader $filename $outname $run $rerun $camCol $fMin $fMax $sName $sColor mod
                 # dump pixBins, color correction and its error to file
                 set f [open $outname a]
                 set Nrows [exprGet $pixBins.dimen]
                 # correct by the mean correction
	         set MeanCorr [vMean [lindex $offset 0]]
	         set MeanMean [vMean [lindex $offset 3]]
	         ## get the width of chi=signal/err distribution
                 set chiML [vectorExprEval ([lindex $offset 0]-$MeanCorr)/(abs([lindex $offset 1])+1.0e-6)]
	         set ChiSigML [format "%5.2f" [vSigma $chiML]]
                 vectorExprDel $chiML
                 set Nmean [lindex $offset 5]
                 set chiMean [vectorExprEval abs(([lindex $offset 3]-$MeanMean)/(abs([lindex $offset 4]/(sqrt($Nmean)))+1.0e-6))]
	         set ChiSigMean [format "%5.2f" [vSigma $chiMean]]
                 vectorExprDel $chiMean
                 # add this line to output file 
                 puts $f "# "
                 puts $f "#     MeanML = $MeanCorr, ChiSigML = $ChiSigML, MeanMean = $MeanMean  ChiSigMean = $ChiSigMean"    
                 puts $f "# "
                 echo "   Model => MeanML = $MeanCorr, ChiSigML = $ChiSigML, MeanMean = $MeanMean  ChiSigMean = $ChiSigMean"
                 ## loop over all entries
                 puts $f "# binCenter (pix)  MLcorr     MLcorrErr      MLchi       MLN         mean       meanErr       chi      N"
                 puts $f "#                   -- in 1/1000 mag --                              -- in 1/1000 mag --"
                 loop j 0 $Nrows {
                    # bin position
		    set v1 [format "%4.0f" [exprGet $pixBins.vec<$j>]]
                    # ML entries
		    set v2 [format "%7.2f" [expr 1000*([exprGet [lindex $offset 0].vec<$j>]-$MeanCorr)]]
		    set v3 [format "%7.2f" [expr 1000*[exprGet [lindex $offset 1].vec<$j>]]]
		    set v4 [format "%7.2f" [expr $v2/[expr abs($v3)+1.0e-6]]]
		    set v5 [format "%4.0f" [exprGet [lindex $offset 2].vec<$j>]]
                    # clipped mean entries
		    set v6 [format "%7.2f" [expr 1000*([exprGet [lindex $offset 3].vec<$j>]-$MeanMean]]]
	            set v9 [format "%4.0f" [expr 1000*[exprGet [lindex $offset 5].vec<$j>]]]
	            set v7 [format "%7.2f" [exprGet [lindex $offset 4].vec<$j>]]
		    set v8 [format "%7.2f" [expr $v6/(abs($v7)+1.0e-6)]]
                    puts $f "       $v1      $v2      $v3      $v4      $v5      $v6      $v7    $v8    $v9"
                 }
                 # close file
                 close $f 
                 # clean
                 vListDel "$pixBinsAux $pixBins"
                 vListDel $offset
	      }
           }
        }
 

}

### for specified run/rerun, camCols and field range, 
### dump data files with principal color histograms binned by chip column
proc makePrincipalColors {run rerun {outputDir "default"} {camCols "all"} {fieldStart 0} {fieldEnd -1} \
			      {samples all} {binSize 32} {doModMags 0} {doHistFiles 1}} {

global data_root openit

     # local dir?
     if {$outputDir == "."} {
         set outputDir [pwd]
     }
     # default dir (relative to data_root/run/rerun)
     set defaultOutputDir "nfcalib/ptflatmatch"

     # do we want all camCols?
     if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}

     # do we want all color samples
     if {$samples == "all"} {set samples "0 1 2 3"}

     # sample definitions
     set sampleName(0) "s"
     set sampleCcoeffs(0) "-0.249  0.794  -0.555   0.0  0.0 0.234"
     set samplePcoeffs(0) "0.910 0.415 0 0 -1.28"
     set sampleCond(0) "19.0  -0.2   0.8"

     set sampleName(1) "w"
     set sampleCcoeffs(1) "0.0   -0.227   0.792  -0.567   0.0   0.040"
     set samplePcoeffs(1) "0 0.928 0.372 0 -0.425"
     set sampleCond(1) "20.0 -0.2   0.6"

     set sampleName(2) "x"
     set sampleCcoeffs(2) "0.0    0.707  -0.707   0.0     0.0  -0.983"
     set samplePcoeffs(2) "0 0 1.0 0 0" 
     set sampleCond(2) "19.0 0.8 1.6"

     set sampleName(3) "y"
     set sampleCcoeffs(3) "0.0    0.0    -0.270   0.800  -0.534 0.059"
     set samplePcoeffs(3) "0 0 0.895 0.447 -0.6"
     set sampleCond(3) "19.5 0.1 1.2" 

     # number of chip column bins
     set Nbins [expr 2048 / $binSize]

     # maximum allowed errors 
     set magErrMax(0) 10.0 
     set magErrMax(1) 10.0 
     set magErrMax(2) 10.0 
     set magErrMax(3) 10.0 
     set magErrMax(4) 10.0
     # principal color range
     set cMin -0.15
     set cMax 0.15

     # do not need stars fainter than this
     set rmax 21.0 
    
     ### loop over all requested camera columns
     foreach camCol $camCols {
       # this (long) part may be bypassed
       if {$doHistFiles} {
         ## read all tsObj files
         echo " ===== reading tsObj files for column $camCol ====="
         set tsObjList [tsObjListGet $run $rerun $camCol $fieldStart $fieldEnd] 
         # ask for stars brighter than r=rmax
         set chain [tsObjList2chain $tsObjList 1 6 $rmax]
         echo " -- tsObj files read, extracting vectors --"

         ## get needed vectors 
         set reclist ""
         # psf mags and errors
         set psfStartCount 0
         loop i 0 5 {
            set reclist "$reclist psfCounts<$i>"
         }
         set psfErrStartCount 5
         loop i 0 5 {
            set reclist "$reclist psfCountsErr<$i>"
         }
         # model mags and errors
         set modStartCount 10
         loop i 0 5 {
            set reclist "$reclist counts_model<$i>"
         }
         set modErrStartCount 15
         loop i 0 5 {
            set reclist "$reclist counts_modelErr<$i>"
         }
         # reddening
         set reddStartCount 20
         loop i 0 5 {
            set reclist "$reclist reddening<$i>"
         }
         # flags
         set flagsStartCount 25
         set reclist "$reclist objc_flags objc_flags2"
         # other miscaleneous
         set miscStartCount 27
         set reclist "$reclist objc_colc"
         set reclist "$reclist field"

         ## extract vectors and get rid of chain
         set veclist [vectorsFromChain $chain $reclist] 
         echo " -- data vectors extracted, cleaning chains --"
         chainDestroy $chain genericDel
               
         # make reddening corrected magnitude vectors
         set psfMags ""; set modMags "";
         set psfMagErrs ""; set modMagErrs "";
         loop i 0 5 {
             set auxP [lindex $veclist [expr $psfStartCount + $i]]
             set auxM [lindex $veclist [expr $modStartCount + $i]]
             set auxRedd [lindex $veclist [expr $reddStartCount + $i]]
	     set psf($i) [vectorExprEval $auxP-$auxRedd]
             set psfMags "$psfMags $psf($i)"
	     set psfErr($i) [lindex $veclist [expr $psfErrStartCount + $i]]
             set psfMagErrs "$psfMagErrs $psfErr($i)"
	     set mod($i) [vectorExprEval $auxM-$auxRedd]
             set modMags "$modMags $mod($i)"
	     set modErr($i) [lindex $veclist [expr $modErrStartCount + $i]]
             set modMagErrs "$modMagErrs $modErr($i)"
         }
         # chip column position (for binning)
         set colc [lindex $veclist [expr $miscStartCount + 0]]
         # for getting field range
         set field [lindex $veclist [expr $miscStartCount + 1]]
         set fieldMin [vExtreme $field min]
         set fieldMax [vExtreme $field max]

         #### all data vectors are set, now process ####
         
         ### first set quality masks
         set flags [lindex $veclist [expr $flagsStartCount + 0]]
         set flags2 [lindex $veclist [expr $flagsStartCount + 1]]
         set GoodPhotometryMask [flagsMask $flags {BINNED1} {BRIGHT SATUR BLENDED}] 
         set NotMovingMask [flagsMask $flags2 {} {DEBLENDED_AS_MOVING}] 
         set okMask [vectorExprEval $GoodPhotometryMask*$NotMovingMask]
         ## magErr mask
         # psf mags
         set cond ""
         loop i 0 5 {
            set cond "$cond $psfErr($i) < $magErrMax($i)" 
	    if {$i < 4} {set cond "$cond && "}
         }
         set goodPsfMags [vectorExprEval "$cond ? 1 : 0"]  
         set okMaskPsf [vectorExprEval $okMask*$goodPsfMags] 
         # mod mags
         set cond ""
         loop i 0 5 {
            set cond "$cond $modErr($i) < $magErrMax($i)" 
	    if {$i < 4} {set cond "$cond && "}
         }
         set goodModMags [vectorExprEval "$cond ? 1 : 0"]  
         set okMaskMod [vectorExprEval $okMask*$goodModMags] 
         # clean aux masks 
         vListDel "$GoodPhotometryMask $NotMovingMask $okMask $goodPsfMags $goodModMags"

         ### and loop over samples 
         echo " -- all set, analyzing color subsamples (samples = $samples) --" 
         foreach Tsample $samples {
            echo "   - working on sample $Tsample -" 
            set sName $sampleName($Tsample)
            set sCcoeffs $sampleCcoeffs($Tsample)
            set sPcoeffs $samplePcoeffs($Tsample)
            set sCond $sampleCond($Tsample)
            # dump color histogram 
	    if {$outputDir == "default"} {
                set dir $data_root/$run/$rerun/$defaultOutputDir
            } else {
                set dir $outputDir 
            }
	    set name $dir/PixPsf_${sName}_run${run}_${rerun}_c${camCol}_${sName}.dat
            set nColorBin [expr int(1000*($cMax-$cMin))]
            # count good stars and dump histograms
            doPixFileHeader $name $run $rerun $camCol $fieldMin $fieldMax \
                               $sName $sName psf $binSize
	    set Ns [dumpPrincipalColorHist a $name $binSize $Nbins $colc $psfMags \
			 $sCcoeffs $sPcoeffs $sCond $okMaskPsf $cMin $cMax $nColorBin]
	    echo "        from [lindex $Ns 0] selected [lindex $Ns 1] good sources"
            # do we need the same with model magnitudes?
	    if {$doModMags} {
               # using mod mags
	       set name $outputDir/PixMod_${sName}_run${run}_${rerun}_c${camCol}_${sName}.dat
               doPixFileHeader $name $run $rerun $camCol $fieldMin $fieldMax \
                                   $sName $sName mod $binSize
               dumpPrincipalColorHist a $name $binSize $Nbins $colc $modMags \
                          $sCcoeffs $sPcoeffs $sCond $okMaskMod $cMin $cMax $nColorBin
	    } 
         }         
         # clean all vectors from analysis
         vListDel "$okMaskPsf $okMaskMod"
         vListDel $psfMags
         vListDel $modMags
         # clean all vectors extracted from chain
         vListDel $veclist  
       } else {
         set fieldMin $fieldStart
         set fieldMax $fieldEnd
       }
     }

   # the processed range of fields
   return "$fieldMin $fieldMax"
}

# dump color histograms binned by vector $binner, in $binSize wide steps 
# color is defined as [lindex $mags $b1]-[lindex $mags $b2] and only
# elements with mask=1 are to be used
# for mode = a add to file specified by $filename, otherwise overwrite
proc dumpPrincipalColorHist {mode filename binSize Nbins binner mags Ccoeffs \
                             Pcoeffs Cond mask Cmin Cmax CnBin} {

     # first make color and mask for all elements 
     set color [vectorExprEval [lindex $mags 0]*0]
     loop i 0 5 {
	set aux [vectorExprEval $color+[lindex $Ccoeffs $i]*[lindex $mags $i]]
        vectorExprDel $color
        set color $aux     
     }
     set PC [vectorExprEval $color+[lindex $Ccoeffs 5]]
     vectorExprDel $color
     # and now P1 (for mask)
     set c(0) [vectorExprEval [lindex $mags 0]-[lindex $mags 1]]
     set c(1) [vectorExprEval [lindex $mags 1]-[lindex $mags 2]]
     set c(2) [vectorExprEval [lindex $mags 2]-[lindex $mags 3]]
     set c(3) [vectorExprEval [lindex $mags 3]-[lindex $mags 4]]
     set P1 [vectorExprEval [lindex $Pcoeffs 0]*$c(0)]
     loop i 1 4 {
        set aux [vectorExprEval $P1+[lindex $Pcoeffs $i]*$c($i)]
        vectorExprDel $c($i)
        vectorExprDel $P1
        set P1 $aux             
     }
     set P1aux [vectorExprEval $P1+[lindex $Pcoeffs 4]]
     vectorExprDel $P1
     set P1 $P1aux

     # set mask
     set r [lindex $mags 2]
     set rMax [lindex $Cond 0]
     set P1min [lindex $Cond 1]
     set P1max [lindex $Cond 2]
     set Smask [vectorExprEval "$r > 14 && $r < $rMax && $P1 > $P1min && $P1 < $P1max ? $mask : 0"]
     set Ninmask [vNnonzero $mask]
     set Nthismask [vNnonzero $Smask]

     # and then bin and get color histograms for each bin
     set headerLine "# color  Ntotal"
     loop i 0 $Nbins {
	set binL [expr $i * $binSize]
        set binR [expr ($i+1) * $binSize]
        set binC [format "%4.0f" [expr ($i+0.5) * $binSize]]
        set headerLine "$headerLine $binC"
        # mask for binning
        set binmask [vectorExprEval "$binner >= $binL && $binner < $binR ? $Smask : 0"]
        # get histogram     
        set hg($i) [vecToHist $PC $CnBin "$Cmin $Cmax" $binmask] 
        vListDel $binmask
     }

     ### all histograms made, now loop over bin positions and dump to file
     set f [open $filename $mode]
     puts $f "#                   ---- bin centers ---- "   
     puts $f $headerLine

     set Nrows [exprGet $hg(0).nbin]
     ## loop over all entries
     loop j 0 $Nrows {
        set Ntotal 0
	set colorBin [format "%7.3f" [exprGet $hg(0).binPosition<$j>]]
        ## loop over all bins to make the full line for this row
        set line ""
        loop i 0 $Nbins {
	   set nn [format "%4.0f" [exprGet $hg($i).contents<$j>]]
	   set Ntotal [expr $Ntotal + $nn]
           set line "$line $nn" 
        }
	set Ntotal [format "%6.0f" $Ntotal]
        puts $f "$colorBin $Ntotal $line"
     }
     # close file
     close $f 

     # clean
     loop i 0 $Nbins {
        hgDel $hg($i)
     }
     vListDel "$PC $Smask"

    return "$Ninmask $Nthismask"

}





### for specified run/rerun, camCols and field range, 
### dump data files with color histograms binned by chip column
proc doColorPixBins {run rerun {outputDir .} {camCols "all"} {fieldStart 0} {fieldEnd -1} \
			 {samples "all"} {binSize 32} {doModMags 0} {doHistFiles 1}} {

global data_root openit

     # do we want coarser color and pixel bins? 
     set NrebinColor 1 
     set NrebinPix 1
     # maximum plausible correction
     set OffMax 10

     # local dir?
     if {$outputDir == "."} {
         set outputDir [pwd]
     }
 
     # do we want all camCols?
     if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}

     # do we want all color samples
     if {$samples == "all"} {set samples "0 1 2 3"}

     # sample names, colors, and useful color ranges (for each color and sample)
     set sampleName(0) "brightRedDwarf"
     set sampleColors(0) "ug gr"
     set colorMin(0,ug) 2.2; set colorMax(0,ug) 3.0
     set colorMin(0,gr) 1.2; set colorMax(0,gr) 1.6

     set sampleName(1) "faintRedDwarf"
     set sampleColors(1) "gr iz"
     set colorMin(1,gr) 1.2; set colorMax(1,gr) 1.6
     set colorMin(1,iz) 0.3; set colorMax(1,iz) 1.0

     set sampleName(2) "blueDisk"
     set sampleColors(2) "ug gr ri iz"
     set colorMin(2,ug) 0.9; set colorMax(2,ug) 1.6
     set colorMin(2,gr) 0.1; set colorMax(2,gr) 0.8
     set colorMin(2,ri) 0.0; set colorMax(2,ri) 0.4
     set colorMin(2,iz) -0.1; set colorMax(2,iz) 0.3

     set sampleName(3) "blueHalo"  
     set sampleColors(3) "ug gr ri iz"
     set colorMin(3,ug) 0.6; set colorMax(3,ug) 1.3
     set colorMin(3,gr) 0.1; set colorMax(3,gr) 0.8
     set colorMin(3,ri) 0.0; set colorMax(3,ri) 0.4
     set colorMin(3,iz) -0.2; set colorMax(3,iz) 0.3

     set sampleName(4) "allBrightBlue"  
     set sampleColors(4) "ug gr ri"
     set colorMin(4,ug) -0.5; set colorMax(4,ug) 3.5
     set colorMin(4,gr) -0.5; set colorMax(4,gr) 1.6
     set colorMin(4,ri) -0.5; set colorMax(4,ri) 2.0

     set sampleName(5) "allBrightRed"  
     set sampleColors(5) "gr ri iz"
     set colorMin(5,gr) -0.5; set colorMax(5,gr) 1.6
     set colorMin(5,ri) -0.5; set colorMax(5,ri) 2.0
     set colorMin(5,iz) -0.5; set colorMax(5,iz) 1.5


     # bands for each color
     set band1(ug) 0; set band2(ug) 1; 
     set band1(gr) 1; set band2(gr) 2; 
     set band1(ri) 2; set band2(ri) 3; 
     set band1(iz) 3; set band2(iz) 4; 
 
     # number of chip column bins
     set Nbins [expr 2048 / $binSize]

     # maximum allowed errors 
     set magErrMax(0) 0.15 
     set magErrMax(1) 0.1 
     set magErrMax(2) 0.1 
     set magErrMax(3) 0.1 
     set magErrMax(4) 0.15

     set rmax 21


     ### loop over all requested camera columns
     foreach camCol $camCols {
       # this (long) part may be bypassed
       if {$doHistFiles} {
         ## read all tsObj files
         echo " ===== reading tsObj files for column $camCol ====="
         set tsObjList [tsObjListGet $run $rerun $camCol $fieldStart $fieldEnd] 
         set chain [tsObjList2chain $tsObjList 1 6 $rmax]
         echo " -- tsObj files read, extracting vectors --"

         ## select stars brighter than r = 21 
         set s [chainSearch $chain "{objc_type == 6} {psfCounts<2> < $rmax}"]

         ## get needed vectors 
         set reclist ""
         # psf mags and errors
         set psfStartCount 0
         loop i 0 5 {
            set reclist "$reclist psfCounts<$i>"
         }
         set psfErrStartCount 5
         loop i 0 5 {
            set reclist "$reclist psfCountsErr<$i>"
         }
         # model mags and errors
         set modStartCount 10
         loop i 0 5 {
            set reclist "$reclist counts_model<$i>"
         }
         set modErrStartCount 15
         loop i 0 5 {
            set reclist "$reclist counts_modelErr<$i>"
         }
         # reddening
         set reddStartCount 20
         loop i 0 5 {
            set reclist "$reclist reddening<$i>"
         }
         # flags
         set flagsStartCount 25
         set reclist "$reclist objc_flags objc_flags2"
         # other miscaleneous
         set miscStartCount 27
         set reclist "$reclist objc_colc"
         set reclist "$reclist field"

         ## extract vectors and get rid of chain
         set veclist [vectorsFromChain $s $reclist] 
         echo " -- data vectors extracted, cleaning chains --"
         chainDel $s 
         chainDestroy $chain genericDel
               
         # make reddening corrected magnitude vectors
         set psfMags ""; set modMags "";
         set psfMagErrs ""; set modMagErrs "";
         loop i 0 5 {
             set auxP [lindex $veclist [expr $psfStartCount + $i]]
             set auxM [lindex $veclist [expr $modStartCount + $i]]
             set auxRedd [lindex $veclist [expr $reddStartCount + $i]]
	     set psf($i) [vectorExprEval $auxP-$auxRedd]
             set psfMags "$psfMags $psf($i)"
	     set psfErr($i) [lindex $veclist [expr $psfErrStartCount + $i]]
             set psfMagErrs "$psfMagErrs $psfErr($i)"
	     set mod($i) [vectorExprEval $auxM-$auxRedd]
             set modMags "$modMags $mod($i)"
	     set modErr($i) [lindex $veclist [expr $modErrStartCount + $i]]
             set modMagErrs "$modMagErrs $modErr($i)"
         }
         # chip column position (for binning)
         set colc [lindex $veclist [expr $miscStartCount + 0]]
         # for getting field range
         set field [lindex $veclist [expr $miscStartCount + 1]]
         set fieldMin [vExtreme $field min]
         set fieldMax [vExtreme $field max]

         #### all data vectors are set, now process ####
         
         ### first set quality masks
         set flags [lindex $veclist [expr $flagsStartCount + 0]]
         set flags2 [lindex $veclist [expr $flagsStartCount + 1]]
         set GoodPhotometryMask [flagsMask $flags {BINNED1} {BRIGHT SATUR BLENDED}] 
         set NotMovingMask [flagsMask $flags2 {} {DEBLENDED_AS_MOVING}] 
         set okMask [vectorExprEval $GoodPhotometryMask*$NotMovingMask]
         ## magErr mask
         # psf mags
         set cond ""
         loop i 0 5 {
            set cond "$cond $psfErr($i) < $magErrMax($i)" 
	    if {$i < 4} {set cond "$cond && "}
         }
         set goodPsfMags [vectorExprEval "$cond ? 1 : 0"]  
         set okMaskPsf [vectorExprEval $okMask*$goodPsfMags] 
         # mod mags
         set cond ""
         loop i 0 5 {
            set cond "$cond $modErr($i) < $magErrMax($i)" 
	    if {$i < 4} {set cond "$cond && "}
         }
         set goodModMags [vectorExprEval "$cond ? 1 : 0"]  
         set okMaskMod [vectorExprEval $okMask*$goodModMags] 
         # clean aux masks 
         vListDel "$GoodPhotometryMask $NotMovingMask $okMask $goodPsfMags $goodModMags"

         ### and loop over samples 
         echo " -- all set, analyzing color subsamples --" 
         foreach Tsample $samples {
            echo "   - working on sample $Tsample -" 
            set sName $sampleName($Tsample)
            set sColors $sampleColors($Tsample)
            # set mask for this sample using psf mags
	    set SmaskPsf [getColorSampleMask $psfMags $psfMagErrs $Tsample $okMaskPsf]
	    if {$doModMags} {
   	        set SmaskMod [getColorSampleMask $modMags $modMagErrs $Tsample $okMaskMod]
            }
            # dump color histograms 
	    foreach sColor $sColors {
               # bands to make this color
               set b1 $band1($sColor)
               set b2 $band2($sColor)
	       set name $outputDir/PixPsf_${sName}_run${run}_${rerun}_c${camCol}_${sColor}.dat
               # color range
               set cMin $colorMin($Tsample,$sColor)
               set cMax $colorMax($Tsample,$sColor)
               set nColorBin [expr int(100*($cMax-$cMin))]
               # count good stars and dump histograms
               doPixFileHeader $name $run $rerun $camCol $fieldMin $fieldMax \
                               $sName $sColor psf $binSize
               dumpColorHist a $name $binSize $Nbins $colc $psfMags $b1 $b2 $SmaskPsf \
                             $cMin $cMax $nColorBin
               # do we need the same with model magnitudes?
	       if {$doModMags} {
                   # using mod mags
	           set name $outputDir/PixMod_${sName}_run${run}_${rerun}_c${camCol}_${sColor}.dat
                   doPixFileHeader $name $run $rerun $camCol $fieldMin $fieldMax \
                                   $sName $sColor mod $binSize
                   dumpColorHist a $name $binSize $Nbins $colc $modMags $b1 $b2 $SmaskMod \
                             $cMin $cMax $nColorBin
	       }   
	    } 
            # clean
            vListDel $SmaskPsf 
	    if {$doModMags} {
                vListDel $SmaskMod
            }
         }
         
         # clean all vectors from analysis
         vListDel "$okMaskPsf $okMaskMod"
         vListDel $psfMags
         vListDel $modMags
         # clean all vectors extracted from chain
         vListDel $veclist  
       } else {
         set fieldMin $fieldStart
         set fieldMax $fieldEnd
       }
     }


     ### all files produced, now process them and make files with 
     ### color correction vs. pixel
     ### loop over all requested camera columns
     foreach camCol $camCols {
        foreach Tsample $samples {
           echo "   - working on sample $Tsample -" 
           set sName $sampleName($Tsample)
           set sColors $sampleColors($Tsample)
           foreach sColor $sColors {
              set filename $outputDir/PixPsf_${sName}_run${run}_${rerun}_c${camCol}_${sColor}.dat
              set offset [processColorHist $filename $NrebinColor $NrebinPix $OffMax] 
              set pixBinsAux [vIndex [exprGet [lindex $offset 0].dimen]]     
	      set pixStep [expr 2048/[exprGet $pixBinsAux.dimen]]
              set pixBins [vectorExprEval $pixStep*$pixBinsAux+$pixStep/2]  
              # output file name and header
              set outname $outputDir/ColorCorrPsf_${sName}_run${run}_${rerun}_c${camCol}_${sColor}.dat
              doColorCorrFileHeader $filename $outname $run $rerun $camCol $fieldMin $fieldMax $sName $sColor psf
              # dump pixBins, color correction and its error to file
              set f [open $outname a]
              puts $f "# binCenter (pix)  corr     corrErr        chi"
              puts $f "#                 -- in 1/100 mag --"
              set Nrows [exprGet $pixBins.dimen]
              # correct by the mean correction
	      set MeanCorr [vMean [lindex $offset 0]]
              ## loop over all entries
              loop j 0 $Nrows {
		  set v1 [format "%4.0f" [exprGet $pixBins.vec<$j>]]
		  set v2 [format "%6.3f" [expr [exprGet [lindex $offset 0].vec<$j>]-$MeanCorr]]
		  set v3 [format "%6.3f" [exprGet [lindex $offset 1].vec<$j>]]
		  set v4 [format "%6.3f" [expr $v2/[expr abs($v3)+1.0e-6]]]
                  puts $f "       $v1      $v2      $v3      $v4"
              }
              # close file
              close $f 
              # clean
              vListDel "$pixBinsAux $pixBins"
              vListDel $offset
	      if {$doModMags} {
                 set filename $outputDir/PixMod_${sName}_run${run}_${rerun}_c${camCol}_${sColor}.dat
                 set offset [processColorHist $filename $NrebinColor $NrebinPix $OffMax] 
                 set pixBinsAux [vIndex [exprGet [lindex $offset 0].dimen]]      
                 set pixBins [vectorExprEval 2048/[exprGet $pixBinsAux.dimen]*$pixBinsAux]  
                 # output file name and header
                 set outname $outputDir/ColorCorrMod_${sName}_run${run}_${rerun}_c${camCol}_${sColor}.dat
                 doColorCorrFileHeader $filename $outname $run $rerun $camCol $fieldMin $fieldMax $sName $sColor mod
                 # dump pixBins, color correction and its error to file
                 set f [open $outname a]
                 puts $f "# binCenter (pix)  corr     corrErr        chi"
                 puts $f "#                 -- in 1/100 mag --"
                 set Nrows [exprGet $pixBins.dimen]
                 # correct by the mean correction
	         set MeanCorr [vMean [lindex $offset 0]]
                 ## loop over all entries
                 loop j 0 $Nrows {
		     set v1 [format "%4.0f" [exprGet $pixBins.vec<$j>]]
		     set v2 [format "%6.3f" [expr [exprGet [lindex $offset 0].vec<$j>]-$MeanCorr]]
		     set v3 [format "%6.3f" [exprGet [lindex $offset 1].vec<$j>]]
		     set v4 [format "%6.3f" [expr $v2/[expr abs($v3)+1.0e-6]]]
                     puts $f "       $v1      $v2      $v3      $v4"
                 }
                 # close file
                 close $f 
                 # clean
                 vListDel "$pixBinsAux $pixBins"
                 vListDel $offset
	      }
           }
        }
     }

}


# aux proc to write header to Pix* files
proc doPixFileHeader {filename run rerun camCol fieldMin fieldMax sName sColor magType binSize} {

       # open file
       set f [open $filename w]
       set date [exec date]
       set user [exec whoami]
       puts $f "#  Color Histograms Binned by Chip Column Position"
       set fldStr "[expr int($fieldMin)]-[expr int($fieldMax)]"
       puts $f "#  For run $run rerun $rerun camCol $camCol and fields $fldStr"
       puts $f "#  magType = $magType, sample = $sName, color = $sColor, bin size = $binSize,"
       puts $f "#  produced using photo version [photoVersion]"
       puts $f "#  by $user on $date"
       # close file
       close $f
}

# aux proc to write header to ColorCorr* files
proc doColorCorrFileHeader {datafile filename run rerun camCol fieldMin fieldMax sName sColor magType} {

       # open file
       set f [open $filename w]
       set date [exec date]
       set user [exec whoami]
       puts $f "#  Color Correction vs. Chip Column Position determined from data file"
       puts $f "#  $datafile"
       set fldStr "[expr int($fieldMin)]-[expr int($fieldMax)]"
       puts $f "#  For run $run rerun $rerun camCol $camCol and fields $fldStr"
       puts $f "#  magType = $magType, sample = $sName, color = $sColor"
       puts $f "#  produced using photo version [photoVersion]"
       puts $f "#  by $user on $date"
       # close file
       close $f
}


# dump color histograms binned by vector $binner, in $binSize wide steps 
# color is defined as [lindex $mags $b1]-[lindex $mags $b2] and only
# elements with mask=1 are to be used
# for mode = a add to file specified by $filename, otherwise overwrite
proc dumpColorHist {mode filename binSize Nbins binner mags b1 b2 mask Cmin Cmax CnBin} {

     # first make color for all elements 
     set color [vectorExprEval [lindex $mags $b1]-[lindex $mags $b2]]

     # and then bin and get color histograms for each bin
     set headerLine "# color  Ntotal"
     loop i 0 $Nbins {
	set binL [expr $i * $binSize]
        set binR [expr ($i+1) * $binSize]
        set binC [format "%4.0f" [expr ($i+0.5) * $binSize]]
        set headerLine "$headerLine $binC"
        # mask for binning
        set binmask [vectorExprEval "$binner >= $binL && $binner < $binR ? $mask : 0"]
        # get histogram     
        set hg($i) [vecToHist $color $CnBin "$Cmin $Cmax" $binmask] 
        vListDel $binmask
     }

     ### all histograms made, now loop over bin positions and dump to file
     set f [open $filename $mode]
     puts $f "#                   ---- bin centers ---- "   
     puts $f $headerLine

     set Nrows [exprGet $hg(0).nbin]
     ## loop over all entries
     loop j 0 $Nrows {
        set Ntotal 0
	set colorBin [format "%7.2f" [exprGet $hg(0).binPosition<$j>]]
        ## loop over all bins to make the full line for this row
        set line ""
        loop i 0 $Nbins {
	   set nn [format "%4.0f" [exprGet $hg($i).contents<$j>]]
	   set Ntotal [expr $Ntotal + $nn]
           set line "$line $nn" 
        }
	set Ntotal [format "%6.0f" $Ntotal]
        puts $f "$colorBin $Ntotal $line"
     }
     # close file
     close $f 

     # clean
     loop i 0 $Nbins {
        hgDel $hg($i)
     }
     vListDel $color
 
}





## given a run, rerun, camCol, fieldStart, fieldEnd, return the corresponding
## list of tsObj files. If fieldEnd < fieldStart, return all found tsObj files
proc tsObjListGet {run rerun camCol {fieldStart 0} {fieldEnd -1}} {
   
global data_root openit

      # make sure data dir is kosher
      if {![info exist data_root]} {
          set data_root [get_data_root ""]
      }
      if {![info exist data_root]} {
           error "Specified data_root does NOT exist: $data_root"
      }


      # find all tsObj files     
      set_run $run -rerun $rerun $camCol -tsObj 
      set tsObjdir $openit(objdir)
      if {$fieldEnd >= $fieldStart} {
         if [catch {
             set tsObjList ""
             loop i $fieldStart [expr $fieldEnd + 1] {
                if [catch {
                    set tsObj [glob $tsObjdir/tsObj-[format %06d $run]-$camCol-*-[format %04d $i].fit]
                    }] {
                    echo " Couldn't find tsObj file for field $i in $tsObjdir !?!"
                } else {
                    set tsObjList "$tsObjList $tsObj"
                }    
             }
                 }] {
             error " Problem accessing tsObj files from $tsObjdir"
             return ""
         } 
      } else { 
         if [catch {
            set tsObjList [glob $tsObjdir/tsObj-[format %06d $run]-$camCol-*-*.fit]
                 }] {
             echo " Couldn't find any tsObj files in"
             error " $tsObjdir/tsObj-[format %06d $run]-$camcol-*-[format %04d $field].fit"
             return ""
         } 
      }
    
    return $tsObjList

}



# given a list of reddening corrected magnitudes {u g r i z},
# return a vector mask set to $inputMask for objects which qualify for 
# the specified sample, and 0 otherwise. The available samples are
# type = 0: bright red dwarfs, defined by: g-r>1, r-i>0.8, 14<r<17 
# type = 1: faint red dwarfs, defined by: g-r>1, r-i>0.8, 19<r<20 
# type = 2: blue disk stars, defined by: 0.9<u-g<1.6, 14<r<17
# type = 3: blue halo stars, defined by: 0.6<u-g<1.3, 19<r<20
# type = 4: all bright stars, defined by g<20
# type = 5: all bright stars, defined by r<20
# type = 6: RR Lyrae box
# type = 7: UV excess: u-g<0.7 && r<21
proc getColorSampleMask {mags magErrs type {inmask ""}} {

     set filterlist {u g r i z}
     foreach f $filterlist {
         set ifilter [lsearch $filterlist $f]
         set $f [lindex $mags $ifilter]
     }
     if {$inmask == ""} {
         set inmask [vectorExprNew [exprGet $u.dimen]]
         vectorExprSet $inmask 1
     }

     if {$type == 0} {
         # bright red dwarfs
         set gr [vectorExprEval "$g-$r"]
         set ri [vectorExprEval "$r-$i"]
         set cond "$gr > 1 && $ri > 0.8 && $r > 14 && $r <17"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $gr $ri]
     } elseif {$type == 1} {
         # faint red dwarfs
         set gr [vectorExprEval "$g-$r"]
         set ri [vectorExprEval "$r-$i"]
         set cond "$gr > 1 && $ri > 0.8 && $r > 19 && $r <20"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $gr $ri]
     } elseif {$type == 2} {
         # blue disk stars
         set ug [vectorExprEval "$u-$g"]
         set cond "$ug > 0.9 && $ug < 1.6 && $r > 14 && $r < 17"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug]
     } elseif {$type == 3} {
         # blue halo stars
         set ug [vectorExprEval "$u-$g"]
         set cond "$ug > 0.6 && $ug < 1.3 && $r > 19 && $r < 20"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug]
     } elseif {$type == 4} {
         # all bright stars (g selection)
         set cond "$g > 14 && $g < 20"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
     } elseif {$type == 5} {
         # all bright stars (r selection)
         set cond "$r > 14 && $r < 20"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
     } elseif {$type == 6} {
         # RR Lyrae box
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
         set x1 [vectorExprEval "$ug+0.67*$gr-1.07"]
         set x2 [vectorExprEval "0.45*$ug-$gr-0.12"]
         set cond "$ug > 0.98 && $ug <1.30 && $r > 14 && $r < 21.5"
         set cond "$cond && $x1 > -0.05 && $x1 < 0.35"
         set cond "$cond && $x2 > 0.06 && $x2 < 0.55"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug $gr $x1 $x2]
         return $outmask
     } elseif {$type == 7} {
         # UV excess
         set ug [vectorExprEval "$u-$g"]
         set cond "$ug > -2 && $ug < 0.7 && $r > 14 && $r < 21.5"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug]
     } elseif {$type == 8} {
         # especially non-variable stars (roughly K type)
         set ug [vectorExprEval "$u-$g"]
         set gr [vectorExprEval "$g-$r"]
         set cond "$ug > 1.7 && $ug < 1.9 && $gr > 0.6 && $gr < 0.9 && $r > 14 && $r < 21.5"
         set outmask [vectorExprEval "$cond ? $inmask : 0"]
         vListDel [list $ug]
     } else {
         error "proc getColorSampleMask doesn't recognize type $type!?"
     }
  
   return $outmask

}



## DRAFT ##
#### for available rerun, dump various types of sources according
## to selectionType:
## selectionType == 1 
##   - unresolved and nchild = 0, and either 
##       a) UV sources defined by u-g < 0.7 && r<21 (dereddened)
##    or b) RR Lyrae color box (see proc getColorSampleMask)
## 
## n.b. a user-defined list of {run rerun} can be directly 
## specified by RRlist 
proc dumpFromRerun {rerun {selectionType 1} {outputDir "./"} {RRlist ""} {N1 0} {N2 -1}} {

     # open log file   
     set fd [open $outputDir/dumpFromRerun.log "w"]
     puts $fd "# *** LOG FILE from dumpFromRerun for rerun $rerun ***"
     puts $fd "# started on [exec date]"
     flush $fd

     ### first get a list of all run/reruns
     if {$RRlist == ""} {
        set RRlist [doSDSSMOCrerun $rerun "" "return"]
     }
     if {$N1 <= $N2} {
        set aux "$RRlist"
        set RRlist ""
        loop i [expr $N1-1] $N2 {
           set RRlist "$RRlist [list [lindex $aux $i]]"
        } 
     }

     set line " Found [llength $RRlist] good runs, going to process..."
     echo $line; puts $fd $line; flush $fd
 
     set condition " UVx (u-g < 0.7 && r<20) or RR Lyrae color box"
     set NtsObjMax 100
     set NtotC 0; set NtotS 0; set NtotG 0; set NtotD 0
     set NtotDUV 0; set NtotDRR 0; set NtotDNV 0

     ### loop over all runs
     foreach rr $RRlist {
        set run [lindex $rr 0]
        set rerun [lindex $rr 1]
        set line "  --- processing run $run ----"
        echo $line; puts $fd $line; flush $fd
        # continue
        ## make file and header for this run
        set outfile $outputDir/Dump-$run-$rerun
        # doDumpFileHeader $outfile $run $rerun ""
        set Nc 0; set Ns 0; set Ng 0; set Nd 0
        set NdUV 0; set NdRR 0; set NdNV 0
        ## loop over all camera columns
        foreach camCol {1 2 3 4 5 6} {
          if [catch {
            set tsObjList [tsObjListGet $run $rerun $camCol] 
	  } msg] {
            echo "problems getting tsObj list: $msg"
	  }
          if [catch {
            set line "     camera column $camCol, found [llength $tsObjList] tsObj files"
	    echo $line; puts $fd $line; flush $fd
            # have to split list in smaller chunks
            set Nfiles [llength $tsObjList]
            set Nchunks [expr 1+$Nfiles/$NtsObjMax]
            # loop over chunks
            loop i 0 $Nchunks {
               set j1 [expr $i*$NtsObjMax]; set j2 [expr $j1+$NtsObjMax]
	       if {$j2 > $Nfiles} {set j2 $Nfiles}
               set list ""
               loop j $j1 $j2 {
                  set list "$list [lindex $tsObjList $j]"
               }
               ## dump data for this list of tsObj files
               if [catch {
                   set Ndumped [tsObjList2dump $list $outfile $selectionType 0.3 21.5 6] 
               } msg] {
                   echo "problems with dumping [llength $list] files: $msg"
	       }
               set NcX [lindex $Ndumped 0]
               set NsX [lindex $Ndumped 1]
               set NgX [lindex $Ndumped 2]
               set NdXUV [lindex $Ndumped 3]
               set NdXRR [lindex $Ndumped 4]
               set NdXNV [lindex $Ndumped 5]
               set NdX [expr $NdXUV + $NdXRR + $NdXNV]
               set Nc [expr $Nc + $NcX]
               set Ns [expr $Ns + $NsX]
               set Ng [expr $Ng + $NgX]
               set NdUV [expr $NdUV + $NdXUV]
               set NdRR [expr $NdRR + $NdXRR]
               set NdNV [expr $NdNV + $NdXNV]
               set Nd [expr $Nd + $NdX]
               set aux "        for $run $rerun $camCol chunk $i"
               if {$NcX <= 0} { 
                  set line " PROBLEMS $aux"
               } else {
	          set NtotC [expr $NtotC + $NcX]     
	          set NtotS [expr $NtotS + $NsX]     
	          set NtotG [expr $NtotG + $NgX]     
	          set NtotD [expr $NtotD + $NdX]     
	          set NtotDUV [expr $NtotDUV + $NdXUV]     
	          set NtotDRR [expr $NtotDRR + $NdXRR]     
	          set NtotDNV [expr $NtotDNV + $NdXNV]     
                  set line " $aux DUMPED $NdX (Nc=$NcX, Ns=$NsX, Ng=$NgX)"      
               }
	       echo $line; puts $fd $line; flush $fd
            }  
	  } msg] {
            echo "problems: $msg"
	  }
        }
        ##
        set line "  -- FOR $run $rerun $camCol processed $Nc sources,"
        set line "$line selected $Ns, $Ng are good, and dumped total of $Nd"
	echo $line; puts $fd $line; flush $fd        
     }

     set lineH "  #########################################################################"
     echo $lineH; puts $fd $lineH; flush $fd        
     set line "  --> processed $NtotC sources, selected $NtotS, $NtotG are good, and dumped total of $NtotD"
     echo $line; puts $fd $line; flush $fd        
     set line "      $NtotDUV UV, $NtotDRR RR Lyrae, $NtotDNV nonVariable"
     echo $line; puts $fd $line; flush $fd        
     echo $lineH; puts $fd $lineH; flush $fd        

     puts $fd "# finished on [exec date]"
     close $fd

}


proc testTime {tsObjList {outfile mem.trace.testTime}} {

   if {0 && [info commands "phGenericChainDestroy"] == ""} {
       rename genericChainDestroy {}
       rename phGenericChainDestroy genericChainDestroy
   }

   # for measuring execution time
   timerStart

   set mem [open $outfile w]

   # for  memory reporting
   set startMem [memNextSerialNumber]

   set NtsObj [llength $tsObjList]
   write_mem mem "reading chain for $NtsObj files"
   echo "reading chain for $NtsObj files"

   set c [tsObjList2chain $tsObjList] 
   set Nc [chainSize $c] 

   write_mem mem "selecting chain with $Nc els."
   set cond "{objc_rowc >= 64} {objc_rowc < 1425} {psfCounts<2> < 21} {objc_type == 6}" 
   set s [chainSearch $c $cond] 
   set Ns [chainSize $s] 

   set reclist "objc_flags objc_flags2"
   loop i 0 5 {
      set reclist "$reclist psfCounts<$i>"
      set reclist "$reclist psfCountsErr<$i>"
      set reclist "$reclist reddening<$i>"
   }
   
   write_mem mem "extracting vectors for $Ns els."
   echo "extracting vectors for $Ns els."
   set veclist [vectorsFromChain $s $reclist] 
   
   echo "deleting s chain"
   write_mem mem "deleting s chain"
   chainDel $s 
   echo "destroying c chain"
   write_mem mem "destroying c chain"
   if {0} {
      chainDestroy $c genericDel
   } else {
      phGenericChainDestroy $c
   }
        
   echo "deleting vectors"
   write_mem mem "deleting vectors"
   vListDel $veclist
  
   write_mem mem "done with testTime"

   close $mem
   

}

## this should be a more general proc
proc tsObjList2dump {tsObjList outfileRoot {selectionType 1} {magErrMax 0.3} \
	                {rMax 21.5} {objcType 6}} {

         echo "         reading and selecting tsObj chains..."
         ## first read chain
         set c [tsObjList2chain $tsObjList] 
         set Nc [chainSize $c] 

         ## select objType sources with r<rMax
         set cond "{objc_rowc >= 64} {objc_rowc < 1425} {psfCounts<2> < $rMax} {objc_type == $objcType}" 
         set s [chainSearch $c $cond] 
         set Ns [chainSize $s] 
         echo "            => Nc = $Nc Ns=$Ns"

         ### now get vectors ###
         set reclist ""
         # basic data
         set basicCount 0
         set reclist "$reclist run rerun camCol field id objc_rowc objc_colc"
         # psf mags and errors
         set psfStartCount 7
         loop i 0 5 {
            set reclist "$reclist psfCounts<$i>"
         }
         set psfErrStartCount 12
         loop i 0 5 {
            set reclist "$reclist psfCountsErr<$i>"
         }
         # reddening
         set reddStartCount 17
         loop i 0 5 {
            set reclist "$reclist reddening<$i>"
         }
         # flags
         set flagsStartCount 22
         set reclist "$reclist objc_flags objc_flags2"
         # other miscaleneous
         set miscStartCount 24
         set reclist "$reclist primTarget secTarget ra dec objc_type nchild parent"
         # mod mags and errors
         set modStartCount 31
         loop i 0 5 {
            set reclist "$reclist counts_model<$i>"
         }
         set modErrStartCount 36
         loop i 0 5 {
            set reclist "$reclist counts_modelErr<$i>"
         }

         ## extract vectors and get rid of chain
         echo "         making vectors..."
         set veclist [vectorsFromChain $s $reclist] 
         echo "         deleting chains..."
         chainDel $s 
         phGenericChainDestroy $c
         echo "         selecting and dumping..."

         ### given list of vectors in veclist, now dump to $outfile
         # make reddening corrected magnitude vectors
         set psfMags ""; set psfMagErrs ""
         loop i 0 5 {
             set auxP [lindex $veclist [expr $psfStartCount + $i]]
             set auxRedd [lindex $veclist [expr $reddStartCount + $i]]
	     set psf($i) [vectorExprEval $auxP-$auxRedd]
             set psfMags "$psfMags $psf($i)"
	     set psfErr($i) [lindex $veclist [expr $psfErrStartCount + $i]]
             set psfMagErrs "$psfMagErrs $psfErr($i)"
         }

         #### all data vectors are set, now select sources ####         
         ## first set quality masks
         set flags [lindex $veclist [expr $flagsStartCount + 0]]
         set flags2 [lindex $veclist [expr $flagsStartCount + 1]]
         set GoodPhotometryMask [flagsMask $flags {BINNED1} {BRIGHT SATUR BLENDED}] 
         set NotMovingMask [flagsMask $flags2 {} {DEBLENDED_AS_MOVING}] 
         set okMask [vectorExprEval $GoodPhotometryMask*$NotMovingMask]
         ## magErr mask for r psf mag (ONLY, in all 5 bands becomes too restrictive)
         set cond "$psfErr(2) < $magErrMax" 
         set goodPsfMags [vectorExprEval "$cond ? 1 : 0"]  
         set okMaskPsf [vectorExprEval $okMask*$goodPsfMags] 
         # number of potential candidates, i.e. without color cuts
         set Ng [vectorExprGet sum($okMaskPsf)]
         # clean aux masks 
         vListDel "$GoodPhotometryMask $NotMovingMask $okMask $goodPsfMags"
     
         ### THE MAIN SELECTION (n.b. RR Lyrae and UV sources are exclusive!)
         # set mask for RR Lyrae 
	 set RRmask [getColorSampleMask $psfMags $psfMagErrs 6 $okMaskPsf]
         # set mask for UV sample using psf mags
	 set UVmask [getColorSampleMask $psfMags $psfMagErrs 7 $okMaskPsf]
         # roughly K stars selected as being the least variable
         set nonVarmask [getColorSampleMask $psfMags $psfMagErrs 8 $okMaskPsf]

         ### dump data
         # first form vector list
         set Dveclist "[lindex $veclist [expr $miscStartCount + 2]]"
         set Dveclist "$Dveclist [lindex $veclist [expr $miscStartCount +3]]"
         set Dveclist "$Dveclist [lindex $veclist [expr $reddStartCount +2]]"
         loop i 0 5 {
            set Dveclist "$Dveclist [lindex $veclist [expr $psfStartCount+$i]]"
         }
         set Dveclist "$Dveclist [lindex $veclist [expr $miscStartCount + 4]]"
         loop i 0 5 {
            set Dveclist "$Dveclist [lindex $veclist [expr $psfErrStartCount+$i]]"
         }
         set Dveclist "$Dveclist [lindex $veclist [expr $miscStartCount +5]]"
         set Dveclist "$Dveclist [lindex $veclist [expr $miscStartCount +6]]"
         loop i 0 7 {
            set Dveclist "$Dveclist [lindex $veclist [expr $basicCount+$i]]"
         }
         ## add model magnitudes and errors
         loop i 0 10 {
            set Dveclist "$Dveclist [lindex $veclist [expr $modStartCount+$i]]"
         }
         
         # dump all flavors
         set outfile $outfileRoot.UVx
         set NdumpedUVx [vecListToFile $Dveclist $outfile $UVmask]
         set outfile $outfileRoot.RRLyr
         set NdumpedRRLyr [vecListToFile $Dveclist $outfile $RRmask]
         set outfile $outfileRoot.nonVar
         set NdumpedNonVar [vecListToFile $Dveclist $outfile $nonVarmask]

         # clean masks 
         vListDel "$okMaskPsf $RRmask $UVmask $psfMags $nonVarmask"
         vListDel $veclist

    return [list $Nc $Ns $Ng $NdumpedUVx $NdumpedRRLyr $NdumpedNonVar]
}


    

## similar to vectorsWriteToFile, but specialized for tsObjList2dump:
## vList includes
## RA Dec Ar u g r i z type uErr gErr rErr iErr zErr 
##             nchild parent run rerun camCol field id objc_rowc objc_colc  
proc vecListToFile {vList filename mask {mode "a"}} {

    set header "#   RA    Dec   Ar u g r i z type uErr gErr rErr iErr zErr"
    set header "$header nchild parent run rerun camCol field id objc_rowc objc_colc" 
    set header "$header umod gmod rmod imod zmod umodErr gmodErr rmodErr imodErr zmodErr" 

    # open file
    set f [open $filename $mode]
    if {$mode == "w"} {
       puts $f " #  $header" 
    }

    # get number of elements
    set N [exprGet [lindex $vList 0].dimen]

    # loop through all elements
    set format ""; set vecs ""
    set Ngood 0
    loop i 0 $N {
       set good [exprGet $mask.vec<$i>]
       if {$good} {
          # RA and Dec
          set ra  [format "%10.5f" [exprGet [lindex $vList 0].vec<$i>]]
          set dec [format "%10.5f" [exprGet [lindex $vList 1].vec<$i>]]
          set str "  $ra $dec"
          # Ar and ugriz
          loop j 2 8 {
	    set vec [lindex $vList $j]
            set val [format "%5.2f" [exprGet $vec.vec<$i>]]
	    if {$val < 0 || $val >= 100} {set val 99.99}          
            set str "$str $val"
          }
          # type
          set type [format "%2.0f" [exprGet [lindex $vList 8].vec<$i>]]
          set str "$str $type"
          # errors for ugriz
          loop j 9 14 {
	    set vec [lindex $vList $j]
            set val [format "%5.2f" [exprGet $vec.vec<$i>]]
	    if {$val < 0 || $val >= 100} {set val 99.99}          
            set str "$str $val"
          }
          # nchild parent
          set nchild [format "%3.0f" [exprGet [lindex $vList 14].vec<$i>]]
          set parent [format "%3.0f" [exprGet [lindex $vList 15].vec<$i>]]
          set str "$str $nchild $parent"

          # run data 
          loop j 16 21 {
	    set vec [lindex $vList $j]
            set val [exprGet $vec.vec<$i>]          
            set str "$str $val"
          }
          loop j 21 23 {
	    set vec [lindex $vList $j]
            set val [format "%6.1f" [exprGet $vec.vec<$i>]]         
            set str "$str $val"
          }
          # model magnitudes
          loop j 23 33 {
	    set vec [lindex $vList $j]
            set val [format "%5.2f" [exprGet $vec.vec<$i>]]
	    if {$j < 29} {
	       if {$val < 0 || $val >= 100} {set val 99.99} 
	    }         
            set str "$str $val"
          }

          puts $f $str
          incr Ngood  
       }
    }

    # close file
    close $f

  return $Ngood
}



# aux proc to write header to DumpFile* files
proc doDumpFileHeader {filename run rerun condition} {

       # open file
       set f [open $filename w]
       if {$condition != ""} {
         set date [exec date]
         set user [exec whoami]
         puts $f "#  Dump for run $run rerun $rerun satisfying"
         puts $f "#  $condition"       
         puts $f "#  produced using photo version [photoVersion]"
         puts $f "#  by $user on $date"
       }
       # close file
       close $f
}
