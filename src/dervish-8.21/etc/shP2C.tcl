#PR5885 Commented out cmdtrace per PR5885


set tcl_precision 17
#
# param2Chain -- paramFile hdrPtr [-type=TYPE]
#	parse in an ASCII param file and put it in a Chain/Hdr pair.

set param2ChainArgs {
   {param2Chain "parse an ASCII param file an put it in a Chain/Hdr pair\n" }
   {<paramFile>  STRING "" paramFile "ASCII param file name"}
   {<hdrPtr> STRING "" hdrPtr "header variable"}
   {-type  STRING ""  type "schemaType to read into"}
   {-structList STRING ""  structList "only read these structs"}
}

ftclHelpDefine dervish param2Chain  [shTclGetHelpInfo param2Chain $param2ChainArgs]

proc param2Chain { args } {
    #
    # parse the args
    #
    #I dont think we want a global here, in any case making 'a' global is not a good idea
    #global a

    #set savecmdtrace [cmdtrace depth]

    # cmdtrace 0

    upvar #0 param2ChainArgs formal_list
    if {[shTclParseArg $args $formal_list param2Chain] == 0} {
	#PR5885 cmdtrace $savecmdtrace
        return
    }
    
    if { ![ftclPassed  $args $formal_list -type] } {
	set type NULL
    } else {
	set ll [schemaGetFromType $type]
	loop i 0 [llength $ll] {
	    set cur [lindex [lindex $ll $i] 0]
	    set typefields($cur) 1 
	}	
    }
    
    if { ![ftclPassed $args $formal_list -structList] } {
	set structList ""
    } else {
	#echo structList is $structList [llength $structList]
    }
    
    #
    # setup the hdr variable
    #
    
    upvar $hdrPtr hdr
    set hdr ""
    
    #
    # we'll need these later
    #
    set lbrace "\{"
    set rbrace "\}"
    
    #
    # open the ascii file
    #
    if [catch {
       set fd [open $paramFile r]
    } msg] {
       #PR5885 cmdtrace $savecmdtrace
       return -code error -errorinfo $msg
    }
    
    #
    # go through it line by line
    #
    while { ![eof $fd] } {
	
	set line [gets $fd]
	
	#strip comments and blank lines (are we worried about tabs?)
	
	regsub -all {	+} $line { } line
	if { [regexp {^ *#} $line] } { continue; }
	if { [regexp {^ *$} $line] } { continue; }
	set line [string trimright $line]
	while { [regexp {(.*)\\[ 	]*$} $line junk tline] } { 
	    set nextline [gets $fd]; 
	    append tline $nextline;
	    set line $tline;
	}
	
	#there's a tab in the next regexp {} ...
	regsub -all {	+} $line { } line
	regsub -all { +} $line { } line
	set line [string trim $line " "]
	
	# split the line into pieces separated by white-space,
	# are we worried about embedded {}? Not yet.
	
	
	#set a [split $line]
	# should allow embedded blanks...

	# Fix for PR 3505 -- insert spaces between double quotes
	set a [p2cDoubleQuoteFix $line]
	set k [lindex $a 0]
	set ku [string toupper $k]
	
	if { $ku == "TYPEDEF" } {
	    #echo $ku $a
	    set done 0
	    regsub {#.*} $line {} line 
            set typetype [string tolower [lindex $a 1]]
            set fullline $line
	    while { !$done } {
		    
		if { [eof $fd] } {
		    echo no closing brace for typedef
		    close $fd
		    #PR5885 cmdtrace $savecmdtrace
		    return -1
		}
		
		if { [regexp $rbrace $fullline junk]  } {
		    set done 1 
		} else {
		    set line [gets $fd]
		    regsub -all {	+} $line { } line
		    regsub {#.*} $line {} line 
		    set line [string trimright $line]
		    append fullline $line
		}
	    }
		
	    set kl [split $fullline ";"]
	    set kll [llength $kl]
	    set kli [expr $kll - 1]
	    set krs [lindex $kl $kli]
		
	    if { $krs == "" } {
		set kli [expr $kll -2]
		set krs [lindex $kl $kli]
	    } 
		
	    set pattern $rbrace
	    append pattern "(.*)\;?"
	    regexp $pattern $krs junk matchem
	    set keyname [string trim $matchem]
		
	    if { $type  == "NULL" } {
		
		if {[catch "schemaGetFromType $keyname"] } {
		    eval $fullline;
		}
		
	    }
		
	    if { $typetype == "struct" } {

		if { $type == "NULL" } {
		    set chn($keyname) [chainNew $keyname]
		    set ellist [schemaGetFullFromType $keyname]
		} else {
		    set chn($keyname) [chainNew $type]
		    set ellist [schemaGetFullFromType $type]
		}
		
		
		set len [llength $ellist]
		set counter 0
		loop i 1 $len { 
		    set cur [lindex $ellist $i]
		    set name [lindex $cur 0]
		    set elem($keyname$counter) $name
		    incr counter
		}
		set elem(${keyname}COUNT) $counter
	    }
  
	} else {
		
	    #echo ku is $ku keyname is $keyname
	    if { [info exists elem(${ku}COUNT)] } {
		
		#CHECK FOR STRUCTLIST
		    
		if { $structList == "" || [regexp $ku $structList junk]} {
			
			
		    if { $type == "NULL" } {
			set cur [genericNew $ku]
			set eltype $ku
			if { $elem(${ku}COUNT) != [expr [llength $a] -1] } {
			    echo "struct $ku - length mismatch between typedef and input"
			    echo "Offending line: $line"
			    #echo "param count mismatch $elem(${ku}COUNT) != [llength $a] -1"
			    #PR5885 cmdtrace $savecmdtrace
			    error "Trouble in param2Chain with args=$args"
			    return -1
			}
		    } else {
			set cur [genericNew $type]
			#set eltype $type
			set eltype $keyname
		    }
		    
		    
		    #echo [llength $a]
		    loop i 1 [llength $a] {
			set curval [lindex $a $i]
			set curval [string trim $curval {"}]
			
			set j [expr $i - 1]
			
			#need to handle embedded {} somewhere?
			
			handleSet $cur.$elem($eltype$j) $curval
		    }
			
			
		    chainElementAddByPos $chn($ku) $cur
		    
		    #clean up
		    handleDel $cur
		    
		    
		} 
		#END OF STRUCTCHECK

	    } else {
		    
		#echo got a hdr line... $ku
		
		if { $hdr == 0 } {
		    echo "must supply a hdr pointer to load header"
		    #PR5885 cmdtrace $savecmdtrace
		    return -1
		}
		#set value [lindex $a 1]
		
		regsub {\(} $k {\\(} kbrace
		regsub {\)} $kbrace {\\)} kbrace2
		regsub -all {\+} $kbrace2 {\\+} kbrace3
		regsub "$kbrace3 *" $line {} value
		
		regsub {#.*} $value {} value
		set value [string trim $value]
		lappend hdr "$k {$value}"
		
	    }
		
	}
	
    }
    close $fd
    
    if { [catch "array startsearch chn" el] } {
	#PR5885 cmdtrace $savecmdtrace
	return 
    } else {
	set n 0
	set output {}
	while { [array anymore chn $el] } {
	    set g [array nextelement chn $el]
	    if { $structList == "" || [regexp $g $structList junk] } {
		if { $n == 0 } {
		    set first $chn($g)
		} else {
		    append output " "
		}
		append output $chn($g)
		incr n
	    } else {
		chainDestroy $chn($g)
	    }
	}
	
	array donesearch chn $el
	
	if { $n == 1 } {
	    set output $first
	}
	#PR5885 cmdtrace $savecmdtrace
	return $output
    }
    
}

proc p2cDoubleQuoteFix {line} {
    # If there are not any double quotes in the line, return right away
    if {[string first \"\" $line] == -1} {return $line}
    set l [string length $line]
    if {$l > 0} {set out [string range $line 0 0]} else {return}
    set prev $out
    loop i 1 $l {
	set this [string range $line $i $i]
	if {$prev == $this} {
	    if {$this == "\""} {
		# Here is where we add an extra space
		set out "$out "
	    }
	}
	set out "$out$this"
	set prev $this
    }
    return $out
} 
   
    
#
# chain2Param -- paramFile chain [hdr]
#	dump a chain (with optional hdr) out to a ASCII param file

ftclHelpDefine procs chain2Param "chain2Param paramFile chain hdr \[-append\]"

set chain2ParamArgs {
    {chain2Param "dump a chain/hdr pair to an ASCII param file\n" }
    {<outfile>  STRING "" outfile "output ASCII param file name"}
    {<chnlist>  STRING "" chnlist "list of one of more chains to output"}
    {<hdr> STRING "" hdrp "header variable, may be empty string"}
    {-append  CONSTANT -append appendflag "append to existing file"}
    {-notypedump  CONSTANT -notypedump nodumpflag "don't dump typedef(s)"}
}

proc chain2Param {args} {
    
    upvar #0 chain2ParamArgs formal_list
    if {[shTclParseArg $args $formal_list chain2Param] == 0} {
	return
    }
    
    if { [ftclPassed  $args $formal_list -append] } {
	set appendmode 1
	set fd [open $outfile "a+"]
    } else {
	set appendmode 0
	set fd [open $outfile w]
    }
    
    if { [ftclPassed  $args $formal_list -notypedump] } {
	set notypedump 1
    } else {
	set notypedump 0
    }
    
    
    set lbrace "\{"
    set rbrace "\}"
    
    if { ($hdrp != "") && ($hdrp != "0") } {
	foreach i [keylkeys hdrp] {
	    if { [clength $i] >= 8 } { set tabs "   " } else {set tabs "            "}
	    set val [keylget hdrp $i]
	    #if {[string first " " $val]>0} { set val "{$val}"} 
	    puts $fd "$i$tabs$val"
	}
	
	puts $fd ""
	
    }
    
    if { $chnlist != "0" } {
	
	if { [llength $chnlist ] > 0 } {
	    
	    loop i 0 [llength $chnlist] {
		set chn [lindex $chnlist $i]
		#echo chn is $chn
		set out [chainTypeGet $chn]
		set type [keylget out type]
		
		set keyPrefix $type
		
		set enumlist ""

		set ellist [schemaGetFullFromType $type]
		set len [llength $ellist]
                set counter 0
                loop j 1 $len {
		    set cur [lindex $ellist $j]
		    set name [lindex $cur 0]
		    set selt [lindex $cur 1]
		    set mult [lindex $cur 3]
		    
		    if { [schemaKindGetFromType $selt] == "ENUM" } {
			set enumdump [schemaGetFromType $selt]
			set ekeys [keylkeys enumdump]
			append enumlist "typedef enum {\n"
			set nekeys [llength $ekeys]
			loop kk 0 $nekeys {
			    set cenum [lindex $ekeys $kk]
			    append enumlist "  $cenum"
			    if {[expr $kk + 1] < $nekeys} {
				append enumlist ",\n"
			    } else {
				append enumlist "\n"
			    } 
			}
			append enumlist "} $selt;\n"
		    }
		    
		    if { [regexp -nocase {^char$} $selt] } { set mult "NULL"}
		    set elem($type$counter) $name
		    set elem(${type}${counter}MULT) $mult
		    incr counter
		    
		}
		set elem(${type}COUNT) $counter
		
		set stgl [schemaGetFromType $type]
		set l ""
		set tl ""
		
		foreach i $stgl {
		    append l " " [lindex $i 0]
		    append tl " " [lindex $i 1]
		}
		
		if { ($enumlist != "") && ($notypedump==0)} {
		    puts $fd $enumlist
		}
		
		if { $notypedump == 0 } {
		    puts $fd "typedef struct $lbrace"
		    
		    foreach i $stgl {
			if { [regexp {\[} [lindex $i 1]] } {
			    regexp {(.*)(\[.*\])} [lindex $i 1] cc chdr clater
			    puts $fd "$chdr [lindex $i 0]$clater;"
			} else {
			    #echo lindex is [lindex $i 1]
			    puts $fd "[lindex $i 1] [lindex $i 0];"
			}
		    }
		    puts $fd "$rbrace $keyPrefix;"
		    puts $fd ""
		}
		
		loop k 0 [chainSize $chn] {
		    set cur [chainElementGetByPos $chn $k]
		    set val ${keyPrefix}
		    loop j 0 $elem(${type}COUNT) {
			set val2 [exprGet $cur.$elem(${type}$j) -enum -unfold -flat]
			#echo mult is $elem(${type}${j}MULT)
			if { $elem(${type}${j}MULT) == "NULL" } {
			    append val " " $val2
			} else {
			    append val " {" $val2 "}"
			}
		    }
		    puts $fd $val
                    handleDel $cur
		}
		puts $fd ""
		
	    }
	    
	}
	
    }
    
    close $fd
    
}

ftclHelpDefine procs hdr2Param \
	"
Dump a hdr to a param file
USAGE:  hdr2Param fname hdr
RETURN: 0
"

proc hdr2Param {fname hdr} {
    set file [open $fname w]
    foreach p $hdr {
	set name [lindex $p 0]
	set valu [lindex $p 1]
	puts $file "$name $valu"
    }
    close $file
    return 0
}


typedef struct {
    char defName[32];
    char attributeName[32];
    int  Value;  
}  DEFTABLE;


ftclHelpDefine procs enum2Define \
	"
Make one or more enum schema (or sets of bitmask flag values) into a 
dervish DEFTABLE chain which may then be appended to a FITS file with
schema2Fits <chn> <fits> -hdu <N>

The input enums are simply schema names which have been generated
by the make_io task from .h header files with #defines and enums.

The output chain of DEFTABLE entries lists the mapping between
the enum/bitmask attribute names and their integer values.
strings should not be more than 32 chars long.

INPUT: enum2Define <list of schema> \[optional DEFTABLE chain to append to\]
OUTPUT: chain of DEFTABLE entries
"

proc enum2Define {schemalist {chn ""} } {
    
    if { $chn == "" } {
	set chn [chainNew DEFTABLE]
    }
    
    loop i 0 [llength $schemalist] {
	set curs [lindex $schemalist $i]
	set curl [schemaGetFullFromType $curs]
	loop j 1 [llength $curl] {
	    set curx [lindex $curl $j]
	    set cur [genericNew DEFTABLE]
	    handleSet $cur.defName $curs
	    handleSet $cur.attributeName [lindex $curx 0]
	    handleSet $cur.Value [lindex $curx 2]
	    chainElementAddByPos $chn $cur
	    handleDel $cur
	}
    }
    
    return $chn
    
}

###############################################################################
#
# param2Vectors -- paramFile fieldList elemList hdr [-schema schema] [-size n]
#
#	Parse in an ASCII param file and put it in a Vector/Hdr pair.
# Non-recognised lines are put into the header, and a keyed list of vectors
# is returned.
#
# Note that this isn't as general as param2Chain (e.g. only numerical fields
# can be read), but it is _far_ faster (up to 100x)
#
# e.g.
#  set vecs [param2Vectors file.par [list frame timeSeq nQuartileR5] hdr]
#  vectorExprPrint [keylget vecs timeSeq]
#
# Specifying -size will prevent the neccessity of enlarging the vectors
# as they are being read
#
# If -type is specified, much of the tcl-level parsing of the header is
# skipped, saving several seconds of elapsed time on e.g. sdsshost
#
set param2VectorsArgs {
    {param2Vectors "parse an ASCII param file and put it a list of Vectors\n" }
    {<paramFile>  STRING "" paramFile "ASCII param file name"}
    {<elementList> STRING "" elemList "desired fields of struct"}
    {<hdrPtr> STRING "" hdrPtr "header variable"}
    {-size INTEGER 100 size "initial dimension of vectors"}
    {-struct STRING ""  struct "only read this struct from file"}
    {-type  STRING ""  type "schemaType to read into"}
    {-seek  STRING "" seek_pos \
	    "Start reading this many bytes from beginning of file, or end
    if negative.  This bypasses the parsing of the header, and can
    only be used with the -type flag"}
}

ftclHelpDefine dervish param2Vectors  [shTclGetHelpInfo param2Vectors $param2VectorsArgs]

proc param2Vectors { args } {
    #
    # parse the args
    #
    #PR5885 set savecmdtrace [cmdtrace depth]
    #PR5885 cmdtrace 0
    
    upvar #0 param2VectorsArgs formal_list
    if {[shTclParseArg $args $formal_list param2Vectors] == 0} {
	#PR5885 cmdtrace $savecmdtrace
	return
    }
    
    if { ![ftclPassed $args $formal_list -struct] } {
	set struct ""
    }
    
    if {![ftclPassed  $args $formal_list -type]} {
	set type NULL
    } else {
	set ellist [schemaGetFullFromType $type]
	set elem($type) 1
	
	if {$struct == ""} {
	    set struct $type
	} elseif {$type != $struct} {
	    #PR5885 cmdtrace $savecmdtrace
	    error "If you specify -type and -struct they must be identical"
	}
    }
    
    if ![ftclPassed  $args $formal_list -seek] {
	unset seek_pos
    }
    if {[info exists seek_pos] && $type == "NULL"} {
        #PR5885 cmdtrace $savecmdtrace
	error "You must specify -type with -seek"
    }
    #
    # setup the hdr variable
    #
    upvar $hdrPtr hdr
    set hdr ""
    
    #
    # we'll need these later
    #
    set lbrace "\{"
    set rbrace "\}"
    
    #
    # open the ascii file
    #
    set fd [open $paramFile r]
    set found_data 0
    #
    # If desired, seek to somewhere in the file and start reading there.
    # This bypasses the parsing of the header, and can be useful even
    # if seek_pos == 0. A negative value is taken to be relative to the
    # end of file, and is useful if you only want the last few elements
    # of the vectors
    #
    if [info exists seek_pos] {
	if [catch {
	    if {$seek_pos < 0} {
		seek $fd $seek_pos end
	    } else {
		seek $fd $seek_pos
	    }
	} msg] {
	   #PR5885 cmdtrace $savecmdtrace
	   close $fd
	   error "Cannot perform desired seek: $msg"
	}
	
	while 1 {
	    if {[gets $fd line] < 0} {	# we didn't find any structs
		seek $fd 0
		break;
	    }
	    if [regexp "^ *$type" [string toupper $line]] {# found our place
		seek $fd [expr -([string length $line]+1)] current
		
		set linestart [tell $fd];	# start of that line
		set found_data 1
		set k $type
		break
	    }
	}
    }
    #
    # go through it line by line
    #
    while !$found_data {
	set linestart [tell $fd]
	if {[gets $fd line] < 0} {
	    break;
	}
	
	#strip comments and blank lines (are we worried about tabs?)
	regsub -all {	+} $line { } line
	
	if [regexp {^ *(#.*)?$} $line] {
	    continue
	}
	
	#
	# read possible continuation lines
	#
	if [regsub {\\[ 	]*$} $line "" line] {
	    while 1 {
		set continuation [regsub {\\[ 	]*$} [gets $fd] "" nextline]
		append line $nextline
		if {!$continuation} {
		    break
		}
	    }
	}
	
	#there's a tab in the next regexp {} ...
	regsub -all {	+} $line { } line
	regsub -all { +} $line { } line
	set line [string trim $line " "]
	# split the line into pieces separated by white-space,
	# are we worried about embedded {}? Not yet.
	set a $line
	set k [lindex $a 0]
	set ku [string toupper $k]
	#
	# See what we have; if it's a typedef process it, otherwise
	# read it if we can --- and if we cannot add it the header
	#
	if {$ku == "TYPEDEF" && $type == "NULL"} {
	    set done 0
	    regsub {#.*} $line {} line 
	    set typetype [string tolower [lindex $a 1]]
	    
	    set fullline $line
	    while {!$done} {
		if [eof $fd] {
		    echo no closing brace for typedef
		    close $fd
		    #PR5885 cmdtrace $savecmdtrace
		    return -1
		}
		
		if [regexp $rbrace $fullline] {
		    set done 1 
		} else {
		    set line [gets $fd]
		    regsub -all {	+} $line { } line
		    regsub {#.*} $line {} line 
		    set line [string trimright $line]
		    append fullline $line
		}
	    }
	    
	    set kl [split $fullline ";"]
	    set kll [llength $kl]
	    set kli [expr $kll - 1]
	    set krs [lindex $kl $kli]
	    
	    if { $krs == "" } {
		set kli [expr $kll -2]
		set krs [lindex $kl $kli]
	    } 
	    
	    set pattern $rbrace
	    append pattern "(.*)\;?"
	    regexp $pattern $krs junk matchem
	    set keyname [string trim $matchem]
	    
	    if {[catch "schemaGetFromType $keyname"] } {
		eval $fullline;
	    }
	    
	    if { $typetype == "struct" } {
		set ellist [schemaGetFullFromType $keyname]
		set elem($keyname) 1
	    }
	} else {
	    if [info exists elem($ku)] {	#do we know this type?
		if { $struct != "" && $ku != $struct } {
		    continue;
		}
		set found_data 1;		# we've found our place
		break
	    } else {
		if { $hdr == 0 } {
		    echo "must supply a hdr pointer to load header"
		    #PR5885 cmdtrace $savecmdtrace
		    close $fd
		    return -1
		}
		
		regsub {\(} $k {\\(} kbrace
		regsub {\)} $kbrace {\\)} kbrace2
		regsub -all {\+} $kbrace2 {\\+} kbrace3
		regsub "$kbrace3 *" $line {} value
		
		regsub {#.*} $value {} value
		set value [string trim $value]
		lappend hdr "$k {$value}"
	    }
	}
    }
    #
    # Time to read the data
    #
    if {![info exists found_data]} {
	#PR5885 cmdtrace $savecmdtrace
	close $fd
	error "Failed to read any data from $paramFile"
    }
    #
    # We may only want the type
    #
    if {[llength $elemList] == 0} {
	close $fd
	#PR5885 cmdtrace $savecmdtrace
	return ""
    }
    #
    # Find which element is in which column
    #
    loop i 1 [llength $ellist] { 
	set cur [lindex $ellist $i]
	set name [lindex $cur 0]
	set cols($name) [expr $i+1];	# column 1 is the name of the type
    }
    
    foreach el $elemList {
	if {![info exists cols($el)]} {
	    append errs " $el";		# can't find this one
	    continue;
	}
	set vec [vectorExprNew $size]
	lappend answer [list $el $vec]
	lappend veclist [list $vec $cols($el)]
    }
    #
    # How did we do?
    #
    if {![info exists veclist]} {
	#PR5885 cmdtrace $savecmdtrace
	close $fd
	error "Failed to find any of desired fields in $paramFile"
    }
    
    if [info exists errs] {
	echo "Failed to find fields$errs in $paramFile"
    }
    #
    # Finally do the work!
    #
    if [catch {
	seek $fd $linestart
	vectorsReadFromFile $fd $veclist -type $k
    } msg] {
	#PR5885 cmdtrace $savecmdtrace
	close $fd
	foreach el $veclist { vectorExprDel [lindex $el 0] }
	
	error "Failed to read desired columns from $paramFile: $msg"
    }
    
    #PR5885 cmdtrace $savecmdtrace
    close $fd
    
    return $answer
}
