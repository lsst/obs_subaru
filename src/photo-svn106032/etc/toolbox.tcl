######################################
#### TOOLS FOR HANDLING SDSS DATA ####
######################################

# N.B. Most of these tools require the 
# following setup (at Peyton Hall):
#
# setup photo 
# photo
# photo> set data_root /u/dss/data
# photo> set_display 1

# details about each command can be obtained by
# photo> info args command_name


### Content ###

# 1) Coordinate transformations
# 
#    - pixel coordinates (from a run) to RA-Dec 
#      proc pix2eq 
#      wrappers: pixlist2eq 
#
#    - given a run, where is (RA,Dec) position 
#      proc where_in_run 
#      wrappers: where_in_SDSS, where_in_SDSSs,
#      RADeclist2pix
# 
#    - given (h-m-s) coordinates, get degrees
#      and inverse: procs s2deg and deg2s


# 2) displaying images
#
#    - display a FITS image: display_fits 
# 
#    - for a given run, column, field and filter, 
#      reconstruct and display frame: show_field
#
#    - display frame with objects' ids: show_objects
#
#    - print requested data for an object: o_table 
   

# 3) producing finding charts  ######
#
#    - given a *.par file with the objects' positions, produce 
#      FITS (and optionally PS and ppm) files with finding charts 
#      proc finding_charts, see also make_chart_mosaic


# 4) Miscellaneous
#    - show a FITS file header: print_hdr 
# 
#    - make a mosaic of regions stored in fits files,
#      proc fitlist2mosaic 
#
#    - given RA and Dec in degrees, construct a string 
#      with the IAU - SDSS name for that object,
#      proc RADec2SDSSname




###### Coordinate transformations ######

# given two points on a sphere (RA1,Dec1) and (RA2,Dec2), 
# where the coordinates are expressed in degrees, return their
# distance (in arcsec for $arcsec == 1) 
proc sphericalDist {RA1 Dec1 RA2 Dec2 {arcsec 0}} {

     set d2r [expr acos(0)/90.0]

     set D1 [expr (90.0-$Dec1)*$d2r]
     set D2 [expr (90.0-$Dec2)*$d2r]
     set Dalpha [expr ($RA1-$RA2)*$d2r]
     set Daux [expr cos($D1)*cos($D2)+sin($D1)*sin($D2)*cos($Dalpha)]
     set dist [expr acos($Daux)/$d2r] 
  
     if {$arcsec} {
        return [expr 3600.0*$dist]
     } else {
        return $dist
     }
}


# pixel coordinates (from a run) to RA-Dec 
proc pix2eq {run camcol field row col {filter r} {rootdir ""} \
	     {rerun ""} {mjd 0} {asTransFile ""}} {

global data_root openit

    # does user want to override global data_root?
    if {$rootdir != ""} {
        set data_root $rootdir
    }
    # make sure data dir is kosher
    if {![file exist $data_root]} {
        set data_root [get_data_root ""]
    }
    if {![file exist $data_root]} {
        error "Specified data_root does NOT exist: $data_root"
    }
      
    set runstr [format %06d $run]

    if {$asTransFile == ""} {  
       if {$rerun == "" || $rerun < 0} {
          set rerun [get_highest_dir $data_root/$run]
       } 
       if {$rerun < 0} {
          set_run $run $camcol -rerun ""
       } else {
          set_run $run $camcol -rerun $rerun
       }
       set file $openit(asdir)/asTrans-$runstr.fit
       if {![file exist $file]} {
           set file /u/dss/data/asTrans/asTrans-$runstr.fit
       }
    } else {
       set file $asTransFile
    }

    #  read header data
    set hdr [hdrReadAsFits [hdrNew] $file]
    set node [hdrGetAsDbl $hdr NODE]
    set incl [hdrGetAsDbl $hdr INCL]
    set camcols [hdrGetAsAscii $hdr CAMCOLS]
    set filters [hdrGetAsAscii $hdr FILTERS]
    set ast_nfilters [llength $filters]  
    set run [hdrGetAsInt $hdr RUN]
    set field0 [hdrGetAsInt $hdr FIELD0]
    set nfields [hdrGetAsInt $hdr NFIELDS]
    if {$field < 0} {
         set field $field0
         set deflt 1
    } else {
         set deflt 0
    }
       
    # determine needed positions
    set icol [lsearch $camcols $camcol] 
    set ifilter [lsearch $filters $filter] 
    set ifield [expr $field - $field0]

    # read proper TRANS
    set hdu [expr 1 + $icol*$ast_nfilters]
    set TRANSchain [fits2Schema $file TRANS $hdr -hdu $hdu] 
    set TRANS [chainElementGetByPos $TRANSchain $ifield]
    if {$mjd} {
       set T0 [exprGet $TRANS.mjd]
       if {!$deflt} {
           # this is not default, add correction to MJD
           set T0 [expr $T0 + (3.0471e-07 * $row)]
       }
       set TRANSend [chainElementGetByPos $TRANSchain [expr [chainSize $TRANSchain]-1]]
       set Tend [exprGet $TRANSend.mjd]
    }
 
    # convert pixel coordinates to RA/Dec
    # first get mu/nu
    set gc [transApply $TRANS $filter $row 0 $col 0]
    set mu [keylget gc mu]
    set nu [keylget gc nu]
    # mu/nu -> RA/Dec
    set eq [GCToEq $mu $nu -node $node -inclination $incl]

    set RA [format "%10.6f" [keylget eq ra]]
    set Dec [format "%10.6f" [keylget eq dec]]

    # clean
    hdrDel $hdr
    genericChainDestroy $TRANSchain

    if {$mjd} {
        set T0 [format "%14.6f" $T0]    
        set Tend [format "%14.6f" $Tend]    
        set nodestr  [format "%10.6f" $node]    
        set inclstr  [format "%10.6f" $incl] 
        set munu [eqToGC [keylget eq ra] [keylget eq dec] \
                         -node $node -inclination $incl]
        set mustr [format "%10.6f" [keylget munu mu]]  
        set nustr [format "%10.6f" [keylget munu nu]]  
        return "$RA $Dec $T0 $Tend $nodestr $inclstr $mustr $nustr"
    } else {
        return "$RA $Dec"
    }

}


# given a list of runs, print the MJD and (RA, Dec) for the first field
proc getMJD {runs {data_root ""} {outfile ""}} {

     if {$data_root == ""} {set data_root [get_data_root ""]}
     set str "     run      RA         Dec        MJDstart       MJDend         node  inclination     mu0       nu0"
     echo $str
     if {$outfile != ""} {
         set outf [open $outfile w]
         puts $outf $str
     } 
     
     foreach run $runs {
         set runstr [format "%6d" $run]
         if {![catch {set str "  $runstr  [pix2eq $run 1 -1 0 0 r ${data_root} "" 1]"}]} {
             echo $str
             if {$outfile != ""} {
                 puts $outf $str
             }
         }
     }
 
     if {$outfile != ""} {
         close $outf
     } 
}


# analogous to getMJD but enforces a given rerun (the input list is {run rerun})
proc getMJD2 {rrList {data_root ""} {outfile ""}} {

     if {$data_root == ""} {set data_root [get_data_root ""]}
     set str "     run      RA         Dec        MJDstart       MJDend         node  inclination     mu0       nu0"
     echo $str
     if {$outfile != ""} {
         set outf [open $outfile w]
         puts $outf $str
     } 
     
     foreach rr $rrList {
         set run [lindex $rr 0]
         set rerun [lindex $rr 1]
         set runstr [format "%6d" $run]
         if {![catch {set str "  $runstr  [pix2eq $run 1 -1 0 0 r ${data_root} $rerun 1]"}]} {
             echo $str
             if {$outfile != ""} {
                 puts $outf $str
             }
         }
     }
 
     if {$outfile != ""} {
         close $outf
     } 
}


# take a file full of listings "run camcol field filter row col"
# and dump (RA Dec) to fileout
proc pixlist2eq {filein fileout} {

     set infile [open $filein r]
     set outf [open $fileout w]

     while {![eof $infile]} {
         set line [gets $infile]
         # process line
         if {![eof $infile] && [lindex $line 0] != "#"} {  
            set run [lindex $line 0]
            set camcol [lindex $line 1]
            set field [lindex $line 2]
            set filter [lindex $line 3]
            set row  [lindex $line 4]
            set col [lindex $line 5]
            puts $outf [pix2eq $run $camcol $field $row $col $filter]    
         }
     }
 
     close $outf
     close $infile
    
}


# take a file with "run RA Dec" information, get 
# run - col - field - row - col, and dump to fileout
# for outType = 0 dump only
# run - col - field - row - col
# for outType = 1 dump 
# RA - Dec - run - col - field - row - col
# and for outType = 2 dump 
# orig. line - run - col - field - row - col
# the "run RA Dec" information is given in columns given 
# by (zero-indexed) runCol, rerunCol, RACol, and DecCol
# if run or rerun info is missing, set runCol to -run
# or rerunCol to -rerun
proc eqlist2pixel {filein runCol rerunCol  RACol DecCol outType fileout} {
 
global data_root openit

     set infile [open $filein r]
     set outf [open $fileout w]

     while {![eof $infile]} {
       set line [gets $infile]
       # process line
       if {![eof $infile]} {  
          if {[lindex $line 0] == "#"} {  
             if {$outType == 2} { 
                 puts $outf "$line"
             }              
	  } else {
             if {$runCol < 0} {
                 set run [expr -1*$runCol]
             } else {
                 set run [lindex $line $runCol]
             }
             if {$rerunCol < 0} {
                 set rerun [expr -1*$rerunCol]
             } else {
                 set rerun [lindex $line $rerunCol]
             }          
             set RA [lindex $line $RACol]
             set Dec [lindex $line $DecCol] 
             set pix [where_in_run $run $RA $Dec -rerun $rerun]
	     if {$outType == 0} {
                 puts $outf $pix
	     } elseif {$outType == 1} {
                 puts $outf "$RA $Dec $pix"
	     } elseif {$outType == 2} {
                 puts $outf "$line $pix"
	     } else {
                 error "dunno outType == $outType"
             } 
	  }
       }
     }
 
     close $outf
     close $infile
    
}


# a convenient wrapper around pix2eq to transform pixel coordinate from 
# run 1 to pixels coordinates in run 2 
proc pix2pix {run1 camcol1 field1 row1 col1 run2 {filter r} {rootdir ""}} {
 
  set RADec [pix2eq $run1 $camcol1 $field1 $row1 $col1 $filter $rootdir]
  return "[where_in_run $run2 [lindex $RADec 0] [lindex $RADec 1]]" 
 
} 
 


