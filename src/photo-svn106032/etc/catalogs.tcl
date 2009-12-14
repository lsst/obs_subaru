
#
# The TCL procedures in this file are designed to help users
#    match up objects in simulation catalogs with objects found
#    by PHOTO.  
#

# You should set this variable to a directory in which the simulation
#    catalog files live.  They are ASCII files with names like
#    "galaxycat", "starqsocat", and so forth.
global CATALOG_DIR 
set CATALOG_DIR /scr0/richmond/catalogs

# If you want to run quickly, it will help to create a single FITS
#    binary table which holds entries from all the simulation catalogs
#    over some small area; you can then run tests in that small area
#    using the FITS binary table, rather than the catalogs directly.
#
#    If you do create such a file, set SELECTED_FILE to its name
#    If you don't,                 set SELECTED_FILE to "0"
global SELECTED_FILE
set SELECTED_FILE run581.1.20_35.fit


#
# read a set of catalogs from disk, and combine them into one very
# large chain of items.  If the user gives an argument, use that
# as the directory containing the catalog(s) to be read.
# Attempt to read all the known catalogs that can be found in the directory.
#
# Return a single CHAIN which is filled with items from all catalogs,
# combined together.
#
proc read_catalogs { } {

  global SELECTED_FILE CATALOG_DIR

  set dir $CATALOG_DIR
  set retch [chainNew CATOBJ]

  if { $SELECTED_FILE != 0 } {

    set fname run581.1.20_35.fit
    set ch [fits2Schema $dir/$fname CATOBJ]
    set size [chainSize $ch]
    echo "   .... read $size items from file $fname"
    chainJoin $retch $ch

  } else {

    foreach cat { asteroidcat galaxycat gscprunecat ppmcat qq \
                  starqsocat ugccat } {

      set fname $dir/$cat

      if { [catch { file exist $fname }] } {
        echo "skipping absent catalog file $fname"
      } else {
        echo "reading catalog file $fname ... "
        set ch [simCatalogToChain $fname]
        set size [chainSize $ch]
        echo "   .... read $size items "
        chainJoin $retch $ch
      }
    }
  }

  set size [chainSize $retch]
  echo "total catalog chain contains $size items"

  return $retch
}


proc read_it { dir run col field } {

  set fname [format "%s/fpObjc-%06d-%1d-%04d.fit" $dir $run $col $field]
  set hdr [hdrNew]

  set ch [fits2Schema $fname OBJC_IO $hdr]

  hdrDel $hdr

  return $ch
}

proc read_cbs { dir run col field0 } {
  upvar $field0 f0

  set fname [format "%s/psCB-%06d-%1d.fit" $dir $run $col]

  # read the header, to get FIELD0 value
  set hdr [hdrNew]
  hdrReadAsFits $hdr $fname
  set f0 [hdrGetAsInt $hdr FIELD0]
  set nfields [hdrGetAsInt $hdr NFIELDS]

  # now read the structures for each field, and make sure that the number
  #   of fields matches the "NFIELDS" value
  loop field 1 [expr $nfields+1] {
    set ch [fits2Schema $fname READTRANS -hdu=$field]
    if { $field == 1 } {
      set retch $ch
    } else {
      chainJoin $retch $ch
    }
  }
  set size [chainSize $retch]
  if { $size != $nfields } {
    error "read_transes: NFIELDS is $nfields, but read $size fields"
    assert { 0 }
  }

  return $retch
}


# 
# Given a CHAIN from the "psCB" file, a field number, and a filter index,
#   return a TRANS structure that will take the (row, col)
#   coords of the objects detected in that frame and convert
#   them to (mu, nu) Great Circle coords.
#
# Also, return the values of "node" and "incl" in the final two arguments
#
proc get_trans_from_cb { cbchain field0 field filter_index node incl } {
  upvar $node nod $incl inc

  # figure out which element in the CHAIN contains the field of interest
  set num [expr $field-$field0]
  set size [chainSize $cbchain]
  assert { $num >= 0 && $num < $size }

  # get the correct element from the CHAIN, and extract the proper TRANS
  set el [chainElementGetByPos $cbchain $num]
  set nfilt [exprGet $el.ncolors]
  assert { $nfilt > 0 && $nfilt > $filter_index }
  
  set nod [exprGet $el.node${filter_index}]
  set inc [exprGet $el.incl${filter_index}]
  set trans [transNew]
  handleSet $trans.a [exprGet $el.togcca${filter_index}]
  handleSet $trans.b [exprGet $el.togccb${filter_index}]
  handleSet $trans.c [exprGet $el.togccc${filter_index}]
  handleSet $trans.d [exprGet $el.togccd${filter_index}]
  handleSet $trans.e [exprGet $el.togcce${filter_index}]
  handleSet $trans.f [exprGet $el.togccf${filter_index}]

  return $trans
}


