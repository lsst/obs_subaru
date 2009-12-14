#
# RHL test code for variable PSF stuff. Misc utilities to play with
# algorithms; they were used to generate psPSF files until the PSP
# learnt the trick (see make_psPSF)
#

proc find_KL_PSF {args} {
   global objcIo;			# RHL

   set is_fpObjc 1
   set measure 0;
   set scale_psfs 0
   set opts [list \
		 [list find_KL_PSF "Test the variable PSF code"] \
		 [list <filetag> STRING "" filetag \
		      "file tag supplied to open_mytables (or field number)"] \
		 [list <ncomp> INTEGER 0 ncomp \
		      "Number of KL components to keep"] \
		 [list <n_b> INTEGER 0 nrow_b \
		      "Degree of polynomial for spatial variation"] \
		 [list -use STRING "" psf_stars \
		      "use only listed stars to estimate PSF (row #); may start !"] \
		 [list {[n1]} INTEGER 1 n1 "starting object ID"] \
		 [list {[n2]} INTEGER -1 n2 "ending object ID (-1: all)"] \
		 [list {-band} INTEGER 2 band "index of filter to use"] \
		 [list -ncol_b INTEGER -1 ncol_b "order for col spatial fit"] \
		 [list -measure CONSTANT 1 measure \
		      "Measure properties of PSFs"] \
		 [list -scale CONSTANT 1 scale_psfs \
		      "Scale stars to all have the same amplitude"] \
		 [list "-sao" INTEGER -1 fsao \
		      "Display mosaic on this saoimage"] \
		 [list "-sao_basis" INTEGER -1 fsao_basis \
		      "Display basis images on this saoimage"] \
		 [list "-nann" INTEGER 4 nann \
		      "Number of annuli to use in aperture"] \
		 [list -root_dir STRING "." root_dir \
		      "Directory to look for data files"] \
		 [list -outfile STRING "" outfile \
		      "Name of output file (prefix with + to append)"] \
		 [list -fitsfile STRING "" fitsfile \
		      "Name of output psPSF file (prefix with + to append)"] \
		 [list -tsObj CONSTANT 0 is_fpObjc \
		      "read tsObj rather than fpObjc files"] \
		 [list -min_counts DOUBLE -1 min_counts \
		      "minimum value of psfCounts to use star in KL estimation"] \
		 ]
   if {[shTclParseArg $args $opts tst_var_mosaic] == 0} {
      return 
   }
   if {$ncol_b < 0} { set ncol_b $nrow_b }

   global ID
   set odd_str [join $psf_stars ":"]
   set ID "$filetag KL n_a=$ncomp n_b=$nrow_b,$ncol_b min=$min_counts use=$odd_str Band=$band"

   if {$psf_stars == "a"} { set psf_stars "" }
   set size 35;				# size of PSF postage stamps
   set border 10;			# include $border pixels around image

   set flags ""
   if !$is_fpObjc {
      lappend flags "-tsObj"
   }

   global openit;			# save openit for later
   if [info exists openit] {
      array set saved_openit [array get openit]
   }
   
   eval set_run -root $root_dir 1 1 $flags
   set table [openit $filetag]
   index_table table -identity
   if {$n2 < 0} {
      set n2 [keylget table OBJnrow]
   }

   if {[lindex $psf_stars 0] == "!"} {
      set not_psf [lreplace $psf_stars 0 0]
      unset psf_stars
      loop i $n1 [expr $n2 + 1] {
	 if {[lsearch $not_psf $i] < 0} {
	    lappend psf_stars $i
	 }
      }
      
   } elseif {[regexp {^(e|o)$} $psf_stars]} {# we wish to sample PSF stars
      loop i $n1 [expr $n2 + 1] {
	 set objcIo [read_objc table $i]
	 lappend rowc_list [list $i [exprGet $objcIo.objc_rowc]]
      }
      
      set rowc_list [lsort -command sort_by_rowc $rowc_list]
      loop i 0 [llength $rowc_list] {
	 set id_by_rowc([lindex [lindex $rowc_list $i] 0]) $i
      }
      
      set use_odd [expr [string compare $psf_stars "e"] ? 0 : 1]
      unset psf_stars
      loop i $n1 [expr $n2 + 1] {
	 if {$id_by_rowc($i)%2 == $use_odd} {
	    lappend psf_stars $i
	 }
      }
   }
   #
   # Find the required PSF_KERNEL
   #
   set regs [chainNew PSF_REG]
   loop id $n1 [expr $n2 + 1] {
      if {$psf_stars != "" && [lsearch $psf_stars $id] < 0} {
	 #echo RHL Omitting $id
	 continue;
      }
	    
      if {[set psfReg [get_psfReg table $id $border $size $band]] == ""} {
	 continue
      }

      if ![info exists nrow] {
	 set nrow [exprGet $psfReg.reg->nrow];
	 set ncol [exprGet $psfReg.reg->ncol]
      }

      if {[exprGet $psfReg.counts] < $min_counts} {
	 psfRegDel $psfReg
	 continue;
      }

      if $scale_psfs {
	 #
	 # Fix all stars to have the same amplitude
	 #
	 set fac [expr 1.4e5/[exprGet $psfReg.counts]]
	 handleSet $psfReg.counts [expr [exprGet $psfReg.counts]*$fac]
	 regIntLincom *$psfReg.reg "" [expr 1000*(1 - $fac)] $fac 0
	 #      error [mtv *$psfReg.reg]
      }

      chainElementAddByPos $regs $psfReg TAIL
      handleDel $psfReg
   }

   set b2 [psfKLDecomp -border $border $regs $ncomp $nrow_b]
   psfKernelSet $regs $b2 *$b2.kern

   psfRegChainDestroy $regs

   if {$fitsfile != ""} {
      if [regsub {^\+} $fitsfile "" fitsfile] {
	 set mode "a"
      } else {
	 set mode "w"
      }

      write_psfBasis $b2 $fitsfile $mode
   }
   #print_kernel *$b2.kern 1 1

   if {$fsao_basis > 0} {
      regDel [mtv [mosaic_basis $b2] -s $fsao_basis]
   }

   #
   # Restore the initial openit array
   #
   if [info exists saved_openit] {
      array set openit [array get saved_openit]
   }
   #
   # Calculate PSF at position of each star, and make an Illustrative Mosaic
   #
   set npanel 2
   set Ncol [expr int(sqrt($npanel*($n2-$n1+2)))]
   set Nrow [expr ($n2-$n1+2)/$Ncol]; if {$Nrow <= 0} { set Nrow 1 }
   while {$Ncol*$Nrow < $n2-$n1+2} { incr Ncol }
   set mosaic [regNew -type FL32 [expr $npanel*$nrow*$Nrow] [expr $ncol*$Ncol]]
   regClear $mosaic
   
   set i 0; set j 0;			# used for making mosaic
   loop id2 $n1 [expr $n2 + 1] {
      if {[set psfReg2 [get_psfReg table $id2 $border $size $band]] == ""} {
	 continue
      }
      set reg2 [handleBindFromHandle [handleNew] *$psfReg2.reg]

      set rowc [exprGet $psfReg2.rowc]; set colc [exprGet $psfReg2.colc]
      #
      # Estimate PSF at reg2's position
      #
      set treg2 [psfKLReconstruct $b2 $rowc $colc]
      #echo RHL $id2; mtv -s 2 *$treg2.reg

      #
      # Make mosaic
      #
      set k -1
      set sreg [subRegNew $mosaic \
		    $nrow $ncol [expr $j+[incr k]*$nrow] [expr $i*$ncol]]
      regIntCopy $sreg $reg2; regDel $sreg

      set sreg [subRegNew $mosaic \
		    $nrow $ncol [expr $j+[incr k]*$nrow] [expr $i*$ncol]]
      regIntCopy $sreg *$treg2.reg; regDel $sreg

      if {[incr i] == $Ncol} {
	 incr j [expr $npanel*$nrow]
	 set i 0
      }

      handleDel $reg2; psfRegDel $treg2
      psfRegDel $psfReg2
   }

   psfBasisDel $b2

   objfileClose table
   if {$measure || $fsao > 0} {
      #
      # Convert to U16
      #
      set imosaic [regNew [exprGet $mosaic.nrow] [exprGet $mosaic.ncol]]
      set mask [spanmaskNew \
		    -nrow [exprGet $mosaic.nrow] -ncol [exprGet $mosaic.ncol]]
      handleSetFromHandle $imosaic.mask (MASK*)&$mask
      handleDel $mask
      
      regIntCopy $imosaic $mosaic
   }
   regDel $mosaic

   if {$fsao > 0} {
      mtv -s $fsao $imosaic
   }

   if $measure {
      #
      # Measure properties of raw/corrected PSFs?
      #
      echo ""
      if {$fsao > 0} {
	 saoReset $fsao
      }
      measure_mosaic $imosaic $band [expr $n2 - $n1 + 1] \
	  $npanel -sao $fsao -nann $nann -outfile $outfile
   }

   if {[info exists imosaic] && $fsao <= 0} {
      regDel $imosaic
      set imosaic ""
   } else {
      set imosaic ""
   }

   return $imosaic
}

