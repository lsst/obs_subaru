SHELL=/bin/sh
all:
	@ for f in www overview; do \
	(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) all); \
	done

clean :
	@ echo in .
	rm -f *~ core *.bak *.orig *.old .#* #*#
	@ for f in www overview design; do \
	if [ -d $$f ]; then (cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) clean); else echo $$f does not exist >&2; fi; \
	done
install:
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	/bin/rm -fr $(PHOTO_DIR)/doc
	mkdir $(PHOTO_DIR)/doc
	cp Makefile $(PHOTO_DIR)/doc
	cp htmlSearch $(PHOTO_DIR)/doc
	cp structs.pl $(PHOTO_DIR)/doc
	@ for f in design overview www; do \
		mkdir $(PHOTO_DIR)/doc/$$f; \
		(cd $$f; echo In $$f; $(MAKE) $(MFLAGS) install); \
	done

#		(mkdir $$f; cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) install ); \




