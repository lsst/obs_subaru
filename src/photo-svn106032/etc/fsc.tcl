#
# Fake Stampcollecting Pipeline.
#
# Some routines in this file are adapted from procs in the SSC, to which thanks
#
# Needed typedefs; it's a shame to have to have of all these types here, but...
#
typedef enum { \
   DRKCUR = 0, \
   HOTCOL, \
   DEPCOL, \
   CTECOL, \
   BLKCOL, \
   BADBLK, \
   HOTSPOT, \
   NOISY, \
   TRAP \
} DFTYPE;

typedef struct {
   char program[40];
   int camRow;
   int camCol;
   int rowBinning;
   int colBinning;
   int amp0;
   int amp1;
   int amp2;
   int amp3;
   int nrows;
   int ncols;
   int sPreBias0;
   int nPreBias0;
   int sPostBias0;
   int nPostBias0;
   int sOverScan0;
   int nOverScan0;
   int sMapOverScan0;
   int nMapOverScan0;
   int sOverScanRows0;
   int nOverScanRows0;
   int sDataSec0;
   int nDataSec0;
   int sDataRow0;
   int nDataRow0;
   int sCCDRowSec0;
   int sCCDColSec0;
   int sPreBias1;
   int nPreBias1;
   int sPostBias1;
   int nPostBias1;
   int sOverScan1;
   int nOverScan1;
   int sMapOverScan1;
   int nMapOverScan1;
   int sOverScanRows1;
   int nOverScanRows1;
   int sDataSec1;
   int nDataSec1;
   int sDataRow1;
   int nDataRow1;
   int sCCDRowSec1;
   int sCCDColSec1;
   int sPreBias2;
   int nPreBias2;
   int sPostBias2;
   int nPostBias2;
   int sOverScan2;
   int nOverScan2;
   int sMapOverScan2;
   int nMapOverScan2;
   int sOverScanRows2;
   int nOverScanRows2;
   int sDataSec2;
   int nDataSec2;
   int sDataRow2;
   int nDataRow2;
   int sCCDRowSec2;
   int sCCDColSec2;
   int sPreBias3;
   int nPreBias3;
   int sPostBias3;
   int nPostBias3;
   int sOverScan3;
   int nOverScan3;
   int sMapOverScan3;
   int nMapOverScan3;
   int sOverScanRows3;
   int nOverScanRows3;
   int sDataSec3;
   int nDataSec3;
   int sDataRow3;
   int nDataRow3;
   int sCCDRowSec3;
   int sCCDColSec3;
   int sPreBias0good;
   int nPreBias0good;
   int sPostBias0good;
   int nPostBias0good;
   int sOverScan0good;
   int nOverScan0good;
   int sMapOverScan0good;
   int nMapOverScan0good;
   int sOverScanRows0good;
   int nOverScanRows0good;
   int sDataSec0good;
   int nDataSec0good;
   int sDataRow0good;
   int nDataRow0good;
   int sCCDRowSec0good;
   int sCCDColSec0good;
   int sPreBias1good;
   int nPreBias1good;
   int sPostBias1good;
   int nPostBias1good;
   int sOverScan1good;
   int nOverScan1good;
   int sMapOverScan1good;
   int nMapOverScan1good;
   int sOverScanRows1good;
   int nOverScanRows1good;
   int sDataSec1good;
   int nDataSec1good;
   int sDataRow1good;
   int nDataRow1good;
   int sCCDRowSec1good;
   int sCCDColSec1good;
   int sPreBias2good;
   int nPreBias2good;
   int sPostBias2good;
   int nPostBias2good;
   int sOverScan2good;
   int nOverScan2good;
   int sMapOverScan2good;
   int nMapOverScan2good;
   int sOverScanRows2good;
   int nOverScanRows2good;
   int sDataSec2good;
   int nDataSec2good;
   int sDataRow2good;
   int nDataRow2good;
   int sCCDRowSec2good;
   int sCCDColSec2good;
   int sPreBias3good;
   int nPreBias3good;
   int sPostBias3good;
   int nPostBias3good;
   int sOverScan3good;
   int nOverScan3good;
   int sMapOverScan3good;
   int nMapOverScan3good;
   int sOverScanRows3good;
   int nOverScanRows3good;
   int sDataSec3good;
   int nDataSec3good;
   int sDataRow3good;
   int nDataRow3good;
   int sCCDRowSec3good;
   int sCCDColSec3good;
} CCDCONFIG;

typedef struct {
   char program[40];
   int camRow;
   int camCol;
   int dfcol0;
   int dfncol;
   int dfrow0;
   int dfnrow;
   DFTYPE dftype;
   int astroline;
} CCDBC;

typedef struct {
   int dewarID;
   int camRow;
   int camCol;
   double rowRef;
   double colRef;
   double xc;
   double yc;
   double theta;
   double sfactc;
   double pscale;
   double xmag;
   int rowOffset;
   int frameOffset;
   double dRow0;
   double dRow1;
   double dRow2;
   double dRow3;
   double dCol0;
   double dCol1;
   double dCol2;
   double dCol3;
} CCDGEOMETRY;

typedef struct {
   char program[40];
   
   int camRow;
   int camCol;
   
   float readNoiseDN0;
   float fullWellDN0;
   float gain0;
   float biasLevel0;
   float DN0[13];
   float linearity0[13];

   float readNoiseDN1;
   float fullWellDN1;
   float gain1;
   float biasLevel1;
   float DN1[13];
   float linearity1[13];
   
   float readNoiseDN2;
   float fullWellDN2;
   float gain2;
   float biasLevel2;
   float DN2[13];
   float linearity2[13];
   
   float readNoiseDN3;
   float fullWellDN3;
   float gain3;
   float biasLevel3;
   float DN3[13];
   float linearity3[13];
} ECALIB;

typedef struct {
   int q1;
   int q2;
   int q3;
   int flatVal;
} SCQUARTF;

typedef struct {
   int frame;
   double mjd;
   double ra;
   double dec;
   float spa;
   float ipa;
   float ipaRate;
   double az;
   double alt;
} LOG_FRAMEINFO;
#
# From ssc/include/scStarcalc.h:
#
typedef struct scparambs {
   int midRow;                /* Frame row of the stamp's middle pixel. */
   int midCol;                /* Frame column of the stamp's middle pixel. */
   float rowCentroid;         /* Row position of star center. */
   float rowCentroidErr;      /* Error in row position of star center */
   float colCentroid;         /* Column position of star center. */
   float colCentroidErr;      /* Error in column position of star center */
   float rMajor;              /* The major axis radius. */
   float rMinor;              /* The minor axis radius. */
   float angMajorAxis;        /* The major axis position angle. */
   float peak;                /* Peak of the gaussian fit */
   float counts;              /* Sum of counts above sky */
   float color;               /* = -2.5 log[counts2/counts1], */
                              /* quantum efficiency and gain NOT taken into acco
unt */
                              /* depending on filter, SCPARAMBS.color is */
                              /* u: u-g, g: g-r, all other: r-i */
   float colorErr;            /* estimated color error; when either of 2 counts 
*/
                              /* is missing: color = -99.9, colorErr = -1.0 */
   int statusFit;             /* 0=ok, less than 0 implies error in calculating 
centroid */
   short sscStatus;           /* indicates results of size and intensity tests *
/
                              /* the meaning of the bits is defined in scParam.p
ar */
} SCPARAMBS;

###############################################################################
#
# Given pstamps in a number of bands, make sure that the same objects appear
# in each band.
#
# More precisely, we find the list of stars that are in the canonical
# band ($cc) and one other band (chosen from g, i, u, z, other in that order)
# and ensure that those (and only those) stars are used in each band.
#
proc complete_pstamps {filterlist cc _reg _pstamps _dr _dc {verbose 0}} {
   upvar $_reg reg  $_pstamps pstamps  $_dr dr  $_dc dc

   if {[llength $filterlist] <= 1} {
      return;				# nothing to do
   }
   
   assert {[lsearch $filterlist $cc] >= 0}
   foreach f "r g i u z" {
      if {$f != $cc && [lsearch $filterlist $cc] >= 0} {
	 set cc2 $f
	 break
      }
   }
   if ![info exists cc2] {
      foreach f "r g i u z" {
	 if {$f != $cc} {
	    set cc2 $f
	    break
	 }
      }
   }
   #
   # Add anything in the pstamps($cc2) that isn't in pstamps($cc) to the latter
   #
   set d 2;				# matching radius
   loop i 0 [chainSize $pstamps($cc2)] {
      set o [chainElementGetByPos $pstamps($cc2) $i]
      set orowc [exprGet $o.rowc]; set ocolc [exprGet $o.colc]

      set rowc_cc [expr $orowc - $dr($cc2)];
      set colc_cc [expr $ocolc - $dc($cc2)]
      if ![catch {
	 set iobj [infofileMatch $rowc_cc $colc_cc $d]
      } msg] {
	 handleDel $iobj
      } else {
	 # not in $cc chain
	 set stamp [stampCut $reg($cc) $rowc_cc $colc_cc $verbose]
	 if {$stamp == ""} {
	    set stamp [stampNew]; regClear *$stamp.reg
	 }
	 chainElementAddByPos $pstamps($cc) $stamp; handleDel $stamp
      }
      handleDel $o
   }
   #
   # Now cut new stamp chains for all the bands except $cc.
   #
   # First destroy the old ones
   #
   foreach f $filterlist {
      if {$f == $cc} {
	 continue
      }
      
      stampChainDel $pstamps($f)
      set pstamps($f) [chainNew SCSTAMP]
   }
   #
   # ... and rebuild them
   #
   loop i 0 [chainSize $pstamps($cc)] {
      set o [chainElementGetByPos $pstamps($cc) $i]

      set orowc [exprGet $o.rowc]; set ocolc [exprGet $o.colc]

      foreach f $filterlist {
	 if {$f == $cc} {
	    continue
	 }

	 set rowc [expr $orowc + $dr($f)];
	 set colc [expr $ocolc + $dc($f)]

	 set stamp [stampCut $reg($f) $rowc $colc $verbose]
	 if {$stamp == ""} {
	    set stamp [stampNew]; regClear *$stamp.reg
	 }
	 chainElementAddByPos $pstamps($f) $stamp; handleDel $stamp
      }

      handleDel $o
   }
}

