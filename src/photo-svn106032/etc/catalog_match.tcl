### matching 2MASS

# concatenate par files
proc concat_parfiles {filelist outfile} {

    set file0 [lindex $filelist 0]
    foreach parfile $filelist {
       if {$parfile == $file0} {
           set chain [param2Chain $parfile ""]
       } else {
           chainJoin $chain [param2Chain $parfile ""]
       }
    }
    chain2Param $outfile $chain ""
    set N [chainSize $chain]
    genericChainDestroy $chain

    echo " dumped $N sources to $outfile"

}


proc match2MASSPSC_RArange {h1 h2 run rerun camCols} {

    loop h $h1 $h2 {
        set hS $h
        set hE [expr $hS+1]
        set parfile TWOMASSpsc_hour${hS}_hour${hE}.par  
        match2MASS $parfile 1 $run $rerun $camCols
    }
}



proc classify2MASS_RArange {h1 h2 outname {Jcutoff 15.8}} {

    echo "   RA1  RA2    all    bright    blue    red    galaxies"
    set Ntot 0
    set NtotB 0
    loop h $h1 $h2 {
        set hS $h
        set hE [expr $hS+1]
        set parfile TWOMASSpsc_hour${hS}_hour${hE}.par 
        set chain [param2Chain $parfile ""]
        set Nall [chainSize $chain]
        set bright [chainSearch $chain "{J <= $Jcutoff}"]
        set Nbright [chainSize $bright]
        if {$h == $h1} {
            chain2Param $outname $bright ""
        } else {
            chain2Param $outname $bright "" -append
        }
        chainDel $bright
        genericChainDestroy $chain
        echo "     $hS    $hE   $Nall    $Nbright"
        set Ntot [expr $Ntot + $Nall]
        set NtotB [expr $NtotB + $Nbright]
    }
        
    echo "-----------------------------------"
    echo "      Total  $Ntot    $NtotB"

}


## match 2MASS par file to specified SDSS run
# for 2MASS PSC type=1, for 2MASS XSC type=2
# match2MASSX TWOMASSXSC_eq.par 756 6 {1 2 3} 756
proc match2MASS {parfile type run rerun {camCols "all"} {outdir .} {chain ""}} {

global data_root

     if {$type != 1 && $type != 2} {
         echo "type = $type !?! <= for 2MASS PSC type=1, for 2MASS XSC type=2"
         return
     } 

     ### 2MASS-SDSS matching radius in arcsec
     set matching_radius 2.0
     # where is the data  
     set data_root [get_data_root ""]

     # fields to transfer
     set transferFields {}
     if {$type == 1} {
        # assumes the knowledge of TWOMASSPSC structure    
        lappend transferFields [list ra lambda]   
        lappend transferFields [list dec eta]
        lappend transferFields [list err_maj texture<0>]
        lappend transferFields [list err_min texture<1>]
        lappend transferFields [list err_ang texture<2>]
        lappend transferFields [list J fiberCounts<0>]
        lappend transferFields [list J_err fiberCountsErr<0>]
        lappend transferFields [list H fiberCounts<1>]
        lappend transferFields [list H_err fiberCountsErr<1>]
        lappend transferFields [list K fiberCounts<2>]
        lappend transferFields [list K_err fiberCountsErr<2>]
        lappend transferFields [list rd_flg fracPSF<0>]
        lappend transferFields [list bl_flg fracPSF<1>]
        lappend transferFields [list cc_flg fracPSF<2>]
        lappend transferFields [list extd_flg fracPSF<3>]
        lappend transferFields [list mp_flg fracPSF<4>]
     } else { 
        # assumes the knowledge of TWOMASSXSC structure    
        lappend transferFields [list ra lambda]   
        lappend transferFields [list dec eta]
        lappend transferFields [list r_3sig texture<0>]
        lappend transferFields [list j_ba texture<1>]
        lappend transferFields [list k_ba texture<2>]
        lappend transferFields [list k_phi texture<3>]
        lappend transferFields [list J fiberCounts<0>]
        lappend transferFields [list J_err fiberCountsErr<0>]
        lappend transferFields [list H fiberCounts<1>]
        lappend transferFields [list H_err fiberCountsErr<1>]
        lappend transferFields [list K fiberCounts<2>]
        lappend transferFields [list K_err fiberCountsErr<2>]
        lappend transferFields [list Jflg fracPSF<0>]
        lappend transferFields [list Hflg fracPSF<1>]
        lappend transferFields [list Kflg fracPSF<2>]
        lappend transferFields [list cc_flg fracPSF<3>]
        lappend transferFields [list escore texture<4>]
        lappend transferFields [list gscore fracPSF<4>]
        lappend transferFields [list BR fiberCounts<3>]
        lappend transferFields [list RK fiberCounts<4>]
     } 

     # do we need to read the guest chain
     if {$chain == ""} {
         set chain [param2Chain $parfile ""]
         set read_chain 1
         set Nguest [chainSize $chain]
         echo "Read in $Nguest guests, handle = $chain"
     } else {
         set read_chain 0
     }

     # do work
     match_parfile2run $parfile $run $rerun $camCols -1 -1 \
           $transferFields $matching_radius $data_root $outdir $chain
        
     if {$read_chain} {
         echo "deleting chain $chain"
         genericChainDestroy $chain
     }
    
     
}



### matching deep VLA data (from SIRTF FLS)
proc DeepVLAmatchAll {rerun} {
  foreach run {1336 1339 1356 1359} {
     matchDeepFIRST DeepVLA.par $run $rerun
  }
}


proc matchDeepFIRST {parfile run rerun {cols "all"} {outdir .} {sF -1} {eF -1}} {

global data_root

     if {$cols == "all"} {set cols {1 2 3 4 5 6}}
     # fields to transfer
     set transferFields {}
     lappend transferFields [list ra firstLambda]   
     lappend transferFields [list dec firstEta]
     lappend transferFields [list Fint firstInt]
     lappend transferFields [list Amaj fiberCounts<3>]
     lappend transferFields [list Amin fiberCountsErr<3>]
     lappend transferFields [list Pangle texture<3>]
     lappend transferFields [list irms texture<0>]
     lappend transferFields [list resrms texture<1>]
     lappend transferFields [list respeak texture<2>]
     lappend transferFields [list resflux texture<4>]

     # do work
     foreach col $cols {
        # matching within 3 arcsec radius (square indeed)
        match_parfile2run $parfile $run $rerun $col $sF $eF $transferFields 3.0 
        my_mv ${parfile}_detected ${parfile}_detected_run${run}_col${col}.par
        my_mv ${parfile}_notdetected ${parfile}_notdetected_run${run}_col${col}.par
     }
     
}

# clean a SDSSFIRST par file
proc dumpDeepFIRST {tsObjList nameRoot} {
    
    set A [tsObjList2chain $tsObjList]    
    set prim [get_primaries $A]  

    set  S [chainSearch $prim "{objc_type == 6}"] 
    set  G [chainSearch $prim "{objc_type == 3}"]    

    # write out SDSSFIRST structure definitions
    set hdr ""
    set chain [chainNew SDSSFIRST]
    chain2Param ${nameRoot}_S_raw.par $chain $hdr
    chain2Param ${nameRoot}_G_raw.par $chain $hdr
    chainDestroy $chain genericDel
    dumpSDSSFIRSTmatches $S ${nameRoot}_S_raw.par 0
    dumpSDSSFIRSTmatches $G ${nameRoot}_G_raw.par 0

    # now clean files for proper flags
    set fON {BINNED1}
    set fOFF {BRIGHT SATUR BLENDED}
    flag_search ${nameRoot}_S_raw.par ${nameRoot}_S_clean.par $fOFF $fOFF $fON $fON
    flag_search ${nameRoot}_G_raw.par ${nameRoot}_G_clean.par $fOFF $fOFF $fON $fON
    # and from doubles
    clean_doubles ${nameRoot}_S_clean.par ${nameRoot}_S_unique.par 1.0 
    clean_doubles ${nameRoot}_G_clean.par ${nameRoot}_G_unique.par 1.0 

} 



proc matchFIRST {parfile run rerun {cols "all"} {outdir .} {sF -1} {eF -1}} {

global data_root

     if {$cols == "all"} {set cols {1 2 3 4 5 6}}
     # fields to transfer
     set transferFields {}
     lappend transferFields [list ra firstLambda]   
     lappend transferFields [list dec firstEta]
     lappend transferFields [list Fpeak firstPeak]
     lappend transferFields [list Fint firstInt]
     lappend transferFields [list Rms firstRms]
     lappend transferFields [list Maj fiberCounts<3>]
     lappend transferFields [list Min fiberCountsErr<3>]
     lappend transferFields [list PA texture<3>]
     lappend transferFields [list fMaj fiberCounts<4>]
     lappend transferFields [list fMin fiberCountsErr<4>]
     lappend transferFields [list fPA texture<4>]

     # do work
     foreach col $cols {
        # matching within 3 arcsec radius (square indeed)
        match_parfile2run $parfile $run $rerun $col $sF $eF $transferFields 3.0 
        my_mv ${parfile}_detected ${parfile}_detected_col$col.par
        my_mv ${parfile}_notdetected ${parfile}_notdetected_col$col.par
     }
     
}




# after running matchFIRST do the following in that dir

# 1) dump data from tsObj files to a par file
# photo> set tsObjList [glob tsObj*fit]; dump_SDSSFIRST $tsObjList run$run-FIRST-raw.par 0
# 2) fake primary => this works ONLY for equatorial runs
# photo> fake_primary run$run-FIRST-raw.par run$run-FIRST-prim.par [get_DecRange $run]
# 3) get unique sources 
# photo> get_unique_sources run$run-FIRST-prim.par run$run-FIRST-unique.par

# all 3 in 
proc processFIRST {run} {

     set tsObjList [glob tsObj*fit]
     dump_SDSSFIRST $tsObjList run$run-FIRST-raw.par 0
     fake_primary run$run-FIRST-raw.par run$run-FIRST-prim.par [get_DecRange $run]
     get_unique_sources run$run-FIRST-prim.par run$run-FIRST-unique.par
 
}

# 4) for a list of runs join all "unique" par files
# photo> concat_parfiles $parfilelist $outfile
# 5) get resolved/unresolved 
# photo> 



# for a given SDSSFIRST equatorial matches select sources with
# -1.25 < Dec < 1.25 and (10 < RA < 50 or 145 < RA < 235)
# this works both for matches and input files
proc select_SDSSFIRST_sample {infile outfile} {

     # first read in the chain
     set chain [param2Chain $infile ""]
     echo "read [chainSize $chain] sources"

     set goodDec [chainSearch $chain "{dec >= -1.25} {dec <= 1.25}"]
     echo "selected [chainSize $goodDec] with -1.25 <= dec <= 1.25"    
     set goodArea [chainSearch $goodDec "{ra >= 10.0} {ra <= 50.0}"]
     echo "found [chainSize $goodArea] with 10 < ra < 50"
     chainJoin $goodArea [chainSearch $goodDec "{ra >= 145.0} {ra <= 235.0}"]
     echo "and [chainSize $goodArea] in total"

     chain2Param $outfile $goodArea ""
     echo " dumped to $outfile"

     chainDel $goodDec;  chainDel $goodArea
     genericChainDestroy $chain   
}




# a little hack to go with another hack (fake_primary)
# works only for runs listed below
proc get_DecRange {run} {

   # separation of two successive columns
   set step 0.419797
   set half_step [format "%9.7f" [expr $step / 2]]
   
   # Dec 1 is the left side of column 1
   # Dec 2 is the right side of column 1
   # the rest of columns are generated with half_step 
   if {$run == 94} {
       set Dec1 -1.0358820
       set Dec2 [expr $Dec1 + $half_step]
   } elseif {$run == 125} {
      # this is the south of the two
      set Dec1 -1.252810 
      set Dec2 -1.0358820
   } elseif {$run == 756} {
      set Dec1 -1.049465   
      set Dec2 [expr $Dec1 + $half_step]
   } elseif {$run == 752} {
      # this is the south of the two
      set Dec1 -1.267892
      set Dec2 -1.049465
   } else {
      error "get_DecRange doesn't know about run $run"
   }

   # generate the list
   set GoodDecList {}
   set DecRange [list $Dec1 $Dec2]
   lappend GoodDecList $DecRange
   loop i 1 6 {
      set DecOld [lindex [lindex $GoodDecList [expr $i-1]] 1]
      set DecMin [expr $DecOld + $half_step]
      set DecMax [expr $DecOld + $step]
      set DecRange [list $DecMin $DecMax]
      lappend GoodDecList $DecRange      
   }
   return $GoodDecList
}


# ** this is an outrageous hack and works only for equatorial scans **
# given a par file with matches, and a list of allowed Dec ranges
# extract sources within those ranges
# the Dec list is a list of lists {DecMin DecMax}
proc fake_primary {inparfile outparfile GoodDecList} {
  
   # first read the chain
   set chain [param2Chain $inparfile ""]
   set selected [chainNew [exprGet $chain.type]]
   set Nin [chainSize $chain]
   echo " processing $Nin sources from $inparfile"
   
   # now extract sources within each allowed Dec range 
   foreach Decrange $GoodDecList {
       set DecMin [lindex $Decrange 0]
       set DecMax [lindex $Decrange 1]  
       set cond "{dec > $DecMin} {dec <= $DecMax}"
       chainJoin $selected [chainSearch $chain $cond]
   }
   
   set Nsel [chainSize $selected]
   chain2Param $outparfile $selected ""
   echo " selected $Nsel sources and dumped to $outparfile"

   chainDel $selected
   genericChainDestroy $chain
    
}



# given a par file with matches, produce a list with
# physically unique unsaturated sources: 
# 1) exclude all sources with flags {BRIGHT SATUR BLENDED}
# 2) exclude all positional doubles within dmin arcsec from each other
proc get_unique_sources {inparfile outparfile {dmin 0.15}} {


     # first clean by flags 
     set flagsOFF {BRIGHT SATUR BLENDED}
     catch {unlink aux};
     flag_search $inparfile aux $flagsOFF $flagsOFF 
  
     # and now clean doubles 
     clean_doubles aux $outparfile $dmin

}


# given a par file with ra and dec fields, remove all sources
# that are within dmin arcsec from each other. 
proc clean_doubles {inparfile outparfile {dmin 0.5}} {

     # dmin is in arcsec
     set dmin2 [expr $dmin*$dmin]

     # first read the chain
     set chain [param2Chain $inparfile ""]
  
     # then sort by ra 
     chainFSort $chain ra

     # and now loop and remove all doubles
     set raOld1 -1; set decOld1 -100;
     set raOld2 -1; set decOld2 -100;
     set removed [chainNew [exprGet $chain.type]]
     set Nin [chainSize $chain]
     set cursor [chainCursorNew $chain]
     while {[set el [chainWalk $chain $cursor]] != ""} {
         set ra [exprGet $el.ra];  set dec [exprGet $el.dec]
         # we are checking two nearest neighbors in RA
         if {[expr abs($ra-$raOld1)] < $dmin} {
             set delRA [expr 3600*($ra-$raOld1)]
             set delDec [expr 3600*($dec-$decOld1)]
             set nearby 1
         } elseif {[expr abs($ra-$raOld2)] < $dmin} {
             set delRA [expr 3600*($ra-$raOld2)]
             set delDec [expr 3600*($dec-$decOld2)]
             set nearby 1
         } else {
             set nearby 0
         }  
         if {$nearby} {
             # is d*d < dmin*dmin?
             if {[expr pow($delRA,2)+pow($delDec,2)] < $dmin} {
                 # this is a double source
                 chainElementAddByPos $removed \
                          [chainElementRemByCursor $chain $cursor]
             }
         }
         set raOld2 $raOld1; set decOld2 $decOld1
         set raOld1 $ra; set decOld1 $dec
     }
     chainCursorDel $chain $cursor

     set Nrem [chainSize $removed]
     assert {[expr $Nrem + [chainSize $chain]] == $Nin}

     # write chain to par file
     chain2Param $outparfile $chain ""

     genericChainDestroy $chain
     genericChainDestroy $removed
     
     echo "Removed $Nrem (out of $Nin) double (within $dmin arcsec) elements"


}


# given a dump file with SDSSFIRST data, select unresolved sources in gri band,
# with mpsf(band) < mmax
proc get_FIRST_unresolved {sdss-first-parfile outfile {band r} {mmax 30.0}} {

     # first read all sources   
     set all [param2Chain ${sdss-first-parfile} ""]
     set Nall [chainSize $all]
     echo " read $Nall sources from ${sdss-first-parfile}"

     # select bright sources
     set radiostars [chainSearch $all \
         "{$band < $mmax} {gtype == 6} {rtype == 6} {itype == 6}"]  
     set Nrs [chainSize $radiostars] 
     echo " selected $Nrs gri unresolved sources with mpsf($band) < $mmax"

     set hdr ""
     chain2Param $outfile $radiostars $hdr 
     echo " and dumped to file $outfile"

     chainDel $radiostars
     chainDestroy $all genericDel

}

# given a dump file with SDSSFIRST data, select radio galaxies by
# searching for resolved sources in gri band,  with 
# mmod(band) < mmax 
proc get_radio_galaxies {sdss-first-parfile outfile {band r} {mmax 30.0}} {

     # first read all sources   
     set all [param2Chain ${sdss-first-parfile} ""]
     set Nall [chainSize $all]
     echo " read $Nall sources from ${sdss-first-parfile}"

     # select bright sources
     set radiogalaxies [chainSearch $all \
     "{${band}mod < $mmax} {gtype == 3} {rtype == 3} {itype == 3}"]  
     set Nrs [chainSize $radiogalaxies] 
     echo " selected $Nrs gri resolved sources with mmod($band) < $mmax"

     set hdr ""
     chain2Param $outfile $radiogalaxies $hdr 
     echo " and dumped to file $outfile"

     chainDel $radiogalaxies
     chainDestroy $all genericDel

}


# wrapper for matchFIRST-2MASS-SDSS, e.g.
# set dir /peyton/scr/hydra0/sdss; set run 94
# matchManyFIRST-2MASS-SDSS $dir/FIRST-SDSS/$run $dir/2MASS-SDSS/$run $run {1 2 3} 0 1
proc matchManyFIRST-2MASS-SDSS {dirFIRST dir2MASS run cols RA1 RA2 {dirOut .}} {

    foreach col $cols {
       set aux1 tsObj_run${run}_col$col.fit
       set aux2 tsObj-TWOMASSpsc_hour
       set fileFIRST $dirFIRST/tsObj-SDSSeq-FIRST.par_$aux1
       set chainFIRST [tsObj2chain $fileFIRST]
       loop h $RA1 $RA2 {
          set hS $h
          set hE [expr $hS+1]
          set ra1 [expr $hS*15.0]
          set ra2 [expr $hE*15.0]
          set file2MASS $dir2MASS/${aux2}${hS}_hour${hE}.par_$aux1
          set outfile FIRSTvs2MASS-$run-$col-RA${hS}h-${hE}h
          # select FIRST sources from this RA range
          set sel [chainSearch  $chainFIRST  "{ra > $ra1} {ra <= $ra2}"]
          matchFIRST-2MASS-SDSS dummy $file2MASS $outfile $dirOut $sel
       }
       genericChainDestroy $chainFIRST
    }         

} 


# match tsObj file obtained by matching FIRST to SDSS to tsObj file obtained
# matching 2MASS to SDSS
proc matchFIRST-2MASS-SDSS {tsObjFIRST tsObj2MASS outfile {outdir .} {chain ""}} {

     # start output file for unmatched FIRST sources
     set outpath $outdir/${outfile}_FIRSTnot2MASS.par
     set outf [open $outpath "w"]
     puts $outf "# SDSS - FIRST matches not found in 2MASS"
     close $outf

     # write out SDSSFIRST structure definition
     set hdr ""
     set auxchain [chainNew SDSSFIRST]
     chain2Param $outpath $auxchain $hdr -append
     chainDestroy $auxchain genericDel

     # read FIRST-SDSS objects (if needed)  
     if {$chain == ""} {
        set chain [tsObj2chain $tsObjFIRST]
        set nFIRST [chainSize $chain]
        echo " Read $nFIRST objects from $tsObjFIRST"
        set read_chain 1
     } else {
        set nFIRST [chainSize $chain]
        echo " Processing $nFIRST objects"
        set read_chain 0
     }   

     # fields to transfer
     set transferFields {}
     lappend transferFields [list firstLambda firstLambda]   
     lappend transferFields [list firstEta firstEta]
     lappend transferFields [list firstPeak firstPeak]
     lappend transferFields [list firstInt firstInt]
     lappend transferFields [list firstRms firstRms]

     # match to 2MASS
     set match_radius 2.0
     set chains [guests2tsObj $chain $tsObj2MASS $match_radius \
                      $outfile $outdir $transferFields]
 
     # take care of FIRST sources not detected in 2MASS
     set detected [lindex $chains 0]
     set notdetected [lindex $chains 1]
     chain2Param $outpath $notdetected "" -append
     chainDel $detected; chainDel $notdetected 
     
     echo "FIRST sources not detected by 2MASS are in $outpath"
   
     if {$read_chain} {
         genericChainDestroy $chain
     } 
     echo "Done."

}

############################################################################# 
# match all sources in a *.par file (must include fields "name" "ra" and
# "dec") to a specified SDSS run. The matching is done positionally
# in a circle with radius $match_radius arcsec. The output includes an
# "tsObj"-like file and two *par files. Both *par files are subsets of the
# input par file: one contains all matched sources, and the other one sources 
# within the observed area but not detected in SDSS.
# "tsObj"-like file contains Photo outputs for matched sources. The list
# transferFields is a list of 2-element lists: the two elements are the field
# from the input par file to copy, and the field from tsObj file to copy over.
# E.g. to match a file of FIRST structures and copy some field fields:
# set transferFields {{ra firstLambda} {dec firstEta} {Fpeak firstPeak} 
#                     {Fint firstInt} {Rms firstRms}}
# It is not necessary to copy all the fields from the guest catalog. The field
# "firstMatch" in "tsObj"-like file will contain the listing number in 
# the output *par file of matched sources for the corresponding entry.
proc match_parfile2run {parfile run rerun {camCols "all"} {field1 -1} {field2 -1} 
                        {transferFields {}} {match_radius 1.0} \
            {data_dir ""} {outdir .} {chain ""}} {

global data_root openit

     if {$data_dir != ""} {
         set data_root $data_dir 
     }
     # make sure data dir is kosher
     if {![file exist $data_root]} {
        set data_root [get_data_root ""]
     }
     if {![file exist $data_root]} {
        error "Specified data_root does NOT exist: $data_root"
     }
     # take care of defaults
     if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}

     # read guests
     if {$chain == ""} {
         set chain [param2Chain $parfile ""]
         set read_chain 1
         set Nguest [chainSize $chain]
         echo "Read in $Nguest guests"
     } else {
         set Nguest [chainSize $chain]
         set read_chain 0
     }

     # root for output tsObj file name
     set out_root ${parfile}_tsObj
     # root for output par file names
     set out_det ${parfile}_detected
     set out_notdet ${parfile}_notdetected
     # start output files
     # first some aux information
     set outfD [open $out_det "w"]
     puts $outfD "# Sources from file $parfile" 
     puts $outfD "# matched within $match_radius arcsec to SDSS sources from"
     puts $outfD "# run = $run, rerun $rerun, columns $camCols"
     puts $outfD "# fields transferred to tsObj file:"
     foreach el $transferFields {
         puts $outfD "# $el"  
     }
     puts $outfD "#"
     close $outfD
     set outfN [open $out_notdet "w"]
     puts $outfN "# Sources from file $parfile" 
     puts $outfN "# NOT matched within $match_radius arcsec to SDSS sources from"
     puts $outfN "# run = $run, rerun $rerun, columns $camCols"
     puts $outfN "#"
     close $outfN
 
     # loop over camera columns
     foreach col $camCols {
        # get tsObj dir
        set_run $run $col -tsObj -rerun $rerun 
        set tsObjdir $openit(objdir)
        if {$field1 < 0 || $field2 < 0} {
            # find the field range
            # set fields [find_field_range_asTrans $openit(asdir)]
            set root tsObj-[format %06d $run]
            set fields [find_field_range $tsObjdir $root]
            set startField [lindex $fields 0]
            set endField [lindex $fields 1] 
        } else {
            set startField $field1 
            set endField $field2
        }
        echo "  working on column $col, fields $startField-$endField"        
        # get guests which are approximately within this run
        set good_guests [get_good_guests $chain $run $col \
                                 $startField $endField $data_root $rerun]
        
        # call work horse
        echo "   calling guests2run with [chainSize $good_guests] sources"
        set chains [guests2run $good_guests $run $rerun $col $startField \
             $endField $match_radius $out_root $outdir $transferFields]
 
        # take care of detected/not detected par files
        set good [lindex $chains 0]
        set bad [lindex $chains 1]
        if {$col == [lindex $camCols 0]} {
            chain2Param $out_det $good "" -append
            chain2Param $out_notdet $bad "" -append
        } else {
            chain2Param $out_det $good "" -append -notypedump
            chain2Param $out_notdet $bad "" -append -notypedump
        }
        chainDel $good; chainDel $bad; chainDel $good_guests   
        echo ""
     }  
 
     echo "Your new par files are $out_det and $out_notdet"
   
     if {$read_chain} {
         genericChainDestroy $chain
     } 
     echo "Done."
}



# given a chain of structures which contain fields ra and dec, return
# a chain of elements which satisfy RAstart <= ra <= RAend
# Decstart <= dec <= Decend, where starting and ending values of RA
# and Dec are determined for a box spanned by the specified run, col,
# startField and endField. 
proc get_good_guests {chain run col startF endF root_dir {rerun ""}} {

     # step in fields when getting bounding box  
     set nfields 50
     # edge to account for the scan curvature (degrees)
     set edge 0.05

     # chain of sources which are approximately within this run's area
     set good [chainNew [exprGet $chain.type]]

     # first get the bounding box
     set boundaries [RADecBox4run $run $col $startF $endF $nfields $root_dir $rerun]
     set RAmin [expr [lindex $boundaries 0] - $edge]
     set Decmin [expr [lindex $boundaries 1] - $edge]
     set RAmax [expr [lindex $boundaries 2] + $edge]
     set Decmax [expr [lindex $boundaries 3] + $edge]
     
     if {0} {
         echo "XXX hack in get_good_guests"
         set good [chainSearch $chain "{$Decmin <= dec <= $Decmax}"]
     } else {  
         set good [chainSearch $chain \
          "{$RAmin <= ra <= $RAmax} {$Decmin <= dec <= $Decmax}"]
     }
    
     return $good
}


# given a run, column and the field range, get the bounding RA and Dec
# and return as a list {RAstart Decmin RAend Decmax}
# n.b. if the run overlaps RA=24h, then returned RAmin > RAmax
proc RADecBox4run {run column field1 field2 {nfields 1} {rootdir ""} {rerun ""}} {
  
    set RAmin 360.0; set RAmax 0.0; 
    set Decmin 90.0; set Decmax -90.0; 

    # first check the assumption below that the Dec increases
    # with column on a frame

    if {1} {
       set runstr [format %06d $run]
       set asdir /u/dss/data/asTrans
       set asTransFile $asdir/asTrans-$runstr.fit
    } else {
       set asTransFile ""
    }

    set RADec1 [pix2eq $run $column $field1 0 0 r $rootdir $rerun 0 $asTransFile]
    set RADec2 [pix2eq $run $column $field1 0 2048 r $rootdir $rerun 0 $asTransFile]
    if {[lindex $RADec1 1] < [lindex $RADec2 1]} {
        set colSouth 0
        set colNorth 2048
    } else {
        echo "** Declination does NOT increase with column !? **"
        set colSouth 2048
        set colNorth 0
    }
    
    # loop over fields 
    set row 0
    loop f $field1 [expr $field2 + 1] $nfields {
        if {[expr $f + $nfields - 1] > $field2} {
            set f $field2 
            set row 1361
        }
        set RADecS [pix2eq $run $column $f $row $colSouth r $rootdir $rerun]
        set RADecN [pix2eq $run $column $f $row $colNorth r $rootdir $rerun]
        foreach p {$RADecS $RADecN} {
            eval set RADec $p
            set RA [lindex $RADec 0]
            set Dec [lindex $RADec 1]
            set RAmin [expr min($RAmin,$RA)]
            set RAmax [expr max($RAmax,$RA)]
            set Decmin [expr min($Decmin,$Dec)]
            set Decmax [expr max($Decmax,$Dec)]
        }  
    }

    return [list $RAmin $Decmin $RAmax $Decmax]

}




# given a run, column and the field range, get the bounding lambda and eta
# and return as a list {lambda_min eta_min lambda_max eta_max}
proc eta_lambda_4run {run column field1 field2 {rootdir ""}} {

    # starting field
    set RADec1 [pix2eq $run $column $field1 0 0 r $rootdir]
    set survey1 [eqToSurvey [lindex $RADec1 0] [lindex $RADec1 1]] 
    set lambda1 [keylget survey1 lambda]
    set eta1 [keylget survey1 eta]
    set RADec2 [pix2eq $run $column $field1 0 2048 r $rootdir]
    set survey2 [eqToSurvey [lindex $RADec2 0] [lindex $RADec2 1]] 
    set lambda2 [keylget survey2 lambda]
    set eta2 [keylget survey2 eta]  
    set lminS [expr min($lambda1,$lambda2)]
    set eminS [expr min($eta1,$eta2)]
    set lmaxS [expr max($lambda1,$lambda2)]
    set emaxS [expr max($eta1,$eta2)]

    # ending field
    set RADec1 [pix2eq $run $column $field2 1361 0 r $rootdir]
    set survey1 [eqToSurvey [lindex $RADec1 0] [lindex $RADec1 1]] 
    set lambda1 [keylget survey1 lambda]
    set eta1 [keylget survey1 eta]
    set RADec2 [pix2eq $run $column $field2 1361 2048 r $rootdir]
    set survey2 [eqToSurvey [lindex $RADec2 0] [lindex $RADec2 1]] 
    set lambda2 [keylget survey2 lambda]
    set eta2 [keylget survey2 eta]
    set lminE [expr min($lambda1,$lambda2)]
    set eminE [expr min($eta1,$eta2)]
    set lmaxE [expr max($lambda1,$lambda2)]
    set emaxE [expr max($eta1,$eta2)]

    set lmin [expr min($lminS,$lminE)]
    set lmax [expr max($lmaxS,$lmaxE)]
    set emin [expr min($eminS,$eminE)]
    set emax [expr max($emaxS,$emaxE)]

    return [list $lmin $emin $lmax $emax]

}




       

###############################################################################
#
# Make tsObj files for a chain of objects (assumed that each element has fields
# ra and dec) matched positionally to SDSS objects within a circle with radius 
# match_radius arcsec.
# For each element in list transferFields, take its first element to be a field
# in guestChain and the second element a field in tsObj file, and copy the 
# guest value to output tsObj file, e.g. to copy ra and dec from guest chain to
# fields firstLambda and firstEta:
#   set transferFields {{ra firstLambda} {dec firstEta}}
# Return a list with 2 subsets of the input chain. The first chain contains all
# matched objects, and the second one objects within the run area but not 
# detected by SDSS.
proc guests2run {guestChain run rerun col startField endField \
           match_radius outroot outdir transferFields} { 

global data_root
global objcIo
global openit

   # open my table to write; note that we read field startField only to get 
   # a template for table
   eval set_run -rootdir $data_root -tsObj $run $col -rerun $rerun 
   set table [openit $startField -noAI]
   set file_str ${outroot}_run${run}_col$col
   if ![info exists my_table] {
       set my_table [eval open_mytables table $file_str -tsObj -dir $outdir \
                                    "-noAI" ""]
       set my_table_OBJfd [keylget my_table OBJfd] 
   }
   objfileClose table

   # set pointers to the data
   set_run -rootdir $data_root -tsObj $run $col -rerun $rerun
   set tsObjdir $openit(objdir)
    
   
   # for each guest find whether its position was observed by SDSS
   # if so, dump all sources within the matching radius to my_table
   set good [chainNew [exprGet $guestChain.type]]
   set bad [chainNew [exprGet $guestChain.type]]
   set cursor [chainCursorNew $guestChain]
   set ntotSDSS 0
   set ntotGuest 0
   while {[set guest [chainWalk $guestChain $cursor]] != ""} {
      incr ntotGuest
      echo $ntotGuest
      set ra [exprGet $guest.ra]
      set dec [exprGet $guest.dec]
      set SDSSpos [where_in_run $run $ra $dec $data_root -rerun $rerun]
      if {$SDSSpos != "-1"} {
          # this source was observed in this run
          # check that the camera column and the field are OK
          set obs_col [lindex $SDSSpos 1]
          set obs_field [lindex $SDSSpos 2] 
          set obs_row [lindex $SDSSpos 3]
          if {0} {
             if {$obs_row > 1361} {
                 echo "overlap source (ra=$ra, dec=$dec): $SDSSpos"
                 incr obs_field -1
             } 
          } else {
             if {$obs_row < 64} {
                 set obs_row [expr 1361+$obs_row]
                 set obs_field [expr $obs_field -1]
             }
          }
          if {$obs_col == $col && $obs_field >= $startField && $obs_field <= $endField} {
              # this is a good source  
              set tsObj [get_tsObj_name $tsObjdir $run $rerun $col $obs_field 0]
              set Nfound [dump_matches $guest $transferFields $match_radius $tsObj \
                                       $my_table_OBJfd] 
              incr ntotSDSS $Nfound
              if {$Nfound > 0} {
                  chainElementAddByPos $good $guest TAIL AFTER
                  echo "   detected"
              } else {
                  chainElementAddByPos $bad $guest TAIL AFTER
                  echo "   not detected"
              }
          }
      } else {
         echo "      off frame"
      }
   } 
   chainCursorDel $guestChain $cursor
   
   # sanity check
   assert { $ntotGuest == [chainSize $guestChain] }
   set ntotGood [chainSize $good]
   set ntotBad [chainSize $bad]
   set ntotGuestObs [expr $ntotGood + $ntotBad]
  

   # Finally, close my_table   
   close_mytables my_table
   echo "Run $run, Col $col, startField=$startField, endField=$endField"
   echo "  processed $ntotGuest sources: $ntotGuestObs are within the specified area "
   echo "  matched [chainSize $good], not detected [chainSize $bad]" 
   echo "Your data is in tsObj-$file_str.fit"

   return [list $good $bad]
}



###############################################################################
# Make tsObj files for a chain of objects (assumed that each element has fields
# ra and dec) matched positionally to SDSS objects within a circle with radius 
# match_radius arcsec, and given in a tsObj file tsObjHost.
# For each element in list transferFields, take its first element to be a field
# in guestChain and the second element a field in tsObjHost file, and copy the 
# guest value to output tsObj file, e.g. to copy ra and dec from guest chain to
# fields firstLambda and firstEta:
#   set transferFields {{ra firstLambda} {dec firstEta}}
# Return a list with 2 subsets of the input chain. The first chain contains all
# matched objects, and the second one objects within the run area but not 
# detected by SDSS.
proc guests2tsObj {guestChain tsObjHost match_radius \
                   outname outdir transferFields} { 

global objcIo
global openit

   # open my table to write; note that we read tsObjHost only to get 
   # a template for table
   set table [objfileOpen $tsObjHost "" 1 "" ""]
   if ![info exists my_table] {
       set my_table [eval open_mytables table $outname -tsObj -dir $outdir \
                                    "-noAI" ""]
       set my_table_OBJfd [keylget my_table OBJfd] 
   }
   objfileClose table

   # read hosts
   set hostChain [tsObj2chain $tsObjHost]
   set nHosts [chainSize $hostChain]  
   echo " Read $nHosts objects from $tsObjHost"
        

 
   # for each guest find whether its position was observed by SDSS
   # if so, dump all sources within the matching radius to my_table
   set good [chainNew [exprGet $guestChain.type]]
   set bad [chainNew [exprGet $guestChain.type]]
   set cursor [chainCursorNew $guestChain]
   set ntotIn [chainSize $guestChain]
   set ntotGuest 0
   while {[set guest [chainWalk $guestChain $cursor]] != ""} {
      incr ntotGuest
      echo $ntotGuest
      set ra [exprGet $guest.ra]
      set dec [exprGet $guest.dec]
      set Nfound [dump_triple_matches $guest $transferFields $match_radius \
                                      $hostChain $my_table_OBJfd] 
      if {$Nfound > 0} {
          chainElementAddByPos $good $guest TAIL AFTER
          echo "   detected"
      } else {
          chainElementAddByPos $bad $guest TAIL AFTER
          echo "   not detected"
      }
   } 
   chainCursorDel $guestChain $cursor
   
   # sanity check
   assert { $ntotGuest == $ntotIn }
   set ntotGood [chainSize $good]
   set ntotBad [chainSize $bad]
   assert { $ntotGuest == [expr $ntotGood + $ntotBad] }
  

   # Finally, close my_table   
   close_mytables my_table
   echo "  guests2tsObj: processed $ntotGuest sources and"
   echo "  matched $ntotGood, not detected $ntotBad" 
   echo "  Your data is in $outdir/$outname"

   # delete hosts
   genericChainDestroy $hostChain

   return [list $good $bad]
}



