all:

latex :
	$(SDSSTOOLS_DIR)/bin/html2latex -menu -style report -opt 12pt \
				-2e -show_links 1 overview.html > overview.latex

clean :
	rm -f *~ core *.bak *.orig *.old .#* #*#
	rm -f *.aux *.dvi *.log *.toc overview.latex
make :
install:
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	@if [ "`ls -id $(PHOTO_DIR) | perl -p -e 's/\s+(\d+).*/\1/;'`" = `ls -id . | perl -p -e 's/\s+(\d+).*/\1/;'` ]; then \
		echo The destination is this directory! >&2; \
		exit 1; \
	fi
	@rm -rf $(PHOTO_DIR)/doc/overview
	@mkdir  $(PHOTO_DIR)/doc/overview
	cp *.html $(PHOTO_DIR)/doc/overview
	cp Makefile $(PHOTO_DIR)/doc/overview