#
# Apply a TRANS to every element of the given CHAIN
#
proc apply_trans { chain xname yname trans } {

  set a [exprGet $trans.a]
  set b [exprGet $trans.b]
  set c [exprGet $trans.c]
  set d [exprGet $trans.d]
  set e [exprGet $trans.e]
  set f [exprGet $trans.f]

  chainTransXY $chain $xname $yname $a $b $c $d $e $f 

  return 0
}

#
# convert from Great Circle coordinates (mu, nu) to Survey coords (lambda, eta)
#   every element of the given CHAIN.  Use the fields given by arguments
#   "muname", "nuname" as those containing (mu, nu), and convert in place,
#   so that when we finish, the same fields contain (lambda, eta)
#
proc chain_gc_to_survey { chain muname nuname node incl } {

  set size [chainSize $chain]
  loop i 0 $size {
    set el [chainElementGetByPos $chain $i]
    set mu [exprGet $el.$muname]
    set nu [exprGet $el.$nuname]
    set radec [GCToEq $mu $nu -node $node -inclination $incl]
    set survey [eqToSurvey [keylget radec ra] [keylget radec dec]]
    handleSet $el.$muname [keylget survey lambda]
    handleSet $el.$nuname [keylget survey eta]
    handleDel $el
  }
  return 0
}
  

#
# convert from Survey coords (lambda, eta) to Great Circle coordinates (mu, nu)
#   every element of the given CHAIN.  Use the fields given by arguments
#   "lamname", "etaname" as those containing (lambda, eta), and convert in 
#   place, so that when we finish, the same fields contain (mu, nu)
#
proc chain_survey_to_gc { chain lamname etaname node incl } {

  set size [chainSize $chain]
  loop i 0 $size {
    set el [chainElementGetByPos $chain $i]
    set lambda [exprGet $el.$lamname]
    set eta [exprGet $el.$etaname]
    set radec [surveyToEq $lambda $eta]
    set munu [eqToGC [keylget radec ra] [keylget radec dec] \
                                                -node $node -incl $incl]
    handleSet $el.$lamname [keylget munu mu]
    handleSet $el.$etaname [keylget munu nu]
    handleDel $el
  }
  return 0
}

#
# given a CHAIN of structures, find the max and min values of the
#   given field, returning them in the passed arguments
#
proc chain_minmax { chain fieldname minval maxval } {
  upvar $minval minv $maxval maxv

  set af [afFromChain $chain $fieldname]
  set minv [afExtreme $af min]
  set maxv [afExtreme $af max]
  afDel $af

  return 0
}


# 
# given a CHAIN of objects detected by PHOTO, convert their fluxes
# in the "fieldname" field, which may be
#
#        a) a single value (in which case "index" must be -1)
#   or
#        b) an array (in which case "index" specifies the index into array)
#
# an array of <= index-1 elements,
# convert each element of the given field from intensity to magnitude units.
# We use a magnitude zero point of 30.0.
#
# Any flux <= 0.0 is set to the magzero point.
#
proc chain_flux_to_mag { chain fieldname index } {

  set magzero 30.0

  set size [chainSize $chain]
  loop i 0 $size {
    set el [chainElementGetByPos $chain $i]
    if { $index >= 0 } {

      # "fieldname" is an array of items, from which we must pick 
      # the "index"'th one
      set x [exprGet $el.$fieldname<$index>]
      if { $x <= 0.0 } {
        handleSet $el.$fieldname<$index> $magzero
      } else {
        set mag [expr $magzero-1.0857362*log($x)]
        handleSet $el.$fieldname<$index> $mag
      }

    } else {

      # "fieldname" is not an array, so don't use "index"
      set x [exprGet $el.$fieldname]
      if { $x <= 0.0 } {
        handleSet $el.$fieldname $magzero
      } else {
        set mag [expr $magzero-1.0857362*log($x)]
        handleSet $el.$fieldname $mag
      }

    }

    handleDel $el
  }

  return 0
}
   