# given a position (ra, dec), matching radius in arcsec, path to a tsObj
# file and an open table, dump all sources within matching radius from the
# specified position and return the number of matched sources
proc dump_matches {guest transferFields match_radius tsObj table_OBJfd} { 

     # matching radius in degrees
     set matchRad_deg [expr $match_radius/3600.0]     

     # work in survey coordinates to avoid expensive spherical 
     # geometry calculations    
     set ra [exprGet $guest.ra]; set dec [exprGet $guest.dec] 
   if {0} { 
     set survey_coord [eqToSurvey $ra $dec] 
     set lambda [keylget survey_coord lambda]  
     set eta [keylget survey_coord eta] 
   } else {
     ### FIX THIS xxx
     # temporary use ra and dec for matching (good for equatorial strip)
     set lambda $ra
     set eta $dec    
   }  

     ### read tsObj file
     if [catch {
          set chain [tsObj2chain $tsObj]
     } msg] { 
          echo " problem reading file $tsObj:"
          echo " $msg"
          return 0
     }

     # first get all sources within a square with side 2*matchRad_deg
   if {0} {
     # problem with (lambda, eta) in tsObj files
     set etamin [expr $eta - $matchRad_deg]
     set etamax [expr $eta + $matchRad_deg]
     set lmin [expr $lambda - $matchRad_deg]
     set lmax [expr $lambda + $matchRad_deg]
     set cond "{$etamin < eta < $etamax} {$lmin < lambda < $lmax}"
   } else {
     set RAmin [expr $ra - $matchRad_deg]
     set RAmax [expr $ra + $matchRad_deg]
     set Decmin [expr $dec - $matchRad_deg]
     set Decmax [expr $dec + $matchRad_deg]
     set cond "{$RAmin < ra < $RAmax} {$Decmin < dec < $Decmax}"     
   }

     set boxchain [chainSearch $chain $cond]
     
     # loop over all sources inside the box and dump good ones
     # i.e. this is going from a match inside a square to match within a circle
     # number of matched sources
     set Ngood 0
     set cursor [chainCursorNew $boxchain]
     while {[set source [chainWalk $boxchain $cursor]] != ""} { 
    if {[is_match $source $matchRad_deg $lambda $eta]} {
            # tranfer needed fields
            foreach el $transferFields {
        set fieldIn [lindex $el 0]
        set fieldOut [lindex $el 1]
                # special case for 2MASS characters
                if {[lsearch {rd_flg bl_flg cc_flg} $fieldIn] > -1} { 
                   set aux [exprGet $guest.$fieldIn]
           if {$fieldIn == "cc_flg"} {
                       set aux [cc_flg_translate2int $aux]
                   }
           handleSet $source.$fieldOut [expr [strip_zero $aux]]                  
                } else {
                   handleSet $source.$fieldOut [exprGet $guest.$fieldIn]
                }
            }
            fitsBinTblRowWrite $table_OBJfd $source
            incr Ngood
        } 
     }

     # clean  
     chainCursorDel $boxchain $cursor  
     chainDel $boxchain
     genericChainDestroy $chain

     return $Ngood

}


# given a position (ra, dec), matching radius in arcsec, path to a tsObj
# file and an open table, dump all sources within matching radius from the
# specified position and return the number of matched sources
proc dump_triple_matches {guest transferFields match_radius tsObjChain table_OBJfd} { 

     # matching radius in degrees
     set matchRad_deg [expr $match_radius/3600.0]     

     # work in survey coordinates to avoid expensive spherical 
     # geometry calculations  
     ### well, there is problem with (lambda, eta) in tsObj files  
     # so for now use ra and dec (good for equatorial strip) XXX FIX THIS
     set ra [exprGet $guest.ra]; set dec [exprGet $guest.dec] 
     set lambda $ra
     set eta $dec    

     # first get all sources within a square with side 2*matchRad_deg
     if {0} {
       # problem with (lambda, eta) in tsObj files
       set etamin [expr $eta - $matchRad_deg]
       set etamax [expr $eta + $matchRad_deg]
       set lmin [expr $lambda - $matchRad_deg]
       set lmax [expr $lambda + $matchRad_deg]
       set cond "{$etamin < eta < $etamax} {$lmin < lambda < $lmax}"
     } else {
       set RAmin [expr $ra - $matchRad_deg]
       set RAmax [expr $ra + $matchRad_deg]
       set Decmin [expr $dec - $matchRad_deg]
       set Decmax [expr $dec + $matchRad_deg]
       set cond "{$RAmin < ra < $RAmax} {$Decmin < dec < $Decmax}" 
     }

     # this gets all objects from tsObjChain in a square around this guest
     set boxchain [chainSearch $tsObjChain $cond]
     
     # loop over all sources inside the box and dump good ones
     # i.e. this is going from a match inside a square to match within a circle
     # number of matched sources
     set Ngood 0
     set cursor [chainCursorNew $boxchain]
     while {[set source [chainWalk $boxchain $cursor]] != ""} { 
    if {[is_match $source $matchRad_deg $lambda $eta]} {
            # tranfer needed fields
            foreach el $transferFields {
        set fieldIn [lindex $el 0]
        set fieldOut [lindex $el 1]
                # special case for 2MASS characters
                if {[lsearch {rd_flg bl_flg cc_flg} $fieldIn] > -1} { 
                   set aux [exprGet $guest.$fieldIn]
           if {$fieldIn == "cc_flg"} {
                       set aux [cc_flg_translate2int $aux]
                   }
           handleSet $source.$fieldOut [expr [strip_zero $aux]]                  
                } else {
                   handleSet $source.$fieldOut [exprGet $guest.$fieldIn]
                }
            }
            fitsBinTblRowWrite $table_OBJfd $source
            incr Ngood
        } 
     }

     # clean  
     chainCursorDel $boxchain $cursor  
     chainDel $boxchain

     return $Ngood

}


# aux proc to strip leading zeros, except the last one
proc strip_zero {x} { 
 
    set value [string trimleft $x 0]
    if {$value == ""} {set value 0}

    return $value

}

# we need an interger value to stuff in tsObj files, not char
proc cc_flg_translate2int {cc {Ndigits 3}} {
   
    set f0 0; set f1 0; set f2 0
    loop i 0 $Ndigits {
      set flag [string range $cc $i $i]
      if {$flag == "0"} {set f$i 0}
      if {$flag == "U"} {set f$i 1}
      if {$flag == "N"} {set f$i 2}
      if {$flag == "P"} {set f$i 3}
      if {$flag == "D"} {set f$i 4}
      if {$flag == "S"} {set f$i 5}
      if {$flag == "R"} {set f$i 6}
      if {$flag == "C"} {set f$i 7}
      if {$flag == "B"} {set f$i 8}
    }
    return "${f0}${f1}${f2}"
}


# If given object is within a matching radius match_rad (in degrees !) 
# from a requested position (lambda0, eta0) return 1, 0 otherwise
proc is_match {obj match_rad lambda0 eta0} {
    
   ### FIX THIS xxx
   # temporary match within a square, can cut later to circle
   return 1
    
    set lambda [exprGet $obj.lambda]; set eta [exprGet $obj.eta];
    if {0} {
       # hack due to bad (lambda,eta) in tsObj files
       set survey_coord [eqToSurvey [exprGet $obj.ra] [exprGet $obj.dec]] 
       set lambda [keylget survey_coord lambda]  
       set eta [keylget survey_coord eta] 
    } else {
       #### FIX THIS xxx
       # for eq. strip can do ra and dec, assumes that RA and Dec are
       # passed as lambda0 and eta0
       set lambda [exprGet $obj.ra] 
       set eta [exprGet $obj.dec]
    }
 
    set delLambda [expr abs($lambda-$lambda0)]
    set delEta [expr abs($eta-$eta0)]

    # first a quick check
    if {$delLambda > $match_rad || $delEta > $match_rad} {
    return 0
    } else { 
        set dist [expr sqrt($delLambda*$delLambda+$delEta*$delEta)]
        if {$dist > $match_rad} {
        return 0
        } else { 
        return 1
        }     
    }

}



# Another simple proc for selecting object with a specified id set as
# a global variable requestedID 
# Can be used with e.g. myObj2tsObj
proc sel_id {obj {b 2}} {
   global requestedID
    
   if {[exprGet $obj.id] == $requestedID} {
      return 1
   } else {
      return 0
   }
}





# take FIRST*.par file and an unmatched*par file and 
# extract ra, dec, Fint, Fpeak data
proc get_unmatched {FIRSTfile unmatchedFile output} {
 
     set out [open $output w]
 
     puts $out "# unmatched from $unmatchedFile"
     puts $out "# data from $FIRSTfile"
     puts $out "#  id        ra            dec     Fint  Fpeak"   
 
     set hdr [hdrNew]
     set unmatched [param2Chain $unmatchedFile $hdr]
     set FIRSTchain [param2Chain $FIRSTfile $hdr]
 
     loop i 0 [chainSize $unmatched] {
        set unm [chainElementGetByPos $unmatched $i]
        set id [exprGet $unm.id]
        if {$id} {                            
            set data [chainElementGetByPos $FIRSTchain $id]
            set ra [exprGet $data.ra]
            set dec [exprGet $data.dec]
            assert {$ra == [exprGet $unm.ra]}       
            set fint [exprGet $data.Fint]
            set fpeak [exprGet $data.Fpeak]
            puts $out  "  $id  $ra $dec  $fint    $fpeak"
        }
     }

     close $out

}


### select sources in flags-flux-color space 
# e.g. set selected [clean_tsObj_chain $chain z 14.0 21.0 i z 1.5 2.0
#                      0.2 0.3 {} [list SATUR BLENDED EDGE BRIGHT]
# search for red sources
proc clean_tsObj_chain {chain ifmag im1 im2 if1 if2 ic1 ic2 \
                iferr icerr flagsON flagsOFF \
            {objc_type ""} {itype psf} } {

global m1 m2 f1 f2 fmag c1 c2 type band ferr cerr
global data_root ignore_primary openit
 
     # parameters for proc sel_colormagerr
     set fmag $ifmag; set m1 $im1; set m2 $im2; set type $itype     
     set f1 $if1; set f2 $if2; set c1 $ic1; set c2 $ic2;      
     set ferr $iferr; set cerr $icerr;


     # we want all objects, not only primary
     set ignore_primary 1
     set good [chainSearchByFlags $chain $flagsON $flagsOFF sel_colormag]
     #echo "    found [chainSize $good] clean elements out of [chainSize $chain]"

     # select desired type
     if {$objc_type != ""} {
         set aux $good
         set good [chainSearch $aux "{objc_type == $objc_type}"]
         #echo "selected [chainSize $good] type=$objc_type elements out of [chainSize $aux]"
         chainDel $aux
     }

     return $good
}



# structure which holds SDSS and 2MASS PSC photometry for a selected source
typedef struct {
    int localID;
    int run;
    int camCol;
    int field;
    int id;
    float objc_rowc;
    float objc_colc;
    double ra;
    double dec;
    int objc_flags;
    int objc_flags2;
    int objc_type;
    int utype;
    int gtype;
    int rtype;
    int itype;
    int ztype;
    float u;
    float uErr;
    float g;
    float gErr;
    float r;
    float rErr;
    float i;
    float iErr;
    float z;
    float zErr;
    float umod;
    float umodErr;
    float gmod;
    float gmodErr;
    float rmod;
    float rmodErr;
    float imod;
    float imodErr;
    float zmod;
    float zmodErr;
    float uPet;
    float uPetErr;
    float gPet;
    float gPetErr;
    float rPet;
    float rPetErr;
    float iPet;
    float iPetErr;
    float zPet;
    float zPetErr;
    float uR50;
    float uR50Err;
    float gR50;
    float gR50Err;
    float rR50;
    float rR50Err;
    float iR50;
    float iR50Err;
    float zR50;
    float zR50Err;
    float uRedd;
    float gRedd;
    float rRedd;
    float iRedd;
    float zRedd;
    double ra2MASS; 
    double dec2MASS; 
    float J;
    float Jerr;
    float H;
    float Herr;
    float K;
    float Kerr;
    float rd_flg;
    float bl_flg;
    char  cc_flg[4];
    float extd_flg;
    float mp_flg;
} SDSS2MASS;


# structure which holds SDSS and 2MASS XSC photometry for a selected source
typedef struct {
    int localID;
    int run;
    int camCol;
    int field;
    int id;
    float objc_rowc;
    float objc_colc;
    double ra;
    double dec;
    int objc_flags;
    int objc_flags2;
    int objc_type;
    int utype;
    int gtype;
    int rtype;
    int itype;
    int ztype;
    float u;
    float uErr;
    float g;
    float gErr;
    float r;
    float rErr;
    float i;
    float iErr;
    float z;
    float zErr;
    float umod;
    float umodErr;
    float gmod;
    float gmodErr;
    float rmod;
    float rmodErr;
    float imod;
    float imodErr;
    float zmod;
    float zmodErr;
    float uPet;
    float uPetErr;
    float gPet;
    float gPetErr;
    float rPet;
    float rPetErr;
    float iPet;
    float iPetErr;
    float zPet;
    float zPetErr;
    float uR50;
    float uR50Err;
    float gR50;
    float gR50Err;
    float rR50;
    float rR50Err;
    float iR50;
    float iR50Err;
    float zR50;
    float zR50Err;
    float uisoa;
    float uisoaErr;
    float uisob;
    float uisobErr;
    float uisophi;
    float uisophiErr;
    float gisoa;
    float gisoaErr;
    float gisob;
    float gisobErr;
    float gisophi;
    float gisophiErr;
    float risoa;
    float risoaErr;
    float risob;
    float risobErr;
    float risophi;
    float risophiErr;
    float uRedd;
    float gRedd;
    float rRedd;
    float iRedd;
    float zRedd;
    double ra2MASS; 
    double dec2MASS;
    float r_3sig;
    float j_ba;
    float k_ba;
    float k_phi;
    float J;
    float Jerr;
    float H;
    float Herr;
    float K;
    float Kerr;
    float Jflg;
    float Hflg;
    float Kflg;
    char  cc_flg[4];
    float escore;
    float gscore;
    float BR;
    float RK;
} SDSS2MASSXSC;



# structure which holds SDSS-2MASS-FIRST data 
typedef struct {
    int localID;
    int run;
    int camCol;
    int field;
    int id;
    float objc_rowc;
    float objc_colc;
    double ra;
    double dec;
    int objc_flags;
    int objc_flags2;
    int objc_type;
    int utype;
    int gtype;
    int rtype;
    int itype;
    int ztype;
    float u;
    float uErr;
    float g;
    float gErr;
    float r;
    float rErr;
    float i;
    float iErr;
    float z;
    float zErr;
    float umod;
    float umodErr;
    float gmod;
    float gmodErr;
    float rmod;
    float rmodErr;
    float imod;
    float imodErr;
    float zmod;
    float zmodErr;
    float uPet;
    float uPetErr;
    float gPet;
    float gPetErr;
    float rPet;
    float rPetErr;
    float iPet;
    float iPetErr;
    float zPet;
    float zPetErr;
    float uR50;
    float uR50Err;
    float gR50;
    float gR50Err;
    float rR50;
    float rR50Err;
    float iR50;
    float iR50Err;
    float zR50;
    float zR50Err;
    float uRedd;
    float gRedd;
    float rRedd;
    float iRedd;
    float zRedd;
    double ra2MASS; 
    double dec2MASS; 
    float J;
    float Jerr;
    float H;
    float Herr;
    float K;
    float Kerr;
    float rd_flg;
    float bl_flg;
    char  cc_flg[4];
    float extd_flg;
    float mp_flg;
    double raFIRST; 
    double decFIRST; 
    float Fpeak;
    float Fint;
    float Rms;
} SDSS2MASSFIRST;


  
# For a specified list of tsObj files, dump SDSS2MASS structure to 
# a *.par file (or SDSS2MASSFIRST for FIRSTmatch = 1).
proc dump_SDSS2MASS {tsObjList outfile {FIRSTmatch 0} {primary 1} {XSC 0}} {

    # for  memory reporting
    set startMem [memNextSerialNumber]

    # start output file
    set outf [open $outfile "w"]
    # write out structure definition
    set hdr ""
    if {!$FIRSTmatch} {
       if {!$XSC} {
           puts $outf "# SDSS - 2MASS PSC matches (ZI)"
           set chain [chainNew SDSS2MASS]
       } else {
           puts $outf "# SDSS - 2MASS XSC matches (ZI)"
           set chain [chainNew SDSS2MASSXSC]
       }
    } else {
       puts $outf "# SDSS-2MASS-FIRST matches (ZI)"
       set chain [chainNew SDSS2MASSFIRST]
    }
    close $outf
    chain2Param $outfile $chain $hdr -append
    chainDestroy $chain genericDel

    set nfiles [llength $tsObjList]
    echo ""; echo "  --  DUMPING $nfiles files  --"
    if {$primary} {
        echo " file       N all    N prim    N good" 
    } else {
        echo " file       N all    N good" 
    }
    set Ntot 0  

    # loop over files     
    loop i 0 $nfiles {
   
        set file [lindex $tsObjList $i]
        # read chain from this tsObj file
        set chain [tsObj2chain $file]
        set Na [format "%8d" [chainSize $chain]]

        # dump data to output file
        if {$primary} {
           # we want only primary elements           
           set prim [get_primaries $chain] 
           set Np [format "%8d" [chainSize $prim]]
           # this loops over all of the matched elements and dumps them
           # to the output par file. The return value is the number of good elements.
           set Ng [format "%8d" [dumpSDSS2MASSmatches $prim $outfile $Ntot $FIRSTmatch $XSC]]
           echo " [format "%4d" $i]    $Na  $Np  $Ng"
           chainDel $prim
        } else {
           # no selection on primary
           set Ng [format "%8d" [dumpSDSS2MASSmatches $chain $outfile $Ntot $FIRSTmatch $XSC]]
           echo " [format "%4d" $i]    $Na  $Ng"
        }
        incr Ntot $Ng        
        # clean          
        genericChainDestroy $chain
    }

    set Ntot [format "%8d" $Ntot]   
    echo " Total:   $Ntot"
    echo " ------------------------------------------------"

    # check for memory leaks
    assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}

}


# given a chain of SDSS-2MASS matches, populate a chain of SDSS2MASS structures
# and dump them (append) to outfile. The local counter localID stars from id0.
# Return the number of sources.
proc dumpSDSS2MASSmatches {chain outfile id0 {FIRSTmatch 0} {XSC 0}} {

     # which fields do we want copied over?     
     if {!$FIRSTmatch} {        
        if {!$XSC} { 
           # as defined in SDSS2MASS
           set Pfieldsaux [schemaGetFromType SDSS2MASS]
        } else {
           # as defined in SDSS2MASSXSC
           set Pfieldsaux [schemaGetFromType SDSS2MASSXSC]
        }
     } else {
        # as defined in SDSS2MASSFIRST
        set Pfieldsaux [schemaGetFromType SDSS2MASSFIRST]
     }
     set Pfields {}
     foreach el $Pfieldsaux {   
        set el [lindex $el 0]
        lappend Pfields $el
     }

     # fields from TSOBJ
     set Tfieldsaux [schemaGetFromType TSOBJ]
     set Tfields {}
     foreach el $Tfieldsaux {   
        set el [lindex $el 0]
        lappend Tfields $el
     }

     set psfmags "u uErr g gErr r rErr i iErr z zErr"
     set Tpsfmags "psfCounts<0> psfCountsErr<0> psfCounts<1> psfCountsErr<1> psfCounts<2> psfCountsErr<2> \
                   psfCounts<3> psfCountsErr<3> psfCounts<4> psfCountsErr<4>"
     set modmags "umod umodErr gmod gmodErr rmod rmodErr imod imodErr zmod zmodErr" 
     set Tmodmags "counts_model<0> counts_modelErr<0> counts_model<1> counts_modelErr<1> counts_model<2> \
                   counts_modelErr<2> counts_model<3> counts_modelErr<3> counts_model<4> counts_modelErr<4>"
     set petmags "uPet uPetErr gPet gPetErr rPet rPetErr iPet iPetErr zPet zPetErr"
     set Tpetmags "petroCounts<0> petroCountsErr<0> petroCounts<1> petroCountsErr<1> petroCounts<2> \
                   petroCountsErr<2> petroCounts<3> petroCountsErr<3> petroCounts<4> petroCountsErr<4>" 
     set r50  "uR50 uR50Err gR50 gR50Err rR50 rR50Err iR50 iR50Err zR50 zR50Err"
     set Tr50  "petroR50<0> petroR50Err<0> petroR50<1> petroR50Err<1> petroR50<2> petroR50Err<2> \
                petroR50<3> petroR50Err<3> petroR50<4> petroR50Err<4>"
     set redd "uRedd gRedd rRedd iRedd zRedd"
     set Tredd "reddening<0> reddening<1> reddening<2> reddening<3> reddening<4>"
     set SGtypes "utype gtype rtype itype ztype"
     set TSGtypes "type<0> type<1> type<2> type<3> type<4>"
     set iso "uisoa uisoaErr uisob uisobErr uisophi uisophiErr"
     set iso "$iso gisoa gisoaErr gisob gisobErr gisophi gisophiErr"
     set iso "$iso risoa risoaErr risob risobErr risophi risophiErr"
     set Tiso "iso_a<0> iso_aErr<0> iso_b<0> iso_bErr<0> iso_phi<0> iso_phiErr<0>"
     set Tiso "$Tiso iso_a<1> iso_aErr<1> iso_b<1> iso_bErr<1> iso_phi<1> iso_phiErr<1>"
     set Tiso "$Tiso iso_a<2> iso_aErr<2> iso_b<2> iso_bErr<2> iso_phi<2> iso_phiErr<2>"
 

     if {$FIRSTmatch} { 
        # this has to be the same as in matchFIRST, i.e. it's very volatile
        set FIRSTfields  "raFIRST decFIRST Fpeak Fint Rms \
                       Maj Min PA fMaj fMin fPA"
        set TFIRSTfields "firstLambda firstEta firstPeak firstInt firstRms \
                       fiberCounts<3> fiberCountsErr<3> texture<3> fiberCounts<4> fiberCountsErr<4> texture<4>"
        set Newfields "$Newfields $FIRSTfields"
        set Newtransfer "$Newtransfer $TFIRSTfields"
        set newchain [chainNew SDSS2MASSFIRST]
     } else {
        # this has to be the same as in match2MASS, i.e. it's very volatile
        if {!$XSC} {
           set 2MASSfields "ra2MASS dec2MASS J H K Jerr Herr Kerr rd_flg bl_flg cc_flg extd_flg mp_flg"
           set T2MASSfields "lambda eta fiberCounts<0> fiberCounts<1> fiberCounts<2> fiberCountsErr<0> \
                fiberCountsErr<1> fiberCountsErr<2> fracPSF<0> fracPSF<1> fracPSF<2> fracPSF<3> fracPSF<4>"
           set newchain [chainNew SDSS2MASS]
           # all together
           set Newfields   "$psfmags $modmags $petmags $r50 $redd $SGtypes $2MASSfields"
           set Newtransfer "$Tpsfmags $Tmodmags $Tpetmags $Tr50 $Tredd $TSGtypes $T2MASSfields"
        } else {
           set 2MASSfields "ra2MASS dec2MASS r_3sig j_ba k_ba k_phi J H K Jerr Herr Kerr Jflg \
                         Hflg Kflg cc_flg escore gscore BR RK"
           set T2MASSfields "lambda eta texture<0> texture<1> texture<2> texture<3> fiberCounts<0> \
                          fiberCounts<1> fiberCounts<2> fiberCountsErr<0> fiberCountsErr<1> \
                          fiberCountsErr<2> fracPSF<0> fracPSF<1> fracPSF<2> fracPSF<3> texture<4> \
                          fracPSF<4> fiberCounts<3> fiberCounts<4>"
           set newchain [chainNew SDSS2MASSXSC]
           # all together
           set Newfields   "$psfmags $modmags $petmags $r50 $iso $redd $SGtypes $2MASSfields"
           set Newtransfer "$Tpsfmags $Tmodmags $Tpetmags $Tr50 $Tiso $Tredd $TSGtypes $T2MASSfields"
        }
     }

     assert {[llength $Newfields] == [llength $Newtransfer]}
     set cursor [chainCursorNew $chain]
     set N 0
     while {[set source [chainWalk $chain $cursor]] != ""} { 
        incr N
        if {$FIRSTmatch} { 
           set new [genericNew SDSS2MASSFIRST]
        } else {
           if {!$XSC} {
              set new [genericNew SDSS2MASS]
           } else {
              set new [genericNew SDSS2MASSXSC]
           }
        }
        # first set overall counter
        handleSet $new.localID [expr $id0 + $N] 
        # copy over all the fields that exist
        foreach el $Pfields {
            set record $el
            if {[lsearch $Tfields $record] >= 0} {
                # this field has the same name in tsObj files
                set aux [readSTRUCT TSOBJ $source 0 $record]
                # special case for 2MASS cc_flg (char -> float -> char)
                if {$record == "cc_flg"} {
                   set aux [cc_flg_translate2original $aux]
                } 
                handleSet $new.$el $aux
            } else { 
                # this field does not exist in tsObj file
                set nNew [lsearch $Newfields $record]
                if {$nNew >= 0} {
                    set transferField [lindex $Newtransfer $nNew]
                    set aux [readSTRUCT TSOBJ $source 0 $transferField]
                    if {$record == "cc_flg"} {
                       set aux [cc_flg_translate2original $aux]
                    } 
                    handleSet $new.$el $aux
                }
            }
        } 
        # all set, add it to the chain
        chainElementAddByPos $newchain $new
     }
       
     # dump the chain of good matches to the output file
     set Ngood [chainSize $newchain]
     chain2Param $outfile $newchain "" -append -notypedump

     # clean up   
     chainCursorDel $chain $cursor
     chainDestroy $newchain genericDel
   
     return $Ngood
} 


# we'd like original char value of 2MASS flag cc_flg
proc cc_flg_translate2original {cc} {
   
    set f0 0; set f1 0; set f2 0
    loop i 0 3 {
      set flag [string range $cc $i $i]
      if {$flag == "0"} {set f$i "0"}
      if {$flag == "1"} {set f$i "U"}
      if {$flag == "2"} {set f$i "N"}
      if {$flag == "3"} {set f$i "P"}
      if {$flag == "4"} {set f$i "D"}
      if {$flag == "5"} {set f$i "S"}
      if {$flag == "6"} {set f$i "R"}
      if {$flag == "7"} {set f$i "C"}
      if {$flag == "8"} {set f$i "B"}
    }
    return "${f0}${f1}${f2}"
}




# this proc searches *.par files with SDSS2MASS structure
# obtained by proc dump_SDSS2MASS. The search can be on RA range, magnitude 
# range, and color boxes. 
# Conditions are specified as 
# - mag_range: magnitude range for selected sources given as list
#      {band m_min m_max}, band is specified by its name {u g r i z J H K}
# - color_ranges: required color ranges, specified as list {col_min 
#      col_max ...}, where colors are ordered as 
#      u-g, g-r, r-i, i-z, z-J, J-K, r-K 
# - RA range: RAmin and RAmax (wraps around 360)
# - Nlinemax: only for testing purposes, max. no. of line to process
# - magnitude type can be either "psf" or "model"