###############################################################################
#
# write output to a FITS binary table ("Fang" file). For definition, see
#
#    http://www-sdss.fnal.gov:8000/dm/flatFiles/scFang.html
#
proc write_fang_file { args } {
   global table verbose id_values
   
   set opts [list \
		 [list [info level 0] ""] \
		 [list <run> STRING 0 run "Run number"] \
		 [list <camCol> INTEGER 0 camCol "Camera column"] \
		 [list <field> INTEGER 0 field "Field number"] \
		 [list <filterlist> STRING "" filterlist "Desired filters"] \
		 [list <outputDir> STRING "" outputDir "Output directory"] \
		 [list -pstamps STRING "" _pstamps \
		      "Array of chains of postage stamps, indexed by \$filterlist"] \
		 [list -ncol STRING "" _ncol \
		      "Array of region ncol, indexed by \$filterlist"] \
		 [list -skylev STRING "" _skyarr \
		      "Array of sky levels, indexed by \$filterlist"] \
		 [list -skysig STRING "" _skysigarr \
		      "Array of sky sigma, indexed by \$filterlist"] \
		 [list -params STRING "" _params \
		    "Array of chains of parameters, indexed by \$filterlist"] \
		 [list -headers STRING "" _headers \
		    "Array of HDRs, indexed by e.g. params,\$filterlist"] \
		 [list -prefix STRING "sc" fangprefix \
		    "Prefix for output filename"] \
		 [list -ko_ver STRING "FSC" ko_ver \
		      "Version of known object catalog used"] \
		 [list -ssc_id STRING "photo" ssc_id \
		      "ID from SSC (if used as source of parts of file)"] \
		]
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if {$_headers != ""} {
      upvar $_headers headers
   }
   #
   # figure out the name of the file
   #
   if {$run != "0"} {
      regsub {^0*} $run "" run;		# deal with e.g. 000745
   }
   set fname [format "%s/${fangprefix}Fang-%06d-%1d-%04d.fit" \
		  $outputDir $run $camCol $field]
   
   #
   # What's going in the file?
   #
   set hdusets ""
   if {$_pstamps != ""} {
      lappend hdusets "stamps"
   }
   if {$_ncol != ""} {
      lappend hdusets "quarts"
   }
   if {$_params != ""} {
      lappend hdusets "params"
   }
   #
   # first comes the primary header-data unit
   #
   set hdr [make_primary_header $run $camCol $field $filterlist $hdusets]

   hdrInsWithAscii $hdr SSC_ID $ssc_id 
   hdrInsWithAscii $hdr PS_ID $id_values(PS_ID)
   hdrInsWithAscii $hdr VERSION $id_values(VERSION)
   hdrInsWithAscii $hdr KO_VER $ko_ver

   hdrWriteAsFits $hdr $fname
   hdrDel $hdr
   # 
   # Make the photometric postage stamp HDUs
   #
   # these are postage stamp IMAGES
   #
   if {$_pstamps != ""} {
      upvar $_pstamps pstamps
      foreach filter $filterlist {
	 if [info exists headers(pstamps,$filter)] {
	    set hdr $headers(pstamps,$filter)
	 } else {
	    set hdr [make_stamp_header $camCol [camRowGet $filter] $filter]
	 }
	 set fschain [stampChainToFstampChain $pstamps($filter)]
	 chainWriteAsFits $fschain SCFSTAMP $hdr $fname 1 
	 fstampChainDel $fschain

	 if ![info exists headers(pstamps,$filter)] {
	    hdrDel $hdr
	 }
      }
   }
   #
   # Now for the quartiles.
   #
   if {$_ncol != ""} {
      upvar $_ncol ncol
      assert {$_skyarr != "" && $_skysigarr != ""}
      upvar $_skyarr skyarr  $_skysigarr skysigarr

      set tshift 64
      foreach filter $filterlist {
	 if [info exists headers(quarts,$filter)] {
	    set hdr $headers(quarts,$filter)
	 } else {
	    set hdr \
		[make_quart_header $camCol [camRowGet $filter] $filter $tshift]
	 }
	 
	 set sky $skyarr($filter)
	 if {$sky == 0} {		# this confuses PSP, so set it to 1
	    set sky 1
	 }
	 
	 set siqr [expr $skysigarr($filter)/0.741]
	 
	 set q [quartNew]
	 handleSet $q.q1 [expr ($sky - $siqr)*$tshift]
	 handleSet $q.q2 [expr ($sky)*$tshift]
	 handleSet $q.q3 [expr ($sky + $siqr)*$tshift]
      
	 set qchain [chainNew SCQUARTF]
	 
	 loop i 0 $ncol($filter) {
	    chainElementAddByPos $qchain $q
	 }
	 
	 chainWriteAsFits $qchain SCQUARTF $hdr $fname 1 
	     
	 quartDel $q
	 chainDel $qchain;	# NOT destroy! It doesn't own the SCQUARTs

	 if ![info exists headers(quarts,$filter)] {
	    hdrDel $hdr;
	 }
      }
   }
   #
   # And the star parameter HDUs
   #
   if {$_params != ""} {
      upvar $_params params

      array set field2frame [list r  0  i  2   u  4   z  6   g  8]
      array set field2frame [list l -2  t 10]

      set if -1
      foreach filter $filterlist {
	 incr if
	 
	 verb_echo 3 \
	     Writing $fname PARAMS,$filter: [chainSize $params($filter)] stars

	 set frame [expr $field + $field2frame($filter)]

	 if [info exists headers(params,$filter)] {
	    set hdr $headers(params,$filter)
	 } else {
	    set hdr [make_params_header \
			 $camCol [camRowGet $filter] $frame $filter]
	 }

	 chainWriteAsFits $params($filter) SCPARAMBS $hdr $fname 1

	 if ![info exists headers(params,$filter)] {
	    hdrDel $hdr
	 }
      }
   }
   
   return 0;
}

###############################################################################
#
# Create a primary header for the "Fang" FITS binary table,
# and return a handle to the header.
#
proc make_primary_header { run camCol field filterlist hdusets} {
   set hdr [hdrNew]; hdrInsertLine $hdr 1 "END"
   
   hdrInsWithLogical $hdr SIMPLE T
   hdrInsWithInt $hdr BITPIX 8
   hdrInsWithInt $hdr NAXIS  0
   hdrInsWithLogical $hdr EXTEND T
   hdrInsWithInt $hdr RUN $run "Imaging Run Number"
   hdrInsWithInt $hdr CAMCOL $camCol "Column in imaging camera"
   hdrInsWithInt $hdr FIELD $field "Field sequence number within the run"

   hdrInsWithAscii $hdr HDUSETS $hdusets "Order of HDU sets in the file"

   if {[lsearch $hdusets "stamps"] >= 0} {
      set sfilters $filterlist
   } else {
      set sfilters ""
   }

   if {[lsearch $hdusets "params"] >= 0} {
      set pfilters $filterlist
   } else {
      set pfilters ""
   }

   if {[lsearch $hdusets "quarts"] >= 0} {
      set qfilters $filterlist
   } else {
      set qfilters ""
   }

   hdrInsWithAscii $hdr SFILTERS $sfilters \
       "Filters processed for postage stamps"
   hdrInsWithAscii $hdr PFILTERS $pfilters \
       "Filters processed for bright star params"
   hdrInsWithAscii $hdr QFILTERS $qfilters \
       "Filters processed for quartile/flats"

   return $hdr
}

###############################################################################
#
# Create a secondary FITS header for the "Fang" FITS binary table,
# one that appears just before an HDU full of photometric postage stamps.
#
proc make_stamp_header { camCol row filter } {
   set hdr [hdrNew]
   hdrInsWithAscii $hdr FILTER $filter
   hdrInsWithInt $hdr CAMROW $row "Row in imaging camera"
   hdrInsWithAscii $hdr EXTNAME [format "STAMP %d%d" $row $camCol]
   
   # get the size of a postage stamp by looking at a new stamp
   set stamp [stampNew]
   set size [exprGet $stamp.reg->nrow]
   stampDel $stamp
   hdrInsWithInt $hdr PSSIZE $size "Postage Stamp Size"
   
   hdrInsWithInt $hdr TSHIFT  0 "Bias scaling factor"
   hdrInsWithInt $hdr FRBIASL 0 "Left  Frame bias x TSHIFT"
   hdrInsWithInt $hdr FRBIASR 0 "Right Frame bias x TSHIFT"
   
   # we read the "overscan" values directly from the Gang file quartiles
   #   HDU header
   hdrInsWithInt $hdr FROSCNL 0 "Left Mean Oscn Value x TSHIFT"
   hdrInsWithInt $hdr FROSCNR 0 "Right Mean Oscn Value x TSHIFT"
   
   # these two keywords allow the FITS reader to turn the signed int
   # data (which is the form stored in the FITS file) into unsigned
   # integer values upon reading; those U16 values can be stuffed
   # into U16 variables by the reading process.
   set tscal 1
   set tzero 32768
   hdrInsWithInt $hdr TSCAL1  $tscal  "Scaling factor when reading values"
   hdrInsWithInt $hdr TZERO1  $tzero  "  val = TSCAL1*(x+TZERO1) "
   
   return $hdr
}

###############################################################################
#
# Create a secondary FITS header for the "Fang" FITS binary table,
# with star parameters
#
proc make_params_header { camCol row frame filter } {
   global idFrameLog

   set hdr [hdrNew]
   hdrInsWithAscii $hdr FILTER $filter
   hdrInsWithInt $hdr CAMROW $row "Row in imaging camera"
   hdrInsWithAscii $hdr EXTNAME [format "STAR %d%d" $row $camCol]
   hdrInsWithInt $hdr FRAME $frame
   #
   # Get remaining fields from idFrameLog
   #
   set matches [chainSearch $idFrameLog "{frame == $frame}"]
   if {[chainSize $matches] != 1} {
      error "Cannot find frame $frame in idFrameLog"
   }
   set frameInfo [chainElementGetByPos $matches 0]
   chainDel $matches
   
   foreach el "tai spa dec ra" {
      if {$el == "tai"} {
	 set val [expr 86400.0*[exprGet $frameInfo.mjd]]
      } else {
	 set val [exprGet $frameInfo.$el]
      }
      hdrInsWithDbl $hdr [string toupper $el] $val
   }

   handleDel $frameInfo

   return $hdr
}

###############################################################################
#
# Write an scWing file
#
proc write_wing_file { args } {
   global table verbose id_values
   
   set opts [list \
		 [list [info level 0] ""] \
		 [list <run> STRING 0 run "Run number"] \
		 [list <camCol> INTEGER 0 camCol "Camera column"] \
		 [list <field> INTEGER 0 field "Field number"] \
		 [list <filterlist> STRING "" filterlist "Desired filters"] \
		 [list <outputDir> STRING "" outputDir "Output directory"] \
		 [list <wingRegs> STRING "" _wingRegs \
		    "Array of regions, indexed by \$filterlist"] \
		 [list -ko_ver STRING "FSC" ko_ver \
		      "Version of known object catalog used"] \
		 [list -ssc_id STRING "photo" ssc_id \
		      "ID from SSC (if used as source of parts of file)"] \
		]
   
   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_wingRegs wingRegs

   if {$run != "0"} {
      regsub {^0*} $run "" run;		# deal with e.g. 000745
   }

   foreach f $filterlist {
      set fname [format "%s/scWing-%06d-%s%d-%04d.fit" \
		     $outputDir $run $f $camCol $field]
      
      regWriteAsFits $wingRegs($f) $fname
   }
   #
   # Now write the .par file
   #
   set fname [format "%s/scWing-%06d-%d.par" $outputDir $run $camCol]
   if ![file exists $fname] {
      set fd [open $fname "w"]
      puts $fd \
"run       $run            # Imaging run number
camCol    $camCol            # Column in imaging camera
version [photoVersion]             # Version number returned by versionStamp
ssc_id \"$ssc_id\"       # ID returned by idStamp
ko_ver \"$ko_ver\"       # KO_VER keyword from koCat file
 
typedef struct {         
   int field;            
   char filter\[20\];    
   int row;              
   int col;              
   float mag;            
} SCWING            
                         
"
      close $fd
   }

   set fd [open $fname "a"]
   foreach f $filterlist {
      set rowc [hdrGetAsDbl $wingRegs($f).hdr ROWC]
      set colc [hdrGetAsDbl $wingRegs($f).hdr COLC]

      puts $fd "SCWING  $field  $f   $rowc $colc 10"
   }
   close $fd
}

