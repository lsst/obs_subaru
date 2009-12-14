all: html photo.structs.html photo.index.html

photo.structs.html:
	@../structs.pl > photo.structs.html
photo.index.html:
	@../htmlSearch -index photo > photo.index.html

html: 
	c2html $(PHOTO_DIR)/src/tcl*.c > photo.tcl.html
	c2html $(PHOTO_DIR)/include/*.h > photo.structs.html
	c2html `ls $(PHOTO_DIR)/src/*.c $(PHOTO_DIR)/src/*/*.c | grep -v 'tcl'` > photo.c.html

clean :
	rm -f *~ core *.bak *.orig *.old .#* #*#
	rm -f photo.structs.html photo.index.html
make :
install:
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	@rm -rf $(PHOTO_DIR)/doc/www
	@mkdir  $(PHOTO_DIR)/doc/www
	cp *.html $(PHOTO_DIR)/doc/www
	cp Makefile $(PHOTO_DIR)/doc/www
