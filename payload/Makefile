include ../Makedefs.mk

ifneq ($(wildcard $(PAYLOAD)/Makefile),)
$(BUILD)/$(PAYLOAD).o:
	$(MAKE) -C $(PAYLOAD) $@
else
include Template.mk
endif
