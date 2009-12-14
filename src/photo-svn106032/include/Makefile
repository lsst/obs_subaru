SHELL = /bin/sh
#
install :
	@ if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(PHOTO_DIR)/include
	@ mkdir $(PHOTO_DIR)/include
	cp *.h $(PHOTO_DIR)/include
	cp Makefile $(PHOTO_DIR)/include
	@ for f in prvt; do \
		(mkdir $(PHOTO_DIR)/include/$$f; cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) install ); \
	done

clean :
	@touch touched~
	rm -f *~ core *.bak *.orig *.old .#* #*#
	( cd prvt ; $(MAKE) $(MFLAGS) clean )