#
# modeled after read_asTrans
#
proc where_in_run {args} {
   global data_root openit

   set display_object 0;		# display the object?
   set find_object 0;			# find the object?

   set opts [list \
		 [list [info level 0] \
 "for a given run and RA & Dec return run, column, field number and
 pixel coordinates, or -1 if not found

 Ra and Dec are in decimal degrees or traditional 10:23:45.60 (hours)
 and -00:01:23.34 (degrees) respectively"] \
		 {<run> INTEGER 0 run "desired run"} \
		 {<alpha> STRING 0.0 RA "Object's RA"} \
		 {<delta> STRING 0.0 Dec "Object's Dec"} \
		 [list {[rootdir]} STRING $data_root rootdir \
		      "Root for data; overrides \$data_root"] \
		 [list -root STRING $data_root rootdir \
		      "Root for data; overrides \$data_root"] \
		 {-camCol INTEGER 0 camCol "Only search this column"} \
		 {-rerun STRING "" rerun "rerun number, the highest by default"} \
		 {-asTrans STRING "" asTrans \
		      "Use this asTrans file"} \
		 {-display CONSTANT 1 display_object \
		      "Run mtv_objc_list on matched object"} \
		 {-find CONSTANT 1 find_object \
		      "Find the nearest object to the matched position"} \
	     ]
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set_run -root $rootdir -rerun $rerun $run $camCol

   #
   # Convert :: forms
   #
   if [regexp {^([0-9]+):([0-9]+):(.*)} $RA foo hr min sec] {
      foreach el "hr min sec" {
	 regsub {^0*} [set $el] "" $el
      }
      set RA [expr 15*($hr + ($min + $sec/60.0)/60.0)]
   }
   if [regexp {^([+-])?([0-9]+):([0-9]+):(.*)} $Dec foo sgn hr min sec] {
      foreach el "hr min sec" {
	 regsub {^0*} [set $el] "" $el
      }
      set Dec [expr $hr + ($min + $sec/60.0)/60.0]
      if {$sgn == "-"} {
	 set Dec [expr -$Dec]
      }
   }

    if {$asTrans == ""} {
        set file $openit(asdir)/asTrans-$openit(run).fit
        if ![file exist $file] {
	   set file $data_root/asTrans/asTrans-$openit(run).fit
	}
    } else {
        set file $asTrans
    }

    #READ PDU HEADER DATA
    set hdr [hdrReadAsFits [hdrNew] $file]
    set node [hdrGetAsDbl $hdr NODE]
    set incl [hdrGetAsDbl $hdr INCL]
    set camcols [hdrGetAsAscii $hdr CAMCOLS]
    set nfields [hdrGetAsInt $hdr NFIELDS]
    set filters [hdrGetAsAscii $hdr FILTERS]
    set ast_nfilters [llength $filters]  
    set run [hdrGetAsInt $hdr RUN]
    set field0 [hdrGetAsInt $hdr FIELD0]
    hdrDel $hdr
    
    set dhdu [llength filters]

    # transform coordinates to GC 
    catch [set GCpos [eqToGC $RA $Dec -node $node -inc $incl ]]
    set MU [keylget GCpos mu]
    set NU [keylget GCpos nu]

    # search each column
    foreach camcol $camcols {
       if {$camCol > 0 && $camCol != $camcol} {
	  continue
       }
        set icol [lsearch $camcols $camcol] 
        # taking ifilter=0
        set hdu [expr 1 + $icol*$ast_nfilters]
        if [info exists hdu_chain] {
            chainDestroy $hdu_chain genericDel
        }
        set hdu_chain [fits2Schema $file TRANS -hdu $hdu]        
        set first_field [chainElementGetByPos $hdu_chain HEAD]
        set last_field [chainElementGetByPos $hdu_chain TAIL]
        foreach var {a b c d e f} {
            set ${var}_0 [exprGet $first_field.$var]
	    set ${var}_f [exprGet $last_field.$var]
        }
       
        # nu range
        set nu_BL $d_0
        set nu_BR [expr $d_0 + $f_0*2048]
        set nu_TL [expr $d_f + $e_f*1361] 
        set nu_TR [expr $d_f + $e_f*1361 + $f_f*2048]
        set nu_min [min $nu_BL $nu_TL]
        set nu_max [max $nu_BR $nu_TR]
        # if outside nu range go to next column
        if {[expr ($NU - $nu_min)*($NU - $nu_max)] > 0} {continue}
        # nu is OK, check mu
        set mu_BL $a_0
        set mu_BR [expr $a_0 + $c_0*2048]
        set mu_TL [expr $a_f + $b_f*1361] 
        set mu_TR [expr $a_f + $b_f*1361 + $c_f*2048]
        set mu_min [min $mu_BL $mu_BR]
        set mu_max [max $mu_TL $mu_TR]

        if {$mu_min > $mu_max} {
	   set mu_max [expr $mu_max + 360]
	}
        if {$mu_min > 360.0 && $mu_max > 360.0} {
	    set mu_min [expr $mu_min - 360]
	    set mu_max [expr $mu_max - 360]
	}
        if {$mu_min > 360.0 || $mu_max > 360.0} {
            if {$MU < $mu_min} { 
	       set MU [expr $MU + 360]
	    }
	}

        # if outside mu range go to next column
        if {[expr ($MU - $mu_min)*($MU - $mu_max)] > 0} {continue}
        # inside this run, find the field 
        set delta [expr ($mu_max - $mu_min)/($nfields - 1)]
        set index [expr int(($MU - $mu_min)/$delta)] 

        if {$index < 0 || $index >= [chainSize $hdu_chain]} {continue}
        set this_field [chainElementGetByPos $hdu_chain $index]
        foreach var {a b c d e f} {
            set $var [exprGet $this_field.$var]
	}   
        # calculate position on the frame
        if {$MU > 360} {
	   set MU [expr $MU - 360]
	}
        set num [expr $e*($MU-$a)-$b*($NU-$d)]
        set denum [expr $c*$e-$b*$f]

        if {$denum == 0} {
	   continue;
	}
        set col [expr $num / $denum]
        set row [expr ($MU - $a - $c*$col)/$b]
        set col [expr int($col)]
        set row [expr int($row)]
        # did we narrowly miss it?
        if {$col < 0 || $col > 2048} {continue}
        if {$row < 0 && $row > -1362 && $index > 0} {
           incr index -1
           set row [expr $row + 1362]
        }
        if {$row > 1360 && $row < 2722 && $index < [chainSize $hdu_chain]} {
           incr index 
           set row [expr $row - 1361]
        }   
        if {$row < 0 || $row > 1361} {continue}      
        # found it!
        # now properly call transInverseApply 
        set fp [fieldparamsNew r]
        handleSetFromHandle $fp.frame<0>.toGCC &$this_field
        set pixel_coord [transInverseApply *$fp.frame<0>.toGCC r $MU 0 $NU 0]
        handleSet $fp.frame<0>.toGCC 0
        fieldparamsDel $fp
	set conv_row [keylget pixel_coord row]
	set conv_col [keylget pixel_coord col]
        if {$conv_row > 1361} {
	    set conv_row [expr $conv_row - 1361]
        }
        if {$conv_row < 0} {
	    set conv_row [expr $conv_row + 1361]
            incr index -1                       
        }
        set ret_list "$run $camcol [expr $index + $field0] \
            [format "%4d" [expr int($conv_row)]] \
            [format "%4d" [expr int($conv_col)]]"
        chainDestroy $hdu_chain genericDel

        if $find_object {
	   global table verbose
	   
	   if ![info exists verbose] { set verbose0 0 }
	   if [catch {
	      set_run -root $rootdir -rerun $rerun $run $camcol \
		  -field [lindex $ret_list 2]

	      set verbose 1
	      set id [find_objc table $conv_row,$conv_col]
	      if [info exists verbose0] { set verbose $verbose0 }
	      if $display_object {
		 uplevel \#0 [list mtv_objc_list -id "$id"]
	      }
	   } msg] {
	      if [info exists verbose0] { set verbose $verbose0 }
	      echo "Failed to display object: $msg"
	   }
	}
        return $ret_list
      
    }  
        
    if [info exists hdu_chain] {
        chainDestroy $hdu_chain genericDel
    }

    # did not find it
    return -1

}


# given a dir, return the highest numbered subdir
# n.b. can be used to search for latest run, or rerun
# here dir is assumed to be $data_root/$run
proc get_highest_dir {dir {rerunMax 599} {tsObj 0}} {
    
   set rerun -1

      # get a list of all reruns from the given rerun that have tsObj files
      set list ""
      if {$tsObj} {
          catch {set list [glob $dir/*/calibChunks/6]}

      } else {
          catch {set list [glob $dir/*]}
      }
      if {[llength $list] > 0} {
         set rrList ""
         foreach d $list {
            if {$tsObj} {
               set d [file dirname $d]
               set d [file dirname $d]
            }
            set rr [file tail $d]
	    if {$rr < $rerunMax} {
               set rrList "$rrList $rr"
            }
	 }
         # sort rerun list
         set rrList [lsort -integer -decreasing $rrList] 
         set rerun [lindex $rrList 0]
      }
 
   return $rerun
}

# given a dir, return the list of all subdirectories that are 
# pure numbers
proc get_all_runs {dir} {
    
     set list [glob $dir/*]
     set runs {}
     foreach d $list {
         if {![catch {set rr [format "%06d" [file tail $d]]}]} {
            lappend runs $rr
         }    
     } 

   set runs [lsort $runs]

   set out {}
   foreach r $runs { 
       lappend out [string trimleft $r 0]
   }

   return $out

}


# take a file full of listings "run RA Dec"
# and dump (RA Dec run camcol field row col) to fileout
proc RADeclist2pix {filein fileout} {

     set infile [open $filein r]
     set outf [open $fileout w]

     while {![eof $infile]} {
         set line [gets $infile]
         # process line
         if {![eof $infile] && [lindex $line 0] != "#"} {  
            set run [lindex $line 0]
            set RA [lindex $line 1]
            set Dec [lindex $line 2]
            puts $outf "$RA $Dec $run [where_in_run $run $RA $Dec]"    
         }
     }
 
     close $outf
     close $infile
    
}



# take a file with RA Dec listed in 1-indexed columns
# colRA and colDec, and dump all runs for each source
# NB designed to be used with getSDSSphotometry (opdb tool)
proc RADec2SDSSruns {filein colRA colDec fileout} {

     set infile [open $filein r]
     set outf [open $fileout w]
     set outf2 [open ${fileout}NOTFOUND w]

     set Nin 0; set Nnotfound 0; set Nok 0; set Nepoch 0
     while {![eof $infile]} {
         set line [gets $infile]
         # process line
         if {![eof $infile] && [lindex $line 0] != "#"} {  
            incr Nin 
            echo "working on source $Nin..."
	    set RA  [lindex $line [expr $colRA-1]]
            set Dec [lindex $line [expr $colDec-1]]
	    set list [where_in_SDSS $RA $Dec -quiet]
            set line ""
	    foreach el $list {
	       foreach rec $el {
                  set line "$line $rec"
               }
	    }
	    if {[llength $line] > 0} {
               puts $outf "$line"
   	       set Nepoch [expr $Nepoch + [llength $list]]
               incr Nok
            } else {
               puts $outf2 "$RA $Dec NOT FOUND in SDSS"    
               incr Nnotfound
            }
         }
     }
 
     close $outf
     close $outf2
     close $infile
   echo; echo "---------------------------------------"
   echo Processed $Nin positions from file $filein, 
   echo Found $Nepoch observations for $Nok sources
   echo Did not find SDSS data for $Nnotfound positions
   echo "---------------------------------------"
    
}





## given an output file from RADec2SDSSruns (in photo),
## dump a table with photometry for all entries
proc getSDSSphotometry {filein fileout {Dpix 2} {tableType 0} {rerun ""}} {  

     set infile [open $filein r]

     set Nin 0
     while {![eof $infile]} {
         set list [gets $infile]
         # process line
         if {![eof $infile] && [lindex $list 0] != "#"} {  
            incr Nin 
            echo "working on source $Nin..."
            set Nepochs [expr [llength $list]/5]
            loop i 0 $Nepochs {
               set run [lindex $list [expr 5*$i+0]]
               set camCol [lindex $list [expr 5*$i+1]]
               set field [lindex $list [expr 5*$i+2]]
               set row [lindex $list [expr 5*$i+3]]
               set col [lindex $list [expr 5*$i+4]]
               echo "     epoch: $run $camCol $field $row $col"
               set prefix "  $Nin  $Nepochs  $i"  
               catch {
                  getObjects $run $rerun $camCol $field $field $Dpix $row $col \
                      $tableType $fileout $prefix
  	       }
            }
         }
     }
 
     close $infile

   echo; echo "---------------------------------------"
   echo Processed $Nin positions from file $filein, 
   echo and dumped SDSS data to file $fileout
   echo "---------------------------------------"


}



# this proc prints a table with basic info for all objects from 
# (run rerun camCol field1-field2) that are closer than Dpix pixels from 
# the specified (row,col) position. If filename != "" the same 
# table is dumped to the specified file
# for tableType = 0 print only run number and psf photometry
# e.g. 
# photo> getObjects 745 671 1 114 114 5 100 100 
proc getObjects {run rerun camCol field1 field2 Dpix row col \
		     {tableType 0} {filename ""} {prefix ""}} {   
  
global verbose 

   
     set filterlist {u g r i z}

     set rowMin [expr $row-$Dpix]
     set rowMax [expr $row+$Dpix]
     set colMin [expr $col-$Dpix]
     set colMax [expr $col+$Dpix]

     if {$filename != ""} {
          set dump 1
          set outf [open $filename "a"]
     } else {
          set dump 0; set outf ""
     }
      

     ### this is where the data are read from opdb ###
     set opdb [get_data_column $run $rerun $camCol [list $field1 $field2] \
                    30 0 "BINNED1" "BRIGHT"]
        
     ## get stars and galaxies 
     set stars    [lindex $opdb 1]
     set galaxies [lindex $opdb 2]

     ## select nearby objects
     set Nstars [chainSearch $stars "{objc_rowc.val > $rowMin} {objc_rowc.val < $rowMax} \
                                     {objc_colc.val > $colMin} {objc_colc.val < $colMax}"]
     set Ngalaxies [chainSearch $galaxies "{objc_rowc.val > $rowMin} {objc_rowc.val < $rowMax} \
                                     {objc_colc.val > $colMin} {objc_colc.val < $colMax}"]

     ## make tables 
     set Nfound [expr [chainSize $Nstars]+[chainSize $Ngalaxies]]





     if {$Nfound > 0} {
       foreach chain "$Nstars $Ngalaxies" {
         loop i 0 [chainSize $chain] {
           set el [chainElementGetByPos $chain $i]
           set line [print_one_photo $el $tableType 0 $outf]
           if {$filename != ""} {
               puts $outf $line
           } else {
               echo $line
           }
         }
       }
     } else {
       set strfld [format "%4.0f" $field1]
       set line "     0.000000   0.000000   0.000000   $run   $camCol 0 $strfld   -1    0.0    0.0 psf:  0.00 0.00  0.00 0.00  0.00 0.00  0.00 0.00  0.00 0.00 0  UNK"
       if {$filename != ""} {
           puts $outf "$prefix $line"
       } else {
           echo $line
       }
     }

     # clean
     chainDel $Nstars; chainDel $Ngalaxies
     loop k 0 5 {             
         taGenericChainDestroy [lindex $opdb $k]
     }
     if {$filename != ""} {close $outf} 


   return 0

}






# search available imaging runs for a particular position
# it is assumed that all runs are in $data_root in directories
# named after the run number and that asTrans file is in subdir
# named astrom. RA and Dec need to be given in degrees. 
# if flag != 0 return a list of all {run field row col} and do
# not echo anything 
proc where_in_SDSS {args} {
   global data_root

   set quiet 0;
   set other_strip 0

   set opts [list \ [list [info level 0] "\
 Search available imaging runs for a particular position it is
 assumed that all runs are in \$data_root in directories named after
 the run number and that asTrans file is in subdir named astrom.

 if -quiet return a list of all {run field row col} and don't echo anything"] \
		 [list <ra> STRING "" RA \
		      "Right Ascension; decimal degrees or hh:mm:ss.ss"] \
		 [list <dec> STRING "" Dec \
		      "Declination; decimal degrees or \[+-\]dd:mm:ss.ss"] \
		 [list -otherStrips CONSTANT 1 other_strip \
		      "Return the runs that make up the other strip of the stripe including <ra> <dec>"] \
		 [list -run INTEGER 0 initialRun \
		      "Look for other other strips of stripe including this run"] \
		 [list -quiet CONSTANT 1 quiet "\
 return a list of all {run field row col} and don't echo anything"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set ret_list {}
   
   if {$initialRun == 0} {
      set runs [glob $data_root/\[1-9\]*]
   } else {
      set runs $initialRun
   }
   
   foreach run $runs {
      set runNo [file tail $run]
      
      if ![regexp {^[0-9]+$} $runNo] {	
	 continue;			# e.g. 2d_v4_7
      }
      
      if {[catch {set match [where_in_run $runNo $RA $Dec]}] || $match == -1} {
	 continue;
      }

      if {!$quiet} {
	 if {$ret_list == ""} {
	    echo "   run col field  row  col"
	 }
	 set camCol [lindex $match 1]
	 set field [lindex $match 2]
	 set row  [expr int([lindex $match 3])] 
	 set col  [expr int([lindex $match 4])]
	 echo [format "%6d %3d %5d %4d %4d" $runNo $camCol  $field   $row $col]
      }

      lappend ret_list $match        
   }
   #
   # Did they want the other strips too?
   #
   if {$ret_list != "" && $other_strip} {
      global fieldparams table

      if {$initialRun == 0} {
	 set run [lindex [lindex $ret_list 0] 0]
      }

      set_run $run $camCol $field
      
      if {$col <= 1024} {
	 set left 1;			# look for camCol to left
	 set col [expr 2048/2 - 2048]
      } else {
	 set left 0;			# look for camCol to right
	 set col [expr 2048/2 + 2048]
      }
      set band 2; set filter r
      set gc [transApply *$fieldparams.frame<$band>.toGCC $filter \
		  [expr 1361/2] 0 $col 0]
      
      set eq [GCToEq [keylget gc mu] [keylget gc nu] \
		  -node [keylget table node] -incl [keylget table incl]]
      set ra [keylget eq ra]; set dec [keylget eq dec]

      if $quiet {
	 set qflag "-quiet"
      } else {
	 set qflag ""
	 echo "Other strips in stripe:"
      }
      set ret_list [eval where_in_SDSS $ra $dec $qflag]
   }
   
   if {$ret_list == "" && !$quiet} { 
      echo "This position has not yet been observed."
   }

   return $ret_list
}

# like where_in_SDSS but it takes h-m-s coordinates
proc where_in_SDSSs {RAh RAm RAs DecD DecM DecS {flag 0}} {

       set RA [lindex [s2deg $RAh $RAm $RAs $DecD $DecM $DecS] 0]
       set Dec [lindex [s2deg $RAh $RAm $RAs $DecD $DecM $DecS] 1]
       whereinSDSS $RA $Dec $flag

}



# take h-m-s coordinates and return RA and Dec in degrees
proc s2deg {RAh RAm RAs DecD DecM DecS} {

     set RAdeg  [format %12.5f [expr ($RAh + $RAm/60.0 + $RAs/3600.0)*15.0]]
     if {$DecD < 0.0 || [string range $DecD 0 0] == "-"} {
        set sign -1
        set DecD [expr $sign * $DecD]
     } else {
        set sign 1
     }
     set Decdeg  [format %12.5f [expr $sign*($DecD + $DecM/60.0 + $DecS/3600.0)]]

     return "$RAdeg $Decdeg"
}

# take coordinates in degrees and return h-m-s coordinates
proc deg2s {RA Dec {format 0}} {

     set RAh [expr int($RA/15)]
     set RAm [expr int(($RA/15 - $RAh)*60)]
     set RAs [format "%5.2f" [expr (($RA/15 - $RAh)*60 - $RAm)*60]]

     if {$Dec < 0} {
         set sign "-"
     } else {
         if {$format} {
             set sign "+"
         } else {
             set sign " "
         }
     }
     set DD [expr int($Dec)]
     set Dm [expr int(abs($Dec - $DD)*60+0.00167)]
     set Ds [format "%4.1f" [expr abs(abs($Dec - $DD)*60 - $Dm)*60]]
     set DD [expr abs($DD)]

     if {$format} {
         if {$RAh < 10} {set RAh "0$RAh"}
         if {$RAm < 10} {set RAm "0$RAm"}
         if {$RAs < 10} {
             set RAs "0[format %3.1f $RAs]"
         } else {
             set RAs "[format %4.1f $RAs]"
         }
         if {$DD < 10} {set DD "0$DD"}
         if {$Dm < 10} {set Dm "0$Dm"}
         if {$Ds < 10} {
             set Ds "0[format %3.1f $Ds]"
         } else {
             set Ds "[format %4.1f $Ds]"
         }
         return " $RAh:$RAm:$RAs  $sign$DD:$Dm:$Ds"
     } else {
         return " $RAh $RAm $RAs  $sign$DD $Dm $Ds"
     }
}


# given (l,b) return (vRA,vDec) for two cases
# vL=1, vB=0, and vL=0, vB=1
proc vGal2vEq {l b} {
     
    set EQ1 [galToEq $l $b]
    set ra1 [keylget EQ1 ra] 
    set dec1 [keylget EQ1 dec]

    set EQ [galToEq [expr $l+0.1] $b]
    set ra [keylget EQ ra] 
    set dec [keylget EQ dec]
    set vRA [expr 10*($ra-$ra1)]
    set vDec [expr 10*($dec-$dec1)]
    echo "for vL=1, vB=0: vRA=$vRA, vDec=$vDec"

    
    set EQ [galToEq $l [expr $b+0.1]]
    set ra [keylget EQ ra] 
    set dec [keylget EQ dec]
    set vRA [expr 10*($ra-$ra1)]
    set vDec [expr 10*($dec-$dec1)]
    echo "for vL=0, vB=1: vRA=$vRA, vDec=$vDec"

}



# given (RA,Dec) and (vRA,vDec) return
# (l, b) and (vL, vB)
proc vEq2vGal {RA Dec vRA vDec} {
     
    set Gal [eqToGal $RA $Dec] 
    set L [format "%12.5f" [keylget Gal gLong]] 
    set B [format "%12.5f" [keylget Gal gLat]]

    # assuming that vEq ~ 1-10, change in coord is 2-20 arcmin
    set RA2 [expr $RA+$vRA*100.0/3600]
    set Dec2  [expr $Dec+$vDec*100.0/3600]
    set Gal2 [eqToGal $RA2 $Dec2] 
    set L2 [keylget Gal2 gLong] 
    set B2 [keylget Gal2 gLat]

    set vL [format "%6.2f" [expr 3600.0/100*($L2-$L)]]
    set vB [format "%6.2f" [expr 3600.0/100*($B2-$B)]]
 
    return "$L $B $vL $vB"

}


# given two vectors (RA,Dec) return (l, b)
proc RADec2lb {RA Dec} {

    assert { [exprGet $RA.dimen]  == [exprGet $Dec.dimen] }
    set L [vectorExprEval 0*$RA]
    set B [vectorExprEval 0*$RA]
    
    loop i 0 [exprGet $RA.dimen] {
       set raP  [exprGet $RA.vec<$i>]      
       set decP [exprGet $Dec.vec<$i>]      
       set Gal [eqToGal $raP $decP]
       handleSet $L.vec<$i> [keylget Gal gLong] 
       handleSet $B.vec<$i> [keylget Gal gLat] 
    }

    return "$L $B"

}



# given a file with RA and Dec data, dump all available 
# run-camCol-field-row-col for each position to outfile 
# RA/Dec can be given as decimal degrees (flag=0), or as
# RAh:RAm:RAs DD:Dm:Ds (flag=1), or as 
# RAh RAm RAs DD Dm Ds (flag=2)
# n.b. similar to RADeclist2pix except that asTrans files in a
# single directory are searched and all matches are dumped
# if asTrans files are not in data_root/asTrans set variable
# asTransDir to correct path
proc allSDSSobsList {RADecFile colRA colDec outfile {colName -1} {flag 0}} {

global data_root asTransDir

     if {![info exists data_root]} {  
         sdr;
     }

     if {![info exists asTransDir]} {  
         set asTransDir $data_root/asTrans   
     }
    
     set infile [open $RADecFile r]
     set outf [open $outfile w]
     set string ""; set N 0; set Ngood 0

     while {![eof $infile]} {
         set line [gets $infile]
         # process line
         if {![eof $infile] && [lindex $line 0] != "#"} { 
            incr N 
            echo "source $N ..."
	    if {$flag == 0 || $flag == 1} { 
               set RA [lindex $line [expr $colRA-1]]
               set Dec [lindex $line [expr $colDec-1]]
	       if {$flag == 1} { 
                  set string "$RA $Dec "  
                  set RADec [parseRADec $RA $Dec]
                  set RA [lindex $RADec 0]  
                  set Dec [lindex $RADec 1]
               }
            } elseif {$flag == 2} {
               set RA "[lindex $line [expr $colRA-1]]"
	       set RA "${RA}:[lindex $line $colRA]"
               set RA "${RA}:[lindex $line [expr $colRA+1]]"
               set Dec "[lindex $line [expr $colDec-1]]"
	       set Dec "${Dec}:[lindex $line $colDec]"
	       set Dec "${Dec}:[lindex $line [expr $colDec+1]]"
               # echo parsing parseRADec $RA $Dec
               set RADec [parseRADec $RA $Dec]
               set RA [lindex $RADec 0]  
               set Dec [lindex $RADec 1]               
            } else {
               error "allSDSSobsList flag = $flag !?! (should be 0-2)"
            }         
	    if {$colName > 0} {
               set string "[lindex $line [expr $colName-1]] $string"
            }
            # echo calling  where_in_run_asTrans $asTransDir $RA $Dec $outf $string  
            set Nobs [where_in_run_asTrans $asTransDir $RA $Dec $outf $string]   
            if {$Nobs > 0} {incr Ngood} 
         }
     }
 
     close $outf
     close $infile

  return "$N $Ngood"
    
}
  

# given a directory full of asTrans files, search each one
# for pixel coordinates corresponding to RA and Dec, if found
# dump (Nobs run camCol field row col) to outfile, where 
# Nobs = 1,... is the observation number
proc where_in_run_asTrans {asTransDir RA Dec outfile {string ""}} {
   
     set runs [glob $asTransDir/asTrans*fit] 
     set Nobs 0
     set RAstr [format %10.5f $RA]
     set Decstr [format %9.5f $Dec]
     set matched 0

     foreach run $runs {
        set runNo [string trimleft [string range [file tail $run] 8 13] 0]
        if [catch {set match [where_in_run $runNo $RA $Dec -asTrans $run]}] {set match -1}  
        set r [format %6d $runNo]
        if {$match != -1} { 
           set camCol [format %2d [lindex $match 1]]
           set field [format %4d [lindex $match 2]]
           set row  [format %4d [expr int([lindex $match 3])]] 
           set col  [format %4d [expr int([lindex $match 4])]]
           incr Nobs   
           set matched 1             
           puts $outfile "$string $RAstr $Decstr [format %2d $Nobs] $r $camCol  $field   $row $col"
        }
     }
     if {!$matched} {
         puts $outfile "$string $RAstr $Decstr  0      0  0     0      0    0"
     }

    flush $outfile

  return $Nobs   

}





###### Displaying images ######

# display a region stored in a fits file

proc display_fits {file} {

   set reg [regReadAsFits [regNew] $file]
   display $reg 
   regDel $reg

}

# for a given run, column, field and filter, reconstruct and return frame
proc get_field {run column field {filter r} {rootdir ""} {rerun ""} {tsObj 0}} {

global data_root table openit

    # does user want to override global data_root?
    if {$rootdir != ""} {
        set data_root $rootdir
    }
    # make sure data dir is kosher
    if {![info exist data_root]} {
        error "You must declare AND set global variable data_root!"
    } elseif {![file exist $data_root]} {
        error "Specified data_root does NOT exist: $data_root"
    }

    if {$rerun == ""} {
        set rerun [get_highest_dir $data_root/$run]
        if {$rerun < 0} {
            error "Directory $data_root/$run doesn't contain any reruns!?"
        }  
    }   
    set_run $run $column -rerun $rerun
 

    # need to check whether fpObj files exist, if not use tsObj
    set runstr [format %06d $run]
    set fieldstr [format "%04d" $field]  
    set fpObjc fpObjc-$runstr-$column-$fieldstr.fit
    set objdir $openit(objdir)
    if {![file exists $objdir/$fpObjc]} {
         set tsObj 1
    } 
  
    # we want tsObj files
    if {$tsObj} {
       if {$rerun == ""} {
           set_run $run $column -tsObj
       } else {
           set_run $run $column -rerun $rerun -tsObj
       }
    }
 
    # if fpBIN files are unavailable, settle for an image w/o sky
    if {[catch {
         set frame [recon $filter $field rm cm 0 binned]
         objfileClose table
       }]} {
         set frame [recon $filter $field rm cm 0 ""]
         objfileClose table
    }

    return $frame
}


# for a given run, column, field and filter, reconstruct and display frame
proc show_field {run column field {filter r} {rootdir ""}} {

global data_root

    # does user want to override global data_root?
    if {$rootdir != ""} {
        set data_root $rootdir
    }
    # make sure data dir is kosher
    if {![info exist data_root]} {
        error "You must declare AND set global variable data_root!"
    } elseif {![file exist $data_root]} {
        error "Specified data_root does NOT exist: $data_root"
    }
 
    set frame [get_field $run $column $field $filter $data_root]
    display $frame "$run-$column-$field-$filter"

    regDel $frame
}



# for a given run, column, field and filter, reconstruct and display frame
proc show_objects {run column frame {filter r} {mask 0}} {
   
global table field

     set field $frame
     set_run $run $column -tsObj
     next_field -mask $mask -field $frame -filter $filter
     echo "Objects for $run-$column-$field-$filter"
     echo "The color coding for masks is:"
     echo "    INTERP              green"
     echo "    SATUR               purple"
     echo "    NOTCHECKED          yellow"
     echo "    BRIGHTOBJECT        blue"
     echo "    OBJECT              red"
     echo "    BINOBJECT           magenta"
     echo "    CATOBJECT           cyan"
     echo "    SUBTRACTED          orange"
     echo "    GHOST               orange"
     echo "    CR                  green"
     echo ""

}


# show requested data for a given object
# for flags != 0 print OBJ_FLAGS 
proc o_table {id {flags 0} {params {psfCounts fiberCounts}}} {
global table
      p_objc table $id all $params
      if {$flags} {
          pflags
          pflags2
      }
}




###### Finding Charts  ######


# assemble individual fit finding charts into a mosaic
# dir is the name of directory with individual *.fit files given
# in list InList. InList may contain more fit files than needed
# for this mosiac, files from n1 to n1+nh*nv will be assembled
# nh and nv are the horizontal and vertical sizes 
# if reverse == 1 the finding charts are in negative
proc make_chart_mosaic {dir InList n1 name {nh 3} {nv 4} {reverse 1}} {

      set list ""
      set ntot [expr $nh * $nv]
      loop i $n1 [expr $n1 + $ntot] {
	  set aux [lindex $InList $i]
          #set list "$list $root.$i.fit"
          set list "$list $aux"
      }
  
      set mosaic [fitlist2mosaic $list $dir $nh $nv $reverse]
      display $mosaic
      regWriteAsFits $mosaic $dir/mosaic$name.fit 

      # regPrint flips rows...
      regRowFlip $mosaic 
      if {$reverse} {
          set a [regPrint $mosaic $dir/mosaic$name.ps -title \
                       "mosaic $name" -nodither -sqrt]   
      } else {
          set a [regPrint $mosaic $dir/mosaic$name.ps -title \
                       "mosaic $name" -nodither -sqrt -noreverse]   
      } 

      regDel $mosaic

}



# given a par file, which must include fields name, run, ra, and dec, 
# dump stamps to fits files, 
# for ps != 0 produce PS files, for ppm != 0 also produce color images
# when producing PS files, each image will have the corresponding 
# SDSS-IAU name as a bottom label, and if $text != "", this $text
# will be added in parenthesis, e.g. for $text = "RR Lyrae" the label
# will be "SDSSp J142742.51-002848.8 (RR Lyrae)"
# If the parameter file does not have a field "run" then set the
# input variable runIn to a desired run (i.e. in this case all sources
# must/will be from the same run). Similarly, if the field "name" does
# not exist, set the input variable name to whatever string to be 
# used as the name root. A number will be added to this root which will
# correspond to the source's index in the input file. If no such a root
# is desired, then set name to "none".
# N.B. to avoid adding marks (North, South etc.) set global
# variable ignore_marks to 1, e.g.
# set ignore_marks 1 
# - to avoid marks completely (i.e. even the cross) set ignore_marks 
# to > 1
proc finding_charts {par_file dir filter stampsize {n1 0} {Ntot -1} \
	   {ps 1} {text ""} {ppm 0} {runIn -1} {name ""} {rerun ""} } {

    # debug option
    set display 0
    set Nfound 0

    set chain [param2Chain $par_file hdr]
    if {$Ntot < 0} { 
        set Ntot [expr [chainSize $chain] - $n1]
    } 
    echo " Cutting stamps for $Ntot sources..."

    # make chain for sources not found
    set not_found [chainNew [exprGet $chain.type]]

    loop i $n1 [expr $Ntot + $n1] {
        set el [chainElementGetByPos $chain $i] 
        set f_name ""
        if {$name == ""} {
            set fname $par_file.[exprGet $el.name]
        } else {
            if {$name != "none"} {
                set fname $par_file.$name.$i
            } else {
                set fname $par_file.$i
            }
        }
        if {$runIn < 0} {
           set run [exprGet $el.run]
        } else {
           set run $runIn
        }
        set ra [exprGet $el.ra]
        set dec [exprGet $el.dec]
        if {[finding_chart $run $ra $dec $filter $stampsize $dir \
		 $fname $ps $text $ppm $rerun] < 0} {
            # not found, add it to the chain
            chainElementAddByPos $not_found $el TAIL AFTER
            echo "      source [expr $i + 1] not found (RA=$ra, Dec=$dec)"
        } else {
            echo "      source [expr $i + 1] OK"
            incr Nfound
        }
    }

    if {[chainSize $not_found] > 0} {
        chain2Param finding_charts.NOTfound.par $not_found $hdr
        echo " Did not find [chainSize $not_found] sources"
        echo " They are written to file finding_charts.NOTfound.par"
    }

    echo "  *** Produced charts for $Nfound sources ***"
    # clean
    chainDel $not_found 
    genericChainDestroy $chain 

}


# given a file with the listing of (run, RA, Dec) produce stamps
# analogously to proc finding_charts
# NOTE that columns in the file are zero-indexed!!!
# for file name in the chart title set ps to > 1
# e.g. dump_finding_charts lowzQSO.list 0 1 2 18 40 . r 300 2
# if the par file doesn't have an entry for run, then specify colRun = -$run
# to cut stamps from a given run 
proc dump_finding_charts {file colRun colRA colDec {n1 -1} {n2 1000} {dir .} \
	      {filter r} {stampsize 300} {ps 1} {text ""} {run -1} {rerun ""} } {

       ### search input file
       set Nline 0
       set inf [open $file r]
       # loop over input file
       while {![eof $inf]} {
	  set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#"} {  
                  continue
             }
             incr Nline  
	     if {$Nline < $n1 || $Nline > $n2} {
                 continue
             } else {
               catch { 
                 # extract values
                 if {$colRun < 0} {
                     set run [expr abs($colRun)]
                 } else {
                     set run [lindex $line $colRun] 
                 }  
                 set ra [format "%11.7f" [lindex $line $colRA]]   
                 set dec [format "%11.7f" [lindex $line $colDec]]
                 set name $file.$Nline
                 finding_chart $run $ra $dec $filter $stampsize $dir $name $ps $text 0 $rerun
	       }
             }
	  }
       }

   close $inf

}


# given a file with the listing of (run, RA, Dec) produce ppm (color) stamps
# analogously to proc finding_charts
# NOTE that columns in the file are zero-indexed!!!
# for file name in the chart title set ps to > 1
# e.g. dump_finding_charts lowzQSO.list 0 1 2 18 40 . r 300 2
# if the par file doesn't have an entry for run, then specify colRun = -$run
# to cut stamps from a given run 
# to mark the position with a cross first set
# set ignore_marks -1
proc dump_ppm_stamps {file colRun colRA colDec {n1 -1} {n2 1000} {dir .} \
			  {stampsize 300} {filters {g r i}} {usefpC 0} {stretch -1}} {
global data_root

       set data_root [get_data_root ""]

       ### search input file
       set Nline 0
       set inf [open $file r]
       # loop over input file
       while {![eof $inf]} {
	  set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#"} {  
                  continue
             }
             incr Nline  
	     if {$Nline < $n1 || $Nline > $n2} {
                 echo skipping line $Nline
                 continue
             } else {
               catch { 
                 # extract values
                 if {$colRun < 0} {
                     set run [expr abs($colRun)]
                 } else {
                     set run [lindex $line $colRun] 
                     if {1} {
                        # for files dumped by MAS sm tools
			set run [string range $run 0 3]
                     }
                 }  
                 set run [expr int($run)]
                 set ra [format "%11.7f" [lindex $line $colRA]]   
                 set dec [format "%11.7f" [lindex $line $colDec]]
                 echo working on run=$run at RA,Dec=($ra,$dec)
                 set name $dir/$file.[format "%05d" $Nline]
                 # get frame coordinates
                 set rerun [get_highest_dir $data_root/$run]
                 echo calling where_in_run $run $ra $dec -rerun $rerun
                 set position [where_in_run $run $ra $dec -rerun $rerun]
                 # make sure that this position exists
                 if {$position < 0} {
                     echo "stamp $Nline, run $run at ($ra, $dec) off frame"
                     continue
                 }
                 set camcol [lindex $position 1] 
                 set field  [lindex $position 2]
                 set row    [lindex $position 3]
                 set col    [lindex $position 4]
                 # call the work horse
                 echo "calling get_ppm_stamp with $run $camcol $field $row $col"
                 get_ppm_stamp $run $camcol $field $row $col $stampsize $name \
                               1 1 $stretch $filters $usefpC $rerun
	       }
             }
	  }
       }

   close $inf

}


#########################################################################
# proc for producing an explanatory file to go with a list of ppm files 
# produced from fpC files
# do NOT forget to:
# photo> sdr
# photo> set_run $run $camCol -rerun $rerun 
#
# N.B. !!! THIS VERSION WORKS ONLY FOR EQUATORIAL SCANS !!!
#
proc fpC2ppmTable {run camCol startField endField CCdir {filters {g r i}}} {

global openit data_root 

     assert {[llength $filters] == 3}
     set Cfilters [lindex $filters 0][lindex $filters 1][lindex $filters 2]
     set runstr [format "%06d" $run] 

     set outfile $CCdir/CClist-${runstr}-${Cfilters}${camCol}.dat
     set outf [open $outfile w]
  
     set header "#  RA2000      Dec2000    Width   Height  Angle(N2x)     Filename"
     puts $outf $header
     set header "#   deg          deg       deg      deg      deg"
     puts $outf $header

     # standard fpC files (with the overlap)
     set H [format %8.5f [expr 1489*0.396/3600]]
     set W [format %8.5f [expr 2048*0.396/3600]]
     set A [format %10.6f 1.0]

     loop field $startField [expr $endField + 1] {
        set fieldstr [format "%04d" $field]
        set CCfile CC-${runstr}-${Cfilters}${camCol}-$fieldstr.ppm
	if {![file exist $CCdir/$CCfile]} {
            error "cannot find $CCfile"
	} else {
            # first get coordinates
            set RADec [pix2eq $run $camCol $field 0 0]
            set RA [format %10.6f [lindex $RADec 0]]
            set Dec [format %10.6f [lindex $RADec 1]]
            puts $outf  " $RA $Dec $W $H $A   $CCfile"
        }
     }

     close $outf
     echo "your output is in $CCdir/CClist-${runstr}-${Cfilters}${camCol}.dat"
}



# call fpC2ppm for a file full of (run, camCol, field) listings
# data columns are 1-indexed
# to force a rerun set colRerun to -$rerun
proc file2fpC2ppm {file colRun colCamCol colField {colRerun ""} {outDir .} {filters {g r i}}} {

global data_root openit

       set data_root [get_data_root ""]

       ### search input file
       set Nline 0
       set inf [open $file r]
       # loop over input file
       while {![eof $inf]} {
	  set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#"} {  
                  continue
             }
             incr Nline  
             if [catch { 
                # extract values
                set run [lindex $line [expr $colRun-1]] 
                set camCol [lindex $line [expr $colCamCol-1]] 
                set field [lindex $line [expr $colField-1]] 
                echo "  working on $run $camCol $field..."
		if {$colRerun == ""} {
                   set_run $run $camCol
                } else {
		   if {$colRerun < 0} {
                       set rerun [expr abs($colRerun)]
                   } else {
                       set rerun [lindex $line [expr $colRerun-1]] 
                   }
                   set_run $run $camCol -rerun $rerun
                }
                set fpCdir [file dirname $openit(asdir)]/corr/$openit(col)                 
                fpC2ppm $run $camCol $field $field $fpCdir "" -1 $filters
                set fpCname $outDir/fpC-$run-$camCol-$field.ppm
                exec mv aux-$field.ppm $fpCname
                echo "       ... made $fpCname"
             } msg] {
                echo "TROUBLE: $msg"
             }
	  }
       }

   close $inf
   echo "-- processed $Nline lines from file $file"
}


#########################################################################
# proc for making ppm files from fpC files
# given astrom file, fpC directory, run, col, startField, and endField, 
# produce ppm color images for given filter set.
# stretch is log by default. for other options see comments above
# regU16ToU8LUTGet 
# n.b. gri combination works best, especially for low SB objects
# do NOT forget to:
# photo> sdr
# photo> set_run $run $camCol -rerun $rerun 
proc fpC2ppm {run camCol startField endField fpCdir \
	      {CCdir ""} {stretch -1} {filters {g r i}} } {

# needed for set_run
global openit ignore_marks cross_length cross_width cross_clearance

     assert {[llength $filters] == 3}
     set Cfilters [lindex $filters 0][lindex $filters 1][lindex $filters 2]
     set runstr [format "%06d" $run] 
     if {$CCdir == ""} {set CCdir $fpCdir}

     # default stretch works nicely for low SB objects
     if {$stretch < 0 && $stretch != "histeq"} {set stretch 2:300s}
     # log stretch seems to work best
     set type 2

     loop field $startField [expr $endField + 1] {
        set fieldstr [format "%04d" $field]
        foreach filter $filters {
           set ifilter [lsearch $filters $filter]
           set fpCfile $fpCdir/fpC-${runstr}-${filter}${camCol}-$fieldstr.fit
           catch {exec mv ${fpCfile}. $fpCfile}
           set f($ifilter) $filter:$fpCfile
        }

        if [catch {
	   3fits2ppm $run $camCol $field $CCdir $f(2) $f(1) $f(0) aux $stretch $type 0
         #  exec mv aux-$field.ppm $CCdir/CC-${runstr}-${Cfilters}${camCol}-$fieldstr.ppm
        } msg] {
           echo "Problems in 3fits2ppm: $msg"
        }
     }      

}



# given a run and a (RA, Dec) position dump a stamp to fits file, 
# for ps != 0 produce also a PS file, and for ppm != 0 also produce color 
# images
# when producing PS file, image will have the SDSS-IAU name below the panel, 
# and in parenthesis it will have $text, e.g. for text = "RR Lyrae":
# SDSSp J142742.51-002848.8 (RR Lyrae)
# images will be written to directory $dir, as $name
proc finding_chart {run ra dec {filter r} {stampsize 300} {dir .} \
	{name finding_chart} {ps 1} {text ""} {ppm 0} {rerun ""}} {

global ignore_marks data_root

        set name $dir/$name
        set data_root [get_data_root ""]
        # find the highest rerun number if needed
        if {$rerun == ""} {
            set rerun [get_highest_dir $data_root/$run]
            if {$rerun < 0} {
               error "Directory $data_root/$run doesn't contain any reruns!?"
            }  
        } 
        set position [where_in_run $run $ra $dec -rerun $rerun]

        # make sure that this position exists
        if {$position < 0} {
            return -1
        }

        set camcol [lindex $position 1] 
        set field  [lindex $position 2]
        set row    [lindex $position 3]
        set col    [lindex $position 4]

        # get the reconstructed frame
        # echo "getting frame for $run-$rerun $camcol $field $filter:"
        set frame [get_field $run $camcol $field $filter "" $rerun]

        if {[info exist ignore_marks] && $ignore_marks > 1} {
	   ;
        } else {
           # mark cross
           set char_size [expr int(0.075*$stampsize)] 
            set line_width [expr int($stampsize/250.0)] 
           if {$line_width < 2} {set line_width 2}
           if {$stampsize < 300} {
              set clearance 10
              set char_size [expr int(0.2*$stampsize)] 
              set line_width 1
           } else {
              set clearance 15
              set char_size [expr int(0.1*$stampsize)] 
              set line_width 2
           }
           mark_cross $frame $row $col $char_size $line_width $clearance
        }

        # cut stamp
        set stamp [stamp_from_region $frame $row $col $stampsize offsets]
        # rotate stamp so that North is up and East to the left
        ## first find the rotation angle
        # offset dec by 10 pixels
        set decR [expr $dec+0.0011]
        set Ddec 100.0
        set Rposition [where_in_run $run $ra $decR -rerun $rerun]
        # make sure that this position exists
        if {$Rposition < 0} {
           # offset dec by -100 pixels
           set decR [expr $dec-0.0011]
           set Ddec -100.0
           set Rposition [where_in_run $run $ra $decR -rerun $rerun]
           if {$Rposition < 0} {
               # failed...
               set colR $col
               set rowR $row
           } else {
               set rowR [lindex $Rposition 3]
               set colR [lindex $Rposition 4]
           }
        } else {
           set rowR [lindex $Rposition 3]
           set colR [lindex $Rposition 4]
        }
        # given row,col and rowR,colR, get the rotation angle
        if {$col == $colR} {
            if {row < $rowR} {
                set angle 90.0
            } else {
                set angle -90.0
            } 
        } else {
            set tanAngle [expr 1.0*$Ddec/abs($Ddec)*($rowR-$row)/($colR-$col)]
            set angle [expr atan($tanAngle)/3.14159*180 - 90.0]
        }
        set Rstamp [regIntRotate $stamp $angle]

        if {[info exist ignore_marks] && $ignore_marks} {
	   ;
        } else {
           # 151 is for 151 pixels = 1 arcmin
           mark_sides $Rstamp $char_size 151 $line_width
           #display $Rstamp
        }        

        # write to fits file
        regWriteAsFits $Rstamp $name.fit
        if {$ps} {
            # produce a PS file
            # regPrint flips rows...
            regRowFlip $Rstamp 
            set SDSSname [RADec2SDSSname $ra $dec 0] 
            if {$text == ""} {
                set size_arcmin [format "%2.1f" [expr $stampsize*0.4/60.0]]
                set a [deg2s $ra $dec]    
                set RAhms "[lindex $a 0]h [lindex $a 1]m [lindex $a 2]s"  
                set Decdms "[lindex $a 3]d [lindex $a 4]' [lindex $a 5]\""  
                set t1 "Size ${size_arcmin}x${size_arcmin} arcmin,"
                set t2 "mark at RA = $RAhms, Dec = $Decdms"
                set title "$t1 $t2"
                if {$ps > 1} {
                    set title "$name.ps: RA = $RAhms, Dec = $Decdms" 
                }
            } else {
                set title "$SDSSname ($text)"
            }
            set a [regPrint $Rstamp $name.ps -title $title -nodither -sqrt]    
        }
        if {$ppm} {
            # get a ppm stamp
            get_ppm_stamp $run $camcol $field $row $col $stampsize $name 1 1 -1 {g r i} 0 $rerun
        }
        # clean
        regDel $Rstamp
        regDel $stamp
        regDel $frame

        return 0
 
}


# given an SDSS image = region, cut out a stamp
proc stamp_from_region {reg rc cc ss _offsets} {
# offsets are positions of the lower left corner
upvar $_offsets offsets

     if {$ss <= 1489} {
         set ss_row $ss
     } else {
         set ss_row 1489
     }    
     if {$ss <= 2048} {
         set ss_col $ss
     } else {
         set ss_col 2048
     }

     set ss_row2 [expr $ss_row/2]
     set ss_col2 [expr $ss_col/2]
     set c_ll [expr $cc - $ss_col2]
     set r_ll [expr $rc - $ss_row2] 
     if {$c_ll < 0} {set c_ll 0.0} 
     if {$r_ll < 0} {set r_ll 0.0} 
     if {[expr $r_ll + $ss_row] > [exprGet $reg.nrow]} { 
         set r_ll [expr [exprGet $reg.nrow] -  $ss_row]
         if {$r_ll < 0} {
            set r_ll 0
            set ss_row [exprGet $reg.nrow]
         }
     }
     if {[expr $c_ll + $ss_col] > [exprGet $reg.ncol]} { 
         set c_ll [expr [exprGet $reg.ncol] -  $ss_col]
         if {$c_ll < 0} {
            set c_ll 0
            set ss_col [exprGet $reg.ncol]
         }

     }

     set offsets(row) $r_ll
     set offsets(col) $c_ll

     # cut the stamp   
     set subreg [subRegNew $reg $ss_row $ss_col $r_ll $c_ll]

    return $subreg 

}


# aux proc to get bright galaxies from an orphan file
proc get_bGalOrphans {infile {outfile ""} {rmax 20.0} } {
 
      if {$outfile == ""} {set outfile $infile}

      # select galaxies with good flags
      if {![file exists $outfile.galaxies]} {
          set flagsOFF {BRIGHT SATUR BLENDED EDGE}
          flag_search $infile $outfile.galaxies $flagsOFF $flagsOFF {} {} 3
      }

      # select bright galaxies
      selectPHOTOMETRY $outfile.galaxies $outfile.Bgalaxies [list r 14.0 $rmax]

}

# for a par file containing a structure with fields ra and dec, and
# for 2 SDSS runs, produce 2-epoch mosaics
proc get2epochstamps {file run1 run2 {display 0} {size 300} {f1 r} {f2 r}} {
 
    set chain [param2Chain $file ""]
    set N [chainSize $chain]
    echo "read $N elements from file $file" 

    loop i 0 $N {
        set el [chainElementGetByPos $chain $i]
        set ra [exprGet $el.ra]; set dec [exprGet $el.dec]
        set reg [c2e $run1 $run2 $ra $dec $f1 $f2 $size 0 0]
        if {$reg == -1} { 
           echo "cannot cut stamp with ra= $ra, dec = $dec"
           continue
        }
        # rotate stamp so that North is up and East to the left
        set Rstamp [regIntRotate $reg 90]
        if {$display} {
            echo J2000 = [deg2s $ra $dec]
            display $Rstamp "source $i"
        }
        set a [regPrint $Rstamp $file.2epochs.$i.ps -title "$file, source $i" -nodither -sqrt]
        regWriteAsFits $Rstamp $file.2epochs.$i.fit
        regDel $reg 
        regDel $Rstamp 
    }

    genericChainDestroy $chain


}

proc moving_object_images {run1 ra1 dec1 run2 ra2 dec2 {f1 g} {f2 r} {size 300}} {

    # epoch 1
    foreach band [list $f1 $f2] {
       set reg [c2e $run1 $run2 $ra1 $dec1 $band $band $size 0 0] 
       regWriteAsFits $reg moi-band-$band-epoch1-$run1-$run2.fit
       regDel $reg
    }
    # epoch 2
    foreach band [list $f1 $f2] {
       set reg [c2e $run1 $run2 $ra2 $dec2 $band $band $size 0 0] 
       regWriteAsFits $reg moi-band-$band-epoch2-$run1-$run2.fit
       regDel $reg
    }


}


### compare images from 2 runs when positions are given by (RA, Dec)
proc c2e {run1 run2 ra dec {f1 r} {f2 r} {size 300} {display 1} {dump 0}} {

        set data_root [get_data_root ""]
        set rerun1 [get_highest_dir $data_root/$run1]
        set rerun2 [get_highest_dir $data_root/$run2]

        # make sure that this position exists
        if {[where_in_run $run1 $ra $dec -rerun $rerun1] < 0} {
            echo "Position ($ra1, $dec1) doesn't exist in run $run1"
            return -1
        }
 
        set coord1 [where_in_run $run1 $ra $dec]
        set camcol1 [lindex $coord1 1] 
        set field1  [lindex $coord1 2]
        set row1    [lindex $coord1 3]
        set col1    [lindex $coord1 4]

        # make sure that this position exists
        if {[where_in_run $run2 $ra $dec -rerun $rerun2] < 0} {
            echo "Position ($ra2, $dec2) doesn't exist in run $run2"
            return -1
        }

        set coord2  [where_in_run $run2 $ra $dec]
        set camcol2 [lindex $coord2 1] 
        set field2  [lindex $coord2 2]
        set row2    [lindex $coord2 3]
        set col2    [lindex $coord2 4]

        set mosaic [get2epochsreg $run1 $camcol1 $field1 $row1 $col1 \
			$run2 $camcol2 $field2 $row2 $col2 $size $f1 $f2]     

        if {$display} {display $mosaic}

        if {$dump} {regWriteAsFits $mosaic c2e_$run1-$run2.fit}

        return $mosaic

}


# for a par file containing a structure with fields 
# run, camcol, field, row, col and run2, camcol2, field2, row2 and col2, 
# dump mosaics and PS files
proc dump_2epochs_stamps {file {display 0} {size 300} {f1 r} {f2 r}} {
 
    set chain [param2Chain $file ""]
    set N [chainSize $chain]
    echo "read $N elements from file $file" 

    loop i 0 $N {
        set el [chainElementGetByPos $chain $i]
        set run1 [exprGet $el.run]; set cc1 [exprGet $el.camcol]
        set fld1 [exprGet $el.field]
        set r1 [exprGet $el.row]; set c1 [exprGet $el.col]
        set run2 [exprGet $el.run2]; set cc2 [exprGet $el.camcol2]
        set fld2 [exprGet $el.field2]
        set r2 [exprGet $el.row2]; set c2 [exprGet $el.col2]
	set reg [get2epochsreg $run1 $cc1 $fld1 $r1 $c1 $run2 $cc2 $fld2 $r2 $c2 \
                               $size $f1 $f2]
        # rotate stamp so that North is up and East to the left
        set Rstamp [regIntRotate $reg 90]
        if {$display} {
            echo J2000 = [deg2s [exprGet $el.ra] [exprGet $el.dec]]  
            display $Rstamp "source $i"
        }
        set a [regPrint $reg $file.2epochs.$i.ps -title "source $i" -nodither -sqrt]
        regWriteAsFits $reg $file.2epochs.$i.fit
        regDel $reg 
        regDel $Rstamp 
    }

    genericChainDestroy $chain


}

### compare 2 runs
proc get2epochsreg {run1 camCol1 field1 row1 col1 run2 camCol2 field2 row2 col2 \
		    {size 300} {f1 r} {f2 r}} {


        # for marking crosses
        set char_size [expr int(0.075*$size)] 
        set line_width [expr int($size/250.0)] 
        if {$line_width < 2} {set line_width 2}
        if {$size < 300} {
            set clearance 10
        } else {
            set clearance 15
        }

        # get the reconstructed frame1
        echo "getting frame for $run1 $camCol1 $field1 $f1"
        set frame1 [get_field $run1 $camCol1 $field1 $f1]
        mark_cross $frame1 $row1 $col1 $char_size $line_width $clearance
        # cut stamp1
        set stamp(0) [stamp_from_region $frame1 $row1 $col1 $size offsets]
        
        # get the reconstructed frame2
        echo "getting frame for $run2 $camCol2 $field2 $f2"
        set frame2 [get_field $run2 $camCol2 $field2 $f2]
        mark_cross $frame2 $row2 $col2 $char_size $line_width $clearance
        # cut stamp2
        set stamp(1) [stamp_from_region $frame2 $row2 $col2 $size offsets]
 
        # make mosaic
        set nv 1; set nh 2;
        set gutter 10
        set mosaic [regNew [expr $nv*($size+$gutter)-$gutter] \
		   [expr $nh*($size+$gutter)-$gutter]]
        regClear $mosaic
        set r0 0
        set c0 0
        set full 0
        set hcount 0
        set nstars 2
 
        loop i 0 $nstars {
           set reg $stamp($i)
           if {$hcount < $nh || ($col_even && $hcount < [expr $nh - 1])} {
 	      incr hcount
           } else { 
              set hcount 1
	      set c0 0
	      set r0 [expr $r0+$size+$gutter]
           }
           set sub [subRegNew $mosaic $size $size $r0 $c0]
           set c0 [expr $c0+$size+$gutter]
           regAdd $sub $reg
           regDel $sub
           regDel $reg
        }
  
        regDel $frame1; regDel $frame2;

      return $mosaic       

}

 

# given RA and Dec in degrees, construct a string with IAU - SDSS name
# for that object
# if TeX==1 then every "-" sign is enclosed in $
proc RADec2SDSSname {RA Dec {TeX 1}} {

      # convert :: forms
      if [regexp {^([0-9]+):([0-9]+):(.*)} $RA foo hr min sec] {
         foreach el "hr min sec" {
	    regsub {^0*} [set $el] "" $el
         }
         set RA [expr 15*($hr + ($min + $sec/60.0)/60.0)]
      }
      if [regexp {^([+-])?([0-9]+):([0-9]+):(.*)} $Dec foo sgn hr min sec] {
         foreach el "hr min sec" {
	    regsub {^0*} [set $el] "" $el
         }
         set Dec [expr $hr + ($min + $sec/60.0)/60.0]
         if {$sgn == "-"} {
	   set Dec [expr -$Dec]
         }
      }
            set seks [deg2s $RA $Dec]
             set sRA "[lindex $seks 0] [lindex $seks 1] [lindex $seks 2]"
             set sDec "[lindex $seks 3] [lindex $seks 4] [lindex $seks 5]"
             set sRAh [lindex $seks 0]
             if {$sRAh < 10} {set sRAh "0$sRAh"}
             set sRAm [lindex $seks 1]
             if {$sRAm < 10} {set sRAm "0$sRAm"}
             set sRAs [lindex $seks 2]
             if {$sRAs < 10} {set sRAs "0$sRAs"}
             set sDecD [lindex $seks 3]
             set DecSign [string range $sDecD 0 0]
             if {$DecSign != "-" && $sDecD < 10} {
                 if {$sDecD < 1} {
                    set sDecD "+00"
                 } else {
                    set sDecD "+0$sDecD"
                 }
             }
             if {$DecSign != "-" && $sDecD >= 10} {set sDecD "+$sDecD"}
             if {$DecSign == "-" && $sDecD <= -10} {
                 if {$TeX} {
                     set sDecD "\$-\$[expr abs($sDecD)]"
                 } else {
                     set sDecD "-[expr abs($sDecD)]"
                 }
             } else {
                 if {$DecSign == "-" && $sDecD > -10} {
                   if {$sDecD > -1} {
                      if {$TeX} {
                         set sDecD "\$-\$00"
                      } else {
                         set sDecD "-00"
                      }
                   } else {
                      if {$TeX} {
                         set sDecD "\$-\$0[expr abs($sDecD)]"
                      } else {
                         set sDecD "-0[expr abs($sDecD)]"
                      }
                   }
                 }
	     }

             set sDecM [lindex $seks 4]
             if {$sDecM < 10} {set sDecM "0$sDecM"}
             set sDecS [lindex $seks 5]
             if {$sDecS < 10} {set sDecS "0$sDecS"}
             set IAUname "SDSSp J${sRAh}${sRAm}${sRAs}${sDecD}${sDecM}${sDecS}"

     return $IAUname

}


# given a region, mark cross of specified half length in pixels 
# at the position (rowC, colC)  
# the line width is (2*width+1) pixels
# the distance from the specified position to the beginning of 
# the cross is c pixels
proc mark_cross {region rowC colC half_length {width 3} {c 10}} {
   global ROWS

     set h $half_length
     set rowC [expr int($rowC)]
     set colC [expr int($colC)]
     # distance from the position to the beginning of the cross
     set clearance $c
    
     set nrow [exprGet $region.nrow]
     set ncol [exprGet $region.ncol]
     # vertical part
     loop row [expr $rowC - $h] [expr $rowC + $h + 1] {
         if {$row >= [expr $rowC - $clearance] && \
             $row <= [expr $rowC + $clearance]} continue
         if {$row >= 0 && $row < $nrow} {
            loop w 0 $width {
              if {$w <= $colC} {
                 handleSet $region.$ROWS<$row><[expr $colC-$w]> 65500
              } elseif {[expr $colC+$w] < $ncol} {
                 handleSet $region.$ROWS<$row><[expr $colC+$w]> 65500
              }
            }
         }
     }
     # horizontal part
     loop col [expr $colC - $h] [expr $colC + $h + 1] {
         if {$col >= [expr $colC - $clearance] && \
             $col <= [expr $colC + $clearance]} continue
         if {$col >= 0 && $col < $ncol} {
            loop w 0 $width {
              if {$w <= $rowC} {
                 handleSet $region.$ROWS<[expr $rowC-$w]><$col> 65500
              } elseif {[expr $rowC+$w] < $nrow} {
                # handleSet $region.$ROWS<[expr $rowC+$w]><$col> 65500
              }
            }

         }

     }

}


# given a region, mark N in the middle of the top edge
# and mark E in the middle of the lef edge, with the size 
# of characters given by S (in pixels) 
# mark a length given by L (pixels) in the lower left 
# corner (can be 0)
# the line width is (2*width+1) pixels
proc mark_sides {region S {L 150} {width 3}} {
   global ROWS

     set S [expr $S/2]
     set nrow [exprGet $region.nrow]
     set nrow2 [expr  $nrow / 2]
     set ncol [exprGet $region.ncol]
     set ncol2 [expr $ncol / 2]

     ### length mark
     if {$L > 0} {
        # horizontal line
        loop col $S [expr $S + $L] {
           loop w 0 $width {
	      foreach f {-1 1} {
                 set row [expr $S + $f*$w]
                 if {$row >= 0 && $row < $nrow && $col < $ncol} {
                    handleSet $region.$ROWS<$row><$col> 65500
                 }
              }
	   }
        }
        # vertical bars
        loop row [expr int(0.5*$S)] [expr int(1.5*$S)] {
	  foreach ll {0 $L} {
            loop w 0 $width {
	      foreach f {-1 1} {
                 set col [expr $S + $ll + $f*$w]
	         if {$row >= 0 && $row < $nrow && $col >= 0} {
                     handleSet $region.$ROWS<$row><$col> 65500
                 }
              }
	    }
	  }
        }
     }

     ### E mark
     # vertical line
     loop row [expr $nrow2 - $S - $width + 1] [expr $nrow2 + $S + $width] {
        loop w 0 $width {
	  foreach f {-1 1} {
             set col [expr $S + $f*$w]
	     if {$row >= 0 && $row < $nrow && $col >= 0} {
                handleSet $region.$ROWS<$row><$col> 65500
             }
          }
        }
     }
     # 3 horizontal lines
     loop col $S [expr int(2.1*$S)] {
	foreach f {0 -1 1} {
          # middle bar should be shorter
	  if {$f == 0 && $col > [expr int(1.9*$S)]} {continue}
          loop w 0 $width {
             set rowm [expr $nrow2 + $f*$S - $w]
             set rowp [expr $nrow2 + $f*$S + $w]
	     if {$rowm >= 0} {
                handleSet $region.$ROWS<$rowm><$col> 65000
             }
	     if {$rowm < $nrow} {
                handleSet $region.$ROWS<$rowp><$col> 65000
             }
          }
        }
     }

     ### N mark 
     set Sn [expr int(0.8*$S)]
     loop row [expr $nrow - 3*$S] [expr $nrow - $S] {
	if {$row < 0} continue
        loop w 0 $width {
	  foreach f {-1 1} {
             # two vertical lines
             set colL [expr $ncol2 - $Sn + $f*$w]
             set colR [expr $ncol2 + $Sn + $f*$w]
	     if {$colL >= 0 && $colL < $ncol} { 
                handleSet $region.$ROWS<$row><$colL> 65000
             }
	     if {$colR >= 0 && $colR < $ncol} { 
                handleSet $region.$ROWS<$row><$colR> 65000
             }
             # connecting bar
             set delta_row [expr $row - ($nrow - 3*$S)]
             set delta_col [expr int(1.0*$delta_row / $S * $Sn)]
             set colB [expr $ncol2 + $Sn - $delta_col + $f*$w]
	     if {$colB >= [expr $ncol2 - $Sn + $width] && \
                 $colB <= [expr $ncol2 + $Sn - $width]} { 
                 handleSet $region.$ROWS<$row><$colB> 65000
             }
	  }
	}
     }
}





###### Miscellaneous ######
# make a PS file from a region stored in a fit file
proc fit2ps {path file pathout {title ""}} {

    set reg [regReadAsFits [regNew] $path/$file.fit]
    if {$title == ""} {
         set title $file
    }
    regPrint $reg $pathout/$file.ps -title $title -noreverse -nodither -sqrt
    regDel $reg
}



# print a FITS file header
proc print_hdr {file} {
  hdrPrint [hdrReadAsFits [hdrNew] $file]
} 


# make a mosaic of all regions stored in fit files listed in fitlist,
# e.g. fitlist = {file1.fit file2.fit}
# If col_even is true, the mosaic will have an even number of columns
# add offset to each region (not to gutter)
proc fitlist2mosaic {fitlist dir nh nv {col_even 0} {gutterRow 2} \
	            {gutterCol 2} {offset 0} {scale 0} {blackValue 0}} {


   # Region will have nv by nh stamps, with $gutter pixels between each
   set nstars [llength $fitlist]
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

   set fitfile [lindex $fitlist 0]
   set reg [regReadAsFits [regNew] $dir/$fitfile]
   set Rstampsize [exprGet $reg.nrow]
   set Cstampsize [exprGet $reg.ncol]
   set regType [string range [exprGet -enum $reg.type] 5 100]
   regDel $reg 

   set mosaic [regNew -type $regType [expr $nv*($Rstampsize+$gutterRow)-$gutterRow] \
		   [expr $nh*($Cstampsize+$gutterCol)-$gutterCol]]
   regClear $mosaic
   if {$blackValue != 0} {
      regAddWithDbl $mosaic $blackValue
   }
   set r0 0
   set c0 0
   set full 0
   set hcount 0

   loop i 0 $nstars {

      set fitfile [lindex $fitlist $i]
      if {[file exists $dir/$fitfile]} {
          set reg [regReadAsFits [regNew] $dir/$fitfile]
      } else {
          continue
      }
      if {$hcount < $nh || ($col_even && $hcount < [expr $nh - 1])} {
	 incr hcount
      } else { 
         set hcount 1
	 set c0 0
	 set r0 [expr $r0+$Rstampsize+$gutterRow]
      }
      set sub [subRegNew $mosaic $Rstampsize $Cstampsize $r0 $c0]
      regClear $sub
      set c0 [expr $c0+$Cstampsize+$gutterCol]
      if {$scale} {
          set stats [regStatsFind $reg]
          set C [expr 1.0/[keylget stats high]]
          echo "      scaling by $C"
          regMultiplyWithDbl $reg $C
      }
      regAddWithDbl $reg $offset 
      regAdd $sub $reg
      handleDel $reg
      regDel $sub
   }

   return $mosaic
}


### display Steve's mosaic on netscape for a specified position and
# run and mark the position by a yellow circle (Netscape must be running)
# Run for a given position can be found by using where_in_SDSS
# e.g.
# photo> where_in_SDSS 8:43:7.88 -0:4:51.4
#   run col field   row  col
#   756  3   100    500 1234
# and then:
#photo> RADec2Netscape 756 8:43:7.88 -0:4:51.4
proc RADec2Netscape {run RA Dec} {

   # first get pixel coordinates
   set coord [where_in_run $run $RA $Dec]
   set camcol [lindex $coord 1]
   set field [lindex $coord 2]
   set row [lindex $coord 3]
   set col [lindex $coord 4]

   exec netscape -remote "openURL(http://sdsslnx.fnal.gov:8015/template/tsSingle.tml?run=$run&camcol=$camcol&field=$field&row=$row&col=$col)" &

} 


# display a spectrum netscape for a given plate-mjd-fiber combination 
proc Spec2Netscape {plate mjd fiber} {

   exec netscape -remote "openURL(http://sx.pha.jhu.edu/cgi-bin/sx/getSpec.cgi?platemjd=$plate+%2F+$mjd&fiber=$fiber&format=gif&Get+Spectrum=Get+Spectrum)" &

} 

### for a specified position, run, and radius (in arcsec), list all sources found
# within a circle of that radius (actually a square with side 2*radius). 
# Control the output level by verbosity flag. 
proc pmdb {run RA Dec {radius 1.0} {rerun ""} {verbose 1} {return_chain 0}} {

global openit
 
global usefpObj
   
      if {[info exist usefpObj] && $usefpObj} {
         set fpObj 1
      } else {
         set fpObj 0
      }

      # make sure data dir is kosher
      if {![info exist data_root]} {
          set data_root [get_data_root ""]
      }
      if {![info exist data_root]} {
           error "Specified data_root does NOT exist: $data_root"
      }

      # convert :: forms
      if [regexp {^([0-9]+):([0-9]+):(.*)} $RA foo hr min sec] {
         foreach el "hr min sec" {
	    regsub {^0*} [set $el] "" $el
         }
         set RA [expr 15*($hr + ($min + $sec/60.0)/60.0)]
      }
      if [regexp {^([+-])?([0-9]+):([0-9]+):(.*)} $Dec foo sgn hr min sec] {
         foreach el "hr min sec" {
	    regsub {^0*} [set $el] "" $el
         }
         set Dec [expr $hr + ($min + $sec/60.0)/60.0]
         if {$sgn == "-"} {
	   set Dec [expr -$Dec]
         }
      }

      if {$rerun == ""} {
           set rerun [get_highest_dir $data_root/$run]
      }

      # first get pixel coordinates
      set coord [where_in_run $run $RA $Dec -rerun $rerun]
      if {$coord == -1} {
          set user [exec whoami]
          echo "  Regretfully, dear $user, this position has not been observed in run $run."
          echo "  You may want to try to execute:"
          echo "  where_in_SDSS $RA $Dec"
          echo "  to find out if it was observed at all."
          return
      }
      set camcol [lindex $coord 1]
      set field [lindex $coord 2]
      set row [lindex $coord 3]
      set col [lindex $coord 4]

      # HACK XXX (ZI): will not find tsObj file if the source is in the first field
      if {1 && $row < 64} {
         set row [expr 1361+$row]
         set field [expr $field -1]
      }
      
      # find and read tsObj file
      if {$fpObj} {
          set_run $run $camcol -rerun $rerun 
          set tsObjdir $openit(objdir)
          if [catch {
             set tsObj [glob $tsObjdir/fpObjc-[format %06d $run]-$camcol-[format %04d $field].fit]
                }] {
             echo " Couldn't find any fpObj files as:"
             echo " $tsObjdir/fpObjc-[format %06d $run]-$camcol-[format %04d $field].fit"
             return 0
          } 
      } else {
          set_run $run $camcol -tsObj -rerun $rerun 
          set tsObjdir $openit(objdir)
          if [catch {
             set tsObj [glob $tsObjdir/tsObj-[format %06d $run]-$camcol-*-[format %04d $field].fit]
                }] {
             echo " Couldn't find any tsObj files as:"
             echo " $tsObjdir/tsObj-[format %06d $run]-$camcol-*-[format %04d $field].fit"
             return 0
          } 
      }
      set chain [tsObj2chain $tsObj]

      if {$fpObj} {
          set_run $run $camcol -rerun $rerun 
          set colmax  [expr $col + $radius*2.5]
          set colmin  [expr $col - $radius*2.5]
          set rowmax  [expr $row + $radius*2.5]
          set rowmin  [expr $row - $radius*2.5]
          set local [chainSearch $chain "{$rowmin < objc_rowc < $rowmax} {$colmin < objc_colc < $colmax}"] 
      } else {
         # search for our sources (this should use lambda, eta coordinates but cannot 
         # before astrotools are updated XXX FIX THIS)
         set RAmax  [expr $RA + $radius/3600.0]
         set RAmin  [expr $RA - $radius/3600.0]
         set Decmax [expr $Dec + $radius/3600.0]
         set Decmin [expr $Dec - $radius/3600.0]
         set local [chainSearch $chain "{$RAmin < ra < $RAmax} {$Decmin < dec < $Decmax}"] 
      }

      set Nfound 0
      loop i 0 [chainSize $local] {
	  set el [chainElementGetByPos $local $i]
          if {$verbose > 0} {
            print_one_photo $el 0 0 0 $fpObj 
          } 
          incr Nfound
      }

      if {$Nfound <= 0} {
          if {$fpObj} {
             echo "   No sources are found within $radius arcsec from $run-$camcol-$field-(r=$row,c=$col)"
          } else {
             set RADecstr [format "%8.5f %8.5f" $RA $Dec] 
             echo "   No sources are found within $radius arcsec from $RADecstr"
          }
      }

      
      # clean
      if {!$return_chain} {
          chainDel $local
          genericChainDestroy $chain
      } else {
          return $local
      }

} 




### given a file with RA/Dec listing in column colRA and colDec
# (1-indexed), dump psf photometry for all sources from the given 
# run that are within matchRad arcsec from the specified position
# if run < 0, then dump data from all possible runs
proc pmdbList {filein colRA colDec runAll fileout {matchRad 1.0} \
               {add_modmag 0} {all_flags {SATUR BLENDED CHILD CR INTERP}} \
			 {flagsOFF {BRIGHT BLENDED}} {rerun ""}} {

global openit data_root

     set data_root [get_data_root ""]
 
     set infile [open $filein r]
     set outf [open $fileout w]
  
     set NOK 0; set Nbad 0
     set header "# Nsource  Ne  e  src   MJD  run  camCol field row col    RA    Dec  \
                   Ar  u   uErr g   gErr   r   rErr   i   iErr   z   zErr type  flags"
     if {$add_modmag} {
        set header "$header umod  gmod  rmod  imod  zmod"
     }
     puts $outf $header
     set Nline 0
     set Nsource 0
     while {![eof $infile]} {
         set line [gets $infile]
         echo "processing line $Nline"
         # process line
         if {![eof $infile] && [lindex $line 0] != "#" && $line != ""} {  
            set RA  [lindex $line [expr $colRA -1]]
            set Dec [lindex $line [expr $colDec -1]]
            set RA  [format "%10.6f" $RA]
            set Dec [format "%10.6f" $Dec]
            incr Nsource
            set NsourceF [format "%6d" $Nsource]  
            echo " ... source $Nsource at ($RA, $Dec)"
            set obsList ""
	    if {$runAll < 0} {
                set list [where_in_SDSS $RA $Dec -quiet]
                foreach obs $list {
		   set run [format "%6d" [lindex $obs 0]]
		   set camCol [format "%1d" [lindex $obs 1]]
		   set field [format "%4d" [lindex $obs 2]]
		   set row [format "%6.2f" [lindex $obs 3]]
		   set col [format "%6.2f" [lindex $obs 4]]
                   # get MJD
		   set MJDlist [pix2eq [lindex $obs 0] $camCol $field $row $col r "" "" 1]
		   set MJD [format "%12.6f" [lindex $MJDlist 2]]
		   set obsList [lappend obsList "$MJD $run $camCol $field $row $col"]
                } 
                # sort runs
                # set obsList [lsort -integer -increasing $obsList] 
                echo "....... observed in run $run"
	    } else {
                set obsList "$runAll"
            }
            # loop over all observations for this source
            set allData ""
            set OKsource 0 
            foreach obsData $obsList {
	 	set obsRun [lindex $obsData 1]
                set run [string trimleft $obsRun " "]
                set camCol [lindex $obsData 2]
                set field0 [format "%04d" [lindex $obsData 3]]
		if [catch {set tsObjList [glob $data_root/$run/*/calibChunks/$camCol/tsObj*$field0.fit]}] {
                    set tsObjList ""
                }
 		if {[llength $tsObjList] > 0} {
                     set photo [get_photometry $obsRun $RA $Dec $matchRad $add_modmag \
                                      $all_flags $flagsOFF $rerun]
                     if {$photo != ""} {
                        echo "    .... extracted data for [llength $photo] photo objects for run $obsRun"
                        set SourceCounter 0
                        set OKsource 1
	                foreach line $photo {
                           incr SourceCounter
                           set aux "$SourceCounter $obsData"
                           foreach x $line {
                              set aux "$aux $x"
                           }
		           set allData [lappend allData $aux]
                        }
                     }
                } else {
                   echo "CALLED get_photometry $obsRun $RA $Dec $matchRad $add_modmag |$all_flags| |$flagsOFF| $rerun" 
                }
	    }
            # is there at least one detection?
            if {!$OKsource} {
               incr Nbad
               echo "... source $Nsource at ($RA, $Dec) is WITHOUT DATA (Nbad=$Nbad)"
            } else {
               incr NOK
               echo "... source $Nsource at ($RA, $Dec) is OK (NOK=$NOK)"
               ## dump data
	       set Nepochs [format "%3d" [llength $allData]]
               set iEpoch 0 
	       foreach night $allData {
                  incr iEpoch
	          set iii [format "%3d" $iEpoch]
                  puts $outf " $NsourceF $Nepochs $iii $night"
	       }
               flush $outf
            }
	 }
         incr Nline
         echo "----------------------------------------------------------------"
     }

   close $outf
   close $infile

   echo "Did not match $Nbad objects, and dumped $NOK to $fileout"
    
}