# 
# given a CHAIN of objects detected by PHOTO, convert each element in 
# the "fieldname" field, which may be
#
#        a) a single value (in which case "index" must be -1)
#   or
#        b) an array (in which case "index" specifies the index into array)
#
# from magnitude to intensity units.
# We use a magnitude zero point of 30.0.  Any magnitude = 30.0
# is converted to flux 0.0.
#
proc chain_mag_to_flux { chain fieldname index } {

  set magzero 30.0

  set size [chainSize $chain]
  loop i 0 $size {
    set el [chainElementGetByPos $chain $i]

    if { $index >= 0 } {

      # "fieldname" is an array 
      set x [exprGet $el.$fieldname<$index>]
      if { $x == $magzero } {
        handleSet $el.$fieldname<$index> 0.0
      } else {
        set mag [pow 10.0 [expr -0.4*($x-$magzero)]]]
        handleSet $el.$fieldname<$index> $mag
      }

    } else {

      # "fieldname" is not an array, but a single value
      set x [exprGet $el.$fieldname]
      if { $x == $magzero } {
        handleSet $el.$fieldname 0.0
      } else {
        set mag [pow 10.0 [expr -0.4*($x-$magzero)]]]
        handleSet $el.$fieldname $mag
      }
    }

    handleDel $el
  }

  return 0
}
   

#
# this is a kludge that allows us to sort a PHOTO-output CHAIN by a magnitude;
#   we can't type
#                   chainSort $ch psfcounts<2>
#   because chainSort doesn't understand that.  So, we can use this
#   procedure to set one of the (few) simple fields in the OBJC
#   structure equal to some desired magnitude, and then sort it.
#
proc kludge { ch fromfield tofield } {

  set size [chainSize $ch]
  loop i 0 $size {
    set el [chainElementGetByPos $ch $i]
    set x [exprGet $el.$fromfield]
    handleSet $el.$tofield $x
    handleDel $el
  }
  return 0
}


#
# given a CHAIN of detected PHOTO objects, and a TRANS structure that converts
#   (row, col) to great-circle coordinates (mu, nu), and values for
#   node and inclination of the great circle, convert object's 
#   (row, col) -> (mu, nu) -> (RA, Dec) -> (lambda, eta)
#   and find the max and min values of lambda and eta.  Return
#   these in the passed arguments.
#
proc chain_find_extreme_coords { ch rowname colname trans node incl \
                                   minlambda maxlambda mineta maxeta } {
  upvar $minlambda minl $maxlambda maxl
  upvar $mineta mine $maxeta maxe

  # convert to (lambda, eta) coords
  apply_trans $ch $rowname $colname $trans
  chain_gc_to_survey $ch $rowname $colname $node $incl

  # now find the min and max values of lambda and eta in the detected objects
  chain_minmax $ch $rowname minl maxl
  chain_minmax $ch $colname mine maxe

  return 0
}


# figure out a "better" TRANS structure connecting catalog to detected
#   coordinate system.  The user supplies
#
#     a. a CHAIN of matched catalog objects, whose "(lambda, eta)" values
#            have _already_ been convered to (row, col) by a rough TRANS
#     b. a CHAIN of matched detected objects, one for each of the catalog
#            objects in a.
#     c. a CHAIN of all catalog objects, whose "(lambda, eta)" values
#            really do contain "lambda" and "eta"
#
#   as well as
#
#     d. the name of a magnitude field in catalog objects
#     e. the name of a magnitude field in detected objects
#             (which is assumed to have been convered to magnitude 
#             units already)
#
#   This procedure causes the CHAIN c to contain the same objects as
#   those in a, then runs atFindTrans on the two perfectly-matched
#   sets of objects to get a very good TRANS structure.
#  
proc better_trans { catch detch orig_catch catmag detmag } {

  set nobj 50
  set radius 0.0001

  set tempcatch [chainNew CATOBJ]
  set tempdetch [chainCopy $detch]

  set matched_size [chainSize $catch]
  set orig_size [chainSize $orig_catch]
  chainSort $orig_catch id -increasing

  loop i 0 $matched_size {

    set el [chainElementGetByPos $catch $i]
    set id [exprGet $el.id]

    # we SHOULD be able to call "chainSelect" here, but it seems to be
    #   broken.  MWR 3/11/96

    set index [search_for_id $orig_catch $orig_size $id]
    set el2 [chainElementGetByPos $orig_catch $index]
    chainElementAddByPos $tempcatch $el2 TAIL AFTER
    handleDel $el2

    handleDel $el
  }

  assert { [chainSize $tempcatch] == $matched_size }
  assert { [chainSize $tempcatch] == [chainSize $tempdetch] }

  # now, at this point, the (temporary) CHAINs "tempdetch" and "tempcatch"
  #   have exactly the same number of items, and they match one-for-one.
  #   If I delete all but the first $nobj items from each, and then
  #   call "atFindTrans", I will force it to look at two sets of 
  #   exactly corresponding points.
  set size [chainSize $tempdetch]
  set deln [expr $nobj]
  loop i $nobj $size {
    chainElementRemByPos $tempdetch $deln
    chainElementRemByPos $tempcatch $deln
  }

  # now we can find the better trans
  set trans [atFindTrans $tempcatch lambda eta $catmag  \
                         $tempdetch objc_rowc objc_colc $detmag \
                         -nobj $nobj -radius $radius]

  chainDel $tempcatch
  chainDel $tempdetch

  return $trans
}


