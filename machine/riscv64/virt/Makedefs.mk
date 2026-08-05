include $(HERE)/architecture/riscv64/Makedefs.mk

MACH_CCFLAGS += -march=rv64g_zicsr_zbb -mabi=lp64 -I$(HERE)/machine/virt
CCFLAGS += -I$(HERE)/machine/virt

%.img: %.bin
	$(MKDIR) -p $(dir $@)
	$(MV) -f $< $@
