SHELL = /bin/sh
#
all : photo
	@ for f in lib test doc readAtlasImages; do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) all ); \
	done
photo : ~~photo
	@sleep 0
~~photo :
	@(cd src; $(MAKE) $(MFLAGS) all)
photo.p : 
	@(cd src; $(MAKE) $(MFLAGS) ../bin/photo.p)
photo.pg : 
	@(cd src; $(MAKE) $(MFLAGS) ../bin/photo.pg)
tests :
	-(cd test; $(MAKE) $(MFLAGS) all )
	-(cd readAtlasImages; $(MAKE) $(MFLAGS) tests )
datafiles :
	(cd lib; $(MAKE) $(MFLAGS) all )
#
make :
	@echo "Making make files"
	@echo ""
	@ for f in src; do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) make ); \
	done
#
# Install things in their proper places in $(PHOTO_DIR)
#
install :
	@echo "You should be sure to have updated before doing this."
	@echo "You will be installing in \$$PHOTO_DIR=$$PHOTO_DIR"
	@echo ""
	@echo "I'll give you 5 seconds to think about it"
	@sleep 5
	@echo
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@ rm -rf $(PHOTO_DIR)
	@ mkdir $(PHOTO_DIR)
	@ for f in bin doc etc include lib readAtlasImages src test ups; do \
		(mkdir $(PHOTO_DIR)/$$f; cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) install ); \
	done
	/bin/cp Makefile $(PHOTO_DIR)

tags :
	@ rm -f TAGS
	etags -a -o TAGS src/*.c include/*.h include/*/*.h src/*/*.[ch]
	@- : remove diskio_gen.c as it's a pain in emacs' tags-search
	@ perl -pi -e 'if(m|^src/diskio_gen|) { while(<>) { if(/\f/) { $$_=""; last; } } }' TAGS
	$$SDSSTOOLS_DIR/bin/tcl_tags -a -f TAGS etc/*.tcl src/tcl*.c

protect:
	@echo "Fixing directory and file permissions on $(PHOTO_DIR)..."
	@(cd $(PHOTO_DIR); \
	 find . -perm 664 -exec chmod 444 {} \; ; \
	 find . -perm 775 -exec chmod 555 {} \; ; \
	 find . -perm 660 -exec chmod 444 {} \; ; \
	 find . -type d -exec chmod 555 {} \;)
#
clean :
	@ echo in .
	@ rm -f ~~photo
	rm -f *~ core *.bak *.orig *.old .#* #*#
	@ for f in bin doc etc include lib src test ups; do \
			(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) clean); \
	done



