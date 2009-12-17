SHELL = /bin/sh
#
clean :
	@ $(SDSSTOOLS_DIR)/bin/clean
install :
	rm -rf $(DERVISH_INSTALL_DIR)/etc
	mkdir  $(DERVISH_INSTALL_DIR)/etc
	cp -p Makefile *.tcl $(DERVISH_INSTALL_DIR)/etc
	@chmod 644 $(DERVISH_INSTALL_DIR)/etc/*.tcl
