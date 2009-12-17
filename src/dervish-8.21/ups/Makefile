SHELL = /bin/sh
#
default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1
#
all:
	@echo all target does nothing - make complete
#
install:
	@if [ "$(DERVISH_INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@rm -rf $(DERVISH_INSTALL_DIR)/ups
	@mkdir  $(DERVISH_INSTALL_DIR)/ups
	@cp -p  Makefile 	 $(DERVISH_INSTALL_DIR)/ups
	$(SDSSTOOLS_DIR)/bin/install_ups_table dervish.table $(DERVISH_INSTALL_DIR)/ups
#
clean :
	@$(SDSSTOOLS_DIR)/bin/clean
#	rm -f core *~ *.bak *.jou MAIN.0 MAIN.0.* *.orig *.old .'#'* '#'*'#'
