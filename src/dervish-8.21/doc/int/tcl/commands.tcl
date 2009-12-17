# These are some illustrative and perhaps even useful tcl commands:
echo Reading in sample tcl commands:

proc rfits {fname} \
  {regReadAsFits [regNew] $fname }

proc lfits {dir} \
  {exec ls $dir | grep .fit}

proc rallfits {dir} \
  {foreach fname [split [lfits $dir ] ] \
  {echo $fname --- ; rfits $dir/$fname}}

proc clrall {} \
  {foreach rn [regListNames] \
  {regDel $rn}}

proc status {} \
  {foreach regname [regListNames] \
  { echo $regname; echo [regHdrGetLine $regname OBJECT]}}