#
# E.g.
# search for T dwarfs:
#   selectSDSS2MASS $infile $infile.Tdwarfs {z 14.0 21.0} \
#                    {-10 10 -10 10 -10 10 3.0 10 -10 10 -0.5 0.5 -10 20} 
# RA search:
# selectSDSS2MASS $inf $outf {r 14.0 24.0}  \
#       {-10 10 -10 10 -10 10 -10 10 -10 10 -10 10 -10 10} $RAMin $RAmax 
#
proc selectSDSS2MASS {infile outfile {mag_range {r 0 100}}   \
        {color_ranges {-10 10 -10 10 -10 10 -10 10 -10 10 -10 10 -10 10}} \
    {RAmin 0.0} {RAmax 360.0} {Nlinemax 10000000} {magType "psf"} } {

  global verbose
  global reddening

       set verbose 1
       set Necho 100000

       set bands {u g r i z} 
       set colors {"u-g" "g-r" "r-i" "i-z" "z-J" "J-K" "r-K"}

       ### sanity check on input 

       # magnitude range 
       set mag_band [lindex $mag_range 0]
       set mag_index [lsearch {u g r i z J H K} $mag_band]
       if {$mag_index < 0} {
           error "cannot recognize magnitude band $mag_band"
       }
       set mmin [lindex $mag_range 1]
       set mmax [lindex $mag_range 2]
       # color range
       loop i 0 7 {
          set col_min($i) [lindex $color_ranges [expr 2*$i]]
          set col_max($i) [lindex $color_ranges [expr 2*$i+1]]
       }

       ### open files
       set inf [open $infile r]
       set outf [open $outfile w] 

       # to avoid using param2Chain, copy SDSS2MASS structure (and any
       # other comments) from input file to the output file 
       set structOK 0
       set headerOK 0
       set Nheader 0
       while {$headerOK != 1} {
          set line [gets $inf]
          if {[eof $inf]} {  
              error "Reached the end of file without finding SDSS2MASS data"
          }
          set aux [lindex $line 1]
      if {$aux == "SDSS2MASS;" || $aux == "SDSS2MASSFIRST;" || $aux == "SDSSFIRST;"} {
             set structOK 1
      }
          set aux [lindex $line 0]
      if {$aux == "SDSS2MASS" || $aux == "SDSS2MASSFIRST" || $aux == "SDSSFIRST"} {
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

             # SDSS2MASS structure carries the following 5-element lists magnitudes
             #   index    quantity
             #    18      psfCounts(u)
             #    19      psfCountsErr(u)
             #   ...         
             #    28      counts_model(u)
             #    29      counts_modelErr(u)
             #   ...
             #    38      petroCounts(u)
             #    39      petroCountsErr(u)
             #   ...
             #    58      reddening
             #   ...
             #    65      J;
             #    67      H;
             #    69      K;

             # here we choose psf mags
             if {$magType == "psf"} {
                 set index0 18
             } else {
                 set index0 28
             }
             # reddening
             set redd_index0 58
             loop j 0 5 {               
                # reddening correction
                set redd_index [expr $redd_index0 + $j]
                if {[info exists reddening] && $reddening == 0} {
                     set rc1 0.0
                } else {
                     set rc1 [lindex $line $redd_index] 
                }
                # magnitudes and errors
                set index [expr $index0 + 2*$j]
                set maux1 [lindex $line $index]
                set eaux1 [lindex $line [expr $index + 1]]
                set m1($j) [format "%5.2f" [expr $maux1 - $rc1]] 
                set e1($j) [format "%5.2f" $eaux1] 
                # aux quantities
                set mean_mag($j) $m1($j)
                set err($j) [expr 0.001 + $e1($j)] 
             }  
             catch {
                set J [format "%5.2f" [lindex $line 65]]
                set H [format "%5.2f" [lindex $line 67]]
                set K [format "%5.2f" [lindex $line 69]]
                # include reddening correction for 2MASS
                set redd_index [expr $redd_index0 + 2]
                if {[info exists reddening] && $reddening == 0} {
                    set rc 0.0
                } else {
                    set rc [lindex $line $redd_index]
                }
                set mean_mag(5) [format "%5.2f" [expr $J - 0.327*$rc]]
                set mean_mag(6) [format "%5.2f" [expr $H - 0.209*$rc]]
                set mean_mag(7) [format "%5.2f" [expr $K - 0.133*$rc]]
             }
     } msg] {
             if {$goodRA} {
                echo "bad line for Nline = $Nline:"
                #echo "$line"
                #echo $msg
                #error
             }
             continue
         }
             # SDSS colors
             loop j 0 4 {
                set mean_col($j) [expr $mean_mag($j) - $mean_mag([expr $j+1])]
             }
             catch {
              # OIR colors
              set mean_col(4) [expr $mean_mag(4)-$mean_mag(5)]
              set mean_col(5) [expr $mean_mag(5)-$mean_mag(7)]
              set mean_col(6) [expr $mean_mag(2)-$mean_mag(7)]
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
            loop j 0 7 { 
           catch {
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
      verb_echo 1 "   color range = $col_min(4) -- $col_max(4), $col_min(5) -- $col_max(5), \
               $col_min(6) -- $col_max(6), "
      verb_echo 1 ""
      verb_echo 1 " Found $Nline sources:"
      verb_echo 1 "       [expr $Nline - $Nbadmag] have good magnitude range"
      verb_echo 1 "       [expr $Nline - $Nbadcolor] have good color ranges"
      verb_echo 1 "   and $Ngood selected sources are written to file $outfile"
      verb_echo 1 ""

      puts $outf "# Searched file $infile with:"
      puts $outf "#   magnitude range = $mmin - $mmax in $mag_band band"
      puts $outf "#   SDSS color range = $col_min(0) -- $col_max(0), $col_min(1) -- $col_max(1), \
               $col_min(2) -- $col_max(2), $col_min(3) -- $col_max(3), "
      puts $outf "#   2MASS color range = $col_min(4) -- $col_max(4), $col_min(5) -- $col_max(5), \
               $col_min(6) -- $col_max(6), "

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



# this proc searches *.par files with SDSS2MASS structure for the 
# following condition
# A1*u + A2*g + A3*r + A4*i + A5*z + A6*J + A7*H + A8*K > C
# e.g. to select sources with u-g < 0.6
# photo> set C -0.6; set Alist {-1.0 1.0 0 0 0 0 0}
# photo> selectSDSS2MASScolors $inf $outf $C $Alist
proc selectSDSS2MASScolors {infile outfile C Alist {Nlinemax 10000000} {magType "psf"}} {

  global verbose

       set verbose 1
       set Necho 100000

       ### sanity check on input 
       if {[llength $Alist] != 8} {
           echo "You must specify 8 coefficients, not [llength $Alist]!"
           return
       }

       ### open files
       set inf [open $infile r]
       set outf [open $outfile w] 

       # to avoid using param2Chain, copy SDSS2MASS structure (and any
       # other comments) from input file to the output file 
       set structOK 0
       set headerOK 0
       set Nheader 0
       while {$headerOK != 1} {
          set line [gets $inf]
          if {[eof $inf]} {  
              error "Reached the end of file without finding SDSS2MASS data"
          }
          set aux [lindex $line 1]
          if {$aux == "SDSS2MASS;" || $aux == "SDSS2MASSFIRST;"} {
             set structOK 1
          }
          set aux [lindex $line 0]
          if {$aux == "SDSS2MASS" || $aux == "SDSS2MASSFIRST"} {
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
             catch { 
             # extract values
             # here we choose psf mags
             if {$magType == "psf"} {
                 set index0 18
             } else {
                 set index0 28
             }
             # reddening
             set redd_index0 58
             loop j 0 5 {               
                # reddening correction
                set redd_index [expr $redd_index0 + $j]
                set rc1 [lindex $line $redd_index]
                if {$j == 2} {set Ar $rc1}
                # magnitudes and errors
                set index [expr $index0 + 2*$j]
                set maux1 [lindex $line $index]
                set m1($j) [format "%5.2f" [expr $maux1 - $rc1]] 
             }  
             ### correct 2MASS data for (UKIRT) reddening
             set Jredd [expr 0.327 * $rc1]
             set Hredd [expr 0.209 * $rc1]
             set Kredd [expr 0.133 * $rc1] 
             set m1(5) [format "%5.2f" [expr [lindex $line 65] - $Jredd]]
             set m1(6) [format "%5.2f" [expr [lindex $line 67] - $Hredd]]
             set m1(7) [format "%5.2f" [expr [lindex $line 69] - $Kredd]]

            
             ### check required condition
             set good 1
             set sum 0.0
             loop j 0 8 {  
                 set A [lindex $Alist $j]            
                 set sum [expr $sum + $A * $m1($j)]
             }
             if {$sum < $C} {set good 0}

             # dump to output file
             if {$good} {
                incr Ngood
                puts $outf $line
                verb_echo 2 " ** source $Nline is GOOD (so far n=$Ngood) **"
             }
          }; # end of catching
        }; # end of processing this line
      }; # end of searching input file
      

      verb_echo 1 "Searched file $infile with C = $C"
      verb_echo 1 " and coefficients: $Alist"
      verb_echo 1 " Found $Nline sources:"
      verb_echo 1 "   and $Ngood selected sources are written to file $outfile"
      verb_echo 1 ""

      close $inf
      close $outf

}



# good stars as defined in Finlator et al, 2000
proc get_goodSDSS2MASSstars {inf} {

      # the selection is only for good STARS
      # first produce file with stars only (if not already produced)
      if {![file exists $inf.stars]} {
          set flagsOFF {BRIGHT SATUR EDGE BLENDED CHILD}
          flag_search $inf $inf.stars $flagsOFF $flagsOFF {} {} 6
      }
      catch {unlink aux2};
      catch {unlink aux3};

      # cut at z=14
      selectSDSS2MASS $inf.stars aux1 {z 14.0 23.0}
      # cut at 2MASS 10 sigma
      selectSDSS2MASS aux1 aux2 {J 0.0 15.8}          
      # cut at 2MASS 10 sigma
      selectSDSS2MASS aux2 aux3 {H 0.0 15.1}          
      # cut at 2MASS 10 sigma
      selectSDSS2MASS aux3 $inf.goodstars {K 0.0 14.3}          

}


# implement Peregrine's T Tau selection criteria:
# 1) u < 21.0
# 2) u-g < 2.50
# 3) g-r > 1.10  
# 4) r-i > 0.30
# 5) i-z > 0.25    
# 6) u-g < 1.80 + 0.50*(i-z)
# 7) g-r > 0.50 + 0.45*(u-g)
# and to ensure good 2MASS: K < 15.05 (7 sigma)     
proc findTTau {inf outf} {

  global verbose
  global reddening

      catch {unlink aux0}
      catch {unlink aux1}
      catch {unlink aux2}

      # first select sources with reliable u band fluxes
      set reddening 0
      selectSDSS2MASS $inf aux0 {u 14 21.0}
      set reddening 1

      # conditions 2-5 and K limit
      set colors {-10 2.5 1.1 10 0.3 10 0.25 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS aux0 aux1 {K 0 15.05} $colors 
      # condition 6
      set Alist [list -1.0 1.0 0 0.5 -0.5 0 0 0]; set C -1.8
      selectSDSS2MASScolors aux1 aux2 $C $Alist 1000000 psf     
      # condition 7
      set Alist [list -0.45 1.45 1.0 0 0 0 0 0]; set C 0.50
      selectSDSS2MASScolors aux2 $outf $C $Alist 1000000 psf     

}


# a wrapper around selectSDSS2MASS to find some outliers in color-color
# space from a par file of dumped SDSS2MASS records
proc findSDSS2MASSoutliers {inf} {

      # first produce file with stars only (if not already produced)
      if {![file exists $inf.stars]} {
          set flagsOFF {BRIGHT SATUR EDGE BLENDED}
          flag_search $inf $inf.stars $flagsOFF $flagsOFF {} {} 6
      }

      # reliable 2MASS photometry
      if {![file exists $inf.starsGoodOIR]} {
          catch {unlink aux1}
          catch {unlink aux2}
          # 5 sigma limits in J, H, K
          selectSDSS2MASS $inf.stars aux1 {J 0.0 16.55}
          selectSDSS2MASS aux1 aux2 {H 0.0 15.85} 
          selectSDSS2MASS aux2 $inf.starsGoodOIR {K 0.0 15.05} 
      }

      # truly impeccable stars for plotting
      if {![file exists $inf.starsSDSSperfect]} {
          set flagsOFF {CHILD INTERP CR}
          flag_search $inf.stars $inf.starsSDSSperfect $flagsOFF $flagsOFF
          flag_search $inf.starsGoodOIR $inf.starsOIRperfect $flagsOFF $flagsOFF
      }

      ### the selections below are only for good stars with 5sigma in JHK ###
         set selected_input $inf.starsGoodOIR
      #############################################################

      if {![file exists $inf.OIRred]} {
          ### IR outliers are produced with good isolated stars
          # first get stars with i-K excess (optical-IR excess)
          findOIRexcess $selected_input $iKexcess $Kmax $gmax $imax
          # and then get sources with red J-K color   
          findIRred $selected_input $JKexcess $Kmax $gmax $imax
      }

      ### now that we are to it, produce a file with galaxies
      if {![file exists $inf.galaxies]} {
          set flagsOFF {BRIGHT SATUR BLENDED}
          flag_search $inf $inf.galaxies $flagsOFF $flagsOFF {} {} 3
      }
      if {![file exists $inf.brightIRgalaxies]} {
          catch {unlink aux1}
          catch {unlink aux2}
          # 5 sigma limits in J, H, K
          selectSDSS2MASS $inf.galaxies aux1 {J 0.0 16.55}
          selectSDSS2MASS aux1 aux2 {H 0.0 15.85} 
          selectSDSS2MASS aux2 $inf.brightIRgalaxies {K 0.0 15.05} 
      }

      ##### COLOR CUTS #####

      # 1) Outliers from the stellar locus

      ### ugr cuts ### 
      ## 5 adjacent cuts: everything except the stellar locus
      # "white": u-g < 0.6 && g-r < -0.3 and 14 < u < 21.5
      set outf $inf.white 
      set whitecolors {-10 0.5 -10 -0.3 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {u 14 21.5} $whitecolors 

      # "low-z QSOs": u-g < 0.6 && -0.3 < g-r < 0.5 and  14 < u < 21.5
      set outf $inf.lowzqso
      set colors {-10 0.6 -0.3 0.5 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {u 14 21.5} $colors 

      # "blue":  g-r > 0.24+0.44*(u-g) && u-r < 1.60 && 0.5 < g-r < 1.1
      set outf $inf.ugr-blue
      catch {unlink aux}
      set bluecolors {-10 1.60 0.5 1.1 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input aux {u 14 21.5} $bluecolors 
      set Alist [list -0.44 1.44 -1.0 0 0 0 0 0]; set C 0.24
      selectSDSS2MASScolors aux $outf $C $Alist 1000000 psf     

      # "red":  g-r < -0.5+0.44*(u-g) && u-r > 0.91 && 0.1 < g-r < 1.1
      set outf $inf.ugr-red
      catch {unlink aux}
      set redcolors {0.91 10 0.1 1.1 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input aux {u 14 21.5} $redcolors 
      set Alist [list 0.44 -1.44 1.0 0 0 0 0 0]; set C 0.5
      selectSDSS2MASScolors aux $outf $C $Alist 1000000 psf     

      # A stars: 0.6 < u-g && g-r < 0.1 and 14 < u < 21.5
      set outf $inf.Astars
      set Acolors {0.6 10 -10 0.1 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {u 14 21.5} $colors 
      
     
      ### gri cuts ###
      ## 3 adjacent cuts: everything except the stellar locus
      # "blue":  g-r < 1.1 && r-i > 0.2 + 0.44*(g-r) &&  14 < g < 21.0
      set outf $inf.gri-blue
      catch {unlink aux}
      set colors {-10 10 -10 1.1 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input aux {g 14 21.0} $colors 
      set Alist [list 0 -0.44 1.44 -1.0 0 0 0 0]; set C 0.2
      selectSDSS2MASScolors aux $outf $C $Alist 1000000 psf  

      # below:  g-r < 1.6 && r-i < -0.3 + 0.44*(g-r) &&  14 < g < 21.0
      set outf $inf.gri-below
      catch {unlink aux}
      set colors {-10 10 -10 1.6 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input aux {g 14 21.0} $colors 
      set Alist [list 0 0.44 -1.44 1.0 0 0 0 0]; set C 0.3
      selectSDSS2MASScolors aux $outf $C $Alist 1000000 psf 
 
      # "red":  g-r > 1.6 &&  14 < g < 21.0
      set outf $inf.gri-red
      set colors {-10 10 -10 1.2 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {g 14 21.0} $colors 

     
      ### riz cuts ### 
      ## 2 cuts: above and below the stellar locus
      # above 
      set outf $inf.riz-above
      set Alist [list 0 0 -0.52 1.52 -1.0 0 0 0]; set C 0.2
      selectSDSS2MASScolors $selected_input $outf $C $Alist 1000000 psf     
      # below 
      set outf $inf.riz-below
      set Alist [list 0 0 0.52 -1.52 1.0 0 0 0 0 0]; set C 0.2
      selectSDSS2MASScolors $selected_input $outf $C $Alist 1000000 psf     


      # 2) misc. cuts ### 

      ### JHU dwarf: i-z >~ 4, z-J = 3.36, z=19.22+-0.05, J=15.86+-0.08 
      #              J-H = -0.19+-0.22 H-K = 0.31+-0.21 J-K = 0.12+-0.08
      # "dwarfs1": z-J > 2.5 and 0 < J < 16.55 (5 sigma in J) 
      set outf $inf.dwarfs1
      set dwarfcolors {-10 10 -10 10 -10 10 -10 10 2.5 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {J 0.0 16.55} $dwarfcolors \
                      0.0 360.0 10000000 psf

      # "dwarfs2": i-z > 1.8  and z-J > 2.0 and 0 < J < 16.55 (5 sigma in J)  
      set outf $inf.dwarfs2
      set dwarfcolors {-10 10 -10 10 -10 10 1.8 10 2.0 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {J 0.0 16.55} $dwarfcolors 


      # outliers in z-J vs. i-z diagram
      # i-z > 1.0 and z-J < 1.3 and 0 < J < 16.18 (7 sigma in J) 
      set outf $inf.izJ-LR 
      set colors {-10 10 -10 10 -10 10 1.0 10 -10 1.3 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {J 0.0 16.18} $colors \
                      0.0 360.0 10000000 psf
      # i-z < 0.8 and z-J > 1.6 and r < 18
      set outf $inf.izJ-UL 
      set colors {-10 10 -10 10 -10 10 -10 0.8 1.6 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {r 14 18} $colors \
                      0.0 360.0 10000000 psf





# STOPPED HERE
  if {0} {

      # "high-z QSOs": g-r > 1.6 && J-K > 1.1
      set outf $inf.highzqso
      set colors {-10 10 1.6 10 -10 10 -10 10 -10 10 1.1 10 -10 20}
      selectSDSS2MASS $selected_input $outf {g 14.0 22.0} $colors 

      # "C stars1": g-r > 1.6 and  14.0 < r < 21.5 and J-K > 1.1 
      set outf $inf.Cstars1 
      set Ccolors1 {-10 10 1.6 10 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {r 14.0 21.0} $Ccolors1 

      # "C stars2": g-r > 2.2 and  14.0 < r < 21.0
      catch {unlink aux1};
      selectSDSS2MASS $selected_input aux1 {r 14.0 21.0}  
      ### WRONG     
      set Alist [list 0.0 1.5 -2.5 1.0 0 0 0 0]
      set C 1.5
      set outf $inf.Cstars2 
      selectSDSS2MASScolors aux1 $outf $C $Alist

      # "red1": i-z > 2.0 and 14.0 < z < 20.5
      set outf $inf.red1
      set Redcolors {-10 10 -10 10 -10 10 2.0 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {z 14.0 20.5} $Redcolors 

      # "red2": z-J > 2.5 and 14.0 < z < 21.0
      set outf $inf.red2
      set Redcolors {-10 10 -10 10 -10 10 -10 10 2.5 10 -10 10 -10 20}
      selectSDSS2MASS $selected_input $outf {z 14.0 21.0} $Redcolors 


      # "dusty": J-K > 1.4 and 5 < J < 16.25 and 0 < K < 15.05 (5 sigma)
      set outf $inf.dusty
      catch {unlink aux1};
      set dustycolors {-10 10 -10 10 -10 10 -10 10 -10 10 1.4 10 -10 20}
      selectSDSS2MASS $selected_input aux1 {J 0.0 16.25} $dustycolors 
      selectSDSS2MASS aux1 $outf {K 0.0 15.05}

      # "top gri": r-i > 0.3 + 0.4*(g-r) && 0.2 < g-r < 1.2 
      # && g < 21.2 && r < 20 && 5 < J < 15.8 (10 sigma)  
      catch {unlink aux1}; catch {unlink aux2}; catch {unlink aux3};
      # cut on g and r magnitudes
      selectSDSS2MASS $selected_input aux1 {r 14.0 20.0}
      selectSDSS2MASS aux1 aux2 {g 14.0 21.2}
      # mags OK, now select on colors
      # top, first enforce 0.2 < g-r < 1.2
      set colors {-10 10 0.2 1.2 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS aux2 aux3 {J 5.0 15.8} $colors 
      set Alist [list 0 -0.4 1.4 -1.0 0 0 0 0]
      set C 0.3
      set outf $inf.top
      selectSDSS2MASScolors aux3 $outf $C $Alist

      # "bottom gri": r-i < -0.2 + 0.4*(g-r) && 0.0 < g-r < 1.7
      # && g < 21.2 && r < 20 && 5 < J < 15.8 (10 sigma) 
      # n.b. can recycle aux2 
      catch {unlink aux3};
      # first enforce 0.0 < g-r < 1.7
      set colors {-10 10 0.0 1.7 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS aux2 aux3 {J 5.0 15.8} $colors 
      set Alist [list 0 0.4 -1.4 1.0 0 0 0 0]
      set C 0.2
      set outf $inf.bottom
      selectSDSS2MASScolors aux2 $outf $C $Alist
      # strongest constrain on "bottom"
      set C 0.6
      selectSDSS2MASScolors $inf.bottom $inf.bottom2 $C $Alist
      
      # uv excess in u-g-r
      catch {unlink aux3}
      set colors {-10 10 0.6 1.2 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS aux2 aux3 {r 14.0 20.0} $colors 
      set Alist [list -0.55 1.55 -1.0 0 0 0 0 0]
      set C 0.2
      set outf $inf.uvexcess
      selectSDSS2MASScolors aux3 $outf $C $Alist

      # ir excess in r-i-z
      catch {unlink aux3}
      set colors {-10 10 0.5 1.2 -10 10 -10 10 -10 10 -10 10 -10 20}
      selectSDSS2MASS aux2 aux3 {r 14.0 20.0} $colors 
      set Alist [list 0 0 -0.55 1.55 -1.0 0 0 0]
      set C 0.2
      set outf $inf.irexcess
      selectSDSS2MASScolors aux3 $outf $C $Alist
   }
   
  
}



### search SDSS2MASS par files for objects that lie in the main
# stellar locus in the requested color-color diagrams
proc findLocus {inf outfile {ugr 1} {gri 1} {riz 1}} {

global reddening; # controls reddening correction in selectSDSS2MASS

      set reddening 1
      set N 1000000

      catch {exec cp $inf auxin} 

      if {$ugr} {
	  echo "BYPASSING cut in ugr locus"
      } 

      
      if {$gri} {
          catch {unlink aux1}
          catch {unlink aux2}
          set outf aux1
          ### first find 1.19 < g-r < 1.59 (i.e. +- 0.2 from the locus)
          set Alist [list 0 1 -1 0 0 0 0 0]; set C 1.19
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf 
          catch {exec cp $outf auxin} 
          set Alist [list 0 -1 1 0 0 0 0 0]; set C -1.59
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf  
          catch {exec cp $outf auxin} 
          ### now find r-i > 0.44*(g-r) - 0.3
          set Alist [list 0 -0.44 1.44 -1.0 0 0 0 0]; set C -0.3
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf  
          catch {exec cp $outf auxin} 
          ### and lastly r-i < 2.0
          set Alist [list 0 0 -1 1 0 0 0 0]; set C -2.0
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf  
          # OK, those with g-r>1.19 are in aux1
          # now find those with g-r < 1.19
          set outf aux2
          catch {unlink auxin}
          catch {exec cp $inf auxin} 
          # first g-r limits
          set Alist [list 0 1 -1 0 0 0 0 0]; set C -0.5
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf 
          catch {exec cp $outf auxin} 
          set Alist [list 0 -1 1 0 0 0 0 0]; set C -1.19
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf  
          catch {exec cp $outf auxin}
          ### and then find r-i > 0.44*(g-r) - 0.3
          set Alist [list 0 -0.44 1.44 -1.0 0 0 0 0]; set C -0.3
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf  
          catch {exec cp $outf auxin} 
          ### and then r-i < 0.44*(g-r) + 0.1
          set Alist [list 0 0.44 -1.44 1.0 0 0 0 0]; set C -0.1
          selectSDSS2MASScolors auxin $outf $C $Alist $N psf  
          # OK, those with g-r<1.19 are in aux2,
          # not concatenate files
          concat_parfiles [list aux1 aux2] $outfile 
          catch {exec cp $outfile auxin}
      }
      if {$riz} {
          ### first find i-z > 0.52*(r-i) - 0.2
          set Alist [list 0 0 -0.52 1.52 -1.0 0 0 0]; set C -0.2
          selectSDSS2MASScolors auxin $outfile $C $Alist $N psf 
          catch {exec cp $outfile auxin} 
          ### and then i-z < 0.52*(r-i) + 0.2
          set Alist [list 0 0 0.52 -1.52 1.0 0 0 0]; set C -0.2
          selectSDSS2MASScolors auxin $outfile $C $Alist $N psf 
      }         
      
}



### search SDSS2MASS par files for objects with a redder i-K color than
# expected for stars with the same g-i color; the mean relation is
#     (i-K)* = 0.9 + 0.73*(g-i)
# and the enforced relation is 
#      i-K > iKexcess + (i-K)* 
# here iKexcess ~ Nsigma * 0.2 mag, where 0.2 mag is approx. the locus width;
# it is also required that K < Kmax; assuming (J-K)max=2.5 and 5sigma in both 
# J and K, Kmax=14.05, or for just 5sigma in K take Kmax=15.05)
# g and i cuts are enforced because they are used to calculate the (i-K)* color
proc findOIRexcess {inf outf {iKexcess 0.6} {Kmax 15.05} {gmax 22.0} {imax 22.0}} {

global reddening; # controls reddening correction in selectSDSS2MASS

      ### first cut on g, i and K magnitudes (w/o reddening)
      catch {unlink aux}; catch {unlink auxSDSS}; catch {unlink auxK}          
      set reddening 0
      set g_cut [list g 14.0 $gmax]; set i_cut [list i 14.0 $imax];  
      selectSDSS2MASS $inf aux $g_cut
      selectSDSS2MASS aux auxSDSS $i_cut
      set Klist [list K 0.0 $Kmax]
      selectSDSS2MASS auxSDSS auxK $Klist   
      set reddening 1

      ### cut on i-K vs. g-i: i-K > $iKexcess + 0.9 + 0.73*(g-i)
      catch {unlink $outf}; set N 1000000
      set Alist [list 0 -0.73 0 1.73 0 0 0 -1]
      set C [expr 0.9 + $iKexcess] 
      selectSDSS2MASScolors auxK $outf $C $Alist $N psf     
 
      catch {unlink aux}; catch {unlink auxSDSS}; catch {unlink auxK}          
      
}

### search SDSS2MASS par files for objects with a redder J-K color than
# expected for stars with the same g-i color; the mean relations are
# 1)      g-i < 0: (J-K)* = - inf. (i.e. all stars with g-i<0 are interesting)
# 2) 0 <= g-i < 2: (J-K)* = 0.19 + 0.32*(g-i)
# 3)     g-i >= 2: J-K = 0.83
# and the enforced relation is 
#      J-K > JKexcess + (J-K)* 
# here JKexcess ~ 0.27 determined empirically to minimize the stellar 
# contamination and not to lose known (ugr) quasars 
# it is also required that K < Kmax; assuming (J-K)max=2.5 and 5sigma in both 
# J and K, Kmax=14.05, or for just 5sigma in K take Kmax=15.05)
# g and i cuts are enforced because they are used to calculate the (i-K)* color
proc findIRred {inf outf {JKexcess 0.27} {Kmax 15.05} {gmax 22.0} {imax 22.0}} {

global reddening; # controls reddening correction in selectSDSS2MASS

      ### first cut on g, i and K magnitudes (w/o reddening)
      catch {unlink aux}; catch {unlink auxSDSS}; catch {unlink auxK}          
      set reddening 0
      set g_cut [list g 14.0 $gmax]; set i_cut [list i 14.0 $imax];  
      selectSDSS2MASS $inf aux $g_cut
      selectSDSS2MASS aux auxSDSS $i_cut
      set Klist [list K 0.0 $Kmax]
      selectSDSS2MASS auxSDSS auxK $Klist   
      set reddening 1
      set N 1000000

      ### region I: g-i < 0 && J-K < 0.47 + 0.32(g-i)
      catch {unlink $outf.auxI}
      catch {unlink aux}
      # first cut g-i < 0
      set Alist [list 0 -1 0 1 0 0 0 0]
      set C 0.0
      selectSDSS2MASScolors auxK aux $C $Alist $N psf
      # and then J-K cut
      set Alist [list 0 0.32 0 -0.32 0 -1 0 1]
      set C [expr -0.2 - $JKexcess]
      selectSDSS2MASScolors aux $outf.auxI $C $Alist $N psf      

      ### region II: J-K > 0.47 + 0.32(g-i)
      catch {unlink $outf.auxII}
      set Alist [list 0 -0.32 0 0.32 0 1 0 -1]
      set C [expr 0.2 + $JKexcess]
      selectSDSS2MASScolors auxK $outf.auxII $C $Alist $N psf

      ### region III: 1.10 < J-K < 0.47 + 0.32*(g-i)
      catch {unlink $outf.auxIII}
      catch {unlink aux}
      # first cut J-K > 1.10
      set Alist [list 0 0 0 0 0 1 0 -1]
      set C [expr 0.83 + $JKexcess]
      selectSDSS2MASScolors auxK aux $C $Alist
      # and then J-K cut
      set Alist [list 0 0.32 0 -0.32 0 -1 0 1]
      set C [expr -0.2 - $JKexcess]
      selectSDSS2MASScolors aux $outf.auxIII $C $Alist $N psf    
 
      ### now concatenate all 3 files
      concat_parfiles [list $outf.auxI $outf.auxII $outf.auxIII] $outf

      catch {unlink aux}; catch {unlink auxSDSS}; catch {unlink auxK}     
     
}



### search SDSS2MASS par files for objects that both have redder i-K
# AND redder J-K colors than expected for stars with the same g-i color
# this are not necessarily quasars, but in case are very interesting
proc findSDSS2MASSqsos {inf outf {iKexcess 0.6} {JKexcess 0.27} \
            {Kmax 14.05} {gmax 22.0} {imax 22.0}} {

      ### first get sources with red J-K color
      catch {unlink aux}  
      findIRred $inf aux.qsos $JKexcess $Kmax $gmax $imax

      ### and then a subset with red i-K color
      findOIRexcess aux.qsos $outf $iKexcess $Kmax $gmax $imax

      catch {unlink aux.qsos}
}





###############################################################################
# tools for plotting 2MASS data

# search for EROs
proc analyze_all_red {run {cols} h1 h2 device} {

    # 5 sigma cut on J
    set Jmax 16.55
    if {0} {
      set Hmax 15.85 
      set Kmax 15.05
    } else {
      set Hmax 100
      set Kmax 100

    }
    # 5 sigma cut on z
    set zmax 20.8
    # cut on i-z > 2
    set izmin 2.0

    set root tsObj-TWOMASSpsc_hour

    loop h $h1 $h2 {
       set hS $h
       set hE [expr $h+1]
       echo " *** Working on RA = $hS-$hE hours ***" 
       set tsObjList {}
       foreach col $cols {
       set file ${root}${hS}_hour$hE.par_tsObj_run${run}_col$col.fit
           lappend tsObjList $file
       }
       set chain [tsObjList2chain $tsObjList]
       echo "  read [chainSize $chain], going to analyze..."
       plot_2MASS_SDSS_Red $chain $device 1 $zmax $izmin $Jmax $Hmax $Kmax      
       my_mv SDSS2MASS_RedStars_6cc.ps SDSS2MASS_RedStars_6cc_run$run-RA$hS-$hE.ps 
       my_mv SDSS2MASS_RedStars_JKiz.ps SDSS2MASS_RedStars_JKiz_run$run-RA$hS-$hE.ps
       my_mv EROs.dump EROs_run$run-RA$hS-$hE.dump
       genericChainDestroy $chain
    }

}

# the works...
proc analyze_all_SDSS2MASS {run {cols} h1 h2 device {plot 1} {dumpSDS2MASS 0}} {

      set root tsObj-TWOMASSpsc_hour
      set dumproot SDSS-2MASSpsc-run$run-
      set tsObjList {}

      loop h $h1 $h2 {
         set hS $h
         set hE [expr $h+1]
         foreach col $cols {
        set file ${root}${hS}_hour$hE.par_tsObj_run${run}_col$col.fit
            lappend tsObjList $file
         }
      }

      if {$plot} {plot_2MASS_SDSS_all $tsObjList $device}
 
      if {$dumpSDS2MASS} {
          set file ${dumproot}RAh${h1}-$h2.par
          dump_SDSS2MASS $tsObjList $file
      }

}

# wrapper for finding good targets for observing
# proc to select bright galaxies from SDSS-2MASS par files
proc brightOIRgalaxies {parfile} {

   # first dump good galaxies
   set flagsOFF [list BRIGHT SATUR BLENDED EDGE CR]
   flag_search $parfile $parfile.goodG $flagsOFF {} {} {} 3 
   echo "Produced file $parfile.goodG"
  
   # now look for galaxies with 14 < J < 15 and J-K > 1.2
   set AGNcolors {-10 10 -10 10 -10 10 -10 10 -10 10 1.2 10 -10 10}
   selectSDSS2MASS $parfile.goodG $parfile.G-AGN {J 14 15} $AGNcolors
   echo "Produced file $parfile.G-AGN"

   # for finding charts, 2 arcmin, in r
   # finding_charts $parfile.G-AGN . r 300 0 -1 1 "SDSS-2MASS galaxy" 1 -1 none

}



# plot everything for a list of matched tsObj files
proc plot_2MASS_SDSS_all {files {device /VCPS}} {
 
     ### tunable parameters
     # mag range
     set band z; set mmin 14.0; 
     # 5 sigma in z
     set mmax 20.8
     # 5 sigma in z
     set mmax 20.8

     # color range     
     set band1 i; set band2 z
     set cmin -10.0; set cmax 10.0
     # maximal errors
     set mag_errmax 0.2; set col_errmax 0.3; 
     # 2MASS 5 sigma
     set Jmax 16.55; set Hmax 15.85; set Kmax 15.05 

 
     ### for precise color-color diagrams
     # 2MASS 10 sigma
     set Jmax 15.80; set Hmax 15.10; set Kmax 14.30 
     # SDSS z 10 sigma
     set mmax 20.05
     # tighter maximal errors
     set mag_errmax 0.1; set col_errmax 0.2; 


     # read chain
     set chain [tsObjList2chain $files]
     echo "  read s[chainSize $chain] sources from specified tsObj file list"

     # plot selected sources
     plot_2MASS_SDSS $chain $device $band $mmin $mmax $Jmax $Hmax $Kmax

     # clean
     genericChainDestroy $chain

}


# first read a chain:
# set chain [param2Chain $parfile ""]
proc plot_2MASScounts {chain device {title ""} \
           {Jmax 15.8} {Kmax 15.05} {nbin 36} {min 0.0} {max 360.0} {flags 1}} {

     set nall [chainSize $chain]
     if {$flags} {
         set aux [chainSearch $chain "{J < $Jmax} {K < $Kmax} {ra >= $min} {ra <= $max}"]
         echo " Selected [chainSize $aux] sources with J < $Jmax & K < $Kmax" 
         set good [selectTWOMASSPSC $aux 222 111 000 0 0]
         chainDel $aux
         set Ngood [chainSize $good] 
         echo " Selected $Ngood sources with good flags from $nall sources" 
     } else {
         set good [chainSearch $chain "{J < $Jmax} {K < $Kmax} {ra >= $min} {ra <= $max}"]
         echo " Selected [chainSize $good] sources with J < $Jmax & K < $Kmax" 
     } 

     if {[chainSize $good] < 1} {
        chainDel $good
        return "There are no surviving sources"
     }


     # form vectors
     foreach b {ra dec J H K} {                                           
        set $b [vFromChain $good $b]
     }
     vNameSet $ra "RA (deg)"
     vNameSet $dec "Dec (deg)"
     set Decmin [vExtreme $dec min]
     set Decmax [vExtreme $dec max]
     vNameSet $J "J"; vNameSet $H "H"; vNameSet $K "K"
     set JH [vectorExprEval $J-$H]
     set HK [vectorExprEval $H-$K]
     set JK [vectorExprEval $J-$K]
     vNameSet $JH "J-H"
     vNameSet $HK "H-K"
     vNameSet $JK "J-K"

     # split into classes
     set maskaux  [vectorExprEval "$JK > -20  ? 1 : 0"]
     set maskB  [vectorExprEval "$JK < 0.7  ? 1 : 0"]
     set maskR  [vectorExprEval "$JK <= 1.0 ? 1-$maskB : 0"]
     set maskG  [vectorExprEval "$JK > 1.0  ? 1 : 0"]
     set NB [vNnonzero $maskB]
     set NR [vNnonzero $maskR]
     set NG [vNnonzero $maskG]
     echo "  and split into: "
     echo "      blue stars: $NB"
     echo "       red stars: $NR"
     echo "        galaxies: $NG"

     set titles [list "$title All" "Blue stars (J-K<0.7)" "Red stars (0.7<J-K<1.0)" "Galaxies (J-K>1.0)"] 
     set masks [list $maskaux $maskB $maskR $maskG]

     # plot J histograms
     plot_many_histograms $device $J $masks $titles $nbin 10 17 "J (mag)" 1
     my_mv many_histograms.ps Jhistograms_2MASS.ps

     # plot K histograms
     plot_many_histograms $device $K $masks $titles $nbin 10 17 "K (mag)" 1
     my_mv many_histograms.ps Khistograms_2MASS.ps

     # plot J-K histograms
     plot_many_histograms $device $JK $masks $titles $nbin -0.5 2.0 "J-K"
     my_mv many_histograms.ps JKhistograms_2MASS.ps

     # plot RA histograms
     plot_many_histograms $device $ra $masks $titles $nbin $min $max "RA (deg)"
     my_mv many_histograms.ps RAhistograms_2MASS.ps


     # plot Dec vs. RA
     if [catch {
       if {$device == "/VCPS"} {
          set deviceB /CPS 
       } else {
          set deviceB $device
       }
       # Set up the plot 
       set pg [pgstateNew]
       set deviceA [devicename $deviceB RADec_2MASSclasses.ps] 
       pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.35 -yfract 0.35 -nxwindow 1 -nywindow 4 
       pgstateOpen $pg

     loop i 0 [llength $masks] {
          set mask [lindex $masks $i]
      vPlot $pg $ra $dec -vectorMask $mask -xmin $min -xmax $max -ymin $Decmin -ymax $Decmax 
          titlePlot [lindex $titles $i] 20     
       }

       endplot $pg
     } msg] {
         echo "$msg"
     }

     # plot J and K vs. J-K
     if [catch {
       if {$device == "/VCPS"} {
          set deviceB /CPS 
       } else {
          set deviceB $device
       }
       # Set up the plot 
       set pg [pgstateNew]
       set deviceA [devicename $deviceB JKvsJ-K_2MASSclasses.ps] 
       pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.35 -yfract 0.35 -nxwindow 1 -nywindow 3 
       pgstateOpen $pg

       vPlot $pg $JK $J -xmin -0.5 -xmax 2.0 -ymin 17.0 -ymax 10.0 
       titlePlot [lindex $titles 0] 20       

       vPlot $pg $JK $K -xmin -0.5 -xmax 2.0 -ymin 17.0 -ymax 10.0 

       endplot $pg
     } msg] {
         echo "$msg"
     }


 
     # clean
     chainDel $good
     vListDel [list $ra $dec $J $H $K]
     vListDel [list $JH $HK $JK $maskaux $maskB $maskR $maskG]

}





# first read a chain:
# set file /peyton/scr/ginka1/finlator/Thesis/2MASS/TWOMASSpsc.par
# set chain [param2Chain $file ""]
proc plot_all_2MASS {chain device {err_max 0.1} {err_max2 0.05}} {

     set err2 $err_max2
     set Hmax 14.5
    
     set nall [chainSize $chain]
     # first cut on errors
     set cond "{Jerr < $err_max} {Herr < $err_max} {Kerr < $err_max}"
     set good [chainSearch $chain $cond]
     set Ngood [chainSize $good] 
     echo "Selected $Ngood sources with err < $err_max from $nall sources" 

     # limit s for plotting colos and magnitudes
     set cmin -0.1
     set cmax  2.2
     set mmin 17
     set mmax  5

     # form vectors
     foreach b {J H K Jerr Herr Kerr} {                                           
        set $b [vFromChain $good $b]
     }
     vNameSet $J "J"; vNameSet $H "H"; vNameSet $K "K"
     set JH [vectorExprEval $J-$H]
     set HK [vectorExprEval $H-$K]
     set JK [vectorExprEval $J-$K]
     vNameSet $JH "J-H"
     vNameSet $HK "H-K"
     vNameSet $JK "J-K"

     # tighter errors
     set cond "{Jerr < $err2} {Herr < $err2} {Kerr < $err2}"
     set good2 [chainSearch $chain $cond]
     set Ngood2 [chainSize $good2] 
     echo "Selected $Ngood2 sources with err < $err2 from $nall sources" 
     foreach b {J H K} { 
        set q ${b}2                                        
        set $q [vFromChain $good2 $b]
     }
     vNameSet $J2 "J"; vNameSet $H2 "H"; vNameSet $K2 "K"
     set JH2 [vectorExprEval $J2-$H2]
     set HK2 [vectorExprEval $H2-$K2]
     set JK2 [vectorExprEval $J2-$K2]
     vNameSet $JH2 "J-H"
     vNameSet $HK2 "H-K"
     vNameSet $JK2 "J-K"

     # bright sources observable in SDSS (H < 14.5)
     set cond "{H < $Hmax}"
     set good3 [chainSearch $chain $cond]
     set Ngood3 [chainSize $good3] 
     echo "Selected $Ngood3 sources with H<$Hmax from $nall sources" 
     foreach b {J H K} { 
        set q ${b}3                                        
        set $q [vFromChain $good3 $b]
     }
     vNameSet $J3 "J"; vNameSet $H3 "H"; vNameSet $K3 "K"
     set JH3 [vectorExprEval $J3-$H3]
     set HK3 [vectorExprEval $H3-$K3]
     set JK3 [vectorExprEval $J3-$K3]
     vNameSet $JH3 "J-H"
     vNameSet $HK3 "H-K"
     vNameSet $JK3 "J-K"

     # sources with err < err2 and good flags
     set good4 [selectTWOMASSPSC $good2 222 111 000 0 0]
     set Ngood4 [chainSize $good4] 
     echo "Selected $Ngood4 sources with good flags and err < $err2 from $nall sources" 
     foreach b {J H K} { 
        set q ${b}4                                        
        set $q [vFromChain $good4 $b]
     }
     vNameSet $J4 "J"; vNameSet $H4 "H"; vNameSet $K4 "K"
     set JH4 [vectorExprEval $J4-$H4]
     set HK4 [vectorExprEval $H4-$K4]
     set JK4 [vectorExprEval $J4-$K4]
     vNameSet $JH4 "J-H"
     vNameSet $HK4 "H-K"
     vNameSet $JK4 "J-K"

     lappend transferFields [list rd_flg fracPSF<0>]
     lappend transferFields [list bl_flg fracPSF<1>]
     lappend transferFields [list cc_flg fracPSF<2>]
     lappend transferFields [list extd_flg fracPSF<3>]
     lappend transferFields [list mp_flg fracPSF<4>]


 
     # plot color-color diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device 2MASS_ccdiags.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.4 -yfract 0.4 -nxwindow 2 -nywindow 2 
         pgstateOpen $pg

         # error < $err_max
         vPlot $pg $JH $JK -xmin $cmin -xmax $cmax -ymin $cmin -ymax $cmax 
         titlePlot "2MASS: err<$err_max, n=$Ngood" 20        
 
         # error < $err2
         vPlot $pg $JH2 $JK2 -xmin $cmin -xmax $cmax -ymin $cmin -ymax $cmax 
         titlePlot "err<$err2, n=$Ngood2" 20        
         
         # H < $Hmax
         vPlot $pg $JH3 $JK3 -xmin $cmin -xmax $cmax -ymin $cmin -ymax $cmax 
         titlePlot "H < $Hmax, n=$Ngood3" 20        

         # good flags and error < $err2
         vPlot $pg $JH4 $JK4 -xmin $cmin -xmax $cmax -ymin $cmin -ymax $cmax 
         titlePlot "good flags and err<$err2: n=$Ngood4" 20        

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # plot color-magnitude diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device 2MASS_cmagdiags.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.4 -yfract 0.4 -nxwindow 2 -nywindow 2 
         pgstateOpen $pg

         # J vs. J-H
         vPlot $pg $JH2 $J2 -xmin $cmin -xmax $cmax -ymin $mmin -ymax $mmax 
         titlePlot "2MASS: err<$err2, n=$Ngood2" 20        
 
         # K vs. H-K
         vPlot $pg $JK2 $K2 -xmin $cmin -xmax $cmax -ymin $mmin -ymax $mmax 

         # H vs. J-H
         vPlot $pg $JH2 $H2 -xmin $cmin -xmax $cmax -ymin $mmin -ymax $mmax 

         # H vs. H-K
         vPlot $pg $JK2 $H2 -xmin $cmin -xmax $cmax -ymin $mmin -ymax $mmax 

         endplot $pg
     } msg] {
         echo "$msg"
     }
    


  if {1} {
     # plot color-color diagrams
     plot_xy $JH $HK $device "2MASS: err<$err_max, n=$Ngood" 1 1 1
     my_mv plot_xy.ps JH-HKbig.ps     
     # plot color-magnitude diagrams
     plot_xy $JH $J $device "2MASS: err<$err_max, n=$Ngood" 1 1 1
     my_mv plot_xy.ps J-JHbig.ps
     plot_xy $HK $K $device "2MASS: err<$err_max, n=$Ngood" 1 1 1 
     my_mv plot_xy.ps K-HKbig.ps
     plot_xy $JK $H $device "2MASS: err<$err_max, n=$Ngood" 1 1 1 
     my_mv plot_xy.ps H-JKbig.ps
  }

  if {1} { 
     # plot color histograms
     plot_histogram $device $JH4 100 -0.3 1.2 "J-H" "2MASS: good flags and err<$err2, n=$Ngood4"
     my_mv plot_histogram.ps JHhistogram.ps
     plot_histogram $device $HK4 100 -0.3 1.2 "H-K" "2MASS: good flags and err<$err2, n=$Ngood4"
     my_mv plot_histogram.ps HKhistogram.ps
     plot_histogram $device $JK4 100 -1.0 3.0 "J-K" "2MASS: good flags and err<$err2, n=$Ngood4"
     my_mv plot_histogram.ps JKhistogram.ps
  }

  if {1} { 
     # plot magnitude histograms
     plot_histogram $device $J2 100 5 17 "J" "2MASS: err<$err2, n=$Ngood2"
     my_mv plot_histogram.ps Jhistogram.ps
     plot_histogram $device $H2 100 5 17 "H" "2MASS: err<$err2, n=$Ngood2"
     my_mv plot_histogram.ps Hhistogram.ps
     plot_histogram $device $K2 100 5 17 "K" "2MASS: err<$err2, n=$Ngood2"
     my_mv plot_histogram.ps Khistogram.ps
  }

     chainDel $good; chainDel $good2; chainDel $good3; chainDel $good4
     vectorExprDel $J; vectorExprDel $H; vectorExprDel $K 
     vectorExprDel $J_err; vectorExprDel $H_err; vectorExprDel $K_err 
     vectorExprDel $JH; vectorExprDel $HK; vectorExprDel $JK
     vectorExprDel $J2; vectorExprDel $H2; vectorExprDel $K2 
     vectorExprDel $JH2; vectorExprDel $HK2; vectorExprDel $JK2
     vectorExprDel $J3; vectorExprDel $H3; vectorExprDel $K3 
     vectorExprDel $JH3; vectorExprDel $HK3; vectorExprDel $JK3
     vectorExprDel $J4; vectorExprDel $H4; vectorExprDel $K4
     vectorExprDel $JH4; vectorExprDel $HK4; vectorExprDel $JK4

}

# first read a chain :
# set tsObj tsObj-TWOMASSpsc_hour12_hour13.par_tsObj_run756_col1.fit
# set chain [tsObj2chain $tsObj]
# OR set chain [tsObjList2chain $tsObjList]
# and then use proc clean_tsObj_chain to select desired subsample
proc plot_2MASS_SDSS {inchain device {band z} {mmin 14.0} {mmax 23.0} \
                      {Jmax 15.8} {Hmax 15.1} {Kmax 14.3}} {

### N.B. default values for 2MASS cutoff are 10 sigma limits for point sources
### for 5 sigma cutoff add 0.75 mag to each limit
     set filterlist {u g r i z}
     set type "psfCounts"

     set Nall [chainSize $inchain] 
     # raw values
     set Jr [vFromChain $inchain fiberCounts<0>]
     set Hr [vFromChain $inchain fiberCounts<1>]
     set Kr [vFromChain $inchain fiberCounts<2>]
     set JKr [vectorExprEval $Jr-$Kr]
     vNameSet $Jr "J"; vNameSet $Hr "H"; vNameSet $Kr "K"; vNameSet $JKr "J-K"; 
     set gRaw [vFromChain $inchain $type<1>]
     set rRaw [vFromChain $inchain $type<2>]
     set grRaw [vectorExprEval $gRaw-$rRaw]
     vNameSet $gRaw "g'"; vNameSet $rRaw "r'"; vNameSet $grRaw "g'-r'"; 

     # first cut on 2MASS errors
     #set err_max 0.05
     #set cond "{fiberCountsErr<0> < $err_max} {fiberCountsErr<1> < $err_max} {fiberCountsErr<1> < $err_max}"
     ### cut on H < Hmax
     set cond "{fiberCounts<0> < $Jmax} {fiberCounts<1> < $Hmax} {fiberCounts<2> < $Kmax}" 
     set aux [chainSearch $inchain $cond]
     set Naux [chainSize $aux] 
     echo "-----------------------------------------------------------"
     echo " ** cut on limiting H mag **"
     echo "Selected $Naux with J<$Jmax, H<$Hmax and K<$Kmax from $Nall matched sources" 
     # and on 2MASS flags
     set cond "{fracPSF<0> == 222} {fracPSF<1> == 111}"  
     set chain [chainSearch $aux $cond]
     set Ngood [chainSize $chain] 
     echo " ** cut on 2MASS flags (222 and 111) **"
     echo "Selected $Ngood sources with H < $Hmax and good 2MASS flags (222, 111)"
     echo "-----------------------------------------------------------"

     ### SDSS data
     echo ""; echo " Analyzing SDSS data for the remaining $Ngood sources..."
     set deredden 0
     loop i 0 5 { 
         set f [lindex $filterlist $i] 
         if {$deredden} {
            set reddening [vFromChain $chain $reddening<$i>]
            set mpsfRaw  [vFromChain $chain $type<$i>]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            vectorExprDel $reddening; vectorExprDel $mpsfRaw
         } else {
            set mpsf($f) [vFromChain $chain $type<$i>]
         }
         set epsf($f) [vFromChain $chain ${type}Err<$i>]
         vNameSet $mpsf($f) "${f}'"; vNameSet $epsf($f) ""  
     }     
     set ra [vFromChain $chain ra]
     set dec [vFromChain $chain dec]
     set objc_type [vFromChain $chain objc_type]

     # SDSS mask for plotting
     set maskS  [vectorExprEval "$objc_type == 6 ? 1 : 0"]
     set maskG  [vectorExprEval "$objc_type == 3 ? 1 : 0"]
     set Ns [vNnonzero $maskS] 
     set Ng [vNnonzero $maskG] 
     echo "   ...there are $Ns stars and $Ng galaxies (n.b. [expr $Ns+$Ng] classified out of $Ngood)"
     set mask_aux [vectorExprEval "$mpsf($band) > $mmin ? $maskS : 0"]
     set SDSSmask [vectorExprEval "$mpsf($band) < $mmax ? $mask_aux : 0"]
     set Naux [vNnonzero $mask_aux] 
     set Nplot [vNnonzero $SDSSmask] 
     echo "   ...there are $Naux stars with mag($band) > $mmin and $Nplot sources with $mmin < mag($band) < $mmax" 
     echo "   ** Therefore, plotting $Nplot stars out of $Nall provided matched sources **" 
     vectorExprDel $mask_aux 

     # colors
     set ug [vectorExprEval $mpsf(u)-$mpsf(g)]
     set gr [vectorExprEval $mpsf(g)-$mpsf(r)]
     set ri [vectorExprEval $mpsf(r)-$mpsf(i)]
     set iz [vectorExprEval $mpsf(i)-$mpsf(z)]
     vNameSet $ug "u'-g'"; vNameSet $gr "g'-r'"; 
     vNameSet $ri "r'-i'"; vNameSet $iz "i'-z'"; 

     ### 2MASS data 
     echo "  forming 2MASS data vectors"
     # clipped
     set ra2MASS [vFromChain $chain lambda]
     set dec2MASS [vFromChain $chain eta]
     set J [vFromChain $chain fiberCounts<0>]
     set Jerr [vFromChain $chain fiberCountsErr<0>]
     set H [vFromChain $chain fiberCounts<1>]
     set Herr [vFromChain $chain fiberCountsErr<1>]
     set K [vFromChain $chain fiberCounts<2>]
     set Kerr [vFromChain $chain fiberCountsErr<2>]
     set rd_flg [vFromChain $chain fracPSF<0>]
     set bl_flg [vFromChain $chain fracPSF<1>]
     set cc_flg [vFromChain $chain fracPSF<2>]
     set extd_flg [vFromChain $chain fracPSF<3>]
     set mp_flg [vFromChain $chain fracPSF<4>]
     set JH [vectorExprEval $J-$H]
     set HK [vectorExprEval $H-$K]
     set JK [vectorExprEval $J-$K]
     vNameSet $JH "J-H"; vNameSet $HK "H-K"; vNameSet $JK "J-K"; 
     vNameSet $J "J"; vNameSet $H "H"; vNameSet $K "K"; 
     # some SDSS-2MASS colors
     set rK [vectorExprEval $mpsf(r)-$K]
     set zJ [vectorExprEval $mpsf(r)-$J]
     set zK [vectorExprEval $mpsf(r)-$K]
     vNameSet $rK "r'-K"; vNameSet $zJ "z'-J"; vNameSet $zK "z'-K"; 


     ## now split to blue and red subsamples according to r-i color
     echo "   splitting into r-i vs. g-r spectral classes..."
     set BlueMask [vectorExprEval "$ri <= 0.7 && $gr <= 1.2 ? $SDSSmask : 0"]
     set RedMask [vectorExprEval "$BlueMask == 1 ? 0 : $SDSSmask"]
     set nAll [vNnonzero $SDSSmask] 
     set nB [vNnonzero $BlueMask] 
     set nR [vNnonzero $RedMask] 
     echo "   Sanity check: All = $nAll, B+R = [expr $nB + $nR], blue = $nB, red = $nR"

     ## separate blue stars to 6 bins according to their g-r color
      # OBA stars
      set OBmask [vectorExprEval "$gr > -1.2 && $gr <= 0.1 ? $BlueMask : 0"]
      # early F stars
      set Amask [vectorExprEval "$gr > 0.1  && $gr <= 0.3 ? $BlueMask : 0"]
      # late F and early G stars
      set Fmask [vectorExprEval "$gr > 0.3  && $gr <= 0.45 ? $BlueMask : 0"]
      # late G stars
      set Gmask [vectorExprEval "$gr > 0.45 && $gr <=  0.65 ? $BlueMask : 0"]
      # early K stars
      set eKmask [vectorExprEval "$gr > 0.65 && $gr <= 1.2 ? $BlueMask : 0"]
      # late K stars
      set lKmask [vectorExprEval "$gr > 1.2 && $ri <= 0.7 ? $RedMask : 0"]
      ## separate red stars to 2 bins according to their r-i color
      # early M stars   
      set eMmask [vectorExprEval "$ri > 0.7 && $ri <= 1.1 ? $RedMask : 0"]
      # late M stars      
      set lMmask [vectorExprEval "$ri > 1.1 && $ri <= 10.0 ? $RedMask : 0"]
     ## report
     echo ""; echo " *** The results of separation according to SDSS colors ***" 
     set nOB [vNnonzero $OBmask]
     set nA  [vNnonzero $Amask]
     set nF  [vNnonzero $Fmask]
     set nG  [vNnonzero $Gmask]
     set neK [vNnonzero $eKmask]
     set nlK [vNnonzero $lKmask]
     set neM [vNnonzero $eMmask]
     set nlM [vNnonzero $lMmask]
     set nf "%6d"
     set fnOB [format $nf $nOB]
     set fnA  [format $nf $nA]
     set fnF  [format $nf $nF]
     set fnG  [format $nf $nG]
     set fneK [format $nf $neK]
     set fnlK [format $nf $nlK]
     set fneM [format $nf $neM]
     set fnlM [format $nf $nlM]
     set nTot [expr $nOB+$nA+$nF+$nG+$neK+$nlK+$neM+$nlM]
     set fnTot [format $nf $nTot]
     set nBlue [format $nf [expr $nOB+$nA+$nF+$nG+$neK+$nlK]] 
     set nRed [format $nf [expr  $neM+$nlM]] 
   if {1} {
     echo ""; echo "    Class       N     %"
     set pf "%4.1f"
     echo "      OB    $fnOB  [format $pf [expr 100.0*$nOB/$nTot]]"         
     echo "       A    $fnA  [format $pf [expr 100.0*$nA/$nTot]]"         
     echo "       F    $fnF  [format $pf [expr 100.0*$nF/$nTot]]"         
     echo "       G    $fnG  [format $pf [expr 100.0*$nG/$nTot]]"         
     echo "      eK    $fneK  [format $pf [expr 100.0*$neK/$nTot]]"         
     echo "      lK    $fnlK  [format $pf [expr 100.0*$nlK/$nTot]]"         
     echo "      eM    $fneM  [format $pf [expr 100.0*$neM/$nTot]]"         
     echo "      lM    $fnlM  [format $pf [expr 100.0*$nlM/$nTot]]"         
     echo "    blue    $nBlue  [format $pf [expr 100.0*$nBlue/$nTot]] (of all)"         
     echo " red (M)    $nRed  [format $pf [expr 100.0*$nRed/$nTot]] (of all)"         
     echo "     all    $fnTot  [format $pf [expr 100.0*$nTot/$Nplot]] (of total)"        
   }

  if {1} {
     # plot color-color diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_ccS.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.4 -yfract 0.33 -nxwindow 3 -nywindow 4 
         pgstateOpen $pg

         # SDSS colors
         vPlot $pg $ug $gr -vectorMask $SDSSmask -xmin -0.5 -xmax 4.0 -ymin -1.0 -ymax 2.5
         titlePlot "SDSS - 2MASS match: n=$Nplot" 20        

         vPlot $pg $gr $ri -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin -1.0 -ymax 2.5
         vPlot $pg $ri $iz -vectorMask $SDSSmask -xmin -1.0 -xmax 3.5 -ymin -1.0 -ymax 4.5

         # J-H combinations
         vPlot $pg $JH $gr -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin -1.0 -ymax 2.5
         vPlot $pg $JH $iz -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin -1.0 -ymax 4.5
         vPlot $pg $JH $zJ -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin 0.0 -ymax 4.5
         
         # JHK etc.
         vPlot $pg $rK $zJ -vectorMask $SDSSmask -xmin -1.0 -xmax 10.5 -ymin 0.0 -ymax 4.5    
         vPlot $pg $rK $JK -vectorMask $SDSSmask -xmin -1.0 -xmax 10.5 -ymin 2.5 -ymax -0.5 
         vPlot $pg $JH $HK -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin -0.5 -ymax 1.5 

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }
 
  if {1} {
     # plot color-magnitude diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_cmS.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.4 -yfract 0.33 -nxwindow 3 -nywindow 4 
         pgstateOpen $pg

         # SDSS colors
         vPlot $pg $gr $mpsf(g) -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin 23 -ymax 13

         titlePlot "SDSS - 2MASS match: n=$Nplot" 20        

         vPlot $pg $gr $mpsf(r) -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin 23 -ymax 13
         vPlot $pg $gr $mpsf(z) -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin 23 -ymax 13

         # J-J combinations
         vPlot $pg $iz $mpsf(i) -vectorMask $SDSSmask -xmin -1.0 -xmax 4.5 -ymin 23 -ymax 13
         vPlot $pg $iz $mpsf(z) -vectorMask $SDSSmask -xmin -1.0 -xmax 4.5 -ymin 20 -ymax 11
         vPlot $pg $iz $J -vectorMask $SDSSmask -xmin -1.0 -xmax 4.5 -ymin 16 -ymax 5
         
         # JHK etc. 
         vPlot $pg $JH $mpsf(z) -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5  -ymin 20 -ymax 11         
         vPlot $pg $JH $J -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin 16 -ymax 5
         vPlot $pg $JH $H -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin 16 -ymax 5 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 }

 
 
  #### PAPER PLOTS ####

      ## definition of SpT separation
      set titles [list "OBA, n=$nOB" "eF, n=$nA" "lF and eG, n=$nF"  \
                       "lG, n=$nG" "eK, n=$neK" "lK, n=$nlK" \
               "eM, n=$neM" "lM, n=$nlM" "all" ] 
      set maskAll [vectorExprEval $OBmask+$Amask+$Fmask+$Gmask+$eKmask+$lKmask+$eMmask+$lMmask]
      set masks9 [list $OBmask $Amask $Fmask $Gmask $eKmask $lKmask $eMmask $lMmask $maskAll] 
      set masks  [list $OBmask $Amask $Fmask $Gmask $eKmask $lKmask $eMmask $lMmask] 
      set colors  [list  1  5  2  7  3  4  6  9  8]
      set symbols [list -1 -1 -1 -1 -1 -1 -1 -1  3]
 

   if {1} {
     # plot 6 color-color diagrams on one page 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS2MASS_6CCdiags_dataStars.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.3 -yfract 0.35 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # r-i vs. g-r
         plot_subsamples $pg $gr $ri $masks $titles $colors $symbols -0.5 2.8 -0.2 3.2 
         titlePlot "SDSS - 2MASS match: n=$Nplot" 20        

         # z-J vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zJ $JK $masks $titles $colors $symbols 0.0 7.0 -0.4 2.0 

         # g-r vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $gr $JK $masks $titles $colors $symbols -0.5 2.8 -0.4 2.0

         # i-z vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $iz $JK $masks $titles $colors $symbols -0.5 4.0 -0.4 2.0 

         # H-K vs. J-H
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $JH $HK $masks $titles $colors $symbols 0.0 1.0 -0.3 0.7

         # J-K vs. r-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $rK $JK $masks $titles $colors $symbols -0.5 10.5 -0.4 2.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }


   if {1} {
     # plot 6 color-color diagrams for red sources on one page 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS2MASS_6CCdiags_dataRedStars.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.3 -yfract 0.35 -nxwindow 2 -nywindow 4
         pgstateOpen $pg


         # J-K vs. r-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $rK $JK $masks $titles $colors $symbols -0.5 10.5 -0.4 2.0

         # z-K vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zK $JK $masks $titles $colors $symbols -1.0 10.0 -0.4 2.0

         # i-z vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $iz $JK $masks $titles $colors $symbols 0.0 4.0 -0.4 2.0

         # i-z vs. z
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $iz $mpsf(z) $masks $titles $colors $symbols 0.0 4.0 22.0 14.0

         # z vs. z-K 
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zK $mpsf(z) $masks $titles $colors $symbols -1.0 10.0  22.0 14.0

         # K vs. z-K 
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zK $K $masks $titles $colors $symbols -1.0 10.0  16.0 7.0

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }

   set histMin -0.1
   set histMax 1.3
   set histNbin 28


   if {0} { 
     # plot model star g-r (all) and J-K (classified) histograms
     if [catch {


         ### models
         # get models, default is 22.5 deg2 centered on RA=187, Dec=0 (l=292, b=62) 
         # it seems that z > 14 and H < 14.3 selects 5.6% of all modelled sources 
         set NmodelAll [expr int($nTot / 0.056)] 
         echo "calling get_model for $NmodelAll sources"
         set modelList [get_model $NmodelAll]
         echo " ... model OK"

         set modelmasks  [lindex $modelList 0]
         set modelcolors [lindex $modelList 1]
         set grmodel [lindex $modelcolors 1]
         set rimodel [lindex $modelcolors 2]
         set izmodel [lindex $modelcolors 3]
         set zJmodel [lindex $modelcolors 4]
         set JHmodel [lindex $modelcolors 5]
         set HKmodel [lindex $modelcolors 6]
         set JKmodel [vectorExprEval $JHmodel+$HKmodel]
         set rKmodel [vectorExprEval $rimodel+$izmodel+$zJmodel+$JKmodel]
         vNameSet $grmodel "g'-r'";         
         vNameSet $rimodel "r'-i'";         
         vNameSet $izmodel "i'-z'";         
         vNameSet $zJmodel "z'-J";         
         vNameSet $rKmodel "r'-K";         
         vNameSet $JHmodel "J-H";         
         vNameSet $HKmodel "H-K";         
         vNameSet $JKmodel "J-K";         
         set MOBmask [lindex $modelmasks 0]
         set MAmask [lindex $modelmasks 1]
         set MFmask [lindex $modelmasks 2]
         set MGmask [lindex $modelmasks 3]
         set MeKmask [lindex $modelmasks 4]
         set MlKmask [lindex $modelmasks 5]
         set MeMmask [lindex $modelmasks 6]
         set MlMmask [lindex $modelmasks 7]
         set MmaskAll [vectorExprEval $MOBmask+$MAmask+$MFmask+$MGmask+$MeKmask+$MlKmask+$MeMmask+$MlMmask]   
     
     ### MODEL-DATA comparison:  plot 6 color-color diagrams on one page 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS2MASS_6CCdiags_modelStars.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.3 -yfract 0.35 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # r-i vs. g-r
         plot_subsamples $pg $grmodel $rimodel $modelmasks $titles $colors $symbols -0.5 2.8 -0.2 3.2 
         titlePlot "SDSS - 2MASS match model: n=$Nplot" 20        

         # z-J vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zJmodel $JKmodel $modelmasks $titles $colors $symbols 0.0 7.0 -0.4 2.0 

         # g-r vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $grmodel $JKmodel $modelmasks $titles $colors $symbols -0.5 2.8 -0.4 2.0

         # i-z vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $izmodel $JKmodel $modelmasks $titles $colors $symbols -0.5 4.0 -0.4 2.0 

         # H-K vs. J-H
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $JHmodel $HKmodel $modelmasks $titles $colors $symbols 0.0 1.0 -0.3 0.7

         # J-K vs. r-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $rKmodel $JKmodel $modelmasks $titles $colors $symbols -0.5 9.5 -0.4 2.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }

         lappend modelmasks $MmaskAll 

         set nf "%6d"
         set MnOB [format $nf [vNnonzero $MOBmask]]
         set MnA  [format $nf [vNnonzero $MAmask]]
         set MnF  [format $nf [vNnonzero $MFmask]]
         set MnG  [format $nf [vNnonzero $MGmask]]
         set MneK [format $nf [vNnonzero $MeKmask]]
         set MnlK [format $nf [vNnonzero $MlKmask]]
         set MneM [format $nf [vNnonzero $MeMmask]]
         set MnlM [format $nf [vNnonzero $MlMmask]]
         set MnTot [expr $MnOB+$MnA+$MnF+$MnG+$MneK+$MnlK+$MneM+$MnlM]
      
         echo "SDSS-2MASS matches separated by r-i vs. g-r criteria:"   
         echo "      OB    $MnOB  [format $pf [expr 100.0*$MnOB/$MnTot]]"         
         echo "       A    $MnA   [format $pf [expr 100.0*$MnA/$MnTot]]"         
         echo "       F    $MnF   [format $pf [expr 100.0*$MnF/$MnTot]]"         
         echo "       G    $MnG   [format $pf [expr 100.0*$MnG/$MnTot]]"         
         echo "      eK    $MneK  [format $pf [expr 100.0*$MneK/$MnTot]]"         
         echo "      lK    $MnlK  [format $pf [expr 100.0*$MnlK/$MnTot]]"         
         echo "      eM    $MneM  [format $pf [expr 100.0*$MneM/$MnTot]]"         
         echo "      lM    $MnlM  [format $pf [expr 100.0*$MnlM/$MnTot]]"         
         echo "     all    $MnTot [format $pf [expr 100.0*$MnTot/$NmodelAll]] (of total)"        
         echo "    (n.b. sum of classes   asked from models)"

         # model g-r (all together) 
         plot_histogram $device $grmodel 100 -0.2 1.8 "g'-r'" "Model sample for SDSS-2MASS match (n=$MnTot)"
         my_mv plot_histogram.ps gr_histogram_ModelMatch.ps

         # model J-K (classified) 
         set Mtitles [list "MODELS: OBA, n=$MnOB" "eF, n=$MnA" "lF and eG, n=$MnF"  \
                           "lG, n=$MnG" "eK, n=$MneK" "lK, n=$MnlK" \
               "eM, n=$MneM" "lM, n=$MnlM" "all, n=$MnTot"] 
 
         plot_many_histograms $device $JKmodel $modelmasks $Mtitles 32 -0.1 1.5 "J-K"
         my_mv many_histograms.ps JKhistograms_ModelMatch.ps

  

        ### clean
         foreach m $modelcolors {
             vectorExprDel $m
         }
         foreach m $modelmasks {
             vectorExprDel $m
         }
         vectorExprDel $JKmodel; vectorExprDel $rKmodel

      } msg] {
         echo "$msg"
      }
  } 


  if {1} { 
      # plot data histograms for g-r, J-H, H-K and J-K colors
      if [catch {

         set histMin -0.1
         set histMax 1.3
         set histNbin 28
 
         ### data
         # g-r
         plot_many_histograms $device $gr $masks9 $titles 46 -0.3 2.0 "g'-r'" 
         my_mv many_histograms.ps gr_hist_manyData.ps
         # r-i
         plot_many_histograms $device $ri $masks9 $titles 46 -0.3 2.0  "r'-i'" 
         my_mv many_histograms.ps ri_hist_manyData.ps
         # J-H
         plot_many_histograms $device $JH $masks9 $titles $histNbin $histMin $histMax "J-H" 
         my_mv many_histograms.ps JHhist_manyData.ps
         # H-K
         plot_many_histograms $device $HK $masks9 $titles 28 -0.4 1.0 "H-K" 
         my_mv many_histograms.ps HKhist_manyData.ps
         # J-K
         plot_many_histograms $device $JK $masks9 $titles 32 -0.1 1.5 "J-K"
         my_mv many_histograms.ps JKhist_manyData.ps
     } msg] {
         echo "$msg"
     }
  }

 
  if {1} {
     # plot color-magnitude diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_4cm.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.35 -yfract 0.3 -nxwindow 2 -nywindow 3 
         pgstateOpen $pg

         # SDSS 
         vPlot $pg $grRaw $rRaw -xmin 0.0 -xmax 2.0 -ymin 22 -ymax 14
         titlePlot "Nmatched = $Nall, Nstars(H<$Hmax, mag($band)>$mmin) = $Nplot" 20        
         plot_subsamples $pg $gr $mpsf(r) $masks $titles $colors $symbols 0.0 2.0 22 14

         # 2MASS
         pgstateSet $pg -isNewplot 1
         vPlot $pg $JKr $Jr -xmin 0 -xmax 2.0 -ymin 16 -ymax 9 
         plot_subsamples $pg $JK $J $masks $titles $colors $symbols 0.0 2.0 16 9

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }


  if {1} {
     # plot RA-Dec differences
     if [catch {
         set diffRA [vectorExprEval ($ra-$ra2MASS)*3600]
         set diffDec [vectorExprEval ($dec-$dec2MASS)*3600]

         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_RADec.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.35 -yfract 0.3 -nxwindow 2 -nywindow 2 
         pgstateOpen $pg

         # set up histogram
         set nbin 50; set min -0.5; set max 0.5

         ### fill and plot the histograms
         # RA
         set hg [hgNew]
         set xlab "RA(SDSS)-RA(2MASS) (arcsec)"
         hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlab -yLabel "N"
         hgFillFromV $hg $diffRA
         hgPlot $pg $hg -xmin $min -xmax $max 
         titlePlot "SDSS vs. 2MASS positions for selected stars (n=$Nplot)" 40 
         plot_line $pg -2.0 0.0 2.0 0.0
         plot_line $pg 0.0 0.0 0.0 10000.0
         hgDel $hg 
         # Dec
         set hg [hgNew]
         set xlab "Dec(SDSS)-Dec(2MASS) (arcsec)"
         hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlab -yLabel "N"
         hgFillFromV $hg $diffDec
         hgPlot $pg $hg -xmin $min -xmax $max 
         plot_line $pg -2.0 0.0 2.0 0.0
         plot_line $pg 0.0 0.0 0.0 10000.0
         hgDel $hg 
 
         vectorExprDel $diffRA; vectorExprDel $diffDec;
         endplot $pg
     } msg] {
         echo "$msg"
     }
  }
  if {1} {
     # plot z vs. RA
     if [catch {

         if {$device == "/VCPS"} {
             set deviceB /CPS 
         } else {
             set deviceB $device
         }

         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $deviceB SDSS_2MASS_sky.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.35 -yfract 0.35 -nxwindow 1 -nywindow 2 
         pgstateOpen $pg

         vNameSet $ra "RA (deg)"
         vNameSet $dec "Dec (deg)"

         set RAmin 210.0
         set RAmax 195.0
         set Decmin -1.0
         set Decmax 0.0
 
         # Dec vs. RA
         plot_subsamples $pg $ra $dec $masks $titles $colors $symbols $RAmin $RAmax $Decmin $Decmax 
         titlePlot "Nmatched = $Nall, Nstars(H<$Hmax, mag($band)>$mmin) = $Nplot" 20        

         # z vs. RA
         pgstateSet $pg -isNewplot 1
         set Nsymbols [list 1 1 1 1 1 1 1 1 3]
         plot_subsamples $pg $ra $mpsf(z) $masks $titles $colors $Nsymbols $RAmin $RAmax 17.0 14.0

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }
     

     # lots of cleaning...
     chainDel $chain; chainDel $aux; vectorExprDel $SDSSmask 
     vectorExprDel $J; vectorExprDel $H; vectorExprDel $K 
     vectorExprDel $Jerr; vectorExprDel $Herr; vectorExprDel $Kerr 
     vectorExprDel $JH; vectorExprDel $HK
     vectorExprDel $OBmask; vectorExprDel $Amask; vectorExprDel $Fmask;
     vectorExprDel $Gmask; vectorExprDel $eKmask; vectorExprDel $lKmask;
     vectorExprDel $eMmask; vectorExprDel $lMmask
     vectorExprDel $RedMask; vectorExprDel $BlueMask;
     vectorExprDel $ug; vectorExprDel $gr; vectorExprDel $ri; vectorExprDel $iz 
     vectorExprDel $Jr; vectorExprDel $Hr
     vectorExprDel $Kr; vectorExprDel $JKr; vectorExprDel $JK; vectorExprDel $rK
     vectorExprDel $gRaw; vectorExprDel $rRaw; vectorExprDel $grRaw
     vectorExprDel $ra; vectorExprDel $dec; vectorExprDel $objc_type
     vectorExprDel $ra2MASS; vectorExprDel $dec2MASS;
     vectorExprDel $maskS; vectorExprDel $maskG 
     vectorExprDel $zJ; vectorExprDel $zK; 
     vectorExprDel $rd_flg
     vectorExprDel $bl_flg
     vectorExprDel $cc_flg
     vectorExprDel $extd_flg
     vectorExprDel $mp_flg
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
        vectorExprDel $mpsf($f)
        vectorExprDel $epsf($f)
     }     

}


# first read a chain :
# set tsObj tsObj-TWOMASSpsc_hour12_hour13.par_tsObj_run756_col1.fit
# set chain [tsObj2chain $tsObj]
# OR set chain [tsObjList2chain $tsObjList]
####  plots only Red Sources #### 
proc plot_2MASS_SDSS_Red {inchain device {dump 1} \
        {zmax 20.8} {izmin 2.0} {Jmax 16.55} {Hmax 15.85} {Kmax 15.05}} {

     ### N.B. default values for 2MASS cutoff are 5 sigma limits
     ###      and for SDSS z also 5 sigma
     set filterlist {u g r i z}
     set type "psfCounts"
     set band z
     set mmin 14.0
     set mmax $zmax

     set Nin [chainSize $inchain] 

     ### first cut on SDSS flags
     set flagsOFF [list SATUR BLENDED EDGE BRIGHT]
     set ingood [clean_tsObj_chain $inchain z 0 100 i z -10.0 10.0 10.0 10.0 {} $flagsOFF] 
     echo " ** cut on SDSS flags **"
     set Nall [chainSize $ingood]       
     echo " Selected $Nall sources with good flags from $Nin sources" 
     
     ### cut on 2MASS SNR
     set cond "{fiberCounts<0> < $Jmax} {fiberCounts<1> < $Hmax} {fiberCounts<2> < $Kmax}" 
     set aux [chainSearch $ingood $cond]
     set Naux [chainSize $aux] 
     echo "-----------------------------------------------------------"
     echo " ** cut on 2MASS SNR **"
     echo " Selected $Naux with J<$Jmax, H<$Hmax and K<$Kmax from $Nall matched sources" 
     if {0} {
        # and on 2MASS flags
        set cond "{fracPSF<0> == 222} {fracPSF<1> == 111}"  
        set chain [chainSearch $aux $cond]
        set Ngood [chainSize $chain] 
        echo " ** cut on 2MASS flags (222 and 111) **"
        echo " Selected $Ngood sources (from $Naux) with good 2MASS flags (222, 111)"
        echo "-----------------------------------------------------------"
        chainDel $aux
     } else {
        set chain $aux
        set Ngood [chainSize $chain] 
     }

     ### SDSS data
     echo ""; echo " Analyzing SDSS data for the remaining $Ngood sources..."
     set deredden 0
     loop i 0 5 { 
         set f [lindex $filterlist $i] 
         if {$deredden} {
            set reddening [vFromChain $chain $reddening<$i>]
            set mpsfRaw  [vFromChain $chain $type<$i>]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            vectorExprDel $reddening; vectorExprDel $mpsfRaw
         } else {
            set mpsf($f) [vFromChain $chain $type<$i>]
         }
         set epsf($f) [vFromChain $chain ${type}Err<$i>]
         vNameSet $mpsf($f) "${f}'"; vNameSet $epsf($f) ""  
     }     

     # SDSS mask for arbitrary flux limits in the specified band
     set mask_aux [vectorExprEval "$mpsf($band) > $mmin ? 1 : 0"]
     set SDSSmask [vectorExprEval "$mpsf($band) < $mmax ? $mask_aux : 0"]
     set Naux [vNnonzero $mask_aux] 
     set Nplot [vNnonzero $SDSSmask] 
     echo "   ...there are $Naux sources with mag($band) > $mmin and $Nplot sources with $mmin < mag($band) < $mmax" 
     echo "   ** Therefore, plotting $Nplot sources out of $Nall provided matched sources **" 
     vectorExprDel $mask_aux 

     # colors
     set ug [vectorExprEval $mpsf(u)-$mpsf(g)]
     set gr [vectorExprEval $mpsf(g)-$mpsf(r)]
     set ri [vectorExprEval $mpsf(r)-$mpsf(i)]
     set iz [vectorExprEval $mpsf(i)-$mpsf(z)]
     vNameSet $ug "u'-g'"; vNameSet $gr "g'-r'"; 
     vNameSet $ri "r'-i'"; vNameSet $iz "i'-z'"; 

     ### 2MASS data 
     echo "  forming 2MASS data vectors"
     # clipped
     set J [vFromChain $chain fiberCounts<0>]
     set Jerr [vFromChain $chain fiberCountsErr<0>]
     set H [vFromChain $chain fiberCounts<1>]
     set Herr [vFromChain $chain fiberCountsErr<1>]
     set K [vFromChain $chain fiberCounts<2>]
     set Kerr [vFromChain $chain fiberCountsErr<2>]
     set rd_flg [vFromChain $chain fracPSF<0>]
     set bl_flg [vFromChain $chain fracPSF<1>]
     set cc_flg [vFromChain $chain fracPSF<2>]
     set extd_flg [vFromChain $chain fracPSF<3>]
     set mp_flg [vFromChain $chain fracPSF<4>]
     set JH [vectorExprEval $J-$H]
     set HK [vectorExprEval $H-$K]
     set JK [vectorExprEval $J-$K]
     vNameSet $JH "J-H"; vNameSet $HK "H-K"; vNameSet $JK "J-K"; 
     vNameSet $J "J"; vNameSet $H "H"; vNameSet $K "K"; 
     # some SDSS-2MASS colors
     set rK [vectorExprEval $mpsf(r)-$K]
     set zJ [vectorExprEval $mpsf(z)-$J]
     set zK [vectorExprEval $mpsf(z)-$K]
     vNameSet $rK "r'-K"; vNameSet $zJ "z'-J"; vNameSet $zK "z'-K"; 
     # errors
     set izErr [vectorExprEval sqrt($epsf(i)*$epsf(i)+$epsf(z)*$epsf(z))]
     set JKErr [vectorExprEval sqrt($Jerr*$Jerr+$Kerr*$Kerr)]

     ## now split to blue and red subsamples according to i-z color and flux limit
     echo "   finding EROs..."
     set RedMask [vectorExprEval "$iz > $izmin ? $SDSSmask : 0"]
     set BlueMask [vectorExprEval "$RedMask == 1 ? 0 : $SDSSmask"]
     set nAll [vNnonzero $SDSSmask] 
     set nB [vNnonzero $BlueMask] 
     set nR [vNnonzero $RedMask] 
     echo "   Sanity check: All = $nAll, B+R = [expr $nB + $nR], blue = $nB, red = $nR"
     echo " Selected $nB sources with ($mmin < z < $mmax) from $nAll sources."
     echo " Selected $nR EROs (i-z > $izmin, $mmin < z < $mmax) from $nAll sources."

     # plot 6 color-color diagrams for red sources on one page 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS2MASS_RedStars_6cc.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.3 -yfract 0.35 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         set masks [list $BlueMask $RedMask]
         set colors [list 2 4]
         if {$nR > 100} {
             set symbols [list -1 -1]
         } else {
             set symbols [list -1 2]
         }
         set titles [list "SDSS-2MASS match (n=$nAll)" ""] 

         # J-K vs. r-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $rK $JK $masks $titles $colors $symbols -0.5 10.5 -0.4 2.0

         # J-K vs. z-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zK $JK $masks $titles $colors $symbols -1.0 10.0 -0.4 2.0

         # J-K vs. i-z
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $iz $JK $masks $titles $colors $symbols 0.0 4.0 -0.4 2.0

         # i-z vs. z
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $iz $mpsf(z) $masks $titles $colors $symbols 0.0 4.0 22.0 14.0

         # z vs. z-K 
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zK $mpsf(z) $masks $titles $colors $symbols -1.0 10.0  22.0 14.0

         # J-H vs. J
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $zK $K $masks $titles $colors $symbols -1.0 10.0 17.0 8.0

         endplot $pg
     } msg] {
         echo "$msg"
     }

     # plot J-K vs. i-z, z vs. i-z and J vs. J-K for red sources on one page 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS2MASS_RedStars_JKiz.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb -1 -icError 1 \
          -xfract 0.5 -yfract 0.35 -nxwindow 1 -nywindow 3
         pgstateOpen $pg

         # J-K vs. i-z
         vPlot $pg $iz $JK -vectorMask $RedMask -vectorXErr $izErr -vectorYErr $JKErr \
                           -xmin 1.0 -xmax 5.5 -ymin -0.5 -ymax 2.5

         # z vs. i-z
         vPlot $pg $iz $mpsf(z) -vectorMask $RedMask -vectorXErr $izErr -vectorYErr $epsf(z) \
                           -xmin 1.0 -xmax 5.5 -ymin 23.0 -ymax 13.0

         # K vs. J-K 
         vPlot $pg $JK $K -vectorMask $RedMask -vectorXErr $JKErr -vectorYErr $Kerr \
                          -xmin -0.5 -xmax 2.5 -ymin 17.0 -ymax 8.0

         endplot $pg
     } msg] {
         echo "$msg"
     }

     if {$dump} {
         set selected [clean_tsObj_chain $chain $band $mmin $mmax i z $izmin 10 10 10 {} {}] 
     echo " ** dumping [chainSize $selected] sources **"
         print_photo $selected 0 -1 EROs.dump 1
         chainDel $selected    
     }


     # lots of cleaning...
     chainDel $ingood; chainDel $chain
     vectorExprDel $SDSSmask 
     vectorExprDel $J; vectorExprDel $H; vectorExprDel $K 
     vectorExprDel $Jerr; vectorExprDel $Herr; vectorExprDel $Kerr 
     vectorExprDel $JH; vectorExprDel $HK
     vectorExprDel $izErr; vectorExprDel $JKErr; vectorExprDel $JK; 
     vectorExprDel $ug; vectorExprDel $gr; vectorExprDel $ri; vectorExprDel $iz; 
     vectorExprDel $rK; vectorExprDel $zJ; vectorExprDel $zK; 
     vectorExprDel $RedMask; vectorExprDel $BlueMask;
     vectorExprDel $rd_flg
     vectorExprDel $bl_flg
     vectorExprDel $cc_flg
     vectorExprDel $extd_flg
     vectorExprDel $mp_flg
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
        vectorExprDel $mpsf($f)
        vectorExprDel $epsf($f)
     }
}


# first read a chain :
# set tsObj tsObj-TWOMASSpsc_hour12_hour13.par_tsObj_run756_col1.fit
# set chain [tsObj2chain $tsObj]
# OR set chain [tsObjList2chain $tsObjList]
####   plots only matches classified as galaxies #### 
proc plot_2MASS_SDSS_Gal {inchain device {2MASSerr 0.5} {band z} {mmin 13.0} {mmax 22.0} {Hmax 16.0}} {

     set filterlist {u g r i z}
     set type "counts_model"

     set Nall [chainSize $inchain] 
     # raw values
     set Jr [vFromChain $inchain fiberCounts<0>]
     set Hr [vFromChain $inchain fiberCounts<1>]
     set Kr [vFromChain $inchain fiberCounts<2>]
     set JKr [vectorExprEval $Jr-$Kr]
     vNameSet $Jr "J"; vNameSet $Hr "H"; vNameSet $Kr "K"; vNameSet $JKr "J-K"; 
     set gRaw [vFromChain $inchain $type<1>]
     set rRaw [vFromChain $inchain $type<2>]
     set grRaw [vectorExprEval $gRaw-$rRaw]
     vNameSet $gRaw "g'"; vNameSet $rRaw "r'"; vNameSet $grRaw "g'-r'"; 

     # first cut on 2MASS errors and H < Hmax
     set err_max $2MASSerr
     set cond "{fiberCounts<1> < $Hmax} {fiberCountsErr<0> < $err_max} {fiberCountsErr<1> < $err_max} {fiberCountsErr<1> < $err_max}" 
     set aux [chainSearch $inchain $cond]
     set Naux [chainSize $aux] 
     echo "-----------------------------------------------------------"
     echo " ** cut on limiting H mag **"
     echo "Selected $Naux sources with H < $Hmax and 2MASS errors < $err_max from $Nall matched sources" 
     # and on 2MASS flags
     set cond "{fracPSF<0> == 222} {fracPSF<1> == 111}"  
     set chain [chainSearch $aux $cond]
     set Ngood [chainSize $chain] 
     echo " ** cut on 2MASS flags (222 and 111) **"
     echo "Selected $Ngood sources with H < $Hmax and good 2MASS flags (222, 111)"
     echo "-----------------------------------------------------------"

     ### SDSS data
     echo ""; echo " Analyzing SDSS data for the remaining $Ngood sources..."
     set deredden 0
     loop i 0 5 { 
         set f [lindex $filterlist $i] 
         if {$deredden} {
            set reddening [vFromChain $chain $reddening<$i>]
            set mpsfRaw  [vFromChain $chain $type<$i>]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            vectorExprDel $reddening; vectorExprDel $mpsfRaw
         } else {
            set mpsf($f) [vFromChain $chain $type<$i>]
         }
         set epsf($f) [vFromChain $chain ${type}Err<$i>]
         vNameSet $mpsf($f) "${f}'"; vNameSet $epsf($f) ""  
     }     
     set ra [vFromChain $chain ra]
     set dec [vFromChain $chain dec]
     set objc_type [vFromChain $chain objc_type]

     # SDSS mask for plotting
     set maskS  [vectorExprEval "$objc_type == 6 ? 1 : 0"]
     set maskG  [vectorExprEval "$objc_type == 3 ? 1 : 0"]
     set Ns [vNnonzero $maskS] 
     set Ng [vNnonzero $maskG] 
     echo "   ...there are $Ns stars and $Ng galaxies (n.b. [expr $Ns+$Ng] classified out of $Ngood)"
     set mask_aux [vectorExprEval "$mpsf($band) > $mmin ? $maskG : 0"]
     set SDSSmask [vectorExprEval "$mpsf($band) < $mmax ? $mask_aux : 0"]
     set Naux [vNnonzero $mask_aux] 
     set Nplot [vNnonzero $SDSSmask] 
     echo "   ...there are $Naux galaxies with mag($band) > $mmin and $Nplot sources with $mmin < mag($band) < $mmax" 
     echo "   ** Therefore, plotting $Nplot galaxies out of $Nall provided matched sources **" 
     vectorExprDel $mask_aux 

     # colors
     set ug [vectorExprEval $mpsf(u)-$mpsf(g)]
     set gr [vectorExprEval $mpsf(g)-$mpsf(r)]
     set ri [vectorExprEval $mpsf(r)-$mpsf(i)]
     set iz [vectorExprEval $mpsf(i)-$mpsf(z)]
     vNameSet $ug "u'-g'"; vNameSet $gr "g'-r'"; 
     vNameSet $ri "r'-i'"; vNameSet $iz "i'-z'"; 

     ### 2MASS data 
     echo "  forming 2MASS data vectors"
     # clipped
     set ra2MASS [vFromChain $chain lambda]
     set dec2MASS [vFromChain $chain eta]
     set J [vFromChain $chain fiberCounts<0>]
     set Jerr [vFromChain $chain fiberCountsErr<0>]
     set H [vFromChain $chain fiberCounts<1>]
     set Herr [vFromChain $chain fiberCountsErr<1>]
     set K [vFromChain $chain fiberCounts<2>]
     set Kerr [vFromChain $chain fiberCountsErr<2>]
     set rd_flg [vFromChain $chain fracPSF<0>]
     set bl_flg [vFromChain $chain fracPSF<1>]
     set cc_flg [vFromChain $chain fracPSF<2>]
     set extd_flg [vFromChain $chain fracPSF<3>]
     set mp_flg [vFromChain $chain fracPSF<4>]
     set JH [vectorExprEval $J-$H]
     set HK [vectorExprEval $H-$K]
     set JK [vectorExprEval $J-$K]
     vNameSet $JH "J-H"; vNameSet $HK "H-K"; vNameSet $JK "J-K"; 
     vNameSet $J "J"; vNameSet $H "H"; vNameSet $K "K"; 
     # some SDSS-2MASS colors
     set rK [vectorExprEval $mpsf(r)-$K]
     set zJ [vectorExprEval $mpsf(r)-$J]
     vNameSet $rK "r'-K"; vNameSet $zJ "z'-J"; 

  if {1} {
     # plot color-color diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_ccG.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.4 -yfract 0.33 -nxwindow 3 -nywindow 4 
         pgstateOpen $pg

         # SDSS colors
         vPlot $pg $ug $gr -vectorMask $SDSSmask -xmin -0.5 -xmax 4.0 -ymin -1.0 -ymax 2.5
         titlePlot "SDSS - 2MASS match: n=$Nplot" 20        

         vPlot $pg $gr $ri -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin -1.0 -ymax 2.5
         vPlot $pg $ri $iz -vectorMask $SDSSmask -xmin -1.0 -xmax 3.5 -ymin -1.0 -ymax 4.5

         # J-J combinations
         vPlot $pg $JH $gr -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin -1.0 -ymax 2.5
         vPlot $pg $JH $iz -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin -1.0 -ymax 4.5
         vPlot $pg $JH $zJ -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin 0.0 -ymax 4.5
         
         # JHK etc. 
         vPlot $pg $mpsf(z) $J -vectorMask $SDSSmask -xmin 13.0 -xmax 22.0 -ymin 10.0 -ymax 19.0          
         vPlot $pg $rK $JK -vectorMask $SDSSmask -xmin -1.0 -xmax 10.5 -ymin 2.5 -ymax -0.3 
         vPlot $pg $JH $HK -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin -0.5 -ymax 1.5 

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }
 
  if {1} {
     # plot color-magnitude diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_cmG.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.4 -yfract 0.33 -nxwindow 3 -nywindow 4 
         pgstateOpen $pg

         # SDSS colors
         vPlot $pg $gr $mpsf(g) -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin 23 -ymax 13

         titlePlot "SDSS - 2MASS match: n=$Nplot" 20        

         vPlot $pg $gr $mpsf(r) -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin 23 -ymax 13
         vPlot $pg $gr $mpsf(z) -vectorMask $SDSSmask -xmin -1.0 -xmax 2.5 -ymin 23 -ymax 13

         # J-J combinations
         vPlot $pg $iz $mpsf(i) -vectorMask $SDSSmask -xmin -1.0 -xmax 4.5 -ymin 23 -ymax 13
         vPlot $pg $iz $mpsf(z) -vectorMask $SDSSmask -xmin -1.0 -xmax 4.5 -ymin 20 -ymax 11
         vPlot $pg $iz $J -vectorMask $SDSSmask -xmin -1.0 -xmax 4.5 -ymin 19 -ymax 5
         
         # JHK etc. 
         vPlot $pg $JH $mpsf(z) -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5  -ymin 20 -ymax 11         
         vPlot $pg $JH $J -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin 19 -ymax 5
         vPlot $pg $JH $H -vectorMask $SDSSmask -xmin -0.5 -xmax 1.5 -ymin 19 -ymax 5 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 }

 
 
  #### PAPER PLOTS ####

      set titles [list "SDSS-2MASS galaxies, n =$Nplot"] 
      set masks  [list $SDSSmask] 
      set colors  [list  1  5  9  4  3  7  6  2  8]
      set symbols [list -1 -1 -1 -1 -1 -1 -1 -1  3]
 

   if {1} {
     # plot 6 color-color diagrams on one page 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS2MASS_6CCdiags_dataGalaxies.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.3 -yfract 0.35 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # r-i vs. g-r
         plot_subsamples $pg $gr $ri $masks $titles $colors $symbols 0.2 2.8 -0.2 3.2 
         titlePlot "SDSS - 2MASS matched SDSS galaxies: n=$Nplot" 20        

         # J-K vs. g-r 
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $gr $JK $masks $titles $colors $symbols 0.2 2.8 -0.2 1.5

         # r-i vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $JK $ri $masks $titles $colors $symbols -0.2 1.5 -0.2 3.2

         # z-J vs. J-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $JK $zJ $masks $titles $colors $symbols -0.2 1.5 0.0 6.0 

         # J-K vs. r-K
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $rK $JK $masks $titles $colors $symbols 0.5 9.5 2.0 0.0 

         # H-K vs. J-H
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $JH $HK $masks $titles $colors $symbols 0.0 1.0 -0.3 0.7

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }


  if {1} { 
      # plot data histograms for g-r, J-H, H-K and J-K colors
      if [catch {

         set histMin -0.1
         set histMax 1.3
         set histNbin 28

 
         ### data
         # g-r
         plot_many_histograms $device $gr $masks $titles $histNbin $histMin $histMax "g'-r'" 
         my_mv many_histograms.ps gr_hist_manyDataG.ps
         # r-i
         plot_many_histograms $device $ri $masks $titles $histNbin $histMin $histMax "r'-i'" 
         my_mv many_histograms.ps ri_hist_manyDataG.ps
         # J-H
         plot_many_histograms $device $JH $masks $titles $histNbin $histMin $histMax "J-H" 
         my_mv many_histograms.ps JHhist_manyDataG.ps
         # H-K
         plot_many_histograms $device $HK $masks $titles 18 -0.2 0.7 "H-K" 
         my_mv many_histograms.ps HKhist_manyDataG.ps
         # J-K
         plot_many_histograms $device $JK $masks $titles 32 -0.1 1.5 "J-K"
         my_mv many_histograms.ps JKhist_manyDataG.ps
     } msg] {
         echo "$msg"
     }
  }

 
  if {1} {
     # plot color-magnitude diagrams  
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_4cmGal.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.35 -yfract 0.3 -nxwindow 2 -nywindow 3 
         pgstateOpen $pg

         # SDSS 
         vPlot $pg $grRaw $rRaw -xmin 0.0 -xmax 2.0 -ymin 22 -ymax 14
         titlePlot "Nmatched = $Nall, Ngalaxies(H<$Hmax, mag($band)>$mmin) = $Nplot" 20        
         plot_subsamples $pg $gr $mpsf(r) $masks $titles $colors $symbols 0.0 2.0 22 14

         # 2MASS
         pgstateSet $pg -isNewplot 1
         vPlot $pg $JKr $Jr -xmin 0 -xmax 2.0 -ymin 16 -ymax 9 
         plot_subsamples $pg $JK $J $masks $titles $colors $symbols 0.0 2.0 19 9

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }


  if {1} {
     # plot RA-Dec differences
     if [catch {
         set diffRA [vectorExprEval ($ra-$ra2MASS)*3600]
         set diffDec [vectorExprEval ($dec-$dec2MASS)*3600]

         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device SDSS_2MASS_RADecGal.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.35 -yfract 0.3 -nxwindow 2 -nywindow 2 
         pgstateOpen $pg

         # set up histogram
         set nbin 50; set min -0.5; set max 0.5

         ### fill and plot the histograms
         # RA
         set hg [hgNew]
         set xlab "RA(SDSS)-RA(2MASS) (arcsec)"
         hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlab -yLabel "N"
         hgFillFromV $hg $diffRA -vMask $SDSSmask
         hgPlot $pg $hg -xmin $min -xmax $max 
         titlePlot "SDSS vs. 2MASS positions for selected galaxies (n=$Nplot)" 40 
         plot_line $pg -2.0 0.0 2.0 0.0
         plot_line $pg 0.0 0.0 0.0 10000.0
         hgDel $hg 
         # Dec
         set hg [hgNew]
         set xlab "Dec(SDSS)-Dec(2MASS) (arcsec)"
         hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlab -yLabel "N"
         hgFillFromV $hg $diffDec -vMask $SDSSmask
         hgPlot $pg $hg -xmin $min -xmax $max 
         plot_line $pg -2.0 0.0 2.0 0.0
         plot_line $pg 0.0 0.0 0.0 10000.0
         hgDel $hg 
 
         vectorExprDel $diffRA; vectorExprDel $diffDec;
         endplot $pg
     } msg] {
         echo "$msg"
     }
  }
  if {1} {
     # plot z vs. RA
     if [catch {

         if {$device == "/VCPS"} {
             set deviceB /CPS 
         } else {
             set deviceB $device
         }

         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $deviceB SDSS_2MASS_skyGal.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.35 -yfract 0.35 -nxwindow 1 -nywindow 2 
         pgstateOpen $pg

         vNameSet $ra "RA (deg)"
         vNameSet $dec "Dec (deg)"

         set RAmin 195.0
         set RAmax 180.0
         set Decmin -1.0
         set Decmax 0.0
 
         # Dec vs. RA
         plot_subsamples $pg $ra $dec $masks $titles $colors $symbols $RAmin $RAmax $Decmin $Decmax 
         titlePlot "Nmatched = $Nall, Ngalaxies(H<$Hmax, mag($band)>$mmin) = $Nplot" 20        

         # z vs. RA
         pgstateSet $pg -isNewplot 1
         plot_subsamples $pg $ra $mpsf(z) $masks $titles $colors $symbols $RAmin $RAmax 17.0 14.0

         endplot $pg
     } msg] {
         echo "$msg"
     }
  }
     

     # lots of cleaning...
     chainDel $chain; chainDel $aux; vectorExprDel $SDSSmask 
     vectorExprDel $J; vectorExprDel $H; vectorExprDel $K 
     vectorExprDel $Jerr; vectorExprDel $Herr; vectorExprDel $Kerr 
     vectorExprDel $JH; vectorExprDel $HK
     vectorExprDel $Jr; vectorExprDel $Hr
     vectorExprDel $Kr; vectorExprDel $JKr
     vectorExprDel $gRaw; vectorExprDel $rRaw; vectorExprDel $grRaw
     vectorExprDel $ra; vectorExprDel $dec; vectorExprDel $objc_type
     vectorExprDel $ra2MASS; vectorExprDel $dec2MASS;
     vectorExprDel $maskS; vectorExprDel $maskG 
     vectorExprDel $rd_flg
     vectorExprDel $bl_flg
     vectorExprDel $cc_flg
     vectorExprDel $extd_flg
     vectorExprDel $mp_flg
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
        vectorExprDel $mpsf($f)
        vectorExprDel $epsf($f)
     }

}


# plot adopted classification in i-r vs. g-r color-color diagram
proc plot_SpT {device {Nstars 17277} {file ""}} { 

     if [catch {
         set nf "%6d"

         # get models, Nstars default is number of stars per deg2 centered on RA=187, Dec=0 (l=292, b=62) 
         echo "calling get_model for $Nstars sources"
         set modelList [get_modelSpT $Nstars $file]
         set modelmasks  [lindex $modelList 0]
         set SDSS2MASSmasks  [lindex $modelList 1]
         set modelcolors [lindex $modelList 2]

         # extract needed info
         set grmodel [lindex $modelcolors 1]
         set rimodel [lindex $modelcolors 2]
         set izmodel [lindex $modelcolors 3]
         set zJmodel [lindex $modelcolors 4]
         set rmodel [lindex $modelcolors [expr [llength $modelcolors]-1]]
         vNameSet $grmodel "g'-r'"; vNameSet $rimodel "r'-i'" 
         vNameSet $izmodel "i'-z'"; vNameSet $zJmodel "z'-J" 
         set JHmodel [lindex $modelcolors 5]
         set HKmodel [lindex $modelcolors 6]
         set JKmodel [vectorExprEval $JHmodel+$HKmodel]
         vNameSet $JHmodel "J-H"; vNameSet $JKmodel "J-K";
         # r-K
         set rKmodel [vectorExprEval $rimodel+$izmodel+$zJmodel+$JHmodel+$HKmodel]
         vNameSet $rKmodel "r'-K"; 
         set rzmodel [vectorExprEval $rimodel+$izmodel]
         vNameSet $rzmodel "r'-z'"; 

         set MOBmask [lindex $modelmasks 0]
         set MAmask  [lindex $modelmasks 1]
         set MFmask  [lindex $modelmasks 2]
         set MGmask  [lindex $modelmasks 3]
         set MKmask  [lindex $modelmasks 4]
         set MMmask  [lindex $modelmasks 5]
         set MmaskAll [vectorExprEval $MOBmask+$MAmask+$MFmask+$MGmask+$MKmask+$MMmask]

         set MOBmask2 [lindex $SDSS2MASSmasks 0]
         set MAmask2  [lindex $SDSS2MASSmasks 1]
         set MFmask2  [lindex $SDSS2MASSmasks 2]
         set MGmask2  [lindex $SDSS2MASSmasks 3]
         set MKmask2  [lindex $SDSS2MASSmasks 4]
         set MMmask2  [lindex $SDSS2MASSmasks 5]
         set MmaskAll2 [vectorExprEval $MOBmask2+$MAmask2+$MFmask2+$MGmask2+$MKmask2+$MMmask2]
         set MnTot2 [vNnonzero $MmaskAll2]

         set MnOB [format $nf [vNnonzero $MOBmask]]
         set MnA  [format $nf [vNnonzero $MAmask]]
         set MnF  [format $nf [vNnonzero $MFmask]]
         set MnG  [format $nf [vNnonzero $MGmask]]
         set MnK  [format $nf [vNnonzero $MKmask]]
         set MnM  [format $nf [vNnonzero $MMmask]]
         set MnTot [expr $MnOB+$MnA+$MnF+$MnG+$MnK+$MnM]
         set pf "%4.1f"     

         if {0} {    
         echo ""; echo " *** SDSS prediction ***" 
         echo "      OB    $MnOB  [format $pf [expr 100.0*$MnOB/$MnTot]]"         
         echo "       A    $MnA   [format $pf [expr 100.0*$MnA/$MnTot]]"         
         echo "       F    $MnF   [format $pf [expr 100.0*$MnF/$MnTot]]"         
         echo "       G    $MnG   [format $pf [expr 100.0*$MnG/$MnTot]]"         
         echo "       K    $MnK   [format $pf [expr 100.0*$MnK/$MnTot]]"         
         echo "       M    $MnM   [format $pf [expr 100.0*$MnM/$MnTot]]"         
         echo "     all    $MnTot [format $pf [expr 100.0*$MnTot/$NmodelAll]] (of total)"        
         }



        #### modelSpT_general.ps
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device modelSpT_general.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.3 -yfract 0.35 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         set titles {}
         set colors  [list  8  4  3  7  6  2]
         set symbols [list -1 -1 -1 -1 -1 -1]

         ## r-i vs. g-r for SDSS sample
         plot_subsamples $pg $grmodel $rimodel $modelmasks $titles $colors $symbols -0.1 1.6 0.0 2.0 
         titlePlot "SDSS model: n=$MnTot" 40        
         # SDSS-based spectral classification
         plot_line $pg -10.0   0.7  10.0  0.7
         plot_line $pg   0.95  1.2   1.65 1.2
         plot_line $pg   0.1  -0.3   0.1  0.7
         plot_line $pg   0.3  -0.3   0.3  0.7
         plot_line $pg   0.45 -0.3   0.45 0.7
         plot_line $pg   0.65 -0.3   0.65 0.7
         plot_line $pg   1.2  -0.3   1.2  0.7
         ## r-i vs. g-r for SDSS-2MASS sample
         plot_subsamples $pg $grmodel $rimodel $SDSS2MASSmasks $titles $colors $symbols -0.1 1.6 0.0 2.0 
         titlePlot "2MASS-SDSS model: n=$MnTot2" 40       

         ## r vs. g-r for SDSS sample
         plot_subsamples $pg $grmodel $rmodel $modelmasks $titles $colors $symbols -0.1 1.6  24.0 13.0

         ## r vs. g-r for SDSS-2MASS subsample
         plot_subsamples $pg $grmodel $rmodel $SDSS2MASSmasks $titles $colors $symbols -0.1 1.6 24.0 13.0

         ## H-K vs. J-K for SDSS-2MASS subsample
         plot_subsamples $pg $JHmodel $JKmodel $SDSS2MASSmasks $titles $colors $symbols 0.0 1.2 0.0 1.2

         ## i-z vs. J-K for SDSS-2MASS subsample
         plot_subsamples $pg $izmodel $JKmodel $SDSS2MASSmasks $titles $colors $symbols -0.5 4.0 0.0 1.2

         endplot $pg


         ### clean
         foreach m $modelcolors {
             vectorExprDel $m
         }
         foreach m $modelmasks {
             vectorExprDel $m
         } 
         foreach m $SDSS2MASSmasks {
             vectorExprDel $m
         } 
         vectorExprDel $JKmodel; vectorExprDel $rKmodel; vectorExprDel $rzmodel; 
         vectorExprDel $MmaskAll; vectorExprDel $MmaskAll2

     } msg] {
         echo "$msg"
     }
 
} 

# given a vector and a list of masks, plot histograms for each mask
proc plot_many_histograms {device vector masks titles \
              {nbin ""} {min ""} {max ""} {xlab ""} {logHist 0}} {

     if [catch {
         # number of windows
         set Nwin [llength $masks]
         set Nx [expr int(sqrt($Nwin))]
         set Ny [expr int($Nwin/$Nx)]
         if {[expr $Nx*$Ny] < $Nwin} {incr $Ny}
 
         # Set up the plot
         set pg [pgstateNew]
         set deviceA [devicename $device many_histograms.ps]
         pgstateSet $pg -device $deviceA -lineWidth 3 \
         -xfract 0.42 -yfract 0.38 -nxwindow $Nx -nywindow [expr $Ny + 1]
         pgstateOpen $pg

         # set up histogram
         if {$nbin == ""} {
            set nbin 10
         }
         if {$min == ""} {
            extremes $vector min aux 0.1 
         }
         if {$max == ""} {
            extremes $vector aux max 0.1 
         }
         if {$xlab == ""} {
            set xlab [exprGet $vector.name]
         }


         # fill and plot the histograms
     foreach mask $masks {
            set hg [hgNew]
            hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlab -yLabel N
        set i [lsearch $masks $mask]
        set title [lindex $titles $i]
            set mask1 [vectorExprEval "$vector < $max ? $mask : 0"]
            set mask2 [vectorExprEval "$vector > $min ? $mask1 : 0"]
            hgFillFromV $hg $vector -vMask $mask2            
            vectorExprDel $mask1
            vectorExprDel $mask2
        if {$logHist} {
           set Lhg [hgLog10 $hg]                               
               hgDefine $Lhg -min $min -max $max -nbin $nbin 
               handleSet $Lhg.xLabel $xlab
               handleSet $Lhg.yLabel "log(N)" 
               hgPlot $pg $Lhg -xmin $min -xmax $max 
               hgDel $Lhg 
            } else {
               hgPlot $pg $hg -xmin $min -xmax $max 
            }
            titlePlot $title 40 
            hgDel $hg 
         }
            
         endplot $pg
     } msg] {
         echo "$msg"
     }

}


proc plot_subsamples {pg vX vY masks titles colors symbols xmin xmax ymin ymax} {

     if [catch {
         # fill and plot the histograms
     foreach mask $masks {
        set i [lsearch $masks $mask]
            # note that titles are not plotted
        if {$titles != {}} {set title [lindex $titles $i]}
        if {$colors != {}} {
               set color [lindex $colors $i]
               pgstateSet $pg -isLine 0 -icMark $color
        }
        if {$symbols != {}} {
               set symbol [lindex $symbols $i]
               pgstateSet $pg -symb $symbol
        }
        if {$i > 0} {
                pgstateSet $pg -isNewplot 0
            } else {
                pgstateSet $pg -isNewplot 1
            }      
            vPlot $pg $vX $vY -vectorMask $mask -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax
         }
     } msg] {
         echo "$msg"
     }

}



proc my_mv {file1 file2} {
   if {[file exists $file1]} {
      catch {unlink $file2}
      set in [open $file1]
      set out [open $file2 w]
      copyfile $in $out
      close $in; close $out
   }
} 


# select elements with primary flag on. For secondaries == 1, return
# sources with primary flag off.
proc get_primaries {chain {secondaries 0}} {

     set N 0
     set good [chainNew [exprGet $chain.type]]

     loop i 0 [chainSize $chain] {
        set el [chainElementGetByPos $chain $i]
        set primary [expr ([exprGet $el.status] & 8192) != 0]   
        if {$secondaries} {
           if {!$primary} { 
              chainElementAddByPos $good $el
           }
        } else {
           if {$primary} { 
              chainElementAddByPos $good $el
           }
        }
     }

     return $good

}



# select elements from a chain of TWOMASSPSC by flags
proc selectTWOMASSPSC {chain rd_flg bl_flg cc_flg extd_flg mp_flg} {

    set good [chainNew [exprGet $chain.type]]

    echo "selecting flags for [chainSize $chain] sources..."
    loop i 0 [chainSize $chain] {
        if {[expr $i/10000*10000] == $i} {echo $i}
        set el [chainElementGetByPos $chain $i]
    if {[exprGet $el.rd_flg] == $rd_flg && \
            [exprGet $el.bl_flg] == $bl_flg && \
            [exprGet $el.cc_flg] == $cc_flg && \
            [exprGet $el.extd_flg] == $extd_flg && \
        [exprGet $el.mp_flg] == $mp_flg} {
            chainElementAddByPos $good $el

        }
    }    

    return $good
}


### from Kristian for reading and precoessing model files
# fields: population id, distance, log(T),Mass,[Fe/H],log(g),u',g',r',i',z',J,H,Ks,V
typedef struct {
    int Ptype;
    int SpType;
    float D;
    float Teff;
    float M;
    float metal;
    float grav;
    float u;
    float g;
    float r;
    float i;
    float z;
    float J;
    float H;
    float K;
    float V;
} MODELSTRUCT;


# read model files and return model stars for SDSS-2MASS match
proc get_model {MAXN {infile ""}}  {

    set zmin 14.0
    set Hmax 14.3

    if {$infile == ""} {
       set infile "/peyton/scr/ginka1/finlator/Thesis/Xiaohui_code/star_simulation_l292_b62.dat"
       set Nmodels 388731
    } else {
       # find number of entries in the input file
       set Nmodels [lindex [exec wc -l $infile] 0]
    }

    set Nsparse [expr $Nmodels / $MAXN]
    set infile [open $infile r]

    set chain [chainNew MODELSTRUCT]
    set N 0
    while {![eof $infile] && $N<$MAXN} {
        # sparse reading to get unbiased subsample
        loop k 0 $Nsparse {
       set line [gets $infile]
        }
    set line [gets $infile]
        catch {
      set el [genericNew MODELSTRUCT]
      handleSet $el.u [lindex $line 6]
      handleSet $el.g [lindex $line 7]
      handleSet $el.r [lindex $line 8]
      handleSet $el.i [lindex $line 9]
      handleSet $el.z [lindex $line 10]
      handleSet $el.J [lindex $line 11]
      handleSet $el.H [lindex $line 12]
      handleSet $el.K [lindex $line 13]
      chainElementAddByPos $chain $el
        }
    if {[expr $N % 10000]==0} {echo $N}
    incr N
    }

    ####Convert the chain data into vectors
    foreach band {u g r i z J H K} {
    set $band [vFromChain $chain $band]
    }

    ####Create the color vectors
    set ug [vectorExprEval $u-$g]
    set gr [vectorExprEval $g-$r]
    set ri [vectorExprEval $r-$i]
    set iz [vectorExprEval $i-$z]
    set zJ [vectorExprEval $z-$J]
    set JH [vectorExprEval $J-$H]
    set HK [vectorExprEval $H-$K]


    ### create SDSSmask to exclude objects with z < zmin and H > Hmax
    # The z-cutoff is the SDSS z-band saturation limit, and the H cutoff is 
    # because 14.3 is the 10-sigma H-band 2MASS cutoff
    set aux_mask [vectorExprEval "$z > $zmin ? 1 : 0"] 
    set SDSSmask [vectorExprEval "$H < $Hmax ? $aux_mask : 0"]

    #######################################################################################
    ##This mask part copied verbatim from /u/ivezic/photo/etc/catalog_match.tcl 3/15/00####
    ## now split to blue and red subsamples according to r-i color
    echo "splitting into r-i vs g-r classes"
    set BlueMask [vectorExprEval "$ri <= 0.7 && $gr <= 1.2 ? $SDSSmask : 0"]
    set RedMask [vectorExprEval "$BlueMask == 1 ? 0 : $SDSSmask"]
      # OBA stars
      set OBmask [vectorExprEval "$gr > -1.2 && $gr <= 0.1 ? $BlueMask : 0"]
      # early F stars
      set Amask [vectorExprEval "$gr > 0.1  && $gr <= 0.3 ? $BlueMask : 0"]
      # late F and early G stars
      set Fmask [vectorExprEval "$gr > 0.3  && $gr <= 0.45 ? $BlueMask : 0"]
      # late G stars
      set Gmask [vectorExprEval "$gr > 0.45 && $gr <=  0.65 ? $BlueMask : 0"]
      # early K stars
      set eKmask [vectorExprEval "$gr > 0.65 && $gr <= 1.2 ? $BlueMask : 0"]
      # late K stars
      set lKmask [vectorExprEval "$gr > 1.2 && $ri <= 0.7 ? $RedMask : 0"]
      ## separate red stars to 2 bins according to their r-i color
      # early M stars   
      set eMmask [vectorExprEval "$ri > 0.7 && $ri <= 1.1 ? $RedMask : 0"]
      # late M stars      
      set lMmask [vectorExprEval "$ri > 1.1 && $ri <= 10.0 ? $RedMask : 0"]
    ###########################################################################

    set masklist [list $OBmask $Amask $Fmask $Gmask $eKmask $lKmask $eMmask $lMmask]
    set colorlist [list $ug $gr $ri $iz $zJ $JH $HK $J]
    set outputlist [list $masklist $colorlist]

    #cleaning up...
    vectorExprDel $H; vectorExprDel $K 
    vectorExprDel $u; vectorExprDel $g 
    vectorExprDel $r; vectorExprDel $i
    vectorExprDel $z; vectorExprDel $SDSSmask 
    vectorExprDel $aux_mask; vectorExprDel $BlueMask; vectorExprDel $RedMask
    genericChainDestroy $chain;

    return $outputlist
    
}

# return a list of 3 lists: SDSSmasklist SDSS2MASSmasklist colorlist
proc get_modelSpT {MAXN {infile ""}}  {

 
    set zmin 14.0
    set Hmax 14.3

    if {$infile == ""} {
       set infile "/peyton/scr/ginka1/finlator/Thesis/Xiaohui_code/star_simulation_l292_b62.dat"
       set Nmodels 388731
    } else {
       # find number of entries in the input file
       set Nmodels [lindex [exec wc -l $infile] 0]
    }

    set Nsparse [expr $Nmodels / $MAXN]
    set infile [open $infile r]

    set chain [chainNew MODELSTRUCT]
    set N 0
    while {![eof $infile] && $N<$MAXN} {
        # sparse reading to get unbiased subsample
        loop k 0 $Nsparse {
       set line [gets $infile]
        }
    set el [genericNew MODELSTRUCT]
    handleSet $el.Ptype [lindex $line 0]
    handleSet $el.D [lindex $line 1]
    handleSet $el.Teff [expr pow(10.0,[lindex $line 2])]
    handleSet $el.M [lindex $line 3]
    handleSet $el.metal [lindex $line 4]
    handleSet $el.grav [lindex $line 5]
    handleSet $el.u [lindex $line 6]
    handleSet $el.g [lindex $line 7]
    handleSet $el.r [lindex $line 8]
    handleSet $el.i [lindex $line 9]
    handleSet $el.z [lindex $line 10]
    handleSet $el.J [lindex $line 11]
    handleSet $el.H [lindex $line 12]
    handleSet $el.K [lindex $line 13]
        # OB = 0
        if {[exprGet $el.Teff] > 11000} {handleSet $el.SpType 0}
        # A = 1
        if {[exprGet $el.Teff] > 7500 && [exprGet $el.Teff] <= 11000 } {handleSet $el.SpType 1}
        # F = 2
        if {[exprGet $el.Teff] > 6000 && [exprGet $el.Teff] <= 7500 } {handleSet $el.SpType 2}
        # G = 3
        if {[exprGet $el.Teff] > 5000 && [exprGet $el.Teff] <= 6000 } {handleSet $el.SpType 3}
        # K = 4
        if {[exprGet $el.Teff] > 3500 && [exprGet $el.Teff] <= 5000 } {handleSet $el.SpType 4}
        # M = 5
        if {[exprGet $el.Teff] <= 3500} {handleSet $el.SpType 5}
        ### all set, add it to the chain
    chainElementAddByPos $chain $el
    if {[expr $N % 1000]==0} {echo $N}
    incr N
    }

    ####Convert the chain data into vectors
    foreach band {u g r i z J H K} {
    set $band [vFromChain $chain $band]
    }
    set SpType [vFromChain $chain SpType]

    ####Create the color vectors
    set ug [vectorExprEval $u-$g]
    set gr [vectorExprEval $g-$r]
    set ri [vectorExprEval $r-$i]
    set iz [vectorExprEval $i-$z]
    set zJ [vectorExprEval $z-$J]
    set JH [vectorExprEval $J-$H]
    set HK [vectorExprEval $H-$K]

    # create SDSSmask to exclude objects with z < zmin and H > Hmax
    # The z-cutoff is the SDSS z-band saturation limit, and the H cutoff is 
    # because 14.3 is the 10-sigma H-band 2MASS cutoff
    set SDSSmask [vectorExprEval "$z > $zmin ? 1 : 0"] 
    set aux_mask [vectorExprEval "$z < 23.0 ? $SDSSmask : 0"]     
    set SDSS2MASSmask [vectorExprEval "$H < $Hmax ? $aux_mask : 0"]

    #######################################################################################
    ## split to spectral classes: SDSS sources
    set Nall [vNnonzero $SDSSmask]
    echo "splitting $Nall stars into spectral classes"   
    set OBmask [vectorExprEval "$SpType == 0 ? $SDSSmask : 0"]
    set NOB [vNnonzero $OBmask]
    set p [format "%4.1f" [expr 100.0*$NOB/$Nall]]
    echo "  found $NOB OB stars ($p %)"
    set Amask  [vectorExprEval "$SpType == 1 ? $SDSSmask : 0"]
    set NA [vNnonzero $Amask]
    set p [format "%4.1f" [expr 100.0*$NA/$Nall]]
    echo "  found $NA A stars ($p %)"
    set Fmask  [vectorExprEval "$SpType == 2 ? $SDSSmask : 0"]
    set NF [vNnonzero $Fmask]
    set p [format "%4.1f" [expr 100.0*$NF/$Nall]]
    echo "  found $NF F stars ($p %)"
    set Gmask  [vectorExprEval "$SpType == 3 ? $SDSSmask : 0"]
    set NG [vNnonzero $Gmask]
    set p [format "%4.1f" [expr 100.0*$NG/$Nall]]
    echo "  found $NG G stars ($p %)"
    set Kmask  [vectorExprEval "$SpType == 4 ? $SDSSmask : 0"]
    set NK [vNnonzero $Kmask]
    set p [format "%4.1f" [expr 100.0*$NK/$Nall]]
    echo "  found $NK K stars ($p %)"
    set Mmask  [vectorExprEval "$SpType == 5 ? $SDSSmask : 0"]
    set NM [vNnonzero $Mmask]
    set p [format "%4.1f" [expr 100.0*$NM/$Nall]]
    echo "  found $NM M stars ($p %)"
    ## split to spectral classes: SDSS-2MASS sources
    set Nall [vNnonzero $SDSS2MASSmask]
    echo "splitting $Nall stars into spectral classes"   
    set OBmask2 [vectorExprEval "$SpType == 0 ? $SDSS2MASSmask : 0"]
    set NOB [vNnonzero $OBmask2]
    set p [format "%4.1f" [expr 100.0*$NOB/$Nall]]
    echo "  found $NOB OB stars ($p %)"
    set Amask2  [vectorExprEval "$SpType == 1 ? $SDSS2MASSmask : 0"]
    set NA [vNnonzero $Amask2]
    set p [format "%4.1f" [expr 100.0*$NA/$Nall]]
    echo "  found $NA A stars ($p %)"
    set Fmask2  [vectorExprEval "$SpType == 2 ? $SDSS2MASSmask : 0"]
    set NF [vNnonzero $Fmask2]
    set p [format "%4.1f" [expr 100.0*$NF/$Nall]]
    echo "  found $NF F stars ($p %)"
    set Gmask2  [vectorExprEval "$SpType == 3 ? $SDSS2MASSmask : 0"]
    set NG [vNnonzero $Gmask2]
    set p [format "%4.1f" [expr 100.0*$NG/$Nall]]
    echo "  found $NG G stars ($p %)"
    set Kmask2  [vectorExprEval "$SpType == 4 ? $SDSS2MASSmask : 0"]
    set NK [vNnonzero $Kmask2]
    set p [format "%4.1f" [expr 100.0*$NK/$Nall]]
    echo "  found $NK K stars ($p %)"
    set Mmask2  [vectorExprEval "$SpType == 5 ? $SDSS2MASSmask : 0"]
    set NM [vNnonzero $Mmask2]
    set p [format "%4.1f" [expr 100.0*$NM/$Nall]]
    echo "  found $NM M stars ($p %)"
    ###########################################################################

    set SDSSmasklist [list $OBmask $Amask $Fmask $Gmask $Kmask $Mmask]
    set SDSS2MASSmasklist [list $OBmask2 $Amask2 $Fmask2 $Gmask2 $Kmask2 $Mmask2]
    set colorlist [list $ug $gr $ri $iz $zJ $JH $HK $J $r]
    set outputlist [list $SDSSmasklist $SDSS2MASSmasklist $colorlist]

    #cleaning up...
    vectorExprDel $H; vectorExprDel $K 
    vectorExprDel $u; vectorExprDel $g 
    vectorExprDel $i; vectorExprDel $z; 
    vectorExprDel $SDSSmask; vectorExprDel $SDSS2MASSmask 
    vectorExprDel $aux_mask; vectorExprDel $SpType
    genericChainDestroy $chain;

    return $outputlist
    
}

#########################################################################
# dumping FIRST matches to a par file

# structure which holds SDSS and FIRST data
typedef struct {
    int localID;
    int run;
    int camCol;
    int field;
    int id;
    float objc_rowc;
    float objc_colc;
    double ra;
    double dec;
    int objc_flags;
    int objc_flags2;
    int objc_type;
    int utype;
    int gtype;
    int rtype;
    int itype;
    int ztype;
    float u;
    float uErr;
    float g;
    float gErr;
    float r;
    float rErr;
    float i;
    float iErr;
    float z;
    float zErr;
    float umod;
    float umodErr;
    float gmod;
    float gmodErr;
    float rmod;
    float rmodErr;
    float imod;
    float imodErr;
    float zmod;
    float zmodErr;
    float uPet;
    float uPetErr;
    float gPet;
    float gPetErr;
    float rPet;
    float rPetErr;
    float iPet;
    float iPetErr;
    float zPet;
    float zPetErr;
    float uR50;
    float uR50Err;
    float gR50;
    float gR50Err;
    float rR50;
    float rR50Err;
    float iR50;
    float iR50Err;
    float zR50;
    float zR50Err;
    float uRedd;
    float gRedd;
    float rRedd;
    float iRedd;
    float zRedd;
    double raFIRST; 
    double decFIRST; 
    float Fpeak;
    float Fint;
    float Rms;
    float Maj;
    float Min;
    float PA;
    float fMaj;
    float fMin;
    float fPA;
} SDSSFIRST;

  
# For a specified list of tsObj files, dump SDSS2FIRST structure to 
# a *.par file
proc dump_SDSSFIRST {tsObjList outfile {primary 1}} {

    # for  memory reporting
    set startMem [memNextSerialNumber]

    # start output file
    set outf [open $outfile "w"]
    puts $outf "# SDSS - FIRST matches (ZI)"
    close $outf
    # write out SDSSFIRST structure definition
    set hdr ""
    set chain [chainNew SDSSFIRST]
    chain2Param $outfile $chain $hdr -append
    chainDestroy $chain genericDel

    set nfiles [llength $tsObjList]
    echo ""; echo "  --  DUMPING $nfiles files  --"
    echo " file       N all    N prim    N good" 
    set Ntot 0  

    # loop over files     
    loop i 0 $nfiles {    
        set file [lindex $tsObjList $i]
        # read chain from this tsObj file
        set chain [tsObj2chain $file]
        if {$primary} {
           # we want only primary elements
           set prim [get_primaries $chain]        
        } else {
           set prim $chain
        } 
        set Na [format "%8d" [chainSize $chain]]
        set Np [format "%8d" [chainSize $prim]]

        # This loops over all of the matched elements and dumps them
        # to the output par file. The return value is the number of good elements.
        set Ng [format "%8d" [dumpSDSSFIRSTmatches $prim $outfile $Ntot]]
        echo " [format "%4d" $i]    $Na  $Np  $Ng"
        incr Ntot $Ng
        
        # clean   
        if {$primary} {chainDel $prim}
        genericChainDestroy $chain
    }

    set Ntot [format "%8d" $Ntot]   
    echo " Total:   $Ntot"
    echo " ------------------------------------------------"

    # check for memory leaks
    assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}

}


# given a chain of SDSS-FIRST matches, populate a chain of SDSS2FIRST structures
# and dump them (append) to outfile. The local counter localID stars from id0.
# Return the number of sources.
proc dumpSDSSFIRSTmatches {chain outfile id0} {

     # which fields do we want copied over? 
     # as defined in SDSSFIRST
     set Pfieldsaux [schemaGetFromType SDSSFIRST]
     set Pfields {}
     foreach el $Pfieldsaux {   
        set el [lindex $el 0]
        lappend Pfields $el
     }
     # fields from TSOBJ
     set Tfieldsaux [schemaGetFromType TSOBJ]
     set Tfields {}
     foreach el $Tfieldsaux {   
        set el [lindex $el 0]
        lappend Tfields $el
     }

     set psfmags "u uErr g gErr r rErr i iErr z zErr"
     set Tpsfmags "psfCounts<0> psfCountsErr<0> psfCounts<1> psfCountsErr<1> psfCounts<2> psfCountsErr<2> \
                   psfCounts<3> psfCountsErr<3> psfCounts<4> psfCountsErr<4>"
     set modmags "umod umodErr gmod gmodErr rmod rmodErr imod imodErr zmod zmodErr" 
     set Tmodmags "counts_model<0> counts_modelErr<0> counts_model<1> counts_modelErr<1> counts_model<2> counts_modelErr<2> \
                   counts_model<3> counts_modelErr<3> counts_model<4> counts_modelErr<4>"
     set petmags "uPet uPetErr gPet gPetErr rPet rPetErr iPet iPetErr zPet zPetErr"
     set Tpetmags "petroCounts<0> petroCountsErr<0> petroCounts<1> petroCountsErr<1> petroCounts<2> petroCountsErr<2> \
                   petroCounts<3> petroCountsErr<3> petroCounts<4> petroCountsErr<4>" 
     set r50  "uR50 uR50Err gR50 gR50Err rR50 rR50Err iR50 iR50Err zR50 zR50Err"
     set Tr50  "petroR50<0> petroR50Err<0> petroR50<1> petroR50Err<1> petroR50<2> petroR50Err<2> \
                petroR50<3> petroR50Err<3> petroR50<4> petroR50Err<4>"
     set redd "uRedd gRedd rRedd iRedd zRedd"
     set Tredd "reddening<0> reddening<1> reddening<2> reddening<3> reddening<4>"
     set SGtypes "utype gtype rtype itype ztype"
     set TSGtypes "type<0> type<1> type<2> type<3> type<4>"
 
     # this has to be the same as in matchFIRST, i.e. it can be easily broken 
     if {1} {
        set FIRSTfields  "raFIRST decFIRST Fpeak Fint Rms \
                       Maj Min PA fMaj fMin fPA"
        set TFIRSTfields "firstLambda firstEta firstPeak firstInt firstRms \
                       fiberCounts<3> fiberCountsErr<3> texture<3> fiberCounts<4> fiberCountsErr<4> texture<4>"
     } else {
        echo "WARNING: dumping VLA data (instead of FIRST)"
        set FIRSTfields  "raFIRST decFIRST Fint Maj Min PA"
        set TFIRSTfields "firstLambda firstEta firstInt  \
                          fiberCounts<3> fiberCountsErr<3> texture<3>"
     }

     # all together
     set Newfields   "$psfmags $modmags $petmags $r50 $redd $SGtypes $FIRSTfields"
     set Newtransfer "$Tpsfmags $Tmodmags $Tpetmags $Tr50 $Tredd $TSGtypes $TFIRSTfields"
     assert {[llength $Newfields] == [llength $Newtransfer]}
     set newchain [chainNew SDSSFIRST]
     set cursor [chainCursorNew $chain]
     set N 0
     while {[set source [chainWalk $chain $cursor]] != ""} { 
        incr N
        set new [genericNew SDSSFIRST]
        # first set overall counter
        handleSet $new.localID [expr $id0 + $N] 
        # copy over all the fields that exist
        foreach el $Pfields {
            set record $el
            if {[lsearch $Tfields $record] >= 0} {
                # this field has the same name in tsObj files (but treat specially chromatic fields)
                set aux [readSTRUCT TSOBJ $source 0 $record]
                handleSet $new.$el $aux
            } else { 
                # this field does not exist in tsObj file
                set nNew [lsearch $Newfields $record]
        if {$nNew >= 0} {
                    set transferField [lindex $Newtransfer $nNew]
                    set aux [readSTRUCT TSOBJ $source 0 $transferField]
                    handleSet $new.$el $aux
                }
            }
        } 
        # all set, add it to the chain
        chainElementAddByPos $newchain $new
     }
       
     # dump the chain of good matches to the output file
     set Ngood [chainSize $newchain]
     chain2Param $outfile $newchain "" -append -notypedump

     # clean up   
     chainCursorDel $chain $cursor
     chainDestroy $newchain genericDel
   
     return $Ngood
} 




####################################################################
# RED SOURCES

proc analyze_red_sources {run {camCols "all"} {device /VPS}} {

     ### tunable parameters
     # mag range
     set band z; set mmin 14.5; set mmax 21.0
     # color range     
     set band1 i; set band2 z
     set cmin 1.5; set cmax 6.0
     # maximal errors
     set mag_errmax 0.5; set col_errmax 1.0; 
 
     # take care of defaults
     if {$camCols == "all"} {set camCols {1 2 3 4 5 6}}

     # get tsObj file list
     set files {}
     foreach col $camCols {
    lappend files [doglob tsObj-red_sources_run${run}_col${col}*.fit]
     }

     # read chain
     set chain [tsObjList2chain $files]

     # select good sources
     set flagsON {}
     set flagsOFF [list SATUR EDGE BRIGHT CR BLENDED CHILD]
     set sel [clean_tsObj_chain $chain $band $mmin $mmax $band1 $band2 $cmin $cmax \
                                $mag_errmax $col_errmax $flagsON $flagsOFF]

     # plot selected sources
     plot_zzi $sel $device "cleaned red sources for run $run" $mag_errmax $col_errmax   
   
     if {$device != "/XWINDOW"} {
    catch {my_mv red_sources.ps red_sources-run$run.ps}
        echo "Your z vs. i-z plot is in red_sources-run$run.ps"
     }

     # dump selected sources
     print_photo $sel 0 -1 red_sources-run$run.analyzed

     chainDel $sel
     genericChainDestroy $chain
     

} 




proc plot_zzi {chain device {title ""} {err_zmax 0.1} {err_izmax 0.1}} {
   
  
     # form vectors
     set i [vFromChain $chain psfCounts<3>]
     set z [vFromChain $chain psfCounts<4>]
     set ierr [vFromChain $chain psfCountsErr<3>]
     set zerr [vFromChain $chain psfCountsErr<4>]
     set iz [vectorExprEval $i-$z]
     set izerr [vectorExprEval sqrt($ierr*$ierr+$zerr*$zerr)]
     vNameSet $i "i"; vNameSet $z "z"; vNameSet $iz "i-z"

     # selection
     set maskaux [vectorExprEval "$zerr < $err_zmax ? 1 : 0"]
     set mask [vectorExprEval "$izerr < $err_izmax ? $maskaux : 0"]
     vectorExprDel $maskaux 
     set Ngood [vNnonzero $mask] 
     echo " selected $Ngood sources (out of [chainSize $chain]) \
            with z_err < $err_zmax and iz_err < $err_izmax"
 
     # plot color-magnitude diagram 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device red_sources.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb 1 -icError 1 \
          -xfract 0.4 -yfract 0.4 -nxwindow 1 -nywindow 1 
         pgstateOpen $pg

         # z vs. i-z
         vPlot $pg $iz $z -vectorMask $mask -vectorXErr $izerr -vectorYErr $zerr \
                     -xmin 1.0 -xmax 6.0 -ymin 20.5 -ymax 14.0 
         titlePlot $title 20      
         pgstateSet $pg -device $deviceA -icMark 2 -symb 7 -isNewplot 0 -icLine 2
         vPlot $pg $iz $z -vectorMask $mask

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     vectorExprDel $i; vectorExprDel $z; vectorExprDel $iz 
     vectorExprDel $ierr; vectorExprDel $zerr; vectorExprDel $izerr 

}

# print photometry for a chain
proc print_photo {chain {e1 0} {e2 -1} {outfile ""} {2MASS 0}} {

    if {$e2 < 0} {set e2 [chainSize $chain]}
    set filters {u g r i z}    

    if {$outfile != ""} {
        set outf [open $outfile w]
        set dump 1
    } else {
        set outf ""
        set dump 0
    }

    loop i $e1 $e2 {
       set el [chainElementGetByPos $chain $i]
       print_one_photo $el $dump $outf $2MASS
    }

    if {$dump} {
        close $outf 
    }
}   

# print photometry for an element read from tsObj files
proc print_one_photo {el {dump 0} {outf ""} {2MASS 0} {usefpObj 0}} {

       set filters {u g r i z}    

       set id [exprGet $el.id]
       if {$usefpObj} {
          # echo "reading from fpObjc - no RA and Dec!"
       } else {
          set RA [format "%10.6f" [exprGet $el.ra]]
          set Dec [format "%10.6f" [exprGet $el.dec]]
       }
       set flags [flags [exprGet $el.objc_flags]]
       set flags2 [flags2 [exprGet $el.objc_flags2]]
       set nchild [exprGet $el.nchild]
       set parent [exprGet $el.parent]
       if {$usefpObj} {
          set primary "unknown"
       } else {
          set primary [expr ([exprGet $el.status] & 8192) != 0]    
       }
       set objc_type [obj_type_translate [exprGet $el.objc_type]]
       echo ""; 
       if {$usefpObj} {
            echo "Source ID=$id, OBJC_TYPE=$objc_type"
       } else {
            echo "Source at RA=$RA, Dec=$Dec, ID=$id, OBJC_TYPE=$objc_type"
       }
       echo "           parent ID = $parent, N(children) = $nchild"
       set row [format "%6.1f" [exprGet $el.objc_rowc]]
       set col [format "%6.1f" [exprGet $el.objc_colc]]
       echo "           pixel coordinates: row = $row, col = $col"
       echo "           filter      mpsf          mmodel          type"
       if {$dump} {
          puts $outf ""; puts $outf "Source at RA=$RA, Dec=$Dec" 
          puts $outf "          filter      mpsf          mmodel            type"
       }
       loop i 0 5 {
          set f [lindex $filters $i]
          set mp($f) [format "%4.2f" [exprGet $el.psfCounts<$i>]]
          set ep($f) [format "%4.2f" [exprGet $el.psfCountsErr<$i>]]
          set mm($f) [format "%4.2f" [exprGet $el.counts_model<$i>]]
          set em($f) [format "%4.2f" [exprGet $el.counts_modelErr<$i>]]
          set obj_type [obj_type_translate [exprGet $el.type<$i>]]
          echo "             $f    $mp($f) +- $ep($f)   $mm($f) +- $em($f)      $obj_type"
          if {$dump} {
             puts $outf "             $f    $mp($f) +- $ep($f)   $mm($f) +- $em($f)      $obj_type"  
          }    
       }
       set ug [format "%4.2f" [expr $mp(u)-$mp(g)]]
       set gr [format "%4.2f" [expr $mp(g)-$mp(r)]]
       set ri [format "%4.2f" [expr $mp(r)-$mp(i)]]
       set iz [format "%4.2f" [expr $mp(i)-$mp(z)]]
       echo "            psf: u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
       if {$dump} {
           puts $outf "            psf: u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
       }
       set ug [format "%4.2f" [expr $mm(u)-$mm(g)]]
       set gr [format "%4.2f" [expr $mm(g)-$mm(r)]]
       set ri [format "%4.2f" [expr $mm(r)-$mm(i)]]
       set iz [format "%4.2f" [expr $mm(i)-$mm(z)]]
       echo "            mod: u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
       echo "             primary = $primary"
       echo "             objc_flags = $flags"
       echo "            objc_flags2 = $flags2"
       loop i 0 5 {
           echo "            flags [lindex {u g r i z} $i] = [flags [exprGet $el.flags<$i>]]"
       }
       if {$dump} {
           puts $outf "            mod: u-g = $ug, g-r = $gr, r-i = $ri, i-z = $iz"
           puts $outf "             primary = $primary"
           puts $outf "             objc_flags = $flags"
           puts $outf "            objc_flags2 = $flags2"
       }
       echo "            rowv = [exprGet $el.rowv]+-[exprGet $el.rowvErr]"
       echo "            colv = [exprGet $el.colv]+-[exprGet $el.colvErr]"
       

       if {$2MASS} {
           set J [format "%4.2f" [exprGet $el.fiberCounts<0>]]
           set Jerr [format "%4.2f" [exprGet $el.fiberCountsErr<0>]]
           set H [format "%4.2f" [exprGet $el.fiberCounts<1>]]
           set Herr [format "%4.2f" [exprGet $el.fiberCountsErr<1>]]
           set K [format "%4.2f" [exprGet $el.fiberCounts<2>]]
           set Kerr [format "%4.2f" [exprGet $el.fiberCountsErr<2>]]
           set JH [format "%4.2f" [expr $J-$H]]
           set JHerr [format "%4.2f" [expr sqrt($Jerr*$Jerr+$Herr*$Herr)]]
           set HK [format "%4.2f" [expr $H-$K]]
           set HKerr [format "%4.2f" [expr sqrt($Kerr*$Kerr+$Herr*$Herr)]]
           set JK [format "%4.2f" [expr $J-$K]]
           set JKerr [format "%4.2f" [expr sqrt($Kerr*$Kerr+$Jerr*$Jerr)]]
           set rd_flg [exprGet $el.fracPSF<0>]
           if {$rd_flg < 10} {set rd_flg 0$rd_flg}
           if {$rd_flg < 100} {set rd_flg 0$rd_flg}
           set bl_flg [exprGet $el.fracPSF<1>]
           if {$bl_flg < 10} {set rd_flg 0$bl_flg}
           if {$bl_flg < 100} {set rd_flg 0$bl_flg}
           set cc_flg [cc_flg_translate2char [exprGet $el.fracPSF<2>]]
           set extd_flg [exprGet $el.fracPSF<3>]
           set mp_flg [exprGet $el.fracPSF<4>]           
           echo ""; echo "  2MASS: J = $J+-$Jerr  H = $H+-$Herr  K = $K+-$Kerr"
           echo "         J-H = $JH+-$JHerr H-K = $HK+-$HKerr J-K = $JK+-$JKerr" 
           echo "         rd_flg = $rd_flg  bl_flg = $bl_flg  cc_flg = $cc_flg"
           echo "         extd_flg = $extd_flg  mp_flg = $mp_flg"
           if {$dump} {
             puts $outf ""; puts $outf "  2MASS: J = $J+-$Jerr  H = $H+-$Herr  K = $K+-$Kerr"
             puts $outf "         J-H = $JH+-$JHerr H-K = $HK+-$HKerr J-K = $JK+-$JKerr" 
             puts $outf "         rd_flg = $rd_flg  bl_flg = $bl_flg  cc_flg = $cc_flg"
             puts $outf "         extd_flg = $extd_flg  mp_flg = $mp_flg"
           }
       }

}   



# we need an interger value to stuff in tsObj files, not char
proc cc_flg_translate2char {cc} {

    if {$cc < 10} {set cc 0$cc}
    if {$cc < 100} {set cc 0$cc}
      
    set f0 0; set f1 0; set f2 0
    loop i 0 3 {
      set flag [string range $cc $i $i]
      if {$flag == "0"} {set f$i 0}
      if {$flag == "1"} {set f$i U}
      if {$flag == "2"} {set f$i N}
      if {$flag == "3"} {set f$i P}
      if {$flag == "4"} {set f$i D}
      if {$flag == "5"} {set f$i S}
      if {$flag == "6"} {set f$i R}
      if {$flag == "7"} {set f$i C}
      if {$flag == "8"} {set f$i B}
    }
    return "${f0}${f1}${f2}"
}

proc obj_type_translate {type} {

    # there must be a better way to do this
    if {$type == 0} {return "UNK"}
    if {$type == 1} {return "CR"}
    if {$type == 2} {return "DEFECT"}
    if {$type == 3} {return "GALAXY"}
    if {$type == 4} {return "GHOST"}
    if {$type == 5} {return "KNOWNOBJ"}
    if {$type == 6} {return "STAR"}
    if {$type == 7} {return "TRAIL"}
    if {$type == 8} {return "SKY"}
    if {$type == 9} {return "NTYPE"}
    return "UNKNOWN" 
}



################################################
###  some aux proc used in RR Lyrae research ###

#####################################################################
# Given a pencil beam of (small) area delA which is centered on the 
# Earth and whose orientation is described by RA and Dec, find and
# return the fraction of the volume of a spherical shell centered on 
# the Galactic center and  given by R1 < r < R2. Assume that the beam 
# can "see" up to Dmax. All distances should be given in kpc and delA
# in deg2.
# Distances from the GC are referred to as R, distances from us as D.
proc beam_thru_shell {delA RA Dec Dmax R1 R2} {


    ### distance to the Galactic center (from Reid, 199?)
    set Ro 8.0 


    # for trigonometry
    set deg2rad [expr 3.14159/180]  

    # first find galactic coordinates     
    set gal_coord [eqToGal $RA $Dec] 
    set gLong [keylget gal_coord gLong]
    set gLat  [keylget gal_coord gLat]
    set cosb  [expr cos($gLat*$deg2rad)] 
    set cosl  [expr cos($gLong*$deg2rad)] 
 
    # given this direction,
    # the closest approach to the GC is for this D
    set Dmin [expr $Ro * $cosb * $cosl]
    # and the distance from the GC is
    set Rmin [expr $Ro * sqrt(1-$cosb*$cosb*$cosl*$cosl)]
    if {$gLong >= 90 && $gLong <=270} {
        set Dmin 0
        set Rmin $Ro
    }
                     
    # various possibilities...
    # shell smaller than Ro
    if {$R1 < $Ro && $R2 < $Ro} {
        if {$R2 < $Rmin} {
            return 0
        }
        if {$R1 < $Rmin} {
            set delD [expr sqrt($Dmin*$Dmin + $R2*$R2 - $Ro*$Ro)]
            set D1 [expr $Dmin - $delD]
            set D2 [expr $Dmin + $delD]
        return [shell_frac $R1 $R2 $D1 $D2 $Dmax $delA]
        } else {
            set delD1 [expr sqrt($Dmin*$Dmin + $R1*$R1 - $Ro*$Ro)]
            set delD2 [expr sqrt($Dmin*$Dmin + $R2*$R2 - $Ro*$Ro)]
            set D1 [expr $Dmin - $delD2]
            set D2 [expr $Dmin - $delD1]
        set aux [shell_frac $R1 $R2 $D1 $D2 $Dmax $delA]
            set D1 [expr $Dmin + $delD1]
            set D2 [expr $Dmin + $delD2]
        return [expr $aux + [shell_frac $R1 $R2 $D1 $D2 $Dmax $delA]]
        }
    }

    # shell containing Ro
    if {$R1 <= $Ro && $R2 >= $Ro} {
        if {$R1 < $Rmin} {
            set delD [expr sqrt($Dmin*$Dmin + $R2*$R2 - $Ro*$Ro)]
            set D1 0
            set D2 [expr $Dmin + $delD]
        return [shell_frac $R1 $R2 $D1 $D2 $Dmax $delA]
        } else {
            set delD1 [expr sqrt($Dmin*$Dmin + $R1*$R1 - $Ro*$Ro)]
            set delD2 [expr sqrt($Dmin*$Dmin + $R2*$R2 - $Ro*$Ro)]
            set D1 0
            set D2 [expr $Dmin - $delD1]
        set aux [shell_frac $R1 $R2 $D1 $D2 $Dmax $delA]
            set D1 [expr $Dmin + $delD1]
            set D2 [expr $Dmin + $delD2]
        return [expr $aux + [shell_frac $R1 $R2 $D1 $D2 $Dmax $delA]]
        }
    }

    # shell larger than Ro
    if {($R1 >= $Ro && $R2 >= $Ro) || [expr abs($gLong)] > 90} {
       set delD1 [expr sqrt($Dmin*$Dmin + $R1*$R1 - $Ro*$Ro)]
       set delD2 [expr sqrt($Dmin*$Dmin + $R2*$R2 - $Ro*$Ro)]
       set D1 [expr $Dmin + $delD1]
       set D2 [expr $Dmin + $delD2]
       return [shell_frac $R1 $R2 $D1 $D2 $Dmax $delA]
    }

    error "beam_thru_shell: should have returned by now"

}


# aux. proc to go with beam_thru_shell
proc shell_frac {R1 R2 D1 D2 Dmax delA} {

    assert {$R1 <= $R2}
    assert {$D1 <= $D2}

    # first check the limit on D
    if {$D1 > $Dmax} {return 0}
    if {$D2 > $Dmax} {set D2 $Dmax}

    # conversion from deg2 to the fraction of entire sky
    set sqdeg2sky [expr 1.0/41253.0]
    
    # factor 4Pi/3 ignored in both expressions
    set Vbeam [expr $delA*$sqdeg2sky*($D2*$D2*$D2-$D1*$D1*$D1)]
    set Vshell [expr ($R2*$R2*$R2-$R1*$R1*$R1)]

    return [expr $Vbeam / $Vshell]

}



# wrapper around beam_thru_shell:
# given a shell (R1, R2) find the fraction of its total volume 
# observed by an equatorial SDSS run extending from RA1 to RA2.
# assume that the beam extends up to Dmax kpc from the Earth
proc beam_from_eqScan {RA1 RA2 Dmax R1 R2} {


    assert {$RA1 <= $RA2}
    assert {$R1 <= $R2}
 
    set frac 0.0

    # loop over RA in steps of 2 deg
    set n [expr 1 + int(($RA2-$RA1)/2.0)]
    set stepRA [expr 1.0*($RA2-$RA1)/$n]
    set Dec 0.0
    set delA [expr $stepRA * 1.365333]

    loop i 0 [expr $n] {
    set RA [expr $RA1 + $i * $stepRA]
    set frac [expr $frac + [beam_thru_shell $delA $RA $Dec $Dmax $R1 $R2]]
    }

    # return correction (compared to naive assumption of being in the GC)
    set naivefrac [expr ($RA2-$RA1) * 1.365333 / 41253.0]
    return [expr $frac / $naivefrac]

} 



# wrapper around beam_from_eqScan for a set of radii from Rs to Re with 
# fractional step delR
proc bin_correction {Rs Re delR Dmax RA1 RA2 outfile} {

    set f [open $outfile w]
    puts $f "\# output from bin_correction"
    puts $f "\# $Rs $Re $delR $Dmax $RA1 $RA2 "

    set R2 $Rs

    while {$R2 < $Re} {
        set R1 [format "%6.2f" $R2]
        set R2 [format "%6.2f" [expr $R2 * $delR]]
        set corr [format "%5.3f" [beam_from_eqScan $RA1 $RA2 $Dmax $R1 $R2]]
        puts $f "  $R1   $R2     $corr" 
        #echo "  $R1   $R2     $corr"
    }
  
    close $f

}



# given a file, and a data column (1-indexed), produce subsets of data:
# for type = 1, according to the integer value of data records
# for type = 2, according to the full record
# return the list of files
# e.g. make_subfiles WE00.0000.dat 1 
proc make_subfiles {file column type {Nlinemax 99999999}} {

       set Nline 0
       set return_list {}  
       set open_files {}
       # open input file
       set inf [open $file r]
       # loop over input list file
       while {![eof $inf] && $Nline < $Nlinemax} {
          set trouble 0
          set line [gets $inf]
          # process line
          if {![eof $inf]} {  
	     if {[string range $line 0 0] == "#" || [string range $line 0 0] == ";"} {  
                 continue
             }
	     if {[llength $line] <= $column} {  
                 continue
             }
             incr Nline 
             # get record
	     set rec [lindex $line [expr $column - 1]]
	     if {$type == 1} {
                 if {[catch {set rec [expr int($rec)]}]} {
                     echo "can't get int of $rec"
                     set trouble 1
                 }
	     }
	     if {$trouble} {continue}
	     set outname ${file}_$rec
	     if {[lsearch $return_list $outname] < 0} {
                 # new file
                 set return_list [lappend return_list $outname]
                 set outf [open $outname w]
                 set open_files [lappend open_files $outf]
                 set outfp($rec) $outf
                 puts $outf "# output from make_subfiles $file $column $type $Nlinemax"
             } else {
                 # existing file
                 set outf $outfp($rec)
             }
	     puts $outf $line; flush $outf
          }
       }            
       # sanity checks
       assert { [llength $open_files] == [llength $return_list] }
       # close all files
       close $inf
       foreach fp $open_files {
          close $fp
       }

       echo Processed $Nline records and stored to [llength $return_list] files
    
    return $return_list

} 

####
# - given a list with coordinates and a photometry file, cross reference
# the list and write out all sources that are matched to outfile
# - copy sublist of sources which are not found to notmatchfile
# list has coordinates in columns colRA colDec. For
# coord_format = 0 assume that coordinates are given in degrees. For 
# coord_format = 1 assume that coordinates are given as 
# 10 59 26.10 -00 59 27.6 (columns are starting columns for RA and Dec)
# coord_format = 2 assume that coordinates are given as 
# 10:59:26.10 -00:59:27.6.
# - n.b. this can be used for a general match of 2 files. In this case
#   specify RA-Dec columns for the second file as photo_colRA-photo_colDec
# - original format of matched sources is dumped to matched.dat
# E.g.
# cross_correlate_list PIER.list All-745-756-all.corrected.apCorr.RRLyrae.5 test.list 1 2 1
#
# to speed up matching, set express to 1, which splits photofile into 360 subsets
# ordered by RA; if these files already exist, set express to 2; to remove
# these files after the job is done set express to -1, or -2
proc cross_correlate_list {list photofile outfile notmatchfile colRA colDec \
                       {coord_format 0} {photo_colRA 5} {photo_colDec 6} \
			       {photo_coord_format 0} {match_rad 10} {add_input 0}} {

global verbose express reverseOutput allMatches

       if {![info exist verbose]} {set verbose 0}

       if {![info exist allMatches]} {set allMatches 0}

       if {[info exist express] && $express != 0} {
           # express version
           if {[expr abs($express)] == 1} {
              echo calling make_subfiles with $photofile 
              set sub_file_list [make_subfiles $photofile $photo_colRA 1]
	   }
       } else {
           set express 0
       }


       # match radius in arcsec
       set match_rad [expr $match_rad / 3600.0]
       set Nlinemax 10000000
       set Nline 0  
       # 0-index columns
       set colRA [expr $colRA - 1]
       set colDec [expr $colDec - 1]
       set photo_colRA [expr $photo_colRA - 1]
       set photo_colDec [expr $photo_colDec - 1]

       # open list file
       set inf [open $list r]
       set Nline 0
       set Nphoto 0   
  
       # open out files
       set outf [open $outfile w]
       set outfNM [open $notmatchfile w]
       puts $outfNM "# Not found in $photofile"
       flush $outfNM
       set matched [open matched.dat w]
       puts $matched "# From $list, found in $photofile"
       flush $matched 

       # set outf2 [open FASTT.list.RADec w]
       # loop over input list file
       while {![eof $inf] && $Nline < $Nlinemax} {
          set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#" || [string range $line 0 0] == ";"} {  
                 continue
             }
             incr Nline 
             # get coordinates
             set RA [lindex $line $colRA]
             set Dec [lindex $line $colDec]
	     if {$verbose} {echo "working on source $Nline ($RA, $Dec)"}
             if {$coord_format == 0 } {
                 #puts $outf "# coord. in deg: $RA $Dec"  
             } else {
                 if {$coord_format == 1} {
                    set RAh $RA
                    set DecDeg $Dec
                    set RAm [lindex $line [expr $colRA + 1]]
                    set DecMin [lindex $line [expr $colDec + 1]]
                    set RAs [lindex $line [expr $colRA + 2]]
                    set DecSec [lindex $line [expr $colDec + 2]] 
                    puts $outf "# input coord: $RAh $RAm $RAs $DecDeg $DecMin $DecSec" 
                    set deg_coord [convert2deg $RAh $RAm $RAs $DecDeg $DecMin $DecSec]
                    set RA [lindex $deg_coord 0]
                    set Dec [lindex $deg_coord 1]            
                    puts $outf "#    coord. in deg: $RA $Dec" 
                    #puts $outf2 "$RA $Dec"
                 }
                 if {$coord_format == 2} { 
                    puts $outf "# input coord: $RA $Dec" 
                    set deg_coord [Jeff2deg $RA $Dec]
                    set RA [lindex $deg_coord 0]
                    set Dec [lindex $deg_coord 1]            
                    puts $outf "#    coord. in deg: $RA $Dec" 
                 }
             } 

             set matchOK 0
             ## ready to search ###########################
	     if {!$express} {
                 set photo [open $photofile r]     
                 set close_photo 1
             } else {
                 set RAint [expr int($RA)]
                 if {[file exist ${photofile}_$RAint]} {
		    set photo [open ${photofile}_$RAint r]  
                    set close_photo 1
                 } else {
                    set close_photo 0
                 }  
             }    
             # loop over input file
             while {$close_photo && ![eof $photo] && !$matchOK} {
                 set Pline [gets $photo]
                 # process line
                 if {![eof $photo]} {  
                    if {[string range $Pline 0 0] == "#"} {  
                       continue
                    }
                    if [catch { 
                      # extract coordinates
                      set ra1 [format "%11.7f" [lindex $Pline $photo_colRA]]   
                      set dec1 [format "%11.7f" [lindex $Pline $photo_colDec]]
  	              if {$verbose > 1} {echo "  matching $Nline ($RA, $Dec) to ($ra1, $dec1)"}
                      if {$photo_coord_format == 1} {
                          set RAh $ra1
                          set DecDeg $dec1
                          set RAm [lindex $Pline [expr $photo_colRA + 1]]
                          set DecMin [lindex $Pline [expr $photo_colDec + 1]]
                          set RAs [lindex $Pline [expr $photo_colRA + 2]]
                          set DecSec [lindex $Pline [expr $photo_colDec + 2]] 
                          set deg_coord [convert2deg $RAh $RAm $RAs $DecDeg $DecMin $DecSec]
                          set ra1 [lindex $deg_coord 0]
                          set dec1 [lindex $deg_coord 1]            
                      }
                      if {$photo_coord_format == 2} { 
                          set deg_coord [Jeff2deg $ra1 $dec1]
                          set ra1 [lindex $deg_coord 0]
                          set dec1 [lindex $deg_coord 1]            
                      }
                      set meanRA $ra1
                      set meanDec $dec1
                      # distance
                      set delRA [expr $RA - $meanRA]
                      set delDec [expr $Dec - $meanDec]
                      set dist [expr sqrt($delRA*$delRA + $delDec*$delDec)]
                      # is it a match?
                      if {$dist > $match_rad} {
                          continue
                      } else {
                         # this is a match!
			 if {!$allMatches} {set matchOK 1}
                         incr Nphoto 
                         # write this source out 
                         if {$add_input} { 
                           # puts $outf "$Pline $line" 
                             if {[info exists reverseOutput] && $reverseOutput} {
                                 puts $outf "$Pline REVERSE $line" 
                             } else {
                                 puts $outf "$line $Pline" 
                             }
                         } else { 
                            puts $outf $Pline 
                         } 
                         flush $outf 
                         # and original data to default file
                         puts $matched $line 
                         flush $matched 
                      }    
                    }] {
                     continue
                    }
                 }
             }            
             if {!$matchOK} {
                 #puts $outf "#  ===== NOT FOUND ====="
                 puts $outfNM $line
                 flush $outfNM
             }
             #puts $outf "# "
	     if {$close_photo} {close $photo}   
             ## end of search ##############################
          }
      }

      if {$express == -1} {
          foreach file $sub_file_list {
	      catch {exec rm $file}
          }
      }
      close $inf
      close $outf
      #close $outf2
      close $outfNM
      close $matched
      echo "Processed $Nline sources from $list"
      echo "Matched $Nphoto sources from $photofile"
    
}


####
# - given two files, match the strings from two specified columns and 
#   dump all matches 
# - columns are specified as 1-indexed
# E.g.
# cross_correlate_files file1 3 file2 3 matches.dat notmatched 
proc cross_correlate_files {file1 C1 file2 C2 outfile notmatchfile \
			     {add_input 1} {Nlinemax 10000000} } {  

       # open input file
       set inf [open $file1 r]
       set Nline 0
       set Nphoto 0   
  
       # open out files
       set outf [open $outfile w]
       set outfNM [open $notmatchfile w]
       puts $outfNM "# Not found in $file2"
       set matched [open matched.dat w]
       puts $matched "# From $file1, found in $file2"

       # loop over input file1
       while {![eof $inf] && $Nline < $Nlinemax} {
          set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#"} {  
                 continue
             }
             incr Nline 
             echo $Nline
             # get the matching string
	     set string1 [lindex $line [expr $C1-1]]
             set matchOK 0
             ## ready to search ###
             set fp2 [open $file2 r]          
             # loop over input file
             while {![eof $fp2] && !$matchOK && $Nline < $Nlinemax} {
                 set Pline [gets $fp2]
                 # process line
                 if {![eof $fp2]} {  
                    if {[string range $Pline 0 0] == "#"} {  
                       continue
                    }
                    if [catch { 
                        # extract matching string coordinates
	                set string2 [lindex $Pline [expr $C2-1]]
                        # is it a match?
                        if {$string1 != $string2} {
                            continue
                        } else {
                            # this is a match!
                            set matchOK 1 
                            incr Nphoto 
                            # write this source out 
                            if {$add_input} {                            
                                puts $outf "$line $Pline" 
                            } else { 
                                puts $outf $Pline 
                            } 
                            # and original data to default file
                            puts $matched $line 
                        }    
                    } msg] {
                        #echo "problems: $msg"
                        continue
                    }
                 }
             }            
             if {!$matchOK} {
                 #puts $outf "#  ===== NOT FOUND ====="
                 puts $outfNM $line
             }
             #puts $outf "# "
             close $fp2       
             ## end of search ##############################
         }
         
      }

      close $inf
      close $outf
      close $outfNM
      close $matched
      echo "Processed $Nline sources from $file1"
      echo "Found $Nphoto matches in $file2"
    
}


proc match_orphans {file1 file2 fileout rmin rmax match_radius} {

    set colors {-10 10 -10 10 -10 10 -10 10}
    if {$rmax > $rmin} {
       selectPHOTOMETRY $file1 $file1.aux [list r $rmin $rmax] $colors
       selectPHOTOMETRY $file2 $file2.aux [list r $rmin $rmax] $colors
    }

    set chain1 [param2Chain $file1.aux ""]
    set chain2 [param2Chain $file2.aux ""]


    echo "Read [chainSize $chain1] and [chainSize $chain2] elements" 
    set match_condition "{ra ra $match_radius} {dec dec $match_radius}"
    set matchchn [chainMatch $chain1 $chain2 $match_condition] 

    set Nmatch [chainSize $matchchn]   
    echo "Matched $Nmatch elements within $match_radius arcsec"
    
    # first 
    # set outf [open $fileout "w"]
    # puts $outf "string"

   
    loop i 0 [chainSize $matchchn] {
       set match [chainElementGetByPos $matchchn $i]
       set ra1 [readSTRUCT PHOTOMETRY $match 1 ra]
       set ra2 [readSTRUCT PHOTOMETRY $match 2 ra]
       set dec1 [readSTRUCT PHOTOMETRY $match 1 dec]
       set dec2  [readSTRUCT PHOTOMETRY $match 2 dec]
       set delra [format "%5.2f" [expr 60*($ra2-$ra1)]]
       set deldec [format "%5.2f" [expr 60*($dec2-$dec1)]]
       set g1 [format "%4.2f" [readSTRUCT PHOTOMETRY $match 1 psfCounts<1>]]
       set g2 [format "%4.2f" [readSTRUCT PHOTOMETRY $match 2 psfCounts<1>]]
       set r1 [format "%4.2f" [readSTRUCT PHOTOMETRY $match 1 psfCounts<2>]]
       set r2 [format "%4.2f" [readSTRUCT PHOTOMETRY $match 2 psfCounts<2>]]
       set gr1 [format "%4.2f" [expr $g1-$r1]]
       set gr2 [format "%4.2f" [expr $g2-$r2]]
       echo "Match $i: RA1 = $ra1, Dec1 = $dec1; RA2 = $ra2, Dec2 = $dec2"
       echo " dRA = $delra, dDec = $deldec (arcmin), r1 = $r1, r2 = $r2, (g-r)1 = $gr1, (g-r)2 = $gr2"       
    }
    


} 





# set tsObjList [glob tsObj*fit]
# set chain [tsObjList2chain $tsObjList]
# and then use procs clean_tsObj_chain etc. to select desired subsample
# plotSDSS $device weird $title $selectedChainofweird 
proc plot_tsObj {device root title chain {symbol -1} {2MASS 0} {FIRST 0} {modmag 1}} {

     set filterlist {u g r i z}
 
     ### SDSS data
     set deredden 1
     loop i 0 5 { 
         set f [lindex $filterlist $i] 
         if {$deredden} {
            set reddening [vFromChain $chain reddening<$i>]
            set mpsfRaw  [vFromChain $chain psfCounts<$i>]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            set mmodRaw  [vFromChain $chain counts_model<$i>]
            set mmod($f) [vectorExprEval $mmodRaw-$reddening]
            vectorExprDel $reddening
            vectorExprDel $mpsfRaw
            vectorExprDel $mmodRaw
         } else {
            set mpsf($f) [vFromChain $chain psfCounts<$i>]
            set mmod($f) [vFromChain $chain counts_model<$i>]
         }
         set epsf($f) [vFromChain $chain psfCountsErr<$i>]
         set emod($f) [vFromChain $chain counts_modelErr<$i>]
         set petroR50($f) [vFromChain $chain petroR50<$i>]
         vNameSet $petroR50($f) "petroR50(r')";
         vNameSet $mpsf($f) "mpsf(${f}')"; vNameSet $epsf($f) ""  
         vNameSet $mmod($f) "mmod(${f}')"; vNameSet $emod($f) ""  
     }     
     set ra [vFromChain $chain ra]; vNameSet $ra "RA J2000 (deg)"   
     set dec [vFromChain $chain dec]; vNameSet $dec "Dec J2000 (deg)"   
     set objc_type [vFromChain $chain objc_type]
     set type(r) [vFromChain $chain type<2>]

     # colors (based on psf/model magnitudes)
     if {$modmag} {
        set ug [vectorExprEval $mmod(u)-$mmod(g)]
        set gr [vectorExprEval $mmod(g)-$mmod(r)]
        set ri [vectorExprEval $mmod(r)-$mmod(i)]
        set iz [vectorExprEval $mmod(i)-$mmod(z)]
        vNameSet $ug "mod u'-g'"; vNameSet $gr "mod g'-r'"; 
        vNameSet $ri "mod r'-i'"; vNameSet $iz "mod i'-z'"; 
        loop i 0 5 { 
            set f [lindex $filterlist $i]  
            set mag($f) $mmod($f)
        }       
     } else {
        set ug [vectorExprEval $mpsf(u)-$mpsf(g)]
        set gr [vectorExprEval $mpsf(g)-$mpsf(r)]
        set ri [vectorExprEval $mpsf(r)-$mpsf(i)]
        set iz [vectorExprEval $mpsf(i)-$mpsf(z)]
        vNameSet $ug "psf u'-g'"; vNameSet $gr "psf g'-r'"; 
        vNameSet $ri "psf r'-i'"; vNameSet $iz "psf i'-z'"; 
        loop i 0 5 { 
            set f [lindex $filterlist $i]  
            set mag($f) $mpsf($f)
        }       
     }

     if {$2MASS} {
        # 2MASS data
        set J [vFromChain $chain fiberCounts<0>]; vNameSet $J "J"
        set H [vFromChain $chain fiberCounts<1>]; vNameSet $H "H"
        set K [vFromChain $chain fiberCounts<2>]; vNameSet $K "K"
        set JH [vectorExprEval $J-$H]; vNameSet $JH "J-H"             
        set HK [vectorExprEval $H-$K]; vNameSet $HK "H-K"             
        set JK [vectorExprEval $J-$K]; vNameSet $JK "J-K"             

        # O-IR colors
        set zJ [vectorExprEval $mag(z)-$J]; vNameSet $zJ "z-J"  
        set rK [vectorExprEval $mag(r)-$K]; vNameSet $rK "r-K" 
     }

     if {$FIRST} {
        # FIRST data
        set raFIRST [vFromChain $chain firstLambda]; vNameSet $raFIRST "RA (deg)"
        set decFIRST [vFromChain $chain firstEta]; vNameSet $decFIRST "Dec (deg)"
        set Frms [vFromChain $chain firstRms]; vNameSet $Frms "Frms (mJy)"
        set Fpeak [vFromChain $chain firstPeak]; vNameSet $Fpeak "Fpeak (mJy)"
        set Fint [vFromChain $chain firstInt]; vNameSet $Fint "Fint (mJy)"
        set mFpeak [vectorExprEval -2.5*lg(0.001*$Fpeak/3631.0)]
        set mFint [vectorExprEval -2.5*lg(0.001*$Fint/3631.0)]
    vNameSet $mFint "mFint (AB)"; vNameSet $mFpeak "mFpeak (AB)";
        set thetaFirst [vectorExprEval lg($Fint/$Fpeak)]; 
        vNameSet $thetaFirst "log10(Fint/Fpeak)"
        set rF [vectorExprEval $mag(r)-$mFpeak]; vNameSet $rF "r'-mFIRSTpeak" 
     }

     if {$2MASS && $FIRST} {
        set KF [vectorExprEval $K-$mFpeak]; vNameSet $KF "K-mFIRSTpeak" 
     }
     
    

     ### all read, now plot ###
     
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
         vPlot $pg $ri $iz -xmin -0.5 -xmax 2.0 -ymin -0.5 -ymax 2.0 

         # g vs. g-r
         vPlot $pg $gr $mag(g) -xmin -0.5 -xmax 2.0 -ymin 23.0 -ymax 14.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
 
     # plot color histograms
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-colhist.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # set up histogram
         set nbin 100

         set vectors [list $ug $gr $ri $iz]
         set mins [list -0.5 -0.5 -0.5 -0.5]
         set maxs [list  3.5  2.0  2.0  1.2]
        
         foreach vector $vectors {
    
            set ivec [lsearch $vectors $vector]
            set min [lindex $mins $ivec]
            set max [lindex $maxs $ivec]
            set xlab [exprGet $vector.name]

            set hg [hgNew]
            hgDefine $hg -min $min -max $max -nbin $nbin -xLabel $xlab -yLabel N
            hgFillFromV $hg $vector
            hgPlot $pg $hg -xmin $min -xmax $max 
            if {$ivec == 0} {
               titlePlot $title 40 
            }
            hgDel $hg 
         }

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
         vPlot $pg $ug $mag(g) -xmin -0.5 -xmax 3.5 -ymin 23.0 -ymax 14.0 
         titlePlot $title 20      

         # r vs. g-r
         vPlot $pg $gr $mag(r) -xmin -0.5 -xmax 2.0 -ymin 23.0 -ymax 14.0 

         # i vs. r-i
         vPlot $pg $ri $mag(i) -xmin -0.5 -xmax 2.0 -ymin 23.0 -ymax 14.0 

         # z vs. i-z
         vPlot $pg $iz $mag(z) -xmin -0.5 -xmax 2.0 -ymin 23.0 -ymax 14.0 

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

         # mpsf(r)-mod(r) vs. mpsf(g)-mod(g)
         pgstateSet $pg -isNewplot 1 -icMark 1
         vPlot $pg $delmag(g) $delmag(r) -xmin -3.5 -xmax 4.5 -ymin -1.5 -ymax 4.5
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

       # plot color-color diagrams (FIRST and 2MASS)
       if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags_3match.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # r-i vs. g-r
         vPlot $pg $gr $ri -xmin -0.5 -xmax 2.0 -ymin -0.5 -ymax 2.0 
         titlePlot $title 20      

         if {$2MASS} {
            # J-K vs. i-z 
            vPlot $pg $iz $JK -xmin -0.5 -xmax 2.5 -ymin -0.5 -ymax 2.0

            # J-K vs. g-r
            vPlot $pg $gr $JK -xmin -0.5 -xmax 2.0 -ymin -0.5 -ymax 2.0
     }

         if {$2MASS && !$FIRST} {
            # J-K vs. r-K
            vPlot $pg $rK $JK -xmin -1.5 -xmax 10.0 -ymin -0.5 -ymax 2.0

         }

         if {$2MASS && $FIRST} {
            # K-mFIRST vs. J-K 
            vPlot $pg $JK $KF -xmin -0.5 -xmax 2.0 -ymin -5.0 -ymax 5.0 

            # K-mFIRST vs. g-r
            vPlot $pg $gr $KF -xmin -0.5 -xmax 2.0 -ymin -5.0 -ymax 5.0 

            # K-mFIRST vs. thetaFirst
            vPlot $pg $thetaFirst  $KF -xmin -0.5 -xmax 1.5 -ymin -5.0 -ymax 5.0 
         }

         # for FIRST matches only
         if {!$2MASS && $FIRST} {

           # g-r vs. u-g
           vPlot $pg $ug $gr -xmin -0.5 -xmax 3.5 -ymin -0.5 -ymax 2.0 

           # g-r vs. r-mFIRST 
           vPlot $pg $rF $gr -xmin -15.0 -xmax 15.0 -ymin -0.5 -ymax 2.0 

           # g-r vs. thetaFirst
           vPlot $pg $thetaFirst $gr -xmin -0.5 -xmax 1.5  -ymin -0.5 -ymax 2.0 
         }


         endplot $pg
      } msg] {
         echo "$msg"
      }

       # plot color-magnitude diagrams
       if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cm_diags_3match.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # r vs. g-r
         vPlot $pg $gr $mag(r) -xmin -0.5 -xmax 2.0 -ymin 22.0 -ymax 14.0
         titlePlot $title 20      

         if {$2MASS} {
            # K vs. J-K
            vPlot $pg $JK $K -xmin -0.5 -xmax 2.0 -ymin 16.0 -ymax 11.0
         }

         if {$2MASS && $FIRST} {
           # K vs. K-mFIRST
           vPlot $pg $KF $K -xmin -5.0 -xmax 5.0 -ymin 16.0 -ymax 11.0

           # mFIRSTpeak vs. K-mFIRST
           vPlot $pg $KF $mFpeak -xmin -5.0 -xmax 5.0 -ymin 18.0 -ymax 10.0

           # K vs. mFIRSTpeak 
           vPlot $pg $mFpeak $K -xmin 18.0 -xmax 10.0 -ymin 16.0 -ymax 11.0

           # mFIRSTpeak vs. thetaFirst
           vPlot $pg $thetaFirst $mFpeak -xmin -0.5 -xmax 1.5 -ymin 18.0 -ymax 10.0
         }

       
         # for FIRST matches only
         if {!$2MASS && $FIRST} {
           # r vs. r-mFIRST 
           vPlot $pg $rF $mag(r) -xmin -15.0 -xmax 15.0 -ymin 22.0 -ymax 14.0

           # mFIRSTpeak vs. r-mFIRST
           vPlot $pg $rF $mFpeak -xmin -15.0 -xmax 15.0 -ymin 18.0 -ymax 10.0

           # r vs. mFIRSTpeak 
           vPlot $pg $mFpeak $mag(r) -xmin 18.0 -xmax 10.0 -ymin 22.0 -ymax 14.0

           # mFIRSTpeak vs. thetaFirst
           vPlot $pg $thetaFirst $mFpeak -xmin -0.5 -xmax 1.5 -ymin 18.0 -ymax 10.0
         }


         endplot $pg
      } msg] {
         echo "$msg"
      }

      # plot various diagrams dealing with extended sources
      if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-ext_diags3.ps] 
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

         # petro50(r) vs. mpsf(r)-mod(r)
         vPlot $pg $delmag(r) $petroR50(r) -xmin -1.5 -xmax 4.5 -ymin 0.0 -ymax 15.0 

         if {$2MASS} {
           # petro50(r) vs. J-K
           vPlot $pg $JK $petroR50(r) -xmin -0.5 -xmax 2.0 -ymin 0.0 -ymax 15.0 
         }

         if {$2MASS && $FIRST} {
           # petro50(r) vs. K-mFIRST
           vPlot $pg $KF $petroR50(r) -xmin -5.0 -xmax 5.0 -ymin 0.0 -ymax 15.0 
         }

        

         vectorExprDel $delmag(g); vectorExprDel $delmag(r); 
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
     if {$2MASS} {
        vListDel [list $J $H $K $JH $HK $JK $zJ $rK]
     }
     if {$FIRST} {
     vListDel [list $rF $raFIRST $decFIRST $thetaFirst]
     vListDel [list $Frms $Fpeak $Fint $mFpeak $mFint]
     }
     if {$2MASS && $FIRST} {
    vListDel [list $KF]
     }
 
}



proc plot_all_SDSS2MASSpar {device parfile title {types ""} {symbol 4}} {

     if {$types == ""} {
         # n.b. does not include "stars" due to their large number
         set types [list white Astars blue lowzqso highzqso Cstars1 Cstars2 \
                  red1 red2 dwarfs1 dwarfs2 dusty top bottom uvexcess irexcess]
     } 
     foreach type $types {
          set c [param2Chain $parfile.$type ""]
          set N [chainSize $c]
          echo Found $N $type sources
      if {$N > 0} {
             plotSDSS2MASSpar $device $type "$title $type ($N)" $c $symbol
          }
          genericChainDel $c
     }
}



# for testing astrometry by using external catalogs, e.g. 2MASS
# set tsObjList [glob tsObj*fit]
# set chain [tsObjList2chain $tsObjList]
# check_2MASSastrometry $device astro_test $title $selectedChainofweird 
proc check_2MASSastrometry {device root title chain {filename ""}} {

     set allfilterlist {u g r i z}
 
     ### SDSS data
     set deredden 0; set modmag 1; set filterlist {g r} 
     foreach f $filterlist { 
         set i [lsearch $allfilterlist $f]
         if {$deredden} {
            set reddening [vFromChain $chain reddening<$i>]
            set mpsfRaw  [vFromChain $chain psfCounts<$i>]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            set mmodRaw  [vFromChain $chain counts_model<$i>]
            set mmod($f) [vectorExprEval $mmodRaw-$reddening]
            vectorExprDel $reddening
            vectorExprDel $mpsfRaw
            vectorExprDel $mmodRaw
         } else {
            set mpsf($f) [vFromChain $chain psfCounts<$i>]
            set mmod($f) [vFromChain $chain counts_model<$i>]
         }
         set epsf($f) [vFromChain $chain psfCountsErr<$i>]
         set emod($f) [vFromChain $chain counts_modelErr<$i>]
         set petroR50($f) [vFromChain $chain petroR50<$i>]
         vNameSet $petroR50($f) "petroR50(r')";
         vNameSet $mpsf($f) "mpsf(${f}')"; vNameSet $epsf($f) ""  
         vNameSet $mmod($f) "mmod(${f}')"; vNameSet $emod($f) ""  
     }     
     # colors (based on psf/model magnitudes)
     if {$modmag} {
        set gr [vectorExprEval $mmod(g)-$mmod(r)]
    foreach f $filterlist {
           set mag($f) $mmod($f)
        }       
     } else {
        set gr [vectorExprEval $mpsf(g)-$mpsf(r)]
    foreach f $filterlist {
           set mag($f) $mpsf($f)
        }       
     }
     set objc_type [vFromChain $chain objc_type]
     set type(r) [vFromChain $chain type<2>]

     set raSDSS [vFromChain $chain ra]
     set decSDSS [vFromChain $chain dec]
     set ra2MASS [vFromChain $chain lambda]
     set dec2MASS [vFromChain $chain eta]

     ### 2MASS data
     set J [vFromChain $chain fiberCounts<0>]; vNameSet $J "J"
     set H [vFromChain $chain fiberCounts<1>]; vNameSet $H "H"
     set K [vFromChain $chain fiberCounts<2>]; vNameSet $K "K"
     set JH [vectorExprEval $J-$H]; vNameSet $JH "J-H"             
     set HK [vectorExprEval $H-$K]; vNameSet $HK "H-K"             
     set JK [vectorExprEval $J-$K]; vNameSet $JK "J-K"             
     set rK [vectorExprEval $mag(r)-$K]; vNameSet $rK "r-K" 

     ### analysis
     set delRA [vectorExprEval 3600.0*($raSDSS-$ra2MASS)]
     set delDec [vectorExprEval 3600.0*($decSDSS-$dec2MASS)]
     vNameSet $delRA "RA(SDSS)-RA(2MASS) (arcsec)"; vNameSet $delDec "Dec(SDSS)-Dec(2MASS) (arcsec)"
     # histogram of differences
     set bSDSS  [vectorExprEval "$mag(r) < 19 ? 1 : 0"] 
     set b [vectorExprEval "$K < 13.55 ? $bSDSS : 0"] 
     set bS [vectorExprEval "$objc_type == 6 ? $b : 0"] 
     # galaxies
     set bG [vectorExprEval "$objc_type == 3 ? $b : 0"] 
     # blue stars
     set bB [vectorExprEval "$gr < 0.8 ? $bS : 0"] 
     # red stars
     set bR [vectorExprEval "$gr > 0.8 ? $bS : 0"] 

     set raHS [vecToHist $delRA 50  [list -0.5 0.5] $bS]
     set raHB [vecToHist $delRA 50 [list -0.5 0.5] $bB]
     set raHR [vecToHist $delRA 50 [list -0.5 0.5] $bR]
     set raHG [vecToHist $delRA 50 [list -0.5 0.5] $bG]
     set decHS [vecToHist $delDec 50 [list -0.5 0.5] $bS]
     set decHB [vecToHist $delDec 50 [list -0.5 0.5] $bB]
     set decHR [vecToHist $delDec 50 [list -0.5 0.5] $bR]
     set decHG [vecToHist $delDec 50 [list -0.5 0.5] $bG]
     set medianSra  [format "%4.2f" [vMedian $delRA -vMask $bS]] 
     set sigmaSra   [format "%4.2f" [vSigma $delRA -vMask $bS]] 
     set medianSdec  [format "%4.2f" [vMedian $delDec -vMask $bS]] 
     set sigmaSdec   [format "%4.2f" [vSigma $delDec -vMask $bS]]
     set medianSraB  [format "%4.2f" [vMedian $delRA -vMask $bB]] 
     set sigmaSraB   [format "%4.2f" [vSigma $delRA -vMask $bB]] 
     set medianSdecB  [format "%4.2f" [vMedian $delDec -vMask $bB]] 
     set sigmaSdecB   [format "%4.2f" [vSigma $delDec -vMask $bB]]
     set medianSraR  [format "%4.2f" [vMedian $delRA -vMask $bR]] 
     set sigmaSraR   [format "%4.2f" [vSigma $delRA -vMask $bR]] 
     set medianSdecR  [format "%4.2f" [vMedian $delDec -vMask $bR]] 
     set sigmaSdecR   [format "%4.2f" [vSigma $delDec -vMask $bR]]

     handleSet $raHS.xLabel "RA(SDSS)-RA(2MASS) (arcsec)"
     handleSet $decHS.xLabel "Dec(SDSS)-Dec(2MASS) (arcsec)"
     handleSet $raHS.yLabel "N (star)"
     handleSet $decHS.yLabel "N (star)"
     handleSet $raHB.xLabel "RA(SDSS)-RA(2MASS) (arcsec)"
     handleSet $decHB.xLabel "Dec(SDSS)-Dec(2MASS) (arcsec)"
     handleSet $raHB.yLabel "N (blue star)"
     handleSet $decHB.yLabel "N (blue star)"
     handleSet $raHR.xLabel "RA(SDSS)-RA(2MASS) (arcsec)"
     handleSet $decHR.xLabel "Dec(SDSS)-Dec(2MASS) (arcsec)"
     handleSet $raHR.yLabel "N (red star)"
     handleSet $decHR.yLabel "N (red star)"
     handleSet $raHG.xLabel "RA(SDSS)-RA(2MASS) (arcsec)"
     handleSet $decHG.xLabel "Dec(SDSS)-Dec(2MASS) (arcsec)"
     handleSet $raHG.yLabel "N (galaxy)"
     handleSet $decHG.yLabel "N (galaxy)"      
  
     ### all read, now plot ###
         # Set up the plot 
         set pg [pgstateNew]
         if {$filename == ""} {
             set filename astrometrySDSSvs2MASS.ps
         }
         set deviceA [devicename $device $filename] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -isLine 1 \
           -xfract 0.35 -yfract 0.35 -nxwindow 2 -nywindow 3
         set icLineDef [exprGet $pg.icLine]
         pgstateOpen $pg

         # panel 1
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raHS -xmin -0.5 -xmax 0.5
         titlePlot "$title" 10
         #pgstateSet $pg -isNewplot 0 -icLine 4
         #hgPlot $pg $raHG -xmin -0.5 -xmax 0.5
     if {1} {
         plot_line $pg -10 0 10 0 1 1 
         plot_line $pg 0 0 0 1000000 1 1 
         plot_info "med S: $medianSra" 90.0 0.1 -0.2
         plot_info "sig S: $sigmaSra" 90.0 0.1 0.5
         }

         # panel 2
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decHS -xmin -0.5 -xmax 0.5
         titlePlot "all stars with r < 19 and K < 13.55" 40 
         #pgstateSet $pg -isNewplot 0 -icLine 4
         #hgPlot $pg $decHG -xmin -0.5 -xmax 0.5
         if {1} {
         plot_line $pg -10 0 10 0 1 1 
         plot_line $pg 0 0 0 1000000 1 1 
         plot_info "med S: $medianSdec" 90.0 0.1 -0.2
         plot_info "sig S: $sigmaSdec" 90.0 0.1 0.5
         }

         if {0} { 
         # panel 3
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raHG -xmin -0.5 -xmax 0.5
         hgPlot $pg $raHG -xmin -0.5 -xmax 0.5
         # panel 4
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decHG -xmin -0.5 -xmax 0.5
         }


         # panel 3
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raHB -xmin -0.5 -xmax 0.5
         titlePlot "blue stars (g-r < 0.8)" 40
     if {1} {
         plot_line $pg -10 0 10 0 1 1 
         plot_line $pg 0 0 0 1000000 1 1 
         plot_info "med B: $medianSraB" 90.0 0.1 -0.2
         plot_info "sig B: $sigmaSraB" 90.0 0.1 0.5
         }
         # panel 4
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decHB -xmin -0.5 -xmax 0.5
         if {1} {
         plot_line $pg -10 0 10 0 1 1 
         plot_line $pg 0 0 0 1000000 1 1 
         plot_info "med B: $medianSdecB" 90.0 0.1 -0.2
         plot_info "sig B: $sigmaSdecB" 90.0 0.1 0.5
         }

         # panel 5
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raHR -xmin -0.5 -xmax 0.5
         titlePlot "red stars (g-r > 0.8)" 40
     if {1} {
         plot_line $pg -10 0 10 0 1 1 
         plot_line $pg 0 0 0 1000000 1 1 
         plot_info "med R: $medianSraR" 90.0 0.1 -0.2
         plot_info "sig R: $sigmaSraR" 90.0 0.1 0.5
         }
         # panel 6
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decHR -xmin -0.5 -xmax 0.5
         if {1} {
         plot_line $pg -10 0 10 0 1 1 
         plot_line $pg 0 0 0 1000000 1 1 
         plot_info "med R: $medianSdecR" 90.0 0.1 -0.2
         plot_info "sig R: $sigmaSdecR" 90.0 0.1 0.5
         }
         endplot $pg
        

     vListDel [list $bS $bG $bSDSS $b $bB $bR] 
     hgDel $raHS; hgDel $raHG; hgDel $decHS; hgDel $decHG;
     hgDel $raHB; hgDel $raHR; hgDel $decHB; hgDel $decHR;

    echo "leaking memory"



 }


# for testing astrometry by using triple SDSS-2MASS-FIRST
# set tsObjList [glob tsObj*fit]
# set chain [tsObjList2chain $tsObjList]
# check_triple_astrometry $device astro_test $title $chain
proc check_triple_astrometry {device filename title chain} {

     # get vectors
     set raSDSS  [vFromChain $chain ra]
     set ra2MASS [vFromChain $chain lambda]   
     set raFIRST [vFromChain $chain firstLambda]
     set decSDSS  [vFromChain $chain dec]
     set dec2MASS [vFromChain $chain eta]   
     set decFIRST [vFromChain $chain firstEta]

     # differences  
     set RAdiff1  [vectorExprEval 3600.0*($raSDSS-$ra2MASS)]
     set Decdiff1 [vectorExprEval 3600.0*($decSDSS-$dec2MASS)]
     set RAdiff2  [vectorExprEval 3600.0*($raSDSS-$raFIRST)]
     set Decdiff2 [vectorExprEval 3600.0*($decSDSS-$decFIRST)]
     set RAdiff3  [vectorExprEval 3600.0*($ra2MASS-$raFIRST)]
     set Decdiff3 [vectorExprEval 3600.0*($dec2MASS-$decFIRST)]
   
     # histogram of differences   
     set diffMin -1.1; set diffMax 1.1; set Nbin 22
     set raH1 [vecToHist $RAdiff1 $Nbin [list $diffMin $diffMax]]
     set decH1 [vecToHist $Decdiff1 $Nbin [list $diffMin $diffMax]]
     set raH2 [vecToHist $RAdiff2 $Nbin [list $diffMin $diffMax]]
     set decH2 [vecToHist $Decdiff2 $Nbin [list $diffMin $diffMax]]
     set raH3 [vecToHist $RAdiff3 $Nbin [list $diffMin $diffMax]]
     set decH3 [vecToHist $Decdiff3 $Nbin [list $diffMin $diffMax]]
     handleSet $raH1.xLabel "RA(SDSS)-RA(2MASS) (arcsec) "
     handleSet $raH1.yLabel "N"
     handleSet $decH1.xLabel "Dec(SDSS)-Dec(2MASS) (arcsec) "
     handleSet $decH1.yLabel "N"
     handleSet $raH2.xLabel "RA(SDSS)-RA(FIRST) (arcsec) "
     handleSet $raH2.yLabel "N"
     handleSet $decH2.xLabel "Dec(SDSS)-Dec(FIRST) (arcsec) "
     handleSet $decH2.yLabel "N" 
     handleSet $raH3.xLabel "RA(2MASS)-RA(FIRST) (arcsec) "
     handleSet $raH3.yLabel "N"
     handleSet $decH3.xLabel "Dec(2MASS)-Dec(FIRST) (arcsec) "
     handleSet $decH3.yLabel "N"

     # statistics
     set mask [vectorExprEval "$RAdiff1 > $diffMin && $RAdiff1 < $diffMax? 1 : 0"]
     set medianRA1  [format "%4.2f" [vMedian $RAdiff1 -vMask $mask]]
     set sigmaRA1   [format "%4.2f" [vSigma $RAdiff1 -vMask $mask]] 
     vectorExprDel $mask
     set mask [vectorExprEval "$Decdiff1 > $diffMin && $Decdiff1 < $diffMax? 1 : 0"]
     set medianDec1  [format "%4.2f" [vMedian $Decdiff1 -vMask $mask]] 
     set sigmaDec1   [format "%4.2f" [vSigma $Decdiff1  -vMask $mask]] 
     vectorExprDel $mask
     set mask [vectorExprEval "$RAdiff2 > $diffMin && $RAdiff2 < $diffMax? 1 : 0"]
     set medianRA2  [format "%4.2f" [vMedian $RAdiff2 -vMask $mask]] 
     set sigmaRA2   [format "%4.2f" [vSigma $RAdiff2 -vMask $mask]] 
     vectorExprDel $mask
     set mask [vectorExprEval "$Decdiff2 > $diffMin && $Decdiff2 < $diffMax? 1 : 0"]
     set medianDec2  [format "%4.2f" [vMedian $Decdiff2 -vMask $mask]] 
     set sigmaDec2   [format "%4.2f" [vSigma $Decdiff2 -vMask $mask]] 
     vectorExprDel $mask
     set mask [vectorExprEval "$RAdiff3 > $diffMin && $RAdiff3 < $diffMax? 1 : 0"]
     set medianRA3  [format "%4.2f" [vMedian $RAdiff3 -vMask $mask]] 
     set sigmaRA3   [format "%4.2f" [vSigma $RAdiff3 -vMask $mask]] 
     vectorExprDel $mask
     set mask [vectorExprEval "$Decdiff3 > $diffMin && $Decdiff3 < $diffMax? 1 : 0"]
     set medianDec3  [format "%4.2f" [vMedian $Decdiff3 -vMask $mask]] 
     set sigmaDec3   [format "%4.2f" [vSigma $Decdiff3 -vMask $mask]] 
     vectorExprDel $mask


     set info 1

      ### plot
         # Set up the plot 
         set pg [pgstateNew]
         if {$filename == ""} {
             set filename triple_astrometry.ps
         } 
         set deviceA [devicename $device $filename] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -isLine 1 \
           -xfract 0.35 -yfract 0.38 -nxwindow 2 -nywindow 3
         set icLineDef [exprGet $pg.icLine]
         pgstateOpen $pg

         # panel 1 RA
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raH1 -xmin $diffMin -xmax $diffMax -ymin 0     
         if {$info} {
            plot_line $pg -10 0 10 0 1 1 
            plot_line $pg 0 0 0 1000000 1 1 
            plot_info "med: $medianRA1" 90.0 0.1 -0.1
            plot_info "rms: $sigmaRA1" 90.0 0.1 0.5
         }  
         titlePlot $title 10    
         # panel 1 Dec
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decH1 -xmin $diffMin -xmax $diffMax -ymin 0 
         if {$info} {
            plot_line $pg -10 0 10 0 1 1 
            plot_line $pg 0 0 0 1000000 1 1 
            plot_info "med: $medianDec1" 90.0 0.1 -0.1
            plot_info "rms: $sigmaDec1" 90.0 0.1 0.5
         }
      
         # panel 2 RA
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raH2 -xmin $diffMin -xmax $diffMax -ymin 0      
         if {$info} {
            plot_line $pg -10 0 10 0 1 1 
            plot_line $pg 0 0 0 1000000 1 1 
            plot_info "med: $medianRA2" 90.0 0.1 -0.1
            plot_info "rms: $sigmaRA2" 90.0 0.1 0.5
         }        
         # panel 2 Dec
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decH2 -xmin $diffMin -xmax $diffMax -ymin 0 
         if {$info} {
            plot_line $pg -10 0 10 0 1 1 
            plot_line $pg 0 0 0 1000000 1 1 
            plot_info "med: $medianDec2" 90.0 0.1 -0.1
            plot_info "rms: $sigmaDec2" 90.0 0.1 0.5
         }
      
         # panel 3 RA
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $raH3 -xmin $diffMin -xmax $diffMax -ymin 0      
         if {$info} {
            plot_line $pg -10 0 10 0 1 1 
            plot_line $pg 0 0 0 1000000 1 1 
            plot_info "med: $medianRA3" 90.0 0.1 -0.1
            plot_info "rms: $sigmaRA3" 90.0 0.1 0.5
         }        
         # panel 3 Dec
         pgstateSet $pg -isNewplot 1 -icLine $icLineDef
         hgPlot $pg $decH3 -xmin $diffMin -xmax $diffMax -ymin 0 
         if {$info} {
            plot_line $pg -10 0 10 0 1 1 
            plot_line $pg 0 0 0 1000000 1 1 
            plot_info "med: $medianDec3" 90.0 0.1 -0.1
            plot_info "rms: $sigmaDec3" 90.0 0.1 0.5
         }
      
         endplot $pg
        
	 hgDel $raH1; hgDel $decH1; hgDel $raH2
         hgDel $decH2; hgDel $raH3; hgDel $decH3
    
   
}


### aux: will be done by Kristian in SM
proc plotSDSS2MASSpar {device root title chain {symbol -1}} {

     set filterlist {u g r i z}

     set grMax 3.0
     set grMax 5.0
 
     ### SDSS data
     set deredden 1
     foreach band $filterlist { 
         set f $band
         if {$deredden} {
            set redField ${band}Redd
            set reddening [vFromChain $chain $redField]
            set mpsfRaw  [vFromChain $chain $band]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            set modField ${band}mod
            set mmodRaw  [vFromChain $chain $modField]
            set mmod($f) [vectorExprEval $mmodRaw-$reddening]
            vectorExprDel $reddening
            vectorExprDel $mpsfRaw
            vectorExprDel $mmodRaw
         } else {
            set mpsf($f) [vFromChain $chain $band]
            set modField ${band}mod
            set mmodRaw  [vFromChain $chain $modField]
         }
         set errField ${band}Err
         set epsf($f) [vFromChain $chain $errField]
         set errField ${band}modErr
         set emod($f) [vFromChain $chain $errField]
         set p50Field ${band}R50
         set petroR50($f) [vFromChain $chain $p50Field]
         vNameSet $petroR50($f) "petroR50(r')";
         vNameSet $mpsf($f) "mpsf(${f}')"; vNameSet $epsf($f) ""  
         vNameSet $mmod($f) "mmod(${f}')"; vNameSet $emod($f) ""  
         ###
         # set mmod($f) $mpsf($f)  
     }     
     set ra [vFromChain $chain ra]; vNameSet $ra "RA J2000 (deg)"   
     set dec [vFromChain $chain dec]; vNameSet $dec "Dec J2000 (deg)"   
     set objc_type [vFromChain $chain objc_type]
     set type(r) [vFromChain $chain rtype]

     # SDSS colors (based on model magnitudes)
     set ug [vectorExprEval $mmod(u)-$mmod(g)]
     set gr [vectorExprEval $mmod(g)-$mmod(r)]
     set ri [vectorExprEval $mmod(r)-$mmod(i)]
     set iz [vectorExprEval $mmod(i)-$mmod(z)]
     set ur [vectorExprEval $mmod(u)-$mmod(r)]
     vNameSet $ug "u'-g'"; vNameSet $gr "g'-r'"; 
     vNameSet $ri "r'-i'"; vNameSet $iz "i'-z'"; 
     vNameSet $ur "u'-r'"; 


     # 2MASS data
     set J [vFromChain $chain J]; vNameSet $J "J"
     set H [vFromChain $chain H]; vNameSet $H "H"
     set K [vFromChain $chain K]; vNameSet $K "K"
     set Jerr [vFromChain $chain Jerr]
     set Herr [vFromChain $chain Herr]
     set Kerr [vFromChain $chain Kerr]
     set JH [vectorExprEval $J-$H]; vNameSet $JH "J-H"             
     set HK [vectorExprEval $H-$K]; vNameSet $HK "H-K"             
     set JK [vectorExprEval $J-$K]; vNameSet $JK "J-K"             

     # O-IR colors
     set zJ [vectorExprEval $mmod(z)-$J]; vNameSet $zJ "z-J"  
     set rK [vectorExprEval $mmod(r)-$K]; vNameSet $rK "r-K" 

     ### all read, now plot
     
     # plot SDSS color-color diagrams
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # g-r vs. u-g
         vPlot $pg $ug $gr -xmin -0.5 -xmax 3.5 -ymin -0.5 -ymax $grMax
         titlePlot $title 20      

         # r-i vs. g-r
         vPlot $pg $gr $ri -xmin -0.5 -xmax $grMax -ymin -0.5 -ymax 3.0 

         # i-z vs. r-i
         vPlot $pg $ri $iz -xmin -0.5 -xmax 3.0 -ymin -0.5 -ymax 3.0 

         # g vs. g-r
         vPlot $pg $gr $mmod(g) -xmin -0.5 -xmax $grMax -ymin 23.0 -ymax 14.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }
 
 
     # plot SDSS color-magnitude diagrams
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
         vPlot $pg $gr $mmod(r) -xmin -0.5 -xmax $grMax -ymin 23.0 -ymax 14.0 

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
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         set delmag(g) [vectorExprEval $mpsf(g)-$mmod(g)]
         set delmag(r) [vectorExprEval $mpsf(r)-$mmod(r)]
         set Lp50g [vectorExprEval 5.0*lg($petroR50(g))]  
         set Lp50r [vectorExprEval 5.0*lg($petroR50(r))]  
         vNameSet $delmag(g) "mpsf(g')-mmod(g')"
         vNameSet $delmag(r) "mpsf(r')-mmod(r')"
         vNameSet $Lp50g "5*log(petroR50(g))"
         vNameSet $Lp50r "5*log(petroR50(r))"


         # mpsf(r)-mod(r) vs. mmod(r)
         vPlot $pg $mmod(r) $delmag(r) -xmin 14.0 -xmax 23.0 -ymin -1.4 -ymax 4.5
         titlePlot $title 20      
         # definition for classifier
         set dmC 0.14
         plot_line $pg 0.0 $dmC 30.0 $dmC 1 2

         # mpsf(r)-mod(r) vs. mpsf(g)-mod(g)
         pgstateSet $pg -isNewplot 1 -icMark 1
         vPlot $pg $delmag(g) $delmag(r) -xmin -1.5 -xmax 4.5 -ymin -1.5 -ymax 4.5
         plot_line $pg $dmC $dmC 10.0 $dmC 1 2
         plot_line $pg $dmC $dmC $dmC 10.0 1 2 

         # 5*log(petro50(r)) vs. mod(r)
         pgstateSet $pg -isNewplot 1 -icMark 1
         vPlot $pg $mmod(r) $Lp50r -xmin 14.0 -xmax 23.0 -ymin 0.0 -ymax 6.5 

         # 5*log(petro50(r)) vs. 5*log(petro50(g))
         pgstateSet $pg -isNewplot 1 -icMark 1
         vPlot $pg $Lp50g $Lp50r -xmin 0.0 -xmax 6.5 -ymin 0.0 -ymax 6.5

         # 5*log(petro50(r)) vs. mod(g)-mod(r) 
         pgstateSet $pg -isNewplot 1 -icMark 1
         vPlot $pg $gr $Lp50r -xmin -0.5 -xmax 2.0 -ymin 0.0 -ymax 6.5  

         # mpsf(r)-mod(r) vs. 5*log(petro50(r))
         vPlot $pg $Lp50r $delmag(r) -xmin 0.0 -xmax 6.5 -ymin -0.5 -ymax 4.5 


         vectorExprDel $delmag(g); vectorExprDel $delmag(r); 
         vectorExprDel $Lp50g; vectorExprDel $Lp50r
         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # plot RA-Dec distribution 
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


     ### O-IR combined diagrams
  
     # plot color-magnitude diagrams I
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-OIRcm.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # r vs. g-r
         vPlot $pg $gr $mmod(r) -xmin -0.5 -xmax $grMax -ymin 23.0 -ymax 14.0 
         titlePlot $title 20      

         # r vs. z-J
         vPlot $pg $zJ $mmod(r) -xmin -1.5 -xmax 5.0 -ymin 23.0 -ymax 14.0 

         # J vs. J-K
         vPlot $pg $JK $J -xmin -0.5 -xmax 2.0 -ymin 17.0 -ymax 12.0 

         # J vs. z-J
         vPlot $pg $zJ $J -xmin -1.5 -xmax 5.0 -ymin 17.0 -ymax 12.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }

     # plot color-magnitude diagrams II
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-excesses.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 3
         pgstateOpen $pg

         # r-i vs. g-r
         vPlot $pg $gr $ri -xmin -0.5 -xmax 2.0 -ymin -0.5 -ymax 2.5 
         titlePlot $title 20 

         #  r vs.  r-i
         vPlot $pg $gr $mmod(r) -xmin -0.5 -xmax 2.5 -ymin 22.0 -ymax 14.0 
     
         # u-r vs. r-i
         vPlot $pg $ri $ur -xmin -0.5 -xmax 2.5  -ymin -1.0 -ymax 5.5

         # r-K vs. r-i
         vPlot $pg $ri $rK -xmin -0.5 -xmax 2.5  -ymin -0.5 -ymax 10.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }


     # plot 6 color-color diagrams as in Stars paper 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-OIRcc.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # r-i vs. g-r
         vPlot $pg $gr $ri -xmin -0.5 -xmax $grMax -ymin -0.5 -ymax 3.0 
         titlePlot $title 20      

         # z-J vs. i-z
         vPlot $pg $iz $zJ -xmin -0.5 -xmax 5.0 -ymin -1.5 -ymax 5.0 

         # J-K vs. g-r
         vPlot $pg $gr $JK -xmin -0.5 -xmax $grMax -ymin -0.5 -ymax 2.0

         # J-K vs. i-z
         vPlot $pg $iz $JK -xmin -0.5 -xmax 5.0 -ymin -0.5 -ymax 2.0

         # H-K vs. J-H
         vPlot $pg $JH $HK -xmin -0.5 -xmax 1.5 -ymin -0.5 -ymax 1.5
 
         # J-K vs. r-K
         vPlot $pg $rK $JK -xmin -0.5 -xmax 10.0 -ymin -0.5 -ymax 2.0

         endplot $pg
     } msg] {
         echo "$msg"
     }


     # plot 6 color-color and color-magnitude diagrams to emphasize S/G separation
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-OIRsg.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # r-i vs. g-r
         vPlot $pg $gr $ri -xmin -0.5 -xmax 2.0 -ymin -0.5 -ymax 3.0 
         titlePlot $title 20      

         # z-J vs. i-z
         vPlot $pg $iz $zJ -xmin -0.5 -xmax 2.0 -ymin -2.0 -ymax 2.5 

         # r vs. z-J
         vPlot $pg $zJ $mmod(r) -xmin -2.0 -xmax 2.5 -ymin 22.0 -ymax 14.0 

         # J-K vs. z-J
         vPlot $pg $zJ $JK -xmin -2.0 -xmax 2.5 -ymin -0.5 -ymax 2.0

         # J vs. z-J
         vPlot $pg $zJ $J -xmin -2.0 -xmax 2.5 -ymin 17.0 -ymax 12.0 
 
         # J-K vs. r-K
         vPlot $pg $rK $JK -xmin -1.5 -xmax 7.0 -ymin -0.5 -ymax 2.5

         endplot $pg
     } msg] {
         echo "$msg"
     }

     # plot EROs diagrams
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-EROs.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # z-J vs. i-z
         vPlot $pg $iz $zJ -xmin -0.5 -xmax 5.0 -ymin -1.5 -ymax 5.0 
         titlePlot $title 20      

         # J-K vs. z-J
         vPlot $pg $zJ $JK -xmin -1.0 -xmax 5.5 -ymin -0.5 -ymax 3.0

         # H-K vs. J-H
         vPlot $pg $JH $HK -xmin -1.0 -xmax 2.5 -ymin -1.0 -ymax 2.5

         # z vs. z-J
         vPlot $pg $zJ $mmod(z) -xmin -1.5 -xmax 5.0 -ymin 21.0 -ymax 14.0 

         # J vs. z-J
         vPlot $pg $zJ $J -xmin -1.5 -xmax 5.0 -ymin 17.0 -ymax 12.0 

         # z vs J with error bars
         vPlot $pg $mmod(z) $J -vectorXErr $emod(z) -vectorYErr $Jerr \
               -xmin 14.0 -xmax 21.0 -ymin 17.0 -ymax 12.0 

         endplot $pg
     } msg] {
         echo "$msg"
     }

 
     # clean
     vListDel [list $ug $gr $ri $iz $ra $dec $objc_type $type(r)]
     vListDel [list $J $H $K $Jerr $Herr $Kerr $JH $HK $JK $zJ $rK]
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
        vListDel [list $mpsf($f) $mmod($f) $epsf($f) $emod($f) $petroR50($f)]
     }     

}    