### given a file with RA/Dec listing in column colRA and colDec
# (1-indexed), and run listing in column colRun (set to -$run if not present)
# dump psf and mod photometry for all sources from the given 
# run that are within matchRad arcsec from the specified position
# type 0 or 1 if flags listed in all_flags are off/on
# for fiber mags instead of psf mags set fiberMags to 1
proc missedList {filein colRun colRA colDec fileout {matchRad 1.0} \
		  {all_flags {SATUR BLENDED CHILD DEBLENDED_AS_MOVING}} \
                  {flagsOFF {BRIGHT SATUR BLENDED}}   } {

global openit fiberMags dataInfo

     if ![info exists fiberMags] { set fiberMags 0 }     
     if {$fiberMags} {
        set add_modmags -1
     } else {
        set add_modmags 1
     } 

     set infile [open $filein r]
     set outf [open $fileout w]
  
     set NOK 0; set Nbad 0
     set header " n     RA       Dec    run rr c  fld id    runRA     runDec    u     uErr \
                   g     gErr  r     rErr  i     iErr  z     zErr type flags     Ar   \
                    ======   model mags   ======"
     puts $outf $header

     set Nline 0
     while {![eof $infile]} {
         set line [gets $infile]
         echo "processing line $Nline"
         # process line
         if {![eof $infile] && [lindex $line 0] != "#" && $line != ""} {  
            set RA  [lindex $line [expr $colRA -1]]
            set Dec [lindex $line [expr $colDec -1]]
	    if {$colRun < 0} {
                set run [expr -1*$colRun]
	    } else {
                set run [lindex $line [expr $colRun -1]]
            }
            set photo [get_photometry $run $RA $Dec $matchRad $add_modmags $all_flags $flagsOFF]
            if {$photo != ""} {
               incr NOK
	       foreach line $photo {
		  set iLine [lsearch $photo $line]
                  set dInfo ""
                  set idList [lindex $dataInfo [expr [llength $dataInfo] - 1]]
                  set id [lindex $idList $iLine]
                  loop j 0 [expr [llength $dataInfo] - 3] {
                     set dInfo "$dInfo [lindex $dataInfo $j]"
                  }      
                  set dInfo "$dInfo $id" 
                  puts $outf " $NOK $RA $Dec $dInfo $line"
               }
            } else {
               incr Nbad
            }
	 }
         incr Nline
     }

   close $outf
   close $infile

   echo "Did not match $Nbad objects, and dumped $NOK to $fileout"
    
}



# given run and a position (RA, Dec), return
# "RA   Dec u uErr g gErr r rErr i iErr z zErr type flags"
# where flags is a 1/0 list specified by all_flags
# ignore objects with any of flagsOFF set on
proc get_photometry {run RA Dec radius {add_modmag 0} \
                    {all_flags {SATUR BLENDED CHILD CR INTERP}} \
			 {flagsOFF {BRIGHT}} {rerun ""}} {

global openit dataInfo

      # make sure data dir is kosher
      if {![info exist dataInfo]} {
          set dataInfo ""
      }
      if {![info exist data_root]} {
          set data_root [get_data_root ""]
      }
      if {![info exist data_root]} {
           error "Specified data_root does NOT exist: $data_root"
      }

      set RADec [parseRADec $RA $Dec]
      set RA [lindex $RADec 0]; set Dec [lindex $RADec 1]

      if {$rerun == ""} {
          set rerun [get_highest_dir $data_root/$run]
      }

      # first get pixel coordinates
      set coord [where_in_run $run $RA $Dec -rerun $rerun]
      if {$coord == -1} {
          return ""
      }
      set camcol [lindex $coord 1]
      set field [format "%3.0f" [lindex $coord 2]]
      set row [format "%6.1f" [lindex $coord 3]]
      set col [format "%6.1f" [lindex $coord 4]]
      set dataInfo "$run $rerun $camcol $field $row $col"

      # find and read tsObj file
      set_run $run $camcol -tsObj -rerun $rerun 
      set tsObjdir $openit(objdir)
      if [catch {
          set tsObj [glob $tsObjdir/tsObj-[format %06d $run]-$camcol-*-[format %04d $field].fit]
                }] {
          echo " Couldn't find any tsObj files as:"
          echo " $tsObjdir/tsObj-[format %06d $run]-$camcol-*-[format %04d $field].fit"
          return ""
      } 
      set chain [tsObj2chain $tsObj]

      ## first select all possible matches (and later enforce precise cut
      set fudge 1.1
      set deltaDecMax [sphericalDist 0 0 0 [expr $radius/3600.0]]
      set decMin [expr $Dec-$fudge*$deltaDecMax]
      set decMax [expr $Dec+$fudge*$deltaDecMax]
      if {[expr abs($decMax)] > [expr abs($decMin)]} {
         set decStar $decMax  
      } else {
         set decStar $decMin
      }
      set deltaRAMax [sphericalDist 0 $decStar [expr $radius/3600.0] $decStar]
      set raMin [expr $RA-$fudge*$deltaRAMax]
      set raMax [expr $RA+$fudge*$deltaRAMax]
      set cond "{dec >= $decMin} {dec <= $decMax} {ra >= $raMin} {ra <= $raMax}"
      set local [chainSearch $chain $cond] 

      set Nfound 0
      set list {}
      set idList {}
      loop i 0 [chainSize $local] {
	  set el [chainElementGetByPos $local $i]
          # check the true distance
          set raS [exprGet $el.ra]
          set decS [exprGet $el.dec]
          # distance is returned in arcsec
          set distance [sphericalDist $raS $decS $RA $Dec 1]
          if {$distance > $radius} {continue}
          set flags [flags [exprGet $el.objc_flags]]
          set flagsOK 1
          foreach flag $flagsOFF {
              if {[lsearch $flags $flag] > -1} {
                  set flagsOK 0
              }
          }
          if {$flagsOK} {
             lappend list [photo2photometryOPDB $el $add_modmag $all_flags]
             lappend idList [exprGet $el.id]
          }
      }
      set dataInfo "$dataInfo [list $idList]" 
   
      # clean
      chainDel $local
      genericChainDestroy $chain
    
    return $list

} 



# given an element from tsObj files, return a string: 
# RA Dec Ar u uErr g gErr r rErr i iErr z zErr type flags,
# and, if requested, umod  gmod  rmod  imod  zmod
# NB add_modmag < 0 is a secret code to get fiber instead of psf magnitudes 
proc photo2photometryOPDB {el {add_modmag 0} {all_flags {SATUR BLENDED CR CHILD}}} {

       set filters {u g r i z}    

       set RA [format "%10.6f" [exprGet $el.ra]]
       set Dec [format "%10.6f" [exprGet $el.dec]]
       set flags [flags [exprGet $el.objc_flags]]
       set flags2 [flags2 [exprGet $el.objc_flags2]]
       set objc_type [obj_type_translate [exprGet $el.objc_type]]
      
       set line "$RA $Dec"
       set Ar [format "%6.3f" [exprGet $el.reddening<2>]]
       set line "$line $Ar"
       loop i 0 5 {
          set f [lindex $filters $i]
          ## add_modmag < 0 is a secret code to get fiber magnitudes (for targeting)
	  if {$add_modmag < 0} {
             set mfib [format "%5.2f" [exprGet $el.fiberCounts<$i>]]
             set efib [format "%5.2f" [exprGet $el.fiberCountsErr<$i>]]
             set line "$line $mfib $efib"
          } else {
             set mpsf [format "%5.2f" [exprGet $el.psfCounts<$i>]]
             set epsf [format "%5.2f" [exprGet $el.psfCountsErr<$i>]]
             set line "$line $mpsf $epsf"
          }
       }

       set line "$line $objc_type"
       foreach flag $all_flags {
	   if {[lsearch $flags $flag] < 0} {
	       if {[lsearch $flags2 $flag] < 0} {
                   set line "$line 0"
               } else {
                   set line "$line 1"
               }
           } else {
               set line "$line 1"
           }
       }
       
       if {$add_modmag} {
          loop i 0 5 {
             set f [lindex $filters $i]
             set mmod [format "%4.2f" [exprGet $el.counts_model<$i>]]
             set line "$line $mmod"
          }
       }

       return $line

}




# given an element from tsObj files, return a string with 
# "RA   Dec u uErr g gErr r rErr i iErr z zErr type flags"
# where flags is a list showing if a flag is set, for this flags:
# SATUR BLENDED CR CHILD 
proc photo2photometry {el {add_modmag 0} {all_flags {SATUR BLENDED CR CHILD}}} {

       set filters {u g r i z}    

       set RA [format "%10.6f" [exprGet $el.ra]]
       set Dec [format "%10.6f" [exprGet $el.dec]]
       set flags [flags [exprGet $el.objc_flags]]
       set flags2 [flags2 [exprGet $el.objc_flags2]]
       set objc_type [obj_type_translate [exprGet $el.objc_type]]
      
       set line "$RA $Dec"
       loop i 0 5 {
          set f [lindex $filters $i]
          ## add_modmag < 0 is a secret code to get fiber magnitudes (for targetting)
	  if {$add_modmag < 0} {
             set mfib [format "%5.2f" [exprGet $el.fiberCounts<$i>]]
             set efib [format "%5.2f" [exprGet $el.fiberCountsErr<$i>]]
             set line "$line $mfib $efib"
          } else {
             set mpsf [format "%5.2f" [exprGet $el.psfCounts<$i>]]
             set epsf [format "%5.2f" [exprGet $el.psfCountsErr<$i>]]
             set line "$line $mpsf $epsf"
          }
       }

       set line "$line $objc_type"
       foreach flag $all_flags {
	   if {[lsearch $flags $flag] < 0} {
	       if {[lsearch $flags2 $flag] < 0} {
                   set line "$line 0"
               } else {
                   set line "$line 1"
               }
           } else {
               set line "$line 1"
           }
       }
       set Ar [format "%6.3f" [exprGet $el.reddening<2>]]
       set line "$line $Ar"
       
       if {$add_modmag} {
          loop i 0 5 {
             set f [lindex $filters $i]
             set mmod [format "%4.2f" [exprGet $el.counts_model<$i>]]
             set line "$line $mmod"
          }
       }

       return $line

}


# wrap around proc flags to read objc_flags2
proc flags2 {objc_flags2} {

  return [flags OBJECT2_FLAGS $objc_flags2] 

}



proc parseRADec {RA Dec} { 

      # convert :: forms
      if [regexp {^([0-9]+):([0-9]+):(.*)} $RA foo hr min sec] {
         foreach el "hr min sec" {
	    regsub {^0*} [set $el] "" $el
         }
         if {$hr == ""} {set hr 0}
         if {$min == ""} {set min 0}
         if {$sec == ""} {set sec 0}
         set RA [format "%11.6f" [expr 15*($hr + ($min + $sec/60.0)/60.0)]]
      }
      if [regexp {^([+-])?([0-9]+):([0-9]+):(.*)} $Dec foo sgn hr min sec] {
         foreach el "hr min sec" {
	    regsub {^0*} [set $el] "" $el
         }
         if {$hr == ""} {set hr 0}
         if {$min == ""} {set min 0}
         if {$sec == ""} {set sec 0}
         set Dec [expr $hr + ($min + $sec/60.0)/60.0]
         if {$sgn == "-"} {
	   set Dec [expr -$Dec]
         }
         set Dec [format "%11.6f" $Dec]
      }

   
   return [list $RA $Dec]
}



### given a file with RA/Dec listing in column colRA and colDec
# (1-indexed), dump psf photometry for all sources from the given 
# run that are within matchRad arcsec from the specified position
# if run < 0, then dump data from all possible runs

proc getID {run camcol field objID {verbose 1} {return_chain 0}} {

global openit
 
      # find and read tsObj file
      set_run $run $camcol -tsObj
      set tsObjdir $openit(objdir)
      set tsObj [glob $tsObjdir/tsObj-[format %06d $run]-$camcol-?-[format %04d $field].fit]
      set chain [tsObj2chain $tsObj]

      set object [chainSearch $chain "{id == $objID} "] 
      set Nfound [chainSize $object]
      if {$Nfound < 1} {
          echo "Couldn't find object $objID (there are [chainSize $chain] objects)"
      } else {
	  set el [chainElementGetByPos $object 0]
          if {$verbose > 0} {
             print_one_photo $el 0 0 0
          } 
      }
     
      # clean
      chainDel $object
      if {!$return_chain} {
          genericChainDestroy $chain
      } else {
          return $chain
      }

} 




### given a file with RA/Dec listing in column colRA and colDec
# (1-indexed), dump psf photometry for all sources from the given 
# run that are within matchRad arcsec from the specified position
# if the file doesn't have column for run, then set colRun to -$run
# e.g.
# photo> set fOFF [list BRIGHT SATUR BLENDED] 
# photo> set fON DEBLENDED_AS_MOVING 
# photo> dumpPHOTOMETRY test.dat -94 11 12 test.par $fON $fOFF 3.0
proc dumpPHOTOMETRY {filein colRun colRA colDec outfile \
			 {flagsON {}} {flagsOFF {}} {matchRad 1.0}} {

global OBJECT1 OBJECT2
global OBJ_TYPE
global openit

    
 
     # split flags according to OBJECT1 vs. OBJECT2 flags
     # all possible flags
     set OBJ1flags [array names OBJECT1]
     set OBJ2flags [array names OBJECT2]
     # get subsets of flagsON and flagsOFF that correspond to OBJ1 and OBJ2
     set flags_ON1 {}
     set flags_ON2 {}
     set flags_OFF1 {}
     set flags_OFF2 {}

     foreach f $flagsON {
	if {[lsearch $OBJ1flags $f] >= 0} {
           lappend flags_ON1 $f
        }
	if {[lsearch $OBJ2flags $f] >= 0} {
           lappend flags_ON2 $f
        }        
     }
     foreach f $flagsOFF {
	if {[lsearch $OBJ1flags $f] >= 0} {
           lappend flags_OFF1 $f
        }
	if {[lsearch $OBJ2flags $f] >= 0} {
           lappend flags_OFF2 $f
        }        
     }

     # translate flags and obj_type
     set fOFF1 [translate_flags $flags_OFF1]
     set fON1 [translate_flags $flags_ON1]
     set fOFF2 [translate_flags $flags_OFF2]
     set fON2 [translate_flags $flags_ON2]
 

     # start output file
     # first some aux information
     set outf [open $outfile "w"]
     puts $outf "# Dumped sources from file $filein using dumpPHOTOMETRY"
     puts $outf "#"
     close $outf
     # write out PHOTOMETRY structure definition
     set hdr ""
     set chain [chainNew PHOTOMETRY]
     chain2Param $outfile $chain $hdr -append
     chainDestroy $chain genericDel

     set infile [open $filein r]
     set Nline 0
     set Nsource 0
     set Ngood 0
     set Nbad 0
     # make sure data dir is kosher
     if {![info exist data_root]} {
         set data_root [get_data_root ""]
     }
     if {![info exist data_root]} {
         error "Specified data_root does NOT exist: $data_root"
     }

     while {![eof $infile]} {
         set line [gets $infile]
         # process line
         if {![eof $infile] && $line != "" && [string range $line 0 0] != "#" } { 
            incr Nline 
	    set RA  [format "%10.5f" [lindex $line [expr $colRA -1]]]
	    set Dec [format "%10.5f" [lindex $line [expr $colDec -1]]]
	    if {$colRun < 0} {
                set run [expr -1*$colRun]
	    } else {
                set run [lindex $line [expr $colRun -1]]
            }
	    echo "processing position $Nline: run=$run ($RA, $Dec]"
            # sanity check for this position
            set rerun [get_highest_dir $data_root/$run]

            # first get pixel coordinates
            set coord [where_in_run $run $RA $Dec -rerun $rerun]
            if {$coord == -1} {
                   echo "                THIS POSITION IS NOT WITHIN SPECIFIED RUN"  
                   incr Nbad
                   continue             
            }
	    set photo [getPHOTOMETRY $run $RA $Dec $matchRad $Nsource \
                                      $fON1 $fOFF1 $fON2 $fOFF2]
	    catch {
	      if {[chainSize $photo] > 0} {
                  set Nfound [chainSize $photo]
                  echo "                found $Nfound matches within $matchRad arcsec"
                  incr Ngood
                  incr Nsource $Nfound
                  chain2Param $outfile $photo "" -append -notypedump
              } else {
                  echo "                NO MATCH FOR THIS POSITION"

              }
              chainDestroy $photo genericDel
            }
	 }
     }

     close $infile

   if {$Nbad > 0} {
       echo "Dumped $Nsource objects, for $Ngood good input positions (out of $Nline), to $outfile"
       echo "*** $Nbad input positions were bogus!?!? ***"
   } else {
       echo "Dumped $Nsource objects for $Ngood input positions (out of $Nline), to $outfile"   
   }
    
}






# given run and a position (RA, Dec), return
# a chain of PHOTOMETRY structures that are within
# $radius arcsec within (RA, Dec)
proc getPHOTOMETRY {run RA Dec {radius 1.0} {id0 0}  \
	     {fON1 {}} {fOFF1 {}} {fON2 {}} {fOFF2 {}} } {

global openit
global fband  

      set good_chain [chainNew PHOTOMETRY]
      set Ngood 0

      # make sure data dir is kosher
      if {![info exist data_root]} {
          set data_root [get_data_root ""]
      }
      if {![info exist data_root]} {
           error "Specified data_root does NOT exist: $data_root"
           return $good_chain
      }

      set RADec [parseRADec $RA $Dec]
      set RA [lindex $RADec 0]; set Dec [lindex $RADec 1]

      set rerun [get_highest_dir $data_root/$run]

      # first get pixel coordinates
      set coord [where_in_run $run $RA $Dec -rerun $rerun]
      if {$coord == -1} {
          return $good_chain
      }
      set camcol [lindex $coord 1]
      set field [lindex $coord 2]
      set row [lindex $coord 3]
      set col [lindex $coord 4]

      # find and read tsObj file
      set_run $run $camcol -tsObj -rerun $rerun 
      set tsObjdir $openit(objdir)
      if [catch {
          set tsObj [glob $tsObjdir/tsObj-[format %06d $run]-$camcol-?-[format %04d $field].fit]
                }] {
          echo " Couldn't find any tsObj files as:"
          echo " $tsObjdir/tsObj-[format %06d $run]-$camcol-?-[format %04d $field].fit"
          return ""
      } 
      set chain [tsObj2chain $tsObj]


      # search for our sources (this should use lambda, eta coordinates but cannot 
      # before astrotools are updated XXX FIX THIS)
      set RAmax  [expr $RA + $radius/3600.0]
      set RAmin  [expr $RA - $radius/3600.0]
      set Decmax [expr $Dec + $radius/3600.0]
      set Decmin [expr $Dec - $radius/3600.0]

      set local [chainSearch $chain "{$RAmin < ra < $RAmax} {$Decmin < dec < $Decmax}"] 
      set filters {u g r i z}

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

      # This loops over all of the elements, checks that the flags are 
      # set/not set as requested, and dumps good ones to the output file
      loop i 0 [chainSize $local] {
	  set source [chainElementGetByPos $local $i]    
          if {![good_source $source "all" $fON1 $fOFF1 $fON2 $fOFF2 1]} {
             continue
          } 
          incr Ngood
          set photo [genericNew PHOTOMETRY]
          # first set overall counter
          handleSet $photo.localID [expr $id0 + $Ngood] 
          # copy over all the fields that exist
          foreach el $Pfields {
              set record $el
              if {[lsearch $Tfields $record] >= 0} {
                  # this field exists, treat specially chromatic fields
                  if {[lsearch $chromatic $record] >= 0} {
                      loop j 0 5 {
                         set rec "$record<$j>"
                         set aux [readSTRUCT TSOBJ $source 0 $rec]
                         handleSet $photo.$el<$j> [format "%6.3f" $aux]
                      }
                  } else {
                      set aux [readSTRUCT TSOBJ $source 0 $record]
                      handleSet $photo.$el $aux
                  }
                  # special case for a single band search
                  if {[info exist fband]} {
                      set iband [lsearch $filters $fband]
                      if {$iband > -1} {
                          echo dump_good_sources: band $fband, record = $record
                      }
                      if {$record == "objc_type" && $iband > -1} {
                          set aux [readSTRUCT TSOBJ $source 0 type<$iband>]
                          handleSet $photo.$el $aux
                      }
                      if {$record == "objc_flags" && $iband > -1} {
                          set aux [readSTRUCT TSOBJ $source 0 flags<$iband>]
                          handleSet $photo.$el $aux
                      }
                  }
              }
          } 
          # all set, add it to the chain
          chainElementAddByPos $good_chain $photo
      }
 
      # clean
      chainDel $local
      genericChainDestroy $chain
    
    return $good_chain

} 






########################################################################################
# aux procs to search par files with PHOTOMATCH structure obtained by proc match_tsObj. 

## select general variables for a given type:

# type = general
#  0) good flags (not BRIGHT, SATUR, EDGE, BLENDED)
#  1) unresolved sources (objc_type=6 for both epochs)
#  2) brighter than r=21.5 (psf mags corrected for reddening) 
#  3) at least 3 sigma variability in gri bands (based on mean sigma for 2 epochs)
#  4) at least 0.1 mag variability in gri bands 

# type = RR Lyrae
#     for RR Lyrae1 (step 1, can also be used for variable QSOs) 
#  0) good flags (not BRIGHT, SATUR, EDGE, BLENDED)
#  1) unresolved sources (objc_type=6 for both epochs)
#  2) brighter than r=22.0 (psf mags corrected for reddening) 
#  3) at least 5 sigma variability in g and r bands (based on mean sigma for 2 epochs)
#  4) at least 0.15 mag variability in g and r bands 
#  5) brighter in r when bluer in g-r
#     and for RR Lyrae2 (step 2) 
#  6) color cuts: 1.0<u-g<1.5, -0.1<g-r<0.4, -0.2<r-i<0.2, -0.2<i-z<0.2

# type = uflare
#  0) good flags (not BRIGHT, SATUR, EDGE, BLENDED)
#  1) unresolved sources (objc_type=6 for both epochs)
#  2) brighter than u=20.0 (psf mags corrected for reddening) 
#  3) at least 5 sigma variability in u band (based on mean sigma for 2 epochs)
#  4) at least 0.2 mag variability in u band

# type = dwarfs, 
# well, these are not variable, rather we look for extremely i-z red sources 
# with consistent detections between the two runs
#  0) good flags (not BRIGHT, SATUR, EDGE, BLENDED)
#  1) unresolved sources (objc_type=6 for both epochs)
#  2) brighter than z=21.0 (psf mags corrected for reddening) 
#  3) i-z > 1.5
#  4) positions consistent within 1 arcsec
# 
proc get_variables {infile {types {general RRLyrae uflare dwarfs}}} {

  foreach type $types {

     ## first select unresolved sources with good flags 
     if {![file exists $infile.stars]} {
         echo "selecting unresolved sources with good flags"
         set flagsOFF {BRIGHT SATUR EDGE BLENDED}
         flag_search $infile $infile.stars $flagsOFF $flagsOFF {} {} 6
     }
    
     if {$type == "general"} {
        echo "selecting general variables"
        ## now search for variables
        set bands {0 1 1 1 0}
        set mag_range {r 14.0 21.5}
        catch {unlink aux1}
        catch {unlink aux2}
        set outf $infile.general
        # need to repeat it 3 times to get delta(m) > 0.10 in 3 bands 
        set ampl {g 0.10}
        selectPHOTOMATCH $infile.stars aux1 3 3 $bands $ampl $mag_range 
        set ampl {r 0.10}
        selectPHOTOMATCH aux1 aux2 3 3 $bands $ampl $mag_range 
        set ampl {i 0.10}
        selectPHOTOMATCH aux2 $outf 3 3 $bands $ampl $mag_range  
     } 

     if {$type == "RRLyrae"} {
        echo "selecting RR Lyrae"
        ## now search for variables
        set bands {0 1 1 0 0}
        set mag_range {r 14.0 22.0}
        # need to repeat it 2 times to get delta(m) > 0.15 in 2 bands 
        catch {unlink aux1}
        set outf1 $infile.RRLyrae1
        set outf2 $infile.RRLyrae2
        set ampl {g 0.15}
        selectPHOTOMATCH $infile.stars aux1 5 2 $bands $ampl $mag_range 
        set ampl {r 0.15}
        set col_range {-10 10 -10 10 -10 10 -10 10}
        selectPHOTOMATCH aux1 $outf1 5 2 $bands $ampl $mag_range $col_range 2 
        # step 2 
        set col_range {1.0 1.5 -0.1 0.4 -0.2 0.2 -0.2 0.2}
        selectPHOTOMATCH $outf1 $outf2 5 2 $bands $ampl $mag_range $col_range
     } 

     if {$type == "uflare"} {
        echo "selecting u-flare stars"
        ## now search for variables
        set bands {1 0 0 0 0}
        set mag_range {u 14.0 20.0}
        set outf $infile.uflare
        set ampl {u 0.2}
        selectPHOTOMATCH $infile.stars $outf 5 1 $bands $ampl $mag_range 
     } 

     if {$type == "dwarfs"} {
        # search for _very_ red sources: i-z > 1.5
        set outf $infile.dwarfs
        selectPHOTOMATCH $infile.stars $outf 0 0 {0 0 0 0 0} {r 0.0} \
                     {z 14.0 21.0} {-10 10 -10 10 -10 10 1.5 10} 0 -1.0         
     }
 
  }

}







#####
##### selecting a subsample for the HET spectroscopy
# 1) consider only not blended sources
# photo> set in Photo5-745-756-all.nocorr.RRLyrae.5
# photo> set out Photo5-745-756-all.nocorr.RRLyrae.5.NOTBLENDED
# photo> flag_search $in $out {BLENDED} {BLENDED}  
#  => 148 sources become a sample of 104 sources  
   
# photo> set s {0 0 0 0 0}
# photo> set allcol {-10 10 -10 10 -10 10 -10 10}
# photo> set f1 Photo5-745-756-all.nocorr.RRLyrae.5.NOTBLENDED 
# photo> set f2 Photo5-745-756-all.nocorr.RRLyrae.5.HET1
# photo> set rA  {r 0.4}
# photo> set rM  {r 18.5 21.0}
#    4 candidates are outside BBB:
# photo> select_variables $f1 $f2 0 0 $s $rA $rM $allcol 0 none 160 205
#    5 candidates are inside BBB:
# photo> set f3 Photo5-745-756-all.nocorr.RRLyrae.5.HETBBB
# photo> select_variables $f1 $f3 0 0 $s $rA $rM $allcol 0 none 210 2



# search for comets
proc select_comets {prefix suffix {cols all} {runs {745-756}}} {

   if {$cols == "all"} {set cols {1 2 3 4 5 6}}

   # some abbreviations
   set no_sigma {0 0 0 0 0}
   set gr_sigma {0 1 1 0 0}
   set all_colors {-10 10 -10 10 -10 10 -10 10}
   set RR_colors {-10 10 -0.1 0.4 -10 10 -10 10}
   set AstarsBox {1.0 1.5 -0.1 0.4 -0.2 0.2 -0.2 0.2}
   set RedClumpBox {1.1 1.3 0.55 0.65 0.15 0.35 -0.2 0.3}
   set RedClumpBox {-10 10 0.45 0.65 -0.2 0.3 -0.2 0.3}
   set col_rangeQSO {-10 0.8 -10 0.8 -10 10 -10 10}
   set flagsOFF {BRIGHT SATUR EDGE BLENDED}


   foreach col $cols {
       echo " ##### COLUMN $col #####"
       foreach run $runs {
           echo "   selecting from $run match"
           set inf $prefix-$run-col$col.$suffix

           ### if not already produced, get *good and *goodstars 
           if {1 && ![file exists $inf.good]} {
              flag_search $inf $inf.good $flagsOFF $flagsOFF
              echo "produced $inf.good"
           }
           if {1 && ![file exists $inf.goodGalaxies]} {
              flag_search $inf.good $inf.goodGalaxies {} {} {} {} 3 
              echo "produced $inf.goodGalaxies"            
           }

           # search for galaxies that moved more than 0.5 arcsec
           select_variables $inf.goodGalaxies $inf.comets 0 0 {0 0 0 0 0} {g 0} \
                            {g 14.0 22.5} $all_colors 0 0.5  
           echo "produced $inf.comets"            
       }
   }

}

