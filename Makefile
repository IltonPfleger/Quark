include Makedefs.mk

KERNEL_SOURCES       := $(shell find src -name '*.cpp' | grep -v '*architecture*' | grep -v '*machine*')
KERNEL_SOURCES 	     += $(shell find src/architecture/$(ARCH) -name '*.cpp')
KERNEL_OBJECTS       := $(patsubst src/%.cpp,$(BUILD)/%.o,$(KERNEL_SOURCES))
KERNEL_DEPENDENCIES  := $(KERNEL_OBJECTS:.o=.d)
PAYLOAD_ELF          := $(BUILD)/$(PAYLOAD).elf

run: $(IMAGE).img
	-$(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $<

debug: $(IMAGE).img
	-$(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $< -S -gdb tcp::1234

gdb:
	$(GDB) -ex "file $(KERNEL_ELF)" -ex "target extended-remote:1234"

$(IMAGE).bin: $(KERNEL_BINARY) 
	$(MV) $^ $@

$(KERNEL_BINARY) : $(KERNEL_ELF) $(PAYLOAD_ELF)
	$(OBJCOPY) -O binary --set-section-flags .bss=alloc,load,contents $(KERNEL_ELF) $(KERNEL_BINARY)
	$(CAT) $(PAYLOAD_ELF) >> $(KERNEL_BINARY)

$(PAYLOAD_ELF): $(KERNEL_ELF)
	make PAYLOAD=$(PAYLOAD) -C $(PAYLOADS) all

$(KERNEL_ELF): $(KERNEL_OBJECTS)
	$(LD) -T Linker.ld --defsym=__BOOT__=$(MemoryMap_BootStart) -o $@ $^

$(BUILD)/%.o: src/%.cpp 
	mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -MMD -MP -c $< -o $@

%.bin: %.elf 
	$(OBJCOPY) -O binary $< $@

$(KERNEL_OBJECTS): $(CONFIG)

clean:
	rm -rf build

-include $(KERNEL_DEPENDENCIES)