### for studying variod magnitudes
proc plotSDSS2MASSXSCpar {device root title chain {symbol -1}} {

     set filterlist {u g r i z}

     set grMax 3.0
     set grMax 5.0
 
     ### SDSS data
     set deredden 1
     foreach band $filterlist { 
         set f $band
         if {$deredden} {
            set redField ${band}Redd
            set reddening [vFromChain $chain $redField]
            set mpsfRaw  [vFromChain $chain $band]
            set mpsf($f) [vectorExprEval $mpsfRaw-$reddening]
            set modField ${band}mod
            set mmodRaw  [vFromChain $chain $modField]
            set mmod($f) [vectorExprEval $mmodRaw-$reddening]
            set petField ${band}Pet
            set mpetRaw  [vFromChain $chain $petField]
            set mpet($f) [vectorExprEval $mpetRaw-$reddening]
            vectorExprDel $reddening
            vectorExprDel $mpsfRaw
            vectorExprDel $mmodRaw
            vectorExprDel $mpetRaw
         } else {
            set mpsf($f) [vFromChain $chain $band]
            set modField ${band}mod
            set mmod($f) [vFromChain $chain $modField]
            set petField ${band}Pet
            set mpet($f)  [vFromChain $chain $petField]
         }
         set errField ${band}Err
         set epsf($f) [vFromChain $chain $errField]
         set errField ${band}modErr
         set emod($f) [vFromChain $chain $errField]
         set errField ${band}PetErr
         set epet($f) [vFromChain $chain $errField]
         set p50Field ${band}R50
         set petroR50($f) [vFromChain $chain $p50Field]
         vNameSet $petroR50($f) "petroR50(r')";
         vNameSet $mpsf($f) "mpsf(${f}')"; vNameSet $epsf($f) ""  
         vNameSet $mmod($f) "mmod(${f}')"; vNameSet $emod($f) ""  
         vNameSet $mpet($f) "mpet(${f}')"; vNameSet $epet($f) ""  
         ###
     }     
     set ra [vFromChain $chain ra]; vNameSet $ra "RA J2000 (deg)"   
     set dec [vFromChain $chain dec]; vNameSet $dec "Dec J2000 (deg)"   
     set objc_type [vFromChain $chain objc_type]
     set type(r) [vFromChain $chain rtype]

     # 2MASS data
     set J [vFromChain $chain J]; vNameSet $J "J"
     set H [vFromChain $chain H]; vNameSet $H "H"
     set K [vFromChain $chain K]; vNameSet $K "K"
     set Jerr [vFromChain $chain Jerr]
     set Herr [vFromChain $chain Herr]
     set Kerr [vFromChain $chain Kerr]
     set JH [vectorExprEval $J-$H]; vNameSet $JH "J-H"             
     set HK [vectorExprEval $H-$K]; vNameSet $HK "H-K"             
     set JK [vectorExprEval $J-$K]; vNameSet $JK "J-K"             


     # Galaxy separation
     set ur(mod) [vectorExprEval $mmod(u)-$mmod(r)]
     set Smask [vectorExprEval "$ur(mod) < 2.22  ? 1 : 0"]
     set Emask [vectorExprEval "$ur(mod) >= 2.22  ? 1 : 0"]



     # Colors  
     set gi(psf) [vectorExprEval $mpsf(g)-$mpsf(i)]
     vNameSet $gi(psf) "gpsf-ipsf"
     set iK(psf) [vectorExprEval $mpsf(i)-$K]
     vNameSet $iK(psf) "ipsf-Kdef"
     set gi(mod) [vectorExprEval $mmod(g)-$mmod(i)]
     vNameSet $gi(mod) "gmod-imod"
     set iK(mod) [vectorExprEval $mmod(i)-$K]
     vNameSet $iK(mod) "imod-Kdef"
     set gi(pet) [vectorExprEval $mpet(g)-$mpet(i)]
     vNameSet $gi(pet) "gpet-ipet"
     set iK(pet) [vectorExprEval $mpet(i)-$K]
     vNameSet $iK(pet) "ipet-Kdef"

     # Sizes
     set psfpetr [vectorExprEval $mpsf(r)-$mpet(r)]
     vNameSet $psfpetr "rpsf-rpet"
     set logSBr [vectorExprEval 0*$mpsf(r)]
     loop i 0 [exprGet $logSBr.dimen] { 
	 set aux1 [expr 0.0001+abs([exprGet $petroR50(r).vec<$i>])]
         set aux2 [exprGet $mpet(r).vec<$i>]
         handleSet $logSBr.vec<$i> [expr $aux2+5*log10($aux1)]
     } 
     vNameSet $logSBr "rpet+5*log10\[R50(r)\]"

 
   ### all read, now plot ###    
     # gi and iK histograms with different magnitudes
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags1.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg


         foreach mtype {psf mod pet} {
	    if {$mtype == "psf"} {titlePlot $title 20}      
	    set hg [vecToHist $gi($mtype) 30 {0.0 3.0}]
            handleSet $hg.xLabel "g$mtype-i$mtype"
            handleSet $hg.yLabel "N"
            hgPlot $pg $hg -xmin -1.0 -xmax 3.0 
            hgDel $hg
	    set hg [vecToHist $iK($mtype) 60 {1.0 7.0}]
            handleSet $hg.yLabel "N"
            handleSet $hg.xLabel "i$mtype-Kdef"
            hgPlot $pg $hg -xmin 1.0 -xmax 7.0 
            hgDel $hg
         }

      
         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # gi and iK histograms with different magnitudes for S galaxies
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags1S.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg
         pgstateSet $pg -icLine 4

         foreach mtype {psf mod pet} {
	    if {$mtype == "psf"} {titlePlot $title 20}      
	    set hg [vecToHist $gi($mtype) 30 {0.0 3.0} $Smask]
            handleSet $hg.xLabel "g$mtype-i$mtype"
            handleSet $hg.yLabel "N"
            hgPlot $pg $hg -xmin -1.0 -xmax 3.0 
            hgDel $hg
	    set hg [vecToHist $iK($mtype) 60 {1.0 7.0} $Smask]
            handleSet $hg.yLabel "N"
            handleSet $hg.xLabel "i$mtype-Kdef"
            hgPlot $pg $hg -xmin 1.0 -xmax 7.0 
            hgDel $hg
         }

      
         endplot $pg
     } msg] {
         echo "$msg"
     }

     # gi and iK histograms with different magnitudes for E galaxies
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags1E.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg
         pgstateSet $pg -icLine 2

         foreach mtype {psf mod pet} {
	    if {$mtype == "psf"} {titlePlot $title 20}      
	    set hg [vecToHist $gi($mtype) 30 {0.0 3.0} $Emask]
            handleSet $hg.xLabel "g$mtype-i$mtype"
            handleSet $hg.yLabel "N"
            hgPlot $pg $hg -xmin -1.0 -xmax 3.0 
            hgDel $hg
	    set hg [vecToHist $iK($mtype) 60 {1.0 7.0} $Emask]
            handleSet $hg.yLabel "N"
            handleSet $hg.xLabel "i$mtype-Kdef"
            hgPlot $pg $hg -xmin 1.0 -xmax 7.0 
            hgDel $hg
         }

      
         endplot $pg
     } msg] {
         echo "$msg"
     }


     # sizes in r
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags2.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg
 
         pgstateSet $pg -icMark 4
         vPlot $pg $mpet(r) $psfpetr  -vectorMask $Smask \
               -xmin 12.0 -xmax 19.0 -ymin -0.5 -ymax 4.0 
         pgstateSet $pg -icMark 2
         vPlot $pg $mpet(r) $psfpetr  -vectorMask $Emask \
               -xmin 12.0 -xmax 19.0 -ymin -0.5 -ymax 4.0 
  
         pgstateSet $pg -icMark 4
         vPlot $pg $petroR50(r) $psfpetr  -vectorMask $Smask \
               -xmin 0.0 -xmax 10.0 -ymin -0.5 -ymax 4.0 
         pgstateSet $pg -icMark 2
         vPlot $pg $petroR50(r) $psfpetr  -vectorMask $Emask \
               -xmin 0.0 -xmax 10.0 -ymin -0.5 -ymax 4.0 
  
         pgstateSet $pg -icMark 4
         vPlot $pg $logSBr $psfpetr  -vectorMask $Smask \
               -xmin 15.0 -xmax 21.0 -ymin -0.5 -ymax 4.0 
         pgstateSet $pg -icMark 2
         vPlot $pg $logSBr $psfpetr  -vectorMask $Emask \
               -xmin 15.0 -xmax 21.0 -ymin -0.5 -ymax 4.0 
  
      
         endplot $pg
     } msg] {
         echo "$msg"
     }
 

     # g-i color vs. size in r
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags3.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg
 
         foreach mtype {psf mod pet} {   
            pgstateSet $pg -icMark 4
            vPlot $pg $petroR50(r) $gi($mtype)  -vectorMask $Smask \
               -xmin 0.0 -xmax 10.0 -ymin 0.0 -ymax 3.0 
            pgstateSet $pg -icMark 2
            vPlot $pg $petroR50(r) $gi($mtype)  -vectorMask $Emask \
               -xmin 0.0 -xmax 10.0 -ymin 0.0 -ymax 3.0 
         }
  
      
         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # i-K color vs. size in r
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags4.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg
 
         foreach mtype {psf mod pet} {   
            pgstateSet $pg -icMark 4
            vPlot $pg $petroR50(r) $iK($mtype)  -vectorMask $Smask \
               -xmin 0.0 -xmax 10.0 -ymin 1.0 -ymax 7.0 
            pgstateSet $pg -icMark 2
            vPlot $pg $petroR50(r) $iK($mtype)  -vectorMask $Emask \
               -xmin 0.0 -xmax 10.0 -ymin 1.0 -ymax 7.0 
         }
  
      
         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # g-i color vs. SB  in r
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags5.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg
 
         foreach mtype {psf mod pet} {   
            pgstateSet $pg -icMark 4
            vPlot $pg $logSBr $gi($mtype)  -vectorMask $Smask \
               -xmin 15.0 -xmax 21.0 -ymin 0.0 -ymax 3.0 
            pgstateSet $pg -icMark 2
            vPlot $pg $logSBr $gi($mtype)  -vectorMask $Emask \
               -xmin 15.0 -xmax 21.0 -ymin 0.0 -ymax 3.0 
         }
  
      
         endplot $pg
     } msg] {
         echo "$msg"
     }
 
     # i-K color vs. SB in r
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $root-cc_diags6.ps] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.3 -nxwindow 2 -nywindow 4
         pgstateOpen $pg
 
         foreach mtype {psf mod pet} {   
            pgstateSet $pg -icMark 4
            vPlot $pg $logSBr $iK($mtype)  -vectorMask $Smask \
               -xmin 15.0 -xmax 21.0 -ymin 1.0 -ymax 7.0 
            pgstateSet $pg -icMark 2
            vPlot $pg $logSBr $iK($mtype)  -vectorMask $Emask \
               -xmin 15.0 -xmax 21.0 -ymin 1.0 -ymax 7.0 
         }
  
      
         endplot $pg
     } msg] {
         echo "$msg"
     }
 





     # clean
     vListDel [list $ra $dec $objc_type $type(r)]
     vListDel [list $J $H $K $Jerr $Herr $Kerr $JH $HK $JK]
     vListDel [list $Smask $Emask $psfpetr $logSBr]
     loop i 0 5 { 
        set f [lindex $filterlist $i] 
	 vListDel [list $mpsf($f) $mmod($f) $mpet($f) $epsf($f) $emod($f) $epet($f)]
	 vListDel [list $petroR50($f)]
     }     
     foreach mtype {psf mod pet} {
         vListDel [list $gi($mtype) $iK($mtype)] 
     }

}    


