SHELL = /bin/sh
#
# There are three targets:
#	all:  to make the_dervish_book (.dvi and .ps) and int/primers.ps
#	clean: to get rid of extra files (core, *~, and so on)
#	install: copy selected parts to $DERVISH_DIR
#
#       Generate TeX for the_dervish_book
#

.IGNORE:
.SUFFIXES: .ps .dvi .tex .toc .html

WWW  = ./www/
WWW_NEW = ./www_new/
BIN  = ../bin/
SRC  = ../src/
SRCWWW = $(WWW)

SDSSTOOLS = $(SDSSTOOLS_DIR)/bin

BOOK = the_dervish_book

.html.tex :
	$(SDSSTOOLS)/html2latex  < $< > $@
.tex.dvi :
	latex $<
.dvi.ps :
	dvips -o $@ $<
	@- rm $*.aux $*.toc $*.dvi

# Organize the dervish doc so that documentation about features of dervish is
# grouped together.

REGION_SECTS = \
	$(WWW)dervish.region.html \
	$(SRCWWW)tclRegion.c.html \
	  $(WWW)tclRegUtils.html \
	  $(WWW)region.html \
	  $(WWW)regUtils.html \
	  $(WWW)regPhysical.html

MASK_SECTS = \
	$(WWW)dervish.mask.html \
          $(WWW)tclMask.html \
          $(WWW)tclMaskUtils.html \
          $(WWW)mask.html

HDR_SECTS = \
	$(WWW)dervish.hdr.html \
	  $(WWW)tclHdr.html \
	  $(WWW)hdr.html

CHAIN_SECTS = \
	$(WWW)dervish.chain.html \
	  $(WWW)tclChain.html \
	  $(WWW)shChain.html

VECTOR_SECTS = \
	$(WWW)dervish.vector.html \
	  $(WWW)tclVectorExpr.html \
	  $(WWW)tclVecChain.c.html \
	  $(WWW)vecOps.c.html \
	  $(WWW)vecChain.c.html

ARRAY_SECTS = \
	$(WWW)dervish.array.html \
	  $(WWW)tclArray.html \
	  $(WWW)shArray.html

FITSIO_SECTS = \
	$(WWW)dervish.fitsio.html \
	  $(WWW)tclFitsIo.html \
	  $(WWW)tclFits.html \
	  $(WWW)fitsIo.html \
	  $(WWW)shFits.html \
	  $(WWW)shTblAccess.html \
	  $(WWW)shTblRead.html \
	  $(WWW)shTblWrite.html

F2S_SECTS = \
	$(WWW)dervish.f2s.html \
	  $(WWW)tclTbl.html \
	  $(WWW)tclSchema.html \
	  $(WWW)shTbl.html \
	  $(WWW)shSchema.html \
	  $(WWW)tblDesign.html \
	  $(WWW)convDesign.html

PLOT_SECTS = \
	$(WWW)dervish.plotting.html \
	  $(WWW)tclPgplotSum.html \
	$(SRCWWW)tclPgplot.c.html \
	  $(WWW)tclPgplotVerbs1.html \
	  $(WWW)tclPgplotVerbs2.html \
	  $(WWW)tclPgplotVerbs3.html \
	  $(WWW)shPlotAsciiFile.html \
	  $(WWW)shPlot.html

IMDISP_SECTS = \
	$(WWW)dervish.imagedisp.html \
	$(SRCWWW)tclSaoDisplay.c.html \
	$(SRCWWW)tclSaoPan.c.html \
	$(SRCWWW)tclSaoMaskDisplay.c.html \
	$(SRCWWW)tclSaoDraw.c.html \
	$(SRCWWW)tclSaoRegion.c.html \
	$(SRCWWW)tclSaoMouse.c.html \
	$(SRCWWW)saoMaskDisplay.c.html \
	$(SRCWWW)saoDraw.c.html \
	$(WWW)fsaoFermiMods.html

HG_SECTS = \
	$(WWW)dervish.hgaf.html \
	$(WWW)tclHg.html \
	$(SRCWWW)hg.c.html

DUMP_SECTS = \
	$(WWW)dervish.dump.html \
	  $(WWW)tclDiskio.html \
	  $(WWW)diskio.html

MEMORY_SECTS = \
	$(WWW)dervish.memory.html \
	  $(WWW)tclGarbage.html \
	  $(WWW)shGarbage.html