###############################################################################
#
# Deal with quartile arrays
#
proc quartNew {} {
   set q [genericNew SCQUARTF]
   
   foreach el "q1 q2 q3 flatVal" {
      handleSet $q.$el 0
   }

   return $q
}

proc quartDel {q} {
   genericDel $q
}

proc make_quart_header { camCol row filter tshift} {
   set hdr [hdrNew]
   
   hdrInsWithAscii $hdr EXTNAME "QFLAT ??"
   hdrInsWithAscii $hdr FILTER $filter
   hdrInsWithInt $hdr CAMROW $row "Row in imaging camera"
   hdrInsWithInt $hdr TSHIFT $tshift
   
   return $hdr
}

##############################################################################
#
# Given a CHAIN of stuctures of type $type, and a header, write the
# structures to a FITS binary table, placing the header info into
# the (secondary) header.
#
ftclHelpDefine ssc chainWriteAsFits \
    "
     Write a chain of structures and a header to a FITS binary table with 
     the given name.  If the 'append' argument is 1, then append to 
     the FITS file.  Otherwise, create a new one.

     USAGE: chainWriteAsFits chain type header filename append
    "

proc chainWriteAsFits { chain type header fname append } {
   set xsch [schemaTransAuto $type]
   set tbl [schemaToTbl $chain $xsch -schemaName $type]

   hdrCopy $header $tbl.hdr
   if { $append == 1 } {
      fitsWrite $tbl $fname -binary -append -standard 0
   } else {
      fitsWrite $tbl $fname -binary -pdu MINIMAL -standard 0 
   }

   schemaTransDel $xsch
   handleDelFromType $tbl
   
   return 0
}

###############################################################################
#
# Given a region, return a chain of SCSTAMPs
#
proc make_scstamp_chain {args} {
   set disp 0; set mask 0
   
   set opts [list \
		 [list [info level 0] ""] \
		 [list <reg> STRING "" reg "Region to search"] \
		 [list <thresh> DOUBLE "" threshold \
		      "Object detection threshold"] \
		 [list {[filter]} STRING "" filter \
		      "Name of filter being processed"] \
		 [list -satur INTEGER 0 satur "Saturation level (0 => none)"] \
		 [list -mask CONSTANT 1 mask \
 "Set saturated bits in <reg>'s SPANMASK (which will be created if necessary)"] \
		 [list -wingRegs STRING "" _wingRegs \
		      "Array of wingstar regions"] \
		 [list -display CONSTANT 1 disp "Display objects?"] \
		 [list -verbose INTEGER 0 verbose "How verbose should I be?"] \
		]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   initObjectFinder

   if {$satur == 0} {
      if $mask {
	 error "Please specify -satur along with -mask"
      }
      if {$_wingRegs != ""} {
	 error "Please specify -satur along with -wingRegs"
      }
   } 
   #
   # Set the SATUR bits in the region?
   #
   if {$mask || $_wingRegs != ""} {
      global MASK_TYPE

      set thresh [vectorExprEval "($satur)"]
      set saturchain [regFinder $reg $thresh]

      if $mask {
	 if {[exprGet (int)$reg.mask] == 0x0} {
	    set sm [spanmaskNew -nrow [exprGet $reg.nrow] \
			-ncol [exprGet $reg.ncol]]
	    handleSetFromHandle $reg.mask (MASK*)&$sm; handleDel $sm
	 }

	 spanmaskSetFromObjectChain $saturchain *$reg.mask $MASK_TYPE(SATUR) 0
      }

      if {$_wingRegs != ""} {
	 upvar $_wingRegs wingRegs
	 if [chainSize $saturchain] {
	    loop i 0 [chainSize $saturchain] {
	       set o [chainElementGetByPos $saturchain $i]
	       foreach rc "rowc colc" {
		  set $rc [exprGet $o.peaks->peaks<0>->$rc]
	       }
	       handleDel $o

	       if [catch {set sreg [subRegNew $reg 200 200 \
					[expr int($rowc + 0.5 - 100)] \
					[expr int($colc + 0.5 - 100)]]}] {
		  verb_echo 2 \
		      [format "Wing star at %.1f, %.1f is too close to edge" \
			   $rowc $colc]
	       } else {
		  set wingRegs($filter) [regNew 200 200]
		  hdrInsWithDbl $wingRegs($filter).hdr ROWC $rowc
		  hdrInsWithDbl $wingRegs($filter).hdr COLC $colc
		  regIntCopy $wingRegs($filter) $sreg
		  regDel $sreg

		  break;
	       }
	    }
	 }
      }

      objectChainDel $saturchain
      vectorExprDel $thresh
   }
   #
   # Find objects
   #
   set thresh [vectorExprEval "($threshold)"]
   set objs [regFinder $reg $thresh]

   if $verbose {
      if {$filter != ""} {
	 puts -nonewline "Filter $filter "
      }
      puts -nonewline "found [chainSize $objs] objects"
   }
   #
   # Show what we found?
   #
   if $disp {
      global display sao
      
      if {![info exists display] || !$display} {
	 set display -1
      } else {
	 set display 0
      }

      display $reg "" default 1; saoReset $sao(default)
   }
   #
   # Create those SCSTAMPs
   #
   set pstamps [chainNew SCSTAMP]
   
   set nsat 0; set nbad 0;
   set curs [chainCursorNew $objs]
   while {[set o [chainWalk $objs $curs]] != ""} {
      assert {[exprGet $o.peaks->npeak] >= 1}
      
      foreach rcp "rowc colc peak" {
	 set $rcp [exprGet $o.peaks->peaks<0>->$rcp]
      }
      handleDel $o

      if $disp {
	 if {$peak < $satur} {
	    set ei i
	 } else {
	    set ei e
	 }
	 draw_cross + $rowc $colc "-$ei -s $sao(default)"
      }

      if {$satur > 0 && $peak > $satur} {
	 incr nsat
	 set o [chainElementRemByCursor $objs $curs]
	 objectDel $o
	 continue;
      }

      set stamp [stampCut $reg $rowc $colc $verbose]
      if {$stamp == ""} {
	 incr nbad;
      } else {
	 #display *$stamp.reg "$rowc $colc";		# RHL XXX
	 chainElementAddByPos $pstamps $stamp; handleDel $stamp
      }
   }
   chainCursorDel $objs $curs
   if $verbose {
      echo "; cut [chainSize $pstamps] postage stamps ($nsat saturated; $nbad bad)"
   }
   #
   # Cleanup
   #
   objectChainDel $objs
   vectorExprDel $thresh
   finiObjectFinder

   return $pstamps
}

###############################################################################
#
# Cut an SCSTAMP
#
#
# N.b. the error codes for daCentroidFind are given at
# http://sdsshost.apo.nmsu.edu/astrotools/doc/www/atObject.html#atDACentroidFind
#
if ![info exists daCentroidFindCodes] {
   array set \
       daCentroidFindCodes \
       [list \
          0    [list "OK" 1] \
         -1    [list "Center isn't locally highest pixel" 0] \
         -2    [list "Object is too close to the edge" 0] \
         -3    [list "Object has vanishing 2nd derivative" 0] \
         -4    [list "Sky is too high" 1] \
        -10    [list "Zero sigma - peak+counts not calculated" 1] \
        -11    [list "Infinite sigma" 1] \
        -21    [list "Error in setfsigma (sigma too large)" 1] \
        -22    [list "Sigma out of range (inf sharp or flat" 0] \
        -23    [list "Too many iterations" 1] \
        -24    [list "Too close to edge" 1] \
        -25    [list "Error in lgausmom - check sky value" 1] \
        -26    [list "Too flat in atFindFocMom" 1] \
        -30    [list "Zero sigma - peak+counts not calculated" 1] \
        -31    [list "Infinite sigma" 1] \
        -40    [list "Sky > 65535" 1] \
       ]
}

proc stampCut {reg rowc colc {verbose 0}} {
   global daCentroidFindCodes ROWS
   set stamp [stampNew]

   foreach rc "row col" {
      set i$rc [expr int([set ${rc}c] + 0.5)]
      set n$rc [exprGet $stamp.reg->n$rc]
      set ${rc}0 [expr [set i$rc] - [set n$rc]/2]
   }

   set ll [daCentroidFind $reg $rowc $colc 1000 1 1 1]
   set flag [keylget ll flag]
   
   if [info exists daCentroidFindCodes($flag)] {
      set code $daCentroidFindCodes($flag)
      set msg [lindex $code 0]
      set fatal [expr 1 - [lindex $code 1]]
   } else {
      set fatal 1
      set msg "unknown error"
   }
   
   if $fatal {
      if {$verbose > 1} {
	 echo \
	     " daCentroidFind retuns $flag ($msg) for object at ($rowc, $colc)"
      }
      stampDel $stamp
      
      return ""
   }
   
   handleSet $stamp.rowc [keylget ll row]
   handleSet $stamp.colc [keylget ll col]
   handleSet $stamp.peak [exprGet $reg.$ROWS<$irow><$icol>]

   if {$row0 < 0 || $row0 + $nrow >= [exprGet $reg.nrow] ||
       $col0 < 0 || $col0 + $ncol >= [exprGet $reg.ncol]} {
      if {$verbose > 1} {
	 echo "object at ($rowc, $colc) is too close to the edge"
      }
      stampDel $stamp
      
      return ""
   } else {
      set sreg [subRegNew $reg $nrow $ncol $row0 $col0]

      handleSet $stamp.midRow $irow
      handleSet $stamp.midCol $icol
      regIntCopy *$stamp.reg $sreg

      regDel $sreg
   }

   return $stamp
}

###############################################################################
#
# Given a chain of OBJECTs, setup the photo infofile mechanism to support
# matching
#
proc infoObjNew {} {
   return [genericNew INFO_OBJ]
}

proc infoObjDel {info_obj} {
   genericDel $info_obj
}

