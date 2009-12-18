#! /usr/local/bin/perl
#  (above path set for the Fermilab location of perl)
#
# astropage
#
# parse all at*.c, tcl*.c, and at*.tcl files to extract 
# all the TCL procedures and public C functions
#
# create HTML pages with list of TCL, and C routines
#
# numerous print statements, useful for diagnostics, have been  
# commented out

#-----------------------------------------------------
#  setup astrotools path and initial parameters
#-----------------------------------------------------
#
# find ASTROTOOLS_DIR
#

$isAstrotoolsSetup = 0;
foreach $key (keys %ENV) {
  if ($key eq "ASTROTOOLS_DIR" ) {
    $astrotoolsDir = $ENV{$key};
    $isAstrotoolsSetup = 1;
  }
}

if ( $isAstrotoolsSetup == 0 ) {
  print("error: astrotools has not been set up\n");
  exit;
}

$tclFuncCt = 0;
$cFuncCt = 0;
$tclFileOutName = "astrotools.tcl.list.html";
$cFileOutName = "astrotools.C.list.html";

print("   astropage.pl scanning :  $astrotoolsDir\n\n");

#-----------------------------------------------------
#  scan through tcl*.c files in astrotools/src
#-----------------------------------------------------
#
# scan through each of the tcl*.c files
#    get the actual file name - without '.c' -- from the full path
#    input each file line looking for line with 'VERB'
#    get name of TCL routine for  '@tclFuncName'
#

@tclFiles = glob("$astrotoolsDir/src/tcl*.c");