# run a binary search to find an item in the given chain with the given
#   ID value.  Return the index to the item, which MUST exist.
#   
#     chain        CHAIN of CATOBJs in which we search
#     size         size of CHAIN
#     id           ID value we are looking for
# 
# We ought to be able to use "chainSelect" to do this, but it isn't
#   working as of Mar 11, 1996.  MWR
#
proc search_for_id { chain size id } {

  set top 0
  set bottom [expr $size-1]

  while { [expr $bottom-$top] > 1 } {

    set mid [expr ($top+$bottom)/2]

    set el [chainElementGetByPos $chain $mid]
    set this_id [exprGet $el.id]
    handleDel $el
    if { $this_id < $id } {
      set top $mid
    } else {
      set bottom $mid
    }
  }

  # at this point, desired item is either the "top'th" or "bottom'th"
  #   (which may be equal)
  set el [chainElementGetByPos $chain $top]
  set this_id [exprGet $el.id]
  handleDel $el
  if { $this_id < $id } {
    return $bottom
  } else {
    return $top
  }
}
    

   
  
#
# count the number of OBJC_IOs in the given CHAIN with each "type";
#   print a message giving the total number of objects, and the
#   number with each type
#
# use the zero'th color of each object to access the "type" field
#
# We look specifically for STAR, GAL, QSO, and ignore all other types.
#   
proc count_types { ch } {

  set color 0
  set size [chainSize $ch]

  set num(STAR) 0
  set num(GAL)  0
  set num(QSO)  0

  loop i 0 $size {
    set el [chainElementGetByPos $ch $i]
    set type [exprGet $el.type<$color>]
    foreach t { STAR GAL QSO } {
      if { [regexp $t $type] == 1 } {
        set num($t) [expr $num($t)+1]
      }
    } 

    handleDel $el
  }

  echo "total of $size detected and matched objects"
  echo "  number of stars:      $num(STAR)"
  echo "  number of galaxies:   $num(GAL)"
  echo "  number of quasars:    $num(QSO)"
}
  


