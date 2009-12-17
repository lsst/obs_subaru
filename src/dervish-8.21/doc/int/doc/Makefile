SHELL = /bin/sh

.IGNORE:
.SUFFIXES: .ps .dvi .tex .html

WWW = ./www/

BOOK = the_sdss_book

.html.tex :
	html2latex  < $< > $@
.tex.dvi :
	latex $<
.dvi.ps :
	dvips -o $@ $<

all: $(BOOK).ps

#
#
$(BOOK).tex : chapters.tex 
	echo " ";
#

HTML_FILES =    \
                $(WWW)goals.html \
                $(WWW)institutions.html \
                $(WWW)oversight.html \
                $(WWW)software.html \
                ../whoswho/people.html

$(BOOK).ps : $(BOOK).dvi
	-dvips $(BOOK) -o ../doc/$(BOOK).ps

$(BOOK).dvi : $(BOOK).tex chapters.tex
	-latex $(BOOK)

chapters.tex : $(HTML_FILES) 
	html2latex $(WWW)sdssintro.home.html $(HTML_FILES) \
								> chapters.tex



#
# Move into the distribution directory.
install :
	echo NOTE:  this document is installed from dervish/doc
#
#
clean :
	rm -f *.log *.toc *.aux
	@(SDSSTOOLS_DIR)/bin/clean
	rm -f *.log *.toc  *.aux  book.tex  chapters.tex appendicies.tex 
	rm -f *.dvi *~ core *.bak *.orig *.old .\#*
	rm -r www/*~