# clean *.comets files from faint sources
proc good_comets {prefix suffix {cols all} {runs {745-756}}} {

   if {$cols == "all"} {set cols {1 2 3 4 5 6}}

   foreach col $cols {
       echo " ##### COLUMN $col #####"
       foreach run $runs {
           echo "   selecting good comets for runs $run, column $col"
           set inf $prefix-$run-col$col.$suffix.comets
           set outf $prefix-$run-col$col.$suffix.goodcomets
           catch {unlink aux}
           # first select isolated sources    
           flag_search $inf aux {CHILD} {CHILD} 
           # and then brighter than psf(r) = 21.0 and amplitude
           # less then 0.5 in r
           select_variables aux $outf 0 0 {0 0 0 0 0} {r -0.5} \
                            {r 14.0 21.0} 
       }
   }


}



# fit a power-law to 2 points, and return y(x)
proc powlaw {x1 y1 x2 y2 x0} {

    set Lx1 [expr log10($x1)]
    set Ly1 [expr log10($y1)]
    set Lx2 [expr log10($x2)]
    set Ly2 [expr log10($y2)]

    set a [expr ($Ly2-$Ly1)/($Lx2-$Lx1)]
    set b [expr $Ly2 - $a * $Lx2]

    set Lx0 [expr log10($x0)]
    set Ly0 [expr $a * $Lx0 + $b]

    echo a = $a
    echo b = $b
    echo Lx0 = $Lx0
    echo Ly0 = $Ly0

    return [expr pow(10.0,$Ly0)]

} 




# assumes file structure as in FIRST-SDSS/ControlSample/galaxies.tbl
# lindex $line X to get
#  X record
# -----------
#  0 run
#  1 camCol
#  2 field
#  3 id
#  4 rowc2
#  5 colc2
#  6 flags0
#  7 flags1
#  8 flags2
#  9 flags3
# 10 flags4
# 11 ??????
# 12 flags20
# 13 flags21
# 14 flags22
# 15 flags23
# 16 flags24
# 17 objctype
# 18 type1
# 19 type2
# 20 type3
# 21 ra
# 22 dec
# 23 GL
# 24 GB
# 25 psfCounts0
# 26 psfCountsErr0
# 27 psfCounts1
# 28 psfCountsErr1
# 29 psfCounts2
# 30 psfCountsErr2
# 31 psfCounts3
# 32 psfCountsErr3
# 33 psfCounts4
# 34 psfCountsErr4
# 35 modelCounts0
# 36 modelCountsErr0
# 37 modelCounts1
# 38 modelCountsErr1
# 39 modelCounts2
# 40 modelCountsErr2
# 41 modelCounts3
# 42 modelCountsErr3
# 43 modelCounts4
# 44 modelCountsErr4
# 45 petroCounts0
# 46 petroCountsErr0
# 47 petroCounts1
# 48 petroCountsErr1
# 49 petroCounts2
# 50 petroCountsErr2
# 51 petroCounts3
# 52 petroCountsErr3
# 53 petroCounts4
# 54 petroCountsErr4
# 55 petroR502
# 56 petroR50Err2
# 57 reddening2


# off18 moves the Ia-Ib-Ic regions faint limit up (if > 0)
proc countGalaxies {file Nobj {urMax 10.0} {off18 0.5} {errMax 0.3}} {

    # read data (raw undereddened values)
    if {0} {
    set redd(r) [lindex [file2Vector $file 57 44 [expr 44+$Nobj] 0] 1]  
    set Rmod(u) [lindex [file2Vector $file 35 44 [expr 44+$Nobj] 0] 1]   
    set Rmod(g) [lindex [file2Vector $file 37 44 [expr 44+$Nobj] 0] 1]  
    set err(g)  [lindex [file2Vector $file 38 44 [expr 44+$Nobj] 0] 1]  
    set Rmod(r) [lindex [file2Vector $file 39 44 [expr 44+$Nobj] 0] 1]     
    set err(r)  [lindex [file2Vector $file 40 44 [expr 44+$Nobj] 0] 1]   
    } else { 
     # for galaxiesEDR.tbl from FIRST-SDSS/ControlSample
    set redd(r) [lindex [file2Vector $file 49 44 [expr 44+$Nobj] 0] 1]  
    set Rmod(u) [lindex [file2Vector $file 27 44 [expr 44+$Nobj] 0] 1]   
    set Rmod(g) [lindex [file2Vector $file 29 44 [expr 44+$Nobj] 0] 1]  
    set err(g)  [lindex [file2Vector $file 30 44 [expr 44+$Nobj] 0] 1]  
    set Rmod(r) [lindex [file2Vector $file 31 44 [expr 44+$Nobj] 0] 1]     
    set err(r)  [lindex [file2Vector $file 32 44 [expr 44+$Nobj] 0] 1]     
    }
  
    # correct for reddening 
    set mod(u) [vectorExprEval $Rmod(u)-1.87*$redd(r)] 
    set mod(g) [vectorExprEval $Rmod(g)-1.38*$redd(r)] 
    set mod(r) [vectorExprEval $Rmod(r)-$redd(r)]  
    set ur [vectorExprEval $mod(u)-$mod(r)] 
    set Cmask [vectorExprEval "$ur < $urMax ? 1 : 0"]
    
    # call proc in charge of galaxy classification
    countsGalInRegions $mod(g) $mod(r) $err(g) $err(r) $mod(u) $off18 $errMax $Cmask
 
    # clean 
    vListDel [list $redd(r) $Rmod(u) $Rmod(g) $Rmod(r) $mod(u) $mod(g) $mod(r) $err(g) $err(r) $Cmask]

    return 

} 


# count galaxies in 5 regions of the r vs. g-r color-magnitude diagram
# for modU != "" split region Ib to IbS and IbE regions (u-r cut of 2.22)
# the flux cut for regions I is r < 18-off18
proc countsGalInRegions {modG modR errG errR {modU ""} {off18 0.5} {errMax 0.1} {inMask ""}} {

    set gr [vectorExprEval $modG-$modR] 
    set Nall [exprGet $gr.dimen]

    # basic galaxy mask: err(g,r)<0.1 && mod(r) < 20.0
    if {$inMask == ""} {
       set Gmask [vectorExprEval "$modR > 10 && $modR < 20.0 && $errG < $errMax && $errR < $errMax ? 1 : 0"]
    } else {
       set Gmask [vectorExprEval "$modR > 10 && $modR < 20.0 && $errG < $errMax && $errR < $errMax ? $inMask : 0"]
    }
    set Nobj [vectorExprGet sum($Gmask)] 
    set Ntot 0
    echo "-------"
    echo "  There are $Nall input objects of which $Nobj ([format %6.3f [expr 100.0*$Nobj/$Nall]] %)"
    echo "  are galaxies with 10<mod(r)<20 and err(g,r) < $errMax"
    echo "-------"

    # mag cutoff for regions Ia-Ib-Ic
    set mMax [expr 18.0-$off18]
    if {$modU != ""} {
       set ur [vectorExprEval $modU-$modR]
    }
    
    # region Ia: gr < 0.7 && mod(r) < mMax
    set mask [vectorExprEval "$gr <= 0.7 && $modR <= $mMax ? $Gmask : 0"]
    set N [vectorExprGet sum($mask)] 
    echo "selected $N from region Ia with mod(r) < $mMax ([format %6.3f [expr 100.0*$N/$Nobj]])"
    if {$modU != ""} {
       splitSE $ur $mask $N Ia
    }
    set Ntot [expr $Ntot + $N]; vListDel [list $mask]    
    
    # region Ib: 0.7 < gr < 1.1 && mod(r) < mMax
    set mask [vectorExprEval "$gr > 0.7 && $gr <= 1.1 && $modR <= $mMax ? $Gmask : 0"]
    set N [vectorExprGet sum($mask)] 
    echo "selected $N from region Ib with mod(r) < $mMax ([format %6.3f [expr 100.0*$N/$Nobj]])"
    set Ntot [expr $Ntot + $N]; 
    if {$modU != ""} {
       splitSE $ur $mask $N Ib
    }
    vListDel [list $mask]    

    # region Ic: gr > 1.1 && mod(r) < mMax
    set mask [vectorExprEval "$gr > 1.1 && $modR <= $mMax ? $Gmask : 0"]
    set N [vectorExprGet sum($mask)] 
    echo "selected $N from region Ic with mod(r) < $mMax ([format %6.3f [expr 100.0*$N/$Nobj]])"
    if {$modU != ""} {
       splitSE $ur $mask $N Ic
    }
    set Ntot [expr $Ntot + $N]; vListDel [list $mask]    
    set NtotI $Ntot

    # region IIa: mMax < mod(r) < 20 && g-r < 0.7 
    set mask [vectorExprEval "$modR > $mMax && $gr < 0.7 ? $Gmask : 0"]
    set N [vectorExprGet sum($mask)] 
    echo "selected $N from region IIa with mod(r) > $mMax and g-r<0.7 ([format %6.3f [expr 100.0*$N/$Nobj]])"
    if {$modU != ""} {
       splitSE $ur $mask $N IIa
    }
    set Ntot [expr $Ntot + $N]; vListDel [list $mask]    

    # regions IIb, IIc, IId 
    # magic line for separating regions IIb and IIc (for mMax=17.5: r(L1)=15.3+3.13*gr)
    set L1 [vectorExprEval $mMax+(25-1.25*$mMax)*($gr-0.7)] 
    # magic line for separating regions IIc and IId (for mMax=17.5: r(L1)=14.06+3.13*gr)
    set L2 [vectorExprEval $mMax-1.24+(25-1.25*$mMax)*($gr-0.7)] 
     
    # region IIb: mMax < mod(r) < 20 && g-r > 0.7 && mod(r) > L1
    set mask [vectorExprEval "$modR > $mMax && $gr >= 0.7 && $modR >= $L1 ? $Gmask : 0"]
    set N [vectorExprGet sum($mask)] 
    echo "selected $N from region IIb ([format %6.3f [expr 100.0*$N/$Nobj]])"
    if {$modU != ""} {
       splitSE $ur $mask $N IIb
    }
    set Ntot [expr $Ntot + $N]; vListDel [list $mask]    
    # region IIc: mMax < mod(r) < 20 && mod(r) < L1 && mod(r) > L2
    set mask [vectorExprEval "$modR > $mMax && $modR < $L1 && $modR >= $L2 ? $Gmask : 0"]
    set N [vectorExprGet sum($mask)] 
    echo "selected $N from region IIc ([format %6.3f [expr 100.0*$N/$Nobj]])"
    if {$modU != ""} {
       splitSE $ur $mask $N IIc
    }
    set Ntot [expr $Ntot + $N]; vListDel [list $mask]    
    # region IId: mMax < mod(r) < 20 && mod(r) < L2
    set mask [vectorExprEval "$modR > $mMax && $modR < $L2 ? $Gmask : 0"]
    set N [vectorExprGet sum($mask)] 
    echo "selected $N from region IId ([format %6.3f [expr 100.0*$N/$Nobj]])"
    if {$modU != ""} {
       splitSE $ur $mask $N IId
    }
    set Ntot [expr $Ntot + $N]; vListDel [list $mask]    

    echo "Total classified = $Ntot, out of $Nobj ([format %6.3f [expr 100.0*$Ntot/$Nobj]])"
    echo "Total I = $NtotI, Total II = [expr $Ntot-$NtotI]"

  vListDel [list $Gmask $gr $L1 $L2]    

  return

}

# aux proc
proc splitSE {ur mask N regName} {
       set aux [vectorExprEval "$ur <= 2.22 ? $mask : 0"]
       set Nb [vectorExprGet sum($aux)] 
       if {$N > 0} {
          echo "    selected $Nb from region $regName S ([format %6.3f [expr 100.0*$Nb/$N]])"
       } else {
          echo "    none in region $regName"
       }
       vListDel [list $aux]     
       set aux [vectorExprEval "$ur > 2.22 ? $mask : 0"]
       set Nr [vectorExprGet sum($aux)] 
       if {$N > 0} {
          echo "    selected $Nr from region $regName E ([format %6.3f [expr 100.0*$Nr/$N]])"
       } else {
          echo "    none in region $regName"
       }
       vListDel [list $aux]    
}


# count galaxies detected by FIRST (in 5 regions)
# it is assumed that parfile contains SDSSFIRST data
proc countFIRSTgalaxies {parfile {raMin 0.0} {raMax 360.0} {decMin -90.0} {decMax 90} \
                                 {off18 0.5} {errMax 0.1}} {

    # get chain of SDSSFIRST
    set chain [param2Chain $parfile ""]
    set N [chainSize $chain]
    echo "read in $N sources"

    # select on type 
    set galaxies [chainSearch $chain "{objc_type == 3}"]  
    set Ng [chainSize $galaxies]
    echo "selected $Ng galaxies (cut on objc_type)"

    # get needed vectors
    set redd(r) [vFromChain $galaxies rRedd]   
    set Rmod(u) [vFromChain $galaxies umod]    
    set Rmod(g) [vFromChain $galaxies gmod]   
    set Rmod(r) [vFromChain $galaxies rmod]   
    set err(g)  [vFromChain $galaxies gmodErr]   
    set err(r)  [vFromChain $galaxies rmodErr]   
    set ra   [vFromChain $galaxies ra]    
    set dec [vFromChain $galaxies dec]
    set raFIRST   [vFromChain $galaxies raFIRST]    
    set decFIRST  [vFromChain $galaxies decFIRST]

    # correct for reddening and calculate color
    set mod(u) [vectorExprEval $Rmod(u)-1.87*$redd(r)] 
    set mod(g) [vectorExprEval $Rmod(g)-1.38*$redd(r)] 
    set mod(r) [vectorExprEval $Rmod(r)-$redd(r)] 
 
    # get mask for max distance
    set dRA [vectorExprEval 3600.0*($ra-$raFIRST)]
    set dDec [vectorExprEval 3600.0*($dec-$decFIRST)]
    set dist [vectorExprEval sqrt($dRA*$dRA+$dDec*$dDec)] 
    set raCond "$raMin <= $ra && $raMax >= $ra"
    set decCond "$decMin <= $dec && $decMax >= $dec"
    set mask [vectorExprEval "$dist <= 1.5 && $raCond && $decCond ? 1 : 0"]
    set N [vectorExprGet sum($mask)] 
    echo " there are $N sources with matching distance < 1.5 arcsec and"
    echo " $raMin <= RA <= $raMax and $decMin <= Dec <= $decMax"
     
    # call proc in charge of galaxy classification
    countsGalInRegions $mod(g) $mod(r) $err(g) $err(r) $mod(u) $off18 $errMax $mask
 
    # clean
    vListDel [list $redd(r) $Rmod(u) $Rmod(g) $Rmod(r) $mod(u) $mod(g) $mod(r) $err(g) $err(r)] 
    vListDel [list $ra $dec $raFIRST $decFIRST $dRA $dDec $dist $mask]
    chainDel $galaxies; genericChainDestroy $chain 

    return 

}  




# count galaxies detected by 2MASS (in 5 regions)
# it is assumed that parfile contains SDSS2MASS data
proc count2MASSgalaxies {parfile {raMin 0.0} {raMax 360.0} {decMin -90.0} {decMax 90} \
                                 {off18 0.5} {errMax 0.3}} {

    # get chain of SDSS2MASS
    set chain [param2Chain $parfile ""]
    set N [chainSize $chain]
    echo "read in $N sources"

    # select on type 
    set galaxies [chainSearch $chain "{objc_type == 3}"]  
    set Ng [chainSize $galaxies]
    echo "selected $Ng galaxies (cut on objc_type)"

    # get needed vectors
    set redd(r) [vFromChain $galaxies rRedd]   
    set Rmod(u) [vFromChain $galaxies umod]    
    set Rmod(g) [vFromChain $galaxies gmod]   
    set Rmod(r) [vFromChain $galaxies rmod]   
    set err(g)  [vFromChain $galaxies gmodErr]   
    set err(r)  [vFromChain $galaxies rmodErr]   
    set ra   [vFromChain $galaxies ra]    
    set dec [vFromChain $galaxies dec]
    set ra2MASS   [vFromChain $galaxies ra2MASS]    
    set dec2MASS  [vFromChain $galaxies dec2MASS]

    # correct for reddening and calculate color
    set mod(u) [vectorExprEval $Rmod(u)-1.87*$redd(r)] 
    set mod(g) [vectorExprEval $Rmod(g)-1.38*$redd(r)] 
    set mod(r) [vectorExprEval $Rmod(r)-$redd(r)] 
 
    # get mask for max distance
    set dRA [vectorExprEval 3600.0*($ra-$ra2MASS)]
    set dDec [vectorExprEval 3600.0*($dec-$dec2MASS)]
    set dist [vectorExprEval sqrt($dRA*$dRA+$dDec*$dDec)] 
    set raCond "$raMin <= $ra && $raMax >= $ra"
    set decCond "$decMin <= $dec && $decMax >= $dec"
    set mask [vectorExprEval "$dist <= 1.0 && $raCond && $decCond ? 1 : 0"]
    set N [vectorExprGet sum($mask)] 
    echo " there are $N sources with matching distance < 1 arcsec and"
    echo " $raMin <= RA <= $raMax and $decMin <= Dec <= $decMax"
     
    # call proc in charge of galaxy classification
    countsGalInRegions $mod(g) $mod(r) $err(g) $err(r) $mod(u) $off18 $errMax $mask
 
    # clean
    vListDel [list $redd(r) $Rmod(u) $Rmod(g) $Rmod(r) $mod(u) $mod(g) \
                   $mod(r) $err(g) $err(r)] 
    vListDel [list $ra $dec $ra2MASS $dec2MASS $dRA $dDec $dist $mask]
    chainDel $galaxies; genericChainDestroy $chain 

    return 

}  



### imported from 2MASStools.tcl on Sep 20, 2000 (ZI)

proc sdr {} {
global data_root
   set data_root [get_data_root ""]
}

# find the data
proc get_data_root {root} {
   
    if {$root != ""} {return $root}
 
    set data_roots {}
    # here add all possible data roots
    #lappend data_roots /data/dp3.b/data;         # FNAL dp
    lappend data_roots /data/dp30.a/data;        # FNAL dp
    lappend data_roots /u/dss/data;              # Princeton
    lappend data_roots /usr/sdss/data01/imaging; # FNAL distribution     
        
    foreach root $data_roots {
	if {[file exists $root]} {return $root}
    }
    
    echo "Could not find data, set data_root manually"
    return "" 

}


# given a par file full of (ra,dec) listings get a stamp in the specified
# band and calculate the counts distribution width. If less than skysigma
# call it a blank and dump to file blanks.par, otherwise to sources.par
# n.b. empirically sky_sigma(r) ~ 0.4, and for 6 sigma detection skysigma = 2.4
proc find_blanks {file run {stampsize 5} {Nmax -1} {root ""} {band r} {skysigma 2.4} \
		  {display 0}} {

global data_root table

  
    # find data_root
    if {$root == ""} {
        set data_root [get_data_root ""]
    } else {
        set data_root $root
    }

    set chain [param2Chain $file ""]
    set Ntot [chainSize $chain]
    echo " Read $Ntot sources..."
    if {$Nmax < 0} {set Nmax $Ntot}

    # make chains for 2 types of sources
    set blanks  [chainNew [exprGet $chain.type]]
    set sources [chainNew [exprGet $chain.type]]


    loop i 0 $Ntot {
        set el [chainElementGetByPos $chain $i] 
        set ra [exprGet $el.ra]
        set dec [exprGet $el.dec]
        set run_data [where_in_run $run $ra $dec]
        set camCol [lindex $run_data 1]
        set field [lindex $run_data 2]
        set row [lindex $run_data 3]
        set col [lindex $run_data 4]
        # get stamp
        if {$camCol < 1 || $camCol > 6} {
             echo "for source at ($ra,$dec) camCol = $camCol (run_data = $run_data)"
             continue
        }
        set_run $run $camCol  
        set frame [recon $band $field]
        objfileClose table
        set stamp [stamp_from_region $frame $row $col $stampsize offsets]   
        set stats [regStats $stamp]; set sigma [keylget stats sigma]
        if {$display} {display $stamp "sigma = $sigma"}
        if {$sigma < $skysigma} {  
           # not detected, add it to the chain
           chainElementAddByPos $blanks $el TAIL AFTER
           echo "      source [expr $i + 1] is a blank"
        } else {
           chainElementAddByPos $sources $el TAIL AFTER
           echo "      source [expr $i + 1] is not a blank"
        }
        regDel $stamp;  regDel $frame
    }

    # write to files
    chain2Param blanks.par $blanks ""
    chain2Param sources.par $sources ""
    set Nblanks [chainSize $blanks]
    set Nsources [chainSize $sources] 
    
    # clean
    chainDel $blanks; chainDel $sources
    chainDestroy $chain genericDel

    return [list $Nblanks $Nsources]
}




# assuming file name of the form TWOMASSpsc_hour0_hour1.par_notdetected
# separate the sources into blanks and failed SDSS sources for the 
# given run and dump to files outBlanks and outSources
proc find2MASSblanks_RArange {run h1 h2 outBlanks outSources} {

    echo "   RA1    RA2   all   blanks  failed"
    set Ntot 0
    set NtotB 0    
    set NtotS 0
    loop h $h1 $h2 {
        set hS $h
        set hE [expr $hS+1]
        set file TWOMASSpsc_hour${hS}_hour${hE}.par_notdetected
        set Nlist [find_blanks $file $run]
        set NB [lindex $Nlist 0]
        set NS [lindex $Nlist 1] 
        set Nall [expr $NB + $NS]
        echo "     $hS    $hE   $Nall    $NB     $NS"
        set Ntot [expr $Ntot + $Nall]
        set NtotB [expr $NtotB + $NB] 
        set NtotS [expr $NtotB + $NS] 
        # transfer data to proper files
        set chain [param2Chain blanks.par ""]
        if {$h == $h1} {
            chain2Param $outBlanks $chain ""
        } else {
            chain2Param $outBlanks $chain "" -append -notypedump
        }
        chainDestroy $chain genericDel
        set chain [param2Chain sources.par ""]
        if {$h > $h1} {
            chain2Param $outSources $chain "" -append -notypedump
        } else {
            chain2Param $outSources $chain "" 
        }
        chainDestroy $chain genericDel
    }       
    echo "-----------------------------------"
    echo "      Total    $Ntot    $NtotB  $NtotS"

}





# first read a chain:
# set chain [param2Chain $file ""]
proc plot_2MASS {chain device {Jmax 16.55} {Kmax 15.05} {err_max 1}
		 {symb 4} {plot_all 0}} {

 
     set nall [chainSize $chain]
     # first cut on errors
     # set cond "{J_err < $err_max} {H_err < $err_max} {K_err < $err_max}"
     set cond "{J_err < $err_max}"
     set good [chainSearch $chain $cond]
     set Ngood [chainSize $good] 
     echo "Selected $Ngood sources with err < $err_max from $nall sources" 

     # limit s for plotting colors and magnitudes
     set cmin -0.7
     set cmax  2.2
     set mmin 17
     set mmax  5

     # form vectors
     foreach b {J H K J_err H_err K_err ra dec} {   
        set $b [vFromChain $good $b]
     }
     vNameSet $J "J"; vNameSet $H "H"; vNameSet $K "K"
     set JH [vectorExprEval $J-$H]
     set HK [vectorExprEval $H-$K]
     set JK [vectorExprEval $J-$K]
     vNameSet $JH "J-H"
     vNameSet $HK "H-K"
     vNameSet $JK "J-K"

     # bright sources
     set cond "{J < $Jmax} {K < $Kmax}"
     set goodB [chainSearch $good $cond]
     set NgoodB [chainSize $goodB] 
     echo "Selected $NgoodB sources with J < $Jmax and K < $Kmax from $nall sources" 
     foreach b {J H K ra dec} { 
        set q ${b}B                                        
        set $q [vFromChain $goodB $b]
     }
     vNameSet $JB "J"; vNameSet $HB "H"; vNameSet $KB "K"
     set JHB [vectorExprEval $JB-$HB]
     set HKB [vectorExprEval $HB-$KB]
     set JKB [vectorExprEval $JB-$KB]
     vNameSet $JHB "J-H"
     vNameSet $HKB "H-K"
     vNameSet $JKB "J-K"

 
     # plot color-color diagram 
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device 2MASS_col-col.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb $symb -icError 1 \
	      -xfract 0.4 -yfract 0.4 -nxwindow 1 -nywindow 1 
         pgstateOpen $pg

         # error < $err_max
         vPlot $pg $JHB $HKB -xmin $cmin -xmax $cmax -ymin $cmin -ymax $cmax 
         titlePlot "2MASS: J<$Jmax, K<$Kmax, err<$err_max, n=$NgoodB" 20        
  
         ## plot asteroid's region(s)
         # tight 2MASS box 
         plot_line $pg  0.2 0.25 0.55 0.25 2 2
         plot_line $pg  0.55 0.25 0.65 0.15 2 2
         plot_line $pg  0.65 0.15 0.65 -0.1 2 2
         plot_line $pg  0.65 -0.1 0.2 -0.1 2 2
         plot_line $pg  0.2 -0.1 0.2 0.25 2 2

         # relaxed 2MASS box 
         plot_line $pg  0.0 0.5 0.5 0.5 1 4
         plot_line $pg  0.5 0.5 0.8 0.2 1 4
         plot_line $pg  0.8 0.2 0.8 -0.4 1 4
         plot_line $pg  0.8 -0.4 0.0 -0.4 1 4
         plot_line $pg  0.0 -0.4 0.0 0.5 1 4
      
         endplot $pg
     } msg] {
         echo "$msg"
     }

    
     
     # plot color-magnitude diagrams  
     if [catch {
         if {$plot_all > 0} {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device 2MASS_col-mag.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb $symb -icError 1 \
	      -xfract 0.4 -yfract 0.4 -nxwindow 1 -nywindow 2 
         pgstateOpen $pg

         # J vs. J-K
         vPlot $pg $JK $J -xmin $cmin -xmax $cmax -ymin $mmin -ymax $mmax   
         titlePlot "2MASS: err<$err_max, n=$Ngood" 20  

         # K vs. J-K
         vPlot $pg $JK $K -xmin $cmin -xmax $cmax -ymin $mmin -ymax $mmax 

         endplot $pg
     }
     } msg] {
         echo "$msg"
     }
    
     # plot RA-Dec 
     if [catch {
         if {$plot_all > 1} {
         # Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device 2MASS_RA-Dec.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -symb $symb -icError 1 \
	      -xfract 0.4 -yfract 0.4 -nxwindow 1 -nywindow 2 
         pgstateOpen $pg

         # all
         vPlot $pg $ra $dec  
         titlePlot "2MASS: err<$err_max, n=$Ngood" 20  

         # bright
         vPlot $pg $raB $decB
         titlePlot "2MASS: J<$Jmax, K<$Kmax, err<$err_max, n=$NgoodB" 20     

         endplot $pg
         }
     } msg] {
         echo "$msg"
     }
    
     chainDel $good; chainDel $goodB; 
     vectorExprDel $J; vectorExprDel $H; vectorExprDel $K 
     vectorExprDel $J_err; vectorExprDel $H_err; vectorExprDel $K_err 
     vectorExprDel $JH; vectorExprDel $HK; vectorExprDel $JK
     vectorExprDel $ra; vectorExprDel $dec
     vectorExprDel $raB; vectorExprDel $decB
     vectorExprDel $JB; vectorExprDel $HB; vectorExprDel $KB
     vectorExprDel $JHB; vectorExprDel $HKB; vectorExprDel $JKB
   
}


# given a chain of 2MASS sources, select those with 
# colors consistent with them being asteroids
# for type = 1 use tight box, for type = 2 use relaxed box
proc select2MASSasteroids {inchain outfileRoot {type 1} {Kmax 100}} {

 
    # first select bright candidates     
    set chain [chainSearch $inchain "{K < $Kmax}"]
    echo "selected [chainSize $chain] with K < $Kmax out of [chainSize $inchain]"

    if {$type == 1} {
         # tight 2MASS box 
         set cond ""
         # J-H > 0.2
         lappend cond [list -1 1 0 -0.2]
         # J-H < 0.65
         lappend cond [list 1 -1 0 0.65]
         # H-K > -0.1
         lappend cond [list 0 -1 1 0.1]
         # H-K < 0.25
         lappend cond [list 0 1 -1 0.25]
         # J-K < 0.8
         lappend cond [list 1 0 -1 0.8]
         select2MASS $chain $cond $outfileRoot 
     } else {
         # relaxed 2MASS box 
         set cond ""
         # J-H > 0.0
         lappend cond [list -1 1 0 0.0]
         # J-H < 0.8
         lappend cond [list 1 -1 0 0.8]
         # H-K > -0.4
         lappend cond [list 0 -1 1 0.4]
         # H-K < 0.5
         lappend cond [list 0 1 -1 0.5]
         # J-K < 1.0
         lappend cond [list 1 0 -1 1.0]
         select2MASS $chain $cond $outfileRoot 
     }

} 



# first read a chain:
# set chain [param2Chain $file ""]
# for a list of lists of the form {a1 a2 a3 C}, select all sources
# that satisfy a1*J + a2*H + a3*K < C for each list and that have
# Jerr < errMax etc., and RAmin <= RA <= RAmax and DecMin <= Dec <= DecMax 
proc select2MASS {chain CondList outRoot {errMax 10}  {RAmin 0} {RAmax 360} \
		        {DecMin -90} {DecMax 90}} {

     set nall [chainSize $chain]
     # first cut on positions
     set cond "{ra <= $RAmax} {ra >= $RAmin} {dec >= $DecMin} {dec <= $DecMax}"
     set goodPos [chainSearch $chain $cond]
     set NgoodPos [chainSize $goodPos] 
     echo "Selected $NgoodPos sources within the requested area" 

     # then cut on errors
     set cond "{Jerr < $errMax} {Herr < $errMax} {Kerr < $errMax}"
     set good [chainSearch $goodPos $cond]
     set Ngood [chainSize $good] 
     echo "Selected $Ngood sources with err < $errMax from $NgoodPos sources"

     # form vectors
     foreach b {J H K} {   
         set $b [vFromChain $good $b]
     }
     vNameSet $J "J"; vNameSet $H "H"; vNameSet $K "K"
     set JH [vectorExprEval $J-$H]
     set HK [vectorExprEval $H-$K]
     set JK [vectorExprEval $J-$K]
     vNameSet $JH "J-H"
     vNameSet $HK "H-K"
     vNameSet $JK "J-K"    

     # and then select on all color-magnitude conditions
     set mask [vectorExprNew [exprGet $J.dimen]]
     vectorExprSet $mask 1
     foreach cond $CondList {
	 set a1 [lindex $cond 0]; set a2 [lindex $cond 1]
	 set a3 [lindex $cond 2]; set C [lindex $cond 3]
         set LHS [vectorExprEval $a1*$J+$a2*$H+$a3*$K]
         vectorExprSet $mask "$LHS < $C ? $mask : 0"
     }

     ## and now dump nicely formatted data to output files
     set selected [chainNew [exprGet $good.type]]
     # open text file
     set outf [open $outRoot.dat w] 
     puts $outf "# 2MASS sources selected by:"
     puts $outf "# $RAmin < ra < $RAmax && $DecMin < dec < $DecMax, and err < $errMax, and"  
     puts $outf "# a1*J + a2*H + a3*K < C, where the lists of indices" 
     puts $outf "# {a1 a2 a3 C} are:"
     foreach cond $CondList {
	set n [lsearch $CondList $cond]
        puts $outf "# condition $n: $cond"
     }        
     puts $outf "# "
     puts $outf "#          ra         dec     J    Jerr    H    Herr    K    Kerr  J-H   H-K   J-K"

     set Ngood 0
     loop i 0 [chainSize $good] {
	 if {[exprGet $mask.vec<$i>]} {
             incr Ngood
	     set el [chainElementGetByPos $good $i]
             chainElementAddByPos $selected $el TAIL AFTER 
             # dump to text file
             foreach rec {ra dec J Jerr H Herr K Kerr} {
		 set S$rec [exprGet $el.$rec]
             }
             set sJH [expr $SJ-$SH]; set sHK [expr $SH-$SK]; set sJK [expr $SJ-$SK]
             set coord "[format "%10.6f %10.6f" $Sra $Sdec]"
             set data "[format "%5.2f %5.2f  %5.2f %5.2f  %5.2f %5.2f" \
                           $SJ $SJerr $SH $SHerr $SK $SKerr]"
             set col "[format "%5.2f %5.2f %5.2f" $sJH $sHK $sJK]"
             set line " [format "%4.0f" $Ngood] $coord $data $col"
             puts $outf $line
         }
     }

     # par file
     echo Selected [chainSize $selected] elements
     chain2Param $outRoot.par $selected ""  
     close $outf
    
     chainDel $selected; chainDel $good; chainDel $goodPos; 
     vectorExprDel $J; vectorExprDel $H; vectorExprDel $K 
     vectorExprDel $JH; vectorExprDel $HK; vectorExprDel $JK
     
}



# read a *.par file with fields ra, dec, and run. Call where_in_run
# to get camCol and field 
proc stuff_col_field {file outfile {root ""}} {

global data_root
    
    # find data_root
    if {$root == ""} {
        set data_root [get_data_root ""]
    } else {
        set data_root $root
    }

    set chain [param2Chain $file ""]
    set Ntot [chainSize $chain]
    echo " Read $Ntot sources..."
  
    # loop over all sources
    loop i 0 $Ntot {
        if {[expr $i/100*100] == $i} {echo $i}
        set el [chainElementGetByPos $chain $i]
        catch { 
           set run [exprGet $el.run]
           set ra [exprGet $el.ra]
           set dec [exprGet $el.dec]
           set run_data [where_in_run $run $ra $dec]
           handleSet $el.camCol [lindex $run_data 1]
           handleSet $el.field [lindex $run_data 2]
        }
    }

    echo "done, writing output file..."
    chain2Param outfile $chain ""
    echo "done."

}

    

# read a *.par file with fields ra, dec. Call where_in_run
# for specified run to get camCol, field and pixel coordinates.
# If run is available in *.par file, then set run to -1
proc find_in_run {file run outfile {root ""}} {

global data_root
    
    # find data_root
    if {$root == ""} {
        set data_root [get_data_root ""]
    } else {
        set data_root $root
    }

    # open output file
    set outf [open $outfile w]
    #puts $outf "#   N        RA            Dec       B     run camCol field row col"
    puts $outf "#   N        RA            Dec       run camCol field row col"

    set chain [param2Chain $file ""]
    set Ntot [chainSize $chain]
    echo " Read $Ntot sources..."
  
    # loop over all sources
    set Ngood 0
    loop i 0 $Ntot {
        if {[expr $i/100*100] == $i} {echo $i}
        set el [chainElementGetByPos $chain $i]
        catch { 
	   if {$run == -1} {
               set this_run [exprGet $el.run]
           } else {
               set this_run $run
           }
	   set ra [format "%12.6f" [exprGet $el.ra]]
	   set dec [format "%12.6f" [exprGet $el.dec]]
           set run_data [where_in_run $this_run $ra $dec]
	   if {$run_data != -1} {
               incr Ngood
               set str [format "%5d" $Ngood] 
	       # set B [format "%7.2f" [exprGet $el.bmag]]
               #puts $outf "$str  $ra $dec $B     $run_data"   
               puts $outf "$str  $ra $dec     $run_data"   
           }
        }
    }


    close $outf
    echo "dumped $Ngood positions (out of $Ntot)"

}

    
    

# read a text file with two columns listing ra and dec call 
# where_in_SDSS to get run, camCol, field and pixel coordinates.
# Alternatively, specify the third column with run number and
# save the time by calling where_in_run
proc find_in_run2 {file run outfile {root ""}} {

global data_root
    
     # find data_root
     if {$root == ""} {
        set data_root [get_data_root ""]
     } else {
        set data_root $root
     }

     set outf [open $outfile w]
     puts $outf "#   N        RA            Dec       run camCol field row col"



     set Nline 0
     set infile [open $filein r]
     while {![eof $infile]} {
         set line [gets $infile]
         echo "processing line $Nline"
         # process line
         if {![eof $infile] && [lindex $line 0] != "#" && $line != ""} {  

       # old
        catch { 
	   if {$run == -1} {
               set this_run [exprGet $el.run]
           } else {
               set this_run $run
           }
	   set ra [format "%12.6f" [exprGet $el.ra]]
	   set dec [format "%12.6f" [exprGet $el.dec]]
           set run_data [where_in_run $this_run $ra $dec]
	   if {$run_data != -1} {
               incr Ngood
               set str [format "%5d" $Ngood] 
	       # set B [format "%7.2f" [exprGet $el.bmag]]
               #puts $outf "$str  $ra $dec $B     $run_data"   
               puts $outf "$str  $ra $dec     $run_data"   
           }
        }
       # new
            set RA  [lindex $line [expr $colRA -1]]
            set Dec [lindex $line [expr $colDec -1]]
	    if {$colRun != ""} {
               set run [lindex $line [expr $colRun -1]]  
               set list [where_in_run $run $RA $Dec]             
	    } else {
               set list [where_in_SDSS $RA $Dec]
            }
            if {$list != ""} {
               set NOK 0
	       foreach line $list {
                  incr NOK
                  puts $outf " $NOK $RA $Dec $run $line"
               }
            } else {
               incr Nbad
            }
	 }
         incr Nline
     }

   close $outf
   close $infile
   echo "Did not match $Nbad objects, and dumped [expr $Nline-$Nbad] to $fileout"
    
}





typedef struct {
   int index;
   float value;
} SORT; 


