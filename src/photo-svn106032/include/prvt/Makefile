all:
clean :
	@touch touched~
	rm -f *~ core *.bak *.orig *.old .#* #*#
install :
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	cp *.h $(PHOTO_DIR)/include/prvt
	cp Makefile $(PHOTO_DIR)/include/prvt