### aux: produce a list of colors and magnitudes for a SDSS-2MASS match
proc formatSDSS2MASSpar {parfile outfile {good_flags 0}} {

   
     set chain [param2Chain $parfile ""]
 
     # dump to file 
     set outf [open $outfile w]
     puts $outf "# "
     puts $outf "#          ra         dec   S/G  Ar    r      u-g   g-r   r-i  \
                 i-z  J     J-H   H-K   J-K   z-J   r-K  -- 2MASS flags --"
 
     set Nall [chainSize $chain]
     set Ngood 0
     loop i 0 $Nall {
   
             set el [chainElementGetByPos $chain $i]
             set sg [exprGet $el.objc_type]
             if {$sg == 3} {
                  set type " G"
             } elseif {$sg == 6} {
                  set type " S" 
             } else {
                  set type " X"
             }
             # dump to text file
             foreach rec {ra dec J Jerr H Herr K Kerr} {
                 set S$rec [exprGet $el.$rec]
             }
             foreach rec {u g r i z} {
                 set S$rec [exprGet $el.${rec}mod]
             }
             set flags "[exprGet $el.rd_flg] [exprGet $el.bl_flg] [exprGet $el.cc_flg]"
             set sJH [expr $SJ-$SH]; set sHK [expr $SH-$SK]; set sJK [expr $SJ-$SK]
             set coord "[format "%10.6f %10.6f" $Sra $Sdec]"
             set data "[format "%5.2f %5.2f  %5.2f %5.2f  %5.2f %5.2f" \
                           $SJ $SJerr $SH $SHerr $SK $SKerr]"
             set ug [expr $Su - $Sg]; set gr [expr $Sg - $Sr]; 
             set ri [expr $Sr - $Si]; set iz [expr $Si - $Sz]; 
             set zJ [expr $Sz - $SJ]; set rK [expr $Sr - $SK]
             set sdss "[format "%5.2f  %5.2f %5.2f %5.2f %5.2f" \
                           $Sr $ug $gr $ri $iz]"
             set Ar "[format "%5.2f" [exprGet $el.rRedd]]"
             set oir "[format "%5.2f %5.2f" $zJ $rK]"
             set col "[format "%5.2f %5.2f %5.2f" $sJH $sHK $sJK]"
             set line " [format "%4.0f" $i] $coord $type $Ar $sdss [format "%5.2f" $SJ] $col $oir"
 	     if {$good_flags < 0} { 
                  puts $outf "$line"
                  incr Ngood
             } elseif {!$good_flags || ($good_flags && [string first "222 111 000" $flags] != -1)} {
	          puts $outf "$line    $flags"
                  incr Ngood
             }
        
     }

     close $outf
     genericChainDestroy $chain

     echo "dumped $Ngood (out of $Nall) elements"
    
}




