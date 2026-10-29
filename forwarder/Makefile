.SUFFIXES:
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := $(ARCH) -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -D__3DS__ -I$(DEVKITPRO)/libctru/include
LDFLAGS := $(ARCH) -specs=3dsx.specs -Wl,--gc-sections -L$(DEVKITPRO)/libctru/lib
MAKEROM ?= makerom
BANNERTOOL ?= bannertool
TARGET := dist/CoreLab-HOME.cia
all: $(TARGET)
build/forwarder.elf: source/main.c
	@mkdir -p build
	$(CC) $(CFLAGS) $(LDFLAGS) $< -lctru -lm -o $@
build/banner.bnr: assets/banner.png assets/silence.wav
	@mkdir -p build
	$(BANNERTOOL) makebanner -i assets/banner.png -a assets/silence.wav -o $@
build/icon.smdh: assets/icon.png
	@mkdir -p build
	$(BANNERTOOL) makesmdh -s "Ultima Online 3DS" -l "CoreLab - acceso directo" -p "Rodrigo Torres" -i $< -o $@
$(TARGET): build/forwarder.elf build/banner.bnr build/icon.smdh forwarder.rsf
	@mkdir -p dist
	$(MAKEROM) -f cia -o $@ -target t -exefslogo -rsf forwarder.rsf -elf build/forwarder.elf -banner build/banner.bnr -icon build/icon.smdh -major 1 -minor 0 -micro 0
.PHONY: all
