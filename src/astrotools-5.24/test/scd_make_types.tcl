# create all file type making sure to wrap data
proc create {type stand dim } {
set wrap_reg [regNew -type $type $dim $dim ]
for {set j 0} {$j < $dim} {incr j} {
for {set i 0} {$i < $dim} {incr i} {
  set val [expr $i+65530+($i+65530)*$j]
  if {$val > 65536} {
     set val [ expr $val % 65536]
  }
  regPixSetWithDbl $wrap_reg $i $j $val
}}
regWriteAsFits $wrap_reg wrap$type$stand.fits -standard $stand
}

create S16 T 16
create U16 T 16
create U16 F 16