#
# given a CHAIN, make an X-Y plot, using data from the field "xfield"
#   on the X-axis and from "yfield" on the Y-axis.  Use symbols of
#   different types for stars and galaxies and QSOs.  Use optional
#   min and max values along each axis, if they are supplied.
#
proc chain_plot { ch xfield yfield color {xmin -99} {xmax -99} \
                                         {ymin -99} {ymax -99} } {

  set pg [pgstateNew]
  pgstateOpen $pg

  set ltypes [list STAR GAL QSO]
  set lsymbs [list 3    2   5  ]

  # now we make chains containing only stars (or galaxies, or ...), and
  #   plot each of those chains with different symbols.  We assume that
  #   the $color'th color contains a valid type.
  set size [chainSize $ch]
  
  set klist [chainTypeGet $ch]
  set chtype [keylget klist type]
  set first 1
  set ntype [llength $ltypes]
  loop n 0 $ntype {
    set t [lindex $ltypes $n]
    set newch [chainNew $chtype]
    loop i 0 $size {
      set el [chainElementGetByPos $ch $i]
      set type [exprGet $el.type<$color>]
      if { [regexp $t $type] == 1 } {
        chainElementAddByPos $newch $el TAIL AFTER
      }
      handleDel $el
    }
    if { [chainSize $newch] == 0 } {
      continue
    }
    set newsize [chainSize $newch]
    echo "type $t has $newsize elems"
    
    set afx [afFromChain $newch $xfield]
    set afy [afFromChain $newch $yfield]
    if { $xmin == -99 } {
      set xmin [afExtreme $afx min]
    }
    if { $xmax == -99 } {
      set xmax [afExtreme $afx max]
    }
    if { $ymin == -99 } {
      set ymin [afExtreme $afy min]
    }
    if { $ymax == -99 } {
      set ymax [afExtreme $afy max]
    }

    if { $first == 1 } {
      set first 0
      pgstateSet $pg -isNewplot 1 -plotTitle "stars *  galaxies +" 
    } else {
      pgstateSet $pg -isNewplot 0
    }

    pgstateSet $pg -symb [lindex $lsymbs $n]
    afPlot $pg $afx $afy -xmin $xmin -xmax $xmax -ymin $ymin -ymax $ymax 

    afDel $afx
    afDel $afy

    chainDel $newch
  }

}
      
  
  

#
# given a CHAIN containing all catalog items, and the identity of some field, 
#
#   1. find the PHOTO output file, and read it into a chain
#          convert the "magfield" data from intensity to magnitude
#   2. find the corresponding area in all the object catalogs
#          and create a chain of catalog objects
#   3. calculate a TRANS that takes catalog objects to detected objects
#          using the TRANS appropriate for the "ref_filt" filter index
#          (we hope that this is almost identical to the TRANS for
#          other filter indices)
#          Use magnitudes based on, in the detected object list, 
# 
#                   "$magfield<$magindex>"     if $magindex >= 0
#                    $magfield                 if $magindex < 0
#
#          and based on the "$catmag" field in the catalog
#          to perform the match.
#   4. use that TRANS to match up the catalog and detected objects
#          and return that trans in the "cat_to_det_trans" argument
#   5. return (in the final two arguments) CHAINs with
#          matched objects from the catalog (with row/col coordinates)
#          matched detected objects from PHOTO outputs (also row/col coords)
#          unmatched objects from catalog
#          unmatched objects from detected output
#
proc match_output { catch catmag \
                    photo_dir run col field magfield magindex ref_filt \
                    cat_to_det_trans \
                    matched_cat matched_det unmatched_cat unmatched_det } {
  upvar $cat_to_det_trans trans
  upvar $matched_cat matc $matched_det matd 
  upvar $unmatched_cat umatc $unmatched_det umatd

  # some variables used in this proc only; vary to taste
  #   this the number of objects to use in atFindTrans
  set findTransNobj 50
  #   this is the radius in triangle-space for a match in atFindTrans
  set findTransRadius 0.001
  #   this is the radius, in pixels, used to match catalog and detected objects
  set matchChainsRadius 3.0

  # read the PHOTO output file _twice_ (we use one copy just to find
  #   extremes of (lambda, eta) and then delete that copy)
  set detch [read_it $photo_dir $run $col $field]
  set detch2 [read_it $photo_dir $run $col $field]
  chain_flux_to_mag $detch $magfield $magindex
  
  # read the TRANS structures that convert (row, col) to (mu, nu)
  set cbch [read_cbs $photo_dir $run $col field0]
  set size [chainSize $cbch]
  #echo "size of CB chain is $size, and field0 is $field0"
  set trans [get_trans_from_cb $cbch $field0 $field $ref_filt node incl]
  
  # this will cause the (objc_rowc, objc_colc) values of items in $detch2
  #   to become (lambda, eta) instead of (row, col)
  chain_find_extreme_coords $detch2 objc_rowc objc_colc $trans $node $incl \
                                   minlambda maxlambda mineta maxeta 
  # we don't need $detch2 anymore, so delete it
  chainDestroy $detch2 genericDel

  # pick the catalog objects which have positions within the rectangle
  #   defined by the detected objects
  set smallcatch [catobjSelectByPosition $catch $minlambda $maxlambda \
                                         $mineta $maxeta]
  set smallcatch2 [catobjSelectByPosition $catch $minlambda $maxlambda \
                                         $mineta $maxeta]

  # find the TRANS which converts catalog (lambda, eta) -> (row, col)
  if { $magindex >= 0 } {
    set trans1 [atFindTrans $smallcatch lambda eta $catmag \
                            $detch objc_rowc objc_colc $magfield<$magindex> \
                            -nobj $findTransNobj -radius $findTransRadius]
  } else {
    set trans1 [atFindTrans $smallcatch lambda eta $catmag \
                            $detch objc_rowc objc_colc $magfield \
                            -nobj $findTransNobj -radius $findTransRadius]
  }

  # now apply the transformation to catalog object in $smallcatch, and then
  #   find matches between them and detected objects in $detch
  atApplyTrans $smallcatch lambda eta $trans1
  atMatchChains $smallcatch lambda eta $detch objc_rowc objc_colc \
                      $matchChainsRadius matc matd umatc umatd

  # having made the matches, calculate a better TRANS using only matched items
  #   (actually, this doesn't often help, and it takes a while ... )
#  if { $magindex >= 0 } {
#    set trans2 [better_trans $matc $matd $smallcatch2 \
#                                      $catmag $magfield<$magindex>]
#  } else {
#    set trans2 [better_trans $matc $matd $smallcatch2 \
#                                      $catmag $magfield]
#  }


  # finally, convert the detected objects' magnitudes back into flux
  #   units
  chain_mag_to_flux $detch $magfield $magindex

  chainDestroy $smallcatch2 genericDel 

  set trans $trans1

}



