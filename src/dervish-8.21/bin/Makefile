SHELL = /bin/sh
#
default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all:
	@echo all target does nothing - make complete

install :
	@if [ "$(DERVISH_INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@rm -rf $(DERVISH_INSTALL_DIR)/bin
	@mkdir  $(DERVISH_INSTALL_DIR)/bin
	cp -p dervish make_io xdither Makefile\
           $(DERVISH_INSTALL_DIR)/bin

	chmod 755 $(DERVISH_INSTALL_DIR)/bin/*
clean :
	@$(SDSSTOOLS_DIR)/bin/clean
	rm -f make_io dervish xdither
