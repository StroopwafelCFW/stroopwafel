INPUT := bin/fw.img.full.bin
SECTIONS := 0x10700000 0x10800000 0x8120000 0x5000000 0x5100000 0x8140000 0x4000000 0xE6000000 0xE0000000 0xE4000000
BSS_SECTIONS := 0x10835000 0x5074000 0x8150000
INPUT_SECTIONS := $(addprefix sections/, $(addsuffix .bin, $(SECTIONS)))
PATCHED_SECTIONS := $(addprefix patched_sections/, $(addsuffix .bin, $(SECTIONS)))

.PHONY: all clean wupserver/wupserver.bin

all: ios.img ios.patch

sections/%.bin: $(INPUT)
	@mkdir -p sections
	@python3 scripts/anpack.py -in $(INPUT) -e $*,$@

extract: $(INPUT_SECTIONS)

wupserver/wupserver.bin:
	@cd wupserver && make

patched_sections/%.bin: sections/%.bin patches/%.s wupserver/wupserver.bin
	@mkdir -p patched_sections
	@echo patches/$*.s
	@armips patches/$*.s

patch: $(PATCHED_SECTIONS)

ios.img: $(INPUT) $(PATCHED_SECTIONS)
	python3 scripts/anpack.py -nc -in $(INPUT) -out ios.img $(foreach s,$(SECTIONS),-r $(s),patched_sections/$(s).bin) $(foreach s,$(BSS_SECTIONS),-b $(s),patched_sections/$(s).bin)
    
ios.patch: ios.img salt-patch
	./salt-patch
    
ifeq ($(OS),Windows_NT)
    SP_EXECNAME :=salt-patch.exe
else
    SP_EXECNAME :=salt-patch
    endif

salt-patch:
	$(MAKE) -C salt-patch-src
	cp salt-patch-src/$(SP_EXECNAME) .
	chmod 755 ./$(SP_EXECNAME)

clean:
	@$(MAKE) -C salt-patch-src clean
	@$(MAKE) -C wupserver clean
	@rm -f fw.img ios.img ios.patch $(SP_EXECNAME)
	@rm -rf patched_sections