# dump a file with all sources in $run, $columns, $field1-$field2 within $R arcsec
# from a given position, where positions are generated for a rectangular grid with 
# spacing Npix pixels starting from position (100,100) and limited by (1300,2000);
# that is, for e.g Npix=100, the grid contains 13x20=260 positions per field
# dump 
# del_radius  type(band)  psfmag(band)  modmag(band)
proc dump_neighbors {run columns field1 field2 R Npix outfile data_dir \
	             {flags_OFF {BRIGHT SATUR}}} {

global openit
global data_root
global OBJECT1 OBJ_TYPE

     set data_root $data_dir 
 
     # generate positions
     set Ncol [expr 1+int(1999.99 / $Npix)]
     set Nrow [expr 1+int(1299.99 / $Npix)]
     set positions {}
     loop i 0 $Ncol {
         set col [expr 100 + $i*$Npix]
	 loop j 0 $Nrow {
	     set row [expr 100 + $j*$Npix]
             lappend positions [list $row $col]
         }
     }
     set Npos [llength $positions]
     set Nfields [expr [llength $columns]*($field2-$field1+1)]
     set Ntot [expr $Npos*$Nfields]

     set outf [open $outfile w]
     puts $outf "# searched run $run, cols $columns, fields $field1-$field2 at $Ntot"
     puts $outf "# positions on a rectangular grid with delta=$Npix pixels"
     puts $outf "# output contains all sources within a circle with radius $R arcsec" 
     puts $outf "# which have these flags OFF: $flags_OFF"    
     puts $outf "#"
     set psf "psf(u) psf(g) psf(r) psf(i) psf(z)" 
     set mod "mod(u) mod(g) mod(r) mod(i) mod(z)" 
     puts $outf "#  neighbor delta(\")  type(g,r,i)  $psf  $mod"
     set runstr [format %06d $run]
     # maximal distance in pixels
     set delta [expr $R * 2.5]
     set Nfound 0; set Nprimtot 0
     set flagsOFF 0
     foreach flag $flags_OFF {
        set flagsOFF [expr $flagsOFF | $OBJECT1($flag)] 
     } 

     ### loop over columns and fields
     foreach camCol $columns {
         # find and read tsObj file
         set_run $run $camCol -tsObj
         set tsObjdir $openit(objdir) 
         if {$field1 == -1} {
            set field1 [lindex [find_field_range $tsObjdir] 0]
         }
         if {$field2 == -1} { 
            set field2 [lindex [find_field_range $tsObjdir] 1]
         }
         loop field $field1 [expr $field2 + 1] {
             set fstr [format %04d $field]
	     set tsObj [glob $tsObjdir/tsObj-$runstr-$camCol-?-$fstr.fit]
             set chain [tsObj2chain [lindex $tsObj 0]]            
             # we want only primary elements
             set prim [get_primaries $chain]    
             set Nprim [chainSize $prim]
             echo col $camCol field $field ($field1-$field2), Nprim = $Nprim
             set Nprimtot [expr $Nprimtot + $Nprim]
             # search for sources around each position
             foreach position $positions {
		set row [lindex $position 0]
	        set col [lindex $position 1]
                set rowMax [expr $row + $delta]
                set rowMin [expr $row - $delta]
                set colMax [expr $col + $delta]
                set colMin [expr $col - $delta]
		# this is actual search
                set cond "{$rowMin < objc_rowc < $rowMax} {$colMin < objc_colc < $colMax}" 
                set local [chainSearch $prim $cond] 
                set cursor [chainCursorNew $local]
                set index 0
                set sortchain [chainNew SORT]                     
                while {[set source [chainWalk $local $cursor]] != ""} { 
                    if [good_source $source "all" {} $flagsOFF 0] {	             
                        # write output
                        set rDiff [expr [exprGet $source.objc_rowc]-$row] 
                        set cDiff [expr [exprGet $source.objc_colc]-$col] 
                        # real distance in arcsec                         
                        set diff [format "%5.2f" [expr 0.4*sqrt(pow($rDiff,2)+pow($cDiff,2))]]
                        if {$diff < $R} {
                           #echo "searched at ($row, $col), found source $diff arcsec away."
                           set psf ""; set mod ""
                           loop i 0 5 {
			       set psf "$psf [format "%6.2f" [exprGet $source.psfCounts<$i>]]" 
  			       set mod "$mod [format "%6.2f" [exprGet $source.counts_model<$i>]]" 
                           }
                           loop i 1 4 {                        
                              set t($i) [exprGet $source.type<$i>]
                           } 
                           set str($index) "  $diff        $t(1) $t(2) $t(3)   $psf $mod" 
                           set el [genericNew SORT]
                           handleSet $el.index $index
                           handleSet $el.value $diff		
                           chainElementAddByPos $sortchain $el TAIL AFTER                           
                           incr index 
                           incr Nfound  
                        }
                    }
                }
    	        if {[chainSize $sortchain] > 0} { 
                   # sort neighbors
                   chainSort $sortchain value -increasing
                   loop i 0 [chainSize $sortchain] {
                      set el [chainElementGetByPos $sortchain $i]
                      set index [exprGet $el.index]
                      puts $outf "      $i   $str($index)"        
                   }
	       }   
                chainCursorDel $local $cursor
		chainDel $local
                genericChainDestroy $sortchain      
             }
             chainDel $prim
             genericChainDestroy $chain                         
	  }
      }
   
      close $outf
  
      echo "Searched at $Ntot positions"
      echo "and found $Nfound neighbors"
      echo "There are $Nfields fields and $Nprimtot primary objects"
      echo "Your output is in $outfile"

}


# given a chain of objects read from tsObj files
# calculate aperture counts and overwrite them as fiber counts
proc apCounts2tsObj {chain {nann_ap_run 7}} {

     # get radii 
     initProfileExtract
     # fake central pixel
     set radii "0.0 [profileRadii]"      
     set Nap 0

     # loop over all objects
     loop i 0 [chainSize $chain] {
        set obj [chainElementGetByPos $chain $i]
        # loop over filters 
        loop j 0 5 { 
           set Nprof [exprGet $obj.nprof<$j>]  
	   if {$Nprof < $nann_ap_run} {
               # not enough points
               handleSet $obj.fiberCounts<$j> -99.99
               handleSet $obj.fiberCountsErr<$j> -99.99
           } else {
               incr Nap
               set flux 0.0; set fluxErr 0.0
               if [catch {          
                   loop k 0 $nann_ap_run {
                      set r1 [lindex $radii $k]
                      set r2 [lindex $radii [expr $k + 1]]
                      # dA: assuming that the units for profile are something/arcsec^2
                      # Pi*0.396^2 = 0.49265 (radii are in pixels = 0.396 arcsec)
                      set area [expr 0.49265*(pow($r2,2)-pow($r1,2))]   
                      # counts in dA
                      set prof [exprGet $obj.profMean<$j><$k>]  
                      set flux [expr $flux + $area * $prof] 
                      set profErr [exprGet $obj.profErr<$j><$k>]  
                      set fluxErr [expr $fluxErr + $area * $profErr] 
                   } 
                   set apMag [expr -2.5*log10($flux)]
                   set apMagErr [expr $apMag + 2.5*log10($flux+$fluxErr)]
                   handleSet $obj.fiberCounts<$j> $apMag
                   handleSet $obj.fiberCountsErr<$j> $apMagErr
               } msg] {                
                   echo "cannot calculate aperture counts for object $i, in band $j: $msg" 
               }    
           }
        }
     }    

     echo "Successfully computed aperture magnitude for $Nap profiles out of \
           [expr 5*[chainSize $chain]] (=5*[chainSize $chain])"
     return 0       
}


 
# wrapper for producing table with starting and ending positions
proc runs_on_sky {{runs {}} {fileout runs_on_sky.dat} {data_root ""} } {


     if {$data_root == ""} {set data_root [get_data_root ""]}
     set outf [open $fileout w]

     if {$runs == {}} {
         set runs [glob $data_root/\[1-9\]*]
     }
     set runList {}
     foreach run $runs { 
        lappend runList [file tail $run] 
     }
     set sortedRuns [lsort -integer $runList]
     set line "  Run fields   RA1    Dec1   RA2    Dec2     \
                      l1     b1     l2     b2   \
                  lambda1  beta1 lambda2  beta2"
     echo $line
     puts $outf $line     
     foreach run $sortedRuns {
        set runNo [file tail $run]
        set rerun "" 
	set str [run_on_sky $runNo $rerun $data_root]
        set line "[format "%5d" $runNo] $str"
	echo $line
        puts $outf $line
     }

     close $outf
     
}





# for finding the sky area covered by this run
# returns a string "fields RA1 Dec1 RA2 Dec2 l1 b1 l2 b2 
# lambda1 eta1 lambda2 eta2" 
proc run_on_sky {run {rerun ""} {dataRoot /u/dss/data} {asTransFile ""}} {

global data_root


    if {$asTransFile == ""} {
       # data path
       if {$dataRoot != ""} {set data_root $dataRoot}
       set dataRoot $data_root/$run/$rerun

       # look for asTrans file in Astrom
       if {![file exist $dataRoot/astrom/asTrans-[format %06d $run].fit]} {
          # error " Run $run: asTrans file does NOT exist!?"
          return " asTrans file does not exist"
       }
       set asTransFile $dataRoot/astrom/asTrans-[format %06d $run].fit]
    }

    # find the field range from asTrans file
    set hdr [hdrReadAsFits [hdrNew] $asTransFile]
    set f1 [format "%4.0f" [hdrGetAsInt $hdr FIELD0]]
    set f2 [format "%4.0f" [expr $f1 + [hdrGetAsInt $hdr NFIELDS] - 1]]
    hdrDel $hdr

    ### starting position
    # equatorial 
    set eq [pix2eq $run 1 $f1 0 0 r "" "" 0 $asTransFile]  
    set RA1 [lindex $eq 0]; set Dec1 [lindex $eq 1]
    # galactic
    set gal [eqToGal $RA1 $Dec1]
    set l1 [keylget gal gLong]; set b1 [keylget gal gLat]; 
    # ecliptic
    set eclipt [eqToGC $RA1 $Dec1 -node 0 -inclination 23.439291]    
    set lam1 [keylget eclipt mu]; set beta1 [keylget eclipt nu]; 
    # ending position
    set eq [pix2eq $run 1 $f2 0 0 r "" "" 0 $asTransFile]  
    set RA2 [lindex $eq 0]; set Dec2 [lindex $eq 1]
    set gal [eqToGal $RA2 $Dec2]
    set l2 [keylget gal gLong]; set b2 [keylget gal gLat]; 
    # ecliptic
    set eclipt [eqToGC $RA2 $Dec2 -node 0 -inclination 23.439291]    
    set lam2 [keylget eclipt mu]; set beta2 [keylget eclipt nu]; 
 

    # form the return string
    set outf "%6.2f"
    set eq [format "$outf $outf $outf $outf" $RA1 $Dec1 $RA2 $Dec2]   
    set gal [format "$outf $outf $outf $outf" $l1 $b1 $l2 $b2]   
    set eclipt [format "$outf $outf $outf $outf" $lam1 $beta1 $lam2 $beta2]   

    set fieldstring "$f1-$f2"
    return "$fieldstring $eq   $gal   $eclipt"

 
}



# e.g.
# cd asTransDir
# set list [glob asTrans-*fit]
# foreach a $list {
#   set r0 [string range $a 8 13]
#   loop k 0 4 {
#     if {[string range $r0 0 0] == "0"} {set r0 [string range $r0 1 [expr 6-$k]]}
#   }
#   run_boundary $r0 . $a -111
# }
# set allList [glob *-boundary.dat]
# set myList "001035-boundary.dat 001040-boundary.dat"
# plot_runs_boundaries /XWINDOW $allList $myList
# 
# for finding the sky area covered by this run in equatorial, galactic and ecliptic
# coordinates, dumps a table to $dir/$run-boundary.dat with each line containing 
# run field RA Dec l b lambda beta
# e.g. for asTransDir full of asTrans files:
# foreach run $runs {
#    run_boundary $run $asTransDir $asTransDir -999 
# }
#
proc run_boundary {run dir {dataRoot ""} {rerun ""}} {

global data_root

  if {$rerun == -111} {
    set asTransFile $dataRoot
  } elseif {$rerun == -999} {
    set asTransFile $dataRoot/asTrans-[format %06d $run].fit
  } else {
    # data path
    if {$dataRoot != ""} {
         set data_root $dataRoot
    } else {
	set data_root [get_data_root ""]
    }
    set dataRoot $data_root/$run
    if {$rerun != ""} {set dataRoot $dataRoot/$rerun}

    # look for asTrans file in Astrom
    set asTransFile $dataRoot/astrom/asTrans-[format %06d $run].fit
  }

    if {![file exist $asTransFile]} {
       # error " Run $run: asTrans file does NOT exist!?"
       echo " cannot find asTrans file $asTransFile"
       return
   } else {
       # echo " asTrans file is OK: $asTransFile"
   }

    # find the field range from asTrans file
    set hdr [hdrReadAsFits [hdrNew] $asTransFile]
    set f1 [format "%4.0f" [hdrGetAsInt $hdr FIELD0]]
    set f2 [format "%4.0f" [expr $f1 + [hdrGetAsInt $hdr NFIELDS] - 1]]
    hdrDel $hdr

    # open output file
    set outf [open $dir/$run-boundary.dat w]
    puts $outf "# Boundary for run $run, fields $f1-$f2"
    puts $outf "# in equatorial, galactic and ecliptic coordinates"
    puts $outf "# " 
    puts $outf "# run field    RA         Dec         l          b        lambda       beta"
  
    set runFormated [format "%5.0f" $run]
    ### loop over fields, upwards along the left edge of column 1
    loop f $f1 [expr $f2+1] {
        set f [format "%4.0f" $f]
        # equatorial 
        set eq [pix2eq $run 1 $f 0 0 r "" "" 0 $asTransFile]  
        set RA [format "%10.6f" [lindex $eq 0]]; set Dec [format "%10.6f" [lindex $eq 1]]
        # galactic
        set gal [eqToGal $RA $Dec]
        set l [format "%10.6f" [keylget gal gLong]]; set b [format "%10.6f" [keylget gal gLat]]
        # ecliptic
        set eclipt [eqToGC $RA $Dec -node 0 -inclination 23.439291]    
	set lam [format "%10.6f" [keylget eclipt mu]]; set beta [format "%10.6f" [keylget eclipt nu]]
        set line "$runFormated $f $RA $Dec $l $b $lam $beta"
        puts $outf $line
        if {$f == $f1} {
            set line0 $line
        }
    }
    ### loop over fields, downwards along the right edge of column 6
    loop f $f2 [expr $f1-1] -1 {
        set f [format "%4.0f" $f]
        # equatorial 
        set eq [pix2eq $run 6 $f 0 2048 r "" "" 0 $asTransFile]  
        set RA [format "%10.6f" [lindex $eq 0]]; set Dec [format "%10.6f" [lindex $eq 1]]
        # galactic
        set gal [eqToGal $RA $Dec]
        set l [format "%10.6f" [keylget gal gLong]]; set b [format "%10.6f" [keylget gal gLat]]
        # ecliptic
        set eclipt [eqToGC $RA $Dec -node 0 -inclination 23.439291]    
	set lam [format "%10.6f" [keylget eclipt mu]]; set beta [format "%10.6f" [keylget eclipt nu]]
        set line "$runFormated $f $RA $Dec $l $b $lam $beta"
        puts $outf $line
    }
    # for connecting the first and last point
    puts $outf $line0
    close $outf
  
}


# given data_root, and a run list, return a list with all runs 
# that have calibChunks directory 
proc runs_with_tsObj {{runs {}} {data_root ""}} {


     if {$data_root == ""} {set data_root [get_data_root ""]}
     if {$runs == {}} {
         set runs [glob $data_root/\[1-9\]*]
     }
     set good_runs {}   

     foreach run $runs {               
        if {[file exist $run/calibChunks/1]} {lappend good_runs [file tail $run]}
     }

   return $good_runs    

}


# given two coordinate vectors, return two vectors in a new
# coordinate system specified by node and incl
# e.g. equatorial to ecliptic
# set eclip [vCoordTrans $ra $dec 0.0 23.439291]
# or ecliptic to equatorial 
# set equatorial [vCoordTrans $lambda $beta 0.0 -23.439291]
#
proc vCoordTrans {ra dec node incl} {

   set N [exprGet $ra.dimen] 
   set outRA  [vIndex $N]
   set outDec [vIndex $N]

   loop i 0 $N {
      set inRA  [exprGet $ra.vec<$i>]
      set inDec [exprGet $dec.vec<$i>]
      set kl [eqToGC $inRA $inDec -node $node -inclination $incl]
      handleSet $outRA.vec<$i>  [keylget kl mu]
      handleSet $outDec.vec<$i> [keylget kl nu] 
   }

   return "$outRA $outDec"
}


# given equatorial coordinate by vectors, return galactic coordinates
# e.g. 
# set galac [veqToGal $ra $dec]
proc veqToGal {ra dec} {

   set N [exprGet $ra.dimen] 
   set l  [vIndex $N]
   set b [vIndex $N]

   loop i 0 $N {
     set inRA  [exprGet $ra.vec<$i>]
     set inDec [exprGet $dec.vec<$i>]
     set kl [eqToGal $inRA $inDec]
     handleSet $l.vec<$i>  [keylget kl gLong]
     handleSet $b.vec<$i>  [keylget kl gLat] 
  }

   return "$l $b"
}



# given a list of run-boundary.dat files, fileList, produce a 3 panel plot
# showing run boundaries in equatorial, galactic and ecliptic coordinates
proc plot_runs_boundaries {device fileList {fileList2 ""}} {

     ## first read the data for all runs
     set Nruns [llength $fileList]
     if {$fileList2 != ""} {
         set N1 $Nruns
         set Nruns [expr $Nruns + [llength $fileList2]]
         set fileList "$fileList $fileList2"
         set do2 1
     } else {
         set do2 0
     }
     loop i 0 $Nruns {
         set file [lindex $fileList $i]
	 set run($i) [lindex [file2Vector $file 1] 1]
	 set field($i) [lindex [file2Vector $file 2] 1]
	 set RA($i) [lindex [file2Vector $file 3] 1]
	 set Dec($i) [lindex [file2Vector $file 4] 1]
	 set l($i) [lindex [file2Vector $file 5] 1]
	 set b($i) [lindex [file2Vector $file 6] 1]
	 set lambda($i) [lindex [file2Vector $file 7] 1]
	 set beta($i) [lindex [file2Vector $file 8] 1]
     }

 
     vNameSet $RA(0) "RA (deg)"
     vNameSet $Dec(0) "Dec (deg)"
     vNameSet $l(0) "Gal. Long. (deg)"
     vNameSet $b(0) "Gal. Lat. (deg)"
     vNameSet $lambda(0) "Eclipt. Long. (deg)"
     vNameSet $beta(0) "Eclipt. Lat. (deg)"


     ## plot
     if [catch {
         ## Set up the plot 
         set pg [pgstateNew]
         set deviceA [devicename $device runs_boundaries.ps] 
         pgstateSet $pg -device $deviceA -lineWidth 1 -isLine 0 \
                -xfract 0.3 -yfract 0.5 -nxwindow 1 -nywindow 3 -symb 1
         set icLineDef [exprGet $pg.icLine]
         pgstateOpen $pg
         set xmin 330.0; set xmax 30.0; set ymin -70.0; set ymax 70.0

         ## equatorial         
         pgstateSet $pg -isNewplot 1 -icMark 4
         loop i 0 $Nruns {
	    if {$do2 && $i >= $N1} {
                pgstateSet $pg -icMark 2
	    }
            vPlot $pg $RA($i) $Dec($i) -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax
	    pgstateSet $pg -isNewplot 0
         }
         plot_line $pg 0 0 360 0 
         plot_line $pg 0 30 360 30 
         plot_line $pg 0 -30 360 -30 
         plot_line $pg 0 60 360 60 
         plot_line $pg 0 -60 360 -60 
         loop i 0 12 {
	     set x [expr $i*30]
             plot_line $pg $x -90 $x 90 
         }
         ## galactic    
         pgstateSet $pg -isNewplot 1
         loop i 0 $Nruns {
	    if {$do2 && $i >= $N1} {
                pgstateSet $pg -icMark 2
	    }
            vPlot $pg $l($i) $b($i) -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax
            pgstateSet $pg -isNewplot 0  -icMark 4
         }
         plot_line $pg 0 0 360 0 
         plot_line $pg 0 30 360 30 
         plot_line $pg 0 -30 360 -30 
         plot_line $pg 0 60 360 60 
         plot_line $pg 0 -60 360 -60 
         loop i 0 12 {
	     set x [expr $i*30]
             plot_line $pg $x -90 $x 90 
         }       
         ## ecliptic      
         pgstateSet $pg -isNewplot 1
         loop i 0 $Nruns {
	    if {$do2 && $i >= $N1} {
                pgstateSet $pg -icMark 2
	    }
            vPlot $pg $lambda($i) $beta($i) -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax
            pgstateSet $pg -isNewplot 0  -icMark 4
         }

         plot_line $pg 0 0 360 0 
         plot_line $pg 0 30 360 30 
         plot_line $pg 0 -30 360 -30 
         plot_line $pg 0 60 360 60 
         plot_line $pg 0 -60 360 -60 
         loop i 0 12 {
	     set x [expr $i*30]
             plot_line $pg $x -90 $x 90 
         }
         endplot $pg
      } msg] {
         echo "caught error in plot_runs_boundaries"
         echo "$msg"
      }



     ## clean
     loop i 0 $Nruns {
	 vListDel [list $run($i) $field($i) $RA($i) $Dec($i)]
         vListDel [list $l($i) $b($i) $lambda($i) $beta($i)]
     }



}


# given a file with the listing of run and rerun in runCol and 
# rerunCol columns (1-indexed), dump all sources specified with
# flagsON, flagsOFF, objType, and rmodMax (see dump_tsObj for
# details), into files $outfileRoot-$run-$rerun.par, e.g.
#    set fON  BINNED1
#    set fOFF [list BRIGHT SATUR BLENDED] 
#    set rmodMax 21
#    set root brightStars
#  tsObj2PHOTOMETRY runs.DR1.FNAL 3 4 $root $fON $fOFF STAR $rmodMax
#  *** can be used to search for moving objects ***
#  set fOFF [list BRIGHT SATUR BLENDED]; set fON DEBLENDED_AS_MOVING
#  set root movingObjects
#  tsObj2PHOTOMETRY runs.DR1.FNAL 3 4 $root $fON $fOFF all 21.5 
proc tsObj2PHOTOMETRY {infile runCol rerunCol outfileRoot \
                       flagsON flagsOFF objType rmodMax} {

global data_root 

     set inf [open $infile r]
     set Nruns 0 
     set NobjTotal 0 

     # loop over all run/rerun entries
     while {![eof $inf]} {
         set line [gets $inf]
         # process line
         if {![eof $inf] && [string range $line 0 0] != "#"} {  
	     set run [lindex $line [expr $runCol-1]]
	     set rerun [lindex $line [expr $rerunCol-1]]
             echo; echo " **** processing run $run ($rerun) ****" 
             incr Nruns
             loop col 1 7 { 
                 set outfile $outfileRoot-$run-$rerun-col$col
                 dump_tsObj $outfile $run $rerun $col -1 -1 \
                            $flagsON $flagsOFF $objType $rmodMax    
             }
             # all six columns are processed, not concatenate into a single file
             set all [glob $outfileRoot-$run-$rerun-col?.par]
             concat_parfiles $all $outfileRoot-$run-$rerun.par
             set Nlines [lindex [exec wc -l $outfileRoot-$run-$rerun.par] 0]
             set Nobj [expr $Nlines - 44]
             set NobjTotal [expr $NobjTotal + $Nobj]
             foreach f $all {
                exec rm $f
             }
             echo "       made $outfileRoot-$run-$rerun.par"
         }
     }
   
     echo; echo "==== Processed all $Nruns runs from $infile ===="
     echo "==== and found $NobjTotal requested objects ===="
}






# given a file with the listing of run and rerun in runCol and 
# rerunCol columns (1-indexed), dump all sources brighter than
# r=19 into files KBO-$run-$rerun.par, e.g.
#
# tsObj2kbo runs.DR1.FNAL 3 4
proc tsObj2kbo {infile runCol rerunCol} {

global data_root 
global kbo

     set kbo 1

     # KBO search
     set flagsON   BINNED1
     set flagsOFF [list BRIGHT SATUR BLENDED] 
     set rmodMax 19
     set outfileRoot KBO 
     set objType all

     set inf [open $infile r]
     set Nruns 0 
     set NobjTotal 0 

     # loop over all run/rerun entries
     while {![eof $inf]} {
         set line [gets $inf]
         # process line
         if {![eof $inf] && [string range $line 0 0] != "#"} {  
	     set run [lindex $line [expr $runCol-1]]
	     set rerun [lindex $line [expr $rerunCol-1]]
             echo; echo " **** processing run $run ($rerun) ****" 
             incr Nruns
             loop col 1 7 { 
                 set outfile $outfileRoot-$run-$rerun-col$col
                 dump_tsObj $outfile $run $rerun $col -1 -1 \
                            $flagsON $flagsOFF $objType $rmodMax    
             }
             # all six columns are processed, not concatenate into a single file
             set all [glob $outfileRoot-$run-$rerun-col?.par]
             concat_parfiles $all $outfileRoot-$run-$rerun.par
             set Nlines [lindex [exec wc -l $outfileRoot-$run-$rerun.par] 0]
             set Nobj [expr $Nlines - 44]
             set NobjTotal [expr $NobjTotal + $Nobj]
             foreach f $all {
                exec rm $f
             }
             echo "       made $outfileRoot-$run-$rerun.par"
         }
     }
   
     echo; echo "==== Processed all $Nruns runs from $infile ===="
     echo "==== and found $NobjTotal KBO candidates  ===="
}








# for a given run, search tsObj files for moving objects
proc tsObj2moving {outfileroot runs {rerun ""} {cols ""} \
		       {startfield -1} {endfield -1} {type STAR}} {
 
global data_root 

    if {$cols == ""} {set cols {1 2 3 4 5 6}}
    set data_root [get_data_root ""]
    if {$rerun == ""} {  
        set latest 1
    }  else {
        set latest 0
    }

    # set flagsOFF [list SATUR BLENDED EDGE BRIGHT CR] 
    # Mario found CR being off is too conservative (Sep 01)
    set flagsOFF [list BRIGHT SATUR BLENDED] 
    set flagsON DEBLENDED_AS_MOVING

    foreach run $runs {
       # find the highest rerun number if needed
       if {$latest} {   
           set rerun [get_highest_dir $data_root/$run]
           if {$rerun < 0} {
              error "Directory $data_root/$run doesn't contain any reruns!?"
           }  
       } 
       foreach col $cols {
          set file $outfileroot-run$run-col$col
          dump_tsObj $file $run $rerun $col $startfield $endfield \
                     $flagsON $flagsOFF $type 30 1 
       }
    }

    set all [glob $outfileroot-run$run-col?.par]
    concat_parfiles $all $outfileroot-run$run.par

}

# given a chain read from a par file with moving objects
# produce some informative plots
# this assumes that all bad flags are OFF, and that 
# DEBLENDED_AS_MOVING is ON (see tsObj2moving above)
proc plot_moving_ZI {chain device {mBright 21.5} {vMin 0.03} {title ""} {psfile ""} {symbol -1}} {

     ### get vectors
     set run [vFromChain $chain run]
     set field [vFromChain $chain field]
     set g [vFromChain $chain psfCounts<1>]
     set r [vFromChain $chain psfCounts<2>]
     set modr [vFromChain $chain counts_model<2>]
     set rmodr [vectorExprEval $r-$modr]
     set gr [vectorExprEval $g-$r]
     set ra [vFromChain $chain ra]
     set dec [vFromChain $chain dec]
     set rowv [vFromChain $chain rowv]
     set colv [vFromChain $chain colv]
     # transform to deg/day 
      # set vRow [vectorExprEval 0.2643*$rowv]
      # set vCol [vectorExprEval 0.2643*$colv]
     set vRow [vectorExprEval 1.0*$rowv]
     set vCol [vectorExprEval 1.0*$colv]
     set v [vectorExprEval sqrt($vRow*$vRow+$vCol*$vCol)]
     vNameSet $field "field"
     vNameSet $g "g"
     vNameSet $r "r"
     vNameSet $gr "g-r"
     vNameSet $ra "RA (deg)"
     vNameSet $dec "Dec (deg)"
     vNameSet $vCol "vColumn (deg/day)"
     vNameSet $vRow "vRow (deg/day)"
     vNameSet $v "v (deg/day)"
     vNameSet $modr "mod(r)"
     vNameSet $rmodr "psf(r)-mod(r)"

     ## set masks
     # bright
     set mB [vectorExprEval "$r < $mBright ? 1 : 0"]
     # moving 
     set mM [vectorExprEval "$v > $vMin ? $mB : 0"]
     set Ngood [vNnonzero $mM]
     echo "Selected $Ngood good candidates from [chainSize $chain]"


     set startField [vExtreme $field min]
     set endField [vExtreme $field max]
     set nbin [expr int(($endField-$startField+1)/10.0)]
     set RAmin [vExtreme $ra min]
     set RAmax [vExtreme $ra max]  

     ### all read, now plot ###     
     # plot color-color diagrams
     if {$psfile == ""} {set psfile moving.ps}
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $psfile] 
         pgstateSet $pg -device $dev -lineWidth 1 -symb $symbol -icError 1 \
          -xfract 0.3 -yfract 0.48 -nxwindow 2 -nywindow 4
         pgstateOpen $pg

         # vRow vs. vCol
         vPlot $pg $vCol $vRow -xmin -0.5 -xmax 0.5 -ymin -0.5 -ymax 0.5 \
                      -vectorMask $mM 
         plot_line $pg 0 -1 0 1; plot_line $pg -1 0 1 0 

         titlePlot "$title, N=$Ngood" 20      

         # dec vs ra
         vPlot $pg $ra $dec -vectorMask $mM -xmin $RAmax -xmax $RAmin

         # vCol vs. field 
         vPlot $pg $field $vCol -ymin -0.5 -ymax 0.5 -vectorMask $mM 

         # RA histogram
         set raHist [vecToHist $ra $nbin [list $RAmin $RAmax] $mM] 
         handleSet $raHist.xLabel "RA (deg)"
         handleSet $raHist.yLabel "N"
         hgPlot $pg $raHist -xmin $RAmax -xmax $RAmin        
         hgDel $raHist
 
         # field histogram
         set fieldHist [vecToHist $field $nbin [list $startField $endField] $mM]  
         handleSet $fieldHist.xLabel "field"
         handleSet $fieldHist.yLabel "N"
         hgPlot $pg $fieldHist          
         hgDel $fieldHist

         # r vs. g-r
         vPlot $pg $gr $r -xmin 0.0 -xmax 1.5 -ymin 21.5 -ymax 14.0 -vectorMask $mM

         # psf(r)-mod(r) vs. mod(r) 
         vPlot $pg $modr $rmodr -xmin 14.0 -xmax 22.0 -ymin -0.5 -ymax 3.5 -vectorMask $mM 

         # v vs. mod(r)
         vPlot $pg $modr $v  -xmin 14.0 -xmax 22.0 -ymin 0.0 -ymax 0.6 -vectorMask $mM 

         endplot $pg
     } msg] {
         echo "$msg"
     }

     if {0} {
       set goodC [vectorExprEval "$rmodr > 0.2 ? $mM : 0"]
       vectorsWriteToFile [list $run $ra $dec $v $r $modr $rmodr] GoodComets.dat "GoodComets" $goodC 
       vListDel $goodC
     }

     vListDel [list $run $field $g $r $modr $rmodr $gr $ra $dec $rowv $colv $vRow $vCol $v $mB $mM]
    
}



# given a chain read from tsObj files (e.g. by tsObjList2chain) produce
# histograms of rowv and colv for all point source objects.
# produce some informative plots
proc plot_relative_astrom {chain device {title ""} {psfile ""}} {

     ### get vectors
     set g [vFromChain $chain psfCounts<1>]
     set r [vFromChain $chain psfCounts<2>]
     set gr [vectorExprEval $g-$r]
     set vec(rowv) [vFromChain $chain rowv]
     set vec(colv) [vFromChain $chain colv]
     set vec(v) [vectorExprEval sqrt($vec(rowv)*$vec(rowv)+$vec(colv)*$vec(colv))]
     vNameSet $g "g"
     vNameSet $r "r"
     vNameSet $gr "g-r"
     vNameSet $vec(colv) "vColumn (deg/day)"
     vNameSet $vec(rowv) "vRow (deg/day)"
     vNameSet $vec(v) "v (deg/day)"

     ## set masks
     # bright
     set mG [vectorExprEval "$r > 15 && $r < 21.5 ? 1 : 0"]
     set mB [vectorExprEval "$r > 15 && $r < 19 ? 1 : 0"]
     set Nbright [vNnonzero $mB]; set Ngood [vNnonzero $mG]; 
     echo "Selected $Nbright bright, and $Ngood good candidates from [chainSize $chain]"

     ### get some interesting numbers
     set mask [vectorExprEval "$vec(v) > 0.02 && $vec(v) < 0.03 ? $mG : 0"]
     set N [vNnonzero $mask]; vectorExprDel $mask
     set frac [expr 100.0*$N/$Ngood]
     echo "There are $N out of $Ngood sources (${frac}%) with 0.02<v<0.03 (deg/day)"
     set mask [vectorExprEval "$vec(v) > 0.02 && $vec(v) < 0.03 ? $mB : 0"]
     set N [vNnonzero $mask]; vectorExprDel $mask
     set frac [expr 100.0*$N/$Nbright]
     echo "There are $N out of $Nbright sources (${frac}%) with 0.02<v<0.03 (deg/day)"

     ### all read, now plot ###     
     # plot color-color diagrams
     if {$psfile == ""} {set psfile rel_astrom.ps}
     if [catch {
         # Set up the plot 
         set pg [pgstateNew]
         set dev [devicename $device $psfile] 
         pgstateSet $pg -device $dev -lineWidth 1 -isLine 1 -icError 1 \
          -xfract 0.3 -yfract 0.5 -nxwindow 1 -nywindow 4
         pgstateOpen $pg

         foreach type {rowv colv v} {
             # get histograms  
             if {$type != "v"} {
                 set min -0.01; set max 0.01
             } else {
                 set min 0.0; set max 0.03
             }
             pgstateSet $pg -isNewplot 1 -lineWidth 3 -isLine 4
             set hist [vecToHist $vec($type) 100 [list $min $max] $mG] 
             handleSet $hist.xLabel "$type (deg/day)"
             handleSet $hist.yLabel "N"
             hgPlot $pg $hist -xmin $min -xmax $max
             hgDel $hist
             pgstateSet $pg -isNewplot 0 -lineWidth 1 -isLine 2
             set hist [vecToHist $vec($type) 100 [list $min $max] $mB] 
             handleSet $hist.xLabel ""
             handleSet $hist.yLabel ""
             hgPlot $pg $hist -xmin $min -xmax $max
             hgDel $hist
             if {$type == "rowv"} {titlePlot "$title, N=$Ngood, Nb=$Nbright" 20}      
	 }
       
         endplot $pg
     } msg] {
         echo "$msg"
     }

     vListDel [list $g $r $gr $vec(rowv) $vec(colv) $vec(v) $mB]
    
}


# very aux proc
# take a par file with PHOTOMETRY structure and dump a *.dat file 
# compatible with SX outputs and SM plotting procs 
proc PHOTOMETRY2ast {parfile outfile {add_mod 0}} {

    # read data
    set chain [param2Chain $parfile ""]
    set Nel [chainSize $chain]
    echo "read in $Nel elements"

    # open output file
    set outf [open $outfile w]
    puts $outf "# FAKE SX OUTPUT PRODUCED BY PHOTOMETRY2ast"
    puts $outf "#   field.segment.run, field.segment.camCol, field.field,"
    puts $outf "#   obj.id, obj.rowc\[2\], obj.colc\[2\], obj.objc_type,"
    puts $outf "#   obj.rowv, obj.colv, obj.rowvErr, obj.colvErr,"
    puts $outf "#   obj.ra, obj.dec, GL(), GB(), obj.reddening\[2\],"
    puts $outf "#   obj.psfCounts\[0\], obj.psfCountsErr\[0\],"
    puts $outf "#   obj.psfCounts\[1\], obj.psfCountsErr\[1\],"
    puts $outf "#   obj.psfCounts\[2\], obj.psfCountsErr\[2\],"
    puts $outf "#   obj.psfCounts\[3\], obj.psfCountsErr\[3\],"
    puts $outf "#   obj.psfCounts\[4\], obj.psfCountsErr\[4\]"

    set recs(1) [list run camCol field id objc_rowc objc_colc objc_type]
    set recs(2) [list rowv colv rowvErr colvErr ra dec dummy dummy reddening]

    # loop over all elements and dump data
    loop i 0 $Nel {
	set el [chainElementGetByPos $chain $i]
        set line ""
        foreach k {1 2} {
           foreach rec $recs($k) {
	       if {$rec == "dummy"} {
                   set line "$line -999"
               } else {
                   if {$rec != "reddening"} {
                       set line "$line [exprGet $el.$rec]"
                   } else {
                       set line "$line [exprGet $el.reddening<2>]"
                   }
               }
           }
        }
        loop j 0 5 { 
           set line "$line [exprGet $el.psfCounts<$j>]"
           set line "$line [exprGet $el.psfCountsErr<$j>]"
        }
        if {$add_mod} {
           loop j 0 5 { 
              set line "$line [exprGet $el.counts_model<$j>]"
              set line "$line [exprGet $el.counts_modelErr<$j>]"
           }
        }
        puts $outf $line
    }

    genericChainDestroy $chain
    close $outf

    
}