UTILITY_SECTS = \
	$(WWW)dervish.utilities.html \
	$(SRCWWW)tclRegPrint.c.html \
	  $(WWW)tclErrStack.html \
	  $(WWW)tclDirUtils.html \
	$(SRCWWW)tclParseArgv.c.html \
	  $(WWW)tclPerformance.html \
	  $(WWW)tclTapes.html \
	  $(WWW)tclHandle.html \
	  $(WWW)tclHistory.html \
	  $(WWW)tclP2c.html \
	  $(WWW)tclUtils.html \
	$(SRCWWW)handleExpr.c.html \
	  $(WWW)tclRegSupport.html \
	  $(WWW)handle.html \
	$(SRCWWW)regPrint.c.html \
	  $(WWW)errStack.html \
	  $(WWW)dirUtils.html \
	  $(WWW)utils.html

XDITHER_SECTS = \
	$(WWW)Xdither.c.html

TCLPROCS_SECTS = \
	$(WWW)tclProcedures.html

OTHER_SECTS = \
	$(WWW)dervish.other.html \
	$(WWW)program.feature.html \
	$(WWW)docs.feature.html \
	$(WWW)dervish.neat.verbs.html

DEVELOP_SECTS = \
	$(WWW)dervish.develop.html \
	$(WWW)examples.feature.html \
	$(WWW)dervish.adding.to.html \
	$(WWW)dervish.adding.test.scripts \
	$(WWW)dervish.FORTRAN.html

APPENDIX1 = $(WWW)dervish.exprGet.html $(WWW)dervish.new.html

HTML_FILES = $(REGION_SECTS) $(MASK_SECTS) $(HDR_SECTS) $(CHAIN_SECTS) \
	$(VECTOR_SECTS) $(ARRAY_SECTS) $(FITSIO_SECTS) $(F2S_SECTS) \
	$(PLOT_SECTS) $(IMDISP_SECTS) $(HG_SECTS) $(DUMP_SECTS) \
	$(MEMORY_SECTS) $(UTILITY_SECTS) $(XDITHER_SECTS) $(TCLPROCS_SECTS) \
	$(OTHER_SECTS) $(DEVELOP_SECTS)

WWW_NEW_FILES = $(HTML_FILES) $(APPENDIX1) $(WWW)dervish.home.html \
	$(DERVISH.AUTHORS.HTML) $(DERVISH.INDEX.HTML)

PRIMER_FILES = \
	int/primers.html \
	int/setup/setup.html \
        int/dervish/dervish.html \
        int/tcl/tcl.html \
        int/pgplot/pgplot.html \
        int/help/help.html \
        int/saodisplay/saodisplay.html \
        int/rcvs/rcvs.html \
        int/regions/regions.html \
        int/lists/lists.html \
        int/hg/hg.html 

#
# TARGETS
#
default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

#all: www_new www_index $(BOOK).ps int/primers.ps 
all: www_new www_index

# Get HTML files - converting from source files if necessary.
html : $(HTML_FILES)
	echo Done converting source files to HTML

$(BOOK).ps : $(BOOK).dvi
	-dvips $(BOOK) -o ../doc/$(BOOK).ps
	@- rm $(BOOK).aux $(BOOK).toc

$(BOOK).dvi : $(BOOK).tex chapters.tex appendix1.tex appendix2.tex
# Run LaTeX twice to get a .toc included.
	echo Run LaTeX twice:  first time
	@- rm  $(BOOK).aux $(BOOK).toc $(BOOK).dvi $(BOOK).ps
	-latex $(BOOK)
	echo Run LaTeX twice:  second time
	-latex $(BOOK)
#
chapters.tex : $(HTML_FILES) 
	$(SDSSTOOLS)/html2latex $(WWW)dervish.home.html $(HTML_FILES) > chapters.tex
#
www_new: $(WWW_NEW_FILES)
	@echo Begin www_new
	@- rm -rf $(WWW_NEW)
	mkdir $(WWW_NEW)
	./moveHtmlFiles.pl  $(WWW_NEW) $(WWW_NEW_FILES)
	./addButton.pl $(WWW_NEW)*
	./moveOtherFiles.pl $(WWW) $(WWW_NEW) $(WWW_NEW_FILES)

#
www_index: $(HTML_FILES)
	@echo Begin www_index
	$(SDSSTOOLS)/htmlSearch -index Dervish $(WWW_NEW) \
         > $(WWW_NEW)dervish.index.html 

