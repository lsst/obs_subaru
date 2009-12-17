SHELL=/bin/sh

#
# Makefile for contrib/www area. There are three targets:
#
#    all     : a dummy target
#    clean   : get rid of extra files
#    install : install the HTML scripts in destination
#

.IGNORE:
.SUFFIXES: .html

all:

clean:
	@$(SDSSTOOLS_DIR)/bin/clean

install :
	@rm -fr         $(DERVISH_INSTALL_DIR)/contrib/www
	@mkdir -p       $(DERVISH_INSTALL_DIR)/contrib/www
	@cp -p *.html   $(DERVISH_INSTALL_DIR)/contrib/www
	@chmod 644      $(DERVISH_INSTALL_DIR)/contrib/www/*.html
	@cp -p Makefile $(DERVISH_INSTALL_DIR)/contrib/www
	@chmod 644      $(DERVISH_INSTALL_DIR)/contrib/www/Makefile
