#!/usr/local/bin/perl
#
# Run ctags on all of the files in the include directory.
# parse the output to find the typededfs.
# generate an html file which indexes types to the files defining them
#

#
# ctags gets confused by typedefs such as
# typedef struct FOO {
#    ...
# } FOO;
# (it'd like the first FOO to be, e.g., foo). But that's OK; if there are
# real problems in the header files the compiler will find them. So we'll
# throw away error output.
#
open(INPUT, " ctags -tx ../../include/*.h 2>/dev/null |");

print <<"EOF";
<HTML>
<HEAD>

<TITLE>Structures in PHOTO</TITLE>
<BODY>

<H1>Structures in PHOTO</H1>

<P>
The following is a list of all the types defined for 
the PHOTO product.  Click a struct to see its include file.
<P>
From TCL, get a new struct (ie OBJECT1) using structNew 
   (ie object1New).  Some of the commands require extra information.
   Use the TCL help commmand (ie help object1New) to get specific
   information.  To get rid of a struct (and its handle), use
   structDel (ie object1Del).
<P>
The prototypes for the C level new and delete routines are in the
   include files for each structure.
<DIR> \n
EOF
    
while (<INPUT>) {
    if (!/#define/) {
       ($type, $lineon, $file)  = split;
       print "<LI> <A HREF=$file> $type </A> \n";
    }
}
print "</DIR>\n"