# very aux proc
# take a par file with PHOTOMETRY structure and dump a *.dat file 
# compatible with SX outputs and SM plotting procs 
proc PHOTOMETRY2star {parfile outfile} {

    # read data
    set chain [param2Chain $parfile ""]
    set Nel [chainSize $chain]
    echo "read in $Nel elements"

    # open output file
    set outf [open $outfile w]
    puts $outf "# FAKE SX OUTPUT PRODUCED BY PHOTOMETRY2star"
    puts $outf "#   field.segment.run, field.segment.camCol, field.field,"
    puts $outf "#   obj.rowc\[2\], obj.colc\[2\], obj.objc_type,"
    puts $outf "#   obj.ra, obj.dec, GL(), GB(), obj.reddening\[2\],"
    puts $outf "#   obj.psfCounts\[0\], obj.psfCountsErr\[0\],"
    puts $outf "#   obj.psfCounts\[1\], obj.psfCountsErr\[1\],"
    puts $outf "#   obj.psfCounts\[2\], obj.psfCountsErr\[2\],"
    puts $outf "#   obj.psfCounts\[3\], obj.psfCountsErr\[3\],"
    puts $outf "#   obj.psfCounts\[4\], obj.psfCountsErr\[4\]"
    puts $outf "#   id, parent, nchild, firstMatch, rosatMatch, pmMatch, primT, secT"
    puts $outf "#   flags, flags2, SATUR, BLND, CHILD, CR, INTERP, DBL_MV, EDGE"
    puts $outf "#   pmDelta, pm, pmAngle, mJ, mF"


    set recs(1) [list run camCol field objc_rowc objc_colc objc_type]
    set recs(2) [list ra dec galCoord reddening]
    set recs(3) [list id parent nchild firstMatch rosatMatch propermotionmatch \
                      primTarget secTarget objc_flags objc_flags2]
    set recs(4) [list propermotiondelta propermotion propermotionangle usnoBlue usnoRed]


    # loop over all elements and dump data
    loop i 0 $Nel {
	set el [chainElementGetByPos $chain $i]
        set line ""
        set cCfNotSet 1
        foreach k {1 2} {
           foreach rec $recs($k) {
               if {$rec == "camCol" || $rec == "field"} {
                    if {1 && $cCfNotSet} {
                        catch { 
                            set run [exprGet $el.run]
                            set ra [exprGet $el.ra]
                            set dec [exprGet $el.dec]
                            set run_data [where_in_run $run $ra $dec]
                            handleSet $el.camCol [lindex $run_data 1]
                            handleSet $el.field [lindex $run_data 2]
                            set cCfNotSet 0 
                        }
                    }
		    set line "$line [exprGet $el.$rec]"
	       } elseif {$rec == "galCoord"} {
                    set ra [exprGet $el.ra]
                    set dec [exprGet $el.dec]
                    set galCoord [eqToGal $ra $dec]
		    set gLong [format "%10.6f" [keylget galCoord gLong]]
		    set gLat [format "%10.6f" [keylget galCoord gLat]]
                    set line "$line $gLong $gLat"
               } elseif {$rec == "ra" || $rec == "dec"} {
                    set val [format "%10.6f" [exprGet $el.$rec]]
                    set line "$line $val"
               } elseif {$rec == "reddening"} {
                    set redd [format "%4.2f" [exprGet $el.reddening<2>]]
                    set line "$line $redd"
               } else {
                    set val [format "%5.0f" [exprGet $el.$rec]]
                    set line "$line $val"
               }
           }
        }
        loop j 0 5 { 
            set m [format "%5.2f" [exprGet $el.psfCounts<$j>]]
	    set line "$line $m"
            set mErr [format "%5.2f" [exprGet $el.psfCountsErr<$j>]]
	    set line "$line $mErr"
        }
        foreach rec $recs(3) {
	    if {[lsearch "id parent" $rec] >= 0} {
                set Frec [format "%5.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } elseif {[lsearch "primTarget secTarget objc_flags objc_flags2" $rec] >= 0} {
                set Frec [format "%12.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } elseif {$rec == "nchild"} {
                set Frec [format "%3.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } else {
                set Frec [format "%1.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            }
        }

        # translate special flags:
        set flags [exprGet $el.objc_flags]
        set flags2 [exprGet $el.objc_flags2]
        foreach flag "SATUR BLENDED CHILD CR INTERP EDGE" {
            if {[expr $flags & [translate_flags $flag]]} {
                set line "$line 1"
            } else {
                set line "$line 0"
            }
	}
        if {[expr $flags2 & [translate_flags DEBLENDED_AS_MOVING]]} {
            set line "$line 1"
        } else {
            set line "$line 0"
        }     

        foreach rec $recs(4) {
	    if {[lsearch "usnoBlue usnoRed" $rec] >= 0} {
                set Frec [format "%5.2f" [exprGet $el.$rec]]
                if {$Frec < 0} {set Frec -99.99}
                if {$Frec > 30} {set Frec " 99.99"}
                set line "$line $Frec"
            } else {
                set Frec [format "%6.2f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } 
        }
        # the output line is now assembled:
        puts $outf $line
    }

    genericChainDestroy $chain
    close $outf
    
    echo "Processed $Nel objects from $parfile and dumped to $outfile"
  
}


# analogous to PHOTOMETRY2star, except that it dumps model mags
# used to analyze POSS completeness
# take a par file with PHOTOMETRY structure and dump a *.dat file 
# compatible with SX outputs and SM plotting procs 
proc PHOTOMETRY2all {parfile outfile} {

    # read data
    set chain [param2Chain $parfile ""]
    set Nel [chainSize $chain]
    echo "read in $Nel elements"

    # open output file
    set outf [open $outfile w]
    puts $outf "# FAKE SX OUTPUT PRODUCED BY PHOTOMETRY2star"
    puts $outf "#   field.segment.run, field.segment.camCol, field.field,"
    puts $outf "#   obj.rowc\[2\], obj.colc\[2\], obj.objc_type,"
    puts $outf "#   obj.ra, obj.dec, GL(), GB(), obj.reddening\[2\],"
    puts $outf "#   obj.counts_model\[0\], obj.counts_modelErr\[0\],"
    puts $outf "#   obj.counts_model\[1\], obj.counts_modelErr\[1\],"
    puts $outf "#   obj.counts_model\[2\], obj.counts_modelErr\[2\],"
    puts $outf "#   obj.counts_model\[3\], obj.counts_modelErr\[3\],"
    puts $outf "#   obj.counts_model\[4\], obj.counts_modelErr\[4\]"
    puts $outf "#   id, parent, nchild, firstMatch, rosatMatch, pmMatch, primT, secT"
    puts $outf "#   flags, flags2, SATUR, BLND, CHILD, CR, INTERP, DBL_MV, EDGE"
    puts $outf "#   pmDelta, pm, pmAngle, mJ, mF"


    set recs(1) [list run camCol field objc_rowc objc_colc objc_type]
    set recs(2) [list ra dec galCoord reddening]
    set recs(3) [list id parent nchild firstMatch rosatMatch propermotionmatch \
                      primTarget secTarget objc_flags objc_flags2]
    set recs(4) [list propermotiondelta propermotion propermotionangle usnoBlue usnoRed]


    # loop over all elements and dump data
    loop i 0 $Nel {
	set el [chainElementGetByPos $chain $i]
        set line ""
        set cCfNotSet 1
        foreach k {1 2} {
           foreach rec $recs($k) {
               if {$rec == "camCol" || $rec == "field"} {
                    if {1 && $cCfNotSet} {
                        catch { 
                            set run [exprGet $el.run]
                            set ra [exprGet $el.ra]
                            set dec [exprGet $el.dec]
                            set run_data [where_in_run $run $ra $dec]
                            handleSet $el.camCol [lindex $run_data 1]
                            handleSet $el.field [lindex $run_data 2]
                            set cCfNotSet 0 
                        }
                    }
		    set line "$line [exprGet $el.$rec]"
	       } elseif {$rec == "galCoord"} {
                    set ra [exprGet $el.ra]
                    set dec [exprGet $el.dec]
                    set galCoord [eqToGal $ra $dec]
		    set gLong [format "%10.6f" [keylget galCoord gLong]]
		    set gLat [format "%10.6f" [keylget galCoord gLat]]
                    set line "$line $gLong $gLat"
               } elseif {$rec == "ra" || $rec == "dec"} {
                    set val [format "%10.6f" [exprGet $el.$rec]]
                    set line "$line $val"
               } elseif {$rec == "reddening"} {
                    set redd [format "%4.2f" [exprGet $el.reddening<2>]]
                    set line "$line $redd"
               } else {
                    set val [format "%5.0f" [exprGet $el.$rec]]
                    set line "$line $val"
               }
           }
        }

        loop j 0 5 { 
            set m [format "%5.2f" [exprGet $el.counts_model<$j>]]
	    set line "$line $m"
            set mErr [format "%5.2f" [exprGet $el.counts_modelErr<$j>]]
	    set line "$line $mErr"
        }
        foreach rec $recs(3) {
	    if {[lsearch "id parent" $rec] >= 0} {
                set Frec [format "%5.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } elseif {[lsearch "primTarget secTarget objc_flags objc_flags2" $rec] >= 0} {
                set Frec [format "%12.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } elseif {$rec == "nchild"} {
                set Frec [format "%3.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } else {
                set Frec [format "%1.0f" [exprGet $el.$rec]]
                set line "$line $Frec"
            }
        }

        # translate special flags:
        set flags [exprGet $el.objc_flags]
        set flags2 [exprGet $el.objc_flags2]
        foreach flag "SATUR BLENDED CHILD CR INTERP EDGE" {
            if {[expr $flags & [translate_flags $flag]]} {
                set line "$line 1"
            } else {
                set line "$line 0"
            }
	}
        if {[expr $flags2 & [translate_flags DEBLENDED_AS_MOVING]]} {
            set line "$line 1"
        } else {
            set line "$line 0"
        }     

        foreach rec $recs(4) {
	    if {[lsearch "usnoBlue usnoRed" $rec] >= 0} {
                set Frec [format "%5.2f" [exprGet $el.$rec]]
                if {$Frec < 0} {set Frec -99.99}
                if {$Frec > 30} {set Frec " 99.99"}
                set line "$line $Frec"
            } else {
                set Frec [format "%6.2f" [exprGet $el.$rec]]
                set line "$line $Frec"
            } 
        }
        # the output line is now assembled:
        puts $outf $line
        # clean 
        handleDel $el
    }

    genericChainDestroy $chain
    close $outf
    
    echo "Processed $Nel objects from $parfile and dumped to $outfile"
  
}




# e.g. set c [param2Chain ControlVariables125-7vs1755-0.par ""]  
# proc test_overlap $c
proc test_overlap {chain {rootname test_overlap} {title ""} {rmax 19} {device /XWINDOW}} {

     set vList [list mg mr mi dr ra raB dec decB]
     foreach q $vList {                                  
         set v($q) [vFromChain $chain $q]
     }


     set gi [vectorExprEval $v(mg)-$v(mi)]
     set delra [vectorExprEval 3600*($v(ra)-$v(raB))]
     set deldec [vectorExprEval 3600*($v(dec)-$v(decB))]
     set dist [vectorExprEval sqrt($delra*$delra+$deldec*$deldec)]
     set Bmask [vectorExprEval "$v(mr) < $rmax ? 1 : 0"]            

     vNameSet $gi "g-i"
     vNameSet $dist "motion (arcsec)"
     vNameSet $v(mr) "mean r"
     vNameSet $v(dr) "r2-r1"
 
     # all data
     plot_xy $v(mr) $v(dr) $device $title "points" 1 1  14 23 -0.5 0.5
     exec mv plot_xy.ps $rootname-Ar-r.ps; echo made $rootname-Ar-r.ps
     plot_xy $v(mr) $dist  $device $title "points" 1 1 14 23 0 1 
     exec mv plot_xy.ps $rootname-r-motion.ps; echo made $rootname-r-motion.ps
     # masked data
     plot_xy $gi    $v(dr) $device $title "points" 1 1 -0.5 3 -0.5 0.5 $Bmask 
     exec mv plot_xy.ps $rootname-gi-Ar.ps; echo made $rootname-gi-Ar.ps
     plot_xy $gi    $dist  $device $title "points" 1 1 -0.5 3    0   1 $Bmask
     exec mv plot_xy.ps $rootname-gi-motion.ps; echo made $rootname-gi-motion.ps
     plot_xy $v(dr) $dist  $device $title "points" 1 1 -0.5 0.5  0   1 $Bmask
     exec mv plot_xy.ps $rootname-Ar-motion.ps; echo made $rootname-Ar-motion.ps

     foreach q $vList {
        vectorExprDel $v($q)
     }
     foreach q [list $gi $delra $deldec $dist $Bmask] {
        vectorExprDel $q
     }

}


### aux: to test mpsf-mmod vs. master row
# photo> set list [glob 3/fpObjc-000745-3-03* 3/fpObjc-000745-3-041*]
# photo> set c [tsObjList2chain $list] 
# photo> make_float_field $c; ### OR make_master_row $c
# photo> apCounts2tsObj $c
# photo> set filter r     
# photo> set ifilter [lsearch {u g r i z} $filter] 
# photo> set sB [chainSearch $c "{objc_type == OBJ_STAR} {psfCounts<$ifilter> > 10000}"]
# photo> set row [vFromChain $sB rowc<$ifilter>]
# photo> set mpsf [vFromChain $sB psfCounts<$ifilter>]
# photo> set mmod [vFromChain $sB counts_model<$ifilter>]
# photo> set mfib [vFromChain $sB fiberCounts<$ifilter>]
# photo> set pmm [vectorExprEval -2.5*lg($mpsf/$mmod)] 
# photo> set pmf [vectorExprEval -2.5*lg($mpsf)-$mfib+2.007] 
# photo> set fmm [vectorExprEval $mfib-2.007+2.5*lg($mmod)] 
# photo> vNameSet $row "master row"
# photo> vNameSet $pmm "mpsf($filter)-mmod($filter)"
# photo> vNameSet $pmf "mpsf($filter)-maper($filter)"
# photo> vNameSet $fmm "maper($filter)-mmod($filter)"
# photo> set title "745(t111)-3r: 395-409"
# photo> set dev /XWINDOW 
# photo> plot_histogram $dev $mmp 100 -0.05 0.05
# photo> plot_xy $row $pmm $dev $title 1 1 1 0 [expr 1361*15] -0.1 0.1                 
# photo> plot_xy $row $pmf $dev $title 1 1 1 0 [expr 1361*15] -0.1 0.1                 
# photo> plot_xy $row $fmm $dev $title 1 1 1 0 [expr 1361*15] -0.1 0.1                 


proc testApCorr {c filter fieldMin fieldMax {dev /XWINDOW} {title ""}} {

     set ifilter [lsearch {u g r i z} $filter] 
     set sB [chainSearch $c "{objc_type == OBJ_STAR} {psfCounts<$ifilter> > 10000}"] 
     set mpsf [vFromChain $sB psfCounts<$ifilter>]
     set mfib [vFromChain $sB fiberCounts<$ifilter>]
     set pmf [vectorExprEval -2.5*lg($mpsf)-$mfib+2.007] 
     set row [vFromChain $sB objc_rowc]
     vNameSet $row "field"
     vNameSet $pmf "mpsf($filter)-maper($filter)"
     plot_xy $row $pmf $dev $title 4 1 1 $fieldMin $fieldMax -0.1 0.1


} 


proc testModPsf {c filter fieldMin fieldMax {dev /XWINDOW} {title ""}} {

     set ifilter [lsearch {u g r i z} $filter] 
     set sB [chainSearch $c "{objc_type == OBJ_STAR} {psfCounts<$ifilter> > 10000}"] 
     set mpsf [vFromChain $sB psfCounts<$ifilter>]
     set mmod [vFromChain $sB counts_model<$ifilter>]
     set mmp [vectorExprEval -2.5*lg($mmod/$mpsf)] 
     make_float_field $sB $fieldMin
     set row [vFromChain $sB objc_rowc]
     vNameSet $row "field"
     vNameSet $mmp "mmod($filter)-mpsf($filter)"
     plot_xy $row $mmp $dev $title 4 1 1 $fieldMin $fieldMax -0.1 0.1


} 


# take filein and copy its content to fileout except for the blank lines
# if remstruct == 1, do not copy lines ending with a ";"
proc remlines {filein fileout {remstruct 0} } {

     set infile [open $filein r]
     set outf [open $fileout w]
     
     set Nall 0; set Nempty 0

     while {![eof $infile]} {
         set line [gets $infile]
         if {![eof $infile] && $line != ""} {  
	    if {$remstruct && [lindex $line 0] == "typedef" && [lindex $line 1] == "struct"} {
               incr Nempty
               continue
            }
	    if {$remstruct && [string match *\;* $line]} {
               incr Nempty
               continue
            }
            puts $outf $line   
            incr Nall
         } else {
            incr Nempty
         }
     }
 
     close $outf
     close $infile
  
     echo "found $Nempty lines, and copied over $Nall lines"  
}


# take filein and copy its content to fileout except for the 
# commented (#) lines
# if copyFirst == 1, copy the first line starting with "#"
proc rem_comments {filein fileout {copyFirst 1}} {

     set infile [open $filein r]
     set outf [open $fileout w]
     
     set Nall 0; set Nempty 0; set Ncomm 0

     while {![eof $infile]} {
         set line [gets $infile]
         if {![eof $infile] && $line != ""} {  
	    if {[string range $line 0 0] == "#"} {
		if {$copyFirst} {
                  puts $outf $line   
                  set copyFirst 0
                  continue
                } else {
                  incr Ncomm
                  continue
	        }
	    } else {
                puts $outf $line   
                incr Nall
            }
         } else {
            incr Nempty
         }
     }
 
     close $outf
     close $infile
  
     echo "removed $Ncomm comments, found $Nempty empty lines, and copied over $Nall lines"  
}




# given a dat file (e.g. SDSSVAR processed by remlines), dump a subset
# of sources that have $flagsON set on, and $flagsOFF set off
# flags are given in 1-indexed columns specified in $flagColList
#  
# e.g. for SDSSVAR.dat file (i.e no structure def)
# set flagList "12 34"; set flagsON "BINNED1"
# set flagsOFF "BRIGHT SATUR BLENDED CR"  
# cutflags All745-20vs756-20.dat All745-20vs756-20-clean.dat \
#          $flagColList 0 $flagsOFF $flagsON
#
# or for impecable photometry (obtained from opdb by e.g. getPHOTOMETRY)
#
# set flagsOFF "BRIGHT SATUR BLENDED CR NOTCHECKED INTERP MOVED BADSKY"
# cutflags springEq_run752.par springEq_run752_clean.par 12 36 $flagsOFF BINNED1
# and then e.g.
# remlines springEq_run752_clean.par springEq_run752_clean.dat 1

proc cutflags {filein fileout flagColList {Ncopy 0} {flagsOFF ""} {flagsON ""}} {

     set infile [open $filein r]
     set outf [open $fileout w]
     puts $outf "# output from cutflag proc with:"
     puts $outf "# flagsOFF = $flagsOFF"
     puts $outf "#  flagsON = $flagsON"
     
     set Nline 0; set Nall 0; set Ngood 0; set Nempty 0
     set fOFF [translate_flags $flagsOFF]
     set fON [translate_flags $flagsON]
    
     while {![eof $infile]} {
        set line [gets $infile]
        incr Nline 
	if {$Nline <= $Ncopy} {
            puts $outf $line
            continue
        }
        if {![eof $infile] && $line != "" && [string range $line 0 0] != "#"} {  
            incr Nall
            set good 1
   	    foreach flagCol $flagColList {
	        set flag [lindex $line [expr $flagCol-1]]
                # must be ON for this source to be good, i.e.
                # if OFF the source is bad
                if {![are_flagsOK $flag $fON $fOFF]} {
                    set good 0
                    continue
                }
            }
            # done with flag checking, if OK dump it                     
	    if {$good} {
                puts $outf $line
                incr Ngood
            }   
         } else {
            incr Nempty
         }
     }
 
     close $outf
     close $infile
  
     echo "processed $Nall lines, and selected $Ngood with requested flags"  
     echo "$Nempty lines ignored."  
}


# clean a file with PHOTOMETRY dumped from opdb
proc cleanPHOTOMETRY {infile outfile} {

    # first get rid of moving objects
    cutflags $infile aux 13 36 "DEBLENDED_AS_MOVING" ""      
    # next get rid of known bad sources
    set flagsOFF "BRIGHT SATUR BLENDED NOTCHECKED" 
    cutflags aux $outfile 12 38 $flagsOFF BINNED1
    # clean
    exec rm aux

}


# clean a file with SDSSVAR dumped from opdb
proc cleanSDSSVAR {infile outfile} {

    # based on QSO target selection
    set flagsOFF "BRIGHT SATUR EDGE BLENDED" 
    # before we have DR1 data, be more cautious
    set flagsOFF "$flagsOFF CR NOTCHECKED INTERPOLATED"  
    cutflags $infile $outfile "12 34" 95 $flagsOFF BINNED1

}


# take a par file and make an SM reading macro
proc par2sm {file outfile} {  

      # first get a list of elements
      set c  [param2Chain $file ""]
      set el [chainElementGetByPos $c 0]
      set L  [exprGet $el]
      set name [lindex [lindex [exprGet $c] 1] 1]
      handleDel $el
      chainDel $c

      # translate 
      set L1 ""
      set L2 ""
      loop i 0 [llength $L] {
          set L1 "$L1 [lindex [lindex $L $i] 0]_\$2 [expr $i+2]"   
          set L2 "$L2 [lindex [lindex $L $i] 0]"   
      } 


      # make output file
      set outf [open $outfile w]
     
      puts $outf "read$name 2" 
      puts $outf ""
      puts $outf "      \# read raw data"
      puts $outf "      define file \"\$!1\" "     
      puts $outf "      data \$file"   
      puts $outf "      echo \"reading data from file \$!file\" "
      puts $outf ""
  
      set line "      define veclist \"$L1 \" "
      puts $outf $line 
      puts $outf ""
      puts $outf "    read <\$veclist>"
      puts $outf ""
      set line "      define veclist \"$L2 \" "
      puts $outf $line 

      close $outf

     echo "made macro read$name and saved it to file $outfile"

}




### wrappers for studying flatfields:


# aux proc: once psSLprofMedian*fit files are produced
# make images for all 30 chips
proc makeSLprofMedImages {{nBin 8} {pMaxu 10} {pMaxOther 2}} {


      foreach filter {u g r i z} {
	if {$filter == "u"} {
            set pMax $pMaxu
	} else {
            set pMax $pMaxOther
        }
        foreach camCol {1 2 3 4 5 6} {
           echo " working on $filter $camCol"
           set list [glob psSLprofMedian*${filter}${camCol}.fit]
           set images [makeFFimage $list $nBin -$pMax $pMax 1 1.0 1.0]   
           regWriteAsFits [lindex $images 0] SLprofMedImage-${filter}${camCol}.fit
	   foreach k {0 1 2} {regDel [lindex $images $k]} 
        }
        # and now make 2x3 mosaics for each band
        set list [glob SLprofMedImage-${filter}?.fit]
        set mosaic [fitlist2mosaic $list . 2 3]
        regWriteAsFits $mosaic SLprofMedImage-${filter}-mosaic.fit
        regDel $mosaic
      }

}






# make median psSLprofMedian regions for all runs in a given rerun
# makeSLprofMed 2?
proc makeSLprofMed {rerun {outputDir .}} {

global data_root 

        # first get all runs
        echo "getting run list..."
        set list [doSDSSMOCrerun $rerun "" return]

        # and now loop over all runs
        foreach rr $list {
           echo "working on $rr ([lsearch $list $rr] out of [llength $list])"
           set run [lindex $rr 0]
           set runstr [format %06d $run]
           set rerun [lindex $rr 1]
           # and now loop over all chips
	   foreach camCol {1 2 3 4 5 6} {
	      foreach filter {u g r i z} {
                  set chip ${filter}${camCol}
		  set file $data_root/$run/$rerun/objcs/$camCol/psSLprof-${runstr}-${chip}.fit
                  if {![file exist $file]} {
                      echo "CANNOT FIND $file"
                  } else {
                    if [catch {      
                      set reg [regReadAsFits [regNew] $file]
                      set ncol [exprGet $reg.ncol]
                      set regM [regNew -type FL32 1 $ncol]
                      regMedianFindByColumn $reg $regM
                      # dump median region
                      set outfile $outputDir/psSLprofMedian-${runstr}-${chip}.fit
                      regWriteAsFits $regM $outfile
                      regDel $regM
                      regDel $reg
                    } msg] {
                      echo PROBLEMS ($msg) with file $file
                    }
                  }
	      }
	   }
        }

    return
}







# call makeFFimage for all 30 chips
# e.g.
# dumpFFimages VI VI 20 -2 2 
proc dumpFFimages {{seasons "all"} {title s} {nBin 10} {pmin -10} {pmax 10} {normalize 1} {root psFF-}} {


#### run boundaries for flatfield seasons
#  from sdss-calib 800
# -------------------------------
#      Season   run range
# -------------------------------
#        1     94 to 1140 
#        2   1231 to 1478 
#        3   1659 to 1755 
#        4   1869 to 2078 
#        5   2125 to 2143 
#        6   2188 to 2961
# -------------------------------
    set run1(I)      0; set run2(I)    1140
    set run1(II)  1231; set run2(II)   1478
    set run1(III) 1659; set run2(III)  1755
    set run1(IV)  1869; set run2(IV)   2078
    set run1(V)   2125; set run2(V)    2143
    set run1(VI)  2188; set run2(VI)  99999


    loop i 1 7 {
       foreach f {u g r i z} {
           echo working on $i $f
           set list [glob ${root}*${f}$i.fit]
           if {$seasons == "all"} {  
               set Slist $list
           } else {
               set Slist ""
               foreach FF $list {
                   set run [string trimleft [string range $FF 5 10] 0]  
		   foreach s $seasons {
		       if {$run >= $run1($s) && $run <= $run2($s)} {
                           set Slist "$Slist $FF"
                       } else {
                           # echo run $run not in season $s
                       }
                   }
               }
           }    
           set Slist [lsort $Slist]      
           set images [makeFFimage $Slist]  
           set name ${f}${i}FFseason${title}_time.fit 
           regWriteAsFits [lindex $images 2] $name
           echo "      dumped $name"
           loop k 0 3 {  
              regDel [lindex $images $k]
           }
       } 
    }

  return [FFlist2runlist $Slist]

}


# make an image with FF vectors
proc makeFFimage {filelist {nBin 10} {pmin -10} {pmax 10} {normalize 1} {Cnorm 10000.0} {Coffset 0.0}} {


    set Nfile [llength $filelist] 
    set reg [regReadAsFits [regNew] [lindex $filelist 0]] 
    set ncol [exprGet $reg.ncol]  
    set nrow [exprGet $reg.nrow]  
    regDel $reg
    set image [regNew -type FL32 [expr $Nfile + 20] $ncol]  
    regClear $image  

    # side bar
    loop i [expr $Nfile+10] [expr $Nfile+20] {
        set thisFF [subRegNew $image 1 $ncol $i 0]
        loop j 0 $ncol {
           set Pixel [subRegNew $thisFF 1 1 0 $j]
           set C [expr $Cnorm*(1.0+$pmin/100.0+1.0*$j*($pmax-$pmin)/($ncol-1)/100)-$Coffset]
           regAddWithDbl $Pixel $C
           regDel $Pixel
        }
        regDel $thisFF
    }    

    
    loop i 0 $Nfile {
	set reg [regReadAsFits [regNew] [lindex $filelist $i]]
        # check region
        assert { $ncol == [exprGet $reg.ncol] }
        assert { $nrow == [exprGet $reg.nrow] }
        fixFF $reg 1024 
        set thisFF [subRegNew $image 1 $ncol $i 0]
        regAdd $thisFF $reg 
        regDel $thisFF
    }

    if {$normalize} {
       # find the row median for this image and then renormalize
       ## median using regMedianFindByColumn
       set regM [regNew -type FL32 1 $ncol]
       # do not include the scale bar
       set subFF [subRegNew $image $Nfile $ncol 0 0]
       regMedianFindByColumn $subFF $regM
       if {0} {
          echo median: 
          dervishPlot $regM -row 0
       }
       # divide by median
       set imageN [regNew -type FL32  [expr $Nfile + 20] $ncol]  
       regClear $imageN
       # copy the scale bar
       regAdd $imageN $image
       # and now normalize
       set subN [subRegNew $imageN $Nfile $ncol 0 0]
       regDivByRow $subFF $regM -scale $Cnorm -regOut $subN
       regDel $subN      
       regDel $subFF       
    }


  set imageBinned [regBin $image -row 1 -col $nBin -normalize 1]
  regDel $image

  if {$normalize} {
      set imageNBinned [regBin $imageN -row 1 -col $nBin -normalize 1]
      regDel $imageN
      return [list $imageBinned $regM $imageNBinned]
  } else {
      return $imageBinned
  }     
   

}

 

  
# take a 1xN region, acount for the split around nSplit column and
# renormalize such that       
proc fixFF {reg {nSplit 1024} {Lbuffer 50} {Rbuffer 50} {Npoly 3}} {

    set ncol [exprGet $reg.ncol]  
    set nrow [exprGet $reg.nrow] 

    # do the work
    set vJump [get_jump $reg $nSplit $Lbuffer $Rbuffer $Npoly]
    set Jmedian [exprGet $vJump.vec<0>]
    vectorExprDel $vJump
    set FFcorrectionL [format "%6.4f" [expr 2.0*$Jmedian/(1+$Jmedian)]]
    set FFcorrectionR [format "%6.4f" [expr 2.0/(1+$Jmedian)]]
    set leftFF [subRegNew $reg 1 $nSplit 0 0]
    regIntLincom $leftFF "" 0 $FFcorrectionL 0
    regDel $leftFF
    set length [expr $ncol - $nSplit]  
    set rightFF [subRegNew $reg 1 $length 0 $nSplit] 
    regIntLincom $rightFF "" 0 $FFcorrectionR 0
    regDel $rightFF

 return 0

}


# print bordering runs for each flatfield season
proc printFFseasons {FFList N1s N1e N2s N2e N3s N3e N4s N4e N5s N5e N6s N6e} {

    echo "Season 1 ($N1s-$N1e): [lindex $FFList $N1s] to [lindex $FFList $N1e]"
    echo "       2 ($N2s-$N2e): [lindex $FFList $N2s] to [lindex $FFList $N2e]"
    echo "       3 ($N3s-$N3e): [lindex $FFList $N3s] to [lindex $FFList $N3e]"
    echo "       4 ($N4s-$N4e): [lindex $FFList $N4s] to [lindex $FFList $N4e]"
    echo "       5 ($N5s-$N5e): [lindex $FFList $N5s] to [lindex $FFList $N5e]"
    echo "       6 ($N6s-$N6e): [lindex $FFList $N6s] to [lindex $FFList $N6e]"

} 

# given a NcolxNrow region, renormalize the subregion with 0 < row < Nrow,
# by its median, and scale to Cnorm. 
# Return the list of medians and the renormalized region.
proc renormImage {reg {Nrow -1}  {Cnorm 10000.0}} {

    set showMed 0

    set nrow [exprGet $reg.nrow]  
    set ncol [exprGet $reg.ncol]  
    if {$Nrow < 0} {set Nrow $nrow}

    # output region
    set regN [regNew -type FL32 $nrow $ncol]  
    regClear $regN
    # copy the scale bar
    regAdd $regN $reg

    # subregion 
    set regM [regNew -type FL32 1 $ncol]
    set sub [subRegNew $reg $Nrow $ncol 0 0]
    regMedianFindByColumn $sub $regM
    if {$showMed} {
       echo median: 
       dervishPlot $regM -row 0
    }
    set subN [subRegNew $regN $Nrow $ncol 0 0]
    regDivByRow $subN $regM -scale $Cnorm
    regDel $subN
    regDel $regM
 

 return $regN

}


     
           
# given a NcolxNrow region, renormalize the six subregions defined by 
# N1s <= row <= N1e, etc., by their row medians, and scale to Cnorm. 
# Return the list of medians and the renormalized region.
proc renormFFimage {reg N1s N1e N2s N2e N3s N3e N4s N4e N5s N5e N6s N6e \
                        {Cnorm 10000.0}} {

    set showMed 0

    set nrow [exprGet $reg.nrow]  
    set ncol [exprGet $reg.ncol]  
    # output region
    set regN [regNew -type FL32 $nrow $ncol]  
    regClear $regN
    # copy the scale bar
    regAdd $regN $reg

    # subregion 1 
    set regM1 [regNew -type FL32 1 $ncol]
    set N [expr $N1e-$N1s+1] 
    set sub [subRegNew $reg $N $ncol $N1s 0]
    regMedianFindByColumn $sub $regM1
    if {$showMed} {
       echo median 1: 
       dervishPlot $regM1 -row 0
    }
    set subN [subRegNew $regN $N $ncol $N1s 0]
    regDivByRow $subN $regM1 -scale $Cnorm
    regDel $subN      
           
    # subregion 2
    set regM2 [regNew -type FL32 1 $ncol]
    set N [expr $N2e-$N2s+1] 
    set sub [subRegNew $reg $N $ncol $N2s 0]
    regMedianFindByColumn $sub $regM2
    if {$showMed} {
       echo median 2: 
       dervishPlot $regM2 -row 0
    }
    set subN [subRegNew $regN $N $ncol $N2s 0]
    regDivByRow $subN $regM2 -scale $Cnorm
    regDel $subN      

    # subregion 3
    set regM3 [regNew -type FL32 1 $ncol]
    set N [expr $N3e-$N3s+1] 
    set sub [subRegNew $reg $N $ncol $N3s 0]
    regMedianFindByColumn $sub $regM3
    if {$showMed} {
       echo median 3: 
       dervishPlot $regM3 -row 0
    }
    set subN [subRegNew $regN $N $ncol $N3s 0]
    regDivByRow $subN $regM3 -scale $Cnorm
    regDel $subN      
           
    # subregion 4
    set regM4 [regNew -type FL32 1 $ncol]
    set N [expr $N4e-$N4s+1] 
    set sub [subRegNew $reg $N $ncol $N4s 0]
    regMedianFindByColumn $sub $regM4
    if {$showMed} {
       echo median 4: 
       dervishPlot $regM4 -row 0
    }
    set subN [subRegNew $regN $N $ncol $N4s 0]
    regDivByRow $subN $regM4 -scale $Cnorm
    regDel $subN      
           
    # subregion 5
    set regM5 [regNew -type FL32 1 $ncol]
    set N [expr $N5e-$N5s+1] 
    set sub [subRegNew $reg $N $ncol $N5s 0]
    regMedianFindByColumn $sub $regM5
    if {$showMed} {
       echo median 5: 
       dervishPlot $regM5 -row 0
    }
    set subN [subRegNew $regN $N $ncol $N5s 0]
    regDivByRow $subN $regM5 -scale $Cnorm
    regDel $subN                 

    # subregion 6
    set regM6 [regNew -type FL32 1 $ncol]
    set N [expr $N6e-$N6s+1] 
    set sub [subRegNew $reg $N $ncol $N6s 0]
    regMedianFindByColumn $sub $regM6
    if {$showMed} {
       echo median 6: 
       dervishPlot $regM6 -row 0
    }
    set subN [subRegNew $regN $N $ncol $N6s 0]
    regDivByRow $subN $regM6 -scale $Cnorm
    regDel $subN      
           

  return [list $regM1 $regM2 $regM3 $regM4 $regM5 $regM6 $regN]

}

proc checkFF {runlist {ignoreList ""}} {

    set outlist ""
    foreach run $runlist {
       foreach f {u g r i z} {
	  foreach c {1 2 3 4 5 6} {  
             set name psFF-$run-${f}$c.fit
	     if {![file exist $name] && [lsearch $ignoreList $run] < 0} {
                echo missing $name
                error
             } else {
                lappend outlist $name
             }
          }   
       } 
    }

   return $outlist

}

proc run2FFlist {runlist filter camCol {ignoreList ""}} {

    set outlist ""
    foreach run $runlist {
       set f $filter
       set c $camCol
       set name psFF-$run-${f}$c.fit
       if {[lsearch $ignoreList $run] < 0} {             
           lappend outlist $name
       }  
    }

  return $outlist

}

proc FFlist2runlist {list {ignoreList ""}} {

     set outlist ""
     foreach FF $list {
	set run [string trimleft [string range $FF 5 10] 0]
	if {[lsearch $outlist $run] < 0 && [lsearch $ignoreList $run] < 0} {
           lappend outlist $run
        }
     }

   return $outlist
}



# wrapper for dumpFF

proc dumpFFs {psFFlist} {

    foreach FF $psFFlist {
       regsub -all {\-} $FF _ out
       regsub fit $out dat out2 
       dumpFF $FF $out2
    }

}


# dump flatfield as ascii
proc dumpFF {psFFfile outfile} {

    set Cnorm 10000.0

    set reg [regReadAsFits [regNew] $psFFfile ] 
    fixFF $reg 1024 $Cnorm
    set ncol [exprGet $reg.ncol]  
    set x [vIndex $ncol]
    set y [vectorGetFromRegion $reg 0 0 0 [expr $ncol-1] 0] 
    regDel $reg

    # dump to file
    vectorsWriteToFile [list $x $y] $outfile
    vListDel [list $x $y]
 
}


# wrapper for makeFFfile
proc makeFFfiles {{filterlist ""}} {

     if {$filterlist == ""} {set filterlist "u g r i z"}

     ## chosen combinations
     set s(1) FFcorr_001033_002662
     set s(2) FFcorr_001345_002335
     set s(3) FFcorr_001752_002589
     set s(4) FFcorr_001894_002709
     set s(5) FFcorr_002125_002247
     set s(6) FFcorr_002299_002304
     set romans "I II III IV V VI"
 
     loop season 1 7 {
        set rs [lindex $romans [expr $season - 1]]
	loop col 1 7 {
	    foreach f $filterlist {
	       if {$f == "u"} {
                  set Cname $s($season)_u_col$col.dat
               } else {
                  set Cname ""
               }
	       if {$season == 6} {set Cname ""}
	       if {$season == 5} {set Cname ""}
	       set trueFF psFF-000001-${f}$col.fit
               set newFlat psFF-99999$season-${f}$col.fit
	       set newVERSION season$rs
               set title "FF correction for season $rs, chip ${f}$col"
               echo calling makeFFfile $trueFF $Cname $newFlat $newVERSION $title
               makeFFfile $trueFF $Cname $newFlat $newVERSION $title
	       if {$Cname != ""} {
                   exec cp plot_fit.ps FFfit_$newVERSION-${f}$col.ps 
               }
            }
        }
     }

}


# another wrapper for makeFFfile: individual fix
# e.g.
# set rootTrue /u/dss/data/Flatfields/FFseasons/psFF-999991-
# makeFFfiles2 psFFcorr_col _bin40.dat $rootTrue psFF-999991- seasonI-2ndIter
proc makeFFfiles2 {rootCorr1 rootCorr2 rootTrue rootNew VER {true 0} {chipList ""}} {

global bad_chips

     if {![info exist bad_chips]} {set bad_chips ""}
     if {$chipList == ""} { 
	 foreach f "u g r i z" {
	     foreach c "1 2 3 4 5 6" {
		 lappend chipList ${f}$c
             }
         }
     }
 
     foreach chip $chipList {
	   set f [string range $chip 0 0] 
	   set col [string range $chip 1 1] 
	   set Cname ${rootCorr1}${col}_${f}$rootCorr2
	   set trueFF ${rootTrue}${f}$col.fit
	   set newFlat ${rootNew}${f}$col.fit
	   set newVERSION $VER
	   set title "FFcorr, $VER, ${f}$col"           
	   if {$true} {
	      set Cname ${rootCorr1}${f}${col}$rootCorr2
              makeFFfile $trueFF "" $newFlat $newVERSION $title	$Cname
           } else {
              echo calling makeFFfile $trueFF $Cname $newFlat $newVERSION $title
              makeFFfile $trueFF $Cname $newFlat $newVERSION $title	
           }
	   catch {exec cp plot_fit.ps FFfit_$newVERSION-${f}$col.ps} 
     }

}




# given a "true" flatfield file, and a file with corrections,
# make a corrected flatfield file
# e.g. makeFFfile psFF-000001-u3.fit FFcorr_001033_002662_u_col3.dat psFF-999993-u3.fit FFseasonI  
proc makeFFfile {trueFlat corrFile newFlat newVERSION {title ""} {trueCorrectionFile ""}} {
 
      # get true FF 
      set true [regReadAsFits [regNew] $trueFlat]
      set ncol [exprGet $true.ncol]
      assert { [exprGet $true.nrow] == 1 }
      # replace VERSION by newVERSION
      set version [hdrGetAsAscii $true.hdr VERSION]
      hdrDelByKeyword $true.hdr VERSION
      hdrInsWithAscii $true.hdr VERSION $newVERSION "from $version"
     
      # read correction and fit a polynomial
      set x [vIndex $ncol]

      if {$corrFile != ""} {
        # read correction and fit a polynomial
        set bin [lindex [file2Vector $corrFile 1 3 55] 1] 
        set mag [lindex [file2Vector $corrFile 2 3 55] 1] 

        # now we have two vectors, bin and mag, and we fit a Norder polynomial
        # to mag(bin) and return a new vector which contains the
        # best-fit value for each element in x
        set Norder 3; set Nhwindow 3
        set fit [piecewisePolynom $bin $mag $Norder $Nhwindow $x]

        # smooth
        vMeanSmooth $fit

        # plot 
        vNameSet $x "chip column"; vNameSet $fit "FF correction (mag)"
        vNameSet $bin ""; vNameSet $mag ""
        plot_fit $bin $mag "" $x $fit 0 2048 -0.2 0.2 $title /CPS   
        vListDel "$bin $mag"     
      } elseif {$trueCorrectionFile != ""}  {
        set fit [lindex [file2Vector $trueCorrectionFile 2] 1]     
      } else {
        error "shouldn't be here"
      }

      # from mags to factors      
      set corr [vectorExprEval 10^(0.4*$fit)]
      # correct
      loop i 0 $ncol {
          set C [exprGet $corr.vec<$i>]
          assert { $C > 0.5 } 
          assert { $C < 1.5 } 
          set old [exprGet $true.rows_u16<0><$i>]
          handleSet $true.rows_u16<0><$i> [expr $C*$old]
      }
     
      # dump new FF
      regWriteAsFits $true $newFlat
      regDel $true
      vListDel "$fit $x $corr"
}




# given vectors x and y, return yfit for each value of xfit by linearly
# connecting the (x,y) dots. Assume that x is monotonously increasing. 
proc linearInterp {x y xfit} {

      assert { [exprGet $x.dimen] == [exprGet $y.dimen] }
      set yfit [vectorExprNew [exprGet $xfit.dimen]]
      set xmin [exprGet  $x.vec<0>]
      set xmax [exprGet  $x.vec<[expr [exprGet $x.dimen]-1]>] 

      loop i 0 [exprGet $xfit.dimen] {
	 set x0 [exprGet  $xfit.vec<$i>]
	 if {$x0 <= $xmin} {
	     handleSet $yfit.vec<$i> [exprGet $y.vec<0>]
             continue
	 }
	 if {$x0 >= $xmax} {
	     handleSet $yfit.vec<$i> [exprGet $y.vec<[expr [exprGet $x.dimen]-1]>]
             continue
	 }
         # bracket x0
         set jstop 0
         set j 0
         while {$jstop == 0} {
	     set j [expr $j + 1]  
             assert { $j < [exprGet $x.dimen] }                                                      
             set xloc [exprGet $x.vec<$j>]                                   
             if {$xloc >= $x0} {                                        
                set jstop 1                                                         
	        set xL [exprGet $x.vec<[expr $j-1]>]
	        set xR [exprGet $x.vec<[expr $j]>]
	        set yL [exprGet $y.vec<[expr $j-1]>]
	        set yR [exprGet $y.vec<[expr $j]>]
	        set slope [expr ($yR-$yL)/($xR-$xL)]
		set y0 [expr $yL + $slope * ($x0-$xL)]                                                
                handleSet $yfit.vec<$i> $y0
             }                                                      
         }
      }

   return $yfit

}


# given vectors x and y, fit a piecewise Norder-th polynomial to y(x)
# such that +-Nhwindow points around each position are taken for
# the fit. Return the fit for positions given in xfit.
proc piecewisePolynom {x y Norder Nhwindow xfit} {

      assert { [exprGet $x.dimen] == [exprGet $y.dimen] }
      set Nwindow [expr 2*$Nhwindow+1]
      assert { $Nwindow <= [exprGet $x.dimen] }
      set xwindow [vectorExprNew $Nwindow]
      set ywindow [vectorExprNew $Nwindow]
      set yfit [vectorExprNew [exprGet $xfit.dimen]]
      

      loop i 0 [exprGet $xfit.dimen] {
         #echo i=$i
         # fit for this position
	 set xL [exprGet $xfit.vec<$i>]
         # find the index of the closest match in x-y
         set iC 0; set xmindist -1
         loop j 0 [exprGet $x.dimen] {
	     set dist [expr abs([exprGet $x.vec<$j>]-$xL)]
             if {$dist < $xmindist || $xmindist < 0} {
                 set xmindist $dist; set iC $j
             } 
         }
         # OK, the closest point is x(iC), populate the window
         set jLeft  [expr $iC-$Nhwindow]
         set jRight [expr $iC+$Nhwindow+1]
         if {$jLeft < 0} {set jLeft 0; set jRight $Nwindow}
         if {$jRight > [exprGet $x.dimen]} {
             set jRight [exprGet $x.dimen]
             set jLeft  [expr $jRight - $Nwindow]
         }
         assert { [expr $jRight-$jLeft] == $Nwindow }
         loop j $jLeft $jRight {
             set j0 [expr $j-$jLeft]
	     handleSet $xwindow.vec<$j0> [exprGet $x.vec<$j>]
	     handleSet $ywindow.vec<$j0> [exprGet $y.vec<$j>]
             #echo W($j0,$j): [exprGet $xwindow.vec<$j0>] [exprGet $ywindow.vec<$j0>]
         }
         set Clist [get_lsq_polynom $xwindow $ywindow $Norder]  
         set yF [lindex $Clist 0]
         loop k 1 [llength $Clist] {
	     set yF [expr $yF + [lindex $Clist $k]*pow($xL,$k)]
         }
         handleSet $yfit.vec<$i> $yF
         #echo "   x=$xL, yFit = $yF" 
      }

   return $yfit

}

# plot yFit vs. xFit as line, and y vs. x with symbols
proc plot_fit {x y error xFit yFit xmin xmax ymin ymax {title ""} {device /XWINDOW} {xorig ""} {yorig ""}} {

        if [catch {
          # Set up the plot 
          set pg [pgstateNew]
          set dev [devicename $device plot_fit.ps]
          pgstateSet $pg -device $dev -lineWidth 3 -isLine 1 \
              -xfract 0.3 -yfract 0.4 -nxwindow 1 -nywindow 1 -icLine 4
          pgstateOpen $pg
         
          vPlot $pg $xFit $yFit -ymin $ymin -ymax $ymax \
                     -xmin $xmin -xmax $xmax 

          pgstateSet $pg -isNewplot 0 -isLine 0 -icMark 2 -symb 4
          if {$error != ""} {
               vPlot $pg $x $y -vectorYErr $error -ymin $ymin -ymax $ymax \
                     -xmin $xmin -xmax $xmax 
          } else {
               vPlot $pg $x $y -ymin $ymin -ymax $ymax \
                     -xmin $xmin -xmax $xmax 
          }

          plot_line $pg -1000 0 3000 0 1 13
          plot_line $pg -1000 -0.02 3000 -0.02 1 3
          plot_line $pg -1000 0.02 3000 0.02 1 3
          plot_line $pg 0 -1 0 1 1 13
          plot_line $pg 2047 -1 2047 1 1 13
         
          titlePlot "$title" 40
    
          if {$xorig != "" && $yorig != ""} {
              pgstateSet $pg -isNewplot 0 -isLine 0 -icMark 1 -symb 1
              vPlot $pg $xorig $yorig -ymin $ymin -ymax $ymax \
                     -xmin $xmin -xmax $xmax 
          }
               
          endplot $pg
        } msg] {
          echo "problems in plot_fit:"
          echo "$msg"
          return 1
        }

   return 0
}


### 
# given a list of psField files, make a table with 
# run camCol field RA Dec psf_width_u ... psf_width_z
# assuming that asTrans files exist in asTransDir
proc psFields2seeing {psFieldList outfile {N1 -1} {N2 -1} } {

     # for memory check
     set startMem [memNextSerialNumber]

     set outf [open $outfile w]
     set user [exec whoami]
     set date [exec date]
     puts $outf "#" 
     puts $outf "# dumped using psFields2seeing by $user on $date"
     puts $outf "#" 
     puts $outf "#run col fld     RA         Dec    = psf_width (arcsec) in ugriz ="

     set Ntotal [llength $psFieldList]
     if {$N1 < 0} {set N1 0}
     if {$N2 < 0} {set N2 $Ntotal}
     loop i $N1 $N2 {
         set file [lindex $psFieldList $i] 
         echo working on file $i (out of $Ntotal): $file
         set run [string trimleft [string range $file 8 13] 0]
         set camCol [string trimleft [string range $file 15 15] 0]
         set field [string trimleft [string range $file 17 20] 0]
         set asTransFile asTrans-[format %06d $run].fit
         set hdr [hdrReadAsFits [hdrNew] $asTransFile]
         set field0 [hdrGetAsInt $hdr FIELD0]
         set nfields [hdrGetAsInt $hdr NFIELDS]
         hdrDel $hdr
         if {$field < $field0 || $field > [expr $field0+$nfields-1]} {
            echo "For field=$field in file $asTransFile: field0=$field0, nfields=$nfields"
            set RADec "999.999999 999.999999" 
         } else {
            if {[catch { 
                set RADec [pix2eq $run $camCol $field 680 1024 r "" "" 0 $asTransFile]
	        } msg]} {
                echo "Problems with $asTransFile: $msg"
                set RADec "999.999999 999.999999" 
            }
         }
         if {![file exist $file]} {
            echo "Doesn't exist: $file"
            continue
         }
         if {[catch { 
               set seeing [get_seeing $file]
               set line "[format %5d $run] $camCol [format %4d $field] $RADec $seeing" 
               puts $outf $line
	      } msg]} {
            echo "Problems with $file: $msg"
         }
     }

     close $outf

     # check for memory corruption, if available in dervish
     if {[info commands memCheck] != ""} {
        assert {[memCheck] == ""}
     }
     # check for memory leaks
     if {1} {
        assert {[memBlocksGetRange $startMem [memSerialNumber]] == {}}
     } else {
       echo "    ****** BYPASSING MEMORY LEAK CHECK ******"
     }
 
}
 

# given a psField file, return the string with psf_width
proc get_seeing {psField} {
   
global allow_old_psField 
global allow_opdb_psField

     # to get around problems with psField files dumped from opdb
     set allow_opdb_psField 1  
     set allow_old_psField 1   
 
     set psFieldroot [string range $psField 0 [expr [string length $psField]-9]]
     set field [string trimleft [string range $psField 17 20] 0]
     set cframes [get_cal_chain_from_psField $psFieldroot $field $field]
     set el [chainElementGetByPos $cframes 0]
     # extract seeing in all bands
     set seeing ""
     loop i 0 5 {
        set s  [format "%5.3f" [exprGet $el.calib<$i>->psf->width]]
        set seeing "$seeing $s"
     }     
     handleDel $el

     chainDestroy $cframes genericDel
  
   return $seeing

}


# e.g.
# set root1 flats/preliminary  
# set root2 1894/15/objcs 
# set b1 /data/dp3.q/data 
# set b2 /data/dp3.b/data 
# compareAllFF u $b1 $root1 999994 0 $b2 $root2 1894 1 /XWINDOW 
proc compareAllFF {filter baseDir1 root1 run1 type1 baseDir2 root2 run2 type2 {device /XWINDOW} {camCols "all"}} {
 
    if {$camCols == "all"} {set camCols "1 2 3 4 5 6"} 

    foreach c $camCols {
        set runstr1 [format %06d $run1]
        if {$type1 == 0} {
           set f1 ${root1}/psFF-$runstr1-${filter}$c.fit
        } else {
           set f1 ${root1}/$c/psFF-$runstr1-${filter}$c.fit
        }
        set runstr2 [format %06d $run2]
        if {$type2 == 0} {
           set f2 ${root2}/psFF-$runstr2-${filter}$c.fit
        } else {
           set f2 ${root2}/$c/psFF-$runstr2-${filter}$c.fit
        }
        set title "$f1 VS $f2"
        set rms [compareFF $baseDir1/$f1 $baseDir2/$f2 $device $title 1]
        if {$device != "/XWINDOW"} {
           set name compareFF-$run1-$run2-col$c-$filter.ps
           if {![catch {exec mv compareRegRows.ps $name} msg]} {
               echo made $name (rms=$rms)
           } else {
               echo caught error: $msg
           }         
        } else {
           echo compareFF for col. $c, filter $filter
        }
    }
    
}


# wrapper for makePTcorrection 
# e.g.
# set stats [makePTcorrectionAll]
proc makePTcorrectionAll {{device /CPS} {dir /u/dss/data/Science/Variability/flatcorrdata}} {

    set Npoly 3
    set binsize 128
    set stats {}
    echo " filter  column  mean median sigma peak-to-peak"
    foreach f {u g r i z} {
	foreach c {1 2 3 4 5 6} {
	    set PTfile $dir/m25mptcolc-${f}${c}-6.dat  
            set outfile PTcorr-${f}${c}-6.dat
            set order $Npoly 
            set bWidth $binsize
            ## special cases 
            if {$f == "u" && $c == "3"} {
               set order 2               
            } elseif {$f == "z" && $c == "6"} {
               set order 2  
               set bWidth 256             
	    } elseif {$f == "i" && $c == "1"} {
               set order 2  
               set bWidth 256            
	    } elseif {$f == "i" && $c == "3"} {
               set order 1
	    } elseif {$f == "i" && $c == "4"} {
               set order 2
               set bWidth 256            
	    } elseif {$f == "i" && $c == "5"} {
               set order 2
	    } elseif {$f == "r" && $c == "2"} {
               set order 1
	    } elseif {$f == "r" && $c == "3"} {
               set order 2
               set bWidth 256       
            } elseif {$f == "r" && $c == "4"} {
               set order 2
               set bWidth 256            
	    } elseif {$f == "r" && $c == "5"} {
               set order 2
               set bWidth 256            
	    } elseif {$f == "g" && $c == "1"} {
               set order 1           
	    } elseif {$f == "g" && $c == "4"} {
               set order 2
               set bWidth 256                      
	    } elseif {$f == "g" && $c == "5"} {
               set order 1           
               set bWidth 256            
	    } elseif {$f == "g" && $c == "6"} {
               set order 1           
               set bWidth 256            
	    } elseif {$f == "u" && $c == "1"} {
               set order 2           
	    } elseif {$f == "u" && $c == "2"} {
               set order 2           
               set bWidth 256            
	    } elseif {$f == "u" && $c == "3"} {
               set order 2            
	    } elseif {$f == "u" && $c == "5"} {
               set order 1          
               set bWidth 64            
	    } elseif {$f == "u" && $c == "6"} {
               set order 2
               set bWidth 256                    
	    }

            set s [makePTcorrection $PTfile $outfile $order $bWidth $device "$outfile"]
            lappend stats [list $f $c $s]
        }
    }   

  return $stats
}


# yet another wrapper for makePTcorrection 
# e.g.
# set stats [makePTcorrectionAlliter5 2]
proc makePTcorrectionAlliter5 {season {device /CPS} {dir /u/dss/data/Science/Variability/flatcorrdata/Iter4}} {

global medianSmooth 

    set medianSmooth 1
    set stats {}
    echo " filter  column  mean median sigma peak-to-peak"
    foreach f {u g r i z} {
        if {$f == "u"} {
            set binsize 64
        } else {
            set binsize 32
        }
	foreach c {1 2 3 4 5 6} {
	    set PTfile $dir/m25mptcolc-${f}${c}-$season.dat  
            set outfile PTcorr-Iter4to5-${f}${c}-$season.dat
            set bWidth $binsize
            set s [makePTcorrection $PTfile $outfile -1 $bWidth $device "$outfile"]
            lappend stats [list $f $c $s]
        }
    }   

  return $stats
}



# given a "true" flatfield file, and a file with corrections,
# make a corrected flatfield file
# e.g.
# set PTfile /u/dss/data/Science/Variability/flatcorrdata/m25mptcolc-r1-6.dat  
# makePTcorrection $PTfile PTcorr-r1-6.dat 3 128 /XWINDOW "PTcorr-r1-6.dat"
proc makePTcorrection {inputFile corrFile {Npoly 3} {binsize 64} {device /CPS} {title ""} {Nhwindow ""}} {
 
global medianSmooth 

      set ncol 2048
      # read correction and fit a polynomial
      set x [vIndex $ncol]

      # read data
      set colc [lindex [file2Vector $inputFile 1] 1] 
      set delm [lindex [file2Vector $inputFile 2] 1] 

      # bin data and find median
      set half_win [expr $binsize / 2]
      set binned [bin_vector $x $half_win "colc"]
      set bins [lindex $binned 0]
      set half_win [lindex $binned 1]
      set w [lindex $binned 2]
      set mask [vectorExprEval "$delm >= -0.1 && $delm <= 0.1 ? 1 : 0"]
      set Nmin 5; set qdefault 0
      set bin_stats [binned_stats $colc $delm $mask $bins $half_win $Nmin $qdefault]
      set median [lindex $bin_stats 1]
      set sigma [lindex $bin_stats 2]
      set Npts [lindex $bin_stats 0]
      set error [vectorExprEval $sigma/sqrt($Npts)] 

      # smooth?
      if {[info exists medianSmooth] && $medianSmooth == 1} {
          vTriangleSmooth $median
      }

      # now we have two vectors, bins and median, and we fit a Norder polynomial
      # to median(bins) and return a new vector which contains the
      # best-fit value for each element in x
      set Norder $Npoly
      # +-Nhwindow points around each position are taken for the fit.
      if {$Nhwindow == ""} {
         set Nhwindow 3
      }
      if {$Npoly > 0} {
          set fit [piecewisePolynom $bins $median $Norder $Nhwindow $x]
      } else {
          set fit [linearInterp $bins $median $x]      
      }

      # smooth
      vMeanSmooth $fit
    
      # plot 
      vNameSet $x "chip column"; vNameSet $fit "FF correction (mag)"
      vNameSet $colc ""; vNameSet $delm ""

      # echo $bins $median $sigma $Npts $error $colc $delm   
      plot_fit $bins $median $error $x $fit 100 1948 -0.05 0.05 $title $device $colc $delm   
      if {$device != "/XWINDOW"} {exec cp plot_fit.ps $corrFile.ps}

      # write to file
      vectorsWriteToFile [list $x $fit] $corrFile 

      # statistics
      set mean   [format "%5.3f" [vMean $fit]]
      set med [format "%5.3f" [vMedian $fit]]
      set sigma  [format "%5.3f" [vSigma $fit]]
      set max [vExtreme $fit max]
      set min [vExtreme $fit min]
      set P2P  [format "%5.3f" [expr $max - $min]]
      set mean2   [format "%5.3f" [vMean $median]]
      set med2 [format "%5.3f" [vMedian $median]]
      set sigma2  [format "%5.3f" [vSigma $median]]
      set max [vExtreme $median max]
      set min [vExtreme $median min]
      set P2P2  [format "%5.3f" [expr $max - $min]]

      # clean
      vListDel "$x $fit $bins $colc $delm"
      vListDel $bin_stats

     return "$mean   $med    $sigma       $P2P      $mean2   $med2    $sigma2       $P2P2"     
     
}


# bin vector with step size 2*half_win and return list
# {binnedVector half_win windowWidth}
proc bin_vector {vector half_win {name ""}} {

     set fmin [expr int([vExtreme $vector min])]
     set fmax [expr int([vExtreme $vector max])]   
     if {$half_win > 0} {
         set Nbin [expr 1+($fmax - $fmin)/(2*$half_win)]
         set w [expr 2*$half_win]
     } else {
         set Nbin [expr $fmax - $fmin + 1]
         set half_win 0
         set w 1 
     } 
     if {$Nbin < 1} {
         set Nbin 1
         set w [expr $fmax - $fmin]
         set half_win [expr $w/2+1]
     }
 
     set fbinned [vectorExprNew $Nbin]
     vNameSet $fbinned $name
     handleSet $fbinned.vec<0> [expr $fmin + $half_win]
     loop i 1 $Nbin {
         set j [expr $i-1]
         handleSet $fbinned.vec<$i> [expr [exprGet $fbinned.vec<$j>] + $w]
     }

    return [list $fbinned $half_win $w $Nbin]

}


#######################################################################
# ** procedure for correcting flatfield files relative to season VI **
# 1) dump matched data from opdb, e.g.
#    ts> getVariables 1894 16  all -1 -1 2709 16 All1894-16vs2709-16 selectVarCandAll 1
#
# 2) produce data files with corrections, e.g.
#    sm> getFFcorrSeasonsFinal 2 g 4
#    for ugriz:
#    define season 1
#    foreach f <u g r i z> {
#       getFFcorrSeasonsFinal 2 $f $season
#    }
#
# 3) produce new psFF*fit files, e.g.
#    photo> set rootTrue /u/dss/data/Flatfields/FFseasons/psFF-999993-
#    photo> makeFFfiles2 psFFcorrSeason3_col _bin40.dat $rootTrue psFF-999993- seasonIII-2ndIter
#
# 4) sanity tests, e.g.
#    photo> set b1 /u/dss/data/Flatfields; set root1 FFseasons
#    photo> set b2 /u/dss/data/Science/Variability; set root2 Flatfields
#    photo> set filter u
#    photo> compareAllFF $filter $b1 $root1 999993 0 $b2 $root2 999993 0 /XWINDOW
# 
# 5) the "truth" from season VI can be changed at will: simply change rootTrue in 3)
#
# ** procedure for correcting season VI flatfield files relative to external (e.g. PT) photometry **
# 1) given a list of m2.5m - mExt vs. colc, e.g. m25mptcolc-z6-6.dat files, produce QA plots by
#    sm>  doPTvs2.5m $device $season $binsize
#    
# 2) photo> makePTcorrectionAll
#    which produces plots PTcorr-z1-6.dat.ps and data files PTcorr-z1-6.dat
# 3) not all season VI flats are bad, first copy over all of them
#    cp /u/dss/data/Flatfields/FFseasons/psFF-999996-*fit temp2/
# 3) inspect plots and declare problematic chips  
#    set bad_chips "u1 u3 u6 g4 g5 r1 r2 r3 r4 r5 i3 i5 z6"      
# 4) set rootTrue /u/dss/data/Flatfields/FFseasons/psFF-999996-   
#    makeFFfiles2 PTcorr- -6.dat $rootTrue temp2/psFF-999996- seasonVI-4thIter 1 $bad_chips
#
# 5) correct other seasons in the same way 
#    first copy all
#    cp /u/dss/data/Science/Variability/Flatfields/psFF-99999[1,2,3,4,5]-*fit /u/ftp/ivezic/Flatfields/psFFfilesFinal2
#    and then correct (overwrite) bad ones
#    set bad_chips "u1 u3 u6 g4 g5 r1 r2 r3 r4 r5 i3 i5 z6" 
#    set sRoman(1) I; set sRoman(2) II; set sRoman(3) III; set sRoman(4) IV; set sRoman(5) V; 
#    foreach season "1 2 3 4 5" {     
#        set rootTrue   /u/dss/data/Science/Variability/Flatfields/psFF-99999${season}-
#        set rootFixed  /u/ftp/ivezic/Flatfields/psFFfilesFinal2/psFF-99999${season}-
#        makeFFfiles2 PTcorr- -6.dat $rootTrue $rootFixed season$sRoman($season)-4thIter 1 $bad_chips  
#    }
# 6) and sanity checks
#    every season, relative to season VI
#    set b1 /u/ftp/ivezic/Flatfields; set root1 psFFfilesFinal1
#    set b2 /u/ftp/ivezic/Flatfields; set root2 psFFfilesFinal1
#    foreach season "1 2 3 4 5" {
#       foreach filter "u g r i z" {
#          compareAllFF $filter $b1 $root1 99999$season 0 $b2 $root2 999996 0 /CPS  
#       }
#    }
#
#
#
#
# #### Final Converged Procedure: use PT-2.5m residuals to correct each season independtly ####
# 
# 1) There are files with  PT-2.5m residuals produced by Yanny
#    QA plots: doPTvs2.5mAll
#    note: change root directory in macro read_allPT 
# 2) Inspect plots plotPT25m_seasonN_$filter.ps (can bypass, see 5)
# 3) set stats [makePTcorrectionAlliter5 $seasonN]
#    which produces correction files PTcorr-Iter4to5-$filter$camCol-$seasonN.dat (and *.ps)
# 4) set sRoman(1) I; set sRoman(2) II; set sRoman(3) III; set sRoman(4) IV; set sRoman(5) V; 
#    foreach season "1 2 3 4 5 6" {     
#        set rootOld   /u/ftp/ivezic/Flatfields/psFFfilesFinal2/psFF-99999${season}-
#        set rootNew   /u/ftp/ivezic/Flatfields/psFFfilesFinal5/psFF-99999${season}-
#        makeFFfiles2 PTcorr-Iter4to5- -$season.dat $rootOld $rootNew season$sRoman($season)-5thIter 1   
#    }
# 5) sanity check
#      every season, new vs. old
#    set b1 /u/ftp/ivezic/Flatfields; set root1 psFFfilesFinal2
#    set b2 /u/ftp/ivezic/Flatfields; set root2 psFFfilesFinal5
#    foreach season "1 2 3 4 5 6" {
#       foreach filter "u g r i z" {
#          compareAllFF $filter $b1 $root1 99999$season 0 $b2 $root2 99999$season 0 /CPS  
#       }
#    }
#


### wrapper for doing everything in one go
#   directory with m25mptcolc-*dat files: PTdir 
#   directory with FFs from previous iteration: dirOld
#   directory for FFs from this iteration: dirNew
# e.g.
# set PTdir  /u/dss/data/Science/Variability/flatcorrdata/Iter4
# set dirOld /u/ftp/ivezic/Flatfields/psFFfilesFinal2
# set dirNew /u/ftp/ivezic/Flatfields/psFFfilesFinal5
# getNewFFsFromPTdata $PTdir $dirOld $dirNew 5
proc getNewFFsFromPTdata {PTdir dirOld dirNew iterNo {seasons "1 2 3 4 5 6"}} {

global medianSmooth 

    set medianSmooth 1
    set stats ""
    
    ## get PTcorr-Iter4to5-* files
    foreach season $seasons {
        set stats "$stats [makePTcorrectionAlliter5 $season /CPS $PTdir]"
    }
   
    ## produce new FFs
    set sRoman(1) I; set sRoman(2) II; set sRoman(3) III; set sRoman(4) IV; set sRoman(5) V; set sRoman(6) VI;     
    foreach season $seasons {     
       set rootOld  $dirOld/psFF-99999${season}-
       set rootNew  $dirNew/psFF-99999${season}-
       makeFFfiles2 PTcorr-Iter4to5- -$season.dat $rootOld $rootNew season$sRoman($season)-${iterNo}thIter 1   
    }

    ## QA plots
    # every season, new vs. old
    foreach season $seasons {
       foreach filter "u g r i z" {
          compareAllFF $filter $dirOld "" 99999$season 0 $dirNew "" 99999$season 0 /CPS  
	  foreach c "1 2 3 4 5 6" {
             set name compareFF-99999$season-99999$season-col$c-$filter.ps 
             exec mv $name $dirNew/$name
          }
       }
    }
    # this new season vs. new season VI
    foreach season $seasons {
       foreach filter "u g r i z" {
          compareAllFF $filter $dirNew "" 99999$season 0 $dirNew "" 999996 0 /CPS  
	  foreach c "1 2 3 4 5 6" {
             set name compareFF-99999$season-999996-col$c-$filter.ps 
             exec mv $name $dirNew/$name
          }
       }
    }

  return $stats

}



# translate  jeg's file to "PTcorr" files
# e.g.
# translateJEGcorrFF u_02_20.txt PTcorr-jeg1- u 2 
proc translateJEGcorrFF {inFile PTroot filter season} {

    # loop over columns
    set pix [lindex [file2Vector $inFile 1 3 2050 0] 1]
    foreach c {1 2 3 4 5 6} {
	set corr [lindex [file2Vector $inFile [expr 1+$c] 3 2050 0] 1]
        set filename ${PTroot}${filter}${c}-$season.dat
        # write to file
        vectorsWriteToFile [list $pix $corr] $filename   
        echo "produced $filename"      
    }

}


### wrapper for getNewFFsFromJEGdata
# set season 5; set iterNo 5
# set jegDir /u/jeg/surtel/FFseason/s05i4to5
# set jegTail 20.txt
# set oldFFdir   /u/ftp/ivezic/Flatfields/psFFfilesFinal4
# set newFFdir   /u/ftp/ivezic/Flatfields/psFFfilesFinal5
# e.g.
# makeFFjeg $season $iterNo $jegDir $jegTail $oldFFdir $newFFdir
proc makeFFjeg {season iterNo jegDir jegTail oldFFdir outDir} {

    # first translate jim's files:
    set jegRoot PTcorr-jeg1-
    foreach f {u g r i z} {
	set Sstr [format "%02d" $season]
        set jegFile ${f}_${Sstr}_$jegTail
	translateJEGcorrFF $jegDir/$jegFile PTcorr-jeg1- $f $season
    }
    echo "translated jeg's files, going to make new flatfields"
    
    getNewFFsFromJEGdata $oldFFdir $jegDir $jegRoot $outDir $iterNo $season 
    echo "made new flatfields, making QA plots"

    ## QA plots
    foreach filter "u g r i z" {
       foreach c "1 2 3 4 5 6" {
           set name compareFF-99999$season-99999$season-col$c-$filter.ps 
           exec mv $name $outDir/OldvsNew-$name
           echo made $outDir/OldvsNew-$name
       }
    }

  echo "done with makeFFjeg"
  return ""    

}

# proc to produce new flats, given old flats and jeg-given corrections
# e.g.
# first translate files:
# set jegRoot PTcorr-jeg1-
# translateJEGcorrFF U_02_20.TXT PTcorr-jeg1- u 2   # etc.
# and then 
# set oldFFdir /u/ftp/ivezic/Flatfields/psFFfilesFinal2   # this is 4th iter
# set jegDir   /u/dss/data/Science/Variability/Flatfields/jeg
# set outDir   /u/dss/data/Science/Variability/Flatfields/jeg
# set iterNo 6; set season 2; set jegRoot PTcorr-jeg1-
# getNewFFsFromJEGdata $oldFFdir $jegDir $jegRoot $outDir $iterNo $season 
# 
proc getNewFFsFromJEGdata {oldFFdir jegDir jegRoot outDir iterNo {seasons "1 2 3 4 5 6 7 8 9"} {plots 1}} {
    
    ## produce new FFs
    set sRoman(1) I; set sRoman(2) II; set sRoman(3) III; set sRoman(4) IV; set sRoman(5) V; 
    set sRoman(6) VI; set sRoman(7) VII; set sRoman(8) VIII; set sRoman(9) IX;  
    foreach season $seasons {    
       set rootOld  $oldFFdir/psFF-99999${season}-
       set rootNew  $outDir/psFF-99999${season}-
       makeFFfiles2 $jegRoot -$season.dat $rootOld $rootNew season$sRoman($season)-${iterNo}thIter 1   
    }

    ## QA plots
    if {$plots} { 
       foreach season $seasons {
          foreach filter "u g r i z" {
             # old vs. new iteration
             compareAllFF $filter $oldFFdir "" 99999$season 0 $outDir "" 99999$season 0 /CPS
	  }  
       }
    }

  return ""

}


##### calibration #####

# given a file with counts and calibrated magnitudes, dump a file with zero points
# for all 30 chips
# e.g.
# calibrate SDSSVAR_orion_calib273.dat orion_calib273_zp.dat 1
proc calibrate {infile outfile {type 1}} {

      set filters "u g r i z"
      set camCols "1 2 3 4 5 6"
      set clip 0.1
  
      if {$type == "1"} {
	  set col(camCol) 1
          loop i 0 5 {
	      set f [lindex $filters $i]
              set col(C$f) [expr 4+$i]
              set col($f)  [expr 25+2*$i]
          }
      } elseif {$type == "2"} {
	  set col(camCol) 1
          loop i 0 5 {
	      set f [lindex $filters $i]
              set col(C$f) [expr 4+$i]
              set col($f)  [expr 12+$i]
          }
      } else {
         error "don't know type $type"
      } 


      # read data 
      set camColvec [lindex [file2Vector $infile $col(camCol)] 1]
      foreach f $filters {
	  echo "reading data for filter $f"
          set counts [lindex [file2Vector $infile $col(C$f)] 1]
          set aux [vLog $counts]
          set logC [vectorExprEval 2.5*$aux]
          set mag [lindex [file2Vector $infile $col($f)] 1]
          set zp($f) [vectorExprEval $mag+$logC] 
          vListDel [list $counts $logC $mag $aux]
      }

      # for each chip get zp and dump to outfile  
      set outf [open $outfile "w"] 
      puts $outf "# photometric zero points from file $infile"
      puts $outf "# clipped at +- $clip mag"
      puts $outf "# chip   median    error    mean     sigma      N"
      foreach camCol $camCols {
         set mask [vectorExprEval "$camColvec == $camCol ? 1 : 0"]   
         foreach f $filters {
	    set chipname ${f}$camCol
	    echo "computing zp for chip $chipname"    
	    set median [vMedian $zp($f) -vMask $mask]
            # clip around (unclipped median)
            set diff [vectorExprEval abs($zp($f)-$median)]
            set fmask [vectorExprEval "$diff < $clip ? $mask : 0"]
	    set N [format "%7.0f" [vNnonzero $fmask]]
            # and now get the final statistics
	    set median [format "%8.4f" [vMedian $zp($f) -vMask $fmask]]
	    set mean   [format "%8.4f" [vMean   $zp($f) -vMask $fmask]]			      
            set sigma  [format "%8.4f" [vSigma  $zp($f) -vMask $fmask]]
	    set error  [format "%8.4f" [expr $sigma/sqrt($N)]]
            set line "   $chipname  $median $error $mean $sigma $N"
            puts $outf $line
	    vListDel [list $fmask $diff]   
         }
         vectorExprDel $mask           
      }

      vListDel [list $camColvec $zp(u) $zp(g) $zp(r) $zp(i) $zp(z)]   
      close $outf 

     echo "your output is in $outfile"

}



# wrapper for calibrate
# e.g.
# set raw_file orion_calib273.dat 
# set cal_file calib1755.dat   
# set out_file orion_calib273_zp.dat   
# do_calibrate $raw_file $cal_file $out_file
proc do_calibrate {rawfile calfile outfile} {

global express

    set express -1
    cross_correlate_list $rawfile $calfile calib_$rawfile trash.dat 2 3 0 2 3 0 1 1  
    calibrate calib_$rawfile $outfile 2

}








# for Shlegel's code to work execute:
# setenv DUST_DIR /u/schlegel/dustpub
# setenv PATH ${PATH}:${DUST_DIR}/bin 
# for details see: http://astro.princeton.edu/~schlegel/dust/local/local.html
proc getAr {l b} {

     if [catch {
          set aEBV [exec dust_getval $gLong $gLat interp=y]
	       }] {
	            echo for Shlegel's code to work execute:
                    echo setenv DUST_DIR /u/schlegel/dustpub
                    echo setenv PATH ${PATH}:${DUST_DIR}/bin 
		  }

     set Ar [format "%5.3f" [expr 2.751 * [lindex $aEBV 2]]]

   return $Ar

}



##############################
#### FOR UPDATING SDSSMOC ####


### for individual runs (to be run regularly at FNAL after each dark run)
proc getAsteroids {run rerun {outputDir ./} {dataRoot /data/dp30.a/data}} {

     set data_root $dataRoot

     tsObj2moving $outputDir/Asteroids $run $rerun
     PHOTOMETRY2ast $outputDir/Asteroids-run$run.par $outputDir/Asteroids-run$run.dat
     appendMJD $outputDir/Asteroids-run$run.dat $rerun
     getMJD $run /data/dp30.a/data $outputDir/RunData-$run.dat  

   return 0
}


### wrapper for dumping SDSSMOC data for all runs in a rerun
## e.g.
## doSDSSMOCrerun X "{752 20} {756 20}"
## or
## doSDSSMOCrerun 2? "" "1350"
proc doSDSSMOCrerun {rerun {runList ""} {ignoreRunList ""} {outputDir "./"} {dataRoot ""}} {

global data_root firstChar

         if {![info exist firstChar]} {set firstChar "?"}

         if {$dataRoot == ""} {
             set data_root [get_data_root ""]
         } else {
             set data_root $dataRoot
         }

         if {![file exist $data_root]} {
             error "CANNOT FIND data_root=$data_root"
         }

         if {$runList == ""} {
             echo " *** getting run list for rerun $rerun... ***"
             # get a list of all runs from the given rerun that have tsObj files
             set list [glob $data_root/${firstChar}*/${rerun}/calibChunks/1]
             set rrList ""
             foreach d $list {
                set d [file dirname $d]
                set d [file dirname $d]
                set rr [file tail $d]
                set d [file dirname $d]
                set run [file tail $d]
		set rrList "$rrList {$run $rr}"
             }
             # now select the highest reruns
             set SrrList ""; set addedRuns "";
             foreach rr $rrList {
		 set run [lindex $rr 0]
		 set rerun [lindex $rr 1]
                 if {[llength $SrrList] > 0} { 
		    if {[lsearch $addedRuns $run] < 0} {
                       # new run, add it
                       set SrrList "$SrrList [list $rr]"   
                       set addedRuns "$addedRuns $run"
		    } else {
                       # old run, depending on rerun take old or new values
                       set old $SrrList
                       set SrrList ""
                       foreach Srr $old {
                          set Srun [lindex $Srr 0]
		          set Srerun [lindex $Srr 1]
		          if {$run == $Srun && $rerun > $Srerun} {
                             # same run with higher rerun number, add new pair 
                             set SrrList "$SrrList [list $rr]"
			  } else {
                             # a different run, simply copy
                             set SrrList "$SrrList [list $Srr]"
                          }
                       }
                    }
                 } else {
                    # first element...
                    set SrrList [list $rr]
                 }
             }
             set runList $SrrList 
         }

         if {$ignoreRunList == "return"} {
            return $runList
         }

         ## all the requested run/rerun pairs are in runList
         ## now loop over each one and dump asteroids and comets
         echo " *** processing [llength $runList] long run list... ***"
         foreach rr $runList {
            set run [lindex $rr 0]
            set rerun [lindex $rr 1] 
            echo "    ---  processing run $run (rerun $rerun) ---"   
            loop col 1 7 {
               # unresolved
               tsObj2moving Asteroids $run $rerun $col -1 -1 STAR
	       catch {exec mv dump_tsObj.log Asteroids-$run-$rerun-col$col.log}
	       if {0} {
                  # resolved
                  tsObj2moving Comets $run $rerun $col -1 -1 GALAXY
		  catch {exec mv dump_tsObj.log Comets-$run-$rerun-$col.log}
               }
            }
            echo; echo " *** done with run $run (rerun $rerun) ***"
            echo; echo; echo;
         }    

    return

}



## append mjd to the end of each line in a file
## where run, camCol and field are given in (1-indexed) columns
## Crun CcamCol Cfield
## rerun is constrained the same way as for doSDSSMOCrerun
## e.g.
## appendMJD SDSSMOC2.dat 2?
proc appendMJD {file rerun {Crun 1} {CcamCol 2} {Cfield 3} {outputDir "./"} {dataRoot ""}} {

global data_root firstChar

         if {![info exist firstChar]} {set firstChar "?"}

         if {$dataRoot == ""} {
             set data_root [get_data_root ""]
         } else {
             set data_root $dataRoot
         }

         if {![file exist $data_root]} {
             error "CANNOT FIND data_root=$data_root"
         }
        
      ## open files 
      set infile [open $file r]
      set outf [open ${file}.MJD w]
      set outfP [open ${file}.problems w]
 
      set Ngood 0 
      set Nbad 0 
      set Ncomm 0
      # process line by line       
      set Nline 0 
      while {![eof $infile]} {
         set line [gets $infile]
         incr Nline
	 if {[expr $Nline/100*100] == $Nline} {echo line $Nline ...}
         # process line
         if {![eof $infile] && [string range $line 0 0] != "#"} {
             set mjd -1  
             set run    [lindex $line [expr $Crun-1]]
             set camCol [lindex $line [expr $CcamCol-1]]
             set field  [lindex $line [expr $Cfield-1]]
             # given run and rerun, get the latest asTrans file
             set asTransList [glob $data_root/${run}/${rerun}/astrom/asTrans*fit]
             if {[llength $asTransList] > 0} {
                 set asTransFile [lindex $asTransList [expr [llength $asTransList]-1]]
	         # given asTrans file and field, return mjd
                 if {[catch {
                     set mjd [asTrans2MJD $asTransFile $run $camCol $field]
		 }]} {
                     set mjd -1  
                 }
             }
             if {$mjd < 0} {
                incr Nbad
                puts $outfP "$line"
             } else {
                incr Ngood
                puts $outf "$line $mjd"
             }
         } else {
             if {![eof $infile]} {
                incr Ncomm
                puts $outf "$line"
             }
         }
      }
 
      close $outfP
      close $outf
      close $infile

   echo "appended mjd for $Ngood lines and dumped to file ${file}.MJD"
   echo "fyi: found $Ncomm comment lines"
   echo "problems for $Nbad lines, dumped to file ${file}.problems"

}


## given asTrans file and field, return mjd when row 0 was read
## if problems, return -1
proc asTrans2MJD {asTransFile run camCol field {filter r}} {

     ## get proper TRANS
     set hdr [hdrReadAsFits [hdrNew] $asTransFile]   
     set Arun [hdrGetAsInt $hdr RUN]
     assert { $run == $Arun }
     set field0 [hdrGetAsInt $hdr FIELD0]
     set camCols [hdrGetAsAscii $hdr CAMCOLS]
     set filters [hdrGetAsAscii $hdr FILTERS]
     set ast_nfilters [llength $filters]  
    
     set icol [lsearch $camCols $camCol] 
     set ifilter [lsearch $filters $filter] 
     set ifield [expr $field - $field0]
     set hdu [expr 1 + $icol*$ast_nfilters]   

     set TRANSchain [fits2Schema $asTransFile TRANS $hdr -hdu $hdu] 
     set TRANS [chainElementGetByPos $TRANSchain $ifield]
     set mjd [exprGet $TRANS.mjd]

     # clean
     hdrDel $hdr
     genericChainDestroy $TRANSchain     

    return [format "%14.6f" $mjd]    
}




# problem with SDSSMOC2: did tsObj2moving use 
# set flagsOFF [list BRIGHT SATUR] or
#  set flagsOFF [list BRIGHT SATUR BLENDED]?

# what's the difference?

# with set flagsOFF [list BRIGHT SATUR]:
# photo> tsObj2moving Asteroids 94 7 1 105 450 STAR
# 1110 sources in Asteroids-run94_7_A.par

# with set flagsOFF [list BRIGHT SATUR BLENDED]:
# photo> tsObj2moving Asteroids 94 7 1 105 450 STAR
# 578 sources in Asteroids-run94_7_B.par 

# ADR1.dat has 395 objects from that region and 14.7<r<21.5
# B version produces 434 and A version 837



# aux proc to split a chain with (ra,dec) into three disjoint regions/chains
# 
proc splitDR1 {chain {ugMin -10} {ugMax 10}} {

     set good [chainSearch $chain "{ug > $ugMin} {ug < $ugMax}"]

     ### first get area covered by the southern survey
     set s [chainSearch $good "{ra < 100}"]
     set aux [chainSearch $good "{ra > 300}"]
     chainJoin $s $aux

     ### towards north pole
     set np [chainSearch $good "{ra > 100} {ra < 300} {dec >= 15.5}"]
     # and north around equator
     set ne [chainSearch $good "{ra > 100} {ra < 300} {dec < 15.5}"] 
     
     set Ns  [chainSize $s]
     set Nnp [chainSize $np]
     set Nne [chainSize $ne]
     set total [expr $Ns + $Nnp + $Nne]
     set Nin [chainSize $good]
     chainDel $good

     echo "Out of [chainSize $chain] objects, selected $Nin with $ugMin < ug < $ugMax,"
     echo "and then selected total of $total objects in these regions:"
     echo "$Ns in south, $Nnp towards north pole, and $Nne around north equator"
     return "$s $np $ne"

}

################
# given a file with (RA, Dec, Ar, run, ugriz with errors), find all multiply 
# observed objects, coadd photometric measurements and produce MULTIEPOCH 
# structure for each unique object, e.g.
# source /u/ivezic/photoLinux/etc/toolbox.tcl 
# set basicCols {1 2 3 17}; set ugrizCols {4 5 6 7 8}; set errCols {10 11 12 13 14}
# selfMatchFile northTest.UVxRRLyr.dump $basicCols $ugrizCols $errCols testMULTI.dat
proc selfMatchFile {infile basicCols ugrizCols errCols outfile {returnChain 0} {Nmax 100000000}} {

global ignoreWarnings

     if {![info exists ignoreWarnings]} {set ignoreWarnings 0}

     # open input file 
     set inf [open $infile r]
 
     # make chain 
     set chain [chainNew SELFMATCH]
    
     # read data and place on chain  
     set Nline 0 
     set Nobj 0 
     while {![eof $inf] && $Nline < $Nmax} {
        set line [gets $inf]
        incr Nline
	if {[expr $Nline/1000*1000] == $Nline} {echo read line $Nline ...}
        # process line
        if {![eof $inf] && [string range $line 0 0] != "#"} {  
            set el [genericNew SELFMATCH]
            incr Nobj
            ## copy data
            handleSet $el.localID $Nobj
            handleSet $el.matched 0
	    handleSet $el.ra  [lindex $line [expr [lindex $basicCols 0]-1]]
            handleSet $el.dec [lindex $line [expr [lindex $basicCols 1]-1]]
            handleSet $el.Ar [lindex $line [expr [lindex $basicCols 2]-1]]
            handleSet $el.run [lindex $line [expr [lindex $basicCols 3]-1]]
            # mags and errors
            loop i 0 5 {
  	       handleSet $el.m<$i> [lindex $line [expr [lindex $ugrizCols $i]-1]]
  	       handleSet $el.mErr<$i> [lindex $line [expr [lindex $errCols $i]-1]]
            }
            # dereddened u-g color for quick selection/separation
            set u [expr [exprGet $el.m<0>]-1.873*[exprGet $el.Ar]]
            set g [expr [exprGet $el.m<1>]-1.377*[exprGet $el.Ar]]
            handleSet $el.ug [expr $u-$g]
            # add this element to chain
            chainElementAddByPos $chain $el TAIL AFTER
            handleDel $el
        }
     }
     close $inf

     if {$returnChain} {  
        return $chain
     }

     assert { $Nobj == [chainSize $chain] }
     echo " Read $Nobj measurements, going to process..."
     set unique [selfMatchChain $chain 2 1.5 -90 90 $ignoreWarnings]
     # clean
     if {[info commands phGenericChainDestroy] == ""} {
        genericChainDestroy $chain genericDel
     } else {
        phGenericChainDestroy $chain
     }
     echo " Found [chainSize $unique] unique sources, going to dump..."

     # dump MULTIEPOCH chain to output file
     chain2Param $outfile $unique  ""

   return $unique

}


## wrapper around selfMatchChain to speed up by splitting
proc selfMatchSplitChain {chain {decStep 0.5} {type 2} {distMax 1.5}} { 

    # step of 0.5 deg implies fuzziness of about 1 in 10,000

    set decVec [vectorsFromChain $chain dec]
    set decMin [vExtreme $decVec min]
    set decMax [vExtreme $decVec max]
    vListDel $decVec

    set Nsteps [expr int(1+($decMax-$decMin)/$decStep)]
    # overlap for fuzziness in astrometry
    set fuzzi [expr 3.0/3600.0]
 
    loop i 0 $Nsteps {
	set Dec1 [expr $decMin + $i*$decStep]
	set Dec1fuzzy [expr $Dec1 - $fuzzi]
        set Dec2 [expr $decMin + ($i+1)*$decStep]
	set Dec2fuzzy [expr $Dec2 + $fuzzi]
        set s [chainSearch $chain "{dec >= $Dec1fuzzy} {dec < $Dec2fuzzy}"]
        # echo "selfmatching [chainSize $s] elements with $Dec1 <= dec < $Dec2"
        set aux [selfMatchChain $s 2 1.5 $Dec1 $Dec2]
        if {$i > 0} {
            chainJoin $unique $aux
        } else { 
            set unique $aux
        }
        chainDel $s
    }

  return $unique

}


# given a SELFMATCH chain,  find all multiply observed objects, 
# coadd photometric measurements and return a MULTIEPOCH chain
# with data for each unique object if type = 1, or an XEPOCH chain
# if type = 2
proc selfMatchChain {chain {type 2} {distMax 1.5} {DecMinChain -90} {DecMaxChain 90} \
			 {ignoreWarnings 0}} {

      # from arcsec to deg
      set distMax [expr $distMax/3600.0]

      # output chain
      if {$type == 1} {
          set unique [chainNew MULTIEPOCH]
      } else {
          set unique [chainNew XEPOCH]
      }
      set Nunique 0

      ## loop over input chain and search for all unmatched
      ## sources within distMax arcsec
      loop i 0 [chainSize $chain] {
         # next unmatched element
	 set el [chainElementGetByPos $chain $i]
	 if {[exprGet $el.matched]} {
            handleDel $el     
            continue
         }
         # match only if within "non-fuzzy" region
	 if {[exprGet $el.dec] < $DecMinChain || [exprGet $el.dec] > $DecMaxChain} {
            handleDel $el     
            continue
         }
         # so far this is an unmatched acceptable source...
	 set ra [exprGet $el.ra]; set dec [exprGet $el.dec] 
         incr Nunique
 	 if {[expr $Nunique/1000*1000] == $Nunique} {echo source $Nunique (meas.=$i)...}
         # find good matches
         set matchedChain [chainNew SELFMATCH]
         set cosDec [expr cos($dec/180.0*3.14159)]
	 if {$cosDec > 1.0e-6} {
	     set raMin [expr $ra-$distMax/$cosDec]
	     set raMax [expr $ra+$distMax/$cosDec]
         } else {
             set raMin 0; set raMax 360
         }
	 set decMin [expr $dec-$distMax]  
	 set decMax [expr $dec+$distMax]  
         # search for nearby unmatched objects
	 set cond "{matched == 0} {ra > $raMin} {ra < $raMax} {dec > $decMin} {dec < $decMax}"
	 set near [chainSearch $chain $cond] 
         # go through this presumably small chain and add all elements 
         # that are inside distMax circle on matchedChain 
	 loop j 0 [chainSize $near] {
    	    set auxel [chainElementGetByPos $near $j]
	    set Xra [exprGet $auxel.ra]; set Xdec [exprGet $auxel.dec]
	    set dist [distEq $ra $dec $Xra $Xdec]
	    if {$dist <= $distMax} {
                chainElementAddByPos $matchedChain $auxel TAIL AFTER
            }
            handleDel $auxel
         }
         chainDel $near
         # coadd all elements from matchedChain and return as an 
         # MULTIEPOCH/XEPOCH, and set el.matched to 1 for all
         set multi [coaddSELFMATCH $matchedChain $type $ra $dec]
         chainDel $matchedChain
         chainElementAddByPos $unique $multi TAIL AFTER 
         handleDel $multi
      }  

   return $unique 

} 


# aux (should go to C level)
proc distEq {RA1 Dec1 RA2 Dec2 {arcsec 0}} {

      set dRA  [expr $RA1-$RA1]
      set dDec [expr $Dec1-$Dec2]
      set cosDec [expr cos(0.5*($Dec1+$Dec2)/180.0*3.14159)]
      set dist [expr sqrt(pow($dRA*$cosDec,2)+pow($dDec,2))] 

      if {$arcsec} {
	  return [expr 3600.0*$dist]
      } else {
          return $dist
      }
}


# given a SELFMATCH chain, coadd all magnitudes and return MULTIEPOCH/XEPOCH
# RA/Dec is for QA
proc coaddSELFMATCH {chain type RA Dec}  {

global ignoreWarnings

     if {![info exists ignoreWarnings]} {set ignoreWarnings 0}

     if {$type == 1} {
         set multi [genericNew MULTIEPOCH]          
     } else {
	 set multi [genericNew XEPOCH]
     }         

     # sort by increasing run (i.e time) number
     chainSort $chain run -increasing

     set Nmatched [chainSize $chain]
     foreach rec "N Ns Ng" {
        handleSet $multi.$rec $Nmatched
     }

     set runList ""
     loop i 0 $Nmatched {
        set el [chainElementGetByPos $chain $i]
        handleSet $el.matched $Nmatched
	set thisRun [exprGet $el.run]
        if {[lsearch $runList $thisRun] < 0} {
            set runList "$runList $thisRun"
	} else {
            # this is match from the same run: this is either 
            # a serious bug, or two photo objects closer than
            # matching radius (~1.5 arcsec), ignore this source 
            if {!$ignoreWarnings} {
              echo " WARNING: duplicate run for chain=$chain, dumping to Duplicate_$chain.par"
              chain2Param Duplicate_$chain.par $chain ""
            }
            continue
	}
        if {$type == 1} {
            # here comes proper coadding of all measurements
        } else {
            # for now simply copy the first four epoch
            if {[stuffXEPOCH $el [expr $i+1] $multi]} {
                echo problems in stuffXEPOCH for i=$i and chain $chain
                if {!$ignoreWarnings} {error}
            }
        }
        handleDel $el  
     }

    return $multi

}


# given a SELFMATCH and its epoch, copy data to XEPOCH
proc stuffXEPOCH {el i multi} {
 
    if {$i <= 0} {error "stuffXEPOCH: i can't be <=0!?!"}

    if {$i > 9} {return 0} 

    if {$i == 1} {
       foreach rec "ra dec Ar" {
          handleSet $multi.$rec [exprGet $el.$rec]
       }      
    }
    set m m$i
    set mErr mErr$i
    set run run$i 
    handleSet $multi.$run [exprGet $el.run]
    loop j 0 5 {
       handleSet $multi.$m<$j>    [exprGet $el.m<$j>]
       handleSet $multi.$mErr<$j> [exprGet $el.mErr<$j>]
    }  
  
    if {[exprGet $multi.run1] == [exprGet $multi.run2]} {
        return 1
    }
   
   return 0
}





# structure which holds basic data for selfmatching
typedef struct {
    int localID;
    int matched;
    double ra;
    double dec;
    float Ar; 
    float ug;
    float m[5];
    float mErr[5];
    int run;
} SELFMATCH;


typedef struct {
    double ra;
    double dec;
    float Ar; 
    int N;
    int Ns;
    int Ng;
    float m[5];
    float mErr[5];
    float psf_mu[5];
    float psf_sig[5];
    float psf_chi2[5];
    float psf_min[5];
    float psf_max[5];
    float mod_mu[5];
    float mod_sig[5];
    float mod_chi2[5];
    float mod_min[5];
    float mod_max[5];
} MULTIEPOCH;

# temp. struct until MULTIEPOCH finished
typedef struct {
    double ra;
    double dec;
    float Ar; 
    int N;
    int Ns;
    int Ng;
    float m1[5];
    float mErr1[5];
    int run1; 
    double mjd1; 
    float m2[5];
    float mErr2[5];
    int run2; 
    double mjd2; 
    float m3[5];
    float mErr3[5];
    int run3; 
    double mjd3; 
    float m4[5];
    float mErr4[5];
    int run4; 
    double mjd4; 
    float m5[5];
    float mErr5[5];
    int run5; 
    double mjd5; 
    float m6[5];
    float mErr6[5];
    int run6; 
    double mjd6; 
    float m7[5];
    float mErr7[5];
    int run7; 
    double mjd7; 
    float m8[5];
    float mErr8[5];
    int run8; 
    double mjd8; 
    float m9[5];
    float mErr9[5];
    int run9; 
    double mjd9; 
} XEPOCH;


### given an XEPOCH chain, stuff MJDs with data obtained from file runMJDfile,
### where run and mjd are given in cols runCol and mjdCol
# e.g.
# XEPOCHstuffMJD $c ADR2.runList 1 4
proc XEPOCHstuffMJD {chain runMJDfile runCol mjdCol} {

     # first read run/mjd data
     set infile [open $runMJDfile r]
     set runList ""
     
     while {![eof $infile]} {
         set line [gets $infile]
         # process line
         if {![eof $infile] && [string range $line 0 0] != "#"} {  
	     set run [lindex $line [expr $runCol-1]]
             if {[lsearch $runList $run] < 0} {
                 set runList "$runList $run"
                 set mjd($run) [lindex $line [expr $mjdCol-1]]
             } else {
                error "double listing for run $run in file $runMJDfile"
             }
         }
     }
     close $infile

     echo Run-MJD data read for [llength $runList] runs
     set orphanRuns ""
     set N10 0

     loop i 0 [chainSize $chain] {
	if {[expr $i/1000*1000] == $i} {echo source $i}
        set el [chainElementGetByPos $chain $i]
        set goodSource 1
        set N [exprGet $el.N]
	if {$N > 9} {
           incr N10
           set N 9
        }
        loop j 1 [expr $N+1] {
	    if {$goodSource} {
	       set run [exprGet $el.run$j]
               if {[lsearch $runList $run] < 0} {
	          # echo [exprGet $el]
                  echo "don't have mjd for run $run"
                  set orphanRuns "$orphanRuns $run"
                  handleSet $el.run$j 0
                  handleSet $el.N [expr $j-1]
                  set goodSource 0
               } else {
                  handleSet $el.mjd$j $mjd($run)
               }
	    }
        } 
        handleDel $el
     }

  echo N10 = $N10
  return $orphanRuns 

}


### given an XEPOCH chain, dump to ascii file with the 
# following output:
# ra  dec  Ar  Nepoch (MJD run u uErr g gErr ... z zErr)1 ... ()9"        
# e.g.
# XEPOCH2ascii $c Catalog.dat
proc XEPOCH2ascii {chain outfile} {

global ignoreWarnings

    if {![info exists ignoreWarnings]} {set ignoreWarnings 0}

    # reddening coeffiecients
    #set reddCoeffs "1.87 1.38 1.00 0.76 0.54"
    #set redd [expr $Ar*[lindex $reddCoeffs $j]]

    set outf [open $outfile w]
       
    set dummyEpoch "    0.0000    0  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00"

    loop i 0 [chainSize $chain] {
	set el [chainElementGetByPos $chain $i] 
        set ra  [format "%10.5f" [exprGet $el.ra]]
        set dec [format "%9.5f" [exprGet $el.dec]]
        set Ar  [format "%5.2f" [exprGet $el.Ar]]
        set N [exprGet $el.N]
        if {$N < 10} {
           set N [format "%1.0f" $N]
        } else {
           set N 9
        }
        set line "$ra $dec $Ar $N" 
        # loop over all epochs 
        loop k 1 10 {
	   if {$k > $N} {
              set line "$line $dummyEpoch"            
           } else {
              set MJDf [format "%10.4f" [exprGet $el.mjd$k]]
              set runf [format "%4d" [exprGet $el.run$k]]
              set epoch "$MJDf $runf"
              loop j 0 5 {
	        set f [lindex "u g r i z" $j]
                set m [exprGet $el.m$k<$j>]
	        set mErr [format "%5.2f" [exprGet $el.mErr$k<$j>]]
                set epoch "$epoch $m $mErr"
              }
              set line "$line $epoch"            
	   }
	}
        puts $outf $line
        handleDel $el
    } 

   close $outf

   return
}


### given an XEPOCH chain, dump basic info (as for Wakker) to outfile
# ra  dec  Ar  u  g  r  i  z  du  dg  dr  di  dz  run1  run2"        
# XEPOCH2catalog $neU NEunique_test.Wdat 0
proc XEPOCH2catalog {chain outfile {meanMag 1}} {

global ignoreWarnings

    if {![info exists ignoreWarnings]} {set ignoreWarnings 0}

    # reddening coeffiecients
    set reddCoeffs "1.87 1.38 1.00 0.76 0.54"

    set outf [open $outfile w]

    loop i 0 [chainSize $chain] {
	set el [chainElementGetByPos $chain $i] 
        set ra  [format "%10.5f" [exprGet $el.ra]]
        set dec [format "%9.5f" [exprGet $el.dec]]
        set Ar  [format "%6.2f" [exprGet $el.Ar]]
        loop j 0 5 {
	   set f [lindex "u g r i z" $j]
	   set redd [expr $Ar*[lindex $reddCoeffs $j]]
           set m1 [exprGet $el.m1<$j>]
           set m2 [exprGet $el.m2<$j>]
	   if {$meanMag} {
              # mean reddening corrected magnitudes
	      set m($f)  [format "%5.2f" [expr 0.5*($m1+$m2)-$redd]]
	   } else { 
              # for dumping unique sources
	      set m($f) [format "%5.2f" [expr $m1-$redd]]               
           }
	   if {$m($f) < 12 && !$ignoreWarnings} {
              echo WARNING m($f) < 12: m1= $m1, m2 = $m2 redd = $redd Ar = $Ar
              set m($f) 99.99
              # error
	   }
           # magnitude difference
	   set dm($f) [format "%5.2f" [expr $m1-$m2]]
        }
        set run1 [exprGet $el.run1]
        set run2 [exprGet $el.run2]
        set Nobs [exprGet $el.N]
        set line "$ra $dec $Ar $m(u) $m(g) $m(r) $m(i) $m(z)"
        set line "$line $dm(u) $dm(g) $dm(r) $dm(i) $dm(z) $run1 $run2 $Nobs"
        puts $outf $line
        handleDel $el
    } 

   close $outf

   return
}


### given an XEPOCH chain, dump basic info (ra, dec, run, Ar, photometry)
# for the first epoch to outfile:
#  ra  dec  run  Ar  u  uErr  g gErr r rErr i iErr z zErr "        
# XEPOCH2catalog2 $c test.dat 0
proc XEPOCH2catalog2 {chain outfile} {

global ignoreWarnings

    if {![info exists ignoreWarnings]} {set ignoreWarnings 0}

    set outf [open $outfile w]

    loop i 0 [chainSize $chain] {
        if {[expr $i/1000*1000] == $i} {echo $i}
	set el [chainElementGetByPos $chain $i] 
        set ra  [format "%10.5f" [exprGet $el.ra]]
        set dec [format "%9.5f"  [exprGet $el.dec]]
        set run [format "%4.d"   [exprGet $el.run1]]
        set Ar  [format "%6.2f"  [exprGet $el.Ar]]
        set photo ""
        loop j 0 5 {
	   set f [lindex "u g r i z" $j]
	   set m  [format "%5.2f" [exprGet $el.m1<$j>]]
	   set mErr  [format "%5.2f" [exprGet $el.mErr1<$j>]]
           set photo "$photo $m $mErr"
        }
        set line "$ra $dec $run $Ar $photo"
        puts $outf $line
        handleDel $el
    } 

   close $outf

   return
}



### given an XEPOCH chain, dump basic info (ra, dec, run)
# for the first epoch to outfile (for quick cleaning)
# XEPOCH2catalogQuick $c test.dat
proc XEPOCH2catalogQuick {chain outfile} {

global ignoreWarnings

    if {![info exists ignoreWarnings]} {set ignoreWarnings 0}

    set outf [open $outfile w]

    loop i 0 [chainSize $chain] {
        if {[expr $i/1000*1000] == $i} {echo $i}
	set el [chainElementGetByPos $chain $i] 
        set ra  [format "%10.5f" [exprGet $el.ra]]
        set dec [format "%9.5f"  [exprGet $el.dec]]
        set run [format "%4.d"   [exprGet $el.run1]]
        set Ar  [format "%6.2f"  [exprGet $el.Ar]]
        set photo ""
        loop j 0 5 {
	   set f [lindex "u g r i z" $j]
	   set m  [format "%5.2f" [exprGet $el.m1<$j>]]
	   set mErr  [format "%5.2f" [exprGet $el.mErr1<$j>]]
           set photo "$photo $m $mErr"
        }
        set line "$ra $dec $run $Ar $photo"
        puts $outf $line
        handleDel $el
    } 

   close $outf

   return
}




### aux tools for quick testing
# e.g.
# set s [chainSearch $c "{N == 2}"]; quickTestXEPOCH $s
proc quickTestXEPOCH {chain} {

      set reclist "ra dec m1<0> m1<1> m1<2> m2<0> m2<1> m2<2>"
      set veclist [vectorsFromChain $chain $reclist]
      set ra [lindex $veclist 0]
      set dec [lindex $veclist 1]
      set u1 [lindex $veclist 2]
      set g1 [lindex $veclist 3]
      set r1 [lindex $veclist 4]
      set u2 [lindex $veclist 5]
      set g2 [lindex $veclist 6]
      set r2 [lindex $veclist 7]
      set dg [vectorExprEval $g1-$g2]
      set dr [vectorExprEval $r1-$r2]
      set mu [vectorExprEval 0.5*($u1+$u2)]
      set mg [vectorExprEval 0.5*($g1+$g2)]
      set mr [vectorExprEval 0.5*($r1+$r2)]
      set ug [vectorExprEval $mu-$mg]
      set gr [vectorExprEval $mg-$mr]

      plot_xy $ra $dec
      plot_xy $ug $mr
      plot_xy $ug $gr
      plot_xy $mr $dr
      plot_xy $ug $dr
      plot_xy $dg $dr
        
    vListDel $veclist

}

# for debug
proc reset_matched {chain} {
    
    loop i 0 [chainSize $chain] {
       set el [chainElementGetByPos $chain $i]
       handleSet $el.matched 0
       handleDel $el
    }

}

###################################################################
# apply the above tools to dumped files


## given a dump file from FNAL, do self matching and
## dump catalog with multiply observed sources
## e.g.
## source /u/ivezic/photoLinux/etc/toolbox.tcl    
## processMultipleObs northTest.UVxRRLyr.dump testOut 0.9 1.5
proc processMultipleObs {infile outfileRoot {ugMin -10} {ugMax 10} } {

     # first make chain
     set basicCols {1 2 3 17}
     set ugrizCols {4 5 6 7 8}
     set errCols {10 11 12 13 14}
     set chain [selfMatchFile $infile $basicCols $ugrizCols $errCols dummy 1]
     set Nin [chainSize $chain]
  
     # now split in three DR1 regions
     set split [splitDR1 $chain $ugMin $ugMax]
     loop i 0 3 {
         chain2Param $outfileRoot.split$i.par [lindex $split $i] ""
         echo dumped [chainSize [lindex $split $i]] elements to $outfileRoot.split$i.par
     }

     # and now process each region 
     set Ntot 0; set first 1
     loop i 0 3 {
         set thisN [chainSize [lindex $split $i]]
         set Ntot [expr $Ntot + $thisN]
         if {$thisN > 0} {
             echo "  selfmatching sample $i ($thisN observations)"
	     set aux [selfMatchSplitChain [lindex $split $i] 0.05 2 1.5] 
             chain2Param $outfileRoot.unique$i.par $aux ""
             echo dumped [chainSize $aux] unique elements to $outfileRoot.unique$i.par
             if {$first} {
                 set unique $aux
                 set first 0 
             } else {
                 chainJoin $unique $aux
             }
         }
     }

     # chain $unique contains all unique objects
     set Nunique [chainSize $unique]
     if {$Nunique <= 0} {
         echo "did not find any unique objects?!?"
         return
     }
     # now select multiply observed     
     set multiple [chainSearch $unique "{N > 1}"] 
     set Nmultiple [chainSize $multiple]

     ## and dump to outfiles
     # first the whole multiple chain
     chain2Param $outfileRoot.par $multiple ""
     # and then Wakker-like catalog
     XEPOCH2catalog $multiple $outfileRoot.Wdat  


     echo " *********************************************************"
     echo " ** From $Nin observations found in file"
     echo " ** $infile"
     echo " ** selected $Ntot with $ugMin < ug < $ugMax"
     echo " ** corresponding to $Nunique unique objects"
     echo " ** Dumped chain with $Nmultiple multiply observed objects to"
     echo " ** $outfileRoot.par"
     echo " ** and a Wakker-like catalog to $outfileRoot.Wdat"
     echo " *********************************************************"
     
     # clean
     chainDel $multiple
     if {[info commands phGenericChainDestroy] == ""} {
        echo destroying chain
        genericChainDestroy $unique genericDel
     } else {
        phGenericChainDestroy $unique
     }

}




# given a fileIn, with RA and Dec in colRA and colDec (1-indexed), 
# produce fileOut with all unique sources defined by distance of D arcsec
# e.g.
# source /u/ivezic/photoLinux/etc/toolbox.tcl
# getUniqueSubset GSC_varJ_2.5.dat GSC_varJ_2.5.unique.dat 1 2
# set ignoreWarnings 1; getUniqueSubset  GSC_varTest.dat unique.dat 1 2 
proc getUniqueSubset {fileIn fileOut colRA colDec {Dmax 1.0}} {

global ignoreWarnings express

     set ignoreWarnings 1

     ## get unique sources
     set f1 ${fileIn}.aux.xed
     set f2 ${fileIn}.aux.xep
    
     set basicCols "$colRA $colDec 1 1"; set ugrizCols {1 1 1 1 1}; set errCols {1 1 1 1 1}  
     set b [selfMatchFile $fileIn $basicCols $ugrizCols $errCols $f2]  
     echo SELFMATCHED, calling XEPOCH2catalogQuick
     XEPOCH2catalogQuick $b $f1

     set express -1 
     echo dumped unique list, cross-correlating with original catalog...  
     cross_correlate_list $f1 $fileIn $fileOut aux 1 2 0 $colRA $colDec 0 $Dmax 0 

     echo "unique sources from $fileIn are in $fileOut"
     echo destroying chain...
     chainDestroy $b genericDel
  
}


# aux: cross-correlate a Wdat catalog with Balmer line catalogs, e.g.
# WdatCCbalmer rrVar.Wdat AllStars_05012003.dat 
proc WdatCCbalmer {WdatFile rootBalmer} {
global express 

       set express -1
 
       foreach Bline {Halpha Hbeta Hgamma Hdelta} {
          set inf1 $WdatFile
	  set inf2 ${rootBalmer}.$Bline
	  set outf ${WdatFile}.$Bline 
          cross_correlate_list $inf1 $inf2 $outf notmatchfile.aux 1 2 0 4 5 0 1.0 1
       }

}

# totally aux
proc doAux {} {

    set c0 [param2Chain all2x.RRLyr.unique0.par ""]
    set c1 [param2Chain all2x.RRLyr.unique1.par ""]
    set c2 [param2Chain all2x.RRLyr.unique2.par ""]

    chainJoin $c0 $c1
    chainJoin $c0 $c2

    echo read [chainSize $c0] unique objects

    echo going to dump W cat

    XEPOCH2catalog $c0 all2x.RRLyr.unique.Wdat  

    echo going to dump par file
    chain2Param all2x.RRLyr.unique.par $c0 ""
 
    echo DONE!

}




# given a file with Stroemgren photometry (like e.g. DR1s.NE.dat), search 
# for doubles
# e.g.
# source /u/ivezic/photoLinux/etc/toolbox.tcl
# getUniqueGalaxies DR1s.NE.dat DR1s.NE_unique.dat
proc getUniqueGalaxies {fileIn fileOut {Dmax 1.0}} {

global ignoreWarnings 

     set ignoreWarnings 1

     ## get unique sources
     set f1 ${fileIn}.aux.xed
     set f2 ${fileIn}.aux.xep
     set fD ${fileIn}.doubles
    
     # RA, Dec, Ar, MJD
     set basicCols "1 2 16 15";
     # u g r plate/fiber 
     set ugrizCols {8 9 10 13 14}; 
     # U V B Y redshift
     set errCols {4 5 6 7 3}  
     ### call matching proc
     set b [selfMatchFile $fileIn $basicCols $ugrizCols $errCols $f2] 
     # dump all matches
     XEPOCH2catalog $b $f1 0
     # dump multiply observed sources
     set s2 [chainSearch $b "{N > 1}"]
     XEPOCH2catalogGal $s2 $fD
     # count and match to the original file
     set Nunique [chainSize $b]
     set Ndoubles [chainSize $s2]
     set express 1 
     echo going to final correlation of $f1 and $fileIn to make $fileOut 
     cross_correlate_list $f1 $fileIn $fileOut aux 1 2 0 1 2 0 $Dmax 0 

     echo "$Nunique unique sources from $fileIn are in $fileOut, $Ndoubles doubles are in $fD"
     echo "going to clean..."

     # clean 
     chainDel $s2
     chainDestroy $b genericDel
  
}


## aux helper for getUniqueGalaxies:
# dump basic info for doubly listed galaxies to outfile
# ra  dec  Ar u  g  r plate1 fib1 mjd1 z1 U1 V1 B1 Y1 plate2 fib2 mjd2 z2 U2 V2 B2 Y2"        
# XEPOCH2catalogGal $b test.Gdat
proc XEPOCH2catalogGal {chain outfile} {

global ignoreWarnings

    if {![info exists ignoreWarnings]} {set ignoreWarnings 0}

    set outf [open $outfile w]
    set line "#   ra         dec       Ar    u     g     r   plt1 fib1  mjd1   z1"
    set line "$line     U1    V1    B1    Y1   plt2 fib2  mjd2   z2      U2    V2    B2   Y2"
    puts $outf $line
    
    loop i 0 [chainSize $chain] {
	set el [chainElementGetByPos $chain $i] 
        set ra  [format "%10.5f" [exprGet $el.ra]]
        set dec [format "%9.5f" [exprGet $el.dec]]
        set Ar  [format "%6.2f" [exprGet $el.Ar]]
        set ugr ""
        loop j 0 3 {
	   set m1 [format "%5.2f" [exprGet $el.m1<$j>]]
           set ugr "$ugr $m1"
        }
        set line "$ra $dec $Ar $ugr"
        # first listing for Stroemgren
        set plate1 [format "%4.0f" [exprGet $el.m1<3>]]      
        set fib1   [format "%4.0f" [exprGet $el.m1<4>]]      
        set mjd1   [format "%5.0f" [exprGet $el.run1]] 
        set z1     [format "%6.4f" [exprGet $el.mErr1<4>]]  
        set Str1 ""
        loop j 0 4 {
	   set s [format "%5.2f" [exprGet $el.mErr1<$j>]]
           set Str1 "$Str1 $s"
        }
        set line "$line $plate1 $fib1 $mjd1 $z1 $Str1"
        set plate2 [format "%4.0f" [exprGet $el.m2<3>]]      
        set fib2   [format "%4.0f" [exprGet $el.m2<4>]]      
        set mjd2   [format "%5.0f" [exprGet $el.run2]] 
        set z2     [format "%6.4f" [exprGet $el.mErr2<4>]]  
        set Str2 ""
        loop j 0 4 {
	   set s [format "%5.2f" [exprGet $el.mErr2<$j>]]
           set Str2 "$Str2 $s"
        }
        set line "$line $plate2 $fib2 $mjd2 $z2 $Str2"
        puts $outf $line
        handleDel $el
    } 

   close $outf

   return
}



### for investigating fast changes of bias drift
### given a region reg:
### first find median value for rectangle
### defined by all rows and Bcol0 <= col <= Bcol1,
### and then correct row by row by adding the difference
### between that overall median and the median of pixels from
### that row and with Bcol0 <= col <= Bcol1, from each pixel
### with Dcol0 < col < Dcol1
### e.g. to fix idR-000211-u3-0309.fit
### set reg [regReadAsFits [regNew] idR-000211-u3-0309.fit.Z]
### straightenBiasDrift $reg 0 20 0 1064
### straightenBiasDrift $reg 2108 2127 1065 2127
### regWriteAsFits 
proc straightenBiasDrift {reg Bcol0 Bcol1 Dcol0 Dcol1} {

    # find median bias drift
    set nrow [exprGet $reg.nrow]
    set ncol [exprGet $reg.ncol]
    set nDataCol [expr $Dcol1-$Dcol0+1]
    set medVec [vectorGetFromRegion $reg 0 [expr $nrow-1] $Bcol0 $Bcol1 0 -median]
    set biasDrift [vMedian $medVec]
    vectorExprDel $medVec

    echo median biasDrift = $biasDrift

    # now correct row by row
    loop row 0 $nrow {
       set medVec [vectorGetFromRegion $reg $row $row $Bcol0 $Bcol1 0 -median]
       set biasDriftLocal [vMedian $medVec]
       vectorExprDel $medVec
       # and now correct
       set corr [expr int($biasDrift - $biasDriftLocal+0.5)]
       #echo row $row: correcting by $corr counts (biasDriftLocal = $biasDriftLocal)
       set subReg [subRegNew $reg 1 $nDataCol $row $Dcol0]
       regAddWithDbl $subReg $corr
       regDel $subReg
    }
}

