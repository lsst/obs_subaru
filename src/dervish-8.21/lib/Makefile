SHELL = /bin/sh
#
default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

library :
	( cd ../src ; $(MAKE) $(PMFLAGS) library )

LIBS = libdervish.a libcontrib.a libcontrib_dummy.a libdervishnotcl.a

all:	library

install :
	@rm -rf         $(DERVISH_INSTALL_DIR)/lib
	@mkdir          $(DERVISH_INSTALL_DIR)/lib
	@cp -p $(LIBS)  $(DERVISH_INSTALL_DIR)/lib
	@cp -p Makefile $(DERVISH_INSTALL_DIR)/lib
	-@if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		for f in $(LIBS); do \
			$(RANLIB) $(DERVISH_INSTALL_DIR)/lib/$$f; \
		done; \
	fi
	@chmod 444 $(DERVISH_INSTALL_DIR)/lib/*.a

clean :
	@$(SDSSTOOLS_DIR)/bin/clean
	rm -f *.a
