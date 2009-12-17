SHELL = /bin/sh
#
default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1
#
all:
	@echo all target does nothing - make complete
#
install :
	@if [ "$(DERVISH_INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(DERVISH_INSTALL_DIR)/include
	@ mkdir  $(DERVISH_INSTALL_DIR)/include
	cp -pr * $(DERVISH_INSTALL_DIR)/include
	@( \
		cd $(DERVISH_INSTALL_DIR)/include ;\
		find . \( -type d -name CVS -prune -o \
			  -name \*~ \) -exec rm -rf {} \; \
	)
	chmod 644 $(DERVISH_INSTALL_DIR)/include/*
	chmod 777 $(DERVISH_INSTALL_DIR)/include/prvt
	chmod 644 $(DERVISH_INSTALL_DIR)/include/prvt/*
#
clean :
	@touch touched~
	@$(SDSSTOOLS_DIR)/bin/clean
	( cd prvt ; $(MAKE) $(MFLAGS) clean )