proc stampChainToInfofile {chain {binsize 0.1}} {
   global ROWS

   set ichain [chainNew INFO_OBJ]
   loop i 0 [chainSize $chain] {
      set o [chainElementGetByPos $chain $i]

      set midRow [exprGet $o.midRow]
      set midCol [exprGet $o.midCol]

      if {$midRow < 0 || $midCol < 0} {
	 handleDel $o
	 continue;
      }

      set rc [regIntMaxPixelFind *$o.reg]
      set r [lindex $rc 0]; set c [lindex $rc 1]

      set iobj [infoObjNew]
      handleSet $iobj.row $midRow
      handleSet $iobj.col $midCol
      handleSet $iobj.mag [expr -[exprGet $o.reg->$ROWS<$r><$c>]]
      handleSet $iobj.props<0> $i

      chainElementAddByPos $ichain $iobj; handleDel $iobj

      handleDel $o
   }

   infofileSetFromChain $ichain $binsize

   chainDestroy $ichain infoObjDel
}

###############################################################################
#
# Find the offsets between a set of bands, based on the chains of detected
# SCSTAMPS
#
# This proc doesn't allow for rotations between the bands; sorry
#
proc findOffsets {args} {
   set opts [list \
		 [list [info level 0] ""] \
		 [list <filterlist> STRING "" filterlist \
		      "filters to process"] \
		 [list <cc> STRING "" cc "name of canonical filter"] \
		 [list <pstamps> STRING "" _pstamps \
		      "TCL atrray of SCSTAMP chains"] \
		 [list -drow STRING "" _drowGuess \
 "TCL array of initial guesses for row offsets, (band - cc)"] \
		 [list -dcol STRING "" _dcolGuess \
 "TCL array of initial guesses for col offsets, (band - cc)"] \
		 [list -dmax STRING "" _dmaxGuess \
		      "TCL array of maximum radii to search for matches"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   upvar $_pstamps pstamps
   foreach rc "max row col" {
      if {[set _d${rc}Guess] != ""} {
	 upvar [set _d${rc}Guess] d${rc}Guess
      }
   }
   #
   # Setup matching code
   #
   stampChainToInfofile $pstamps($cc)
   #
   # Match the other bands
   #
   set drv [vectorExprEval 0];	# vectors of offsets
   set dcv [vectorExprEval 0];	# vectors of offsets
   
   foreach f $filterlist {
      if {$f == $cc} {
	 lappend ll [list $f [list 0 0]]
	 continue
      }
	 
      foreach rc "max row col" {	# set initial guesses
	 if [info exists d${rc}Guess($f)] {
	    set d$rc [set d${rc}Guess($f)]
	 } else {
	    set d$rc 0;
	 }
      }
      if {$dmax == 0} {
	 set dmax 32
      }

      set d $dmax;			# initial matching radius
      while {$d >= 2} {
	 set dr {}; set dc {}
	 loop i 0 [chainSize $pstamps($f)] {
	    set o [chainElementGetByPos $pstamps($f) $i]
	    set orowc [exprGet $o.rowc]; set ocolc [exprGet $o.colc]
	    
	    if ![catch {
	       set iobj [infofileMatch \
			     [expr $orowc-$drow] [expr $ocolc-$dcol] $d]
	    } msg] {
	       set co [chainElementGetByPos $pstamps($cc) \
			   [exprGet $iobj.props<0>]]
	       set corowc [exprGet $co.rowc]; set cocolc [exprGet $co.colc]
	       
	       if 0 {
		  echo [format "$d $drow $dcol  $i: ($orowc,$ocolc) D = %.1f" \
			    [expr sqrt(pow($orowc - $drow - $corowc, 2) + \
					   pow($ocolc - $dcol - $cocolc, 2))]]
	       }
	       
	       lappend dr [expr $orowc - $drow - $corowc]
	       lappend dc [expr $ocolc - $dcol - $cocolc]
	       
	       handleDel $co; handleDel $iobj
	    }
	    handleDel $o
	 }
	 
	 set drow [vMedian [vectorExprSet $drv "$drow + {$dr}"]]
	 set dcol [vMedian [vectorExprSet $dcv "$dcol + {$dc}"]]
      
	 set d [expr int($d/2)]
      }
      #
      # Peak up those offsets
      #
      lappend ll [list $f [list \
			       [vectorExprGet "sum($drv)/dimen($drv)"] \
			       [vectorExprGet "sum($dcv)/dimen($dcv)"]]]
   }
   
   vectorExprDel $dcv; vectorExprDel $drv
   infofileFini

   return $ll
}

###############################################################################
#
# Write a TRANS file for all the provided filters
#
# Note: this isn't really a legal asTrans file, but it's good enough to
# keep PSP and frames happy.  For example, the file has an "ID" field rather
# than a "FIELD" field. Also, the transformation isn't really to (mu,nu), but
# rather simply a shift to the `canonical' band (usually r')
#
proc write_trans_chains {run camCol field file filterlist _translist} {
   upvar $_translist translist
   
   set hdr [hdrNew]; hdrInsertLine $hdr 1 "END"

   hdrInsWithLogical $hdr SIMPLE T
   hdrInsWithInt $hdr BITPIX 8
   hdrInsWithInt $hdr NAXIS  0
   hdrInsWithLogical $hdr EXTEND T
   hdrInsWithAscii $hdr ASTRO_ID "FSC"
   hdrInsWithAscii $hdr VERSION [photoVersion]
   hdrInsWithAscii $hdr KO_VER "FSC"
   hdrInsWithDbl $hdr INCL 0.0
   hdrInsWithDbl $hdr NODE 0.0
   hdrInsWithDbl $hdr EQUINOX 0.0
   hdrInsWithInt $hdr RUN $run
   hdrInsWithAscii $hdr CAMCOLS "$camCol"
   hdrInsWithInt $hdr FIELD0 $field
   hdrInsWithInt $hdr NFIELDS [chainSize $translist([lindex $filterlist 0])]
   hdrInsWithAscii $hdr CCDARRAY "photo"
   hdrInsWithAscii $hdr FILTERS $filterlist

   hdrInsWithAscii $hdr [format SSC%02d_ID $camCol] "FSC"


   hdrWriteAsFits $hdr $file
   hdrDel $hdr

   foreach f $filterlist {
      set hdr [hdrNew]
      hdrInsWithAscii $hdr FILTER $f
      hdrInsWithInt $hdr CAMROW [camRowGet $f]
      hdrInsWithInt $hdr CAMCOL $camCol

      chainWriteAsFits $translist($f) TRANS $hdr $file 1
      hdrDel $hdr
   }
}

#
# Return a filter's camrow
#
proc camRowGet {f} {
   set ind [lsearch "\a r i u z g" $f]
   if {$ind > 0} {
      return $ind
   } else {
      return 0
   }
}

proc ccdName {args} {
   set opts [list \
		 [list [info level 0] "Return a CCD name (e.g. r1) given the camCol and camRow"] \
		 [list <camRow> INTEGER "" camRow "camRow (1..5)"] \
		 [list <camCol> INTEGER "" camCol "camCol (1..6)"] \
		 [list -filters STRING "riuzg" filters "The filters in the camera in order (e.g. riuzg)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   if {$camRow < 1 || $camRow > [string length $filters]} {
      error [format "camRow == $camRow is out of range 1...%d" [string length $filters]]
   }

   return [format "%s%d" [lindex [split $filters ""] [expr $camRow-1]] $camCol]
}

###############################################################################
#
# Run the FSC, creating needed files if so requested
#
proc run_fsc_pipeline {args} {
   global openit

   set disp 0; set mask 0
   set write_all 0
   set write_bias 0; set write_fang 0;
   set write_ff 0; set write_op 0; set write_plans 0; set write_trans 0
   set write_wing 0
   set write_idR 0
   set read_fpC 0;			# run frames in read_fpC mode?
   set link_fpC 0;			# link fields/?/idR* to corr/?/fpC*
   set constantPSF 0
   set is_table 0;			# is <files> actually a table?
   set canon_dirs 0;			# put files in outDir/run/rerun/...
   
   set opts [list \
		 [list [info level 0] \
		      {Generate input files to run psp and/or frames.

 To process a set of files in the current directory, try something like:

   set psfFiles(g) psf.fits;
   run_fsc_pipeline psfFiles -thresh 2000 -constantPSF -writeAll
   run_ps_pipeline psPlan.par
   set dataFiles(g) data.fits; run_fsc_pipeline dataFiles -writeIdR
   run_frames_pipeline fpPlan.par

 An alternative would be
   run_fsc_pipeline -run $run -rerun $rerun -camCol $camCol -filters "g r" \ 
	     -field0 $field0 -field1 $field1 -canon -outdir $data_root \ 
	     -threshhold 45000 -writeAll -constantPSF
   do-psp -rerun $rerun $run $camCol $field0 $field1
   do-frames -rerun $rerun $run $camCol $field0 $field1 -param {nsky_objects 0}
 Where it's assumed that you've put `idR' files in the directory
   $data_root/$run/$rerun/fields/$camCol

 If you want to re-process some photo outputs, take a look at -isTable,
 which assumes that you've used set_run/openit to read fpObjc/tsObj files
 }] \
		 [list {[files]} STRING "" _files \
 "TCL array of files to process; the indices define the filter set"] \
		 [list {[thresh]} STRING "" thresh_arg \
		      "Desired threshold for finding PSF stars"] \
		 [list -threshhold STRING "30s" threshold \
		      "Desired threshold for finding PSF stars"] \
		 [list -outdir STRING "." outdir "Output directory"] \
		 [list -canonical CONSTANT 1 canon_dirs \
		      "Put files in canonical places; outdir/run/rerun/..."]\
		 [list -run INTEGER 0 run "Run number"] \
		 [list -rerun INTEGER 0 rerun "Rerun number"] \
		 [list -camCol INTEGER 7 camCol \
		      "Camera column (0 is taken for MT/Spectro chips)"] \
		 [list -field0 INTEGER 1 field0 "Starting Field number"] \
		 [list -field1 INTEGER -1 field1 \
		      "Ending field number (default: field0)"] \
		 [list -satur INTEGER 0 satur "Saturation level (0 => none)"] \
		 [list -constantPSF CONSTANT 1 constantPSF \
		      "The PSF is constant over the image"] \
		 [list -skirt INTEGER 0 skirt \
		      "Number of extra rows/columns to add to images"] \
		 [list -writeAll CONSTANT 1 write_all \
		      "Set all the -write flags except -writeIdR"] \
		 [list -writeBias CONSTANT 1 write_bias \
		      "Write bias vectors to idB files"] \
		 [list -writeFang CONSTANT 1 write_fang \
		      "Write stamps and quartiles to scFang files"] \
		 [list -writeWing CONSTANT 1 write_wing \
		      "Write wing stars to scWing files"] \
		 [list -writeFF CONSTANT 1 write_ff \
		      "Write (inverted) flat fields to psFF files"] \
		 [list -writeIdR CONSTANT 1 write_idR \
		      "Write raw frame (\"idR\") files"] \
		 [list -writeOp CONSTANT 1 write_op \
		      "Write op{BC,Config,Camera,EConfig}.par files"] \
		 [list -writePlans CONSTANT 1 write_plans \
		      "Write {fp,ps}Plan.par files"] \
		 [list -writeTrans CONSTANT 1 write_trans \
		      "Write TRANSs to asTrans file"] \
		 [list -sky STRING "" _sky_arr \
		      "Array of sky values (indexed by filtername)"] \
		 [list -drow STRING "" _drow \
 "TCL array of initial guesses for row offsets"] \
		 [list -dcol STRING "" _dcol \
 "TCL array of initial guesses for column offsets"] \
		 [list -dmax STRING "" _dmax \
		      "TCL array of maximum radii to search for matches"] \
		 [list -display CONSTANT 1 disp "Display objects?"] \
		 [list -mask CONSTANT 1 mask \
 "Set saturated bits in <reg>'s SPANMASK (which will be created if necessary)"] \
		 [list -bias STRING "" _bias \
		      "array of bias levels in data (DN)"] \
		 [list -flux20 STRING "" _flux20 \
		      "array of counts for 20th magnitude stars (DN)"] \
		 [list -gain STRING "" _gain "array of gains of amplifiers"] \
		 [list -readNoise STRING "" _readNoise \
		      "array of amplifiers' read noise (DN)"] \
		 [list -verbose INTEGER 0 verbose "How verbose should I be?"] \
		 [list -id STRING "" id "ID or rowc,colc for desired object"] \
		 [list -isTable CONSTANT 1 is_table \
		      "<files> is actually the name of a list, as returned by e.g. objfileOpen"] \
		 [list -filters STRING "" filters \
		      "List of filters (overrides that in tables/files)"] \
		 [list -readFpC CONSTANT 1 read_fpC \
		      "Create files to only run frames, reading fpC files"] \
		 [list -linkFpC CONSTANT 1 link_fpC \
		      "Create links from idR to fpC files"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }
   #
   # Deal with those arguments
   #
   if {$thresh_arg != ""} {
      set threshold $thresh_arg; unset thresh_arg
   }

   if {$write_idR && $read_fpC} {
      error "Please decide if you want idR or fpC files, then try again"
   }

   if {$field1 < 0} {
      set field1 $field0
   }
   if {$_files != ""} {
      if {$field1 != $field0} {
	 error "You cannot specify an array with filenames and also -field1: $_files"
      }
   }

   if $is_table {
      upvar $_files table
      global objcIo
      
      if {$id == ""} {
	 error "You must specify the ID for the desired object"
      }
      if [regexp {,} $id] {
	 set id [find_objc table $id]
      }

      set filterlist [keylget table filters]
      foreach el "run camCol field" {
	 set $el $openit($el)
      }
      if {$run != "0"} {
	 regsub {^0*[0-9]+$} $run "" run
      }
   } else {
      if [info exists openit] {
	 unset openit;			# it confuses createPlanFiles
      }

      if $read_fpC {
	 error "I'm afraid that you can only specify -readFpC with -isTable"
      }

      if {$_files == ""} {
	 if {$filters == ""} {
	    error \
	 "You must specify -filters if you don't provide an array of datafiles"
	 }
      } else {
	 upvar $_files files

	 set filterlist [array names files]
      }
      
      if {$_sky_arr != ""} {
	 upvar $_sky_arr sky_arr
      }
      
      foreach rc "max row col" {
	 if {[set _d$rc] != ""} {
	    upvar [set _d$rc] d$rc
	 }
      }
   }
   if {$filters != ""} {
      set filterlist $filters
   }
   set nfilter [llength $filterlist]

   if $mask {
      set mask -mask
   } else {
      set mask ""
   }
   
   if $write_all {
      foreach v [concat [info local read_*] [info local write_*]] {
	 if ![regexp {_(idR|fpC|wing)$} $v] {
	    set $v 1
	 }
      }
   }

   foreach a "bias gain readNoise flux20" {
      if {[set _$a] != ""} {
	 upvar [set _$a] $a
      } else {
	 foreach f $filterlist {
	    switch -- $a {
	       "bias"    { set val 0 }
	       "flux20"  { set val 1000 }
	       "default" { set val 1 }
	    }
	    set ${a}($f) $val
	 }
      }
   }
   
   if [regsub {^~} $outdir "" outdir] {
      global env
      set outdir $env(HOME)$outdir
   }
   if $canon_dirs {
      set dir $outdir/$run/$rerun
   } else {
      set dir $outdir
   }

   if ![file isdirectory $dir] {
      mkdir -path $dir
   }
   
   if $disp {
      set disp "-display"
   } else {
      set disp ""
   }
   #
   # Set IDs if so required
   #
   global id_values
   if ![info exists id_values(PS_ID)] {
      set id_values(PS_ID) PHOTO
   }
   if ![info exists id_values(VERSION)] {
      set id_values(VERSION) [photoVersion]
   }

   foreach f $filterlist {
      set translists($f) [chainNew TRANS]
   }
   #
   # Remove old scWing.par files?
   #
   if $write_wing {
      set fname [format "%s/scWing-%06d-%d.par" $dir/fangs/$camCol $run $camCol]
      if [file exists $fname] {
	 unlink $fname
      }
   }
   #
   # Make requested links
   #
   global idR_format
   if ![info exists idR_format] {
      set idR_format \
	  {idR-%06d-%s%d-%04d.fit  $run $filter $camCol $field}
   }

   if $link_fpC {
      if ![file isdirectory $dir/corr/$camCol] {
	 mkdir -path $dir/corr/$camCol
      }
      loop field $field0 [expr $field1 + 1] {
	 foreach f $filterlist {
	    set idRfile [get_raw_filename "" $run $f $camCol $field]
	    regsub {^idR} $idRfile "fpC" fpCfile

	    set idRfile $dir/fields/$camCol/$idRfile
	    set fpCfile $dir/corr/$camCol/$fpCfile
	    if ![file exists $fpCfile] {
	       catch {
		  link -sym $idRfile $fpCfile
	       }
	    }
	 }
      }
   }
   #
   # Finally do some work
   #
   # Read the files and maybe generate the lists of SCSTAMPs
   #
   loop field $field0 [expr $field1 + 1] {
      foreach f $filterlist {
	 set fi [lsearch $filterlist $f]
	 
	 if {!($write_fang || $write_trans || $write_idR || $read_fpC)} {
	    continue;
	 }
	 
	 if $is_table {
	    if {[phRandomIsInitialised]} {
	       set rand [lindex [handleListFromType RANDOM] 0]
	    } else {
	       upvar rand rand
	       set rand [phRandomNew 100000]
	       echo Setting \$rand
	    }
	    set reg($f) [unbin $f $field $rand]
	    
	    set hdu [get_file_names $openit(objdir) $openit(objdir) \
			 $openit(run) $openit(col) $field $openit(reprocess) \
			 file aifile]
	    
	    set objcIo [objcIoRead table $id]
	    regSetFromAtlasImage *$objcIo.aimage $fi $reg($f)
	    regIntConstAdd $reg($f) [expr int([exprGet $objcIo.sky<$fi>])]
	    
	    foreach t "rmin rmax cmin cmax" {
	       set $t [exprGet (*$objcIo.aimage->master_mask).$t]
	    }
	    
	    catObjDel $objcIo -deep; unset objcIo
	 } else {
	    if ![info exists files] {
	       if $canon_dirs {
		  set dir $outdir/$run/$rerun/fields/$camCol
	       } else {
		  set dir $outdir
	       }

	       set file [get_raw_filename $dir $run $f $camCol $field]
	    } else {
	       if [regexp {^h[0-9]+} $files($f)] {# probably a handle
		  if ![catch { exprGet $files($f).nrow }] {# a REGION handle
		     set reg($f) $files($f)
		  }
	       }
	       if ![info exists reg($f)] {
		  if [file exists $files($f)] {
		     set file $files($f)
		  } elseif {[file exists $outdir/$files($f)]} {
		     set file $outdir/$files($f)
		  } else {
		     error "I can find $files($f) in neither . nor $outdir"
		  }
	       }
	    }
	       
	    set reg($f) [regReadAsFits -keep [regNew] $file]
	 }
	 
	 regStatsFromQuartiles $reg($f) -cmedian cmed -csigma csig
	 
	 if [info exists sky_arr($f)] {
	    set skyarr($f) $sky_arr($f)
	 } else {
	    set skyarr($f) $cmed
	 }
	 set skysigarr($f) $csig
	 #
	 # Were we asked for extra pixels?
	 # We may need them to cut 65x65 postage stamps
	 #
	 if {$skirt > 0} {
	    set rand [phRandomNew 10000:2]
	    
	    set nrow [exprGet $reg($f).nrow]
	    set ncol [exprGet $reg($f).ncol]
	    set breg [regNew [expr $nrow + 2*$skirt] [expr $ncol + 2*$skirt]]
	    
	    regClear $breg;
	    regIntGaussianAdd $breg $rand [expr 1000+$cmed] $csig
	    regIntLincom $breg "" -1000 1 0 
	    
	    set sbreg [subRegNew $breg $nrow $ncol $skirt $skirt]
	    regIntCopy $sbreg $reg($f); regDel $sbreg
	    regDel $reg($f); set reg($f) $breg
	    phRandomDel $rand
	 }
	 #
	 # Write input files to frames?
	 #
	 if $write_idR {
	    regWriteAsFits $reg($f) \
		[format $outdir/idR-%06d-${f}$camCol-%04d.fit $run $field]
	 } elseif $read_fpC {
	    regWriteAsFits $reg($f) \
		[format $outdir/fpC-%06d-${f}$camCol-%04d.fit $run $field]
	 }		      
	 #
	 # Was the threshold specified in sigma?
	 #
	 if [regexp {^([0-9]+)s$} $threshold foo thresh] {
	    set thresh [expr $cmed + $thresh*$csig]
	 } else {
	    set thresh $threshold
	 }

	 set reg_ncol($f) [exprGet $reg($f).ncol]
	 set reg_nrow($f) [exprGet $reg($f).nrow]
	 #
	 # Find the postage stamps
	 #
	 if {$write_fang || $write_trans || $write_wing} {
	    if $write_wing {
	       set wings "-wing wingRegs"
	    } else {
	       set wings ""
	    }
	    set pstamps($f) \
		[eval make_scstamp_chain $reg($f) $thresh $f \
		     $disp -satur $satur $mask -verbose $verbose $wings]
	 }
	 #
	 # If we're writing fang files in more than one band we need to ensure
	 # that the same stars appear on the list in each band, so keep the
	 # regions
	 #
	 if {!$write_fang || $nfilter == 1} {
	    regDel $reg($f); unset reg($f)
	 }
      }
      #
      # Generate trans files?
      #
      if {($write_fang && $nfilter > 1) || $write_trans} {
	 if {[lsearch $filterlist r] >= 0} {
	    set cc r
	 } else {
	    set cc [lindex $filterlist 0]
	 }
	 
	 set offsets [findOffsets $filterlist $cc pstamps \
			  -drow drow -dcol dcol -dmax dmax]
	 
	 foreach f $filterlist {
	    set off [keylget offsets $f]
	    set dr($f) [lindex $off 0]; set dc($f) [lindex $off 1]
	    if $verbose {
	       echo "Filter $f: dr,dc = $dr($f),$dc($f)"
	    }
	    set scale 1e-4
	    set trans [transInit \
			   [expr $scale*(0 - $dr($f))] $scale 0 \
			   [expr $scale*(0 - $dc($f))] 0 $scale]
	    chainElementAddByPos $translists($f) $trans; handleDel $trans
	 }
      }
      #
      # Write the fang file?
      #
      if $write_fang {
	 if {$nfilter > 1} {
	    complete_pstamps $filterlist $cc reg pstamps dr dc
	 }
	 
	 if $canon_dirs {
	    set dir $outdir/$run/$rerun/fangs/$camCol
	 } else {
	    set dir $outdir
	 }
	 
	 if ![file isdirectory $dir] {
	    mkdir -path $dir
	 }
	 
	 write_fang_file $run $camCol $field $filterlist $dir \
	     -pstamps pstamps -ncol reg_ncol -skylev skyarr -skysig skysigarr
      }

      if $write_wing {
	 write_wing_file $run $camCol $field $filterlist $dir wingRegs

	 foreach f $filterlist {
	    if [info exists wingRegs($f)] {
	       regDel $wingRegs($f)
	    }
	 }
	 unset wingRegs
      }

      if ![info exists files] {
	 foreach el [array names reg] {
	    regDel $reg($el)
	    unset reg($el)
	 }
      }
   }
   #
   # Done with loop over fields; write out trans if requested and proceed
   # with bias/flats
   #
   if $write_trans {
      foreach f $filterlist {
	 set trans [format "asTrans-%06d-%d.fit" $run $camCol]
	 if $canon_dirs {
	    set dir $outdir/$run/$rerun/astrom
	    if ![file isdirectory $dir] {
	       mkdir $dir
	    }
	    set trans "$dir/$trans"
	 } else {
	    set trans "$outdir/$trans"
	 }
	 
	 write_trans_chains $run $camCol $field0 $trans \
	     $filterlist translists
      }
   }

   foreach f $filterlist {
      chainDestroy $translists($f) transDel
   }
   #
   # Done with regions
   #
   foreach f $filterlist {
      if [info exists reg($f)] {
	 regDel $reg($f)
      }
   }
   #
   # Generate bias files?
   #
   if $write_bias {
      if $canon_dirs {
	 set dir $outdir/$run/$rerun/photo/calib
      } else {
	 set dir $outdir
      }
      
      if ![file isdirectory $dir] {
	 mkdir -path $dir
      }
      
      set tshift 32
      foreach f $filterlist {
	 set biasvec [regNew -type U16 1 $reg_ncol($f)];
	 regIntSetVal $biasvec [expr $tshift*$bias($f)]
	 hdrInsWithAscii $biasvec.hdr VERSION "[photoVersion]"
	 hdrInsWithAscii $biasvec.hdr BIAS_ID "FSC"
	 hdrInsWithInt $biasvec.hdr TSHIFT $tshift
	 hdrInsWithInt $biasvec.hdr CAMCOL $camCol
	 hdrInsWithInt $biasvec.hdr CAMROW [camRowGet $f]
	 hdrInsWithInt $biasvec.hdr RUN $run

	 regWriteAsFits $biasvec [format $dir/idB-%06d-$f$camCol.fit $run]

	 regDel $biasvec
      }
   }

   #
   # Generate flat field files?
   #
   if $write_ff {
      if $canon_dirs {
	 set dir $outdir/$run/$rerun/objcs/$camCol
      } else {
	 set dir $outdir
      }
      
      if ![file isdirectory $dir] {
	 mkdir -path $dir
      }
      
      set tshift 2048
      foreach f $filterlist {
	 set ffvec [regNew -type U16 1 $reg_ncol($f)];
	 regIntSetVal $ffvec $tshift
	 hdrInsWithAscii $ffvec.hdr VERSION "[photoVersion]"
	 hdrInsWithAscii $ffvec.hdr BIAS_ID "FSC"
	 hdrInsWithAscii $ffvec.hdr PS_ID "FSC"
	 hdrInsWithInt $ffvec.hdr TSHIFT $tshift
	 hdrInsWithInt $ffvec.hdr CAMCOL $camCol
	 hdrInsWithAscii $ffvec.hdr FILTER $f
	 hdrInsWithInt $ffvec.hdr RUN $run
	 hdrInsWithInt $ffvec.hdr FIELD $field0

	 regWriteAsFits $ffvec [format $dir/psFF-%06d-$f$camCol.fit $run]

	 regDel $ffvec
      }
   }
   if $write_op {
      if $canon_dirs {
	 set dir $outdir/$run/$rerun/logs
      } else {
	 set dir $outdir
      }

      if ![file isdirectory $dir] {
	 mkdir -path $dir
      }
      #
      # opBC
      #
      set opBC [chainNew CCDBC]
      chain2Param $dir/opBC.par $opBC ""
      chainDestroy $opBC genericDel
      #
      # opCamera
      #      
      set opCamera [chainNew CCDGEOMETRY]
      foreach filter $filterlist {
	 set row [camRowGet $filter]

	 set el [genericNew CCDGEOMETRY]

	 handleSet $el.dewarID 0
	 handleSet $el.camRow $row
	 handleSet $el.camCol $camCol
	 handleSet $el.xmag   [expr -20 - 2.5*log10($flux20($filter))]
	 
	 chainElementAddByPos $opCamera $el
	 handleDel $el
      }

      chain2Param $dir/opCamera.par $opCamera ""
      chainDestroy $opCamera genericDel
      #
      # opConfig
      #      
      set opConfig [chainNew CCDCONFIG]
      foreach filter $filterlist {
	 set row [camRowGet $filter]

	 set el [genericNew CCDCONFIG]

	 handleSet $el.camRow $row
	 handleSet $el.camCol $camCol
	 handleSet $el.rowBinning 1
	 handleSet $el.colBinning 1
	 handleSet $el.nrows $reg_nrow($filter)
	 handleSet $el.ncols $reg_ncol($filter)

	 loop n 0 4 {
	    handleSet $el.amp$n 0
	    foreach g [list "" "good"] {
	       foreach what [list PreBias PostBias OverScan \
				 MapOverScan OverScanRows \
				 DataSec DataRow
			    ] {
		  foreach ns "n s" {
		     handleSet $el.${ns}${what}${n}$g 0
		  }
	       }
	       handleSet $el.sCCDRowSec${n}$g 0
	       handleSet $el.sCCDColSec${n}$g 0
	    }
	 }
	 
	 handleSet $el.amp0 1
	 handleSet $el.sDataSec0 0
	 handleSet $el.nDataSec0 $reg_ncol($filter)
	 handleSet $el.sDataRow0 0
	 handleSet $el.nDataRow0 $reg_nrow($filter)
	 
	 chainElementAddByPos $opConfig $el
	 handleDel $el
      }

      chain2Param $dir/opConfig.par $opConfig ""
      chainDestroy $opConfig genericDel
      #
      # opECalib
      #
      set sch [schemaGetFromType ECALIB]
      regexp {\[([0-9]+)\]} [keylget sch DN0] foo nlin

      set opECalib [chainNew ECALIB]
      foreach filter $filterlist {
	 set row [camRowGet $filter]

	 set el [genericNew ECALIB]

	 handleSet $el.camRow $row
	 handleSet $el.camCol $camCol

	 loop n 0 4 {
	    handleSet $el.readNoiseDN$n $readNoise($f)
	    handleSet $el.fullWellDN$n [expr $satur==0 ? 1e6 : $satur]
	    handleSet $el.gain$n $gain($f)
	    handleSet $el.biasLevel$n $bias($f)
	    loop i 0 $nlin {
	       handleSet $el.DN$n<$i> 0
	       handleSet $el.linearity$n<$i> 0
	    }
	    handleSet $el.DN$n<0> "-1";	# no non-linearity correction
	 }
	 
	 chainElementAddByPos $opECalib $el
	 handleDel $el
      }

      chain2Param $dir/opECalib.par $opECalib ""
      chainDestroy $opECalib genericDel
   }
   #
   # Write plan files?
   #
   if $write_plans {
      set args "-fsc"
      if $constantPSF {
	 lappend args "-constantPSF"
      }
      if $read_fpC {
	 lappend args "-read_fpC"
      }

      if $canon_dirs {
	 set dir $outdir
	 lappend args "-canon"
	 lappend args "-directories"
      } else {
	 set dir $outdir
      }

      if ![file isdirectory $dir] {
	 mkdir -path $dir
      }
            
      eval createPlanFiles -outDir $dir $run $rerun -camCol $camCol \
	  -filterlist \"$filterlist\" -startField $field0 -endField $field1 \
	  $args
   }
   #
   # Cleanup
   #
   foreach f $filterlist {
      if [info exists pstamps($f)] {
	 stampChainDel $pstamps($f)
      }
   }
}

###############################################################################
#
# Recreate a set of fpC files from fpAtlas/fpBIN files
#
proc make_fpC_files {args} {
   global openit
   set copy_fpM 0
   set force 0;				# allow overwriting of files

   set opts [list \
		 [list [info level 0] "\
 Recreate a set of fpC files from fpAtlas/fpBIN files; uses set_run to
 specify run etc."] \
		 [list <outdir> STRING "" outDir \
		      "Directory to put fpC files in"] \
		 [list <startField> INTEGER 0 startField \
		      "First (or only) field to reconstruct"] \
		 [list {[endField]} INTEGER -1 endField \
		      "Last field to reconstruct (default: <startField>)"]\
		 [list -filters STRING "u g r i z" filters \
		      "Filters to reconstruct"] \
		 [list -skyType STRING "brightSky" skyType \
		      "How to reconstruct sky"] \
		 [list -force CONSTANT 1 force \
		      "Allow [info level 0] to overwrite fpC files"] \
		 [list -fpM CONSTANT 1 copy_fpM \
		      "Copy the fpM files to <outdir>"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![info exists openit] {
      error "Please use set_run to specify a run and column (and maybe rerun)"
   }

   if {$outDir == ""} { set outDir "." }
   if {$endField < 0} { set endField $startField }

   if {[phRandomIsInitialised]} {
      set rand [lindex [handleListFromType RANDOM] 0]
   } else {
      set rand [phRandomNew 100000]
      echo Setting \$rand
   }

   if ![file isdirectory $outDir] {
      if $force {
	 regsub {/[0-6]$} $outDir "" dir
	 if ![file exists $dir] {
	    catch {
	       mkdir $dir
	    }
	 }
	 catch {
	    mkdir $outDir
	 }
      }
   }
   if ![file isdirectory $outDir] {
      set msg "Directory $outDir does not exist"
      if $force {
	 append msg " and cannot be created"
      }
      error $msg
   }

   loop field $startField [expr $endField + 1] {
      foreach f $filters {
	 #
	 # Paste in the atlas images
	 #
	 set reg [recon $f $field {} {} 0 $skyType]

	 set file \
	     [format fpC-$openit(run)-%s$openit(col)-%04d.fit $f $field]
	 echo $file

	 set dfile "$outDir/$file"
	 if {!$force && [file exists "$dfile"]} {
	    error "$dfile exists; not overwriting"
	 } else {
	    regWriteAsFits $reg $dfile
	 }

	 regDel $reg

	 if $copy_fpM {
	    set fpM [format fpM-$openit(run)-%s$openit(col)-%04d.fit $f $field]
	    set dfile "$outDir/$fpM"
	    if {!$force && [file exists "$dfile"]} {
	       error "$dfile exists; not overwriting"
	    } else {
	       link_or_copy $openit(dir)/$fpM $dfile
	    }
	 }
      }
   }
}

  
###############################################################################
#
# Create scFang files from the outputs of photo
#
proc scparambsNew {} {
   set scp [genericNew SCPARAMBS]
   #
   # Set some fields to Jeff Munn's preferred values
   #
   handleSet $scp.peak 0
   handleSet $scp.rMajor 2
   handleSet $scp.rMinor 2
   handleSet $scp.angMajorAxis 0
   
   return $scp
}
proc scparambsDel {scp} { genericDel $scp }

proc photo2Fangs {args} {
   global OBJECT1 OBJECT2
   global id_values openit objcIo table
   set force 0;				# allow overwriting of files

   set opts [list \
		 [list [info level 0] ""] \
		 [list <outdir> STRING "" outDir \
		      "Directory to put scFang files in"] \
		 [list <startField> INTEGER 0 startField \
		      "First (or only) field to process"] \
		 [list {[endField]} INTEGER -1 endField \
		      "Last field to process (default: <startField>)"]\
		 [list -filters STRING "u g r i z" filters \
		      "Filters to process"] \
		 [list -force CONSTANT 1 force \
		      "Allow [info level 0] to overwrite scFang files"] \
		 [list -select STRING "sel_good&&sel_star" select \
		      "Expression to select stars"] \
		 [list -countsMin DOUBLE -1 counts_min \
		      "Minimum acceptable value of psfCounts (in r)"] \
		 ]

   if {[shTclParseArg $args $opts [info level 0]] == 0} {
      return 
   }

   if ![info exists openit] {
      error "Please use set_run to specify a run and column (and maybe rerun)"
   }

   if {$outDir == ""} { set outDir "." }
   if {$endField < 0} { set endField $startField }

   if ![file isdirectory $outDir] {
      if $force {
	 regsub {/[0-6]$} $outDir "" dir
	 if ![file exists $dir] {
	    catch {
	       mkdir $dir
	    }
	 }
	 catch {
	    mkdir $outDir
	 }
      }
   }
   if ![file isdirectory $outDir] {
      set msg "Directory $outDir does not exist"
      if $force {
	 append msg " and cannot be created"
      }
      error $msg
   }
   #
   # Set IDs if so required
   #
   if ![info exists id_values(PS_ID)] {
      set id_values(PS_ID) PHOTO
   }
   if ![info exists id_values(VERSION)] {
      set id_values(VERSION) [photoVersion]
   }
   #
   # Read idFrameLog file for TAI/SCA/RA/DEC
   #
   global idFrameLog

   set framelog idFrameLog-$openit(run)-[format %02d $openit(camCol)].par
   set idFrameLog \
       [param2Chain $openit(logdir)/$framelog -type LOG_FRAMEINFO ""]
   #
   # Is $select actually a logical expression?
   #
   set select [expand_logical_select $select]
   #
   # Process the photo outputs
   #
   if {[info commands exprGetSetPrecision] != ""} {# in dervish 8_7
      set old_precision [exprGetSetPrecision 12]
   }
   
   loop field $startField [expr $endField + 1] {
      set table [openit $field]

      foreach f $filters {
	 set params($f) [chainNew SCPARAMBS]
	 set dummy($f) [chainNew SCSTAMP]
      }

      set n1 1; set n2 [keylget table OBJnrow]
      loop i $n1 [expr $n2+1] {
	 catch {catObjDel $objcIo -deep};		# it may not exist
	 set objcIo [objcIoRead table $i]
	 
	 if {"$select" != "" && ![eval $select $objcIo]} {
	    continue;
	 }

	 if {$counts_min > 0 && [exprGet $objcIo.psfCounts<2>] < $counts_min} {
	    continue;
	 }
	 #
	 # Copy photo outputs into the SCPARAMS
	 #
	 set c -1
	 foreach f $filters {
	    incr c
	    
	    set scp [scparambsNew]

	    if {[exprGet $objcIo.flags2<$c>] & ($OBJECT2(SATUR_CENTER))} {
	       handleSet $scp.peak 70000
	    }


	    handleSet $scp.midRow [expr int([exprGet $objcIo.rowc<$c>]+0.5)]
	    handleSet $scp.midCol [expr int([exprGet $objcIo.colc<$c>]+0.5)]

	    handleSet $scp.rowCentroid [exprGet $objcIo.rowc<$c>]
	    handleSet $scp.rowCentroidErr [exprGet $objcIo.rowcErr<$c>]
	    handleSet $scp.colCentroid [exprGet $objcIo.colc<$c>]
	    handleSet $scp.colCentroidErr [exprGet $objcIo.colcErr<$c>]

	    handleSet $scp.counts [exprGet $objcIo.psfCounts<$c>]

	    if {$c == 0} {
	       set c1 0; set c2 1;	# u - g
	    } elseif {$c == 1} {
	       set c1 1; set c2 2;	# g - r
	    } else {
	       set c1 2; set c2 3;	# r - i
	    }
	    set mag1 [exprGet $objcIo.psfCounts<$c1>]
	    set mag1Err [exprGet $objcIo.psfCountsErr<$c1>]
	    set mag2 [exprGet $objcIo.psfCounts<$c2>]
	    set mag2Err [exprGet $objcIo.psfCountsErr<$c2>]
	    
	    if {$mag1 < 10 || $mag2 < 10} {# bad
	       handleSet $scp.color -99
	       handleSet $scp.colorErr -1
	    } else {
	       handleSet $scp.color [expr -2.5*log10(1.0*$mag1/$mag2)]
	       handleSet $scp.colorErr \
		   [expr 2.5/log(10)*sqrt(pow(1.0*$mag1Err/$mag1,2) + pow(1.0*$mag2Err/$mag2,2))]
	    }
	    handleSet $scp.statusFit [exprGet $objcIo.flags<$c>]
	    handleSet $scp.sscStatus [exprGet $objcIo.flags2<$c>]

	    chainElementAddByPos $params($f) $scp TAIL
	    handleDel $scp
	 }
      }

      foreach f $filters {
	 loop i 0 [chainSize $params($f)] {
	    set stamp [stampNew]
	    chainElementAddByPos $dummy($f) $stamp TAIL
	    handleDel $stamp
	 }
      }
      
      set file [format scFang-$openit(run)-$openit(col)-%04d.fit $field]
      echo $file
      
      set dfile "$outdir/$file"
      if {!$force && [file exists "$dfile"]} {
	 error "$dfile exists; not overwriting"
      } else {
	 write_fang_file $openit(run) $openit(col) $field $filters $outDir \
	     -params params
      }
      #
      # Clean up
      #
      foreach f $filters {
	 chainDestroy $params($f) scparambsDel
	 chainDestroy $dummy($f) stampDel
      }

      objfileClose table
   }
   #
   # Global cleanup
   #
   if [info exists old_precision] {
      exprGetSetPrecision $old_precision
   }

   chainDestroy $idFrameLog genericDel; unset idFrameLog
}

###############################################################################
#
# Set a SCPARAMBS from a STAR1C
#
# This is needed if the SSC gave up on an object, but PSP thinks that
# it's a good star
#
proc set_scparambs_from_star1c {scparambs star1c ifilter} {
   handleSetFromHandle $scparambs.counts $star1c.star1<$ifilter>->apCounts
   
   foreach rc "row col" {
      handleSetFromHandle $scparambs.${rc}Centroid \
	  $star1c.star1<$ifilter>->object->${rc}c
      handleSetFromHandle $scparambs.${rc}CentroidErr \
	  $star1c.star1<$ifilter>->object->${rc}cErr
      
      set n$rc [exprGet $star1c.star1<$ifilter>->object->region->n$rc]
      set ${rc}0 [exprGet $star1c.star1<$ifilter>->object->region->${rc}0]

      set mid$rc [expr [set ${rc}0] + [set n$rc]/2 + 1]
   }
   handleSet $scparambs.midRow $midrow
   handleSet $scparambs.midCol $midcol
   
   handleSet $scparambs.peak \
       [exprGet $star1c.star1<$ifilter>->object->profMean<0>]
   #
   # Calculate color and color error.  Note that the "color" is defined
   # in terms of raw counts with no calibration applied
   #
   # First determine filters to use
   #
   if {$ifilter == 0} {			# u filter: u-g
      set f1 0; set f2 1
   } elseif {$ifilter == 1} {		# g filter: g-r
      set f1 1; set f2 2
   } else {				# all other filters: r-i
      set f1 2; set f2 3
   }
   set ncolor [exprGet $star1c.ncolors]
          
   foreach i "1 2" {
      if {[set f$i] >= $ncolor} {
	 set f$i [expr $ncolor - 1]
      }
      
      set count$i [exprGet $star1c.star1<[set f$i]>->apCounts]
      set countErr$i [exprGet $star1c.star1<[set f$i]>->apCountsErr]
   }
   if {$count1 > 0.01 && $count2 > 0.01} {
      set color [expr 2.5*log10([expr 1.0*$count2/$count1])]
      set colorErr [expr sqrt($countErr1*$countErr1 + $countErr2*$countErr2)]
   } else {
      set color -99.9
      set colorErr -1.0
   }

   handleSet $scparambs.color $color
   handleSet $scparambs.colorErr $colorErr
   #
   # Don't bother to set rMajor, rMinor, angMajorAxis
   #
}

###############################################################################
#
# Write a psFang file given a chain of stamps (i.e. SCPARAMBS).
#
proc write_psFang {outDir _star1clist _scparambs _psfBasis cframe \
		       run camCol filterlist field ignore_ssc_statusFit \
		       star1c_require_mask star1c_ignore_mask \
		       star1_require_mask star1_ignore_mask \
		       {debias_centroid_counts_min 100} \
                       {QAoutfile ""} } {
   upvar $_star1clist star1clist  $_scparambs scparambs  $_psfBasis psfBasis
   global STAR1 display display_scFangs display_psFangs ccdpars
  

   if {$QAoutfile != ""} {
      set outf [open $QAoutfile a]
   }
   #
   # Always set these bits as it makes the logic simpler
   #
   set star1c_ignore_mask  [expr $star1c_ignore_mask  | $STAR1(NOFLAGS)]
   set star1c_require_mask [expr $star1c_require_mask | $STAR1(NOFLAGS)]
   set star1_ignore_mask   [expr $star1_ignore_mask   | $STAR1(NOFLAGS)]
   set star1_require_mask  [expr $star1_require_mask  | $STAR1(NOFLAGS)]

   set star1c_ignore_mask [expr $star1c_ignore_mask | $star1_ignore_mask]
   #
   # Prepare index for $cframe
   #
   loop i 0 [exprGet $cframe.ncolors] {
      set cindex([exprGet $cframe.calib<$i>->filter]) $i
   }
   #
   # Improve astrometry using KL PSF
   #
   set psf [handleNew];			# bind the PSF in $cframe
    
   foreach f $filterlist {
       set ifilter [lsearch $filterlist $f]

       assert { [lsearch [array names psfBasis] $f] >= 0} 
       assert { $ifilter == $cindex($f) }
       assert {[chainSize $star1clist($field)] == \
		  [chainSize $scparambs($field,$f)]}
      #
      # Unpack the CALIB1
      #
      foreach t "dark_variance gain sky" {
	 set $t [exprGet $cframe.calib<$cindex($f)>->$t]
      }
      handleBindFromHandle $psf *$cframe.calib<$cindex($f)>->psf
      set sigma [exprGet $psf.sigma1_2G]
      set psfWidth  [format "%6.3f" [exprGet $psf.width]]

      #
      # Some chips (e.g. 4x4 binned 2-amp chips) have "missing"
      # pixels at the center of the chip.  We deal with this by
      # _adding_ them back when reporting centroids to astrom,
      # and then adding them to all other pixel positions when
      # using the resulting asTrans files. The number of missing
      # pixels to the _left_ of an object in the left/right side
      # of the CCD are $astrom_dcol_{left,right}
      #
      set astrom_dcols [get_astrom_dcol \
			    [exprGet $ccdpars($f).colBinning] \
			    [exprGet $ccdpars($f).namps]]
      set astrom_dcol_left($f) [keylget astrom_dcols "left"]
      set astrom_dcol_right($f) [keylget astrom_dcols "right"]
      #
      # Display SCPARAMBS stars?
      #
      if {$display && $display_scFangs} {
	 if {$display_scFangs == 1} {
	    set msg "Good scFang stars"
	 } elseif {$display_scFangs == 2} {
	    set msg "scFang stars (ignoring statusFit \[red\])"
	 } else {
	    set msg "All scFang stars (bad statusFit \[red\])"
	 }

	 display_fangs $display_scFangs $msg
      }
      #
      # Loop over stars
      #

      loop i 0 [chainSize $scparambs($field,$f)] {
	 set star [chainElementGetByPos $scparambs($field,$f) $i]
	 set star1c [chainElementGetByPos $star1clist($field) $i]

         set statusFit [exprGet $star.statusFit]
         set sscStatus [exprGet $star.sscStatus]
         set counts    [exprGet $star.counts]

	 set Creg "*$star1c.star1<$ifilter>->object->region"
	 #
	 # Possibly look at psp status flags and override statusFit
	 #
	 if $ignore_ssc_statusFit {
	    set star1c_flags [exprGet (long)$star1c.flags]
	    set star1_flags [exprGet (long)$star1c.star1<$ifilter>->flags]
	    set star1_badstar [exprGet $star1c.star1<$ifilter>->badstar]

	    set pspStatus 0;		# good
	    if {($star1c_flags & $star1c_require_mask) !=$star1c_require_mask||
		($star1_flags & $star1_require_mask) != $star1_require_mask} {
	       incr pspStatus
	    }
	    if {($star1c_flags & ~$star1c_ignore_mask) != 0 ||
		($star1_flags & ~$star1_ignore_mask) != 0} {
	       incr pspStatus
	    }
	    if {$star1_badstar != 0} {	# failed clipping on U/Q/cell/sigma/b
	       incr pspStatus
	    }
	    if {$Creg == ""} {		# no region
	       incr pspStatus
	    }
	    
	    if {$pspStatus == 0} {
	       if {$statusFit != 0} {
		  set statusFit 0;	# Override SSC's opinion
		  #
		  # Fill out various fields that ssc decided to leave blank
		  #
		  set_scparambs_from_star1c $star $star1c $ifilter
		  set counts [exprGet $star.counts]
	       }
	    } else {
	       set statusFit 0xfffe
	    }

	    handleSet $star.statusFit $statusFit
	 }

         # definition from astrom
	 if {$statusFit == 0 && ($sscStatus == 0 || $sscStatus == 4)} {
             set good 1 
         } else {
             set good 0
         }

	 if {!$good || ($counts < $debias_centroid_counts_min)} {
            # this star doesn't pass the quality cuts
	    handleSet $star.sscStatus 0xffff
	 } else {
	    if {$Creg == ""} {
	       # this is a garbage stamp that doesn't have any info (not even syncreg)
	       handleSet $star.sscStatus 0xffff
	       continue
	 } 
	     #
	     # Estimate bias using the KL PSF
	     #
	     set rowc [exprGet $star.rowCentroid]
	     set colc [exprGet $star.colCentroid]
	     
	     if [catch {
		 set ans [centroidAndDebias $Creg $psfBasis($f) $psf \
			      $rowc $colc -sky $sky -bkgd $sky \
			      -gain $gain -dark_variance $dark_variance]
	     } msg] {
		     handleSet $star.sscStatus 0xffff
	     } else {
		 handleSet $star.rowCentroid    [keylget ans rowc]
		 handleSet $star.rowCentroidErr [keylget ans rowcErr]
		 handleSet $star.colCentroid    [keylget ans colc]
		 handleSet $star.colCentroidErr [keylget ans colcErr]

		 if {$QAoutfile != ""} {
		     set rowBias [expr [keylget ans rowc] - $rowc] 
		     set rowErr [keylget ans rowcErr]
		     set colBias [expr [keylget ans colc] - $colc] 
		     set colErr [keylget ans colcErr]
		     set SrowErr [format "%6.3f" $rowErr]
		     set ScolErr [format "%6.3f" $colErr]
		     set SrowBias [format "%6.3f" $rowBias]
		     set ScolBias [format "%6.3f" $colBias]
		     set line "  $ifilter    $field   $psfWidth     $rowc  $colc  $SrowErr \
                             $ScolErr    $SrowBias $ScolBias [exprGet $star.counts]"
		     puts $outf $line
		 }
	     }
	     
	     #fix column centroids (may be no-op, e.g for unbinned data)
	     set colc [exprGet $star.colCentroid] 
	     handleSet $star.colCentroid \
		[expr $colc + \
		     (($colc <= [exprGet $ccdpars($f).nDataCol]/2) ? \
			  $astrom_dcol_left($f) : $astrom_dcol_right($f))]
	 }
      }
      #
      # Display SCPARAMBS stars as written to the psFang file
      #
      if {$display && $display_psFangs} {
	 display_fangs $display_psFangs "psFang stars"
      }
   }
   
   handleDel $psf

   if {$QAoutfile != ""} {
       close $outf
   }
   #
   # Repack for write_fang_file
   #
   foreach el [array names scparambs $field,*,hdr] {
      regexp "$field,(\[^,\]*)" $el {} f
      if {$f == "pdu"} {
	 continue
      }
      
      set params($f) $scparambs($field,$f)
      set headers(params,$f) $scparambs($field,$f,hdr)

      if [info exists astrom_dcol_left($f)] {
	 hdrInsWithDbl $headers(params,$f) "DCOL_L_[string toupper $f]" \
	     $astrom_dcol_left($f) "offset ADDED to colc (CCD left 1/2)"
	 hdrInsWithDbl $headers(params,$f) "DCOL_R_[string toupper $f]" \
	     $astrom_dcol_right($f) "offset ADDED to colc (CCD right 1/2)"
      }
   }
   #
   # Write the psFang file
   #
   set filternames [canonizeFilterlist [array names params]]
   write_fang_file $run $camCol $field $filternames $outDir \
       -params params -headers headers -prefix "ps" \
       -ko_ver [hdrGetAsAscii $scparambs($field,pdu,hdr) "KO_VER"] \
       -ssc_id [hdrGetAsAscii $scparambs($field,pdu,hdr) "SSC_ID"]

  return ""

}

#
# Do the work of displaying a set of stars destined for psFang files
#
proc display_fangs {display_fangs msg} {
   upvar scparambs scparambs  field field  f f  ifilter ifilter \
       star1clist star1clist
   global stampsize
      
   proc bad_scparambs_proc {i star1} {
      upvar scparambs scparambs  field field  f f
      upvar display_fangs display_fangs;# don't use global value
      
      global STAR1 
      
      set star [chainElementGetByPos $scparambs($field,$f) $i]
      set statusFit [exprGet $star.statusFit]
      set sscStatus [exprGet $star.sscStatus]
      handleDel $star
      
      if {$display_fangs > 1} {	# display a wider range of stars
	 if {$display_fangs == 2} {	# ignore statusFit
	    set statusFit 0;		
	 } else {
	    return 0;		# display all stars
	 }
      }
      
      if {$statusFit == 0 && ($sscStatus == 0 || $sscStatus == 4)} {
	 if {[exprGet (long)$star1.flags] & $STAR1(SATURATED)} {
	    return 1;		# centroidAndDebias will fail; Bad
	 }
	 return 0;			# Good
      } else {
	 return 1;			# Bad
      }
   }
   
   set mosaic [star1_mosaic $star1clist($field) $stampsize ids $ifilter \
		   bad_scparambs_proc]
   
   if {[saoGetRegion] != ""} {		# an saoImage exists
      saoReset
      
      loop i 0 [chainSize $scparambs($field,$f)] {
	 set id [lindex $ids $i]
	 
	 if {$id == "BAD"} {
	    continue;
	 }
	 
	 set star [chainElementGetByPos $scparambs($field,$f) $i]
	 set statusFit [exprGet $star.statusFit]
	 set sscStatus [exprGet $star.sscStatus]
	 handleDel $star

	 set star1c [chainElementGetByPos $star1clist($field) $i]
	 set star1c_id [exprGet $star1c.id]
	 set star1_badstar [exprGet $star1c.star1<$ifilter>->badstar]
	 handleDel $star1c

	 if {$statusFit == 0} {
	    set ei "-i"
	 } else {
	    set ei "-e"
	 }

	 set info [lindex $id 2]
	 set starid [lindex $info 0]
	 if 1 {				# append unique STAR1 ID
	    append starid "\[$star1c_id\]"
	 }
	 set star1_flags [lindex $info 1]
	 set star1_frame [lindex $info 2]
	 saoDrawText [lindex $id 0] [lindex $id 1] \
	     [format "%s:%s:%s:%s:0x%x:%d" $starid $star1_frame \
		  $statusFit $sscStatus $star1_flags $star1_badstar]
	 draw_frame [lindex $id 0] [lindex $id 1] $stampsize 1 $ei
      }
   }
   display $mosaic " $msg; field $field $f " "" 1
   regDel $mosaic;
}