### aux: produce a nicely formatted table of 2MASS sources
proc format2MASSpar {parfile outfile} {

     set chain [param2Chain $parfile ""]

     # dump to file
     set outf [open $outfile w]
     puts $outf "# "
     puts $outf \
         "#          ra         dec    rd  bl  cc mp   J    Jerr    H    Herr    K    Kerr  J-K"

     loop i 0 [chainSize $chain] {
             set el [chainElementGetByPos $chain $i]
             foreach rec {ra dec J J_err H H_err K K_err mp_flg} {
                 set S$rec [exprGet $el.$rec]
             }
   	     set SJK [format "%5.2f" [expr $SJ-$SK]]
             set coord "[format "%10.6f %10.6f" $Sra $Sdec]"
             set data  "[format "%6.2f %5.2f  %5.2f %5.2f  %5.2f %5.2f %5.2f" \
                           $SJ $SJ_err $SH $SH_err $SK $SK_err $SJK]"
             set flags "[exprGet $el.rd_flg] [exprGet $el.bl_flg] [exprGet $el.cc_flg]"
             set line " [format "%4.0f" [expr $i+1]] $coord $flags $Smp_flg $data"
             puts $outf $line
     }

     close $outf
     genericChainDestroy $chain

}




# A simple proc to select objects with mtype1(f1)-mtype2(f2) between L1 and L2.
# f1 and f2 can be any of {u g r i z}. Allowed magnitude types, mtype1 and mtype2,
# are {psf, fiber, petro, exp, deV, model}. 
# mtype1, mtype2, f1, f2, L1, and L2 are passed as globals and have to be set 
# before calling this proc.
# Can be used with e.g. myObj2tsObj
# e.g. to select asteroids without using moving object algorithm
# photo> global mtype1 mtype2 f1 f2 L1 L2 
# photo> set mtype1 psf; set mtype2 model; set f1 g; set f2 g; set L1 -10; set L2 -1 
# *** DOES NOT CORRECT FOR INTERSTELLAR REDDENING ***
proc sel_psfmod {obj} {
 
global mtype1 mtype2 f1 f2 L1 L2 
  
    if {$mtype1 == "psf" || $mtype1 == "fiber" || $mtype1 == "petro"} {
        set field1 ${mtype1}Counts
    } elseif {$mtype1 == "exp" || $mtype1 == "deV" || $mtype1 == "model"} {
        set field1 counts_${mtype1}
    } else {
        set field1 $mtype1
    }
    if {$mtype2 == "psf" || $mtype2 == "fiber" || $mtype2 == "petro"} {
        set field2 ${mtype2}Counts
    } elseif {$mtype2 == "exp" || $mtype2 == "deV" || $mtype2 == "model"} {
        set field2 counts_${mtype2}
    } else {
        set field2 $mtype2
    }


    set filterlist {u g r i z}  
    set band1 [lsearch $filterlist $f1]
    set band2 [lsearch $filterlist $f2]

    # get the desired quantity
    set counts1 [exprGet $obj.$field1<$band1>]
    set counts2 [exprGet $obj.$field2<$band2>]
    set diff [expr $counts1 - $counts2]
    if {$diff > $L1 && $diff < $L2} {
        return 1
    } else {
        return 0
    }

}




