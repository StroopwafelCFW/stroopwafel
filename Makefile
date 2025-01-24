INPUT := bin/fw.img.full.bin
SECTIONS := 
BSS_SECTIONS := 
INPUT_SECTIONS := $(addprefix sections/, $(addsuffix .bin, $(SECTIONS)))
PATCHED_SECTIONS := $(addprefix patched_sections/, $(addsuffix .bin, $(SECTIONS)))

.PHONY: all clean wafel_core/00core.elf

all: 00core.ipx

sections/%.bin: $(INPUT)
	@mkdir -p sections
	python3 scripts/anpack.py -in $(INPUT) -e $*,$@

00core.ipx: wafel_core/00core.elf
	@cp wafel_core/00core.elf 00core.ipx

wafel_core/00core.elf:
	@cd wafel_core && make

patched_sections/%.bin: sections/%.bin patches/%.s
	@mkdir -p patched_sections
	@echo patches/$*.s
	@armips patches/$*.s

ios.img: $(INPUT) $(PATCHED_SECTIONS) wafel_core/00core.elf
	python3 scripts/anpack.py -nc -in $(INPUT) -out ios.img $(foreach s,$(SECTIONS),-r $(s),patched_sections/$(s).bin) $(foreach s,$(BSS_SECTIONS),-b $(s),patched_sections/$(s).bin)
    
ios.patch: ios.img salt-patch
	./salt-patch
    
ifeq ($(OS),Windows_NT)
    SP_EXECNAME :=salt-patch.exe
else
    SP_EXECNAME :=salt-patch
endif

salt-patch: salt-patch-src/main.c
	$(MAKE) -C salt-patch-src
	cp salt-patch-src/$(SP_EXECNAME) .
	chmod 755 ./$(SP_EXECNAME)

extract: $(INPUT_SECTIONS)
patch: ios.patch $(PATCHED_SECTIONS)

clean:
	@$(MAKE) -C salt-patch-src clean
	@$(MAKE) -C wafel_core clean
	@rm -f fw.img ios.img ios.patch 00core.ipx $(SP_EXECNAME)
	@rm -rf patched_sections
