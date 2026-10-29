.SUFFIXES:
MAKEROM ?= makerom
BANNERTOOL ?= bannertool
VERSION_MAJOR ?= 9
VERSION_MINOR ?= 0
VERSION_MICRO ?= 1
all: ../dist/CoreLab-v0.52.1.cia
../UO-3DS-CoreLab.elf:
	$(MAKE) -C ..
../forwarder/build/banner.bnr ../forwarder/build/icon.smdh:
	$(MAKE) -C ../forwarder build/banner.bnr build/icon.smdh BANNERTOOL=$(BANNERTOOL)
build/icon.smdh: ../forwarder/assets/icon.png Makefile
	@mkdir -p build
	$(BANNERTOOL) makesmdh -s "CoreLab 0.52.1" -l "CoreLab 0.52.1 - Juego HOME" -p "Rodrigo Torres" -i $< -o $@
../dist/CoreLab-v0.52.1.cia: ../UO-3DS-CoreLab.elf ../forwarder/build/banner.bnr build/icon.smdh game.rsf
	@mkdir -p ../dist
	$(MAKEROM) -f cia -o $@ -target t -exefslogo -rsf game.rsf -elf ../UO-3DS-CoreLab.elf -banner ../forwarder/build/banner.bnr -icon build/icon.smdh -major $(VERSION_MAJOR) -minor $(VERSION_MINOR) -micro $(VERSION_MICRO)
.PHONY: all
