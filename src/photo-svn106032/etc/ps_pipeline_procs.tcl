#
# We have loaded this file
#
set ps_pipeline_procs_tcl 1

proc psp { {ext ""} } {
   
   if {$ext == ""} {
      run_ps_pipeline psPlan.par
   } else {
      foreach col $ext {
         run_ps_pipeline psPlan-$col.par
         catch { exec cp mem.trace mem.tracePSP_$col } 
      }
   }
}


#
# Get some numbers from enums
#
set_array_from_define PSP_STATUS PSP_FIELD_ PSP_STATUS


# used for writing PT calibration to psPSF/psField files
typedef struct {
   char filter;
   float flux20;
   float flux20Err;
} PT_AUX; 
 
# used for appending wing correction psField files
typedef struct {
   char filter;
   int nann_ap_frame;
   int nann_ap_run;
   float ap_corr_run;
   float ap_corr_runErr;
} WC_AUX; 


###############################################################################
# It may happen (especially in Princeton) that a file appears not to exist
# because it's on an automounted filesystem. This proc attempts to convince
# amd that it should remount before regReadAsFits fails to read a file
#
proc ping_amd {file} {
   if [file exists $file] {
      file atime $file
   } else {
      catch {
         file atime $file
      }
   }
}

if {[info commands regReadAsFits.vanilla] == ""} {
   rename regReadAsFits regReadAsFits.vanilla
   proc regReadAsFits {reg file} {
      ping_amd $file
      regReadAsFits.vanilla $reg $file
   }
}

if {[info commands regWriteAsFits.vanilla] == ""} {
   rename regWriteAsFits regWriteAsFits.vanilla
   proc regWriteAsFits {reg file} {
      ping_amd $file
      regWriteAsFits.vanilla $reg $file
   }
}




# This proc reads info on scWing files and sets wing_exists to 1 
# for each filter and frame that has an scWing file, and writes
# the column center of star in original trimmed image to wing_col0.
# For fields that have some wing stars, but not in all filters, 
# wing_exists is set to 2 for filters w/o wing stars. 
proc read_wingpar {w_exist w_col0 w_row0 wingdir run camCol startField endField filterlist} {

   upvar $w_exist wing_exists
   upvar $w_col0 wing_col0
   upvar $w_row0 wing_row0

   # form scWing*.par path, read data and assert that everything is OK
   set runstr [format %06d $run]
   set parfile $wingdir/scWing-$runstr-$camCol.par 

   if [file exist $parfile] {
      calc_crc $parfile
      set winglist [param2Chain $parfile wingpar] 
      assert {[keylget wingpar run] == $run}
      assert {[keylget wingpar camCol] == $camCol}

      # collect info on existing wings
      set cursor [chainCursorNew $winglist]
      while {[set wing [chainWalk $winglist $cursor]] != ""} {
         set field [exprGet $wing.field]
         set filter [exprGet $wing.filter]
         set wing_exists($filter,$field) 1 
         set wing_col0($filter,$field) [exprGet $wing.col]
         set wing_row0($filter,$field) [exprGet $wing.row]
      }

      # clean up
      chainCursorDel $winglist $cursor
      chainDestroy $winglist genericDel
 
      # for all fields which have some, but not all wing stars
      # set wing_exists for the missing filters to 2
      loop field $startField [expr $endField+1] { 
          set wing 0
          # if there is at least 1 wing star set wing to 1
          foreach filter $filterlist { 
	     if {$wing_exists($filter,$field) == 1} {
                 set wing 1
             }
          }
          # if there is at least 1 wing star for this field
          # set wing_exists to 2 for all filters w/o wing stars
          if {$wing} {
              foreach filter $filterlist { 
	         if {$wing_exists($filter,$field) == 0} {                
	             set wing_exists($filter,$field) 2 
                 }
              }
          }
      }
   } else { 
      error "    ** scWing-$runstr-$camCol.par file is not available **"
   }
   
   return 0 
}


# This proc selects the masks for good wing stars. wingstar1clist is the
# list of all wing stars, and winglist is the list of selected ones. 
# allwingmasks are masks corresponding to the complete list wingstar1clist,
# and this proc selects those masks corresponding to winglist from allwingmasks, 
# and puts them in wingmasks.  
proc get_wing_masks {_star1clist _list _allmasks _masks _rej_masks filter ifilter
                 {sg1 0} {sg2 1} {s1 0} {s2 -1}} {

   upvar $_star1clist star1clist
   upvar $_list list
   upvar $_allmasks allmasks
   upvar $_masks masks
   upvar $_rej_masks rej_masks
 
   set Nstars [chainSize $star1clist]
   set Nallmasks [chainSize $allmasks($filter)]
   set Ngoodstars [chainSize $list($filter)]
   if {$Ngoodstars == 0} {
     return 0;
   }
   if {$sg1 >= $sg2} {
     return 0;
   }

   assert {$Nstars == $Nallmasks}
   assert {$Ngoodstars <= $Nstars}
   set cursor_good [chainCursorNew $list($filter)]
   loop i 0 [expr $sg1 + 1] {
       set good [chainWalk $list($filter) $cursor_good]
   }
   set IDgood [exprGet $good.id]
   handleDel $good

   if {$s2 < 0} {
       set s2 [expr [chainSize $star1clist] - 1]
   }

   loop star_no $s1 [expr $s2 + 1] {
       set star1c [chainElementGetByPos $star1clist $star_no]
       set mask [chainElementGetByPos $allmasks($filter) $star_no]
       set IDnumber [exprGet $star1c.star1<$ifilter>->id] 
       if {$IDgood == $IDnumber} {
          chainElementAddByPos $masks($filter) $mask TAIL AFTER   
          if {[chainSize $masks($filter)] < $Ngoodstars} {
             set good [chainWalk $list($filter) $cursor_good]
             set IDgood [exprGet $good.id] 
	     handleDel $good
          }
       } else {
          chainElementAddByPos $rej_masks($filter) $mask TAIL AFTER   
       }
       handleDel $star1c; handleDel $mask
   }

   set Nmasks [chainSize $masks($filter)]

   assert {$Ngoodstars == $Nmasks}

   # clean up
   chainCursorDel $list($filter) $cursor_good

   return 0 

}



# This proc selects the masks for good PSF stars, analogously to get_wing_masks
# except that arrays are 2D. 
proc get_masks {_star1clist _list _allmasks _masks _rej_masks filter ifilter field
                 {sg1 0} {sg2 1} {s1 0} {s2 -1}} {

   upvar $_star1clist star1clist
   upvar $_list list
   upvar $_allmasks allmasks
   upvar $_masks masks
   upvar $_rej_masks rej_masks
 
   set Nstars [chainSize $star1clist($field)]
   set Nallmasks [chainSize $allmasks($field,$filter)]
   set Ngoodstars [chainSize $list($field,$filter)]
   if {$Ngoodstars == 0} {
     return 0;
   }
   if {$sg1 >= $sg2} {
     return 0;
   }

   assert {$Nstars == $Nallmasks}
   assert {$Ngoodstars <= $Nstars}
   set cursor_good [chainCursorNew $list($field,$filter)]
   loop i 0 [expr $sg1 + 1] {
       set good [chainWalk $list($field,$filter) $cursor_good]
   }
   set IDgood [exprGet $good.id]
   handleDel $good

   if {$s2 < 0} {
       set s2 [expr [chainSize $star1clist($field)] - 1]
   }

   loop star_no $s1 [expr $s2 + 1] {
       set star1c [chainElementGetByPos $star1clist($field) $star_no]
       set mask [chainElementGetByPos $allmasks($field,$filter) $star_no]
       set IDnumber [exprGet $star1c.star1<$ifilter>->id] 
       if {$IDgood == $IDnumber} {
          chainElementAddByPos $masks($field,$filter) $mask TAIL AFTER   
          if {[chainSize $masks($field,$filter)] < $Ngoodstars} {
             set good [chainWalk $list($field,$filter) $cursor_good]
             set IDgood [exprGet $good.id] 
	     handleDel $good
          }
       } else {
          chainElementAddByPos $rej_masks($field,$filter) $mask TAIL AFTER   
       }
       handleDel $star1c; handleDel $mask
   }

   set Nmasks [chainSize $masks($field,$filter)]

   assert {$Ngoodstars == $Nmasks}

   # clean up
   chainCursorDel $list($field,$filter) $cursor_good

   return 0 
}



# This proc prints info on flags from an integer flag bitwise
# coded according to STAR1_FLAGS listed below 
#
proc print_flags_info {flag} {
   global STAR1

   set list ""
   # print the list of flags which are set 
   if {$flag == 0} {
      echo "  No flags are set!"
   } else {
      echo "  The following flags are set:"
      foreach val [array names STAR1] {
	 if [expr $flag & $STAR1($val)] {
            echo "   $val"
            lappend list $val
         }
      }
      echo "  ----------------------------"
   }
   
   return $list
}

# This procedure prints flags from star1s in star1c for a given filter

proc print_star1c_flags {star1c ifilter} {
   print_flags_info [exprGet (int)$star1c.star1<$ifilter>->flags]
}

# This proc reads the astroms for each filter and puts them in a tcl array
#
proc astrom_read {startField endField camCol \
		      astroms filterlist file ast_node ast_incl} {
   upvar $astroms ast $ast_node astrom_node 
   upvar $ast_incl astrom_incl
   global ccdpars id_values 

   set Nfield [expr $endField - $startField + 1]

   verb_echo 4 "Reading $file"
   set hdr [hdrReadAsFits [hdrNew] $file]
   calc_crc $file

   # get ASTRO_ID keyword, if present
   if [catch {set id_values(ASTRO_ID) [hdrGetAsAscii $hdr ASTRO_ID]}] {
       set id_values(ASTRO_ID) "not available"
   }  
   set astrom_node [hdrGetAsDbl $hdr NODE]
   set astrom_incl [hdrGetAsDbl $hdr INCL]
   set astrom_field0 [hdrGetAsInt $hdr FIELD0]
   set nfields [hdrGetAsInt $hdr NFIELDS]
   set ast_camcols [hdrGetAsAscii $hdr CAMCOLS]
   set ast_filters [hdrGetAsAscii $hdr FILTERS]
   set ast_nfilters [llength $ast_filters]
   set first_element [expr $startField - $astrom_field0]
   set last_element [expr $endField - $astrom_field0]
   # make sure astrom fields fully overlap photometric fields
   assert {$first_element >= 0}
   assert {[expr $astrom_field0 + $nfields] > $endField}

   foreach filter $filterlist {
      set ifilter [lsearch $ast_filters $filter]
      set icol [lsearch $ast_camcols $camCol]
      set hdu [expr 1 + $icol*$ast_nfilters + $ifilter]
      if {$hdu < 0} {
	 error "$file has no HDU for camCol $camCol filter $filter"
      }
      set ast($filter) [fits2Schema $file TRANS $hdr -hdu $hdu]
      set size [chainSize $ast($filter)]
      assert {$nfields == $size} 
      # remove tailing fields
      set rem_el [expr $last_element + 1]
      if {$rem_el <  $nfields} {
         loop i [expr $last_element + 1] $nfields {
             genericDel [chainElementRemByPos $ast($filter) $rem_el]
         }
      }
      # remove leading fields
      loop i 0 $first_element {
          genericDel [chainElementRemByPos $ast($filter) 0]
      }
      # make sure we got the right hdu
      if [info exists ccdpars] {	# it doesn't when reading output tables
	 assert {[exprGet $ccdpars($filter).iRow] == [hdrGetAsInt $hdr CAMROW]}
      } 
      assert {$camCol == [hdrGetAsInt $hdr CAMCOL]}
      hdrFreeAll $hdr;
   }
   hdrDel $hdr
}

# This procedure reads in quartiles from a binary fits table and returns
#   quartile structure
proc quartiles_read {fangdir run ccdrow ccdcol ccdpars startfield \
			 endfield filter _tshift} {
   upvar $_tshift tshift
   global hardpars 


   set nfield [expr 1+$endfield-$startfield]
   set quartiles_nc [exprGet $ccdpars.ncols]
   set quartiles [quartilesNew $nfield $quartiles_nc]

   set file [get_scfang_filename $fangdir $run $filter $ccdcol $startfield]
   
   set hdu [get_hdu $file $filter quarts]

   loop fld $startfield [expr $endfield+1] {
      set file [get_scfang_filename $fangdir $run $filter $ccdcol $fld]

      set tblcol [handleNewFromType TBLCOL]
      fitsRead $tblcol $file -binary -hdu $hdu
      calc_crc $file

      # Check that row is correct
      set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
      assert {$hfilter == $filter}
      assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "QFLAT"} 

      if [catch {
	 tblcolToQuartiles $tblcol $quartiles [expr $fld-$startfield]} msg] {
	 error "file $file $filter: $msg"
      }
      set tshift [hdrGetAsInt $tblcol.hdr TSHIFT]

      handleDelFromType $tblcol
   }
   return $quartiles
}


### account for quartiles affected by bad columns (including BADBLK, PR 4128) 
### if bad column set (q25,q50,q75) to (cQ0,cQ1,cQ2)
### e.g. for resetting quartiles when u3 is bad
###   before clip_ssc_quartiles: quartiles_doBC $Q $BCchain $ColOffset 0 1 2
###                              reset_quartiles $Q 
###    after clip_ssc_quartiles: quartiles_doBC $Q $BCchain $ColOffset 
proc quartiles_doBC {Q BCchain {ColOffset 40} {cQ0 -9} {cQ1 -9} {cQ2 -9}} {
  
      set nrow [exprGet $Q.nr]
      loop i 0 3 {
	 set fq($i) [handleBindFromHandle [handleNew] *$Q.data<$i>]
	 set cQ($i) [set cQ$i]
      }

      loop k 0 [chainSize $BCchain] {
         set bc [chainElementGetByPos $BCchain $k]

         set startCol [expr $ColOffset + [exprGet $bc.dfcol0]]
         set endCol [expr $ColOffset + [exprGet $bc.dfcol0] + [exprGet $bc.dfncol]]
         # echo "defect $k: $startCol - $endCol"

	 if {[exprGet -enum $bc.dfaction] == "FILCOL" &&
	     [exprGet $bc.dfncol] >= 1024} {# at least half a chip
	    if {$startCol == $ColOffset} {
	       set startCol 0
	    } else {
	       set endCol [expr $endCol + $ColOffset]
	    }
	 }

	 loop i 0 3 {
	    set sreg [subRegNew $fq($i) \
		      $nrow [expr $endCol - $startCol] 0 $startCol]
	    regIntSetVal $sreg $cQ($i)
	    regDel $sreg
	 }
         handleDel $bc
      } 

      loop i 0 3 {
	 handleDel $fq($i)
      }
}


### reset quartiles (q25,q50,q75) to (cQ0,cQ1,cQ2) whenever 
### the median (q50) is <= q50Min
### return the number of changed pixels
proc reset_quartiles {Q {q50Min 0} {cQ0 -9} {cQ1 -9} {cQ2 -9}} {
  
      set Nbad 0
      set nrow [exprGet $Q.nr] 
      set ncol [exprGet $Q.nc]    
      set fq25 [handleBindFromHandle [handleNew] *$Q.data<0>]
      set fq50 [handleBindFromHandle [handleNew] *$Q.data<1>]
      set fq75 [handleBindFromHandle [handleNew] *$Q.data<2>]
        
      loop i 0 $nrow { 
         loop j 0 $ncol {
	    if {[exprGet $fq50.rows_s32<$i><$j>] <= $q50Min} {
                incr Nbad
                handleSet $fq25.rows_s32<$i><$j> $cQ0 
                handleSet $fq50.rows_s32<$i><$j> $cQ1 
                handleSet $fq75.rows_s32<$i><$j> $cQ2 
            }
         }
      } 

      handleDel $fq25; handleDel $fq50; handleDel $fq75

  return $Nbad
      
}


### for all pixels that show spurious noise larger than
### Nsigma*gain_noise set the values of all three quartiles to -9
proc clip_ssc_quartiles {Q {colStart -1} {colEnd -1} {nSplit -1} {Nsigma 5} {tshift 64}} {

global verbose PHOTO_CONSTS

      set MAX_S16 [expr $PHOTO_CONSTS(MAX_S16)*$tshift];

      assert { $Nsigma > 0 }
      set nrow [exprGet $Q.nr]
      set ncol [exprGet $Q.data<0>->ncol]
  
      if {$colStart < 0} {set colStart 0}
      if {$colEnd < 0} {set colEnd $ncol}
      if {$nSplit >= 0} {
         assert { $nSplit <= $colEnd }
         assert { [expr $nSplit - $colStart] >= 0 }
      }

      set fq25  [handleBindFromHandle [handleNew] *$Q.data<0>]
      set fq50  [handleBindFromHandle [handleNew] *$Q.data<1>]
      set fq75  [handleBindFromHandle [handleNew] *$Q.data<2>]
  
      set iqr   [regNew -type FL32 $nrow $ncol]
      set iqr2  [regNew -type FL32 $nrow $ncol]
      set gain2 [regNew -type FL32 $nrow $ncol]
      set noise [regNew -type FL32 $nrow $ncol]
      set med   [regNew -type FL32 $nrow $ncol]

      # IQR range
      regSubtract $fq75 $fq25 -regOut $iqr
      # IQR squared
      regMultiply $iqr $iqr -regOut $iqr2
      # median - bias
      set biasVec "";  # XXX do we want to subtract bias? 
      if {$biasVec != ""} {
          set bias_subtract 0; # XXX
          if {$bias_subtract} {
             # regSubtract $fq50 $biasVec -regOut $med;  ### reg-vec
             # "gain" squared
             # regDivide $iqr2 $med -regOut $gain2 
             error "not implemented"
          } else {
             # "gain" squared
             regDivide $iqr2 $fq50 -regOut $gain2 
          }
      } else {
          # "gain" squared
          set stats [regStatsFind $fq50]
          set fq50Min [keylget stats low]
          if {$fq50Min == 0} {
             error "oops: fq50Min = $fq50Min"
          }
          regDivide $iqr2 $fq50 -regOut $gain2
      }


     ### PR 4637 and related ones: need a running window for gain 
     ### determination because fast sky changes result in large iqr range
     # loop over fields with chunks (2*NfieldHalfWin+1) long
     set NfieldHalfWin 2
  ### A BAD HACK BY ZI
  set NfieldHalfWin 2000
     set Nfield [expr 2*$NfieldHalfWin+1]
     loop ichunk 0 $nrow {     
       # starting row for this chunk
       set rowS [expr $ichunk - $NfieldHalfWin] 
       if {$rowS < 0} {set rowS 0} 
       if {$rowS >= [expr $nrow - 1 - $Nfield]} {
           # this is the last chunk
           set NfieldChunk [expr $nrow - $rowS] 
       } else {
           set NfieldChunk $Nfield
       }
       ## 1 or 2 amps?
       if {$nSplit < 0} {
         ## 1 amp
	 set sub [subRegNew $gain2 $NfieldChunk [expr $colEnd - $colStart] $rowS $colStart]
         # find median value
         set medG2 [regMedianFindAsDbl $sub]
         verb_echo 5 median Gain2 = $medG2
         set Nsub [subRegNew $noise $NfieldChunk [expr $colEnd - $colStart] $rowS $colStart] 
         # now make "the region of merit"
         assert { $medG2 > 0 }
         regMultiplyWithDbl $sub [expr 1.0/$medG2] -regOut $Nsub
         # find median value
         set medN2 [regMedianFindAsDbl $Nsub]
         verb_echo 5 median Noise2 = $medN2
         regDel $sub; regDel $Nsub
      } else {
         ## 2 amps
         # Left side
	 set left [subRegNew $gain2 $NfieldChunk [expr $nSplit - $colStart] $rowS $colStart]
         set medG2L [regMedianFindAsDbl $left]
         verb_echo 5 left median Gain2 = $medG2L         
         set Nleft [subRegNew $noise $NfieldChunk [expr $nSplit - $colStart] $rowS $colStart] 
         # now make "the region of merit"
         assert { $medG2L > 0 }
         regMultiplyWithDbl $left [expr 1.0/$medG2L] -regOut $Nleft
         set medN2L [regMedianFindAsDbl $Nleft]
         verb_echo 5 left median Noise2 = $medN2L
         regDel $left; regDel $Nleft 
         # Right side
         set right [subRegNew $gain2 $NfieldChunk [expr $colEnd-$nSplit+1] $rowS $nSplit]
         set medG2R [regMedianFindAsDbl $right]
         verb_echo 5 right median Gain2 = $medG2R         
         set Nright [subRegNew $noise $NfieldChunk [expr $colEnd-$nSplit+1] $rowS $nSplit]
         # now make "the region of merit"
         assert { $medG2R > 0 }
         regMultiplyWithDbl $right [expr 1.0/$medG2R] -regOut $Nright
         set medN2R [regMedianFindAsDbl $Nright]
         verb_echo 5 right median Noise2 = $medN2R
         regDel $right; regDel $Nright
      }
      # end of loop over field chunks
    }

      set Nsub [subRegNew $noise $nrow [expr $colEnd - $colStart] 0 $colStart] 
      set Nstats [regStatsFind $Nsub -nsigma 3 -niterations 3]
      set sigma [keylget Nstats sigma]; regDel $Nsub

      ### unfix quartiles
      set Nall 0; set Nbad 0
      loop i 0 $nrow {
        loop j $colStart [expr $colEnd + 1] {
          incr Nall
          set N [exprGet $noise.rows_fl32<$i><$j>]
          # $noise is already in units of sigma
          # i.e. Nsigma specifies maximum multiplicative change in gain
	  if {$N > $Nsigma || $N < [expr 1.0/$Nsigma]} {
             loop k 0 3 {
                handleSet $Q.data<$k>->rows_s32<$i><$j> -9   
             }
             incr Nbad
          }
          # test on allowed range for quartiles
          loop k 0 3 {
	      if {[exprGet $Q.data<$k>->rows_s32<$i><$j>] > $MAX_S16} { 
                 # any bad quartile condemns all three 
                 loop kk 0 3 {
                    handleSet $Q.data<$kk>->rows_s32<$i><$j> -9   
                 }
                 incr Nbad
	      }
          } 
        }
      }

     
      # clean
      regDel $iqr; regDel $iqr2; regDel $gain2; regDel $noise; regDel $med
      handleDel $fq25;  handleDel $fq50;  handleDel $fq75; 
     
    return "$Nall $Nbad [expr 1.0*$Nbad/$Nall]"

}