#
appendix1.tex : $(APPENDIX1)
	$(SDSSTOOLS)/html2latex  $(APPENDIX1) > appendix1.tex

appendix2.tex : RELEASE_NOTES
	@cp RELEASE_NOTES ReleaseNotes
	@text2verbatim ReleaseNotes > appendix2.tex
	@rm ReleaseNotes
#
int/primers.ps: $(PRIMER_FILES)
	$(SDSSTOOLS)/html2latex $(PRIMER_FILES) > primer_chapters.tex
	@- rm primers.aux primers.toc primers.dvi primers.ps
	latex primers
	latex primers
	dvips primers
	@- rm primers.aux primers.toc

#
remButtons: $(HTML_FILES) $(APPENDIX1)
	./remButton.pl $(HTML_FILES) $(APPENDIX1)

#
# Move into the distribution directory.
install :
	@rm -rf           $(DERVISH_INSTALL_DIR)/doc
	@mkdir		 $(DERVISH_INSTALL_DIR)/doc
	cp -p Makefile $(DERVISH_INSTALL_DIR)/doc
	@if [ "`ls *.dvi 2> /dev/null`" != "" ]; then \
		echo cp -p *.dvi $(DERVISH_INSTALL_DIR)/doc; \
		cp -p *.dvi      $(DERVISH_INSTALL_DIR)/doc; \
	fi
	@if [ "`ls *.ps 2> /dev/null`" != "" ]; then \
		echo cp -p *.ps  $(DERVISH_INSTALL_DIR)/doc; \
		cp -p *.ps       $(DERVISH_INSTALL_DIR)/doc; \
	fi
	cp -pr int RELEASE_NOTES $(DERVISH_INSTALL_DIR)/doc
	@chmod 644 $(DERVISH_INSTALL_DIR)/doc/*
	@chmod 755 $(DERVISH_INSTALL_DIR)/doc/int
	@mkdir		         $(DERVISH_INSTALL_DIR)/doc/www
	cp -p www_new/*.html     $(DERVISH_INSTALL_DIR)/doc/www
	cp -p www_new/*.gif      $(DERVISH_INSTALL_DIR)/doc/www
	cp -p www_new/*.xbm      $(DERVISH_INSTALL_DIR)/doc/www
	cp -p www_new/*.scripts  $(DERVISH_INSTALL_DIR)/doc/www
	@chmod 644 $(DERVISH_INSTALL_DIR)/doc/www/*.html
	@chmod 644 $(DERVISH_INSTALL_DIR)/doc/www/*.gif
	@chmod 644 $(DERVISH_INSTALL_DIR)/doc/www/*.xbm
	@chmod 644 $(DERVISH_INSTALL_DIR)/doc/www/*.scripts
	@chmod 755 $(DERVISH_INSTALL_DIR)/doc/www

#
#
# Remove likely artifacts of building, but not the desirata 
tidy:
	rm -f *.log *.toc *.aux
	rm -f core *~ *.jou *.bak MAIN.0 MAIN.0.* *.orig *.old .'#'* '#'*'#'

#
# Scrub down to what would have been had from the source repository 
clean :
	@ echo in .
	@$(SDSSTOOLS_DIR)/bin/clean
	rm -f *.log *.toc  *.aux  book.tex  *chapters.tex appendix*.tex
	rm -f the*.ps *.dvi
	rm -f .uname~
	rm -rf www_new
	rm -f www/*.c.html
	@ for f in  int int/doc  int/fits int/help int/hg \
		int/lists int/pgplot \
		int/rcvs int/regions int/saodisplay int/setup int/dervish \
		int/style int/tcl int/whoswho ; do \
		if [ -d $$f ]; then \
			(cd $$f ; \
			 echo In $$f; \
			 rm -f chapters.tex *.aux *.dvi *.log *.toc *.o; \
			 rm -f pgplot.ps* ; \
			 rm -f '#'* *~ *.bak core *.o \
			); \
		else \
			echo Directory $$f does not exist; \
		fi; \
	done





make: 
	@/bin/rm -f Makefile~
	@mv Makefile Makefile~
	@sed 1,/\^#START_DEPEND/\!d Makefile~ > Makefile
	@echo "" >> Makefile
	@for h in $(HTML_FILES); do \
		f1="`echo $$h | sed -e 's%\\.c\\.html%%'`";\
		if [ $$f1 != $$h ]; then \
			f="`echo $$h | sed -e 's%\\./www/%%'  | sed -e 's%\\.html%%'`";\
			echo "$$""(WWW)"$$f.html:"	""$$""(SRC)"$$f >> Makefile;\
			echo "	"sed -e \"s/"<DATE>"/ This html file created on \`date\`/g\" "$$""(SRC)"$$f "|" "$$""(SDSSTOOLS)"/c2html ">" "$$""(WWW)"$$f.html >> Makefile ; \
		fi; \
	done


#=============================================================================

# include file dependencies.
# All lines below START_DEPEND are machine generated; Do Not Edit
#
#START_DEPEND

$(WWW)tclSaoPan.c.html:	$(SRC)tclSaoPan.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclSaoPan.c | $(SDSSTOOLS)/c2html > $(WWW)tclSaoPan.c.html
$(WWW)tclRegion.c.html:	$(SRC)tclRegion.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclRegion.c | $(SDSSTOOLS)/c2html > $(WWW)tclRegion.c.html
$(WWW)tclVecChain.c.html:	$(SRC)tclVecChain.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclVecChain.c | $(SDSSTOOLS)/c2html > $(WWW)tclVecChain.c.html
$(WWW)vecOps.c.html:	$(SRC)vecOps.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)vecOps.c | $(SDSSTOOLS)/c2html > $(WWW)vecOps.c.html
$(WWW)vecChain.c.html:	$(SRC)vecChain.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)vecChain.c | $(SDSSTOOLS)/c2html > $(WWW)vecChain.c.html
$(WWW)tclPgplot.c.html:	$(SRC)tclPgplot.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclPgplot.c | $(SDSSTOOLS)/c2html > $(WWW)tclPgplot.c.html
$(WWW)tclSaoDisplay.c.html:	$(SRC)tclSaoDisplay.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclSaoDisplay.c | $(SDSSTOOLS)/c2html > $(WWW)tclSaoDisplay.c.html
$(WWW)tclSaoMaskDisplay.c.html:	$(SRC)tclSaoMaskDisplay.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclSaoMaskDisplay.c | $(SDSSTOOLS)/c2html > $(WWW)tclSaoMaskDisplay.c.html
$(WWW)tclSaoDraw.c.html:	$(SRC)tclSaoDraw.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclSaoDraw.c | $(SDSSTOOLS)/c2html > $(WWW)tclSaoDraw.c.html
$(WWW)tclSaoRegion.c.html:	$(SRC)tclSaoRegion.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclSaoRegion.c | $(SDSSTOOLS)/c2html > $(WWW)tclSaoRegion.c.html
$(WWW)tclSaoMouse.c.html:	$(SRC)tclSaoMouse.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclSaoMouse.c | $(SDSSTOOLS)/c2html > $(WWW)tclSaoMouse.c.html
$(WWW)saoMaskDisplay.c.html:	$(SRC)saoMaskDisplay.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)saoMaskDisplay.c | $(SDSSTOOLS)/c2html > $(WWW)saoMaskDisplay.c.html
$(WWW)saoDraw.c.html:	$(SRC)saoDraw.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)saoDraw.c | $(SDSSTOOLS)/c2html > $(WWW)saoDraw.c.html
$(WWW)hg.c.html:	$(SRC)hg.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)hg.c | $(SDSSTOOLS)/c2html > $(WWW)hg.c.html
$(WWW)tclRegPrint.c.html:	$(SRC)tclRegPrint.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclRegPrint.c | $(SDSSTOOLS)/c2html > $(WWW)tclRegPrint.c.html
$(WWW)tclParseArgv.c.html:	$(SRC)tclParseArgv.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)tclParseArgv.c | $(SDSSTOOLS)/c2html > $(WWW)tclParseArgv.c.html
$(WWW)handleExpr.c.html:	$(SRC)handleExpr.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)handleExpr.c | $(SDSSTOOLS)/c2html > $(WWW)handleExpr.c.html
$(WWW)regPrint.c.html:	$(SRC)regPrint.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)regPrint.c | $(SDSSTOOLS)/c2html > $(WWW)regPrint.c.html
$(WWW)Xdither.c.html:	$(SRC)Xdither.c
	sed -e "s/<DATE>/ This html file created on `date`/g" $(SRC)Xdither.c | $(SDSSTOOLS)/c2html > $(WWW)Xdither.c.html