# 
# This is an example of how one might use the TCL proc "match_output"
#    to end up with a set of 4 CHAINs containing items which
#    did/didn't match from catalog/detected.  The output parameters
#    in the call to "match_output" are
#
#    cat_to_det_trans    TRANS taking (lambda, eta) -> (row, col)
#    matched_cat         CHAIN of CATOBJs which match detected objects
#    matched_det         CHAIN of detected objects which match catalog
#    unmatched_cat       CHAIN of CATOBJs which DON'T match detected objects
#    unmatched_det       CHAIN of detected objects which DON'T match catalog
#
# After creating the above sets of matched objects, this example continues
#    by using one field to create a binary decision tree to classify the
#    objects, then applying that tree to the objects in other fields.
#

if { 0 } {
  set catch [read_catalogs]
  set catmag rmag
  set photo_dir /scr0/richmond/photodata/frames_7/output 
  set run 581 
  set col 1 
  set startfield 20
  set endfield 22
  set reffield 20
  set magfield fiberCounts
  # sort detected objs by these magnitudes
  set magindex 2    
  # use these positions for matching to cat
  set ref_filt 2                      
  # use parameters measured in this passband for classification
  set treeband 2                      

  dTreeSetParams { fiberCounts w_cc w_cr petroRad fracPSF }

  timerStart

  # first, we create a set of trees using "startfield"
  echo "creating a tree for field $startfield"
  match_output $catch $catmag \
                    $photo_dir $run $col $reffield \
                    $magfield $magindex $ref_filt \
                    cat_to_det_trans \
                    matched_cat matched_det unmatched_cat unmatched_det 
  simTypesAssign $matched_cat $matched_det
  count_types $matched_det
  set tree [dTreeCreate $matched_det $treeband]
  dTreeGrow $tree
  set num [dTreePrune $tree]
  echo [timerLap]


  # now, we loop through the fields and test detected objects against
  #  the trees we've created.  We select a small subset of all the 
  #  possible trees, to get a feeling for the variation of accuracy
  #  with size of tree.
  loop field $startfield [expr $endfield+1] {

    if { $field == $reffield } { 
      continue
    }

    echo "comparing tree from ref field $reffield against field $field"
    match_output $catch $catmag \
                    $photo_dir $run $col $field \
                    $magfield $magindex $ref_filt \
                    cat_to_det_trans \
                    matched_cat matched_det unmatched_cat unmatched_det 
    simTypesAssign $matched_cat $matched_det
    count_types $matched_det
    set testch [chainCopy $matched_det]

    loop t [expr $num-10] $num {
      if { $t < 0 } {
        continue
      }
      set ff treeDump.$t
      set tree2 [dTreeReadAsAscii $ff]
      set res [dTreeTest $testch $treeband $tree2]
      # dTreeDel $tree2
    }

    chainDel $testch

  }
  
  echo [timerLap]

  echo "don't forget to delete all 'treeDump.*' files after you are done"
}

