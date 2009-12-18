######################################
# atDb.tcl
#
# Interact with reasonable databases in dervish
#
#

set list ""
foreach element "brand host database tempFileRoot tempFileName clean" {
    set list "$list char ${element}<100>\;"
}
typedef struct $list ATDB

set atDbNewArgs {
    {atDbNew "Create a new atDb structure"}
    {-brand        STRING  postgres brand         "Database brand"}
    {-host         STRING  local    host          "Host machine"}
    {-database     STRING  database database      "Database name"}
    {-tempFileRoot STRING /tmp      tempFileRoot  "Location of temporary files"}
    {-tempFileName STRING default   tempFileName "default is brand-pid"}
    {-clean        STRING 1         clean        "1 to clean up temporary input file"}
}
ftclHelpDefine atDb atDbNew [shTclGetHelpInfo atDbNew $atDbNewArgs]
proc atDbNew { args } {
    upvar \#0 atDbNewArgs formal_list
    if {[shTclParseArg $args $formal_list atDbNew] == 0} {return}
    set db [genericNew ATDB]
    if {$tempFileName == "default"} {set tempFileName $brand-[pid].txt}
    foreach pair [schemaGetFromType ATDB] {
	set name [lindex $pair 0]
	handleSet $db.$name [set $name]
    }
    return $db
}

set atDbQueryArgs {
    { atDbQuery "Perform a query" }
    { <atDb>      STRING ""  atDb  "atDb structure from atDbNew"}
    { <query>     STRING ""  query "The sql query to perform"}
    { <schema>    STRING ""  schema "schema for return chain; NULL to return nothing"}
}
ftclHelpDefine atDb atDbQuery [shTclGetHelpInfo atDbQuery $atDbQueryArgs]
proc atDbQuery { args } {
    upvar \#0 atDbQueryArgs formal_list
    if {[shTclParseArg $args $formal_list atDbQuery] == 0} {return}
    set host [exprGet $atDb.host] 
    if {$host == "local" } {
	set hostString " "
    } else {
	set hostString "-h $host"
    }
    set tempFileRoot [exprGet $atDb.tempFileRoot]
    set tempFileName [exprGet $atDb.tempFileName]
    set outFile $tempFileRoot/$tempFileName
    case [exprGet $atDb.brand] {
	postgres {
	    exec echo $query | psql -t -h $host [exprGet $atDb.database] > $outFile
	    if {$schema != "NULL"} {
		set chain [chainNew $schema]
		set schemaList [schemaGetFromType $schema]
		set nameList ""
		foreach p $schemaList {lappend nameList [lindex $p 0]}
		set n [llength $nameList]
		for_file line $outFile {
		    if {[llength $line] > 0} {
			set o [genericNew $schema]
			set sline [split $line |]
			loop i 0 $n {
			    handleSet $o.[lindex $nameList $i] [lindex $sline $i]
			}
			chainElementAddByPos $chain $o
		    }
		}
	    }
	    if {[exprGet $atDb.clean] == 1} {
		exec rm -f $outFile
	    }
	    return $chain
	}
	default {
	    error "I do not know how to work with [exprGet $atDb.brand]"
	}
    }
}
