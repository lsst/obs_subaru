
dumpOpen /usrdevel/s1/heidi/IRIX/photodata/frames_1/output/objclist-10-1-0.dump r
set chain [dumpHandleRead]
dumpClose
set tbl [obj2Tbl 3]
set tblcol [schemaToTbl $chain $tbl -schemaName=OBJC]
fitsWrite $tblcol junk1 -binary -pdu=MINIMAL -standard=F

set tblcol [handleNewFromType TBLCOL]
fitsRead $tblcol junk1 -hdu=1
set schema [chainNew OBJC]
tblToSchema $tblcol $schema $tbl -proc={objcNew 3} -schemaName=OBJC
schemaToTbl $schema $tbl -schemaName=OBJC