################################################################
#### matching to SDSSVAR files

# set all available variability files
proc setupSDSSVAR {{stats 0}} {
 
     set RootDir /u/dss/data/Science/Variability
     set SDSSVARlist ""

    
     set file  "$RootDir/DR1data/All745-20vs756-20.dat"
     set run1 745
     set T1 51257  
     set run2 756
     set T2 51259
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 10N
     set area 102.4 
     set Nobj 546700
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All1458-2vs1992-0.par.dat "
     set run1 1458
     set T1   51668
     set run2 1992
     set T2   51901
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 11N
     set area 11.5
     set Nobj 39224
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All2125-0vs2247-0.par.dat"
     set run1 2125
     set T1   51960
     set run2 2247
     set T2   52015
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 12S
     set area 7.5
     set Nobj 24274
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All2887-0vs2961-0.dat"
     set run1 2887
     set T1   52288
     set run2 2961
     set T2   52313
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 30S
     set area 26.3
     set Nobj 169936
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


     set file  "$RootDir/AllDat/All2887-0vs3059-0.dat"
     set run1 2887
     set T1   52288
     set run2 3059
     set T2   52354
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 30S
     set area 11.1
     set Nobj 32383
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


     set file  "$RootDir/AllDat/All2826-0vs2863-0.par.dat"
     set run1 2826
     set T1   52262
     set run2 2863
     set T2   52283
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 33N
     set area 40.7
     set Nobj 66731
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


     set file  "$RootDir/AllDat/All2299-0vs2304-0.par.dat"
     set run1 2299
     set T1   52046
     set run2 2304
     set T2   52047
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 35N
     set area 37.8
     set Nobj 118205
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


     set file  "$RootDir/AllDat/All1345-4vs2189-0.par.dat"
     set run1 1345
     set T1   51639
     set run2 2189
     set T2   51987
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 36N
     set area 20.4
     set Nobj 60745
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All1345-4vs2335-0.par.dat"
     set run1 1345
     set T1   51639
     set run2 2335
     set T2   52053
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 36N
     set area 12.1
     set Nobj 99653
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All1402-1vs1450-5.par.dat"
     set run1 1402
     set T1   51659
     set run2 1450
     set T2   51667
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 37N
     set area 20.2
     set Nobj 70694
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/DR1data/All2206-20vs2207-20.dat"
     set run1 2206
     set T1 51991
     set run2 2207
     set T2 51991
     set deltaT [expr 0.125]
     set strip 37S
     set area 31.8
     set Nobj 300621
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


     set file  "$RootDir/AllDat/All1043-2vs1904-0.par.dat"
     set run1 1043
     set T1   51465
     set run2 1904
     set T2   51878
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 76N
     set area 42.3
     set Nobj 139027
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All1739-0vs1904-0.par.dat"
     set run1 1739
     set T1   51814
     set run2 1904
     set T2   51878
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 76N
     set area 32.2
     set Nobj 161648
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All94-7vs2873-0.dat"
     set run1 94
     set T1 51075
     set run2 2873
     set T2 52287
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 82N
     set area 46.7 
     set Nobj 140441
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All1752-0vs2589-0.par.dat"
     set run1 1752
     set T1 51818
     set run2 2589
     set T2 52173
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 82N
     set area 45.3 
     set Nobj 144006
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All1033-2vs2662-0.par.dat"
     set run1 1033
     set T1 51464
     set run2 2662
     set T2 52197
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 82N
     set area 44.9 
     set Nobj 277713 
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file  "$RootDir/AllDat/All125-7vs1755-0.par.dat"
     set run1 125; set T1 51081
     set run2 1755; set T2 51819 
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 82S
     set area 74.9
     set Nobj 183288 
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]

     set file "$RootDir/AllDat/All2583-0vs1755-0.par.dat"
     set run1 2583 
     set T1 52172
     set run2 1755
     set T2 51819 
     set deltaT [expr abs(1.0*($T2-$T1))]
     set strip 82S
     set area 29.3 
     set Nobj 272541
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


     set file "$RootDir/DR1data/All1894-20vs2709-20.dat"
     set run1 1894
     set T1   51875
     set run2 2709
     set T2   52225
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 82S
     set area 33.2
     set Nobj 114517 
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


     set file "$RootDir/AllDat2/All2583-0vs2585-0.dat"
     set run1 2583
     set T1   52172
     set run2 2585
     set T2   52172
     set deltaT 0.125
     set strip 82S
     set area 11.1
     set Nobj 76984
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]