#
# Read a star from an OBJC_IO, center it in the region,
# and pad out the edges for the convolution filters
#
proc get_star {args} {
   set opts [list \
		 [list [info level 0] ""] \
		 [list <objc> STRING "" objc "The OBJC_IO"] \
		 [list <band> INTEGER 0 band "The desired band"] \
		 [list -border  INTEGER 5 border "The border to add around the star"] \
		 [list -size  INTEGER 35 size \
		      "Number of rows and columns in returned REGION (excluding border)"] \
		 [list -sigma DOUBLE 0 sigma "standard deviation of background's noise"] \
		 [list -rand STRING "" rand "Source of entropy (required if sigma > 0)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   global table
   
   set psfCounts [exprGet $objc.psfCounts<$band>]

   if {[info exists table] && [keylget table fileSchema] == "TSOBJ"} {
      set psfCounts [expr 1e6*pow(10, -0.4*($psfCounts - 14))]
   }

   set nrow [expr $size + 2*$border]
   set ncol $nrow

   set nrowAI [expr [exprGet $objc.aimage->master_mask->rmax] - [exprGet $objc.aimage->master_mask->rmin] + 1]
   set ncolAI [expr [exprGet $objc.aimage->master_mask->cmax] - [exprGet $objc.aimage->master_mask->cmin] + 1]
   if {$nrowAI < $nrow} { set nrowAI $nrow }
   if {$ncolAI < $ncol} { set ncolAI $ncol }
   if {$nrowAI%2 == 0} { incr nrowAI }
   if {$ncolAI%2 == 0} { incr ncolAI }

   set reg [regNew [expr $nrowAI + 2*$border] [expr $ncolAI + 2*$border]]
   regIntSetVal $reg 1000
   if {$sigma > 0} {
      regIntGaussianAdd $reg $rand 0 $sigma
   }
   regSetFromAtlasImage *$objc.aimage $band $reg -ignore_off \
       -row0 [expr [exprGet $objc.aimage->master_mask->rmin] - $border] \
       -col0 [expr [exprGet $objc.aimage->master_mask->cmin] - $border]
   set cen [regIntMaxPixelFind $reg]
   set rpeak [lindex $cen 0]; set cpeak [lindex $cen 1]
   set peak [expr [regPixGet $reg $rpeak $cpeak] - 1000]

   if {$peak < 50} {
      if 0 {
	 echo Object at \
	     ([exprGet $objc.objc_rowc], [exprGet $objc.objc_colc]) \
	     band $band is too faint (maximum: $peak)
      }
      regDel $reg
      return ""
   }
   
   if [catch {
      set clist [objectCenterFind $reg $rpeak $cpeak 0 -bkgd 1000 -sigma 1]
   } msg] {
      echo $msg
      regDel $reg
      return ""
   }
   
   set rowc [keylget clist rowc]; set colc [keylget clist colc]
   set drow [expr $nrowAI/2 + 0.5 + $border - $rowc]; set dcol [expr $ncolAI/2 + 0.5 + $border - $colc]
   #mtv $reg; puts -nonewline "Before rc=($rowc,$colc): "; if {[gets stdin] == "q"} { error $reg }
   regIntShift $reg -out $reg $drow $dcol
   #mtv $reg; puts -nonewline "After   d=$drow,$dcol: "; if {[gets stdin] == "q"} { error $reg }
   
   set rowc [exprGet $objc.rowc<$band>];# in parent frame
   set colc [exprGet $objc.colc<$band>]
   #
   # Trim object to desired size
   #
   set treg [regNew $nrow $ncol]
   set sreg [subRegNew $reg $nrow $ncol \
		 [expr ($nrowAI - $nrow)/2 + $border] [expr ($ncolAI - $ncol)/2 + $border]]
   regIntCopy $treg $sreg

   regDel $sreg; regDel $reg
   set reg $treg

   #mtv $reg; gets stdin
   #
   # Replace star by a DGPSF. XXX RHL
   #
   if 0 {
      global star_rowc star_colc
      if {[info exists star_rowc]} {
	 set rowc $star_rowc; set colc $star_colc
	 set sigmax1 [expr 1.2 + $rowc*1e-4]
	 set sigmay1 [expr $sigmax1 + 0.1 - $colc*1e-4]
	 set sigmax1 3.0; set sigmay1 4.0
	 set psf [dgpsfNew -sigmax1 $sigmax1 -sigmay1 $sigmay1 -sigmax2 3 -b 0 \
		      -a [expr 10000+100*$sigmax1]]
	 regIntSetVal $reg 1000;
	 dgpsfAddToReg $reg $psf [expr $border + 0.5*$nrow] [expr $border + 0.5*$ncol]
	 #echo RHL [exprGet $psf.sigmax1] [exprGet $psf.a]
	 set psfCounts [expr 2*acos(-1)*[exprGet $psf.a]*\
			    ($sigmax1*$sigmay1 + [exprGet $psf.b]*[exprGet $psf.sigmax2]*[exprGet $psf.sigmay2])]
	 
	 dgpsfDel $psf
      }
   }
      
   return [list $reg $rowc $colc $psfCounts]
}

proc get_psfReg {_table id border size band} {
   upvar $_table table
   
   set use_gaussians 1
   if $use_gaussians {
      global star_rowc star_colc;
      set star_rowc [expr 100*$id]
      set star_colc [expr 100*$id]
   }

   global objcIo

   read_objc table $id

   if {[set vals [get_star $objcIo $band -border $border -size $size]] == ""} {
      return ""
   }
   
   set psfReg [eval psfRegNew $vals]
   handleDel [lindex $vals 0];		# the region
   
   return $psfReg
}

proc tst_var_mosaic {args} {
   global objcIo;			# RHL

   set nann 4;			# number of annuli for aperture
   set ncol_b -1; set uncorr 0; set measure 0; set band 2; set fsao -1
   set use_odd a;			# use only PSFs with id%2 == use_odd
   set opts [list \
		 [list tst_KL "Test the variable PSF code"] \
		 [list <filetag> STRING "" filetag \
		      "file tag supplied to open_mytables (or field number)"] \
		 [list <sigmas> STRING "" sigmalist "List of sigmas to use"] \
		 [list <n_a> INTEGER 0 n_a \
		      "Degree of polynomial for kernel"] \
		 [list <n_b> INTEGER 0 nrow_b \
		      "Degree of polynomial for spatial variation"] \
		 [list <id> INTEGER 1 id \
		      "Row number for PSF star (-1: use average)"] \
		 [list {[n1]} INTEGER 1 n1 "starting object ID"] \
		 [list {[n2]} INTEGER -1 n2 "ending object ID (-1: all)"] \
		 [list {-band} INTEGER 2 band "index of filter to use"] \
		 [list -ncol_b INTEGER -1 ncol_b "order for col spatial fit"] \
		 [list -uncorr CONSTANT 1 uncorr \
		      "show uncorrected residuals"] \
		 [list -measure CONSTANT 1 measure \
		      "Measure properties of PSFs"] \
		 [list -use INTEGER 1 use_odd \
		      "Only use PSFs with id%2 == use in constructing average"] \
		 [list "-sao" INTEGER -1 fsao \
		      "Display mosaic on this saoimage"] \
		 [list "-nann" INTEGER $nann nann \
		      "Number of annuli to use in aperture"] \
		 ]
   if {[shTclParseArg $args $opts tst_var_mosaic] == 0} {
      return 
   }
   if {$ncol_b < 0} { set ncol_b $nrow_b }
   if {$use_odd == "a"} { set use_odd -1 }

   global star_rowc
   if [info exists star_rowc] { unset star_rowc };# controls faking get_star

   global ID
   if {$use_odd >= 0} {
      set odd_str $use_odd
   } else {
      set odd_str "a";
   }
   set ID "$filetag sigma=\\\"[join $sigmalist ":"]\\\" n_a=$n_a n_b=$nrow_b,$ncol_b PSF=$id use=$odd_str Band=$band"

   set size 35;				# size of PSF postage stamps
   set border 10;			# include $border pixels around image
   set sqrt 0;				# divide by sqrt(image)?

   set_run -root . 1 1
   set table [openit $filetag]
   index_table table -identity
   if {$n2 < 0} {
      set n2 [keylget table OBJnrow]
   }

   if {$id > 0} {
      if {[set psfReg1 [get_psfReg table $id $border $size $band]] == ""} {
	 error "Cannot use $id for PSF"
      }
      set reg2 [handleBindFromHandle [handleNew] *$psfReg1.reg]
      set nrow [exprGet $reg2.nrow]; set ncol [exprGet $reg2.ncol]
      set reg1 [regNew -type FL32 $nrow $ncol]
      regIntCopy $reg1 $reg2
      set counts1 [exprGet $psfReg1.counts]
   } else {
      if {[set psfReg1 [get_psfReg table $n1 $border $size $band]] == ""} {
	 continue;
      }
      set reg2 [handleBindFromHandle [handleNew] *$psfReg1.reg]
      set nrow [exprGet $reg2.nrow]; set ncol [exprGet $reg2.ncol]
      set reg1 [regNew -type FL32 $nrow $ncol]
      regDel $reg2
      handleSetFromHandle $psfReg1.reg &$reg1

      if {$use_odd >= 0} {		# we wish to sample PSF stars
	 loop id $n1 [expr $n2 + 1] {
	    set objcIo [read_objc table $id]
	    lappend rowc_list [list $id [exprGet $objcIo.objc_rowc]]
	 }

	 set rowc_list [lsort -command sort_by_rowc $rowc_list]
	 loop id 0 [llength $rowc_list] {
	    set id_by_rowc([lindex [lindex $rowc_list $id] 0]) $id
	 }
      }

      regClear $reg1; set counts1 0; set npsf 0
      loop id $n1 [expr $n2 + 1] {
	 if {$use_odd >= 0 && $id_by_rowc($id)%2 != $use_odd} {
	    continue;
	 }

	 if {[set psfReg2 [get_psfReg table $id $border $size $band]] == ""} {
	    continue;
	 }
	 incr npsf
	 
	 #echo RHL using $id == [exprGet $objcIo.id]

	 set reg2 [handleBindFromHandle [handleNew] *$psfReg2.reg]
	 regIntLincom $reg1 $reg2 0 1 1
	 set counts1 [expr $counts1 + [exprGet $psfReg2.counts]]

	 handleDel $reg2; handleDel $psfReg2
      }

      foreach el [list \
		      "$nrow $border 0 0" \
		      "$border $ncol 0 0" \
		      "$nrow $border 0 [expr $ncol-$border]" \
		      "$border $ncol [expr $nrow-$border] 0" \
		     ] {
	 set sreg [subRegNew $reg1 \
		       [lindex $el 0] [lindex $el 1] \
		       [lindex $el 2] [lindex $el 3]]
	 regClear $sreg
	 regDel $sreg
      }
      regMultiplyWithDbl -regOut $reg1 $reg1 [expr 1.0/$npsf]
      set counts1 [expr $counts1/$npsf]
   }
   #error [regDel [mtv $reg1]]
   #
   # Create the desired PSF_BASIS. We are going to find the transformation
   # from $reg1's seeing to that of any other star
   #
   # (there's no particular reason for having chosen $reg1)
   #
   if {$sigmalist != "KL"} {		# use the specified kernel
      set k [make_kernel $border $sigmalist $n_a $nrow_b $ncol_b]
      set b2 [regConvolveWithPsfKernel $reg1 $k -type FL32]
      psfKernelDel $k
   }

   if 0 {				# RHL
      regDel [mtv [mosaic_basis $b2]]

      handleDel $reg1
      psfRegDel $psfReg1
      psfBasisDel $b2
      
      error "PSF_BASIS mosaic"
   }
   #
   # Find the required PSF_KERNEL
   #
   set regs [chainNew PSF_REG]
   loop id $n1 [expr $n2 + 1] {
      if {$use_odd >= 0 && $id_by_rowc($id)%2 != $use_odd} {
	 continue;
      }
	    
      if {[set psfReg [get_psfReg table $id $border $size $band]] == ""} {
	 continue;
      }
      #
      # Fix all stars to have the same amplitude
      #
      set fac [expr 1.4e5/[exprGet $psfReg.counts]]
      handleSet $psfReg.counts [expr [exprGet $psfReg.counts]*$fac]
      regIntLincom *$psfReg.reg "" [expr 1000*(1 - $fac)] $fac 0
#      error [mtv *$psfReg.reg]

      chainElementAddByPos $regs $psfReg TAIL
      handleDel $psfReg
   }

   if {$sigmalist == "KL"} {		# find the KL decomposition
      set b2 [psfKLDecomp -border $border $regs $n_a $nrow_b]
      #regDel [mtv [mosaic_basis $b2] -s 2];# RHL
      #print_kernel *$b2.kern 0
   }

   set trans [handleBindFromHandle [handleNew] *$b2.kern]
   psfKernelSet $regs $b2 $trans
   #print_kernel $trans

   psfRegChainDestroy $regs
   #
   # Correct our stars and make an Illustrative Mosaic
   #
   if $uncorr {				# show uncorrected diff image too
      set npanel 4
   } else {
      set npanel 3
   }

   set nrow [exprGet $reg1.nrow]; set ncol [exprGet $reg1.ncol]
   set Ncol [expr int(sqrt($npanel*($n2-$n1+2)))]
   set Nrow [expr ($n2-$n1+2)/$Ncol]; if {$Nrow <= 0} { set Nrow 1 }
   while {$Ncol*$Nrow < $n2-$n1+2} { incr Ncol }
   set mosaic [regNew -type FL32 [expr $npanel*$nrow*$Nrow] [expr $ncol*$Ncol]]
   regClear $mosaic
   
   set i 0; set j 0;			# used for making mosaic
   loop id2 [expr $n1-1] [expr $n2 + 1] {
      if {$id2 < $n1} {
	 set sreg [subRegNew $mosaic $nrow $ncol 0 0]
	 regIntCopy $sreg $reg1; regDel $sreg

	 incr i
	 continue
      }

      if {[set psfReg2 [get_psfReg table $id2 $border $size $band]] == ""} {
	 continue;
      }
      set reg2 [handleBindFromHandle [handleNew] *$psfReg2.reg]

      if $sqrt {
	 set sqrt_reg2 [regNew -type FL32 $nrow $ncol]
	 regIntCopy $sqrt_reg2 $reg2; regAddWithDbl $sqrt_reg2 100
	 regSqrt $sqrt_reg2; regMultiplyWithDbl $sqrt_reg2 1
	 set sqrt_bkgd [regNew $nrow $ncol]; regIntSetVal $sqrt_bkgd 1000
	 regDivide $sqrt_bkgd $sqrt_reg2
      }

      #error [regDel [mtv $sqrt_reg2]]
      set counts2 [exprGet $psfReg2.counts]

      set rowc [exprGet $psfReg2.rowc]; set colc [exprGet $psfReg2.colc]
      handleSet $psfReg1.rowc $rowc
      handleSet $psfReg1.colc $colc
      #
      # Transform reg1's PSF to that at reg2's position
      #
      if {$sigmalist == "KL"} {		# Use a KL basis
	 set treg2 [psfKLReconstruct $b2 $rowc $colc]
      } else {
	 set treg2 [psfKernelApply $psfReg1 $trans]
      }
      #echo RHL $id2; mtv -s 2 $treg2
      #
      # Scale treg2 to the same flux as reg2
      #
      if 1 {
	 set stats [regStatsFind $reg1]
	 set mean1 [keylget stats mean]
	 set stats [regStatsFind $treg2]
	 set meant [expr [keylget stats mean]]
	 regMultiplyWithDbl -regOut $treg2 $treg2 [expr (1.0*$mean1/$meant)]
      }
      if 1 {
	 set scale [expr 1.0*$counts2/$counts1]
	 regMultiplyWithDbl -regOut $treg2 $treg2 $scale
      }

      set tmp [regNew $nrow $ncol]; regIntCopy $tmp $treg2
      set diff [regNew $nrow $ncol]; regIntCopy $diff $reg2
      regIntLincom $diff $tmp 1000 1 -1

      if $sqrt {
	 regDivide $diff $sqrt_reg2;
	 regIntLincom $diff $sqrt_bkgd 1000 1 -1
      }
      regStatsFromQuartiles $diff -sigma var

      regDel $tmp

      if $uncorr {			# show uncorrected diff image too
	 set scale [expr 1.0*$counts2/$counts1]

	 set tmp1 [regNew $nrow $ncol -type FL32]; regIntCopy $tmp1 $reg1
	 regMultiplyWithDbl -regOut $tmp1 $tmp1 $scale
	 set scaled1 [regNew $nrow $ncol]; regIntCopy $scaled1 $tmp1
	 regDel $tmp1
	 
	 set diff_uncorr [regNew $nrow $ncol]; regIntCopy $diff_uncorr $reg2
	 regIntLincom $diff_uncorr $scaled1 1000 1 -1

	 if $sqrt {
	    regDivide $diff_uncorr $sqrt_reg2;
	    regIntLincom $diff_uncorr $sqrt_bkgd 1000 1 -1
	 }

	 if 0 {
	    regStatsFromQuartiles $diff_uncorr -sigma var_un
	    echo $id2 $var_un $var
	 }

	 regDel $scaled1
      }

      set k -1
      set sreg [subRegNew $mosaic \
		    $nrow $ncol [expr $j+[incr k]*$nrow] [expr $i*$ncol]]
      regIntCopy $sreg $reg2; regDel $sreg

      set sreg [subRegNew $mosaic \
		    $nrow $ncol [expr $j+[incr k]*$nrow] [expr $i*$ncol]]
      regIntCopy $sreg $treg2; regDel $sreg

      if [info exists diff_uncorr] {
	 set sreg [subRegNew $mosaic \
		       $nrow $ncol [expr $j+[incr k]*$nrow] [expr $i*$ncol]]
	 regIntCopy $sreg $diff_uncorr; regDel $sreg
	 regDel $diff_uncorr
      }

      set sreg [subRegNew $mosaic \
		    $nrow $ncol [expr $j+[incr k]*$nrow] [expr $i*$ncol]]
      regIntCopy $sreg $diff; regDel $sreg

      if {[incr i] == $Ncol} {
	 incr j [expr $npanel*$nrow]
	 set i 0
      }

      if $sqrt {
	 regDel $sqrt_reg2; regDel $sqrt_bkgd
      }
      handleDel $reg2; regDel $treg2; regDel $diff
      psfRegDel $psfReg2
   }

   handleDel $reg1
   psfRegDel $psfReg1
   handleDel $trans
   psfBasisDel $b2

   objfileClose table
   #
   # Convert to U16
   #
   set imosaic [regNew [exprGet $mosaic.nrow] [exprGet $mosaic.ncol]]
   set mask [spanmaskNew \
		 -nrow [exprGet $mosaic.nrow] -ncol [exprGet $mosaic.ncol]]
   handleSetFromHandle $imosaic.mask (MASK*)&$mask
   handleDel $mask
   
   regIntCopy $imosaic $mosaic
   regDel $mosaic
   #
   # Measure properties of raw/corrected PSFs?
   #
   if $measure {
      if {$fsao > 0} {
	 mtv -s $fsao $imosaic
	 saoReset $fsao
      }
      echo ""
      measure_mosaic $imosaic $band $npanel $fsao $nann
   }

   return $imosaic
}

proc sort_by_rowc {a b} {
   set row_a [lindex $a 1]; set row_b [lindex $b 1]
   set diff [expr $row_a - $row_b]
   
   if {$diff < 0} {
      return -1
   } elseif {$diff > 0} {
      return 1
   } else {
      return 0
   }
}

#
# Generate a PSF_BASIS of the PSF's KL decomposition
# e.g. set basis [tst_KL stars-111-1 4]; regDel [mtv [mosaic_basis $basis]]
#
proc tst_KL {args} {
   set nrow_b 1; set ncol_b -1
   set opts [list \
		 [list tst_KL "Test the KL code"] \
		 [list <filetag> STRING "" filetag \
		      "file tag supplied to open_mytables (or field number)"] \
		 [list <ncomp> INTEGER 0 ncomp \
		      "Number of basis functions to keep"] \
		 [list {[n1]} INTEGER 1 n1 "starting object ID"] \
		 [list {[n2]} INTEGER -1 n2 "ending object ID (-1: all)"] \
		 [list {[band]} INTEGER 2 band "index of filter to use"] \
		 [list -n_b INTEGER 1 nrow_b "order for spatial fit"] \
		 [list -ncol_b INTEGER -1 ncol_b "order for col spatial fit"] \
		 ]
   if {[shTclParseArg $args $opts tst_KL] == 0} {
      return 
   }
   if {$ncol_b < 0} { set ncol_b $nrow_b }
   
   global star_rowc
   if [info exists star_rowc] { unset star_rowc };# controls faking get_star

   set size 35
   set border 10;			# include $border pixels around image
   #
   # Read all the stars
   #
   set_run -root . 1 1
   set table [openit $filetag]
   index_table table -identity
   if {$n2 < 0} {
      set n2 [keylget table OBJnrow]
   }

   set regs [chainNew PSF_REG]
   loop id $n1 [expr $n2 + 1] {
      if {[set psfReg [get_psfReg table $id $border $size $band]] == ""} {
	 continue;
      }
      #
      # Fix all stars to have the same amplitude
      #
      set fac [expr 1.4e5/[exprGet $psfReg.counts]]
      handleSet $psfReg.counts [expr [exprGet $psfReg.counts]*$fac]
      regIntLincom *$psfReg.reg "" [expr 1000*(1 - $fac)] $fac 0
#      error [mtv *$psfReg.reg]

      chainElementAddByPos $regs $psfReg TAIL
      handleDel $psfReg
   }
   #
   # and do the KL transform
   #
   set basis [psfKLDecomp -border $border $regs $ncomp $nrow_b -ncol_b $ncol_b]

   objfileClose table

   return $basis
}

###############################################################################
#
# Measure various parameters from a mosaic as produced with tst_var_mosaic
#
proc measure_mosaic {args} {
   set opts [list \
		 [list measure_mosaic "Measure a mosaic image"] \
		 [list <reg> STRING "" reg \
		      "Region to measure"] \
		 [list <band> INTEGER 0 band \
		      "Band to use"] \
		 [list <nstar> INTEGER 0 nstar \
		      "Number of stars in mosaic"] \
		 [list {[npanel]} INTEGER 3 npanel \
		      "Number of images of each star in mosaic"] \
		 [list "-sao" INTEGER -1 fsao \
		      "Display mosaic on this saoimage"] \
		 [list "-nann" INTEGER 4 nann \
		      "Number of annuli to use in aperture"] \
		 [list "-size" INTEGER 55 size \
		      "size of each image in mosaic"] \
		 [list "-outfile" STRING "" outfile \
		      "File to write results (append if starts \"+\")"] \
		 ]
   if {[shTclParseArg $args $opts measure_mosaic] == 0} {
      return 
   }
      
   set preg [subRegNew $reg $size $size 0 0]
   set psf0 [fitPsfFromReg $preg 0]
   regDel $preg
   psfCountsFromRegionSetup $psf0
   
   set rad_ann [lindex [profileRadii] [expr $nann - 1]]
   if {$fsao > 0} {
      saoLabel off
   }

   global ID
   set table [openit [lindex $ID 0]]
   index_table table -identity

   set nrow [exprGet $reg.nrow]; set ncol [exprGet $reg.ncol]
   set n_per_row [expr $ncol/$size]

   if {$outfile == ""} {
      set fd "stdout"
   } else {
      if [regsub {^\+} $outfile "" outfile] {
	 set mode "a"
      } else {
	 set mode "w"
      }
	 
      set fd [open $outfile $mode]
   }

   puts $fd "#"
   puts $fd "#$ID n_{ann}=$nann"
   puts $fd "#"   
   puts $fd "#ID rowc colc psfCounts apCounts ap_corr_raw ap_corr_corrected star_L"
   puts $fd "#"

   if {$npanel <= 2} {			# There's no PSF to measure
      set psf 0
   } else {
      set psf 1
   }
   set table_row 1
   loop i 0 [expr $nstar + ($psf ? 1 : 0)] {
      set row [expr ($i/$n_per_row)*$npanel*$size]
      set col [expr ($i%$n_per_row)*$size]
      set r0 $row; set c0 [expr $col + 0.25*$size]

      set row [expr $row + 0.5*$size]
      set col [expr $col + 0.5*$size]
      
      fake_measure $reg row col psfCounts apCounts $nann

      set row_obj $row; set col_obj $col;# position of real object
      if {$fsao > 0} {
	 saoDrawText -s $fsao $r0 $c0 [expr $i + ($psf ? 0 : 1)]
	 draw_cross + $row $col "-s $fsao"
	 saoDrawCircle -s $fsao $row $col $rad_ann
      }

      if {$psf && $i == 0} {		# i.e. the PSF
	 set id -1; set field -1; set rowc -1000.0; set colc -1000.0
      } else {
	 set row [expr $row + $size]
	 
	 set objcIo [read_objc table $table_row]; incr table_row
	 set id [exprGet $objcIo.id]
	 set field [exprGet $objcIo.field]
	 set rowc [exprGet $objcIo.objc_rowc]
	 set colc [exprGet $objcIo.objc_colc] \
      }
      #
      # Measure reconstructed PSF
      #
      fake_measure $reg row col psfCounts_c apCounts_c $nann

      if {$i == 0} {			# PSF
	 set star_L 1
      } else {
	 fake_measure_L $reg $objcIo $band $row_obj $col_obj $row $col star_L
      }

      if {$fsao > 0} {
	 draw_cross x $row $col "-s $fsao -e"
      }

      puts $fd [format \
		"%d:%d:%d %.2f %.2f %.1f %.1f   %.4f %.4f %g" \
		[expr $i + ($psf ? 0 : 1)] $field $id $rowc $colc \
		$psfCounts $apCounts \
		[expr 2.5*log10($apCounts/$psfCounts)] \
		[expr 2.5*log10($apCounts_c/$psfCounts_c)] \
	       $star_L]
   }

   dgpsfDel $psf0
   objfileClose table

   if {$fd != "stdout"} {
      puts $fd ""
      close $fd
   }
}

#
# Fake measure objects, specifically psfCounts, an aperture Counts measure
#
proc fake_measure {reg _rowc _colc _psfCounts _apCounts {nann 5}} {
   upvar $_rowc rowc  $_colc colc
   upvar $_psfCounts psfCounts  $_apCounts apCounts

   set val [centroidFind $reg $rowc $colc 0.0 1.0 0 2.0]
   
   set rowc [keylget val row]
   set colc [keylget val col]
   if {[keylget val peak] < 1010} {
      set psfCounts -1; set apCounts -1
      return -1
   }
   
   set psfCounts [psfCountsFromRegion $reg $rowc $colc -err err]
   set cprof [profileExtract $reg $rowc $colc -$nann 1000.0 0.0]

   set apCounts 0
   loop j 0 [expr 1 + 12*($nann - 1)] {
      set apCounts [expr $apCounts + \
			[exprGet $cprof.cells<$j>.mean]* \
			[exprGet $cprof.cells<$j>.area]]
   }
   handleDel $cprof

   return 0
}

#
# Estimate the PSF Likelihood of an object at {rowc,colc}_obj, using
# the object at {rowc,colc}_psf to characterise the PSF
#
# Various properties of the original object are in $objcIo (band $band),
# and of the original frame are in $fiparams
#
proc fake_measure_L {reg objcIo band rowc_obj colc_obj \
			 rowc_psf colc_psf _star_L} {
   upvar $_star_L star_L

   set size 51
   set preg [subRegNew $reg $size $size \
		 [expr int($rowc_psf - 0.5*$size)] \
		 [expr int($colc_psf - 0.5*$size)]]
   set psf [fitPsfFromReg $preg 0]
   regDel $preg

   set objc [objcNew 1]; set obj1 [object1New]
   handleSetFromHandle $objc.color<0> &$obj1
   handleDel $obj1
   
   handleSet $objc.color<0>->rowc $rowc_obj
   handleSet $objc.color<0>->colc $colc_obj
   foreach el "rowcErr colcErr sky skyErr" {
      handleSet $objc.color<0>->$el [exprGet $objcIo.$el<$band>]
   }

   set fiparams [fieldparamsNew "r"]
   handleSetFromHandle $fiparams.frame<0>.data &$reg
   handleSet $fiparams.frame<0>.bkgd 0

   set sky [binregionNewFromConst [exprGet $objcIo.sky<$band>]]
   set skysig [binregionNewFromConst 10]
   set fieldstat [fieldstatNew]

   set calib1 [calib1New "r"]
   handleSetFromHandle $calib1.psf &$psf
   handleSet $calib1.dark_variance 0
   handleSet $calib1.gain 2
   handleSet $calib1.app_correction 1.0

   set cprof [compositeProfileNew]
   handleSet $cprof.psfCounts 1
   handleSetFromHandle $calib1.prof &$cprof; handleDel $cprof

   measureObjColorSet $fiparams $fieldstat 0 \
	  $calib1 $reg $sky $skysig

   fitCellPsfModel $objc 0 $fiparams
   set star_L [exprGet $objc.color<0>->star_L]

   handleDel $sky; handleDel $skysig
   fieldstatDel $fieldstat
   fieldparamsDel $fiparams
   calib1Del $calib1
   objcDel $objc
   handleDel $psf
}

###############################################################################
#
# Initialise needed variables to run the main routine
#
proc fake_init_measure {} {
   param2Chain [envscan \$PHOTO_DIR]/etc/fpParam.par defpars
   set softpars ""
   #
   # choose the _last_ occurrence of each keyword, in accordance with
   # behaviour of the plan files
   #
   foreach file "defpars softpars" {
      foreach el [set $file] {
	 set key [lindex $el 0]
	 set val [join [lrange $el 1 end]]
	 keylset $file $key $val
      }
   }
   
   set catalog [envscan [getsoftpar rawprofile_file]]
   set cellcat [envscan [getsoftpar cellprofile_file]]
   
   catch {unset failed}; set failed 0
   foreach f "catalog cellcat" {
      set file [set $f]
      if {![file readable $file]} {
	 echo "You must make $file before running tests"
	 incr failed
      }
   }
   if $failed {
      error "Required input files are missing (run \"make datafiles\" in photo)"
   }
   unset failed

   set nfilter 1

   initProfileExtract
   initFitobj $catalog
   initCellFitobj $cellcat $nfilter
}

###############################################################################
#
# Procs to generate psPSF files from fpObjc/tsObj files
#
# Proceed in two steps:
#    set_run 94 1; set field 204
#    write_psfs PSF foo $field 3 -1
# (writes PSF/tsObj-foo.fit and PSF/fpAtlas-foo.fit)
#    set fitsfile [format psPSF-$openit(run)-$openit(col)-%04d.fit $field]
#    make_psPSF PSF foo $fitsfile 5 3 3
# (writes the desired psPSF file)
#

proc make_psPSF {dir filetag fitsfile ncomp nrow_b ncol_b {min 5e3}} {
   exec rm -f $dir/$fitsfile
   loop b 0 5 {
      find_KL_PSF -ts -root $dir $filetag $ncomp $nrow_b -ncol $ncol_b \
	  -min $min -use "a" -band $b -fits +$dir/$fitsfile
   }
}


#
# Write a private fpObjc table
#
proc write_mytable {args} {
   set opts {
      {write_mytable "Write a subset of objects to a table"}
      {{<table>} STRING "" _table "Name of original table, as set by openit"}
      {{<select>} STRING "" select \
	   "Proc to select objects (see \"help proc\")"}
      {{[filetag]} STRING "" filetag "Identifying string for new table"}
      {-fds STRING "" _fds "Name of output table, as set by open_mytables"}
      {-mtv CONSTANT 1 mtv "Display chosen objects"}
      {-drow DOUBLE 0 drow "Add this to all object row coordinates"}
   }

   set mtv 0
   if {[shTclParseArg $args $opts write_mytable] == 0} {
      return 
   }

   upvar $_table table
   global objcIo
   
   if {$_fds != ""} {
      if {$filetag != ""} {
	 error "You cannot specify both -fds and a filetag ($filetag)"
      }
      upvar $_fds fds
   } else {
      if {[keylget table fileSchema] == "TSOBJ"} {
	 set ts "-ts"
      } else {
	 set ts ""
      }
      set fds [eval open_mytables table $filetag $ts]
   }
   set nobj 0
   loop i 1 [expr [keylget table OBJnrow]+1] {
      read_objc table $i
      if [eval $select $objcIo] {
	 if $mtv {
	    mtv_objc
	 }
	 incr nobj
	 if {$drow != 0.0} {
	    handleSet $objcIo.objc_rowc \
		[expr [exprGet $objcIo.objc_rowc] + $drow]
	 }
	 write_mytables fds $objcIo
      }
   }
   if {$_fds == ""} {
      close_mytables fds
   }

   return $nobj
}

proc write_psfs {dir filetag field0 nfield {origin_field -1}} {
   global table
   if {$origin_field < 0} {
      set origin_field $field0;		# field with origin for rowc
   }
   
   set table [openit $field0]
   set fds [open_mytables -dir $dir table $filetag -ts]
   objfileClose table

   set nobj 0
   loop f $field0 [expr $field0 + $nfield] {
      set table [openit $f];
      incr nobj [write_mytable table sel_psf -fd fds \
		     -drow [expr ($f-$origin_field)*1361]]
      objfileClose table
   }
   
   close_mytables fds

   return $nobj
}

proc sel_psf {o {band 2}} {
   global OBJECT1

   if ![sel_star $o $band] {
      return 0
   }
   set flgs [exprGet $o.flags<$band>]

   if {$flgs & ($OBJECT1(BLENDED)|$OBJECT1(CHILD))} {
      return 0;
   }

   global table
   if {[keylget table fileSchema] == "TSOBJ"} {
      if {[exprGet $o.psfCounts<$band>] > 19} {
	 return 0
      }
   } else {
      if {[exprGet $o.psfCounts<$band>] < 1000} {
	 return 0
      }
   }

   set rowc [exprGet $o.objc_rowc]
   if {$rowc < 64 || $rowc >= 64 + 1361} {# in overlap region
      return 0;
   }

   return 1
}



# display nrow x ncol PSF mosaic for $filter by using data given 
# in psField*.fit $file
proc displayPSF {file filter {nrow 2} {ncol 3} {display_comp 1} {text ""}} {


    # read data from psPSF file
    if [catch {read_psfBasis psf_basis $file $filter $display_comp}] {
        declare_schematrans 5
	if [catch {read_psfBasis psf_basis $file $filter $display_comp} msg] {  
            echo "cannot read PSF_BASIS:"
            echo $msg
            fitsBinForgetSchemaTrans NULL           
            return -1        
        }
        fitsBinForgetSchemaTrans NULL        
    }


    # make mosaic and display it
    if [ catch {
             set mosaic [psf_grid $psf_basis($filter) $nrow $ncol 10]
             display $mosaic $text
             regDel $mosaic
         } msg] {
         echo "Unable to display PSF mosaic:"
         echo $msg
         return -1
    }

    # debug
    if {$display_comp > 1} {
       fake_setup
       check_psfBasis $psf_basis($filter) 1361 0 2 1
    }

    psfBasisDel $psf_basis($filter)

    return 1

}


# given a file with run-rerun-camCol-field-pixrow-pixcol specified in 
# 1-indexed columns Crun Crerun CcamCol Cfield Cpixrow Cpixcol, dump
# fits files with 51x51 images of PSF to files fileN.fit where is 
# N is the line number (lines beginning with # not counted)  
proc file2PSFfits {file Crun Crerun CcamCol Cfield Cpixrow Cpixcol \
                {N1 0} {N2 1000000} {outputDir .} {rootdir ""} {Nlinemax 1000000}} {

global data_root openit
 
       declare_schematrans 5

       if {$rootdir == ""} {
           sdr
       } else {
           set data_root $rootdir
       } 

       # open input file
       set inf [open $file r]
       set Nline 0; set Ndump 0 
  
       # loop over input file1
       while {![eof $inf] && $Nline < $Nlinemax} {
          set line [gets $inf]
          # process line
          if {![eof $inf]} {  
             if {[string range $line 0 0] == "#" || $line == ""} {  
                 continue
             }
             incr Nline 
	     if {$Nline >= $N1 && $Nline <= $N2} {
                 set run [lindex $line [expr $Crun-1]]
                 set rerun [lindex $line [expr $Crerun-1]]
                 set camCol [lindex $line [expr $CcamCol-1]]
                 set field [lindex $line [expr $Cfield-1]]
                 set pixrow [lindex $line [expr $Cpixrow-1]]
                 set pixcol [lindex $line [expr $Cpixcol-1]]

             #### specials
	     if {$run == 752 || $run == 756} {set rerun 7}	   
                 set psFieldOK 1
                 set runstr [format %06d $run] 
                 set fieldstr [format %04d $field]
                 set objdir $data_root/$run/$rerun/objcs/$camCol
                 set psField $objdir/psField-$runstr-$camCol-$fieldstr.fit
                 if {![file exist $psField]} {
                     echo "WARNING: can't find $psField (objdir=$objdir)"
                     set_run $run $camCol -root $data_root
                     if {![file exist $psField]} {
                         echo "ERROR: not even latest rerun has $psField"
                         exec cp dummy $outname
                         set psFieldOK 0
                     }
                 }
                 incr Ndump
                 # loop over filters
                 echo "dumping $Ndump ($run-$rerun-$camCol-$field)"
	         foreach f {u g r i z} {
                     set outname $file.$Nline.$f.fit
                     if {$psFieldOK} {                     
                        set reg [getPSFregion $psField $f $pixrow $pixcol]
                        regWriteAsFits $reg $outname
                        regDel $reg
		     } else {
                        exec cp dummy $outname
	             }		     
                 }                      
	     }
	  }
      }

      close $inf

      fitsBinForgetSchemaTrans NULL  

      echo dumped stamps for $Ndump positions to dir $outputDir 

    return 0
} 



# given a psField file, return a region with the PSF in 
# specified filter (u g r i z) and at a given position (row, col)
# before calling this proc execute:
# photo> declare_schematrans 5
# and after all is done:
# photo> fitsBinForgetSchemaTrans NULL     
proc getPSFregion {psField filter row col} {

       # read PSF data from psField file
       if [catch {read_psfBasis psfBasis $psField $filter 0} msg] {  
            echo "cannot read PSF_BASIS:"
            echo $msg
            echo "did you execute \"declare_schematrans 5 \" ?"
            return ""        
       }

       # PSF at (row, col)
       set psf_reg [psfKLReconstruct $psfBasis($filter) $row $col]
       set PSFreg [regNew \
           [exprGet (*$psf_reg.reg).nrow] [exprGet (*$psf_reg.reg).ncol]]
       regIntCopy $PSFreg *$psf_reg.reg

       # clean
       psfBasisDel $psfBasis($filter)
       psfRegDel $psf_reg

       return $PSFreg
}


# an example of useful procs for examining PSF, assumed that
# photo> set_display 1
# has been executed
proc demoPSF {{run 745} {camcol 1} {field 20} {filter r} \
	      {row 1000} {col 1800} {dataroot /data/dp3.b/data}} {

global data_root openit

       set data_root $dataroot
       

       # data paths
       set_run $run $camcol
       # this file contains the PSF data
       set runstr [format "%06d" $run]
       set fieldstr [format "%04d" $field]
       set psField $openit(objdir)/psField-$runstr-$camcol-$fieldstr.fit

       # read PSF data from psField file
       if [catch {read_psfBasis psfBasis $psField $filter 0}] {
           declare_schematrans 5
	   if [catch {read_psfBasis psfBasis $psField $filter 0} msg] {  
               echo "cannot read PSF_BASIS:"
               echo $msg
               fitsBinForgetSchemaTrans NULL           
               return -1        
           }
           fitsBinForgetSchemaTrans NULL        
       }


       # PSF at (row, col)
       set psf_reg [psfKLReconstruct $psfBasis($filter) $row $col]
       set PSFregion *$psf_reg.reg 
       display $PSFregion "this is a region with reconstructed PSF"

       # for extracting and fitting the PSF profile etc. we need to 
       # initialize a few things
       fake_setup


       # fit 2 Gaussians to the PSF
       set psf [KLpsf2dgpsf $psfBasis($filter) $row $col 0 0 1 0]
       echo " 2-Gaussian fit to the PSF (in pixels):"
       echo "                     sigma1 = [exprGet $psf.sigma1_2G]"
       echo "                     sigma2 = [exprGet $psf.sigma2_2G]"
       echo "                          b = [exprGet $psf.b_2G]"
       echo "   Effective width (arcsec) = [exprGet $psf.width]"


       # calculate aperture correction at the given position
       set apCor [psfBasis2apCorr $psfBasis($filter) $row $col]
       echo ""; echo " aperture correction at ($row,$col) is $apCor"; echo ""

      
       # display a mosaic of reconstructed PSFs for this frame
       displayPSF $psField $filter 6 9 0 

       # if fake_setup was called then
       finish_fake_setup 
       # clean up   
       psfBasisDel $psfBasis($filter)
       psfRegDel $psf_reg
       dgpsfDel $psf

}




# compare 2 PSFs, for long=0 bypass all fitting
# if file != "" do it just for that file
proc matchPSFs {run1 camcol1 field1 row1  col1 \
                run2 camcol2 field2 row2  col2 \
	 filter1 filter2 {long 1} {file ""}} {

global data_root openit

    # for extracting and fitting the PSF profile etc. we need to 
    # initialize a few things
    fake_setup

     if {$file != ""} {
        set epochs {1}
     } else {
        set epochs {1 2}
     }


     foreach epoch $epochs { 
       if {$epoch == 1} {
          set run $run1
          set camcol $camcol1
          set field $field1
          set filter $filter1
          set row $row1
          set col $col1
       } else {
          set run $run2
          set camcol $camcol2
          set field $field2
          set filter $filter2
          set row $row2
          set col $col2
       }

       if {$file == ""} {    
           # data path
           set_run $run $camcol
           # this file contains the PSF data
           set runstr [format "%06d" $run]
           set fieldstr [format "%04d" $field]
           set psField $openit(objdir)/psField-$runstr-$camcol-$fieldstr.fit
       } else {
           set psField $file
       }
       # read PSF data from psField file
       if {[info exists psfBasis($filter)]} {unset psfBasis($filter)}
       if [catch {read_psfBasis psfBasis $psField $filter 0}] {
           declare_schematrans 5
	   if [catch {read_psfBasis psfBasis $psField $filter 0} msg] {  
               echo "cannot read PSF_BASIS:"
               echo $msg
               fitsBinForgetSchemaTrans NULL           
               return -1        
           }
           fitsBinForgetSchemaTrans NULL        
       }
       # PSF at (row, col)
       set psf_reg [psfKLReconstruct $psfBasis($filter) $row $col]
       set PSFregion${epoch} [regNewFromReg *$psf_reg.reg] 
       set PSFregion *$psf_reg.reg 
       if {$long} {
          display $PSFregion "this is the first region with reconstructed PSF"
          # fit 2 Gaussians to the PSF
          set psf [KLpsf2dgpsf $psfBasis($filter) $row $col 0 0 1 0]
          echo " 2-Gaussian fit to the PSF (in pixels):"
          echo "                     sigma1 = [exprGet $psf.sigma1_2G]"
          echo "                     sigma2 = [exprGet $psf.sigma2_2G]"
          echo "                          b = [exprGet $psf.b_2G]"
          echo "   Effective width (arcsec) = [exprGet $psf.width]"

          # calculate aperture correction at the given position
          set apCor [psfBasis2apCorr $psfBasis($filter) $row $col]
          echo ""; echo " aperture correction at ($row,$col) is $apCor"; echo ""
  
          # display a mosaic of reconstructed PSFs for this frame
          displayPSF $psField $filter 6 9 0 
          dgpsfDel $psf
      }
      psfBasisDel $psfBasis($filter)
      psfRegDel $psf_reg
     }

     if {[llength $epochs] == 2} {        
        # subtract two regions and display the result
        set fac [expr 10000.0/30000]
        regIntLincom $PSFregion2 $PSFregion1 10000 $fac -$fac
        display $PSFregion2 "PSF2 - PSF1 + 10000"
        set stats [regStatsFind $PSFregion2]
        set max [format "%5.2f" [expr ([keylget stats high]-10000.0)/100]]
        set min [format "%5.2f" [expr ([keylget stats low]-10000.0)/100]] 
        echo " PSF difference ranges from $min to $max percent"
        regDel $PSFregion2 
     }
     regDel $PSFregion1 

     # since fake_setup was called then
     finish_fake_setup 
    
 
}


##########################################################################
# for testing psField files

### for comparing different reductions
# given a psField files reconstruct a grid of PSFs for each
# file and subtract them. Return this region.
# Also print out the numerical values from calib1byframe
proc test_psField {{psField ""} {filter r} {row 680} {col 1024} \
		   {nrow 6} {ncol 9} {long 1}} { 


    set testfile [envscan \$PHOTO_DIR]/test/psField-test.tcl         
    if {$psField == ""} {set psField $testfile}      

    # for extracting and fitting the PSF profile etc. we need to 
    # initialize a few things
    fake_setup

    # read PSF data from psField file
    if {[info exists psfBasis($filter)]} {unset psfBasis($filter)}
    if [catch {read_psfBasis psfBasis $psField $filter 0}] {
        declare_schematrans 5
	if [catch {read_psfBasis psfBasis $psField $filter 0} msg] {  
            echo "cannot read PSF_BASIS:"
            echo $msg
            fitsBinForgetSchemaTrans NULL           
            return -1        
        }
        fitsBinForgetSchemaTrans NULL        
    }

     
    # date read in, now get PSF at (row, col)
    set psf_reg [psfKLReconstruct $psfBasis($filter) $row $col]
    set PSFregion [regNewFromReg *$psf_reg.reg] 
    if {$long} {
       display $PSFregion "this is the first region with reconstructed PSF"
       # fit 2 Gaussians to the PSF
       set psf [KLpsf2dgpsf $psfBasis($filter) $row $col 0 0 1 0]
       echo " 2-Gaussian fit to the PSF (in pixels):"
	echo "                     sigma1 = [exprGet $psf.sigma1_2G]"
       echo "                     sigma2 = [exprGet $psf.sigma2_2G]"
       echo "                          b = [exprGet $psf.b_2G]"
       echo "   Effective width (arcsec) = [exprGet $psf.width]"

       if {$long > 1} {
          # calculate aperture correction at the given position
          set apCor [psfBasis2apCorr $psfBasis($filter) $row $col]
          echo ""; echo " aperture correction at ($row,$col) is $apCor"; echo ""
       }
  
       # display a mosaic of reconstructed PSFs for this frame
       displayPSF $psField $filter $nrow $ncol 0 
       dgpsfDel $psf
    }
    psfBasisDel $psfBasis($filter)
    psfRegDel $psf_reg
    regDel $PSFregion



    set type 1
    set calib [get_calib1byframe_from_psField $psField $filter]
    set string [string_from_calib1 $calib 0 $type] 
    calib1byframeDel $calib 
    echo "       PSP status    n   PSFwidth s1_2G  s2_2G  b_2G   \
                  s1     s2     b     p0    sp  beta   N*    CPcnts"
    echo "$string"


    # since fake_setup was called then
    finish_fake_setup 
    
}


#### QA tools for KL stuff ####


### dump a chain of psfRegs: (regName rowc colc counts countsErr) 
### are stored in $outfile, and each region specified by regName
### is written as a separate fits file
proc dump_psfRegChain {chain outname regNameRoot} {


     set outf [open $outname w]
     
     loop i 0 [chainSize $chain] {
        set psf [chainElementGetByPos $chain $i]
	set regName ${regNameRoot}_$i.fit
        set line "$regName" 
        foreach rec {rowc colc counts countsErr} {
           set line "$line [exprGet $psf.$rec]"
        }
        puts $outf $line
        regWriteAsFits *$psf.reg $regName
     }
  
    close $outf

    echo "dumped [chainSize $chain] entries to file $outname"

}


## read a chain of psfRegs dumped by dump_psfRegChain
proc read_psfRegChain {filename} {


     set infile [open $filename r]
     set chain [chainNew PSF_REG]

     while {![eof $infile]} {
        set line [gets $infile]
        if {![eof $infile] && [string range $line 0 0] != "#"} {
            set regName [lindex $line 0]
            set rowc [lindex $line 1]; set colc [lindex $line 2]
            set counts [lindex $line 3]; set countsErr [lindex $line 4]
            set reg [regReadAsFits [regNew] $regName]
            set psfReg [psfRegNew $reg $rowc $colc $counts $countsErr]
            chainElementAddByPos $chain $psfReg TAIL
        }
     }

     close $infile

     echo "read [chainSize $chain] psfRegs"

    return $chain

}



proc getAllKL {fileB fileC} {

    declare_schematrans 5
    set basis_regs [read_psfRegChain $fileB]
    set coeffs_regs [read_psfRegChain $fileC]       
    set psfBasis [psfKLDecomp -border 10 $basis_regs 4 3 -ncol_b 3]     
  
    psfKernelSet $coeffs_regs $psfBasis *$psfBasis.kern
    set grid [psf_grid $psfBasis 12 18]

    set reconMosaic [testKLrecon $coeffs_regs $psfBasis]

  return [list $basis_regs $coeffs_regs $psfBasis $grid $reconMosaic]

}


proc psField2mosaic {file filter {nrow 12} {ncol 18} {display_comp 1}} {

    # read data from psPSF file
    if [catch {read_psfBasis psf_basis $file $filter $display_comp}] {
        declare_schematrans 5
	if [catch {read_psfBasis psf_basis $file $filter $display_comp} msg] {  
            echo "cannot read PSF_BASIS:"
            echo $msg
            fitsBinForgetSchemaTrans NULL           
            return -1        
        }
        fitsBinForgetSchemaTrans NULL        
    }


    # make mosaic and display it
    if [ catch {
             set mosaic [psf_grid $psf_basis($filter) $nrow $ncol]            
         } msg] {
         echo $msg
         return ""
    }

    psfBasisDel $psf_basis($filter)

    return $mosaic

}




### given a chain of coeffs regs, and corresponding psfBasis, 
### return mosaic of reconstructed PSFs and difference images
proc testKLrecon {coeffsChain psfBasis {verbose 0}} {

    set Ngood [chainSize $coeffsChain]
    set SOFT_BIAS [softBiasGet]
    set border 10 

         if {$verbose > 0} { 
	    echo "starid rowc   colc    peakD   peakRes   peakRat(%) fluxRat(%) fErr(%)  chi2" 
	    echo "-----------------------------------------------------------------------------"  
         }
 
	 # calculate PSF at position of each star, and make a mosaic
	 # of reconstructed PSFs and original stellar profiles
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
         set mosaicSet 0

	 # loop over all stars
	 loop id2 0 $Ngood {
	    set psfReg2 [chainElementGetByPos $coeffsChain $id2]
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
	    
   	    # region for mosaic
            if {!$mosaicSet} {
               set PSFmosaic [regNew -type FL32 [expr $nrow*$Nrow] [expr int(1.1*$ncol*$Ncol)]]
               regClear $PSFmosaic
               set mosaicSet 1
            }
	    # estimate PSF at reg2's position
	    set treg2 [psfKLReconstruct $psfBasis $rowc $colc]

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
	    
	    if {$verbose > 0} {
	      # some qa info            
	      set peak_rat [format "%6.2f" [expr 100.0*$respeak/$datapeak]]
	      set flux_rat [format "%6.3f" [expr 100.0*$resflux/$dataflux]]
	      set fErr     [format "%6.2f" [expr 100.0*$datacountsErr/$datacounts]]
	      set chi2 [format "%6.2f" [expr $flux_rat/$fErr]]
	      set str1 "[format "%5d %6.0f %6.0f" $id2 $rowc $colc]"
	      set str2 "[format "%8.0f" $datapeak]  [format "%6.0f" $respeak]"
	      echo "$str1 $str2      $peak_rat     $flux_rat   $fErr  $chi2 $datacounts"
	    }

	    if {[incr i] == $Ncol} {
	       incr j [expr $npanel*$nrow]
	       set i 0
	    }
	    
	    regDel $datareg; regDel $modelreg  
	    handleDel $reg2; psfRegDel $treg2
	   # psfRegDel $psfReg2

           #echo " after star $id2"
           #display $PSFmosaic
	 }
	 
         if {$verbose > 1} {
	     display $PSFmosaic "comparison with reconstructed PSFs"
         }
	 
   return $PSFmosaic
      
}
