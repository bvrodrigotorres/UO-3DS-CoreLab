.SUFFIXES:
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
export DEVKITPRO DEVKITARM
include $(DEVKITARM)/3ds_rules
export LD := $(DEVKITARM)/bin/arm-none-eabi-g++
TARGET := UO-3DS-CoreLab
BUILD := build
SOURCES := source
INCLUDES := include
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -Wextra -Werror -O2 -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions -fstack-usage
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(TARGET).map
LDFLAGS += $(foreach call,open close read write lseek fstat stat unlink rename,-Wl,--wrap=_$(call)_r)
LIBS := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS := $(CTRULIB) $(PORTLIBS)
APP_TITLE := UO CoreLab v0.49
APP_DESCRIPTION := Independent scene and walking experiment
APP_AUTHOR := bvrodrigotorres
export TOPDIR := $(CURDIR)
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(CURDIR)/source $(CURDIR)/shaders
export DEPSDIR := $(CURDIR)/$(BUILD)
export OFILES := main.o platform_io.o expedition_ui.o world_stream.o world_expedition.o world_archive.o save_queue.o quests.o commerce.o character.o combat.o core.o scene_io.o renderer.o exploration.o avatar_io.o hud.o journal.o doors.o npcs.o inventory.o equipment.o startup.o region_migration.o world.shbin.o
export INCLUDE := -I$(CURDIR)/include -I$(CTRULIB)/include -I$(CURDIR)/build
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
.PHONY: all clean test
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
$(BUILD):
	mkdir -p $@
test:
	python3 tests/run_tests.py
clean:
	rm -rf build $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh
else
$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf: $(OFILES)
renderer.o: world_shbin.h
world.shbin: world.v.pica
world.shbin.o: world.shbin
	$(bin2o)
world_shbin.h: world.shbin.o
-include $(DEPSDIR)/*.d
endif