#  There are 20 SDSSVAR files, with 3039331 objects 
#   in AreaTot = 681 deg^2, with deltaT ranging from
#   0.125 to 1212.0 days

# We discuss multi-epoch SDSS observations of about 700 deg^2
# of sky, that provide 5-band photometry for 3,000,000 point sources, 
# with time spans ranging from 3 hours to 3 years. 


     if {0} {
     set file  "$RootDir/"
     set run1 
     set T1 
     set run2 
     set T2 
     set deltaT [expr 1.0*($T2-$T1)]
     set strip 
     set area 
     set Nobj 
     set SDSSVARdata "$file $run1 $run2 $strip $T1 $T2 $deltaT $area $Nobj"
     if {![testSDSSVAR4doubles $run1 $run2 $SDSSVARlist]} {error "double counting!"}
     set SDSSVARlist [lappend SDSSVARlist $SDSSVARdata]
     }


     if {$stats} {
         set areaTot 0.0
         set NobjTot 0
         set deltaTmin 100000
         set deltaTmax 0
	 foreach var $SDSSVARlist { 
             set areaTot [expr $areaTot + [lindex $var 7]] 
	     set NobjTot [expr $NobjTot + [lindex $var 8]]
             if {[lindex $var 6] > $deltaTmax} {set deltaTmax [lindex $var 6]}
             if {[lindex $var 6] < $deltaTmin} {set deltaTmin [lindex $var 6]}

         }
	 echo " There are [llength $SDSSVARlist] SDSSVAR files, with $NobjTot objects "
         echo "  in AreaTot = [expr int($areaTot)] deg^2, with deltaT ranging from"
         echo "  $deltaTmin to $deltaTmax days"           
     }
    

  return $SDSSVARlist

}

# test that SDSSVAR overlaps are not used multiple times
proc testSDSSVAR4doubles {run1 run2 SDSSVARlist} {   
    set OK 1

    foreach var $SDSSVARlist {
	set R1 [lindex $var 1]
        set R2 [lindex $var 2]
        if {$R1 == $run1 && $R2 == $run2} {set OK 0; set varD $var}
        if {$R1 == $run2 && $R2 == $run1} {set OK 0; set varD $var}
        if {!$OK} {continue}
    }
  
    if {1 && !$OK} {
       echo "run1 = $run1, run2 = $run2, file="
       echo "[lindex $varD 0]"
  echo $varD
    }

  return $OK
}


proc makeSDSSVARtable {SDSSVARlist {tex 0}} {

     echo "   run1   run2 strip    T1      T2   deltaT   area     Nobj"

     foreach var $SDSSVARlist {
	set run1 [format "%6d" [lindex $var 1]]
	set run2 [format "%6d" [lindex $var 2]]
	set strip [lindex $var 3]
        set T1 [lindex $var 4]
        set T2 [lindex $var 5]
        set deltaT [format "%6.1f" [lindex $var 6]]
        set area [format "%5.1f" [lindex $var 7]]
        set Nobj [format "%9.0f" [lindex $var 8]]
        echo " $run1 $run2  $strip   $T1   $T2  $deltaT  $area $Nobj"
     }

  return 

}

# matching outside file to SDSSVAR files
# e.g.
# matchSDSSVAR variability_check.dat 1 2 matt_SDSSVAR.dat
proc matchSDSSVAR {inputFile colRA colDec outFile {SDSSVARlist ""}} {

global express reverseOutput makeSubFiles

     # want each line to start with SDSSVAR for compatibility with SM
     set reverseOutput 1
     exec echo "" > $outFile
     # first get list of all available output files
     if {$SDSSVARlist == ""} {
         set SDSSVARlist [setupSDSSVAR]
     }

     # loop over all SDSSVAR files
     foreach var $SDSSVARlist {
         set file [lindex $var 0]
         if {0} {
	    set file ${file}.CLEAN
         }
	 echo; echo " == matching $inputFile to $file"
         if {[info exist makeSubFiles] && $makeSubFiles} {
            set express 1
         } else {
            catch {set subf [glob ${file}_*]}
            if {[info exists subf] && [llength $subf] > 1} {          
               set express 2
               unset subf             
            } else {
               set express 1
            }
         }
         # now do the matching
         cross_correlate_list $inputFile $file matches aux $colRA $colDec 0 10 11 0 1.0 1
         set Nmatch [lindex [exec wc -l matches] 0]
         catch {exec cat matches >> $outFile}
         catch {exec rm matches}
         catch {exec rm aux}
         echo "    -> $Nmatch matches"                  
     }

    set Nmatch [lindex [exec wc -l $outFile] 0]

    echo;
    echo " ======"
    echo " ======  Found $Nmatch matches and stored to $outFile  ======"
    echo " ======"

  return $Nmatch 
}