# This procedure reads the postage stamps into an chain of REGIONs.
proc ps_read {fangdir run col field filter} {
   global ccdpars id_values stampsize

   set file [get_scfang_filename $fangdir $run $filter $col $field]
   set hdu [get_hdu $file $filter stamps]
   set tblcol [handleNewFromType TBLCOL]
   fitsRead $tblcol $file -binary -hdu $hdu
   calc_crc $file
					
   # Check that HDU is correct
   set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
   assert {$hfilter == $filter}
   assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "STAMP"} 

   # get SSC_ID keyword, if present
   set hdr [hdrReadAsFits [hdrNew] $file]
   if [catch {set id_values(SSC_ID) [hdrGetAsAscii $hdr SSC_ID]}] {
        set id_values(SSC_ID) "not available"
   }   
   hdrDel $hdr
 
   set pssize [hdrGetAsInt $tblcol.hdr PSSIZE]
   set stamps [tblcolToPsChain $tblcol]
   
   handleDelFromType $tblcol
   #
   # If the data's binned we need to use smaller postage stamps
   #
   set binfac [exprGet $ccdpars($filter).rowBinning]

   if {$binfac != 1} {
      set nstamps [chainNew REGION]
      loop i 0 [chainSize $stamps] {
	 set reg [chainElementRemByPos $stamps 0]

	 if {$i == 0} {
	    set row0 [expr ([exprGet $reg.nrow] - $stampsize)/2]
	    set col0 [expr ([exprGet $reg.ncol] - $stampsize)/2]
	 }
	 set nreg [regNew $stampsize $stampsize]
	 set sreg [subRegNew $reg $stampsize $stampsize $row0 $col0]

	 regIntCopy $nreg $sreg
	 handleSet $nreg.row0 [expr [exprGet $reg.row0] + $row0]
	 handleSet $nreg.col0 [expr [exprGet $reg.col0] + $col0]

	 regDel $sreg; regDel $reg

	 chainElementAddByPos $nstamps $nreg TAIL
	 handleDel $nreg
      }
      chainDel $stamps;			# it's been emptied
      set stamps $nstamps
   }

   return $stamps
}


#
# this proc sets calib1byframe fields from a psField file
# or alternatively from a psPSF file if psPSF != 0
# the difference between the two is that psPSF file does
# not contain PT calibration (flux20 values)
proc get_calib1byframe_from_psField {psField {my_filters all} {psPSF 0}} {
   global allow_old_psField
   global allow_opdb_psField

   set hdr [hdrNew]
   set fd [fitsBinTblOpen $psField "r" -hdr $hdr]
   set psf_filters [hdrGetAsAscii $hdr FILTERS]
   set nfilters [llength $psf_filters]
   hdrDel $hdr

   if {$my_filters == "all"} {
       set my_filters $psf_filters
   } else {
       foreach filter $my_filters {
	  if {[lsearch $psf_filters $filter] < 0} {
             echo "filter $filter doesn't exist in $psf_filters"
          }
       }
   }

   # skip eigenimage stuff
   foreach filter $psf_filters {
      fitsBinTblHdrRead $fd "NULL"  
   }
   #
   # Read the CALIB_IO; if allow_old_psField is true, define a type from
   # the file and read that, then copy it into the CALIB_IO itself
   #
   set calibio [calibIoNew $nfilters]

   if {[info exists allow_old_psField] && $allow_old_psField} {
      #
      # Try to read a _CALIB_IO; if we fail its schema will be defined for us
      # so all we have to do is declare the schematrans and try again
      #
      set fields [declare_io_schematrans_for_psPSF]
      if [catch {
	 set nrow [fitsBinTblHdrRead $fd _CALIB_IO]
      }] {
	 fitsBinForgetSchemaTrans CALIB_IO
	 set fields [declare_io_schematrans_for_psPSF]
	 set nrow [fitsBinTblHdrRead $fd _CALIB_IO]
      }
      
      set _calibio [genericNew _CALIB_IO]
      fitsBinTblRowRead $fd $_calibio 
      fitsBinTblClose $fd

      fitsBinForgetSchemaTrans _CALIB_IO

      #
      # Convert the _CALIB_IO to a CALIB_IO
      #
      foreach el [schemaGetFromType _CALIB_IO] {
	 set _name [lindex $el 0]
	 set type [lindex $el 1]
	 set is_array [regexp {\[[0-9]+\]} $type]
	 set is_2darray [regexp {\[[0-9]+\]\[([0-9]+)\]} $type foo dimen]

	 set name $_name
      
	 if {$name == "status"} {
            # this block to get around the problem with psField files
            # whereby status and psp_status are mixed up
            # 
            if {[info exists allow_opdb_psField] && $allow_opdb_psField} {
	         loop j 0 $nfilters {
		    handleSetFromHandle (int)$calibio.$name<$j> \
		        $_calibio.psp_status<$j>
	         }
	         continue;
            }
	    if $is_array {
	       loop j 0 $nfilters {
		  handleSetFromHandle (int)$calibio.$name<$j> \
		      $_calibio.$_name<$j>
	       }
	    } else {
	       handleSetFromHandle (int)$calibio.$name $_calibio.$_name
	    }
	    continue;
	 } elseif {$name == "psp_status"} {
            if {[info exists allow_opdb_psField] && $allow_opdb_psField} {
                handleSetFromHandle (int)$calibio.$name $_calibio.status
                continue
            }
 	    handleSetFromHandle (int)$calibio.$name $_calibio.$_name
	    continue;
	 } elseif {$name == "psf_err"} {
	    set name "psf_ap_correctionErr"
	 } 

         # this block to get around different names in opdb psField files
         #   photo version    opdb  version
         #   --------------  --------------
         #      skysig           skySig
         #      skyerr           skyErr
         #      skyslope         skySlope
         if {[info exists allow_opdb_psField] && $allow_opdb_psField} {
             if {$name == "skySig"} {  
                 set name "skysig"
             } elseif {$name == "skyErr"} {  
                 set name "skyerr"
             } elseif {$name == "skySlope"} {  
                 set name "skyslope"
             }          
         }

	 #
	 # Deal with case-insensitive IDL psField files
	 #
	 set name [string tolower $name]
	 regsub {err$} $name "Err" name
	 regsub {2g$} $name "2G" name
	 #
	 # To work
	 #
	 if $is_array {
	    if $is_2darray {
	       loop i 0 $nfilters {
		  loop j 0 $dimen {
		     handleSetFromHandle $calibio.$name<$i><$j> $_calibio.$_name<$i><$j>
		  }
	       }
	    } else {
	       loop i 0 $nfilters {
		  handleSetFromHandle $calibio.$name<$i> $_calibio.$_name<$i>
	       }
	    }
	 } else {
	    handleSetFromHandle $calibio.$name $_calibio.$_name
	 }
      }
      #
      # Deal with psf sigmas; needed for pre v5_0_3 psField files
      #
      set sch [schemaGetFromType _CALIB_IO]
      if [keylget sch psf_sigmax1 foo] {
	 loop i 0 $nfilters {
	    loop j 1 3 {
	       foreach xy "x y" {
		  handleSetFromHandle $calibio.psf_sigma$j<$i> \
		      $_calibio.psf_sigmax$j<$i>
	       }
	    }
	 }
      }

      genericDel $_calibio
   } else {
      set n1 [memNextSerialNumber]
      set fields [declare_io_schematrans_for_psPSF]
      memIgnoreBlocks $n1 [memSerialNumber]
      if 1 {
	 set hdr [hdrNew];
	 fitsBinTblHdrRead $fd "" -hdr $hdr
	 hdr2array $hdr arr
	 if {[lsearch [string toupper [array get arr]] [string toupper psf_ap_correctionErr]] < 0} {
	    echo "$psField has no psf_ap_correctionErr field"
	    loop i 0 5 {
	       handleSet $calibio.psf_ap_correctionErr<$i> 0
	    }
   	 }
	 hdrDel $hdr
      }

      set nrow [fitsBinTblHdrRead $fd CALIB_IO]
      fitsBinTblRowRead $fd $calibio 
      fitsBinTblClose $fd
   }
   #
   # Convert the CALIB_IO to a CALIB1BYFRAME
   #
   foreach filter $my_filters {
      set psindex($filter) [lsearch $psf_filters $filter]
   }
   set calib1byframe \
       [calib1byframeNewFromCalibIo $calibio $my_filters psindex]   

   calibIoDel $calibio
   fitsBinForgetSchemaTrans CALIB_IO
   #
   # Only the sigmax components are in the psField file; set the sigmay too
   #
   loop i 0 [llength $my_filters] {
      loop j 1 3 {
	 handleSetFromHandle $calib1byframe.calib<$i>->psf->sigmay$j \
	     $calib1byframe.calib<$i>->psf->sigmax$j
      }
   }

   if {$psPSF} {return $calib1byframe}

   # get flux20 and flux20Err
   if [catch {
      set ch [fits2Schema $psField PT_AUX -hdu [expr $nfilters + 2]]
      assert { [chainSize $ch] == $nfilters }

      loop i 0 $nfilters {     
	 set PTel [chainElementGetByPos $ch $i]  
	 set filter [exprGet $PTel.filter] 
	 set j [lsearch $my_filters $filter]
	 if { $j > -1 } {
	    foreach el {flux20 flux20Err} {
	       handleSet $calib1byframe.calib<$j>->$el [exprGet $PTel.$el]
	    }
	 }
      }
      chainDestroy $ch genericDel
   } msg] {
      echo Failed to read flux20: $msg
   }

   # get wing aperture correction parameters
   if [catch {
      set ch [fits2Schema $psField WC_AUX -hdu [expr $nfilters + 3]]
      assert { [chainSize $ch] == $nfilters }
      loop i 0 $nfilters {     
	 set WCel [chainElementGetByPos $ch $i]  
	 set filter [exprGet $WCel.filter] 
	 set j [lsearch $my_filters $filter]
	 if { $j > -1 } {
	    foreach el {nann_ap_frame nann_ap_run ap_corr_run ap_corr_runErr} {
	       handleSet $calib1byframe.calib<$j>->$el [exprGet $WCel.$el]
	    }
	 }
      }
      chainDestroy $ch genericDel
   } msg] {
      echo Failed to read wing aperture correction parameters: $msg

      loop i 0 [exprGet $calib1byframe.ncolors] {
	 handleSet $calib1byframe.calib<$i>->nann_ap_frame 5
	 handleSet $calib1byframe.calib<$i>->nann_ap_run 8
	 handleSet $calib1byframe.calib<$i>->ap_corr_run   1
	 handleSet $calib1byframe.calib<$i>->ap_corr_runErr 0
      }
   }

   return $calib1byframe
} 




# given psField file name root, e.g. $outputDir/psField-$runstr-$camCol-
# and starting and ending fields, field1 and field2, return the chain
# of calib1byframe 
proc get_cal_chain_from_psField {root field1 field2 {psPSF 0}} {

   global allow_old_psField
   global allow_opdb_psField

    set calibs [chainNew CALIB1BYFRAME]
 
    loop field $field1 [expr $field2 + 1] {
        set fieldstr [format %04d $field]
        set psField ${root}$fieldstr.fit
        if {[file exists $psField]} {
           set cal [get_calib1byframe_from_psField $psField all $psPSF]
           chainElementAddByPos $calibs $cal TAIL AFTER
        } 
    }

    return $calibs

}


