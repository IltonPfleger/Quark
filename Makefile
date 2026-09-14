include Makedefs.mk

KERNEL_SOURCES       := $(shell find src -name '*.cpp' | grep -v -E 'architecture|machine|abi')
KERNEL_SOURCES 	     += $(shell find src/architecture/$(ARCH) -name '*.cpp')
KERNEL_SOURCES 	     += $(shell find src/machine/$(ARCH)/$(MACHINE) -name '*.cpp')
KERNEL_SOURCES       += $(if $(Payload_Unprivileged),$(shell find src/abi -name '*.cpp'))
KERNEL_OBJECTS       := $(patsubst src/%.cpp,$(BUILD)/%.o,$(KERNEL_SOURCES))
KERNEL_DEPENDENCIES  := $(KERNEL_OBJECTS:.o=.d)
PAYLOAD_ELF          := $(BUILD)/$(PAYLOAD).elf

run: $(IMAGE)
	$(CONSOLE) $(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $<

debug: $(IMAGE)
	-$(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $< -S -gdb tcp::1234

gdb:
	$(GDB) -ex "file $(KERNEL_ELF)" -ex "target extended-remote:1234"

$(IMAGE).bin : $(KERNEL_ELF) $(PAYLOAD_ELF)
	$(OBJCOPY) -O binary --set-section-flags .bss=alloc,load,contents $(KERNEL_ELF) $(IMAGE).bin
	$(CAT) $(PAYLOAD_ELF) >> $(IMAGE).bin

$(BUILD)/$(PAYLOAD).elf: $(KERNEL_ELF)
	$(LD) -e main --just-symbols $(KERNEL_ELF) -Ttext=$(MemoryMap_Application) --image-base=$(MemoryMap_Application) -o $@ $(BUILD)/$(PAYLOAD).o

$(KERNEL_ELF): $(KERNEL_OBJECTS)
	$(MAKE) PAYLOAD=$(PAYLOAD) -C $(PAYLOADS) $(BUILD)/$(PAYLOAD).o
	$(LD) $(LDFLAGS) `nm -u $(BUILD)/$(PAYLOAD).o 2>/dev/null | awk '{print "-u " $$NF}'` -T Linker.ld --defsym=__BOOT__=$(MemoryMap_Boot) -o $@ $^

$(BUILD)/%.o: src/%.cpp 
	$(MKDIR) -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -MMD -MP -c $< -o $@

%.bin: %.elf 
	$(OBJCOPY) -O binary $< $@

$(KERNEL_OBJECTS): $(CONFIG)

clean:
	$(RM) -rf build

-include $(KERNEL_DEPENDENCIES)
