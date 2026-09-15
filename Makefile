include Makedefs.mk

SOURCES       := $(shell find src -name '*.cpp')
SOURCES 	  += $(shell find architecture/$(ARCH)/src -name '*.cpp')
OBJECTS       := $(patsubst %.cpp,$(BUILD)/%.o,$(SOURCES))
DEPENDENCIES  := $(OBJECTS:.o=.d)

run: $(IMAGE)
	$(CONSOLE) $(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $<

debug: $(IMAGE)
	-$(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $< -S -gdb tcp::1234

gdb:
	$(GDB) -ex "file $(ELF)" -ex "target extended-remote:1234"

$(IMAGE).bin : $(ELF) $(BUILD)/$(PAYLOAD).elf
	$(OBJCOPY) -O binary --set-section-flags .bss=alloc,load,contents $(ELF) $(IMAGE).bin
	$(CAT) $(BUILD)/$(PAYLOAD).elf >> $(IMAGE).bin

$(BUILD)/$(PAYLOAD).elf: $(ELF)
	$(LD) -e main --just-symbols $(ELF) -Ttext=$(MemoryMap_Application) --image-base=$(MemoryMap_Application) -o $@ $(BUILD)/$(PAYLOAD).o

$(BUILD)/$(PAYLOAD).o:
	$(MAKE) PAYLOAD=$(PAYLOAD) -C $(PAYLOADS) $(BUILD)/$(PAYLOAD).o

$(ELF): $(OBJECTS) $(BUILD)/$(PAYLOAD).o
	$(LD) $(LDFLAGS) `nm -u $(BUILD)/$(PAYLOAD).o 2>/dev/null | awk '{print "-u " $$NF}'` -T Linker.ld --defsym=__BOOT__=$(MemoryMap_Boot) -o $@ $(filter-out $(BUILD)/$(PAYLOAD).o,$^)

$(BUILD)/%.o: %.cpp 
	$(MKDIR) -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -MMD -MP -c $< -o $@

%.bin: %.elf 
	$(OBJCOPY) -O binary $< $@

$(OBJECTS): $(CONFIG)

clean:
	$(RM) -rf build

-include $(DEPENDENCIES)
