#
# TCL support code for FOOs and BARs
proc fooNew {} {
	 handleNewFromType FOO
}
proc fooDel {hndl} {
	 handleDelFromType $hndl
}

