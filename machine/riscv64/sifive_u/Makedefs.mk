include $(HERE)/architecture/riscv64/Makedefs.mk

MACH_CCFLAGS += -march=rv64g_zicsr -mabi=lp64

%.img: %.bin
	mkdir -p $(dir $@)
	cp -f $< $@
