#
# Utilities used to manipulate the variable PSF data structures
#

if {[info commands find_KL_PSF] == ""} {
   source $env(PHOTO_DIR)/etc/estimate_psf.tcl
}

#
# Make/destroy PSF_REGs
#
proc psfRegNew {args} {
   set opts [list \
		 [list [info level 0] "Create a PSF_REG"] \
		 [list {[reg]} STRING 0 reg "The REGION, or 0"] \
		 [list {[rowc]} DOUBLE 0 rowc "The row-centre"] \
		 [list {[colc]} DOUBLE 0 colc "The column-centre"] \
		 [list {[counts]} DOUBLE 0 counts "The number of counts in the object"] \
		 [list {[countsErr]} DOUBLE 0 countsErr "The error in counts for the object"] \
		 [list -field INTEGER 0 field "The object's field number"] \

		 [list -peak_residual INTEGER 0 peak_residual \
		      "The object's largest residual after KL-PSF subtraction"] \
		 [list -peak_residual_rpeak INTEGER 0 peak_residual_rpeak "Row-position of peak_residual"] \
		 [list -peak_residual_cpeak INTEGER 0 peak_residual_cpeak "Col-position of peak_residual"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   set psfReg [genericNew PSF_REG]

   if {$reg == 0} {
      handleSet $psfReg.reg 0x0
   } else {
      handleSetFromHandle $psfReg.reg &$reg
   }
   
   foreach el [list field rowc colc counts countsErr \
		   peak_residual peak_residual_rpeak peak_residual_cpeak] {
      handleSet $psfReg.$el [set $el]
   }

   return $psfReg
}

proc psfRegChainDestroy {ch} {
   loop i 0 [chainSize $ch] {
      psfRegDel [chainElementRemByPos $ch 0]
   }

   chainDel $ch
}

proc psfRegDel {psfReg} {
   if {[exprGet (int)$psfReg.reg] != 0x0} {
      regDel [handleBindFromHandle [handleNew] *$psfReg.reg]
   }
   genericDel $psfReg

   return ""
}

#
# Print the coefficients in a PSF_KERNEL
#
proc print_kernel {kern {print_KL 0} {SM 0}} {
   set kern "($kern)"

   set sizeof_PTR [sizeofPTR]
   set address [handleBind [handleNew] \
		    [expr [exprGet (long)&$kern.sigma<0>]-$sizeof_PTR] PTR]
   set delta_s [handleBindFromHandle [handleNew] *(float*)&$address]
   handleDel $address

   set address \
       [handleBind [handleNew] [expr [exprGet (long)&$kern.a<0><0><0>]-9*$sizeof_PTR] PTR]
   set delta_a [handleBindFromHandle [handleNew] *(ACOEFF*)&$address]
   handleDel $address
   
   if $print_KL {
      if $SM {
	 puts -nonewline [format "set lambda = \{ %.3f" [exprGet $delta_s]]
      } else {
	 echo [format "lambda = %.3f" [exprGet $delta_s]]
      }
   } else {
      echo [format "sigma = %.3f" [exprGet $delta_s]]
   }
   if !$print_KL {
      loop irb 0 [exprGet $kern.nrow_b] {
	 loop icb 0 [exprGet $kern.ncol_b] {
	    puts [format "    %11.5g" [exprGet $delta_a.c<$irb><$icb>]]
	 }
      }
   }
   handleDel $delta_s; handleDel $delta_a
   
   loop is 0 [exprGet $kern.nsigma] {
      if $print_KL {
	 if $SM {
	    puts -nonewline [format " %.3f" [exprGet $kern.sigma<$is>]]
	    if {$is%6 == 0} {
	       echo " \\"
	    }
	 } else {
	    echo [format "lambda = %.3f" [exprGet $kern.sigma<$is>]]
	 }
	 continue
      } 

      echo ""
      echo [format "sigma = %.3f" [exprGet $kern.sigma<$is>]]
      
      puts -nonewline "   "
      loop irb 0 [exprGet $kern.nrow_b] {
	 loop icb 0 [exprGet $kern.ncol_b] {
	    loop ira 0 [exprGet $kern.nrow_a] {
	       loop ica 0 [exprGet $kern.ncol_a] {
		  puts -nonewline [format " %11.5g" \
			      [exprGet $kern.a<$is><$ira><$ica>.c<$irb><$icb>]]
	       }
	       puts -nonewline "\n   "
	    }
	    puts -nonewline "\n   "
	 }
      }
   }

   if {$print_KL && $SM} {
      echo "\}"
   }
}

#
# Make a mosaic of all the regions in a PSF_KERNEL
#
typedef struct {
   PSF_REG ***reg;
} REGION_ARR;

proc mosaic_basis {basis {trim 0}} {
   set basis "($basis)"
   set kern "(*$basis.kern)"

   set identity [get_basis_region $basis -1 0 0]

   set border [expr [exprGet $kern.border] + $trim]
   set nsigma [exprGet $kern.nsigma]
   set nrow_a [exprGet $kern.nrow_a]
   set ncol_a [exprGet $kern.ncol_a]

   set nrow [expr [exprGet $identity.nrow] - 2*$border]
   set ncol [expr [exprGet $identity.ncol] - 2*$border]
   
   set nmcol [expr $nrow_a*$ncol_a]
   set nmrow [expr 1 + $nsigma]

   set gutter 3
   set mosaic [regNew -type FL32 \
		   [expr $nmrow*($nrow + $gutter) - $gutter] \
		   [expr $nmcol*($ncol + $gutter) - $gutter]]
   regClear $mosaic

   loop is -1 [exprGet $kern.nsigma] {
      set ic 0
      loop ira 0 [exprGet $kern.nrow_a] {
	 loop ica 0 [exprGet $kern.ncol_a] {
	    if {$is == -1} {
	       set reg $identity
	    } else {
	       set reg [handleBindFromHandle [handleNew] \
			    *$basis.regs<$is><$ira><$ica>->reg]
	    }

	    set sreg [subRegNew $reg $nrow $ncol $border $border]
	    set smosaic [subRegNew $mosaic $nrow $ncol \
			  [expr ($is + 1)*($nrow+$gutter)] \
			  [expr $ic*($ncol+$gutter)]]
	    regIntCopy $smosaic $sreg
	    incr ic

	    regDel $sreg; regDel $smosaic
	    handleDel $reg

	    if {$is == -1} { break }
	 }
	 if {$is == -1} { break }
      }
   }

   return $mosaic
}

#
# What's the size of a pointer?
#
proc sizeofPTR {} {
   set reg [regNew 2 1]
   set sizeof \
       [expr [exprGet (long)&$reg.rows<1>] - [exprGet (long)&$reg.rows<0>]]
   regDel $reg

   return $sizeof
}

#
# Return a handle to a region in a PSF_BASIS
#
proc get_basis_region {basis is ira ica} {
   if {$is == -1} {
      #
      # Get a handle to the identity region.  This is complicated by the
      # foolish way that I, RHL, made the PSF_BASIS code use a -1 based index.
      #
      assert {$ira == 0 && $ica == 0}

      set sizeof_PTR [sizeofPTR]
      set address [handleBind [handleNew] \
		       [expr [exprGet (long)&$basis.regs<0>]-$sizeof_PTR] PTR]
      set regs0 [handleBindFromHandle [handleNew] *(REGION_ARR*)&$address]
      handleDel $address
      
      set identity [handleBindFromHandle [handleNew] *$regs0.reg<0><0><0>.reg]
      handleDel $regs0

      return $identity
   }

   if {$is < 0 || $is > [exprGet $basis.kern->nsigma]} {
      error "isigma is out of range"
   }      
   if {$ira < 0 || $ira >= [exprGet $basis.kern->nrow_a]} {
      error "ira is out of range"
   }      
   if {$ica < 0 || $ica >= [exprGet $basis.kern->ncol_a]} {
      error "ica is out of range"
   }      
   return [handleBindFromHandle [handleNew] *$basis.regs<$is><$ira><$ica>]
}

#
# Display one of a PSF_BASIS's regions
#
proc mtv_basis {basis is ira ica} {
   handleDel [mtv [get_basis_region $basis $is $ira $ica]]

   return $basis
}

#
# Create a PSF_KERNEL with specified sigmas and coefficient order
#
proc make_kernel {border sigmalist n_a n_b {ncol_b -1}} {
   if {$ncol_b < 0} { set bcol_b $n_b }

   set nsigma [llength $sigmalist]
   set k [psfKernelNew $nsigma $n_a $n_b -ncol_b $ncol_b]
   handleSet $k.border $border
   
   loop i 0 $nsigma {
      handleSet $k.sigma<$i> [lindex $sigmalist $i]
   }
   
   return $k
}

#
# Create a PSF_BASIS with specified sigmas and coefficient order
#
proc make_basis {border sigmalist n_a n_b reg {ncol_b -1}} {
   if {$ncol_b < 0} { set bcol_b $n_b }

   set k [make_kernel $border $sigmalist $n_a $n_b $ncol_b]
   set b [psfBasisNew $k [exprGet $reg.nrow] [exprGet $reg.ncol] -type FL32]
   psfKernelDel $k
   
   return $b
}

###############################################################################
#
# Write a PSF_BASIS to a FITS binary file
#
# You'll need to declare the SCHEMATRANS for PSF_KL_COMP first, e.g.
#  declare_schematrans 5
#
proc write_psfBasis {basis file mode} {
   if {![file exists $file] && $mode == "a"} {
      set hdr [hdrNew]
      hdrInsWithAscii $hdr PSF_ID [photoVersion] \
	  "Version of photo used for PSF calculation"
      hdrInsWithAscii $hdr FILTERS "u g r i z" "Filters present in file"
      set fd [fitsBinTblOpen $file w -hdr $hdr]
      hdrDel $hdr
   } else {
      set fd [fitsBinTblOpen $file $mode]
   }

   set hdr [hdrNew]
   set nrow_b [hdrInsWithInt $hdr "NROW_B" [exprGet $basis.kern->nrow_b] \
		   "num. of row variation coeffs"]
   set ncol_b [hdrInsWithInt $hdr "NCOL_B"  [exprGet $basis.kern->ncol_b] \
		   "num. of col variation coeffs"]
   fitsBinTblHdrWrite $fd "PSF_KL_COMP" -hdr $hdr
   hdrDel $hdr

   set klc [psfKLCompNew]
   loop i 0 [expr [exprGet $basis.kern->nsigma]+1] {
      psfKLCompSetFromBasis $basis $i -klc $klc
      
      fitsBinTblRowWrite $fd $klc
   }
   fitsBinTblClose $fd; unset fd
   
   handleSet $klc.reg 0
   psfKLCompDel $klc
}

proc read_psfBasis {args} {
   global is_values

   set opts [list \
		 [list [info level 0] ""] \
		 [list <psfBasis> STRING "" _psfBasis \
		      "Name of array of PSF_BASIs, indexed by filtername"] \
		 [list <file> STRING "" file "File to read from"] \
		 [list {[filterlist]} STRING "" filterlist "Desired filters"] \
		 [list -filterlist STRING "" _filterlist "Desired filters"] \
		 [list {[sao]} INTEGER 0 display \
		      "Show Basis functions in this saoimage?"] \
		 [list -sao INTEGER 0 display \
		      "Show Basis functions in this saoimage?"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   upvar $_psfBasis psfBasis

   if [info exists psfBasis] {
      free_psfBasis psfBasis
   }

   if {$filterlist == ""} {
      set filterlist $_filterlist
   } else {
      if {$_filterlist != "" && $_filterlist != $filterlist} {
	 error \
	    "Please don't specify \"$filterlist\" and \"-filter $_filterlist\""
      }
   }

   if {$file == ""} {
      error "You must specify a filterlist if you specify an empty filename"
      foreach filter $filterlist {
	 set id_values(PSF_ID) "not used"
	 set psfBasis($filter) ""
      }

      return
   } elseif {$file == "table"} {
      global field openit

      if ![info exists openit] {
	 error "Please use set_run and try again" 
      }
      
      set file $openit(psdir)/psField-$openit(run)-$openit(col)-[format %04d $field].fit
   }

   calc_crc -field $file
   #
   # Open file and seek to required place
   #
   set hdr [hdrNew]
   set fd [fitsBinTblOpen $file "r" -hdr $hdr]

   # get PSF_ID keyword, if present
   if [catch {set id_values(PSF_ID) [hdrGetAsAscii $hdr PSF_ID]}] {
       set id_values(PSF_ID) "not available"
   }  
   set psf_filters [hdrGetAsAscii $hdr FILTERS]
   hdrDel $hdr

   if {$filterlist == ""} {
      set filterlist $psf_filters
   }
   #
   # Loop through file considering which HDUs to read
   #
   foreach filter $psf_filters {
      #
      # Is this HDU desired?
      #
      if {[lsearch $filterlist $filter] < 0} {# Skip this HDU
	 fitsBinTblHdrRead $fd "NULL"
	 continue;
      }
      #
      # Read desired table
      #
      set ncomp [fitsBinTblHdrRead $fd "PSF_KL_COMP"]
      
      loop i 0 $ncomp {
	 set reg [regNew 0 0]
	 set klc [psfKLCompNew -reg $reg]
	 handleDel $reg	 
	 fitsBinTblRowRead $fd $klc

         # display components
	 if {$display} {
             echo "-----------------------------"
             echo " KLC for filter $filter, component $i:"
             print_klc $klc
             display *$klc.reg "  klc.reg"
	 }

	 if ![info exists psfBasis($filter)] {
	    set kern [psfKernelNew [expr $ncomp-1] 1 \
			  [exprGet $klc.nrow_b] -ncol_b [exprGet $klc.ncol_b]]
	    set psfBasis($filter) [psfBasisNew -type "KL" $kern -1 -1]
	    psfKernelDel $kern
	 }

	 psfBasisSetFromKLComp $psfBasis($filter) $i $klc

	 handleSet $klc.reg 0x0
	 psfKLCompDel $klc
      }
      fitsBinTblEnd $fd
   }
   fitsBinTblClose $fd; unset fd
}

#
# Like read_psfBasis but more user friendly (uses openit etc.)
#
proc get_psfBasis {_psfBasis field {filterlist ""} {_filters ""} {throw_error 0}} {
   upvar $_psfBasis psfBasis
   global openit

   set file $openit(psdir)/psField-$openit(run)-$openit(col)-[format %04d $field].fit
   
   if ![file exists $file] {
      set msg "$file doesn't exist"
      if $throw_error {
	 global errorInfo
	 return -code error -errorinfo $errorInfo $msg
      } else {
	 echo $msg
	 return ""
      }
   }

   if {$_filters != ""} {
      upvar $_filters filters
      
      set hdr [hdrNew]
      if [catch { hdrReadAsFits $hdr $file } msg] {
	 hdrDel $hdr
	 echo "Cannot read header from $file"
	 return ""
      } 
      set filters [hdrGetAsAscii $hdr "FILTERS"]
      hdrDel $hdr
   }

   if [catch {read_psfBasis psfBasis $file $filterlist 0}] {
      #
      # Maybe no schematrans is defined?
      #
      if ![info exists filters] {
	 set hdr [hdrNew]
	 if [catch { hdrReadAsFits $hdr $file } msg] {
	    hdrDel $hdr
	    echo "Cannot read header from $file"
	    return ""
	 } 
	 set filters [hdrGetAsAscii $hdr "FILTERS"]
	 hdrDel $hdr
      }

      declare_schematrans [llength $filters]
      #
      # Try again
      read_psfBasis psfBasis $file $filterlist 0
   }
}

proc free_psfBasis {_psfBasis} {
   upvar $_psfBasis psfBasis

   if ![info exists psfBasis] {
      error "No such array: $_psfBasis"
   }

   foreach filter [array names psfBasis] {
      psfBasisDel $psfBasis($filter)
   }
   unset psfBasis
}

proc print_klc {klc} {
   set nrow [exprGet $klc.nrow_b]; set ncol [exprGet $klc.ncol_b]

   puts [format "  nrow, ncol = %d, %d" $nrow $ncol]
   loop i 0 $nrow {
      loop j 0 $ncol {
	 puts -nonewline [format " %8.3g " [exprGet $klc.c<$i><$j>]]
      }
      puts ""
   }
   puts [format " lambda = %.4g" [exprGet $klc.lambda]]
   puts [format " counts = %8.3g" [exprGet $klc.counts]]

}

###############################################################################
#
# Make a region with the PSF at a point, given a basis. Alternatively,
# make a grid of PSF models at various positions in the frame
#
proc reconstruct_psf {args} {
   global TYPE_PIX_NAME
	    
   set npsf_r 0; set npsf_c 0
   set opts [list \
		 [list reconstruct_psf \
		      "Return a REGION containing the PSF at (rowc, colc)
 or a grid of PSFs"] \
		 [list <basis> STRING "" basis "a PSF_BASIS"] \
		 [list {[rowc]} DOUBLE -1 rowc "row-position of desired PSF"] \
		 [list {[colc]} DOUBLE -1 colc "col-position of desired PSF"] \
		 [list {-nrow} INTEGER 0 npsf_r \
		      "Number of rows of PSFs to generate"] \
		 [list {-ncol} INTEGER 0 npsf_c \
		      "Number of columns of PSFs to generate"] \
		 [list {-trim} INTEGER 0 trim \
		 "Number of rows/cols to trim from each side of PSF regions"] \
		 [list {-gutter} INTEGER 5 gutter \
		      "Gutter between PSFs in mosaic"] \
		 [list -model INTEGER -1 model_band \
		    "Return PSFs as used by model fitting code in this band"] \
		 [list -type STRING "psf" model_type \
	     "Return a model of this type, not just a PSF. E.g. {deV re ab}"] \
		 [list -sigma DOUBLE 0.0 sigma \
		      "Smoothing sigma for centroiding (used with -model)"] \
		 ]
   if {[shTclParseArg $args $opts reconstruct_psf] == 0} {
      return 
   }

   if {$model_band >= 0} {
      set objc [objcNew [expr $model_band + 1]]
      set obj1 [object1New]

      if {$model_type == "psf"} {
	 set type "psf"
	 handleSet $obj1.psfCounts 1e5
      } else {				# should be e.g. "deV re ab"
	 if {[llength $model_type] != 3} {
	    error "Please specify \"type re ab\" not just \"$model_type\""
	 }
	 set type [lindex $model_type 0]

	 handleSet $obj1.counts_$type 1e5
	 
	 handleSet $obj1.r_$type [lindex $model_type 1]
	 handleSet $obj1.ab_$type [lindex $model_type 2]
	 handleSet $obj1.phi_$type 180
      }
      handleSetFromHandle $objc.color<$model_band> &$obj1
      handleDel $obj1
   }

   if {$npsf_r <= 0} {
      if {$model_band >= 0} {
	 handleSet $objc.color<$model_band>->rowc $rowc
	 handleSet $objc.color<$model_band>->colc $colc
	 
	 set reg [fittedModelMake -$type -nocorrect $basis \
		      $objc $model_band -sigma $sigma]
	 
	 objcDel $objc
      } else {
	 set psfReg \
	     [psfKLReconstruct $basis $rowc $colc -regType $TYPE_PIX_NAME]
	 set reg [handleBindFromHandle [handleNew] *$psfReg.reg]
	 handleSet $psfReg.reg 0x0
	 psfRegDel $psfReg

	 set dr [expr $rowc + 0.5 - int($rowc + 0.5)]
	 set dc [expr $colc + 0.5 - int($colc + 0.5)]
	 
	 if {$dr != 0 || $dc != 0} {
	    set sreg [regIntShift $reg $dr $dc]
	    regDel $reg; set reg $sreg
	 }
      }

      return $reg
   }

   if {$npsf_c == 0} { set npsf_c $npsf_r }
   if {$rowc >= 0 || $colc >= 0} {
      error "You may not specify (rowc, colc) and -nrow/ncol"
   }
   
   set ident [get_basis_region $basis -1 0 0]
   set size [exprGet $ident.nrow]
   handleDel $ident
   
   if {$trim > 0} {
      if {$size - 2*$trim < 0} {
	 echo "-trim is too large; ignoring"
	 set trim 0
      } else {
	 incr size [expr -2*$trim]
      }
   }

   set mosaic [regNew \
		   [expr $npsf_r*$size + ($npsf_r-1)*$gutter] \
		   [expr $npsf_c*$size + ($npsf_c-1)*$gutter]]
   regClear $mosaic

   set nrow_f 1489; set ncol_f 2048
   if {$npsf_r == 1} {
      set row0 [expr 0.5*$nrow_f]
      set drow 1
   } else {
      set row0 0
      set drow [expr $nrow_f/($npsf_r - 1.0)]
   }
   if {$npsf_c == 1} {
      set col0 [expr 0.5*$ncol_f]
      set dcol 1
   } else {
      set col0 0
      set dcol [expr $ncol_f/($npsf_c - 1.0)]
   }

   loop i 0 $npsf_r {
      loop j 0 $npsf_c {
	 set rowc [expr int($row0 + $i*$drow + 0.5) + 0.5]
	 set colc [expr int($col0 + $j*$dcol + 0.5) + 0.5]

	 if {$model_band >= 0} {
	    handleSet $objc.color<$model_band>->rowc $rowc
	    handleSet $objc.color<$model_band>->colc $colc
	    
	    set reg [fittedModelMake -$type -nocorrect $basis \
			 $objc $model_band -sigma $sigma]
	 } else {
	    set psfReg \
		[psfKLReconstruct $basis $rowc $colc -regType $TYPE_PIX_NAME]
	    set reg [handleBindFromHandle [handleNew] *$psfReg.reg]
	    handleSet $psfReg.reg 0x0
	    psfRegDel $psfReg
	 }
	 
	 set sreg [subRegNew $mosaic $size $size \
		       [expr $i*$size + ($i==0?0:$i)*$gutter] \
		       [expr $j*$size + ($j==0?0:$j)*$gutter]]
	 if {$trim > 0} {
	    set treg [subRegNew $reg $size $size $trim $trim]
	    regIntCopy $sreg $treg
	    regDel $treg
	 } else {
	    regIntCopy $sreg $reg;
	 }
	 regDel $sreg; regDel $reg
      }
   }

   if {$model_band >= 0} {
      objcDel $objc
   }

   return $mosaic
}

###############################################################################
#
# Generate a grid of PSF estimates, returning a REGION
#
proc psf_grid {basis npsf_r npsf_c {trim 0}} {
   reconstruct_psf $basis -nrow $npsf_r -ncol $npsf_c -trim $trim
}

########################################################################
# new code for PSP



# given a chain of STAR1s, starlist, return PSF_BASIS 
# for given ncomp, nrow_b and ncol_b 
# - for determining the eigen images use stars with indices
#   n1 < i < n2
# - for determining the spatial dependence of the expansion
#   coefficients use only stars from frames with f1 < j < f2 
#   master row for these stars also has to satisfy 
#   mrow_min < master_raw < mrow_max
# - also, include only stars with NSR < maxNSR
# - measure star's row position relative to frame0 
# - add border pixels around syncreg for each star
# - for scale != 0 scale stars to all have the same amplitude
# - for display != 0 display mosaic
proc starlist2KLpsf {starlist n1 n2 f1 f2 psfKLbasis_Nmax this_frame scale \
                     mrow_min mrow_max border maxNSR ncomp nrow_b ncol_b \
		     rowsperframe colsperframe scan_overlap filter apCor_min \
                     apCor_max def_psField cframe ifilter frame_info \
			 {display_KLpsf 0} {display_KLresiduals 0}} {

   global display verbose
   global nrowperframe ncolperframe
   global PSP_STATUS
   global camCol outputDir run

   set debug 0;				# n.b. >= 9 will call error for you
   set display 0
   set problemField 9999

   # NB these are given both as globals and as inputs, they better be same
   assert { $nrowperframe == $rowsperframe }
   assert { $ncolperframe == $colsperframe }

   verb_echo 4 "starlist2KLpsf: WORKING ON FIELD $this_frame"
   if {$debug && [lsearch "$problemField" $this_frame] >= 0} {
       set debug 3
       uplevel \#0 { if !$display { set_display 1 }}
   }   
  
   if {$display > 0} {catch {saoReset}}

   set rowsperFramesframe [expr $rowsperframe + $scan_overlap] 
   set SOFT_BIAS [softBiasGet]

   # if there are no stars on this frame set default PSF
   if {$f1 > $f2} {
       verb_echo 2 "starlist2KLpsf: for field $this_frame - filter $filter there are NO GOOD STARS"
       return  [set_default_PSF $filter $ifilter $def_psField $cframe]
   }

   # sanity checks
   set nstars [chainSize $starlist]
   assert { $n1 >= 0 && $n1 < $nstars && $n1 <= $n2 }
   assert { $n2 >= 0 && $n2 < $nstars}
   assert { $f1 <= $f2 }
   assert { $n1 <= $f1 } 
   assert { $f2 <= $n2 } 
 
   # minimum and maximum values of apCorr computed with KL PSF on this frame
   handleSet $frame_info.apCorr_min  -1
   handleSet $frame_info.apCorr_max 999
   #
   # We accept coefficient stars in the range mrow_min -- mrow_max, but
   # if we fail we'll be a little less picky and try a window that's a
   # 2*psfKLcoeffs_nextra rows wider
   #
   set extra_rows [expr [getsoftpar psfKLcoeffs_nextra]*$rowsperframe]
   set mrow_min_e [expr $mrow_min - $extra_rows]
   set mrow_max_e [expr $mrow_max + $extra_rows]

   # populate chains of PSF_REG
   set basis_regs [chainNew PSF_REG];	# KL basis stars
   set coeffs_regs_init [chainNew PSF_REG];	# KL coefficient stars
   set coeffs_regs_extra [chainNew PSF_REG];# backup KL coefficient stars
   set coeffs_regs $coeffs_regs_init
   set coeffs_stars [chainNew STAR1]
   if {$display_KLpsf > 0} {
      set basis_stars [chainNew STAR1]
   }

   set Nbad 0
   set Nempty 0
   set Nfaint 0
   loop id $n1 [expr $n2 + 1] {
      set star [chainElementGetByPos $starlist $id]
      set good_star($id) 0
      handleSet $star.used4coeffs 0
      if {$debug > 5} {
         echo "processing star $id from frame [exprGet $star.frame]"
      }
      # if it's bad do not include it
      if {[exprGet $star.badstar] != 0} {
         if {$debug > 5} {
            echo "   this ($id) is a bad star"
         }
         incr Nbad
         continue
      }

      # make PSF_REG for this star
      if {[set psfReg [star2psfReg $star $this_frame $border $maxNSR \
                                   $rowsperframe]] == ""} { 
         if {$debug > 5} {
            echo "   empty psfReg ($id)"
	 }
         incr Nempty
	 continue
      }
      if {[exprGet $psfReg.countsErr] > [expr $maxNSR*[exprGet $psfReg.counts]]} {
         if {$debug > 5 && $display_KLpsf > 0} { 
             display *$psfReg.reg "   too faint ($id): counts = [exprGet $psfReg.counts])"
         }
	 psfRegDel $psfReg
         incr Nfaint
	 continue;
      }

      if {$scale != 0} {
	 # fix all stars to have the same number of counts
	 set fac [expr 1.4e5/[exprGet $psfReg.counts]]
	 handleSet $psfReg.counts [expr [exprGet $psfReg.counts]*$fac]
	 regIntLincom *$psfReg.reg "" [expr 1000*(1 - $fac)] $fac 0
      }
      if {$debug > 7 && $display_KLpsf > 0} { 
         display *$psfReg.reg "   psfReg.reg for star $id (counts = [exprGet $psfReg.counts])"
      }
      
      #
      # add to chains, certainly basis stars and maybe coeff stars
      #
      set good_star($id) 1;		# good enough to be used for basis
      handleSet $star.badstar 0

      ### psfRegs
      # regWriteAsFits *$psfReg.reg psfReg[pid]-${this_frame}-$id.fit

      chainElementAddByPos $basis_regs $psfReg TAIL
      if {$debug > 2 && $display_KLpsf > 0} {      
	  chainElementAddByPos $basis_stars $star TAIL
      }      

      # stars for coeffs
      if {$id >= $f1 && $id <= $f2} {
          set master_row [exprGet $psfReg.rowc]

          if {$master_row >= $mrow_min && $master_row <= $mrow_max} {
              handleSet $star.used4coeffs 1
	      chainElementAddByPos $coeffs_regs $psfReg TAIL
	      chainElementAddByPos $coeffs_regs_extra $psfReg TAIL
              if {$debug > 7 && $display_KLpsf > 0} { 
                  display *$psfReg.reg "   coeff star $id (counts = [exprGet $psfReg.counts] pos= [format %.0f,%.0f [exprGet $psfReg.rowc] [exprGet $psfReg.colc]])"
              }
              chainElementAddByPos $coeffs_stars $star TAIL
              if {$debug > 8} {
                 echo " COEFF STAR [chainSize $coeffs_stars]: [exprGet $star]"
                 error "set star $star"
              }
	  } elseif {$master_row >= $mrow_min_e && $master_row <= $mrow_max_e} {
	     lappend extra_star_ids $id;# we may need to set used4coeffs
	     
	     chainElementAddByPos $coeffs_regs_extra $psfReg TAIL
              if {$debug > 7 && $display_KLpsf > 0} { 
                  display *$psfReg.reg "   Extra coeff star $id (counts = [exprGet $psfReg.counts] pos= [format %.0f,%.0f [exprGet $psfReg.rowc] [exprGet $psfReg.colc]])"
              }
          } else {
             if {$debug > 5} {
                 echo "   good frame but master row is bad: doesn't qualify for coeffs"
	     }
          }
      } else {
         if {$debug > 5} {
            echo "   not on a proper frame: doesn't qualify for coeffs"
	 }
      }
      handleDel $psfReg
   }
   set Ngood [chainSize $basis_regs]

   if {$debug} {
      echo "starlist chain has $nstars elements and [expr $n2-$n1+1] candidates"
      echo "         there are $Ngood stars with good PSF_REGs"
      if {$debug > 2 && $display_KLpsf > 0} { 
          echo "there are [chainSize $basis_stars] on basis chain"
          echo "there are [chainSize $coeffs_stars] on coeff chain"
      }
      echo "$Nbad stars had bad flags"
      echo "$Nfaint stars too faint"
      echo "$Nempty stars with empty psfReg"
        
   } 

   # if $basis_regs includes too many stars cut it to psfKLbasis_Nmax brightest ones
   if {$Ngood > $psfKLbasis_Nmax} {
       verb_echo 7 "basis_regs has $Ngood elements > $psfKLbasis_Nmax"
       # first sort the chain in brightness
       chainFSort -reverse $basis_regs counts
       # and take the first psfKLbasis_Nmax elements
       set bright_list [chainExtract $basis_regs 0 $psfKLbasis_Nmax]
       set faint_length [expr $Ngood - $psfKLbasis_Nmax]
       set faint_list [chainExtract $basis_regs $psfKLbasis_Nmax $faint_length] 
       verb_echo 7 "bright_list has [chainSize $bright_list] elements"  
       verb_echo 7 " faint_list has [chainSize $faint_list] elements"  
       chainDel $basis_regs
       set basis_regs $bright_list
   }

   if {$display_KLpsf > 0} {
       # make mosaics of basis and coeff stars
      if {[chainSize $basis_stars] > 0} {
	 set mosaic [star1_mosaic $basis_stars 65]
	 display $mosaic "stars used for KL basis determination ($this_frame:$filter)"
	 regDel $mosaic
      }
       chainDel $basis_stars

       set mosaic [star1_mosaic $coeffs_stars 65]
       display $mosaic "stars used for KL coefficient determination ($this_frame:$filter)"
       regDel $mosaic
   }

   # upvar 1 mem mem; write_mem mem "ready for KL"


   if {$debug && [lsearch "$problemField" $this_frame] >= 0} {
       dump_psfRegChain $basis_regs Field$problemField-basis_v5_4.dat \
                                    Field$problemField-basis_v5_4
       dump_psfRegChain $coeffs_regs_init Field$problemField-coeffs_v5_4.dat \
                                          Field$problemField-coeffs_v5_4
       echo psfKLDecomp -border $border \$basis_regs $ncomp $nrow_b -ncol_b $ncol_b
       echo " *** dumped psfReg chains **"
   }   

   ### do KL stuff
   set string "  starlist2KLpsf:  for field $this_frame - filter $filter failed"
   # first get KL basis function
   if {[chainSize $basis_regs] >= $ncomp} {
       verb_echo 4 "starlist2KLpsf: calling psfKLDecomp with [chainSize $basis_regs] regs"
       set psfBasis \
	   [psfKLDecomp -border $border $basis_regs $ncomp \
		$nrow_b -ncol_b $ncol_b]
   } else {
       verb_echo 0 "starlist2KLpsf: for field $this_frame - filter $filter only [chainSize $basis_regs] good PSF_REGS regs (need >= $ncomp)"
       verb_echo 1 "setting default PSF"
       chainDel $coeffs_stars
       chainDel $coeffs_regs_init; chainDel $coeffs_regs_extra
       psfRegChainDestroy $basis_regs
       if {[info exists faint_list] && [chainSize $faint_list] > 0} {
           psfRegChainDestroy $faint_list
       }
       return [set_default_PSF $filter $ifilter \
		   $def_psField $cframe $frame_info]
   }

   # and then get expansion coefficients
   if {$nrow_b*$ncol_b == 1} {
      set Nstar_min 1
   } else {
      set Nstar_min [expr $nrow_b + $ncol_b]
   }
   if {[chainSize $coeffs_regs] >= $Nstar_min} {
       verb_echo 5 "starlist2KLpsf: calling psfKernelSet with [chainSize $coeffs_regs] regs"
       set coeffs_regs $coeffs_regs_init
       psfKernelSet $coeffs_regs $psfBasis *$psfBasis.kern
      
       if {$debug > 0 && $display_KLpsf > 0} {
           set grid [psf_grid $psfBasis 12 18]
           display $grid "reconstructed PSFs for full KL fit ($filter)"
           regDel $grid
       }

       set force_KL_order 0;	# e.g. 33 to force no worse than 3x3 fit
       if {$force_KL_order > 0} {
	  echo "XXX bypassing KL fall back -- " \
	      "enforced no worse than $force_KL_order"
       }

       # upvar 1 mem mem; write_mem mem "done coeff 1"

       #
       # Check expansion
       #
       # acFmin and acFMax: minimum and maximum values of apCorr
       # computed with KL PSF on this frame
       #
       set status $PSP_STATUS(OK);	# as far as we know so far
      
       if {$debug && [lsearch "$problemField" $this_frame] >= 0} {  
          echo check_psfBasis $psfBasis $rowsperFramesframe $apCor_min $apCor_max acFMin acFMax
          error 
       }  

       if {[check_psfBasis $psfBasis $rowsperFramesframe $apCor_min $apCor_max\
		acFMin acFMax] <= 0 && $force_KL_order < 44} {
          # upvar 1 mem mem; write_mem mem "done check_psfBasis"   
	  #
	  # Expansion isn't good enough.
	  #
	  if {[chainSize $coeffs_regs_extra] > [chainSize $coeffs_regs]} {
	     psfBasisDel $psfBasis
	     verb_echo 2 "$string in check_psfBasis, trying more coeff stars"
	     
	     set psfBasis \
		 [psfKLDecomp -border $border $basis_regs $ncomp 2 -ncol_b 2]
	     set coeffs_regs $coeffs_regs_extra
	     psfKernelSet $coeffs_regs $psfBasis *$psfBasis.kern
             echo "CALLED psfKLDecomp -border $border \$basis_regs $ncomp 2 -ncol_b 2"
             if {$debug && [lsearch "$problemField" $this_frame] >= 0} {
                 dump_psfRegChain $coeffs_regs Field$problemField-coeffsExtra_v5_4.dat \
                                               Field$problemField-coeffsExtra_v5_4
                 echo psfKLDecomp -border $border \$basis_regs $ncomp $nrow_b -ncol_b $ncol_b
                 echo " *** dumped psfReg chains **"
             }       
             # upvar 1 mem mem; write_mem mem "done coeff 2"
  
	     if {$debug > 0 && $display_KLpsf > 0} {
		set grid [psf_grid $psfBasis 6 9]
		display $grid [concat "reconstructed PSFs for " \
				   "extended window parabolic PSF ($filter)"]
		regDel $grid
	     }
	  }
	   #
	   # Check expansion
	   #
           if {$force_KL_order < 33 && \
		   [check_psfBasis $psfBasis $rowsperFramesframe \
			$apCor_min $apCor_max acFMin acFMax] <= 0} {
	      #
	      # Expansion still isn't good enough.
	      #
	      psfBasisDel $psfBasis
	      verb_echo 2 "$string in check_psfBasis, trying linear PSF"

	      set psfBasis \
		  [psfKLDecomp -border $border $basis_regs $ncomp 2 -ncol_b 2]
	      set coeffs_regs $coeffs_regs_init
	      psfKernelSet $coeffs_regs $psfBasis *$psfBasis.kern
	      set status $PSP_STATUS(PSF22)

              # upvar 1 mem mem; write_mem mem "done coeff 3"

	      if {$debug > 0 && $display_KLpsf > 0} {
		 set grid [psf_grid $psfBasis 6 9]
		 display $grid "reconstructed PSFs for parabolic PSF ($filter)"
		 regDel $grid
	      }
	      #
	      # Check expansion
	      #
	      if {$force_KL_order < 22 && \
		      [check_psfBasis $psfBasis $rowsperFramesframe \
			   $apCor_min $apCor_max acFMin acFMax] <= 0} {
		 #
		 # Expansion _still_ isn't good enough.
		 #
		 psfBasisDel $psfBasis
		 verb_echo 2 "$string with linear PSF, enforcing constant PSF"

		 set status $PSP_STATUS(PSF11)
		 
		 set psfBasis \
		     [psfKLDecomp -bord $border $basis_regs $ncomp 1 -ncol_b 1]
		 set coeffs_regs $coeffs_regs_init
		 psfKernelSet $coeffs_regs $psfBasis *$psfBasis.kern

                 # upvar 1 mem mem; write_mem mem "done coeff 4"

		 if {$debug > 0 && $display_KLpsf > 0} {
		    set grid [psf_grid $psfBasis 6 9]
		    display $grid \
			"reconstructed PSFs for constant PSF ($filter)"
		    regDel $grid
		 }
		 
		 if {$force_KL_order < 11 && \
			 [check_psfBasis $psfBasis $rowsperFramesframe \
			      $apCor_min $apCor_max acFMin acFMax] <= 0} {
		    verb_echo 2 "$string with constant PSF, setting default PSF"
		    chainDel $coeffs_stars
		    chainDel $coeffs_regs_init; chainDel $coeffs_regs_extra
		    psfRegChainDestroy $basis_regs
		    psfBasisDel $psfBasis
		    if {[info exists faint_list] && \
			    [chainSize $faint_list] > 0} {
		       psfRegChainDestroy $faint_list
		    }
		    
		    return [set_default_PSF $filter $ifilter \
				$def_psField $cframe $frame_info]
		 }
              }
           }
       }
   } else {
       # not enough stars in coefficients chain, try with all stars
       verb_echo 2 "  starlist2KLpsf: for field $this_frame - filter $filter only [chainSize $coeffs_regs] "
       verb_echo 2 "                  good PSF_REGS regs for expanding coefficients,"
       verb_echo 2 "                  need at least $Nstar_min: trying with constant PSF"
       psfBasisDel $psfBasis
       set status $PSP_STATUS(PSF11)
       set psfBasis [psfKLDecomp -border $border $basis_regs $ncomp 1 -ncol_b 1]
       set coeffs_regs $coeffs_regs_init
       psfKernelSet $coeffs_regs $psfBasis *$psfBasis.kern

       # upvar 1 mem mem; write_mem mem "done coeff all"

       if {[check_psfBasis $psfBasis $rowsperFramesframe \
		$apCor_min $apCor_max acFMin acFMax] <= 0} {
	  verb_echo 2 "$string with constant PSF, setting default PSF"

	  chainDel $coeffs_stars
	  chainDel $coeffs_regs_init; chainDel $coeffs_regs_extra
	  psfRegChainDestroy $basis_regs
	  psfBasisDel $psfBasis
	  if {[info exists faint_list] && [chainSize $faint_list] > 0} {
	     psfRegChainDestroy $faint_list
	  }

	  return [set_default_PSF \
		       $filter $ifilter $def_psField $cframe $frame_info]
       }
   }
   #
   # We have our psfBasis.
   #
   # If we used extra stars for coeffs, set their used4coeffs
   #
   if {$coeffs_regs == $coeffs_regs_extra && [info exists extra_star_ids]} {
      set status [expr $status | $PSP_STATUS(EXTENDED_KL)]

      foreach id $extra_star_ids {
	 set star [chainElementGetByPos $starlist $id]
	 handleSet $star.used4coeffs 1
      }
   }

   #
   # test that the field is uniformly populated by coeff stars
   # 
   # should $Nx, $Ny, $Nempty be input? XXX 
   set Nx 3; set dx [expr $colsperframe/$Nx] 
   set Ny 2; set dy [expr $rowsperframe/$Ny]
   set Nempty 2
   # upvar 1 mem mem; write_mem mem "test_field_coverage"
   set status [expr $status | [test_field_coverage $coeffs_stars $this_frame $dx $Nx $dy $Ny $Nempty]]

   handleSet (int)$cframe.calib<$ifilter>->status $status
   handleSet $frame_info.apCorr_min $acFMin
   handleSet $frame_info.apCorr_max $acFMax
   
   set Ngood [chainSize $coeffs_regs]
   handleSet $frame_info.NgoodKLbasis  [chainSize $basis_regs]
   handleSet $frame_info.NgoodKLcoeffs $Ngood
   #
   # Clean up
   #
   chainDel $coeffs_stars
   chainDel $coeffs_regs_init; chainDel $coeffs_regs_extra
   psfRegChainDestroy $basis_regs
   if {[info exists faint_list] && [chainSize $faint_list] > 0} {
      psfRegChainDestroy $faint_list
   }

   if {$display && ($display_KLresiduals || $debug > 0)} {
      set PSFmosaic \
	  [make_KLResiduals_mosaic $starlist $f1 $f2 good_star $Ngood \
	       $psfBasis $this_frame $border $maxNSR $rowsperframe \
	       $debug KL_star_qa]

      if $display_KLresiduals {
	 display $PSFmosaic "KL residuals for $this_frame $filter "
	 if {$display_KLresiduals > 1} {
	    set file [format "psKlResiduals-%06d-${filter}$camCol-%04d.fit" \
			  $run $this_frame]
	    regWriteAsFits $PSFmosaic $outputDir/$file
	 }
      }
      
      if {$debug > 0} {
	 KL_debug_info
      }
      
      regDel $PSFmosaic
   }

   # upvar 1 mem mem; write_mem mem "done starlist2KLpsf"
   if {$debug && [lsearch "$problemField" $this_frame] >= 0} {
      error
   }   

   return $psfBasis
}



# given a chain of STAR1s, find the number of stars from the given frame 
# in each rectangle defined by x=[i*dx, (i+1)*dx] and y=[j*dy, (j+1)*dy], 
# with i=0..Nx-1, j=0..Ny-1. If there are more than Nempty rectangles 
# return PSP_STATUS(SPARSE), 0 otherwise
proc test_field_coverage {stars frame dx Nx dy Ny Nempty} {

   global PSP_STATUS

    set empty 0
    set thisFrame [chainSearch $stars "{frame == $frame}"]

    loop i 0 $Nx {
	set xmin [expr $i*$dx]
	set xmax [expr ($i+1)*$dx]
        set xChain [chainSearch $thisFrame "{object->colc >= $xmin} {object->colc < $xmax}"]
	loop j 0 $Ny {
	   set ymin [expr $j*$dy]
	   set ymax [expr ($j+1)*$dy]
           set yChain [chainSearch $xChain "{object->rowc >= $ymin} {object->rowc < $ymax}"]
	   if {[chainSize $yChain] == 0} {incr empty}
           chainDel $yChain
	}
        chainDel $xChain
    }

    chainDel $thisFrame

    if {$empty >= $Nempty} {return $PSP_STATUS(SPARSE)}

   return 0

}




###############################################################################
#
# the following is for debugging only.
#
proc KL_debug_info {} {
   uplevel {
      if {0 && $display_KLpsf > 0} {
	 set grid [psf_grid $psfBasis 3 4]
	 display $grid "grid of reconstructed PSFs"
	 regDel $grid
      }
      
      if {$debug > 10 && $display_KLpsf > 0} {
	 catch {
	    set mosaic [mosaic_basis $psfBasis]
	    display $mosaic "mosaic of eigen images"
	    regDel $mosaic
	 }
      }
      
      if {$debug > 10} {   
	 catch {
	    print_kernel *$psfBasis.kern 1 0
	 }
      }
      
      if {$display_KLpsf > 0} { 
	 echo "starid frame width rowc   colc    peakD   peakRes   peakRat(%) fluxRat(%) fErr(%)  chi2" 
	 echo "-----------------------------------------------------------------------------"
	 
	 foreach el $KL_star_qa {
	    echo $el
	 }

	 if !$display_KLresiduals {	# we haven't already seen this mosaic
	    display $PSFmosaic "comparison with reconstructed PSFs ($filter)"
	 }
	 if {1} {
	    echo "XXX dumping reconstructed stars for frame ${this_frame}"
	    regWriteAsFits $PSFmosaic new_recon[pid]-${this_frame}.fit
	    #gets stdin a
	 }
      }
   }
}

# calculate PSF at position of each star, and make a mosaic
# of reconstructed PSFs and original stellar profiles

proc make_KLResiduals_mosaic {starlist f1 f2 _good_star Ngood psfBasis \
				  this_frame border maxNSR rowsperframe \
				  debug {_KL_star_qa ""}} {
   global display
   upvar $_good_star good_star
   if {$_KL_star_qa != ""} {
      upvar $_KL_star_qa KL_star_qa;	# QA information
   }

   set SOFT_BIAS [softBiasGet]
   set npanel 3
   # each panel counts as a unit area  
   set Nrow [expr int(sqrt($npanel*$Ngood))]
   if {$Nrow <= 0} { set Nrow 1 }
   # but npanels must be adjacent
   while {$Nrow/$npanel*$npanel != $Nrow} { incr Nrow }
   # how many columns?     
   set Ncol [expr int($npanel*$Ngood/$Nrow)]
   if {$Ncol <= 0} { set Ncol 1 }
   # do we have enough?
   while {$Ncol*$Nrow < $npanel*$Ngood} { incr Ncol }   
   set i 0; set j 0;	# used for making mosaic
   # loop over all stars
   loop id2 $f1 [expr $f2 + 1] {
      if {!$good_star($id2)} {continue}
      
      set star [chainElementGetByPos $starlist $id2]
      set flag [expr [exprGet $star.flags]]
      set Sframe [exprGet $star.frame]
      set Swidth [exprGet $star.Eff_width]
      if {![exprGet $star.used4coeffs]} {continue}
      
      if {[set psfReg2 [star2psfReg $star $this_frame $border $maxNSR \
			    $rowsperframe]] == ""} {
	 continue
      }
      set reg2 [handleBindFromHandle [handleNew] *$psfReg2.reg]
      set rowc [exprGet $psfReg2.rowc]; set colc [exprGet $psfReg2.colc]
      
      # do we know the reg's size?
      if ![info exists nrow] {
	 set nRegrow [exprGet $psfReg2.reg->nrow]
	 set nRegcol [exprGet $psfReg2.reg->ncol]
	 # account for the dummy border
	 set b_fac [expr int(0.9*$border)]
	 set nrow [expr int($nRegrow - 2*$b_fac)]
	 set ncol [expr int($nRegcol - 2*$b_fac)]
      }
      
      # do we need to set up mosaic?
      if ![info exists PSFmosaic] {
	 set PSFmosaic [regNew -type FL32 [expr $nrow*$Nrow] [expr int(1.1*$ncol*$Ncol)]]
	 regClear $PSFmosaic
      } 
      # estimate PSF at reg2's position
      set treg2 [psfKLReconstruct $psfBasis $rowc $colc]
      if {$debug > 4 && $display > 0} {
	 display *$treg2.reg "star $id2 reconstructed at ($rowc, $colc)"
      }
      
      ### make mosaic
      
      # first measure peak and flux needed for scaling below
      set auxreg [subRegNew $reg2 $nrow $ncol $b_fac $b_fac] 
      set datareg [regNew $nrow $ncol]
      regIntCopy $datareg $auxreg
      regDel $auxreg
      regIntConstAdd $datareg -$SOFT_BIAS
      set datastats [regStatsFind $datareg]
      set datapeak [keylget datastats high]
      set dataflux [keylget datastats total]
      # psf counts obtained by KL code
      set datacounts [exprGet $psfReg2.counts]
      set datacountsErr [exprGet $psfReg2.countsErr]
      regDel $datareg
      
      # extract central portion of data region (with only a bit of border)
      set datareg [subRegNew $reg2 $nrow $ncol $b_fac $b_fac] 
      # extract central portion of the reconstructed PSF
      set modelreg [subRegNew *$treg2.reg $nrow $ncol $b_fac $b_fac] 
      
      # find scale
      set modelC [exprGet $treg2.counts]
      set dataC  [exprGet $psfReg2.counts]
      if {$modelC != 0} {
	 set scale [expr 1.0*$dataC/$modelC]
      } else {
	 error "starlist2KLpsf: ??? PSF model counts = $modelC"
      }
      
      ## make 3 panels in mosaic region
      # data panel
      set sreg [subRegNew $PSFmosaic $nrow $ncol $j [expr int(1.1*$i*$ncol)]]
      regIntCopy $sreg $datareg
      regIntLincom $sreg "" $SOFT_BIAS 1 0; regDel $sreg
      # model panel
      set sreg [subRegNew $PSFmosaic $nrow $ncol \
		    [expr $j + int(0.95*$nrow)] [expr int(1.1*$i*$ncol)]]
      regIntCopy $sreg $modelreg 
      regIntLincom $sreg "" $SOFT_BIAS $scale 0; regDel $sreg
      # residual (data-Scale*model)
      set sreg [subRegNew $PSFmosaic \
		    $nrow $ncol [expr $j + int(1.9*$nrow)] [expr int(1.1*$i*$ncol)]]
      # residual (both data and model do NOT have soft bias included)
      # copy data
      regIntCopy $sreg $datareg
      # subtract scaled model 
      regIntLincom $sreg $modelreg 0 1 [expr -$scale] 
      # measure peak and flux 
      set resstats [regStatsFind $sreg]
      set respeak  [keylget resstats high]
      set reshole  [keylget resstats low]
      if {[expr abs($reshole)] > $respeak} {
	 set respeak $reshole
      }
      set resflux  [keylget resstats total]
      regIntLincom $sreg "" $SOFT_BIAS 1 0; regDel $sreg
      
      # some qa info
      set peak_rat [format "%6.2f" [expr 100.0*$respeak/$datapeak]]
      set flux_rat [format "%6.3f" [expr 100.0*$resflux/$dataflux]]
      set fErr     [format "%6.2f" [expr 100.0*$datacountsErr/$datacounts]]
      set chi2 [format "%6.2f" [expr $flux_rat/$fErr]]
      set msg [format "%5d %5d %6.2f %6.0f %6.0f %8.0f  %6.0f" \
		    $id2 $Sframe $Swidth $rowc $colc $datapeak $respeak]
      append msg "      $peak_rat     $flux_rat   $fErr  $chi2 $datacounts"
      lappend KL_star_qa $msg
      
      if {[incr i] == $Ncol} {
	 incr j [expr $npanel*$nrow]
	 set i 0
      }
      
      regDel $datareg; regDel $modelreg  
      handleDel $reg2; psfRegDel $treg2
      psfRegDel $psfReg2
   }

   return $PSFmosaic
}

# given a STAR1, construct and return PSF_REG
# measure row position relative to frame0
# use only stars with NSR < maxNSR
# add border pixels around syncreg for each star
proc star2psfReg {star1 frame0 border maxNSR rowsperframe} {
  
   if {[set vals [eval prepare_star1 $star1 $frame0 $border \
                       $maxNSR $rowsperframe]] == ""} {
      return ""
   }

   set psfReg [eval psfRegNew $vals]
   handleDel [lindex $vals 0];  # the region
   
   return $psfReg

}



# given a STAR1, center its sync region and pad out the edges for 
# the convolution filters with delta pixels
# use only stars with NSR < maxNSR
proc prepare_star1 {star1 this_frame delta maxNSR rowsperframe} {

   global verbose

   set refine_center 0

   # coordinates for this star
   set frame [exprGet $star1.frame]
   set rowc [exprGet $star1.object->rowc]
   set rowc [format "%8.1f" [expr $rowc + ($frame-$this_frame)*$rowsperframe]]
   set colc [exprGet $star1.object->colc]
   # noise-to-signal ratio for this star
   set counts [exprGet $star1.apCounts]
   set countsErr [exprGet $star1.apCountsErr]
   if {$counts > 0} {
       set NSR [expr $countsErr/$counts]
   } else {
       set NSR 1.0
   }
   if {$NSR > $maxNSR} {
       verb_echo 8 "this object's ($rowc, $colc) NSR is $NSR (c=$counts, cErr=$countsErr)"
       # 10 is signal that this star is condemned forever (vs. e.g. 1)
       handleSet $star1.badstar 10 
       return ""
   }
   # get syncreg 
   set syncreg [handleBindFromHandle [handleNew] *$star1.syncreg]
   set nrow [exprGet $syncreg.nrow]
   set ncol [exprGet $syncreg.ncol] 
   # make region for PSF_REG
   set reg [regNew [expr $nrow+2*$delta] [expr $ncol+2*$delta]]
   regIntSetVal $reg [softBiasGet]
   # copy syncreg to reg
   set temp [subRegNew $reg $nrow $ncol $delta $delta]
   regIntCopy $temp $syncreg
   handleDel $syncreg 
   # subtract sky
   set sky [expr int([exprGet $star1.object->sky] + 0.5)]
   set skyCorrection [expr -1*$sky]
   regIntConstAdd $temp $skyCorrection
   regDel $temp
   # get approximate center
   set rpeak [expr int($delta + $nrow/2.0)]
   set cpeak [expr int($delta + $ncol/2.0)]
   if {$refine_center} {
      # refine center
      if [catch {
	 set clist [objectCenterFind $reg $rpeak $cpeak 0 \
			-bkgd [softBiasGet] -sigma 1]
      } msg] {
          echo $msg
          regDel $reg
          return ""
      }
      set new_rowc [keylget clist rowc]; set new_colc [keylget clist colc]
      # make it dead centered
      set row_shift [expr 0.5*$nrow + $delta - $new_rowc]
      set col_shift [expr 0.5*$ncol + $delta - $new_colc]
      regIntShift $reg -out $reg $row_shift $col_shift
      # correct initial master coordinates
      set rowc [expr $rowc - $row_shift]
      set colc [expr $colc - $col_shift] 
   }

   return [list $reg $rowc $colc $counts $countsErr]
}



# check if PSF_BASIS produces ap.corr in each chip corner 
# between apCor_min and apCor_max; if so return 1
proc check_psfBasis {psfBasis rpf apCor_min apCor_max _FrameMin _FrameMax {print_apCor 0}} {
   
global nrowperframe ncolperframe
upvar $_FrameMin FrameMin $_FrameMax FrameMax

    # n columns
    set cpf $ncolperframe

    set paC $print_apCor

    ## center
    set x [expr $rpf/2]; set y [expr $cpf/2] 
    set apCor [checkPSF $psfBasis $x $y "PSF reconstructed at ($x,$y)" $paC]
    set FrameMin $apCor; set FrameMax $apCor
    if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}

    ## edges with 250 pixel step (30 points)
    # horizontal
    set nstep 250
    set Nrow [expr $rpf/$nstep+1]
    loop i 0 $Nrow {
       set x [expr $i*$nstep]
       if {$x > $rpf} {set x $rpf}
       foreach y "0 $cpf" {
           set apCor [checkPSF $psfBasis $x $y "PSF reconstructed at ($x,$y)" $paC]
           set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
           if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
       }
    }    
    # vertical
    set Ncol [expr $cpf/$nstep+1]
    loop j 0 $Ncol {
       set y [expr $j*$nstep]
       if {$y > $cpf} {set y $cpf}
       foreach x "0 $rpf" {
           set apCor [checkPSF $psfBasis $x $y "PSF reconstructed at ($x,$y)" $paC]
           set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
           if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
       }
    }

    # for binned data (which don't have these positions) return 
    if {$rpf < 907 || $cpf < 1536} {
      return 1
    }
  
    ## inner grid with ~500 pixel step (4 points), only for regular data (no binning)
    foreach x "454 907" {
       foreach y "512 1536" {
           set apCor [checkPSF $psfBasis $x $y "PSF reconstructed at ($x,$y)" $paC]
           set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
           if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
       }
    }  

   return 1

}


# check if PSF_BASIS produces ap.corr in each chip corner 
# between apCor_min and apCor_max; if so return 1
proc check_psfBasis2 {psfBasis rpf apCor_min apCor_max _FrameMin _FrameMax {print_apCor 0}} {
 
upvar $_FrameMin FrameMin $_FrameMax FrameMax

      set paC $print_apCor
      ## PSF at various positions prone to failure
      # 4 corners and top and bottom middle points
      set apCor [checkPSF $psfBasis 0 0 "PSF reconstructed at LL corner" $paC]
      set FrameMin $apCor; set FrameMax $apCor
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis 0 2048 "PSF reconstructed at LR corner" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $rpf 0 "PSF reconstructed at UL corner" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $rpf 2048 "PSF reconstructed at UR corner" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $rpf 1024  "PSF reconstructed at top mid point" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis 0 1024  "PSF reconstructed at bottom mid point" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      # 3 points on left end right edge
      set H1 [expr $rpf/4]
      set H2 [expr $rpf/2]
      set H3 [expr 3*$rpf/4]
      set apCor [checkPSF $psfBasis $H1 0 "PSF reconstructed at bottom left edge" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $H2 0 "PSF reconstructed at mid left edge" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $H3 0 "PSF reconstructed at top left edge" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $H1 2048 "PSF reconstructed at bottom right edge" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $H2 2048 "PSF reconstructed at mid right edge" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      set apCor [checkPSF $psfBasis $H3 2048 "PSF reconstructed at top right edge" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
      # center
      set apCor [checkPSF $psfBasis $H2 1024 "PSF reconstructed at the center" $paC]
      set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
      if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}

      # upvar 2 mem mem; write_mem mem "check_psfBasis: grid"   

    if {0} {
      # 100x100 pixel grid (330 points)
      loop i 0 22 {
         loop j 0 15 {
            set x [expr $j*100]
            set y [expr $i*100]
	    if {$x > $rpf} {set x $rpf}
	    if {$y > 2048} {set y 2048}
            set apCor [checkPSF $psfBasis $x $y "PSF reconstructed at ($x,$y)" $paC]
            set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
            if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
         }
      } 
    }

    if {1} {
      # 300x300 pixel grid (35 points)
      loop i 0 7 {
         loop j 0 5 {
            set x [expr $j*300]
            set y [expr $i*300]
	    if {$x > $rpf} {set x $rpf}
	    if {$y > 2048} {set y 2048}
            set apCor [checkPSF $psfBasis $x $y "PSF reconstructed at ($x,$y)" $paC]
            set FrameMin [expr min($FrameMin,$apCor)]; set FrameMax [expr max($FrameMax,$apCor)] 
            if {$apCor < $apCor_min || $apCor > $apCor_max} {return -1}
         }
      } 
    }


   # upvar 2 mem mem; write_mem mem "done check_psfBasis"   

   return 1

}



# check if PSF_BASIS produces ap.corr in each chip corner 
# between apCor_min and apCor_max; if so return 1
proc checkPSF {psfBasis rowc colc text {print_apCor 0} } {
   global TYPE_PIX_NAME
   set debug 0

    if {1} {
	set apCor [psfBasis2apCorr $psfBasis $rowc $colc]
    } else {
       # PSF at (rowc,colc)
       set reg [psfKLReconstruct $psfBasis $rowc $colc -regType $TYPE_PIX_NAME]
       if {[exprGet $reg.counts] > 1000.0} {
           set apCor 1.0
       } else {
           set apCor -1.0
       }
       #display *$reg.reg $text   
       psfRegDel $reg
    }
   
    # if there are pixels with 0 counts reset apCor
    set reg [psfKLReconstruct $psfBasis $rowc $colc -regType $TYPE_PIX_NAME]
    if {1} {
        # truncation bug in astrotools
        # set stats [regStatsFind *$reg.reg]
        # workaround
        set tmp [handleBindFromHandle [handleNew] *$reg.reg]
        set stats [regStatsFind $tmp]
        handleDel $tmp
        if {[keylget stats low] <= 0} {
           # display *$reg.reg "PSF with 0 count pixels"
           set apCor -1   
        }
    } else {
        regStatsFromQuartiles -coarse 1 *$reg.reg -minpix min  
        if {$min <= 0} {
           # display *$reg.reg "PSF with 0 count pixels"
           set apCor -1   
        }
    }
    psfRegDel $reg

    if {$debug || $print_apCor} {
        echo "For $text apCorr = $apCor" 
    }

  return $apCor

}



# set and return "default" PSF
proc set_default_PSF {filter ifilter psField_default cframe {frame_info ""}} {

 global PSP_STATUS
 global schemaOK


   handleSet $cframe.calib<$ifilter>->status $PSP_STATUS(NOPSF)

   if {$frame_info != ""} {
       handleSet $frame_info.apCorr_min 1.0
       handleSet $frame_info.apCorr_max 1.0
   }
   if {![info exists schemaOK] || $schemaOK != 1} {
       echo "ERROR set_default_PSF: had to declare schematrans !?"
       declare_schematrans 5
   }

   if [catch {read_psfBasis psfBasis $psField_default $filter 0}] {
       echo "cannot read default PSF_BASIS from file $psField_default"
       return ""        
   }

   return $psfBasis($filter)
} 

