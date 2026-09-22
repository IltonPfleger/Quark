include ../Makedefs.mk

ifneq ($(wildcard $(APPLICATION)/Makefile),)
$(BUILD)/$(APPLICATION).o:
	$(MAKE) -C $(APPLICATION) $@
else
include Template.mk
endif