foreach $tclFilePath ( @tclFiles ) {

  @tclFilePathList = split(/\//,$tclFilePath);
  $tclFileName  = pop( @tclFilePathList);
  chop($tclFileName);
  chop($tclFileName);

# print("     $tclFileName\n");

  open( TCL_FILE, $tclFilePath );
  while( <TCL_FILE> ) {
    if ( /VERB/ ) {
      @wordList = split;

      foreach $word ( @wordList ) {
        if ( ($word !~ /\*/) && ($word !~ /TCL/) && ($word !~ /VER/) ) {
          $tclFuncName[$tclFuncCt] = $word;
          $tclFuncModule[$tclFuncCt] = $tclFileName;
          $tclFuncCt++;
        }
      }
    }
  } # end while TCL_FILE
  close( TCL_FILE );

} # end foreach

#-----------------------------------------------------
#  scan through at*.tcl files in astrotools/etc
#-----------------------------------------------------
#
# scan through each of the at*.tcl files
#    get the actual file name - without '.tcl' -- from the full path
#    input each file line looking for line with 'proc'
#    get name of TCL procedures for  '@tclFuncName'
#

@procFiles = glob("$astrotoolsDir/etc/at*.tcl");

foreach $procFilePath ( @procFiles ) {

  @procFilePathList = split(/\//,$procFilePath);
  $procFileName = pop( @procFilePathList );
  chop($procFileName);
  chop($procFileName);
  chop($procFileName);
  chop($procFileName);

# print("     $procFileName\n");

  open( PROC_FILE, $procFilePath );
  while( <PROC_FILE> ) {
    if ( /\bproc\b/ ) {
      @wordList = split(/\s+/,$_);
      $tclFuncName[$tclFuncCt]   = $wordList[1];
      $tclFuncModule[$tclFuncCt] = $procFileName;
      $tclFuncCt++;
    }
  } # - end while

  close( PROC_FILE );
}
$maxNumTclFunc = $tclFuncCt;

#-----------------------------------------------------
#  list all tcl functions and procedures found
#-----------------------------------------------------

# for ( $j = 1; $j < $maxNumTclFunc; $j++ ) {
#   print( "$tclFuncName[$j] : $tclFuncModule[$j]\n");
# }

#-----------------------------------------------------
#  sort list of tcl functions and procedures 
#-----------------------------------------------------

for ( $j = 1; $j < $maxNumTclFunc; $j++ ) {
  $tempName   = $tclFuncName[$j];
  $tempModule = $tclFuncModule[$j];
  $i = $j - 1;
  while ( ($i >= 0) && ($tclFuncName[$i] gt $tempName) ) {
    $tclFuncName[$i+1]   = $tclFuncName[$i];
    $tclFuncModule[$i+1] = $tclFuncModule[$i];
    $i--;
  }
  $tclFuncName[$i+1]   = $tempName;
  $tclFuncModule[$i+1] = $tempModule;
}

#-----------------------------------------------------
#  scan through at*.c files in ../src and ../DA/src
#-----------------------------------------------------
#
# scan through each of the at*.c files
#    get the actual file name - without '.c' -- from the full path
#    input each file line looking for line with 'ROUTINE:'
#    get name of C function for  '@cFuncName'
#    (static functions are ignored)
#

@cFiles = glob("$astrotoolsDir/src/at*.c $astrotoolsDir/DA/src/at*.c");

foreach $cFilePath ( @cFiles ) {

  @cFilePathList = split(/\//,$cFilePath);
  $cFileName = pop( @cFilePathList );
  chop( $cFileName );
  chop( $cFileName );

# print("     $cFileName\n");

  open( C_FILE, $cFilePath );
  while( <C_FILE> ) {
    if ( /ROUTINE:/ ) {
      @wordList = split(/\s+/,$_);

      $index = 0;
      foreach $word ( @wordList ) {
        if ( $word =~ /ROUTINE/ ) {
          if ( $cFileName ne "atPMatch" ) {
            $cFuncName[$cFuncCt]   = $wordList[$index+1];
            $cFuncModule[$cFuncCt] = $cFileName;
            $cFuncCt++;
          } else { 
#
# look for special case in atPMatch, where static functions have ROUTINE
#
            if ( $wordList[$index+1] =~ /\bat/ ) { 
              $cFuncName[$cFuncCt]   = $wordList[$index+1];
              $cFuncModule[$cFuncCt] = $cFileName;
              $cFuncCt++;
            }
          }
        }  
      $index++;
      }
    } 

  } # end while

  close( C_FILE );
}
$maxNumCFunc = $cFuncCt;


#-----------------------------------------------------
#  list all C functions found
#-----------------------------------------------------

# for ( $j = 1; $j < $maxNumCFunc; $j++ ) {
#   print( "$cFuncName[$j] : $cFuncModule[$j]\n");
# }

#-----------------------------------------------------
#  sort list of c functions
#-----------------------------------------------------

for ( $j = 1; $j < $maxNumCFunc; $j++ ) {
  $tempName   = $cFuncName[$j];
  $tempModule = $cFuncModule[$j];
  $i = $j - 1;
  while ( ($i >= 0) && ($cFuncName[$i] gt $tempName) ) { 
    $cFuncName[$i+1]   = $cFuncName[$i]; 
    $cFuncModule[$i+1] = $cFuncModule[$i];  
    $i--;
  }
  $cFuncName[$i+1]   = $tempName;
  $cFuncModule[$i+1] = $tempModule;
}

#-----------------------------------------------------
#  create an html page for the sorted list of tcl functions
#-----------------------------------------------------

$tclFileOutNameFull = ">" . $astrotoolsDir . "/doc/www/" . $tclFileOutName;

open( TCL_FILE_OUT, $tclFileOutNameFull);
#
# header stuff
#
print TCL_FILE_OUT "<HEAD>\n";  
print TCL_FILE_OUT "<TITLE>List of Astrotools TCL Routines</TITLE>\n";
print TCL_FILE_OUT "</HEAD>\n";
print TCL_FILE_OUT "<BODY>\n";

print TCL_FILE_OUT "<CENTER><H5>\n";
print TCL_FILE_OUT "[ <A HREF=\"astrotools.home.html\"> ASTROTOOLS Home </A> |\n";
print TCL_FILE_OUT "  <A HREF=\"astrotools.C.html\"> C interface </A> |\n";
print TCL_FILE_OUT "  <A HREF=\"astrotools.tcl.html\"> TCL interface </A>|\n";
print TCL_FILE_OUT "  <A HREF=\"astrotools.authors.html\"> Authors </A> ]\n";
print TCL_FILE_OUT "<BR>\n";
print TCL_FILE_OUT "[  <A HREF=\"http://www-sdss.fnal.gov:8000/\"> SDSS Home </A> |\n";
print TCL_FILE_OUT "   <A HREF=\"http://www-sdss.fnal.gov:8000/dervish/doc/www/dervish.home.html\"> Dervish Home </A> |\n";
print TCL_FILE_OUT "<A HREF=\"http://sdss.fnal.gov:8000/dss/doc/www/sdss.pipe.html\"> SDSS Pipelines </A> ]\n";
print TCL_FILE_OUT "</A>\n";
print TCL_FILE_OUT "</CENTER></H5>\n";
print TCL_FILE_OUT "<HR>\n";

print TCL_FILE_OUT "<H2>List of all TCL routines available in ASTROTOOLS</H2>\n";
print TCL_FILE_OUT "<P>";
#print TCL_FILE_OUT "Report broken links to <A HREF=\"mailto:rik\@fnal.gov\">Ron Kollgaard</A>.\n";

#
# generate list of links
#
print TCL_FILE_OUT "\n<UL>\n";
for ( $ct = 0; $ct < $maxNumTclFunc; $ct++ ) {
 
  $link1 = "  <LI><A HREF=\"" . $tclFuncModule[$ct] . ".html" . "#";
  $link2 = $tclFuncName[$ct] . "\">" . $tclFuncName[$ct] . "</A>";
  $link = $link1 . $link2;

  print TCL_FILE_OUT "$link\n";

}
print TCL_FILE_OUT "</UL>\n";

print TCL_FILE_OUT "</BODY>\n";
close( TCL_FILE_OUT );

$_ = $tclFileOutNameFull;
s/>//;
# print("   writing: $_ \n");

#-----------------------------------------------------
#  create an html page for the sorted list of c functions
#-----------------------------------------------------

$cFileOutNameFull = ">" . $astrotoolsDir . "/doc/www/" . $cFileOutName;

open( C_FILE_OUT, $cFileOutNameFull);
#
# header stuff
#
print C_FILE_OUT "<HEAD>\n";  
print C_FILE_OUT "<TITLE>List of Astrotools C functions</TITLE>\n";
print C_FILE_OUT "</HEAD>\n";
print C_FILE_OUT "<BODY>\n";

print C_FILE_OUT "<CENTER><H5>\n";
print C_FILE_OUT "[ <A HREF=\"astrotools.home.html\"> ASTROTOOLS Home </A> |\n";
print C_FILE_OUT "  <A HREF=\"astrotools.C.html\">C interface </A> |\n";
print C_FILE_OUT "  <A HREF=\"astrotools.tcl.html\"> TCL interface </A>|\n";
print C_FILE_OUT "  <A HREF=\"astrotools.authors.html\"> Authors </A> ]\n";
print C_FILE_OUT "<BR>\n";
print C_FILE_OUT "[  <A HREF=\"http://www-sdss.fnal.gov:8000/\"> SDSS Home </A> |\n";
print C_FILE_OUT "   <A HREF=\"http://www-sdss.fnal.gov:8000/dervish/doc/www/dervish.home.html\"> Dervish Home </A> |\n";
print C_FILE_OUT "<A HREF=\"http://sdss.fnal.gov:8000/dss/doc/www/sdss.pipe.html\"> SDSS Pipelines </A> ]\n";
print C_FILE_OUT "</A>\n";
print C_FILE_OUT "</CENTER></H5>\n";
print C_FILE_OUT "<HR>\n";

print C_FILE_OUT "<H2>List of all public C functions available in ASTROTOOLS</H2>\n";
print C_FILE_OUT "<P>";
#print C_FILE_OUT "Report broken links to <A HREF=\"mailto:rik\@fnal.gov\">Ron Kollgaard</A>.\n";

#
# generate list of links
#
print C_FILE_OUT "\n<UL>\n";
for ( $ct = 0; $ct < $maxNumCFunc; $ct++ ) {
 
  $link1 = "  <LI><A HREF=\"" . $cFuncModule[$ct] . ".html" . "#";
  $link2 = $cFuncName[$ct] . "\">" . $cFuncName[$ct] . "</A>";
  $link = $link1 . $link2;

  print C_FILE_OUT "$link\n";

}
print C_FILE_OUT "</UL>\n";

print C_FILE_OUT "</BODY>\n";
close( C_FILE_OUT );

$_ = $cFileOutNameFull;
s/>//;
# print("   writing: $_ \n");

#-----------------------------------------------------
#  done
#-----------------------------------------------------