# QA processor 
proc doQA {psDiagFile startframe endframe outputDir run col filterlist \
                      QAquantities _w1 _w2 _f1 _f2 _flux20_chi2} {
 
   upvar $_w1 w1
   upvar $_w2 w2
   upvar $_f1 f1
   upvar $_f2 f2
   upvar $_flux20_chi2 flux20_chi2
 
   # get bias scale
   set runstr [format %06d $run]
   set fieldstr [format "%04d" $startframe]  
   set Fieldfile $outputDir/psField-$runstr-$col-$fieldstr.fit
   set hdr [hdrReadAsFits [hdrNew] $Fieldfile]
   set bias_scale [hdrGetAsInt $hdr DSCALE]
   if {$filterlist == "all"} {
      set filterlist [hdrGetAsAscii $hdr FILTERS]
   }
   hdrDel $hdr

   # get chain of calib1byframe from psPSF file
   set psFieldroot $outputDir/psField-$runstr-$col-
   set cframes [get_cal_chain_from_psField $psFieldroot $startframe $endframe]
   set nframes [chainSize $cframes]
  
   set QAflag 0
   set flagout 0
   # For each color check all parameters:
   if {$psDiagFile != ""} {
      puts $psDiagFile " "
      puts $psDiagFile " *** Output from doQA ***"
      puts $psDiagFile "    analyzing output quantities... "
      puts $psDiagFile " "
   }
   set nfilters [llength $filterlist]
   loop i 0 $nfilters {

      # get filtername
      set cal [chainElementGetByPos $cframes 0]
      set filter [exprGet $cal.calib<$i>->filter<0>]
      if {$psDiagFile != ""} {
          puts $psDiagFile " Filter $filter:"
      }

      # sky
      set sky [vFromChain $cframes calib<$i>->sky]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "sky" "sky" $nframes $sky w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $sky

      # bias drift
      set lbias [vFromChain $cframes calib<$i>->lbias]
      vectorExprSet $lbias $lbias/$bias_scale
      set rbias [vFromChain $cframes calib<$i>->rbias]
      vectorExprSet $rbias $rbias/$bias_scale
      set QAflag [QAcheck $psDiagFile $filter $QAflag "lbias" "drift" $nframes $lbias w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      set QAflag [QAcheck $psDiagFile $filter $QAflag "rbias" "drift" $nframes $rbias w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1} 
      vectorExprDel $lbias
      vectorExprDel $rbias

      ### PSF
      ## 2 Gaussians: b_2G, sigma1_2G & sigma2_2G
      set b [vFromChain $cframes calib<$i>->psf->b_2G]
      set sigma1 [vFromChain $cframes calib<$i>->psf->sigma1_2G]
      set sigma2 [vFromChain $cframes calib<$i>->psf->sigma2_2G]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->b_2G" "b" $nframes $b w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->sigma1_2G" "sigma1" $nframes $sigma1 w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->sigma2_2G" "sigma2" $nframes $sigma2 w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $sigma1
      vectorExprDel $sigma2
      vectorExprDel $b


      ## composite profile
      # sigmax1 & sigmax2
      set sigmax1 [vFromChain $cframes calib<$i>->psf->sigmax1]
      set sigmax2 [vFromChain $cframes calib<$i>->psf->sigmax2]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->sigma1" "sigma1" $nframes $sigmax1 w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->sigma2" "sigma2" $nframes $sigmax2 w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $sigmax1
      vectorExprDel $sigmax2

      # psf->b
      set b [vFromChain $cframes calib<$i>->psf->b]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->b" "b" $nframes $b w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $b

      # psf->sigmap
      set sigmap [vFromChain $cframes calib<$i>->psf->sigmap]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->sigmap" "sigmap" $nframes $sigmap w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $sigmap

      # psf->p0
      set p0 [vFromChain $cframes calib<$i>->psf->p0]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->p0" "p0" $nframes $p0 w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $p0
 
      # psf->beta
      set beta [vFromChain $cframes calib<$i>->psf->beta]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "PSF->beta" "beta" $nframes $beta w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $beta


      # patch calibration
      if {[info exists flux20_chi2] && $flux20_chi2($i) >= 0} {  
	 set chi2 [vectorExprNew 1]
	 handleSet $chi2.vec<0> $flux20_chi2($i) 
	 set QAflag [QAcheck $psDiagFile $filter $QAflag "PTchi2" "PTchi2" 1 $chi2 w1 w2 f1 f2]
	 vectorExprDel $chi2 
      }

      # flux20
      set flux20 [vFromChain $cframes calib<$i>->flux20]
      set QAflag [QAcheck $psDiagFile $filter $QAflag "flux20" "flux20" $nframes $flux20 w1 w2 f1 f2]
      if {$QAflag == -1} {set flagout -1}
      vectorExprDel $flux20   
   }      
   if {$psDiagFile != ""} {
       puts $psDiagFile "***********************************"
   }
   chainDestroy $cframes calib1byframeDel

   if {$flagout == -1} {
      return -1
   } else {
      return $QAflag
   } 
}
 
proc QAcheck {psDiagFile filter QAflag textstring quant N vector _w1 _w2 _f1 _f2} {
 
   upvar $_w1 w1
   upvar $_w2 w2
   upvar $_f1 f1
   upvar $_f2 f2
 
   # get flag 
   set qw1 $w1($quant)
   set qw2 $w2($quant)
   set qf1 $f1($quant)
   set qf2 $f2($quant)
   set flag 0
   set aux 0
   set auxf 0
   loop i 0 $N {
      set q [exprGet $vector.vec<$i>]
      if {$q < $qw1 || $q > $qw2} {
         verb_echo 2 "  Warning for frame-frame0 = $i, filter $filter: $textstring = $q"
         set flag 1
         set aux $q
         break
      }
      if {$q < $qf1 || $q > $qf2} {
         verb_echo 1 "Fatal error for frame-frame0 = $i, filter $filter: $textstring = $q"
         set flag -1
         set auxf $q
         break
      }
   }
 
   # analyze flag 
   if {$flag == -1} {
       if {$psDiagFile != ""} {
          puts $psDiagFile "   $textstring is outside given f1-f2 bounds \[$qf1-$qf2\]: auxf"            
       }
       return -1
   } else {
       if {$flag == 0} {
           if {$psDiagFile != ""} {
               puts $psDiagFile "         $textstring is OK (within  \[$qw1-$qw2\])"
           }
           set outflag $QAflag 
       } else {
           set outflag [expr $QAflag + 1]  
           if {$psDiagFile != ""} {
               puts $psDiagFile "      $textstring is outside given w1-w2 bounds \[$qw1-$qw2\]: $aux"             
           }
       }
   }
   return $outflag
}




# This gets the info for the frameinfo structs from the scFang headers,
# and also possibly the SCPARAMBS from the scFang file
proc make_frameinfos {fangdir run filterlist camCol startfield endfield \
			  {_scparambs ""}} {
   global ccdpars psplan
   #
   # Do we want to return the SCPARAMBs as a chain?
   #
   if {$_scparambs == ""} {
      set read_scparambs 0
   } else {
      upvar $_scparambs scparambs

      set xtbl [schemaTransAuto "SCPARAMBS" write]
      set read_scparambs 1
   }

   set frameinfos [chainNew FRAMEINFO]
   set tblcol [handleNewFromType TBLCOL]

   set best_alt 0

   loop field $startfield [expr $endfield + 1] {
      set file [get_scfang_filename $fangdir $run "" $camCol $field]

      if $read_scparambs {
	 set scparambs($field,pdu,hdr) [hdrReadAsFits [hdrNew] $file]
      }

      get_hdu_for_filterset $file params hdu
      if ![info exists nfilters] {
	 if $read_scparambs {
	    foreach lt "l t" {		# we need the headers for the
	       ;			# astrometrics too, for the psFang file
	       if {[lsearch [array names hdu] $lt] > 0} {
		  lappend filterlist $lt
	       }
	    }
	 }
	 set nfilters [llength $filterlist]
      }

      set fi [frameinfoNew]
      handleSet $fi.field $field
 
      set tai 0.0
      set alt 0.0
      set aux 0.0
      foreach filter $filterlist {
       if [catch {
	 fitsRead $tblcol $file -binary -hdu $hdu($filter)
	 calc_crc $file
	 set row [hdrGetAsInt $tblcol.hdr CAMROW]
	 assert {$row == [keylget psplan ccdrow_$filter]}
	 if [catch {
            set aux [hdrGetAsDbl $tblcol.hdr TAI] 
	    set tai [expr $tai + $aux]
	 } msg] {
            if [catch {
                set aux [hdrGetAsDbl $tblcol.hdr TAI].0
                set tai [expr $tai + $aux]
	    } msg] {            
 	       echo $file $filter: $msg; set tai 0.0
            }
	 }
	 if [catch {
	    set alt [expr $alt + [hdrGetAsDbl $tblcol.hdr ALT]]
	 } msg] {
	    echo $file $filter: $msg
            set alt [expr $alt + $best_alt]  
	 } else {
	    set best_alt [hdrGetAsDbl $tblcol.hdr ALT] 
         }
	 #
	 # Create chain of SCPARAMBS; these'll be written to the psFang file
	 # (once we've fixed up their astrometry using the KL PSF). We'll
	 # also need the header, so keep that too
	 #
	 # Note that tblToSchema is rumoured to run out of memory if used
	 # on too large a file; if this happens here fix dervish.
	 #
	 if $read_scparambs {
	    set scparambs($field,$filter) [chainNew "SCPARAMBS"]
	    tblToSchema $tblcol $scparambs($field,$filter) \
		$xtbl -schemaName "SCPARAMBS"

	    set scparambs($field,$filter,hdr) [hdrNew]
	    hdrCopy $tblcol.hdr $scparambs($field,$filter,hdr)
	 }
       } msg] {
           echo "WARNING: problem reading hdu($filter) for file $file ($msg)"
       }      
      }

      handleSet $fi.mjd [expr ($tai / $nfilters) / 86400.0]

      set alt [expr ($alt / $nfilters) / [radToDeg 1.0]]
      if {$alt <= 0} {
	 handleSet $fi.airmass 1e10
      } else {
	 handleSet $fi.airmass [expr 1/[sin $alt]]
      }
      chainElementAddByPos $frameinfos $fi TAIL AFTER
      handleDel $fi
   }
   
   handleDelFromType $tblcol
   if [info exists xtbl] {
      schemaTransDel $xtbl
   }
 
   return $frameinfos   
}   

proc extinction_read { photomDir photomfile filterlist } {
   global id_values Npatches

   if ![regexp {^/} $photomfile] {
      set photomfile "$photomDir/$photomfile"
   }

   if {![file exists $photomfile]} {
      echo "Extinction file $photomfile doesn't exist"
      set photomfile "/dev/null"
   }
   
   if {$photomfile == "/dev/null"} {
      # no extinction file, set some defaults and return
      set extinctions [chainNew EXTINCTION]
      set nfilters [llength $filterlist]
      set ext [extinctionNew $nfilters]
      loop i 0 $nfilters {
	 set filter [lindex $filterlist $i]
	 handleSet $ext.k<$i>->mag 0.0
	 handleSet $ext.k<$i>->magErr 0.0
	 handleSet $ext.k<$i>->passBand<0> $filter
      }      
      chainElementAddByPos $extinctions $ext
      return $extinctions
   }

   # Read table
   set hdr [hdrNew] 
   hdrReadAsFits $hdr $photomfile
   # get the EXCAL_ID keyword, if present
   if [catch {set id_values(EXCAL_ID) [hdrGetAsAscii $hdr EXCAL_ID]}] {
        set id_values(EXCAL_ID) "not available from exPhotom file"
   }
   set photoms [fits2Schema $photomfile PHOTOM $hdr]
   calc_crc $photomfile
   
   set tfilterlist [hdrGetAsAscii $hdr FILTERS]

   # First make some lookup tables so we can get the filters right.
   foreach filter $filterlist {
      set tindex($filter) [lsearch $tfilterlist $filter]
      set index($filter) [lsearch $filterlist $filter]
   }
		       
   set extinctions [chainNew EXTINCTION]
   set nfilters [llength $filterlist]
   set ntfilters [llength $tfilterlist]
   set nextinct [chainSize $photoms] 
   loop i 0 $nextinct {
      set ext [extinctionNew $nfilters]
      set phot [chainElementGetByPos $photoms $i]
      handleSet $ext.mjd [exprGet $phot.midmjd]
      foreach filter $filterlist {
	 handleSet $ext.k<$index($filter)>->mag \
	     [exprGet $phot.k<$tindex($filter)>]
	 handleSet $ext.k<$index($filter)>->magErr  \
	     [exprGet $phot.kerr<$tindex($filter)>]
	 handleSet $ext.k<$index($filter)>->passBand<0> $filter
      }
      handleDel $phot
      chainElementAddByPos $extinctions $ext TAIL AFTER
      handleDel $ext
   }
   chainDestroy $photoms genericDel
   hdrDel $hdr

   return $extinctions

}



### test CMDs for all patches from a psPlan file
proc testMTpatches {psPlanFile {device /VPS}} {

     process_plan psplan [read_planfile $psPlanFile]     
     foreach var "mtPatches mtPatchFormat" {
        set val [keylget psplan $var]
        set $var $val
     }
       
     set mtPatchList [get_mtPatchList "" $mtPatchFormat $mtPatches]

     foreach patch $mtPatchList {
	 if {![file exist $patch]} {
              error "patch $patch does NOT exist"
         } else {
             set fileName [file tail $patch]
             set s1 [file tail [file dirname [file dirname $patch]]]
             set s2 [file tail [file dirname $patch]]
             set title "$s1/$s2/$fileName"
             testMTpatch $patch $title $device
             if {$device != "/XWINDOW"} {
                 set ipatch [lsearch $mtPatchList $patch]
                 exec cp testMTpatch.ps ${psPlanFile}_MTpatchQA_$ipatch.ps
             }
         }
     }

   return 

}


## test a given MT patch file
proc testMTpatch {file {title ""} {device /XWINDOW}} {

     set hdr [hdrNew]
     hdrReadAsFits $hdr $file
     set mjd [hdrGetAsDbl $hdr MJD]
     # PT field center position (J2000, degrees)
     set fieldRA [hdrGetAsDbl $hdr RA]
     set fieldDec [hdrGetAsDbl $hdr DEC]
     hdrFree $hdr

     echo "reading patch at ($fieldRA, $fieldDec) obtained on MJD=$mjd"

     set mtstds [fits2Schema $file MTSTD $hdr]

     plot_MTpatchChain $mtstds $title $device 

}


# given a chain of MTSTDs read from a patch file, do various QA plots
proc plot_MTpatchChain {mtstds {title ""} {device /XWINDOW}} {


   # first extract vectors
   set reclist "ra dec"
   loop i 0 5 {
      set reclist "$reclist mag<$i> magerr<$i>" 
   }
   set veclist [vectorsFromChain $mtstds $reclist]
   set filterlist "u g r i z"
   set ra [lindex $veclist 0];  set dec [lindex $veclist 1]
   vNameSet $ra ""; vNameSet $dec "";
   loop i 0 5 {
      set f [lindex $filterlist $i]
      set m($f) [lindex $veclist [expr 2+$i*2]]
      set mErr($f) [lindex $veclist [expr 3+$i*2]]
      vNameSet $m($f) ""; vNameSet $mErr($f) ""; 
   }
   set ug [vectorExprEval $m(u)-$m(g)]
   set gr [vectorExprEval $m(g)-$m(r)]
   set ri [vectorExprEval $m(r)-$m(i)]
   set iz [vectorExprEval $m(i)-$m(z)]
   vNameSet $ug ""; vNameSet $gr ""; vNameSet $ri ""; vNameSet $iz ""; 
   set veclist "$veclist $ug $gr $ri $iz"


   set ug_min  0.0
   set ug_max  4.0
   set gr_min -1.0
   set gr_max  3.0
   set ri_min -1.0
   set ri_max  2.0
   set iz_min  -1.0
   set iz_max  2.0
   set mmin  10.0
   set mmax  20.0
   set mEmin  -0.02
   set mEmax  0.2

   set RAmin [vExtreme $ra min]
   set RAmax [vExtreme $ra max]
   set Decmin [vExtreme $dec min]
   set Decmax [vExtreme $dec max]

   # Set up the plot
   if [catch {
      set pg [pgstateNew]
      set device [devicename $device testMTpatch.ps]      
      pgstateSet $pg -device $device \
	  -xfract 0.45 -yfract 0.35 -nxwindow 2 -nywindow 6 -symb 1           
      pgstateOpen $pg      
      
      loop i 0 5 {
         set f [lindex $filterlist $i]
	 vPlot $pg $m($f) $mErr($f) -xmin $mmin -xmax $mmax -ymin $mEmin -ymax $mEmax      
	 xlabel "m($f)"; ylabel "mErr($f)"
         if {$i == 0} {
            ## RA-Dec plot
	    vPlot $pg $ra $dec -xmin $RAmax -xmax $RAmin -ymin $Decmin -ymax $Decmax      
   	    xlabel "RA"; ylabel "Dec"
         }
         if {$i == 1} {
            ## g vs. g-r CMD
	    vPlot $pg $gr $m(g) -xmin $gr_min -xmax $gr_max  -ymin $mmax -ymax $mmin      
   	    xlabel "g-r"; ylabel "g"
         }
         if {$i == 2} {
            ## g-r vs. u-g CCD
	    vPlot $pg $ug $gr -xmin $ug_min -xmax $ug_max -ymin $gr_min -ymax $gr_max      
   	    xlabel "u-g"; ylabel "g-r"
         }
         if {$i == 3} {
            ## r-i vs. g-r CCD
	    vPlot $pg $gr $ri -xmin $gr_min -xmax $gr_max -ymin $ri_min -ymax $ri_max      
   	    xlabel "g-r"; ylabel "r-i"
         }
         if {$i == 4} {
            ## i-z vs. r-i CCD
	    vPlot $pg $ri $iz -xmin $ri_min -xmax $ri_max -ymin $iz_min -ymax $iz_max      
   	    xlabel "r-i"; ylabel "i-z"
         }
	 if {$i == 4} {
	     pgText -8.5 -3.5 "$title"
	     pgText -8.5 -4.5 "There are [chainSize $mtstds] entries in this file"
	 }
      }

      endplot $pg
   } msg] {
      echo $msg
      catch {pgstateClose $pg}
      catch {pgstateDel $pg}
   }

   vListDel $veclist

}



# read patch with stars for photometric calibration
# add all fields overlapping this patch to PTfields
proc mtpatch_read {PatchPath fieldRADec PTfields {node 0} {incl 0}} {

   # Read table
   set file $PatchPath
   if {![file exists $file]} {
      echo " Error: cannot find file $file"
      return ""
   }

   set PTfields ""

   set hdr [hdrNew]
   hdrReadAsFits $hdr $file
   set mjd [hdrGetAsDbl $hdr MJD]
   # PT field center position (J2000, degrees)
   set fieldRA [hdrGetAsDbl $hdr RA]
   set fieldDec [hdrGetAsDbl $hdr DEC]

   hdrFree $hdr
   set mtstds [fits2Schema $file MTSTD $hdr]
   calc_crc $file

   # Copy MTSTDs into STARCs
   # We assert that the MTSTDS have entries for all 5 colors
   # even if not all have good values, since mtpipe doesn't know
   # what color terms we use. We don't need to check the filter order
   # here because we use a lookup table in findPhotoParams

   set filterlist [hdrGetAsAscii $hdr FILTERS]
   set ncolors [llength $filterlist]
   assert { $ncolors == 5 }
   foreach filter $filterlist {
      set index($filter) [lsearch $filterlist $filter]
   }

   ### determine this PT patch - this run overlap
   set good_fields [searchfieldRADec $fieldRADec $fieldRA $fieldDec $node $incl] 
   set PTfields "$PTfields $good_fields"

   set patch [chainNew STARC]
   set cursor [chainCursorNew $mtstds]
   set Nstars 0
   while {[set std [chainWalk $mtstds $cursor]] != ""} {
      set starc [starcNew 5]
      handleSet $starc.mjd $mjd
      incr Nstars
      handleSet $starc.ra [exprGet $std.ra]
      handleSet $starc.dec [exprGet $std.dec]	
      loop i 0 $ncolors {
	 set filter [lindex $filterlist $i]
	 handleSet $starc.mag<$i>->passBand<0> $filter
	 handleSet $starc.mag<$i>->mag [exprGet $std.mag<$i>]
	 handleSet $starc.mag<$i>->magErr [exprGet $std.magerr<$i>]
      }
      genericDel $std
      chainElementAddByPos $patch $starc TAIL AFTER
      handleDel $starc
   }
   chainCursorDel $mtstds $cursor
   chainDel $mtstds
   hdrDel $hdr

   return "$patch [list $PTfields]"

}

# given a list of {field RA Dec} and a position {ra,dec} return
# field for which the distance between the 2 positions is less than 
# 0.4832 deg (sqrt(2)*41'/2, where 41' is the PT field size). 
# If not found return -1. 
proc searchfieldRADec {fieldRADec ra dec {node 0} {incl 0}} {

    set magic_distance 0.4832
    # add buffer
    set magic_distance [expr $magic_distance * 1.2]

    set dold 360.0
    set good_fields {}
    set found_some 0
    set dmin 360.0
    set munu [eqToGC $ra $dec -node $node -inclination $incl]
    set muC [keylget munu mu]
    set nuC [keylget munu nu]

    loop i 0 [llength $fieldRADec] {        
	set el [lindex $fieldRADec $i]
        set RA [lindex $el 1]
        set Dec [lindex $el 2]  
        set munu [eqToGC $RA $Dec -node $node -inclination $incl]
        set mu [keylget munu mu]
        set nu [keylget munu nu]
        set delmu [expr $mu-$muC]
        set delnu [expr $nu-$nuC] 
        set d [expr sqrt($delmu*$delmu + $delnu*$delnu)]
        # echo "field $i, d = $d"
	if {$d < $dmin} {set dmin $d}
        # if within field add it to the list
        if {$d < $magic_distance} {
           lappend good_fields [lindex $el 0]
           set found_some 1
        } else {
           # if the distance is increasing we are done
           if {$d > $dold && $found_some} {return $good_fields}
           # we can increment i assuming the shortest distance
           # (0.1512 is 1361 pixels * 0.4"/pix)
           incr i [expr int($d / 0.1512)]    
        }
        set dold $d     
    }

    # echo DMIN = $dmin
    return $good_fields

}

# given a chain of TRANS return a list of {field RA Dec} where
# (RA,Dec) is the center position of each field. 
proc get_fieldRADec {astroms node incl startField {GC 0} {row ""} {col ""}} {

    set fieldRADec {}
    if {$row == ""} {set row [expr 1361/2]}
    if {$col == ""} {set col 1024}

    loop i 0 [chainSize $astroms] {
	set trans [chainElementGetByPos $astroms $i]
        # first get mu/nu
        set gc [transApply $trans r $row 0 $col 0]
        set mu [keylget gc mu]
        set nu [keylget gc nu]
        set field [expr $startField + $i] 
        if {!$GC} {
           # mu/nu -> RA/Dec
           set eq [GCToEq $mu $nu -node $node -inclination $incl]
           set RA [keylget eq ra]
           set Dec [keylget eq dec]
           set this_field "$field $RA $Dec"
        } else {
           set this_field "$field $mu $nu"
        }
        # add to list
        lappend fieldRADec $this_field  
    }

    return $fieldRADec
}

######################################################################
# Read old patch calibration from psCT file
proc ctime_read {fitsfile filterlist} {
   global id_values

    # get ncolors from the oldcal primary header
    set hdr [hdrNew]
    if [catch {hdrReadAsFits $hdr $fitsfile} msg] {
       hdrDel $hdr
       error $msg
    }
    calc_crc $fitsfile
    set oldfilters [hdrGetAsAscii $hdr FILTERS]
    set ncolors [llength $oldfilters]
    hdrFreeAll $hdr; hdrDel $hdr;

    # make sure we get all the filters we want
    foreach filter $filterlist {
       set pos [lsearch $oldfilters $filter ] 
       if { $pos < 0 || $pos >= $ncolors} {
	  error "ctime_read: filter $filter not found in oldcal"
       }
       set index($filter) $pos
    }

    # We want the last (most recent) oldcal from table
    declare_io_schematrans_for_calib1bytime
    set calibio [calibIoNew $ncolors]

    if {[catch {set fd [fitsBinTblOpen $fitsfile r]} msg]} {
       echo $msg
    } elseif {[catch {
       set nrow [fitsBinTblHdrRead $fd CALIB_IO]
       fitsBinTblRowSeek $fd [expr $nrow-1]  0
       fitsBinTblRowRead $fd $calibio} msg]} {
       
       catch {
	  fitsBinTblClose $fd
       }
       calibIoDel $calibio
       fitsBinForgetSchemaTrans CALIB_IO
       
       error "Failed to read $fitsfile for old calib:\n$msg"
    } else {
       calc_crc $fitsfile
       fitsBinTblClose $fd
    }
    fitsBinForgetSchemaTrans CALIB_IO
    
    set oldcal [calib1bytimeNewFromCalibIo $calibio $filterlist index]
    calibIoDel $calibio
    return $oldcal
}

######################################################################
# Read old calibration from opCamera file
proc opCam_read {opCamera_dir opCamera_file column filterlist} {
   global id_values
   global camera_rows

    # fix this XXX
    set camera_rows {r i u z g}

    # read chains from opCamera file
    if ![regexp {^/} $opCamera_file] {
       set opCamera_file $opCamera_dir/$opCamera_file
    }
    if ![file exists $opCamera_file] {
       error "No such file: $opCamera_file"
    }

    set camera_ch_list [param2Chain $opCamera_file camera_hdr]
    # get new calib1bytime structure
    set oldcal [calib1bytimeNew $filterlist]
    handleSet $oldcal.mjd 0	    
    # extract zero points
    foreach filter $filterlist {
       set ifilter [lsearch $filterlist $filter]
       # camera row for this filter
       set camera_row [expr 1 + [lsearch $camera_rows $filter]]
       # get CCDGEOMETRY structure for this chip   
       if [catch {set ccd_el [get_opCamera_ccd $camera_row $column $camera_ch_list]} msg] {
          error " failed to read photometric zero point from opCamera chain: $msg"
       } 
       set xmag [exprGet $ccd_el.xmag]
       handleDel $ccd_el
       # old opCamera files have "wrong" sign (correct values should be about -27.0)
       if {$xmag > 20} {set xmag [expr -1.0 * $xmag]}
       # n.b. it would make more sense to define xmag as -xmag:
       set f20 [pow 10.0 [expr -0.4 * ($xmag + 20.0)]]
       set f20Err [expr $f20 * 0.1]
       handleSet $oldcal.calib<$ifilter>->flux20 $f20
       handleSet $oldcal.calib<$ifilter>->flux20Err $f20Err
    }

    # cleanup
    foreach ch $camera_ch_list {
       chainDestroy $ch genericDel
    }

    return $oldcal

}
 


# recycled from SSC (ZI)
##########################################################################
# Given a (row, col) position of one CCD in the imaging camera, and a 
# chain list (two chains) derived from the information in the 
# "opCamera" parameters file,
# return a handle to a CCDGEOMETRY structure with information 
# about the CCD at (row, col).
#
# If we can't find an element for the given CCD chip,
# we assert and halt execution.
#
proc get_opCamera_ccd { row col chain_list } {
  set found 0
  foreach ch $chain_list {
    set type [chainTypeGet $ch]
    if { [keylget type type] == "CCDGEOMETRY" } {
      set found 1
      break
    }
  }
  if { $found == 0 } {
    echo "get_opCamera_ccd: can't find chain with type CCDGEOMETRY"
    assert { 0 }
  }

  # Brute force method -- find dewar with 
  #      camRow = $row && camCol = $col
  #
  set size [chainSize $ch]
  loop i 0 $size { 
    set el [chainElementGetByPos $ch $i]
    if { [exprGet $el.camRow] == $row && [exprGet $el.camCol] == $col } {
      return $el
    }
    handleDel $el
  }

  # if we got HERE, then we've failed to find the right dewar.  Print an
  # error message and assert
  error "get_opCamera_ccd: failed to find info in chains for $row,$col "
}


proc write_ps_results {cframes {ctimes ""} {keep_psPSF_files 0} } {

   global run camCol startRun startField endField filterlist outputDir
   global crc crc_nbyte_max

   set runstr [format %06d $run]

   if {$ctimes != ""} {
      # Write out the list of CALIB1BYTIMEs into one big fits table
      verb_echo 1 "   Writing out list of CALIB1BYTIMEs"
      
      set outfile $outputDir/psCT-$runstr-$camCol.fit 
      
      # Make primary header
      set hdr [hdrNew]
      hdrInsWithInt $hdr RUN $run "Imaging Run Number"
      hdrInsWithInt $hdr CAMCOL $camCol "Column in imaging camera"
      hdrInsWithAscii $hdr FILTERS $filterlist "Filters processed"
      insert_id_values -psp $hdr 
      
      declare_io_schematrans_for_calib1bytime
      set fd [fitsBinTblOpen $outfile w -hdr $hdr]
      fitsBinTblHdrWrite $fd "CALIB_IO"
      set cursor [chainCursorNew $ctimes]
      while {[set calib1bytime [chainWalk $ctimes $cursor NEXT]] \
		 != ""} {
	 set calibio [calibIoNewFromCalib1bytime $calib1bytime]
	 fitsBinTblRowWrite $fd $calibio
	 calibIoDel $calibio
	 handleDel $calib1bytime
      }
      chainCursorDel $ctimes $cursor
      fitsBinTblClose $fd
      fitsBinForgetSchemaTrans CALIB_IO;
      hdrDel $hdr
      # Now write a table of CRCs to the end of the psCT file. This will only
      # be useful if it doesn't change the psCT's CRC, i.e. the file is already
      # longer than $crc_nbyte_max
      if {0} {				# also written to psField file
	 calc_crc $outfile
	 if [info exists crc_nbyte_max] {
	    if {$crc_nbyte_max >= 0} {
	       file stat $outfile stat_array
	       if {$crc_nbyte_max == 0 || $stat_array(size) < $crc_nbyte_max} { 
		  echo "Warning: writing CRC table will change CRC for $outfile"
	       }
	    }
	 }
	 crc2Fits $outfile "a"   
      }
   }

   # adding CALIB_IO to psPSF file and renaming to psField
   verb_echo 1 "   Writing out psField files"
   declare_io_schematrans_for_psField
   set i1 [expr $startField - $startRun]
   set i2 [expr $endField - $startField + 1 + $i1]
   assert { $i1 >= 0 && $i1 <= $i2 }
   assert { $i2 <= [chainSize $cframes] }
   loop i $i1 $i2 {
       set calib1byframe [chainElementGetByPos $cframes $i]
       set field [exprGet $calib1byframe.field]
       assert { $field == [expr $startRun + $i] }
       set fieldstr [format "%04d" $field]  
       set PSFfile $outputDir/psPSF-$runstr-$camCol-$fieldstr.fit
       set Ffile \
	   [get_psField_filename $outputDir $run $camCol $field]
       if {[write_psField $PSFfile $calib1byframe $Ffile $keep_psPSF_files] < 0} {
           echo "ERROR: problems with appending PT calibration to $PSFfile"
       } else {
           # crc2Fits $Ffile "a"  
       }
       handleDel $calib1byframe
   }
   fitsBinForgetSchemaTrans CALIB_IO;

   # adding wing correction to psField file
   verb_echo 1 "   Appending wing correction to psField files"
   declare_io_schematrans_for_psFieldWC
   loop i $i1 $i2 {
       set calib1byframe [chainElementGetByPos $cframes $i]
       set field [exprGet $calib1byframe.field]
       assert { $field == [expr $startRun + $i] }
       set fieldstr [format "%04d" $field]  
       set psField $outputDir/psField-$runstr-$camCol-$fieldstr.fit
       if {[appendWC2psField $psField $calib1byframe] < 0} {
           echo "ERROR: problems with appending wing correction to $psField"
       }
       handleDel $calib1byframe
      
      # add crc
      if {[info exists crc_nbyte_max] && $crc_nbyte_max >= 0} {
	 file stat $psField stat_array
	 if {$crc_nbyte_max == 0 || $stat_array(size) < $crc_nbyte_max} {
	    verb_echo 2 "Warning: writing CRC table would change CRC for $psField; only CRCing first $stat_array(size)"
	 }
	 calc_crc $psField $stat_array(size)
      }
      crc2Fits $psField "a"
   }   
}

proc write_psPSF {filter field basis calib1byframe b_scale node incl} {

   global PSP_STATUS
   global run camCol filterlist outputDir startField endField 
   global psfKLbasis_nframe psfKLcoeffs_nframe
   set runstr [format %06d $run]
   set fieldstr [format "%04d" $field]  
   set file $outputDir/psPSF-$runstr-$camCol-$fieldstr.fit

   if {$filter == [lindex $filterlist 0]} {
       catch { unlink $file }
       # stuff header and open file
       set hdr [hdrNew]
       hdrInsWithInt $hdr RUN $run "Imaging Run Number"
       hdrInsWithInt $hdr CAMCOL $camCol "Column in imaging camera"
       hdrInsWithInt $hdr FIELD $field "This field"
       hdrInsWithInt $hdr FIELD0 $startField "First field reduced"
       hdrInsWithInt $hdr NFIELDS [expr $endField-$startField+1] \
                                  "Number of fields reduced"
       hdrInsWithAscii $hdr FILTERS $filterlist "Filters present in file"
       hdrInsWithInt $hdr DSCALE $b_scale \
                          "Scaling for bias drift (lbias and rbias)"
       hdrInsWithDbl $hdr INCL $incl \
                          "Great circle inclination wrt cel. eq. (degrees)"
       hdrInsWithDbl $hdr NODE $node \
                          "RA of great circle's asending node. (degrees)"
       insert_id_values -psp $hdr 
       set fd [fitsBinTblOpen $file w -hdr $hdr]
       hdrDel $hdr
   } else {
       set fd [fitsBinTblOpen $file "a"]
   }

   set hdr [hdrNew]
   hdrInsWithInt $hdr "N_BASIS" $psfKLbasis_nframe \
		                  "num. of fields for basis stars"
   hdrInsWithInt $hdr "N_COEFF" $psfKLcoeffs_nframe \
		                  "num. of fields for coeff stars"
   fitsBinTblHdrWrite $fd "PSF_KL_COMP" -hdr $hdr
   hdrDel $hdr

   set klc [psfKLCompNew]
   # for each eigen image one row 
   loop i 0 [expr [exprGet $basis.kern->nsigma]+1] {
      psfKLCompSetFromBasis $basis $i -klc $klc 
      fitsBinTblRowWrite $fd $klc
   }
   
   handleSet $klc.reg 0
   psfKLCompDel $klc 
   fitsBinTblClose $fd; unset fd

   # if this is the last filter write out the PSF properties
   if {$filter == [lindex $filterlist [expr [llength $filterlist]-1]]} {
       declare_io_schematrans_for_psPSF
       fill_psp_status $calib1byframe
       set calibio [calibIoNewFromCalib1byframe $calib1byframe]
       # PR 4109
       loop j 0 [exprGet $calibio.ncolor] {
	  if {[exprGet $calibio.psf_psfCounts<$j>] < 0} {
             handleSet $calibio.psf_psfCounts<$j> -9999
          }
	  if {[exprGet $calibio.psfCounts<$j>] < 0} {
             handleSet $calibio.psfCounts<$j> -9999
          }
       }
       set fd [fitsBinTblOpen $file "a"]
       set hdr [hdrNew]
       fitsBinTblHdrWrite $fd "CALIB_IO" -hdr $hdr
       fitsBinTblRowWrite $fd $calibio
       calibIoDel $calibio
       fitsBinTblClose $fd
       hdrDel $hdr
   }

 
   return $file

}

# given a calib1byframe, assign the highest value of 
# status(filter) to its psp_status 
proc fill_psp_status {cframe} {
   global PSP_STATUS

   set status $PSP_STATUS(UNKNOWN)
   loop j 0 [exprGet $cframe.ncolors] { 
      set status_j [expr [exprGet (int)$cframe.calib<$j>->status] & \
			$PSP_STATUS(STATUS_MASK)]
      if {$status_j > $status} {
	 set status $status_j
      }
   }

   if {$status >
       [exprGet (int)$cframe.psp_status] & $PSP_STATUS(STATUS_MASK)} {
      handleSet $cframe.psp_status $status
   }
}

# append PT calibration to psPSF file and rename to psField file
# if encountered problems return -1
proc write_psField {PSFfile calib1byframe Ffile {keep_psPSF_files 0}} {

   # check that this psPSF file exists 
   if {[check_file $PSFfile "psPSF file"]} {
       return -1
   }

   # if keeping psPSF files save a copy to a temporary file
   if {$keep_psPSF_files} {
       catch {unlink $PSFfile.PSPtemp}
       set in [open $PSFfile]
       set out [open $PSFfile.PSPtemp w]
       copyfile $in $out
       close $in; close $out
   }

   # make a chain of PT_AUX, one for each band
   set ch [chainNew PT_AUX]
   loop i 0 [exprGet $calib1byframe.ncolors] {
       set PTel [genericNew PT_AUX]    
       handleSet $PTel.filter [exprGet $calib1byframe.calib<$i>->filter]
       handleSet $PTel.flux20 [exprGet $calib1byframe.calib<$i>->flux20]
       handleSet $PTel.flux20Err [exprGet $calib1byframe.calib<$i>->flux20Err]
       chainElementAddByPos $ch $PTel TAIL AFTER
   }

   # append this chain to psPSF file
   schema2Fits $ch $PSFfile -append

   # clean 
   chainDestroy $ch genericDel

   # rename PSF file to Field file
   catch {unlink $Ffile}
   link_or_copy $PSFfile $Ffile 
   unlink $PSFfile

   if {$keep_psPSF_files} {
       catch {unlink $PSFfile}
       set in [open $PSFfile.PSPtemp]
       set out [open $PSFfile w]
       copyfile $in $out
       close $in; close $out    
       catch {unlink $PSFfile.PSPtemp}     
   }

   return 0
}



# calculate bias and error in aperture correction
proc doAperCorr {filterlist cframes nann_ap_frame nann_ap_run} {

     set Nframes [chainSize $cframes]
     # coefficient for error calculation
     set C [expr 1.0/4/sqrt(12)]; # = 0.072, a 0.1 mag spike -> err=0.014 
     set mac ""

     # loop over filters 
     loop j 0 [llength $filterlist] { 
        set filter [lindex $filterlist $j]
        set Nann_frame [lindex $nann_ap_frame $j]
        set Nann_run [lindex $nann_ap_run $j]
        assert {$Nann_frame <= $Nann_run}
        # get overall bias in aperture correction (~0)  
        set apCorrBias [vFromChain $cframes calib<$j>->psf_ap_correctionBias]
        # get median bias for all fields
        set medianapCorrBias [vMedian $apCorrBias]
        set mac "$mac $medianapCorrBias"
        vectorExprDel $apCorrBias
        # stuff calib for each field 
        loop i 0 $Nframes {
            set cal [chainElementGetByPos $cframes $i]
            handleSet $cal.calib<$j>->nann_ap_frame $Nann_frame 
            handleSet $cal.calib<$j>->nann_ap_run $Nann_run
            ### first determine the bias contribution to the error in apCorr
            set thisf [exprGet $cal.calib<$j>->psf_ap_correctionBias]	
            if {$i > 0} { 
                set calL [chainElementGetByPos $cframes [expr $i-1]]
                set fL [exprGet $calL.calib<$j>->psf_ap_correctionBias]	
                set gradL [expr abs($thisf - $fL)]
            } else {
                set gradL 0.0
            }
            if {$i < [expr $Nframes -1]} { 
                set calR [chainElementGetByPos $cframes [expr $i+1]]
                set fR [exprGet $calR.calib<$j>->psf_ap_correctionBias]	
                set gradR [expr abs($fR - $thisf)]
            } else {
                set gradR 0.0
            }
            # error term in aperture correction due to variable bias 
            set errBias [expr $C * ($gradL + $gradR)]
            # rms width of m(apert)-m(psf), i.e. scatter in aperture correction 
            # introduced by imperfect PSF KL expansion
            set apCorrWidth [exprGet $cal.calib<$j>->psf_ap_correctionErr]	
            # add both terms in quadrature
            set errTotal [expr sqrt($apCorrWidth*$apCorrWidth+$errBias*$errBias)]
            # and reset the value in output structure
            handleSet $cal.calib<$j>->ap_corr_runErr $errTotal
            ### and then correct the bias in aperture correction by median bias
            # median bias in aperture correction introduced by imperfect KL PSF
            set apCorrBias [exprGet $cal.calib<$j>->psf_ap_correctionBias]	
            if {0} {
               # correct it for the median value (which comes out in calibration)
               set apCorrBiasCorr [expr $apCorrBias - $medianapCorrBias]
            } else {
               # do not correct for the median bias
               set apCorrBiasCorr $apCorrBias
            }
            # and reset the value in output structure
            handleSet $cal.calib<$j>->ap_corr_run $apCorrBiasCorr
	    #
	    # We might want to use a calculated aperture correction, based on the seeing
	    #
	    if [getsoftpar use_calculated_ap_corr_run] {
	       global ccdpars apCorrCoeffs

	       set ccd [ccdName [exprGet $ccdpars($filter).iRow]  [exprGet $ccdpars($filter).iCol]]
	       set fwhm [expr 1.0*[exprGet $cal.calib<$j>->psf->width]]

	       set calculated_ap_corr_run [estimate_apCorr apCorrCoeffs $ccd $fwhm]

	       verb_echo 2 [format "Changing aperture correction %g to %g" \
				[exprGet $cal.calib<$j>->ap_corr_run] $calculated_ap_corr_run]
	       handleSet $cal.calib<$j>->ap_corr_run $calculated_ap_corr_run
	    }
	}
     }

     return $mac   
}



# append wing aperture correction to psField file
# if encountered problems return -1
proc appendWC2psField {psField calib1byframe} {

   # check that this psField file exists 
   if {[check_file $psField "psField file"]} {
       return -1
   }

   # make a chain of WC_AUX, one for each band
   set ch [chainNew WC_AUX]
   loop i 0 [exprGet $calib1byframe.ncolors] {
       set WCel [genericNew WC_AUX]    
       handleSet $WCel.filter [exprGet $calib1byframe.calib<$i>->filter]
       handleSet $WCel.nann_ap_frame [exprGet $calib1byframe.calib<$i>->nann_ap_frame]
       handleSet $WCel.nann_ap_run [exprGet $calib1byframe.calib<$i>->nann_ap_run]
       handleSet $WCel.ap_corr_run [exprGet $calib1byframe.calib<$i>->ap_corr_run]
       handleSet $WCel.ap_corr_runErr [exprGet $calib1byframe.calib<$i>->ap_corr_runErr]
       chainElementAddByPos $ch $WCel TAIL AFTER
   }

   # append this chain to psPSF file
   schema2Fits $ch $psField -append

   # clean 
   chainDestroy $ch genericDel

   return 0
}

# calculate wing aperture correction
proc stuffWC {filterlist cframes _frameinfolist nann_ap_frame nann_ap_run ignore_WC fracBad} {

global verbose
upvar $_frameinfolist frameinfolist 

 
     # do we want to fit WC vs. seeing or simply interpolate
     set fitWC 0

     # we are ignoring wing stars
     if {$ignore_WC} {
	set Nframes [chainSize $cframes]

         loop j 0 [llength $filterlist] { 
            loop i 0 $Nframes {
               set cal [chainElementGetByPos $cframes $i]
               handleSet $cal.calib<$j>->psf_ap_correctionErr 1.0
            }
	 }
         return 0
     }

     # get radii 
     initProfileExtract
     # fake central pixel
     set radii "0.0 [profileRadii]"      
     set Nframes [chainSize $cframes]
     # loop over filters 
     loop j 0 [llength $filterlist] { 
        set Nbad 0
        set Ngood 0
        set filter [lindex $filterlist $j]
        set Nann_frame [lindex $nann_ap_frame $j]
        set Nann_run [lindex $nann_ap_run $j]
        assert {$Nann_frame <= $Nann_run}  
        # seeing is already there
        set seeing [vFromChain $cframes calib<$j>->psf->width]
        # aperture correction we need to calculate
	set goodSeeingList {}
	set goodWCList {}
	set allWCList {}
        set goodFieldFlags {}
        loop i 0 $Nframes {
           set cal [chainElementGetByPos $cframes $i]
           set Nprof [exprGet $cal.calib<$j>->prof->nprof] 
	   if {$Nprof >= $Nann_run} {
               incr Ngood
               if {0} {
                  # the median profile
                  set comp_prof $cal.calib<$j>->prof->med
               } else {
                  # the mean profile
                  set comp_prof $cal.calib<$j>->prof->mean
               }
               # this calculates the "raw" wing correction for this frame
               set wc [get_wc_correction $Nann_frame $Nann_run $radii $comp_prof]    
               lappend goodWCList $wc    
               lappend goodSeeingList [exprGet $seeing.vec<$i>]  
               lappend goodFieldFlags 1          
               lappend allWCList $wc
           } else {
               # not enough points in the composite profiles
               incr Nbad
               lappend goodFieldFlags 0          
               lappend allWCList -999
               verb_echo 1 "WARNING: for filter $filter, frame $i (0-index): Nprof=$Nprof (<$Nann_run)!"
           }
        }
        # check that the number of fields with short comp. prof is smallish
	if { $Nbad > [expr $fracBad*$Nframes] } {
            echo  " the maximum fraction of bad comp.prof is $fracBad"
            echo  " to ignore this problem set ignore_comp_prof_test to 1 (psPlan)"
            echo  " note that this action may severely impact the photometric accuracy"
            if {$Nbad == $Nframes} {
               echo  " however, here Nbad=Nframes and not even this trick would help"
            }
            error "stuffWC, filter $filter: Nbad=$Nbad (Nframes=$Nframes)" 
	}
        assert { $Nframes == [expr $Ngood + $Nbad] }
	assert { $Nframes == [llength $goodFieldFlags] }
	assert { $Nframes == [llength $allWCList] }
	assert { $Ngood == [llength $goodSeeingList] }
        assert { $Ngood == [llength $goodWCList] }

        # make Ngood-dimensional vectors from good* lists
        set goodwc [vectorExprNew $Ngood]
        set goodseeing [vectorExprNew $Ngood]
	loop i 0 $Ngood {
	    handleSet $goodwc.vec<$i> [lindex $goodWCList $i]
 	    handleSet $goodseeing.vec<$i> [lindex $goodSeeingList $i]
        }        
        # now we have two vectors, goodseeing and goodwc, and we fit a function
        # to goodwc(goodseeing) and return a new vector which contains the
        # best-fit value for each element in seeing
        set wc_fit [fit_wing_correction $seeing $goodseeing $goodwc]

        # now stuff calibs 
        loop i 0 $Nframes {
            set cal [chainElementGetByPos $cframes $i]
            assert {[exprGet $cal.calib<$j>->nann_ap_frame] == $Nann_frame }
            assert {[exprGet $cal.calib<$j>->nann_ap_run] == $Nann_run }
            # n.b. ap_corr_run is known around here as wing correction 
            # we either adopt the best-fit values (fitWC=true), or we use
            # them only to replace the bad wc values, as signaled by goodFieldFlags
	    if {$fitWC} {
                set wc_adopted [exprGet $wc_fit.vec<$i>] 
            } else {
                # is this field OK
                set fieldFlag [lindex $goodFieldFlags $i]
	        if {$fieldFlag} {
                    # good field
                    set wc_adopted [lindex $allWCList $i]
                } else {
                    # bad field, take best-fit value
                    set wc_adopted [exprGet $wc_fit.vec<$i>] 
                }
            }
            assert {$wc_adopted >= 1}; #  XXX

            # if we know that PSF for this field is bad, do not insist on good correction (PR 5633)
            if {$wc_adopted < 1} {
               set status [exprGet -enum $cal.calib<$j>->status]   
               if {$status != "PSP_FIELD_OK" && $status != "PSP_FIELD_PSF11" && $status != "PSP_FIELD_PSF22"} {
                  set wc_adopted 1.0
                  verb_echo 1 "WARNING stuffWC: for frame0=$i, band $j taking WC=1 (status=$status) !!!"
               } 
            }
 
            assert {$wc_adopted >= 1} 

            # go from flux ratio to magnitudes
            set wing_corr [expr 2.5*log10($wc_adopted)]
	   if {$wing_corr < 0} {
	      echo "Resetting wing_corr from $wing_corr to 1"
	      set wing_corr 1
	   }
            assert {$wing_corr >= 0}; #  XXX

            # add this contribution to bias (ap_corr_run)
            # the sign:
            # 1) bias is defined as <maper - mpsf>
            # 2) maper7 = maper5 - wing_corr  (maper7 is brighter, and wing_corr>0) 
            # nb: 5->Nann_frame, 7->Nann_run
            set bias5 [exprGet $cal.calib<$j>->ap_corr_run]
            set bias7 [expr $bias5 - $wing_corr]
            # echo frame $i: bias5=$bias5 bias7=$bias7 (wc=$wing_corr)
            handleSet $cal.calib<$j>->ap_corr_run $bias7
            # store wing_corr for later QA
            set frame_info [chainElementGetByPos $frameinfolist($filter) $i]
            handleSet $frame_info.wingcorr $wing_corr
        }   
        vListDel [list $seeing $goodseeing $goodwc $wc_fit]
     }   

     return 0       
}

# calculate wing aperture correction
proc doWingCorr {filterlist cframes} {

     set Nframes [chainSize $cframes]
     # loop over filters 
     loop j 0 [llength $filterlist] { 
        ## first we need to patch bad fields, and smooth a bit
	set wingcorrVec [vFromChain $cframes calib<$j>->psf_wingcorr]        
        # check that wingcor is more or less OK
        set medWC [vMedian $wingcorrVec]
        assert { $medWC >= 1 }
        if {[exprGet $wingcorrVec.vec<0>] < 1}  {
            handleSet $wingcorrVec.vec<0> $medWC
	}
        # replace all bad values by the preceeding value
        loop i 1 $Nframes {
	   if {[exprGet $wingcorrVec.vec<$i>] < 1}  {
              handleSet $wingcorrVec.vec<$i> [exprGet $wingcorrVec.vec<[expr $i-1]>] 
	   }              
        }
        # median smooth every 3 fields (get rid of 1 point transients)
        if {$Nframes > 3} {
           vMedianSmooth $wingcorrVec $Nframes 3
        }
        # loop over fields
        loop i 0 $Nframes {
            set cal [chainElementGetByPos $cframes $i]
            set wc [exprGet $wingcorrVec.vec<$i>]
            assert { $wc >= 1 }              
            set wing_corr [expr 2.5*log10($wc)]
            assert {$wing_corr >= 0} 
            # store wing_corr as psf_ap_correctionErr for later QA
            handleSet $cal.calib<$j>->psf_ap_correctionErr $wing_corr
            # add this contribution to bias (ap_corr_run)
            # the sign:
            # 1) bias is defined as <maper - mpsf>
            # 2) maper7 = maper5 - wing_corr  (maper7 is brighter, and wing_corr>0) 
            # nb: 5->Nann_frame, 7->Nann_run
            set bias5 [exprGet $cal.calib<$j>->ap_corr_run]
            set bias7 [expr $bias5 - $wing_corr]
            handleSet $cal.calib<$j>->ap_corr_run $bias7
        }   
        vectorExprDel $wingcorrVec  
     } 

   return 0       
}


# given a profile and the list of radii, calculate the ratio of fluxes enclosed
# by radius(i2) and radius(i1), respectively
proc get_wc_correction {i1 i2 radii profile} { 

    assert {$i1 <= $i2} 
    assert {$i2 < [llength $radii]}

    if [catch {
          set counts 0; set flux1 0; set flux2 0
          loop k 0 $i2 {
             set r1 [lindex $radii $k]
             set r2 [lindex $radii [expr $k + 1]]
             # dA (up to a constant) 
             set area [expr pow($r2,2)-pow($r1,2)]   
             # counts in dA (here we need a correction for the fact that
             # we are using median profile ZZZ
             set counts [expr $counts + $area * [exprGet $profile<$k>]] 
	     if {$k == [expr $i1-1]} {set flux1 $counts}   
	     if {$k == [expr $i2-1]} {set flux2 $counts} 
          } 
        } msg] {                
        # this ought to be better ZZZ
        error "cannot calculate wing correction: $msg" 
    }   

    # echo $i1 $i2 $flux1 $flux2
    assert {[expr $flux1*$flux2] > 0} 

    return [format "%6.3f" [expr $flux2 / $flux1]] 
}


### this is where we calculate a fit to the wing correction vs. seeing
# given N-element vectors of goodseeing and goodwc (wing correction), 
# return a vector of fitted values to the values of seeing
proc fit_wing_correction {seeing goodseeing goodwc} {

global verbose
   # we want a second order parabola
   set Norder 2
   
   set fake_fitting 0
   
   set Ngood [exprGet $goodseeing.dimen]
   assert { $Ngood == [exprGet $goodwc.dimen] }

   if {$Ngood < $Norder || $fake_fitting} {
      verb_echo 1 "WARNING: taking defaults in fit_wing_correction!!!"
      # best fit coefficients based on run 1345 (for 2.5*log(f2/f1))
      # n.b. in the loop below the correction is transformed into 
      # a multiplicative correction
      set Clist [list 0.015 -0.02 0.015]
   } else {
      set Clist [get_lsq_polynom $goodseeing $goodwc $Norder]
   }
   
   set wc_fit [vEvalPolynom $seeing $Clist]
   
   if {$fake_fitting} {			# this should be temporary XXX
      vectorExprEval wc_fit 10^(0.4*$wc_fit)
   } 
   
   return $wc_fit
}



# given inverted flatfield vector, ff, renormalize it such that 
# sum(1/ff) = 1 (actually it's 1xN region)
proc renormalizeFF {ff FSHIFT} {

      set stats [regStatsFind $ff]
      if {[keylget stats low] > 0} {
         # scratch FL32 region
         set FF32 [regNew -type FL32 1 [exprGet $ff.ncol]]
         regClear $FF32; regSetWithDbl $FF32 $FSHIFT
         regDivByRow $FF32 $ff -regOut $FF32
         set stats [regStatsFind $FF32]
         set sum [keylget stats total] 
         regMultiplyWithDbl $ff [expr $sum/[exprGet $ff.ncol]] 
         regDel $FF32
      } else {
         echo "The low value in provided ff is [keylget stats low]!?"
      }

    return 0

}  



####################################################################
# bad ZI causing conflicts...

# depending on run, return the correct seasonal flatfield
proc getFFseasonRunNo {run} {

 # see sdss-calib 800    
 #------------------------------
 #    Season   run range
 #------------------------------
 #      1     94 to 1140 
 #      2   1231 to 1478 
 #      3   1659 to 1755 
 #      4   1869 to 2078 
 #      5   2125 to 2143 
 #      6   2188 to 2961 (or larger)
 #------------------------------
    if {$run <= 1140} {
        set runFlatfield 999991 
    } elseif {$run <= 1478} {
        set runFlatfield 999992   
    } elseif {$run <= 1755} {
        set runFlatfield 999993   
    } elseif {$run <= 2078} {
        set runFlatfield 999994   
    } elseif {$run <= 2143} {
        set runFlatfield 999995    
    } else {
        set runFlatfield 999996  
    } 

  return $runFlatfield 

}  


# get run range for a given season 
proc getFFseasonRunRange {season} {
      
    set rR(10) "  94 1140"      
    set rR(20) "1231 1478"       
    set rR(30) "1659 1755"      
    set rR(40) "1869 2078"      
    set rR(50) "2125 2143"       
    set rR(60) "2188 2283"
    set rR(70) "2298 2507"
    set rR(80) "2566 3187"
    set rR(90) "3225 999999"

    set allowedSeasons "10 20 30 40 50 60 70 80 90"

    ## allowedSeasons must be known to getFFseasonNumberFromRun
    if {$season == "getAllowedSeasons"} {return $allowedSeasons} 

    if {[lsearch $allowedSeasons $season] < 0} {
      error "getFFseasonRunRange: season $season is not defined!?!"
    }

  return $rR($season)

}


# given a run number, return its season
proc getFFseasonNumberFromRun {run} {
    
    set allowedSeasons [getFFseasonRunRange "getAllowedSeasons"]

    foreach s $allowedSeasons {
       set runRange [getFFseasonRunRange $s]
       if {$run >= [lindex $runRange 0] && $run <= [lindex $runRange 1]} {
           return $s
       }
    }
   error " getRunFromFFseason: didn't find season for run $run !?!"
}

####################################################################
# take the jrow-th row of a Nrow x Ncol FL32 region, and replace all <=0 
# values by the immediately preceeding value (if the first value is <=0 set 
# it to C
proc interpolateFF {reg jrow {C 1.0}} {

    set ncol [exprGet $reg.ncol]  
    set Nbad 0
 
    if {[exprGet $reg.rows_fl32<$jrow><0>] <= 0} {
        handleSet $reg.rows_fl32<$jrow><0> $C
    }

    loop i 1 $ncol { 
        set val [exprGet $reg.rows_fl32<$jrow><$i>]
	if {$val <= 0} {
            incr Nbad
            handleSet $reg.rows_fl32<$jrow><$i> $C
        }
        set C [exprGet $reg.rows_fl32<$jrow><$i>] 
    }

 return $Nbad

}




# correct bias vector for odd/even bias drift, taking care of odd/even effect, 
proc do_bias_drift {bias nSplit ifilter calib} {

     set lbiasOEdiff [exprGet $calib.calib<$ifilter>->lbiasOEdiff]
     set rbiasOEdiff [exprGet $calib.calib<$ifilter>->rbiasOEdiff]

     loop j 0 [exprGet $bias.ncol] {
         set aux [exprGet $bias.rows_u16<0><$j>]
         # left or right
         if {$j < $nSplit} {
             set OEdiff $lbiasOEdiff
         } else {
             set OEdiff $rbiasOEdiff
         }
         # even or odd
	 if {[expr $j/2*2 == $j]} {
            # even
	    assert { $aux >= [expr 0.5*$OEdiff] }
            handleSet $bias.rows_u16<0><$j> [expr $aux - 0.5*$OEdiff] 
         } else {
            # odd
            handleSet $bias.rows_u16<0><$j> [expr $aux + 0.5*$OEdiff] 
         }
     }


}    

# fix all bad drift values
proc smooth_bias_drift {calib1byframelist ifilter maxDrift} {
   global PHOTO_CONSTS

      # first get median values
      foreach q "lbias rbias lbiasOEdiff rbiasOEdiff" {
         set vec [vFromChain $calib1byframelist calib<$ifilter>->$q]
         set med($q) [vMedian $vec]
	 #
	 # If there are _no_ valid values (e.g. bad u3) don't give up;
	 # we're not going to be using this data anyway.  PR 4156
	 #
	 if {$med($q) == $PHOTO_CONSTS(MAX_S16)} {
	    echo "*** For ifilter=$ifilter median($q)=$med($q); setting to $maxDrift"
	    set med($q) $maxDrift
	 }
	 if {$med($q) < -$maxDrift || $med($q) > $maxDrift} {
            error "*** For ifilter=$ifilter median($q)=$med($q) (maxDrift=$maxDrift)"
	 }
         vectorExprDel $vec
      }
      loop i 0 [chainSize $calib1byframelist] {
         set cframe [chainElementGetByPos $calib1byframelist $i]
	 set calib [handleBindFromHandle [handleNew] *$cframe.calib<$ifilter>]
         foreach q "lbias rbias lbiasOEdiff rbiasOEdiff" { 
            set val [exprGet $calib.$q]
	    if {$val < -$maxDrift || $val > $maxDrift} { 
               if {$i == 0} {
                  handleSet $calib.$q $med($q) 
               } else {
                  set cframePrev [chainElementGetByPos $calib1byframelist [expr $i-1]]
                  set calibPrev [handleBindFromHandle [handleNew] *$cframePrev.calib<$ifilter>]
                  handleSet $calib.$q [exprGet $calibPrev.$q]
                  handleDel $calibPrev
               }
            }
            set val [exprGet $calib.$q]
            assert { $val >= -$maxDrift } 
            assert { $val <= $maxDrift }
         }
         handleDel $calib
      }

}

# wrapper for verify_bias to do all runs of a given rerun
proc verify_bias_all {rerun {rootdir ""} {skipRuns ""}} {

global data_root 

     if {$rootdir == ""} {
        if {![info exists data_root]} {sdr}
        set rootdir $data_root
     }

     set all [glob $rootdir/\[0-9\]*/$rerun/objcs]
     loop i 0 [llength $all] {
	 set run [file tail [file dirname [file dirname [lindex $all $i]]]]
         if {[lsearch $skipRuns $run] < 0} {
            set Q [verify_bias $run $rerun $rootdir]
            if {$Q < 0} {
               echo "Run $run is BAD"
            } else {
               echo "Run $run is OK"
            }
	 }
     }  


}


# verify that psField files do not contain any bad bias drift values
proc verify_bias {run rerun {rootdir ""} {camCols all} {maxDrift 10000}} {

global data_root openit verbose

   if {![info exist verbose]} {set verbose 0}

   if {$rootdir == ""} {
       if {![info exists data_root]} {sdr}
       set rootdir $data_root
   }
   if {$camCols == "all"} {set camCols "1 2 3 4 5 6"}
  
   set Nbad 0

   foreach camCol $camCols {
      set Nbadold $Nbad
      # first get calib1byframe chain
      set_run -root $rootdir -rerun $rerun $run $camCol
      set dir $openit(objdir)
      set fileList [glob $dir/psField*.fit]
      if {[llength $fileList] == 0} {
          echo "No psField files in $dir"
          continue
      }

      # find field range
      set field0 10000
      set fieldN -1
      foreach file $fileList {
         set N [string length $file]
         set field [string range $file [expr $N - 8] [expr $N - 5]]
         while {[string range $field 0 0] == 0} {
            set field [string range $field 1 [expr [string length $field] - 1]]
         }
         if {$field < $field0} {
	    set field0 $field
         } elseif {$field > $fieldN} {
	    set fieldN $field
	 }
      }

      set runstr  [format "%06d" $run]
      set psFieldroot $dir/psField-$runstr-$camCol-
      set calib1byframelist [get_cal_chain_from_psField $psFieldroot $field0 $fieldN]

      # test all values in this chain
      loop i 0 [chainSize $calib1byframelist] {
         set cframe [chainElementGetByPos $calib1byframelist $i]
	 loop ifilter 0 [exprGet $cframe.ncolors] {
	     set calib [handleBindFromHandle [handleNew] *$cframe.calib<$ifilter>]
             foreach q "lbias rbias lbiasOEdiff rbiasOEdiff" { 
                set val [exprGet $calib.$q]
	        if {$val < -$maxDrift || $val > $maxDrift} { 
                   incr Nbad
		   if {$verbose > 2} {
                     echo "$q=$val, Nbad = $Nbad (camCol=$camCol, f=$ifilter, frame=$i)"
                   }
                }
             }  
             handleDel $calib
         }
      }
      chainDestroy $calib1byframelist calib1byframeDel
      set Nthiscol [expr $Nbad-$Nbadold]
      if {$Nthiscol > 0} {echo "For column $camCol: $Nthiscol"}
   }

   if {$Nbad > 0} {
      return -1
   } else {
      return 1
   } 

}





# correct bias vector for gain ratio variation
proc correct_gain {bias nSplit gainRatio bias_scale sky} {

global verbose
     set warning 0

     # first compute additive correction to bias
     set corr [expr $sky*($gainRatio-1.0)/($gainRatio+1.0)]
     # the correction should not be larger than 1 count
     if {$corr > 1} { 
        verb_echo 1 "WARNING correct_gain: corr=$corr (>1 !?!?)"
        verb_echo 1 "        sky=$sky, gainRatio=$gainRatio"
        incr warning
     }  
     # take into accout the bias scaling factor   
     set corr [expr $bias_scale*$corr]

     loop j 0 [exprGet $bias.ncol] {
        set orig_bias [exprGet $bias.rows_u16<0><$j>] 
        # left or right?
        if {$j < $nSplit} {
            handleSet $bias.rows_u16<0><$j> [expr $orig_bias - $corr]
        } else {
            handleSet $bias.rows_u16<0><$j> [expr $orig_bias + $corr]
        }
     }

   return $warning 
}    



proc testSLregion {SLtestfile SLfile \
               {maxAbsMean 0.001} {maxAbsMed 0.01} {maxSigma 0.01} {test 0}} {

     # first read regions
     set regT [regReadAsFits [regNew] $SLtestfile]
     set ncolT [exprGet $regT.ncol]
     set nrowT [exprGet $regT.nrow]
     set reg [regReadAsFits [regNew] $SLfile]
     set ncol [exprGet $reg.ncol]
     set nrow [exprGet $reg.nrow]
     if {$ncolT > $ncol} {
        return "can't compare apples to pears"
     }

     # make median vectors
     set Tmed [vectorGetFromRegion $regT 0 [expr $nrowT-1] 0 [expr $ncolT-1] 0] 
     set med [vectorGetFromRegion $reg 0 [expr $nrow-1] 0 [expr $ncol-1] 0] 
     regDel $reg; regDel $regT
     # smooth odd/even effect
     smoothOE $Tmed
     smoothOE $med

     # and make their difference
     set medDiff [vectorExprEval $Tmed-$med]

     # statistics of the difference
     set mean   [expr abs([vMean $medDiff])]
     set median [expr abs([vMedian $medDiff])]
     set sigma  [vSigma $medDiff]

     if {$test} {
         return "T=$Tmed x=$med diff=$medDiff mean=$mean median=$median sigma=$sigma" 
     } else {
         vListDel "$Tmed $med $medDiff"
         set QAflag 0
         if {$mean > $maxAbsMean || $median > $maxAbsMed || $sigma > $maxSigma} {
            set QAflag 1
         }
         return $QAflag
     }

} 



# given a psSLprof file with the scattered light region, find median and
# dump <x y> data to outfile, where y = 1+<SLprof> 
proc SLregion2dat {SLregion outfile} {

     # first read region
     set reg [regReadAsFits [regNew -type FL32] $SLregion]
     set ncol [exprGet $reg.ncol]
     set nrow [exprGet $reg.nrow]
     # get median vector
     set med [vectorGetFromRegion $reg 0 [expr $nrow-1] 0 [expr $ncol-1] 0] 

     # get output vectors
     set x [vIndex $ncol]
     set y [vectorExprEval 1.0+$med]
     # and dump to file
     vectorsWriteToFile [list $x $y] $outfile

     # clean
     regDel $reg
     vListDel [list $x $y $med]      

} 



# replace each vector element by the mean of its value and that of the prev. el.
proc smoothOE {vec} {
     
    set old [exprGet $vec.vec<0>]
    loop i 1 [exprGet $vec.dimen] {
       set this [exprGet $vec.vec<$i>]
       handleSet $vec.vec<$i> [expr 0.5*($this+$old)]
       set old $this
    }

}



# test for odd/even transients, and if found stuff PSP_STATUS
# if odd/even bias drift difference jumps by > maxOEdiffChange
# between the two adjacent frames, then this frame must have an
# odd/even transient (unless the previous frame had it) and its 
# PSP_STATUS is set to OE_TRANSIENT (in this color _only_) 
proc testOEtransients {cframe cframeOld bias_scale ifilter maxOEdiffChange} {

   global PSP_STATUS

      set transient 0

      # left amplifier          
      set biasOEdiffOld [exprGet $cframeOld.calib<$ifilter>->lbiasOEdiff]
      set biasOEdiff [exprGet $cframe.calib<$ifilter>->lbiasOEdiff]
      set jumpOEdiff [expr 1.0*abs($biasOEdiff-$biasOEdiffOld)/$bias_scale]
      if {$jumpOEdiff > $maxOEdiffChange} {
          set oldStatus [exprGet (int)$cframeOld.calib<$ifilter>->status]
          set OEstatus $PSP_STATUS(OE_TRANSIENT)
          if {$oldStatus != $OEstatus} {
              handleSet $cframe.calib<$ifilter>->status $PSP_STATUS(OE_TRANSIENT) 
              set transient 1
	  }
      }
      # right amplifier  
      set biasOEdiffOld [exprGet $cframeOld.calib<$ifilter>->rbiasOEdiff]
      set biasOEdiff [exprGet $cframe.calib<$ifilter>->rbiasOEdiff]
      set jumpOEdiff [expr abs($biasOEdiff-$biasOEdiffOld)/$bias_scale]
      if {$jumpOEdiff > $maxOEdiffChange} {
          set oldStatus [exprGet (int)$cframeOld.calib<$ifilter>->status]
          set OEstatus $PSP_STATUS(OE_TRANSIENT)
          if {$oldStatus != $OEstatus} {
              handleSet $cframe.calib<$ifilter>->status $PSP_STATUS(OE_TRANSIENT) 
              set transient 1
	  }
      }
        
  return $transient
}



# given 2 vectors, find a best-fit N-th order polynom
# and return a list coefficients {C0 C1 ... CN} such that
# y = sum_0^N C(i)*x^i 
proc get_lsq_polynom {x y N} {
   return [vFitPolynom $x $y $N]
}

# given a region, for each row fit a Npoly-th order polynom
# to cols from (colSplit-Lbuffer) to (colSplit-1) and another
# one to cols from colSplit to (colSplit+Rbuffer-1). Evaluate 
# both polynoms at col=colSplit and return the vector of ratios
# polyR(colSplit)/polyL(colSplit)
proc get_jump {reg colSplit Lbuffer Rbuffer Npoly} {

     set Nrow [exprGet $reg.nrow]
     set R [vectorExprNew $Nrow]

     loop i 0 $Nrow {
         # left part
         set cS [expr int($colSplit - $Lbuffer)]
         set cE [expr int($colSplit - 1)]
         set v [vectorGetFromRegion $reg $i $i $cS $cE 0]
         vMedianSmooth $v [exprGet $v.dimen] 3
         set c [vIndex $Lbuffer]
         set Clist [get_lsq_polynom $c $v $Npoly]
         set polyL [yEvalPolynom [expr $Lbuffer - 1] $Clist]
         vListDel [list $v $c]
         # right part
         set cS [expr $colSplit]
         set cE [expr $colSplit + $Rbuffer - 1]
         set v [vectorGetFromRegion $reg $i $i $cS $cE 0]
         set c [vIndex $Rbuffer]
         set Clist [get_lsq_polynom $c $v $Npoly]
         set polyR [yEvalPolynom 0 $Clist]
         vListDel [list $v $c]
         # and not get the ratio
         if {$polyL == 0} {
             handleSet $R.vec<$i> 0.0
         } else {
             handleSet $R.vec<$i> [expr $polyR/$polyL]
         } 
     }

   return $R

}





proc patchesDestroy {patches} {
   
   set cursor [chainCursorNew $patches]
   while {[set patch [chainWalk $patches $cursor]] != ""} {
      chainDestroy $patch starcDel
   }
   chainCursorDel $patches $cursor
   chainDel $patches
}


# This procedure looks for the color-index filter name corresponding to 
#   each element of the filterlist.  That is, if filterlist is
#
#       u g r i z
# 
#   and we want to use color
#
#          (u-g) to correct u  -> thus, g is other filter
#          (g-r) to correct g  -> thus, r is other filter
#          (g-r) to correct r  -> thus, g is other filter
#          (r-i) to correct i  -> thus, r is other filter
#          (i-z) to correct z  -> thus, i is other filter
#
#   then we want to create ref_filterlist that looks like this:
#
#       g r g r i
#
#   We read values from the param file, each of which has the form
# 
#   fpp_refcolor_u   g                
#
# We also need the color term (and its error) for the calibration: 
# 
# log(flux20) = a + color_term * color_index
# 
# These numbers, one per color, should be determined using a larger 
# number of stars than expected for a single patch.

proc get_color_index_and_slope { filterlist ref_filterlist color_term \
				     color_term_err } {
   
   global softpars
   upvar $ref_filterlist ref_list $color_term slope $color_term_err error


   set ref_list ""
   set slope ""
   set err ""
   foreach f $filterlist {
      set rf [getsoftpar fpp_refcolor_${f}]
      set ct [getsoftpar fpp_color_term_${f}]
      set err [getsoftpar fpp_color_term_err_${f}]
    lappend ref_list $rf
    lappend slope $ct
    lappend error $err
  }  

}


proc get_nann_ap {filterlist nann_ap_frame nann_ap_run} {
   
   global softpars
   upvar $nann_ap_frame N_frame $nann_ap_run N_run 

   set N_frame ""; set N_run ""
   foreach f $filterlist {
      if {![getsoftpar nann_ap_frame_${f} Nf] || $Nf == "default"} {
	 if {$Nf != "default"} {
	    verb_echo 5 \
	       "I cannot find nann_ap_frame_${f}; trying nann_ap_frame_default"
	 }
	 if ![getsoftpar nann_ap_frame_default Nf] {
	    getsoftpar nann_ap_frame_${f};# generate error message
	 }
      }

      if {![getsoftpar nann_ap_run_${f} Nr] || $Nr == "default"} {
	 if {$Nr != "default"} {
	    verb_echo 5 \
		"I cannot find nann_ap_run_${f}; trying nann_ap_run_default"
	 }
	 if ![getsoftpar nann_ap_run_default Nr] {
	    getsoftpar nann_ap_run_${f};# generate error message
	 }
      }
      lappend N_frame $Nf
      lappend N_run $Nr
   }  

}


#
# make a mosaic of all symmetry testing images in SYMDIAGs from a chain
# of STAR1s, optionally returning a list of ids of the objects and their
# places in the mosaic
#
proc sym_mosaic { stars {_ids ""}} {
if {$_ids != ""} {
      upvar $_ids ids
      catch { unset ids }
   }
   global STAR1

   # put the regions on a chain
   set stamps [chainNew REGION]

   set nstars [chainSize $stars]

   set stampsize 1
   loop i 0 $nstars {
      set star [chainElementGetByPos $stars $i]
      if {[exprGet (int)$star.sym] == 0} { continue; }
      if [expr [exprGet (int)$star.flags] & $STAR1(SATURATED)] { continue; }

      set starreg [handleBindFromHandle [handleNew] *$star.sym->star]
      set residreg [handleBindFromHandle [handleNew] *$star.sym->residual]
      
      chainElementAddByPos $stamps $starreg TAIL AFTER
      chainElementAddByPos $stamps $residreg TAIL AFTER
      
      set stampsize [exprGet $starreg.nrow]
      
      if {$_ids != ""} {
	 if 1 {				# include ofset from centre of pixel
	    set rc [exprGet $star.object->rowc];
	    set rc [expr $rc - int($rc)]
	    if {$rc > 0.5} { set rc [expr $rc-1] }
	    set cc [exprGet $star.object->colc];
	    set cc [expr $cc - int($cc)]
	    if {$cc > 0.5} { set cc [expr $cc-1] }
      
	    set rc [format "%5.2f" $rc]; set cc [format "%5.2f" $cc];
	    lappend ids [list [list [exprGet $star.starid]$rc:$cc] ""]
	 } else {
	    lappend ids [list [exprGet $star.starid] ""]
	 }
	 lappend ids [list [exprGet (int)$star.flags] ""]
      }
      
      handleDel $starreg
      handleDel $residreg
      handleDel $star
   }

   set mosaic [ps_mosaic $stamps $stampsize 0 $_ids 1]

   #get rid of the stamp chain
   chainDel $stamps
   return $mosaic
}

# make a mosaic of all postage stamps in star1s, optionally returning
# a list of ids of the objects and their places in the mosaic
#
# If band is >= 0, assume that the starlist actually contains STAR1Cs,
# not STAR1s
#
# If bad_proc is provided, it's a proc that's run for every object
# on the list, called in the scope of the _calling_ program with
# two arguments; the index in the chain and the object at that
# location.  If the proc returns non-zero, that object is ignored.
#
# An entry is made on the ids list ("BAD") even if bad_proc is true
#
proc star1_mosaic {starlist stampsize {_ids ""} {band -1} {bad_proc ""}} {
   if {$_ids != ""} {
      upvar $_ids ids
      if [info exists ids] { unset ids }
   }
   global MASK_TYPE STAR1

   # put the regions on a chain
   set stamps [chainNew REGION]
   set nstars [chainSize $starlist]
   set nfakestars 0
   loop i 0 $nstars {
      if {$band >= 0} {
	 set star1c [chainElementGetByPos $starlist $i]
	 set star [handleBindFromHandle [handleNew] *$star1c.star1<$band>]
	 handleDel $star1c
      } else {
	 set star [chainElementGetByPos $starlist $i]
      }

      if {$bad_proc != ""} {
	 if [uplevel [list $bad_proc $i $star]] {
	    handleDel $star
	    incr nfakestars
	    lappend ids "BAD"
	    continue
	 }
      }

     set reg [handleBindFromHandle [handleNew] *$star.object->region]
     if {[exprGet (long)&$reg] == 0x0 ||
	 [catch { set mm [exprGet $reg.mask] }]} {
	handleDel $reg
	set reg [regNew $stampsize $stampsize]; regIntSetVal $reg [softBiasGet]
	set mask [spanmaskNew -nrow $stampsize -ncol $stampsize]
	handleSetFromHandle $reg.mask (MASK*)&$mask
	handleDel $mask

	incr nfakestars
	lappend made_regs $reg
     }
      
     set flag [exprGet (long)$star.flags]
     if {$flag == $STAR1(FAKEWING)} {
	incr nfakestars
     } else {
      if {[exprGet $reg.mask] == 0} {
	 set mask 0
      } else {
	 set mm [handleBindFromHandle [handleNew] *$reg.mask]
	 set sm [handleBindNew [exprGet &$mm] SPANMASK]
	 handleDel $mm
	 set mask [maskNew $stampsize $stampsize]
	 handleSet $mask.row0 [exprGet $reg.row0]
	 handleSet $mask.col0 [exprGet $reg.col0]

	 if {![info exists masks]} {
	    set masks [chainNew MASK]
	 }

	 loop j 0 [array size MASK_TYPE] {
	    set m [handleBindNew [exprGet $sm.masks<$j>] CHAIN]
	    maskSetFromObjmaskChain $m $mask [expr 1<<($j%8)]
	    handleDel $m
	 }
	 handleDel $sm      
      }

      if {$_ids != ""} {
	 catch {
	    set delr [expr [exprGet $star.object->rowc]-[exprGet $reg.row0]]
	    set delc [expr [exprGet $star.object->colc]-[exprGet $reg.col0]]
	    lappend ids [list \
			     [exprGet $star.starid] \
			     [exprGet (int)$star.flags] \
			     [exprGet (int)$star.frame] $delr $delc]
         }
      }
      
      if {$mask == 0} {
	 set masks 0
      } else {
	 chainElementAddByPos $masks $mask TAIL AFTER
	 handleDel $mask
      }
      chainElementAddByPos $stamps $reg TAIL AFTER
      handleDel $reg
      handleDel $star
    }
   }

   set nstars [expr $nstars - $nfakestars]
   if {$nstars <= 0} {
       set nstars 0
       if {$nfakestars > 0} {
	  echo "      all rejected wing stars are fake ones"
       }
   }
   # make the mosaic
   if {$nstars == 0} {
      set mosaic [regNew 1 1]
   } else {
      set mosaic [eval ps_mosaic $stamps $stampsize $masks $_ids]
   }

   #get rid of the stamp chain
   chainDel $stamps

   if [info exists make_regs] {
      foreach reg $made_regs {
	 regDel $reg
      }
   }

   return $mosaic
}



# make a mosaic of postage stamps in starlist flagged by frame_info, 
# and return a list of ids of the objects and their places in the mosaic
proc star1_mosaic2 {frame_info starlist stampsize {_ids ""} {coeff_stars 0} } {
   if {$_ids != ""} {
      upvar $_ids ids
   }

   # copy stars used for this frame to a new chain
   set selected_stars [chainNew STAR1]   
   set i1 [exprGet $frame_info.firststar]
   set nstars [exprGet $frame_info.Nallstars]
   set i2 [expr [exprGet $frame_info.firststar] + $nstars]   
   loop i $i1 $i2 {
       set star [chainElementGetByPos $starlist $i]  
       set ii [expr $i - $i1]
       set starOK($ii) 1
       if {$coeff_stars && ![exprGet $star.used4coeffs]} { 
           set starOK($ii) 0
       }
       if {$starOK($ii)} {
           chainElementAddByPos $selected_stars $star TAIL AFTER
       }
       handleDel $star
   }
  
   # make mosaic
   set mosaic [star1_mosaic $selected_stars $stampsize ids]

   # add PSF clipping flag from frame_info to ids 
   loop i 0 $nstars { 
       if {$_ids != "" && $starOK($i)} {
          set new2 [concat [lindex [lindex $ids $i] 2] \
                      [exprGet $frame_info.starflags<$i>]] 
          set new [list [lindex [lindex $ids $i] 0] \
                      [lindex [lindex $ids $i] 1] $new2] 
          set ids [lreplace $ids $i $i $new]	
       }
   } 
   chainDel $selected_stars

   return $mosaic
}



# make a mosaic of all wing stars in star1s
proc wing_mosaic {starlist wingsize} {

   # put the regions on a chain
   set stamps [chainNew REGION]
   set nstars [chainSize $starlist]
   loop i 0 $nstars {
      set star [chainElementGetByPos $starlist $i]
      set reg [handleBindFromHandle [handleNew] *$star.object->region]
      chainElementAddByPos $stamps $reg TAIL AFTER
      handleDel $reg
      handleDel $star
   }

   # make the mosaic
   set mosaic [eval ps_mosaic $stamps $wingsize]

   #get rid of the stamp chain
   chainDel $stamps
   return $mosaic
}




# make a mosaic of all postage stamps in pstamps. If _ids is supplied, it's
# a list of characteristics of each object, and this routine will add the
# origin of each object's image to the value (e.g. if the list is
#   {id1} {id2} ... it'll become {r0_1 c0_1 id1} {r0_2 c0_2 id2} ...)
# Entries "BAD" in _ids correspond to missing elements of pstamps (i.e.
# stamps that were deliberately omitted while building the pstamps chain
# from some other data structure)
#
# If col_even is true, the mosaic will have an even number of columns
#
proc ps_mosaic {pstamps stampsize {masks 0} {_ids ""} {col_even 0} {nh -1} {nv -1}} {
   if {$_ids != ""} {
      upvar $_ids ids
   }
   # Region will have nv by nh stamps, with $gutter pixels between each
   set gutter 2
   set nstars [chainSize $pstamps]
   if {$nh == -1 && $nv == -1} {
     set nh [expr int(sqrt($nstars)+0.999)]
     if {$col_even && [expr $nh%2 == 1]} {
       incr nh
     } else {
       set col_even 0
     }
     if {$nh == 0} {
       set nh 1; set nv 1;
     } else {
        set nv [expr int($nstars/$nh)]
        while {[expr $nv*$nh] < $nstars} { incr nv }
     }
   }
   set mosaic [regNew [expr $nv*($stampsize+$gutter)-$gutter] \
		   [expr $nh*($stampsize+$gutter)-$gutter]]
   if {$masks != 0} {
      assert {[exprGet $pstamps.nElements] == [exprGet $masks.nElements]}
      set mask [maskNew [exprGet $mosaic.nrow] [exprGet $mosaic.ncol]]
      maskClear $mask
      handleSetFromHandle $mosaic.mask &$mask
   }
   regClear $mosaic
   set r0 0
   set c0 0
   set full 0
   set hcount 0

   set j -1;				# counter in ids
   loop i 0 $nstars {

      set reg [chainElementGetByPos $pstamps $i]
      if {$masks != 0} {
	 set m [chainElementGetByPos $masks $i]
      }

      if {$hcount < $nh || ($col_even && $hcount < $nh - 1)} {
	 incr hcount
      } else { 
         set hcount 1
	 set c0 0
	 set r0 [expr $r0+$stampsize+$gutter]
      }
      set sub [subRegNew $mosaic $stampsize $stampsize $r0 $c0]
      if {$masks != 0} {
	 set sm [subMaskNew $mask $stampsize $stampsize $r0 $c0]
	 maskCopy $m $sm
      }

      if {$_ids != ""} {
	 while {[lindex $ids [incr j]] == "BAD"} {
	    ;				# Skip missing stamps
	 }

	 set ids [lreplace $ids $j $j [list $r0 $c0 [lindex $ids $j]]]
      }

      set c0 [expr $c0+$stampsize+$gutter]
      regAdd $sub $reg
      handleDel $reg
      if {$masks != 0} {
	 handleDel $m
	 maskDel $sm
      }
      regDel $sub
   }
   if {$masks != 0} {
      handleDel $mask
      chainDestroy $masks maskDel
   }

   return $mosaic
}


############################################################################
# Find the hdu containing the information for type and filter in a fang file
# 

proc get_hdu {file filter type} {


   set hdr [hdrReadAsFits [hdrNew] $file]
   set hdusets [hdrGetAsAscii $hdr HDUSETS]

   
   # Get the name and number of filters for each hduset
   #   Note: this is not perfectly general as we must know that 
   #   SFILTERS corresponds to the type stamps. If a new type 
   #   is added there will be trouble. But the order is free to change.

   set filters(stamps) [hdrGetAsAscii $hdr SFILTERS]
   set nfilters(stamps) [llength $filters(stamps)]

   set filters(params) [hdrGetAsAscii $hdr PFILTERS]
   set nfilters(params) [llength $filters(params)]

   set filters(quarts) [hdrGetAsAscii $hdr QFILTERS]
   set nfilters(quarts) [llength  $filters(quarts)]

   hdrFreeAll $hdr; hdrDel $hdr

   # Get the order for the relevant hduset
   set index [lsearch $hdusets $type]

   # Get the order for the relevant filters
   set filter_index [lsearch $filters($type) $filter]
   assert { $filter_index != -1 }

   # Now calculate the hdu number
   set hdu 1
   loop i 0 $index {
      set hdu [expr $hdu + $nfilters([lindex $hdusets $i])]
   }
   set hdu [expr $hdu + $filter_index]
   return $hdu
}
############################################################################
# Find the hdus containing the information of a given type type for all
# filters in a fang file.
# 

proc get_hdu_for_filterset {file type _hdus} {
   upvar $_hdus hdus

   set hdr [hdrReadAsFits [hdrNew] $file]
   set hdusets [hdrGetAsAscii $hdr HDUSETS]

   
   # Get the name and number of filters for each hduset
   #   Note: this is not perfectly general as we must know that 
   #   SFILTERS corresponds to the type stamps. If a new type 
   #   is added there will be trouble. But the order is free to change.

   set filters(stamps) [hdrGetAsAscii $hdr SFILTERS]
   set nfilters(stamps) [llength $filters(stamps)]

   set filters(params) [hdrGetAsAscii $hdr PFILTERS]
   set nfilters(params) [llength $filters(params)]

   set filters(quarts) [hdrGetAsAscii $hdr QFILTERS]
   set nfilters(quarts) [llength  $filters(quarts)]
  
   hdrFreeAll $hdr; hdrDel $hdr

   # Get the order for the relevant hduset
   set index [lsearch $hdusets $type]
   if {$index < 0} {
      error "I cannot find \"$type\" data in $file"
   }

   foreach filter $filters($type) {
      # Get the order for the relevant filters
      set filter_index [lsearch $filters($type) $filter]
      assert { $filter_index != -1 }
      
      # Now calculate the hdu number
      set h 1
      loop i 0 $index {
	 set h [expr $h + $nfilters([lindex $hdusets $i])]
      }
      set hdus($filter) [expr $h + $filter_index]
   }
}

######################################################################
# Proc to print out the psfs in the calib1byframelist

proc log_psfs {calib1byframelist {fp_verb 6}} {
   global filterlist verbose diagnostics

   if {$verbose > $fp_verb || $diagnostics > $fp_verb} {
      verb_echo $fp_verb  "------------------------------------------------------"
      verb_echo $fp_verb " filter     s1     s2     b       p0    beta  apCorr"
      set cursor [chainCursorNew $calib1byframelist]
      while {[set cal [chainWalk $calib1byframelist $cursor NEXT]] != ""} {
	 set field [exprGet $cal.field]
	 verb_echo $fp_verb " Field $field"
	 foreach filter $filterlist {
	    set ifilter [lsearch $filterlist $filter]
            set apCorr [exprGet $cal.calib<$ifilter>->app_correction]
	    set psf [exprGet *$cal.calib<$ifilter>->psf]
	    set sx1 [keylget psf sigmax1]
	    set sx2 [keylget psf sigmax2]
	    set sy1 [keylget psf sigmay1]
	    set sy2 [keylget psf sigmay2]
	    set b [keylget psf b]
	    set p0 [keylget psf p0]
	    set beta [keylget psf beta]
	    set string [format " %3s   %8.3f %6.3f %7.4f %7.4f %6.3f %6.3f" \
			    $filter $sx1 $sx2 $b $p0 $beta $apCorr]
	    verb_echo $fp_verb "$string"
	 }
      }
      verb_echo $fp_verb  "------------------------------------------------------"
      chainCursorDel $calib1byframelist $cursor
   }
}

######################################################################
# Proc to print out the psfs in the calib1byframelist

proc log_starlist {starlist filter} {
   global filterlist verbose diagnostics

   set fp_verb 8

   if {$verbose > $fp_verb || $diagnostics > $fp_verb} {
      set cursor [chainCursorNew $starlist]
      verb_echo $fp_verb "Filter $filter"
      verb_echo $fp_verb \
        "field  id     sx1     sx2     width        b     chisq        flags"
      while {[set star [chainWalk $starlist $cursor NEXT]] != ""} {
	 set field [exprGet $star.frame]
	 set id [exprGet $star.starid]
	 set psf [exprGet *$star.dgpsf]
	 if {[llength $psf] == 0} {
	    set sx1 -1
	    set sx2 -1
	    set b -1
	    set chisq -1
	    set width -1
	 } else {
	    set sx1 [keylget psf sigmax1]
	    set sx2 [keylget psf sigmax2]
	    set b [keylget psf b]
	    set chisq [keylget psf chisq]
	    set width [keylget psf width]
	 }
	 set flags [print_flags_info [exprGet (int)$star.flags]]
	 set string [format " %3d  %3d  %s  %s  %s  %s  %s %s" \
			 $field $id $sx1 $sx2 $width $b   $chisq     $flags]
	 verb_echo $fp_verb "$string"
      }
      chainCursorDel $starlist $cursor
   }
}


######################################################################
# Proc to print out coordinates of stars in a starlist

proc star_positions {starlist} {

    catch {
      set cursor [chainCursorNew $starlist]
      echo  " star#  star->id   colC   rowC"
      set i 0
      while {[set star [chainWalk $starlist $cursor NEXT]] != ""} {
         set i [expr 1+$i]
	 set id [exprGet $star.starid]
	 set rowc [exprGet $star.object->rowc]
	 set colc [exprGet $star.object->colc]
	 set string [format "  %3d  %3d  %8.1f   %8.1f" $i $id $colc $rowc]
	 echo "$string"
      }
      chainCursorDel $starlist $cursor
   }
}



######################################################################
# Proc to print out the flux20 info in the calib1byframelist

proc log_photoparams {calib1byframelist} {
   global filterlist verbose

   set fpp_verb 6

   if {$verbose > $fpp_verb} {
      set cursor [chainCursorNew $calib1byframelist]
      while {[set cal [chainWalk $calib1byframelist $cursor NEXT]] != ""} {
	 set field [exprGet $cal.field]
	 verb_echo $fpp_verb "Field $field"
	 verb_echo $fpp_verb " filter  flux20    flux20Err"
	 foreach filter $filterlist {
	    set ifilter [lsearch $filterlist $filter]
	    set flux20 [exprGet $cal.calib<$ifilter>->flux20]
	    set flux20Err [exprGet $cal.calib<$ifilter>->flux20Err]
	    set string [format "   %1s   %8s  %8s" $filter $flux20 $flux20Err]
	    verb_echo $fpp_verb "$string"
	 }
      }
      chainCursorDel $calib1byframelist $cursor
   }
}

######################################################################
# Proc to dump sky values for the first filter in filterlist to a file
proc dump_sky {calib1byframelist file ifilter field0} {


    set f [open $file w]
    puts $f " frame      sky_int  sky_raw    sky_slope   sky_sig    sky+error   sky-error  gain"
    set i 0
    set cursor [chainCursorNew $calib1byframelist]
    while {[set cal [chainWalk $calib1byframelist $cursor NEXT]] != ""} {
	set field [exprGet $cal.field]
	set skyint [exprGet $cal.calib<$ifilter>->sky]
	set skyraw [exprGet $cal.calib<$ifilter>->skyraw]
	set skyslope [exprGet $cal.calib<$ifilter>->skyslope]
	set skysig [exprGet $cal.calib<$ifilter>->skysig]
        if {$skysig > 0} {
             set gain [expr $skyint / $skysig / $skysig]
        } else {
             set gain 0.0
        }
        # skyerr = skysig / sqrt(n)
	set skyerr [exprGet $cal.calib<$ifilter>->skyerr] 
        set sky_plus [expr $skyraw + $skyerr]
        set sky_minus [expr $skyraw - $skyerr]
        set field [expr $i + $field0]
	set string [format "   %1s     %8.3f %8.3f %11.6f %11.6f %11.6f %11.6f %5.2f " \
                    $field $skyint $skyraw $skyslope $skysig $sky_minus $sky_plus $gain]
	puts $f $string
        set i [expr $i + 1] 
    }
    chainCursorDel $calib1byframelist $cursor
    close $f
}
   

######################################################################
# Proc to calculate sky slope for each filter
proc get_skyslope {calib1byframelist filters pixperframe} {

      # number of frames
      set Nframes [chainSize $calib1byframelist]
      # loop over filters 
      foreach filter $filters {
         set ifilter [lsearch $filters $filter] 
         # get the array of sky values
         loop frame 0 $Nframes { 
            set cal [chainElementGetByPos $calib1byframelist $frame]
            set sky($frame) [exprGet $cal.calib<$ifilter>->sky]
            set skyslope($frame) 0
         }
         # get slope for the sky values
         get_slope sky $Nframes skyslope
         # set skyslope field for each frame
         loop frame 0 $Nframes { 
            set sky_slope [expr 1.0*$skyslope($frame) / $pixperframe]
            set cal [chainElementGetByPos $calib1byframelist $frame]
            handleSet $cal.calib<$ifilter>->skyslope $sky_slope          
         }
      }
 
}


######################################################################
# Proc to calculate slope for an array
proc get_slope {array N slope} {
   upvar $array y 
   upvar $slope dy
 
    loop i 0 $N { 
       set m [expr $i + 1]
       set n [expr $i - 1]
       set ndelta 1
       if {$i == 0} {
           set n 0 
           if {$N == 1} {
             set m 0
           }   
       } else {
          if {$i == [expr $N - 1]} {
              set m $i 
          } else {
              set ndelta 2
          }
       }
       set deltay [expr $y($m) - $y($n)]
       set dy($i) [expr $deltay / $ndelta]
    } 
}



######################################################################
# Proc to print out the flux20 info in the calib1bytimelist

proc log_patchcal {calib1bytimelist _PatchFileName fpp_verb} {
   global filterlist verbose
   upvar $_PatchFileName PatchFileName

   if {$verbose > $fpp_verb} {
      set patchnum 0
      set cursor [chainCursorNew $calib1bytimelist]
      while {[set cal [chainWalk $calib1bytimelist $cursor NEXT]] != ""} {
	 set mjd [exprGet $cal.mjd]
         incr patchnum 
	 verb_echo $fpp_verb "Patch $patchnum: mjd=$mjd"
	 verb_echo $fpp_verb "File: $PatchFileName($patchnum)"      
	 verb_echo $fpp_verb " filter  flux20    flux20Err  rel. error"
	 foreach filter $filterlist {
	    set ifilter [lsearch $filterlist $filter]
	    set flux20 [exprGet $cal.calib<$ifilter>->flux20]
	    set flux20Err [exprGet $cal.calib<$ifilter>->flux20Err]
            if {$flux20 > 0} {
                set re [expr 100.0*$flux20Err/$flux20]
            } else {
                set re 0.0
            }
	    set string [format "    %1s   %8.2f   %8.2f   %8.2f" $filter $flux20 $flux20Err $re]
	    verb_echo $fpp_verb "$string"
	 }
      }
      verb_echo $fpp_verb "--------------------------"
      chainCursorDel $calib1bytimelist $cursor
   }
}

######################################################################
# Declare schematrans for CALIB1BYTIME version of CALIBIO
# n.b. for writing to psCT files
proc declare_io_schematrans_for_calib1bytime { } {
 
    # Declare schematrans for CALIB1BYTIME version of CALIBIO
    set trans [schemaTransNewAdd CALIB_IO {mjd flux20 flux20Err}]                   
    fitsBinDeclareSchemaTrans $trans CALIB_IO
    handleDel $trans
 
}



######################################################################
# Declare schematrans for psPSF version of CALIBIO
# used for writing to psPSF files
proc declare_io_schematrans_for_psPSF { } {
   # Declare schematrans for psPSF version of CALIBIO
   set trans [schemaTransNewAdd CALIB_IO {field psp_status status \
                      sky skysig skyerr skyslope lbias rbias \    
                      psf_ap_correctionErr \
                      psf_nstar psf_sigma1 psf_sigma2 psf_b psf_p0 \
                      psf_beta psf_sigmap psf_width psf_psfCounts  \
                      psf_sigma1_2G psf_sigma2_2G psf_b_2G psfCounts \
                      prof_nprof prof_mean prof_med prof_sig \
                      gain dark_variance }  \
	              fields]

   fitsBinDeclareSchemaTrans $trans CALIB_IO
   handleDel $trans
   #
   # If we know about _CALIB_IOs declare that too
   #
   if ![catch {schemaGetFromType _CALIB_IO}] {
      set trans [schemaTransNewAll _CALIB_IO];
      fitsBinDeclareSchemaTrans $trans _CALIB_IO;
      handleDel $trans
   }

   return $fields
}


######################################################################
# Declare schematrans for psField version of CALIBIO
# used for writing PT calibration to psField files
proc declare_io_schematrans_for_psField { } {
   # Declare schematrans for psField version of CALIBIO
   set trans [schemaTransNewAdd CALIB_IO {flux20 flux20Err} fields]

   fitsBinDeclareSchemaTrans $trans CALIB_IO
   handleDel $trans

   return $fields
}

######################################################################
# Declare schematrans for WC version of CALIBIO
# used for appending wing correction to psField files
proc declare_io_schematrans_for_psFieldWC { } {
   # Declare schematrans 
   set list [list nann_ap_frame nann_ap_run ap_corr_run ap_corr_runErr]
   set trans [schemaTransNewAdd CALIB_IO $list fields]
   fitsBinDeclareSchemaTrans $trans CALIB_IO
   handleDel $trans

   return $fields
}

#############################################################################
# Forms paths to mt patch files from input specifications given through 
# mtPatchFormat and a list of directories with corresponding suffixes listed 
# in mtPatches
#
# The elements of mtPatches are:
#   Number $patchDir [extra]
# where patchDir may contain variables (i.e. it'll be eval'd; mtPatchDir
# and env are in scope), and "extra" is an optional extra directory component.
# E.g. if mtPatchDir = "/data/Patches" and mtPatchFormat = "kaCalObj-%08d.fit"
#	{72299 $mtPatchDir 10}
#	{666 $env(HOME)}
# would expand to
#   /data/Patches/10/kaCalObj-00072299.fit
# and
#   /home/user/kaCalObj-00000666.fit
#
proc get_mtPatchList {mtPatchDir mtPatchFormat mtPatches} {
   global env

   set aux [lindex $mtPatches 0]
   if {$aux == {} || $aux == { } || $aux == {  }} {
       return "" 
   }  

   # form path for each file in mtPatches
   foreach entry $mtPatches {
      eval set patchDir [lindex $entry 1]

      set patch $patchDir
      set extra_path [lindex $entry 2]
      if {$extra_path != ""} {
	 if ![regexp {^/} $extra_path] {# allow extra to have a leading /
	    append patch "/"
	 }
	 append patch $extra_path
      }
      append patch "/[format $mtPatchFormat [lindex $entry 0]]"
      
      lappend PathList $patch
   }

   return $PathList
}

#############################################################################
# return mask for a vector with all elements set to 1 except those
# that differ by more than fracMax percent from the mean of their
# neighbors, which should be set to 0
proc get_spike_mask {vector fracMax {inmask ""}} {

     set mask [vectorExprNew [exprGet $vector.dimen]]
     if {$inmask != ""} {
        assert {[exprGet $vector.dimen] == [exprGet $inmask.dimen] }
     }
   
     loop i 1 [expr [exprGet $vector.dimen]-2] {
        set im1 [exprGet $vector.vec<[expr $i-1]>]
        set ip1 [exprGet $vector.vec<[expr $i+1]>]
        set val [exprGet $vector.vec<$i>]
        set mean [expr 0.5*($im1+$ip1)]
        set frac [expr abs($val-$mean)]
        handleSet $mask.vec<$i> 1
        if {$inmask != ""} {
	    handleSet $mask.vec<$i> [exprGet $inmask.vec<$i>] 
        }
        if {$frac > $fracMax} {
            handleSet $mask.vec<$i> 0
        }        
     }
     if {$inmask != ""} {
        handleSet $mask.vec<0> [exprGet $inmask.vec<0>] 
	set N [expr [exprGet $vector.dimen]-1]
        handleSet $mask.vec<$N> [exprGet $inmask.vec<$N>] 
     } else {
        handleSet $mask.vec<0> 1
        handleSet $mask.vec<[expr [exprGet $vector.dimen]-1]> 1
     }

   return $mask

}




#############################################################################
# Auxiliary proc to copy flux20 from ctimes to frameinfolist for those frames 
# which overlap with MT patches
proc add_calib {filterlist startField ctimes _frameinfolist} {

   global id_values Npatches
   upvar $_frameinfolist frameinfolist

   set n [chainSize $ctimes]
   # loop over each MT patch
   loop i 0  $n {
       set cal [chainElementGetByPos $ctimes $i]
       # frame corresponding to this patch
       set patch_frame [exprGet $cal.nearest_frame]
       set n_frame [expr $patch_frame - $startField]
       # copy flux20 for each filter      
       foreach filter $filterlist {
           set ifilter [lsearch $filterlist $filter]
           set flux20 [exprGet $cal.calib<$ifilter>->flux20] 
           set frameinfo [chainElementGetByPos $frameinfolist($filter) $n_frame]
	   handleSet $frameinfo.patch_flux20 $flux20
       }
   }

}


#############################################################################
# Auxiliary proc to stuff KAL*ID keywords
proc add_KAL_keywords {filterlist startField Npatches _PatchFileName _PatchFilePrep} {

   global id_values
   upvar $_PatchFileName PatchFileName 
   upvar $_PatchFilePrep PatchFilePrep

   # insert keywords
   loop i 1 [expr $Npatches + 1] {
       set hdr [hdrReadAsFits [hdrNew] $PatchFileName($i)]
       set keyword KAL[format %02d $i]_ID 
       if [catch {set value [hdrGetAsAscii $hdr KALI_ID]}] {
           set id_values($keyword) "not available"
       } else {
           set prep $PatchFilePrep($i)
           set value "$PatchFilePrep($i):$value"
           if [catch {set id_values($keyword) $value}] {
               set id_values($keyword) "not available"
           } 
       }
       hdrDel $hdr    
   }   

}


# test that flatfield vector is OK:
# - it doesn't suffer from the odd/even problem
# if OddEvenTol <= 0 return the actual difference
proc is_good_flat {FFfile {ncol1 0} {ncol2 -1} {OddEvenTol 0.0003}} {

    # get flatfield vector
    set reg [regNew]
    regReadAsFits $reg $FFfile
    set ncol [expr [exprGet $reg.ncol]-1] 
    if {$ncol1 < 0} {
        set ncol1 0
    }    
    if {$ncol2 < $ncol1 || $ncol2 > $ncol} {
        set ncol $ncol2
    }

    set FFvector [vectorGetFromRegion $reg 0 0 $ncol1 $ncol2 0]
    # get masks
    set oddMask [vectorExprNew $ncol]
    set evenMask [vectorExprNew $ncol]   
    loop i 0 [expr $ncol/2] {
	handleSet $oddMask.vec<[expr 2*$i+1]> 1
	handleSet $evenMask.vec<[expr 2*$i]>  1
    }
    # get medians   
    if {0} {
      set meanClipped [vSigmaClip $FFvector -vMask $oddMask -sigmaClip 3.0 -nIter 3]
      set oddMedian [lindex [lindex $meanClipped 1] 1]
      set meanClipped [vSigmaClip $FFvector -vMask $evenMask -sigmaClip 3.0 -nIter 3]
      set evenMedian [lindex [lindex $meanClipped 1] 1]
    } else {
      set oddMedian [vMedian $FFvector -vMask $oddMask]
      set evenMedian [vMedian $FFvector -vMask $evenMask]
    }
    set diff [expr 2*abs($oddMedian-$evenMedian)/($oddMedian+$evenMedian)]
    # clean
    vListDel [list $FFvector $oddMask $evenMask] 
    regDel $reg
    # return what is asked for
    if {$OddEvenTol < 0} {
        return $diff
    } else {
        set isOK 1
        if {$diff > $OddEvenTol} {
           set isOK 0 
        }  
        return $isOK
    }

}



# test that bias vector is OK:
# - each half should have very small sigma/median
#   (i.e. bias should be very flat)
# if Tol <= 0 return the actual scatter
proc is_good_bias {Bfile {OddEvenTol 0.0003} \
		  {Lo 40} {Ro 40} {splitCol 1024}} {

    # get flatfield vector
    set reg [regNew]
    regReadAsFits $reg $FFfile
    set ncol [exprGet $reg.ncol]
    set FFvector [vectorGetFromRegion $reg 0 0 0 [expr $ncol-1] 0]
    # get masks
    set oddMask [vectorExprNew $ncol]
    set evenMask [vectorExprNew $ncol]   
    loop i 0 [expr $ncol/2] {
	handleSet $oddMask.vec<[expr 2*$i+1]> 1
	handleSet $evenMask.vec<[expr 2*$i]>  1
    }
    # get medians   
    set oddMedian [vMedian $FFvector -vMask $oddMask]
    set evenMedian [vMedian $FFvector -vMask $evenMask]
    set diff [expr 2*abs($oddMedian-$evenMedian)/($oddMedian+$evenMedian)]
    # clean
    vListDel [list $FFvector $oddMask $evenMask] 
    regDel $reg
    # return what is asked for
    if {$OddEvenTol <= 0} {
        return $diff
    } else {
        set isOK 1
        if {$diff > $OddEvenTol} {
           set isOK 0 
        }  
        return $isOK
    }

}





#############################################################################
# Auxiliary proc to print astrometric coefficient (debug)
proc print_astro {filterlist calib1byframelist} {

      set cursor [chainCursorNew $calib1byframelist]
      while {[set cal [chainWalk $calib1byframelist $cursor NEXT]] != ""} {
	 set field [exprGet $cal.field]
         echo " === field: $field ==="
	 foreach filter $filterlist {
	    set ifilter [lsearch $filterlist $filter]
	    set a [exprGet $cal.calib<$ifilter>->toGCC->a]	
	    set b [exprGet $cal.calib<$ifilter>->toGCC->b]	
	    set c [exprGet $cal.calib<$ifilter>->toGCC->c]	
	    set d [exprGet $cal.calib<$ifilter>->toGCC->d]	
	    set e [exprGet $cal.calib<$ifilter>->toGCC->e]	
	    set f [exprGet $cal.calib<$ifilter>->toGCC->f]	      
	    echo "    filter: $filter"
            echo "a = $a, b = $b, c = $c"
            echo "d = $d, e = $e, f = $f"
	 }
      }
      chainCursorDel $calib1byframelist $cursor
}


 
proc check_file {file string} {

   if {![file exists $file]} {
      echo "*** $string file $file doesn't exist!? ***"
      return -1
   }
   return 0
}
 

# dump some quantities for PSF stars in each band to files
# PSFstars-run-column-filter.dat  
proc print_PSFlist {open_f close_f file starlist n1 n2 run camCol outputDir  
                    filter start_frame rowsperframe master_row_min master_row_max {this_frame -1}} {
global verbose
        set long 1
 
        set outfile $outputDir/PSFstars-$run-$camCol-$filter.dat  
        verb_echo 8 "dumping PSF stars ($n1-$n2) for field $this_frame, filter $filter to $outfile ($open_f,$close_f)"

        if {$open_f} {
           verb_echo 8 "opening $outfile"
           set file [open $outfile w]
           puts $file "### PSF stars for run $run, camCol $camCol, filter $filter"
	   puts $file "# PSF: if 1 this star was used for QA on KL stuff" 
           puts $file "# apC_PSF: ap. corr. calculated by phPsfSetAtPoint"  
           puts $file "# Effwidth: measured for star, PSFwidth: from local PSF" 
	   if {$long} {
              puts $file "# n.b. psfCnt_PSF don't include apCorr" 

	      puts $file "# ID frame colc rowc masterRow  PSF bad coeff photo \
                           Effwidth \
                           psfCounts psfCnErr apCnts apCntsErr \
                           cnt_rat cnt_ratErr psfCnt_PSF apCnt_PSF \
                           Q    U   sky  Wcorr  apCn5  apCn7"
           } else {
	      puts $file "#    ID frame colc rowc masterRow  PSF bad apC_PSF apC_cnt  \
                           Effwidth PSFwidth"
           }
        }

 
        loop i $n1 [expr $n2 + 1] {      
            set star [chainElementGetByPos $starlist $i]
            set id [exprGet $star.starid] 
            set frame [exprGet $star.frame]
	    set rowc [expr int([exprGet $star.object->rowc])]
            set masterRow [expr int($rowc + ($frame-$this_frame)*$rowsperframe)]    
            if {($this_frame < 0) || ($masterRow >= $master_row_min && $masterRow <= $master_row_max)} {
	       if {$this_frame > 0} {
                  set frame $this_frame
               }
              catch {
	       set colc [expr int([exprGet $star.object->colc])]
	       set rowc [expr int([exprGet $star.object->rowc])]
	       set loc_sky [format "%7.2f" [exprGet $star.object->sky]]
	       set apCounts [format "%7.3g" [exprGet $star.apCounts]]
	       set apCountsErr [format "%7.3g" [exprGet $star.apCountsErr]]
	       set psfCounts [format "%7.3g" [exprGet $star.psfCounts]]	 
	       set psfCountsErr [format "%7.3g" [exprGet $star.psfCountsErr]]	 
	       if {$psfCounts > 0} {
                   set count_rat [format "%6.4g" [expr $apCounts/$psfCounts]]
	       } else {  
                   set count_rat 0.0
               } 
               # redefined count_rat in findPsf.c
               if [catch {
                     set count_rat [format "%7.4f" [exprGet $star.count_rat]]
                     set count_ratErr [format "%7.4f" [exprGet $star.count_ratErr]]
	          }] {
                     echo caught error for star $star
                     error
	       } 
               set apCorr_PSF [format "%6.4g" [exprGet $star.apCorr_PSF]]
	       set apCounts_PSF [format "%7.3g" [exprGet $star.apCounts_PSF]]
	       set psfCounts_PSF [format "%7.3g" [exprGet $star.psfCounts_PSF]]	     
	       set apCounts5 [format "%7.3g" [exprGet $star.apCounts1]]
	       set apCounts7 [format "%7.3g" [exprGet $star.apCounts2]]
               set wingcorr [format "%7.3g" [exprGet $star.wingcorr]] 
               set WC "$apCounts5 $apCounts7 $wingcorr"
               set PSFflag [exprGet $star.PSFflag]
               set used4coeffs [exprGet $star.used4coeffs]
               set used4photo  [exprGet $star.used4photo]
               set badflag [exprGet $star.badstar]
               set width [exprGet $star.Eff_width]
               set PSFwidth [format "%6.2f" [exprGet $star.PSFwidth]]
               set Q [exprGet $star.Q]
               set U [exprGet $star.Q]
 	       set psf [exprGet *$star.dgpsf]
               if {$psf != ""} {
                  set sigma1 [keylget psf sigmax1]
                  set sigma2 [keylget psf sigmax2]
                  set b [keylget psf b]
               } else {
                  set sigma1 -1
                  set sigma2 -1
                  set b -1
               }
	       set position  [format "%4d %4d %4d %4d %7d"  $id $frame $colc $rowc $masterRow]
	       set flags     [format "%2d %2d %2d %2d" $PSFflag $badflag $used4coeffs $used4photo]
	       set widths    [format "%5.2f %5.2f %5.2f %5.2f" $width $sigma1 $sigma2 $b]
               set widths [format "%5.2f" $width]; # XXX   
               set counts    [format "%7.3g %5.3g %7.3g %5.3g" $psfCounts $psfCountsErr $apCounts $apCountsErr]
               set countsPSF [format "%7.3f %7.3f %7.3g %7.3g" $count_rat $count_ratErr $psfCounts_PSF $apCounts_PSF]
               set QU        [format "   %5.2f %5.2f" $Q $U] 
               set Lsky [format "%6.2f" $loc_sky]

               if {$long} {
                   puts $file "$position $flags $widths $counts $countsPSF $QU $Lsky $WC" 
               } else {
	           set widths  [format "   %6.2f   %6.2f" $width $PSFwidth]
                   puts $file "$position $flags $widths" 
               }
	      }
            }
        }

        if {$close_f} {
          close $file
          verb_echo 8 "closed $outfile"
          return 0
        } else {
          flush $file
          return $file
        }  
}

 
# dump info on aperture corrections to file 
# ApCorr-run-column-filter.dat  
proc print_ApCorr {frameinfolist  run  camCol  outputDir  filter} {
 
      set outfile $outputDir/ApCorr-$run-$camCol-$filter.dat  
      set file [open $outfile w]
      set nstars [chainSize $frameinfolist]
      puts $file "#  Aperture corrections for run $run, camCol $camCol, filter $filter"
      puts $file "#  Frame Nstars  Nbasis Ncoeffs   aC_cen  aC_max  aC_min"

      #loop over frames for this filter
      set nn [chainSize $frameinfolist]
      loop j 0 $nn {
         set frame_info [chainElementGetByPos $frameinfolist $j]
         set frame [exprGet $frame_info.frame]
         set nstars [exprGet $frame_info.Nframestars]
         set ngood [exprGet $frame_info.NgoodCoeffs]
         set npsf [exprGet $frame_info.NgoodBasis]
         set aC_center [exprGet $frame_info.ap_corr]
         set aC_max [exprGet $frame_info.apCorr_max]
         set aC_min [exprGet $frame_info.apCorr_min]
 
         set line [format "  %4d   %4d   %4d   %4d    %5.2f   %5.2f   %5.2f" \
              $frame $nstars $npsf $ngood $aC_center $aC_max $aC_min] 
         puts $file $line
      }

      close $file

}
 



proc set_display {dis}  {
  global display 
  set_mtv
  set display $dis
}


# This procedure takes a Fang file, prints the header and displays 
# postage stamps filter by filter (assumed that stamp size is 65),
# and simultaneously prints out tables of stellar properties 
# measured by SSC. 
proc fang2sao {fangdir fangfile filterlist {print_hdr 0} } {

   set stampsize 65
   set file $fangdir/$fangfile.fit
   set hdr [hdrReadAsFits [hdrNew] $file] 
   if {$print_hdr} {
      echo " "
      echo " -----------------------"
      echo " Primary header for file"
      echo " $file" 
      hdrPrint $hdr
      echo " -----------------------"
   } 
   hdrFreeAll $hdr; hdrDel $hdr

   echo " "; echo " " 
   echo "Parameters for stars on leading astrometric chip"
   print_Fang_table $file al 


foreach filter $filterlist {
   set hdu [get_hdu $file $filter stamps]
   set tblcol [handleNewFromType TBLCOL]
   fitsRead $tblcol $file -binary -hdu $hdu
			
   # Check that row is correct
   if {$print_hdr} {
      echo " " 
      echo " Header for filter $filter"
      hdrPrint $tblcol.hdr
   }
   set hfilter [string index [hdrGetAsAscii $tblcol.hdr FILTER] 0]
   assert {$hfilter == $filter}
   assert {[lindex [hdrGetAsAscii $tblcol.hdr EXTNAME] 0] == "STAMP"} 

   # Check stampsize 
   assert {[hdrGetAsInt $tblcol.hdr PSSIZE] == $stampsize}

   set stamps [tblcolToPsChain $tblcol]
   handleDelFromType $tblcol
   if {$filter == [lindex $filterlist 0]} {
       echo " There are [chainSize $stamps] stamps in this Fang file"  
       echo " "
   } 

   # print the table with measured parameters
   print_Fang_table $file $filter

   set mosaic [ps_mosaic  $stamps $stampsize]
   display $mosaic "filter $filter"
   regDel $mosaic
}

   echo " "; echo " " 
   echo "Parameters for stars on trailing astrometric chip"
   print_Fang_table $file at


}




# print the table from a Fang file with measured parameters in given filter
# for leading astrometric chip use filter = al, for the trailing one at. 
proc print_Fang_table {file filter {print_hdr 0} } {

   # initialize needed structures
   set tblcol [handleNewFromType TBLCOL]
   if [catch { get_hdu_for_filterset $file params hdu_params}] {
        set hdu_params(r) 2
        set hdu_params(g) 1
   } 

   if {$filter == "al" || $filter == "at"} {
       set name $filter
       set filter r
       # these assume that filters are {r i u z g}
       if {$name == "al"} {
           set hdu(r) [expr $hdu_params(r) - 1]  
       } else {
           set hdu(r) [expr $hdu_params(g) + 1] 
       }
   } else {
      set name $filter
      set hdu($filter) $hdu_params($filter) 
   }

   # read the relevant column from the file and get the sizes
   fitsRead $tblcol $file -binary -hdu $hdu($filter) 
   if {$filter == "al" || $filter == "u"} {
       if {$print_hdr != 0} {
           hdrPrint $tblcol.hdr
       }
   }
   set list [tblFldGet $tblcol -col 0]
   set Nstars [llength $list]
  
   # print out a nice formatted table with a decent header info
   set line " ============================================================================================"
   echo " "
   echo $line
   echo " Chip $name"
   echo " star midR midC  rowC  rErr   colC  CErr  Maj  Min   PA    peak    \
           counts   color  col_err  Fit Status"

   loop i 0 14 {
      set column [tblFldGet $tblcol -col $i]
      loop j 0 $Nstars {
          lappend row($j) [lindex $column $j]          
      }
   }

   loop j 0 $Nstars {
      loop i 0 15 {
          set str [lindex $row($j) $i]
          if {$i == 0 || $i == 1} {   
              set form "%4d"
          }
          if {$i == 2 || $i == 4} {   
              set form "%6.1f"
          }
          if {$i == 3 || $i == 5} {   
              set form "%4.2f"
          }

          if {$i == 6 || $i == 7} {   
              set form "%3.1f"
          }
          if {$i == 8} {   
              set form "%5.1f"
          }
          if {$i == 9} {   
              set form "%8.1f"
          }
          if {$i == 10} {   
              set form "%9.1f"
          }

          if {$i == 11} {   
              set form "%5.2f"
          }

          if {$i == 12} {   
              set form "%5.2f"
          }
          if {$i == 13 || $i == 14} {   
              set form "%2d"
          }

          if {[catch {set string [format $form $str]}] != 0} {
              set string $str 
          }

          set s($i) [move_blanks $string] 
      }
      set string [format "%5d %4s %4s %6s %4s  %6s %4s  %3s \
                          %3s %5s %8s %9s  %4s %3s %4s" $j \
                          $s(0) $s(1) $s(2) $s(3) $s(4) $s(5) $s(6) \
                          $s(7) $s(8) $s(9) $s(10) $s(11) $s(12) $s(13) $s(14)]
      echo $string 
   }  
   echo $line
   # clean up 
   handleDel $tblcol
}

 

# move any trailing blanks to the front of string
proc move_blanks {str} {

   set Nblanks [expr [string length $str] - [string length [string trimright $str]]]
   set str [string trimright $str]
   loop i 0 $Nblanks {
       set str [format " %s" $str]
   }
 
   return $str 
}



# free not needed memory for each star1 in a star1c list 
proc relax_star1clist {_star1clist filterlist Nrelax1 Nrelax2 {relax 0} } {
  upvar $_star1clist star1clist

     # loop over needed star1c
     loop i $Nrelax1 $Nrelax2 {
         set star1c [chainElementGetByPos $star1clist $i]
         # loop over each star1   
         foreach filter $filterlist {
            set ifilter [lsearch $filterlist $filter]
            set star1 [handleBindFromHandle [handleNew] *$star1c.star1<$ifilter>]
	    if $relax {
	        star1Relax $star1
	        handleDel $star1 
            } else {
                star1Del $star1    
	        handleSet $star1c.star1<$ifilter> 0x0
	    }
         }
     }

}




# copy some fields from one calib1 (source) to another one (target)
proc copy_calib1s {target source} {

     # copy PSF
     set new_psf [dgpsfNew]
     set aux_psf [handleBindFromHandle [handleNew] *$source->psf]
     dgpsfCopy $new_psf $aux_psf
     handleSetFromHandle $target->psf &$new_psf
     handleDel $aux_psf  
     handleDel $new_psf       

     # copy composite profile
     set aux_prof [handleBindFromHandle [handleNew] *$source->prof]
     compositeProfileCopy *$target->prof $aux_prof
     handleDel $aux_prof  

     # other quantities
     handleSet $target->app_correction [exprGet $source->app_correction]
     handleSet $target->app_correctionErr [exprGet $source->app_correctionErr]
     handleSet $target->psf_nstar 0

}

      
# given a PSF_BASIS and a position (row, col) return aperture correction
# n.b. it is assumed that everything is set up, if not call fake_setup
proc psfBasis2apCorr {psfBasis row col} {

   
   if [catch {set psf [KLpsf2dgpsf $psfBasis $row $col 0 0 1 0]} msg] {
       echo "KLpsf2dgpsf failed: $msg"
       set psf [dgpsfNew]
    }
    set apCor [exprGet $psf.chisq] 
    dgpsfDel $psf

    return $apCor

}

# given a PSF_BASIS and a position (row, col) return effective 
# PSF width (in arcsec)
# n.b. it is assumed that everything is set up, if not call fake_setup
proc psfBasis2PSFwidth {psfBasis row col} {
    if [catch {set psf [KLpsf2dgpsf $psfBasis $row $col 0 0 1 0]} ] {
       set psf [dgpsfNew]
    }
    set width [exprGet $psf.width] 
    dgpsfDel $psf

    return $width

}


# return a 2-Gaussian PSF fit at an arbitrary position
proc psField2dgpsf {psFieldFile filter row col} {

    # get PSF_BASIS
    if [catch {read_psfBasis psfBasis $psFieldFile $filter}] {
        declare_schematrans 5
        read_psfBasis psfBasis $psFieldFile $filter
    }
    # fit 2 Gaussians
    return [KLpsf2dgpsf $psfBasis($filter) $row $col 0 0 1 0]
}



# for faking setting up measure objects when using KLpsf2dgpsf offline
proc fake_setup {} {

     set rand [phRandomNew 100000 -seed 819396102 -type 2]
     set sblist "-1 -1 -1 -1 -1"
     set filterlist {u g r i z}; set nfilter [llength $filterlist]
     set fieldparams [fieldparamsNew $filterlist]
     loop i 0 $nfilter {
	handleSet $fieldparams.frame<$i>.nann_ap_frame 6
	handleSet $fieldparams.frame<$i>.ap_corr_run 1.0
     }

     handleSet $fieldparams.pixscale 1.0
     handleSet $fieldparams.fiber_rad 1.5
     initMeasureObj $fieldparams  1  $sblist $rand
     initProfileExtract 
     set file [envscan \$PHOTO_DIR]/lib/cellprof.dat
     initCellFitobj $file 5  
     phRandomDel $rand; unset rand

     if {1} {
       #echo " when done execute finish_fake_setup "
     } else {
       finish_fake_setup 
     }


}

proc finish_fake_setup {} {

   finiObjectFinder
   finiMeasureObj
   finiProfileExtract
   finiCellFitobj
}


proc ee {} {
global errorInfo
   echo $errorInfo
}



# aux proc for rerunning PSP in private directories
# given a run, cols, filters, starting and ending fields,
# for each directory in dirs do the following for each 
# camera column
# 1) make subdir for that column in $dir
# 2) copy psFF files for that run to $dir/$col
# 3) set parametersDir to be $dir
# 3) run PSP with output written to $dir/$col 
# If filterlist is empty, or sF or eF < 0, then they 
# are not changed for the default values
# Note that some other changes to input psPlan*par file
# can be achieved by declaring global cmdlinePlan and
# setting them to desired values before invoking this proc
proc private_PSP_run {dirs run {cols {1 2 3 4 5 6}} {sF -1} {eF -1} \
	              {filterlist {}} {ignore_FF 1} {dataroot ""}} {

global data_root
global cmdlinePlan
global openit

    if {$dataroot == ""} {set data_root /data/dp3.b/data} 

    if {$sF > 0} {
          array set cmdlinePlan [list startField $sF]
    }
    if {$eF > 0} {
          array set cmdlinePlan [list endField $eF]
    }
    if {$filterlist != {}} {
          array set cmdlinePlan [list filterlist $filterlist]
    }
    array set cmdlinePlan [list ignore_FF $ignore_FF]

    foreach dir $dirs {
        array set cmdlinePlan [list parametersDir $dir]
        foreach c $cols {
           cd $dir
	   catch {mkdir $c}
           set_run $run $c
           foreach f {u g r i z} {
              exec cp $openit(psdir)/psFF-$openit(run)-$f$c.fit $c  
           }          
           array set cmdlinePlan [list outputDir $dir/$c]
           run_ps_pipeline $openit(photo)/psPlan-$c.par
	}
    }
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

proc findAstromBias {args} {
   set const 0;

   set opts [list \
		 [list [info level 0] "Given a PSF_BASIS and a position in a frame,
 return the astrometric bias"] \
		 [list <psfBasis> STRING "" psfBasis "The PSF_BASIS describing the PSF"] \
		 [list <rowc> DOUBLE "" rowc "The row-position where the bias is wanted"] \
		 [list <colc> DOUBLE "" colc "The col-position where the bias is wanted"] \
		 [list -size INTEGER 35 size "The size of the region containing a Gaussian"] \
		 [list -sigma DOUBLE 1.0 sigma "The s.d. of the Gaussian we want debiassed"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set cen [expr 0.5*$size];		# centre of object in reg

   set reg [regNew $size $size]; regClear $reg
   handleSet $reg.row0 [expr $rowc - $cen]
   handleSet $reg.col0 [expr $colc - $cen]
   set psf [dgpsfNew -sigmax1 $sigma]
   dgpsfAddToReg -a 60000 $reg $psf $rowc $colc

   set ret [centroidAndDebias $reg $psfBasis $psf $rowc $colc]

   dgpsfDel $psf
   regDel $reg

   return [list \
	       [format "%.6f" [expr [keylget ret rowc] - $rowc]] \
	       [format "%.6f" [expr [keylget ret colc] - $colc]] \
	       ]
}

proc read_apCorrCoeffs {args} {
   set opts [list \
		 [list [info level 0] "Read an apCorrCoeff.par file, setting an array of lists of
 coefficients, indexed by filterCamCol (e.g. r1)
 The highest order is returned as the 0th element the list, so a + bx + cx^2 would be \[c b a\]"] \
		 [list <apCorrCoeffs> STRING "" _apCorrCoeffs "Name of desired array"] \
		 [list <file> STRING "" file "The file to read"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_apCorrCoeffs apCorrCoeffs

   if [catch {set apChain [param2Chain $file ""]} msg] {
      error "Cannot open $file: $msg"
   }
   #
   # Read the schema
   #
   set schema [schemaGet [chainElementGetByPos $apChain 0]]
   set decl [keylget schema "coeffs"]
   regexp {\[([0-9]+)\]} [keylget schema coeffs] {} ncoeff

   loop i 0 [chainSize $apChain] {
      set elem [chainElementGetByPos $apChain $i]

      set coeffs {}

      loop j [expr $ncoeff-1] -1 -1 {
	 lappend coeffs [exprGet $elem.coeffs<$j>]
      }
      set apCorrCoeffs([format %s%d [exprGet $elem.filter] [exprGet $elem.camcol]]) $coeffs
   }

   return ""
}

proc estimate_apCorr {args} {
   set opts [list \
		 [list [info level 0] "Given an array (as read by read_apCorr), a CCD name (e.g. z6) and
 a seeing FWHM in arcseconds, return the proper aperture correction"] \
		 [list <apCorrCoeffs> STRING "" _apCorrCoeffs "Name of desired array of coefficients"] \
		 [list <ccd> STRING "" ccd "The CCD (e.g. g2)"] \
		 [list <fwhm> DOUBLE 0.0 fwhm "The seeing's FWHM in arcseconds"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_apCorrCoeffs apCorrCoeffs


   if [catch {set coeffs $apCorrCoeffs($ccd)} msg] {
      error "Unable to lookup aperture coefficients for $ccd: $msg"
   }

   set ans 0
   foreach c $coeffs {
      set ans [expr $fwhm*$ans + $c]
   }

   return $ans
}
