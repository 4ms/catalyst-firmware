BUILDDIR ?= build
TARGET ?= f401

all: | $(BUILDDIR)
	cmake --build $(BUILDDIR)

$(BUILDDIR):
	cmake -B $(BUILDDIR) -GNinja

test:
	cmake --build $(BUILDDIR) --target tests

clean:
	rm -rf $(BUILDDIR)

wav:
	cmake --build build --target $(TARGET).wav

combo:
	cmake --build build --target $(TARGET)-combo

jflash-app:
	cmake --build $(BUILDDIR) --target $(TARGET)-jflash-app

jflash-combo:
	cmake --build $(BUILDDIR) --target $(TARGET)-jflash-combo

oflash-app:
	(echo program $(BUILDDIR)/$(TARGET)/$(TARGET).elf verify reset; echo exit) | nc -t localhost 4444
#	cmake --build $(BUILDDIR) --target $(TARGET)-oflash-app

oflash-combo:
	(echo program $(BUILDDIR)/$(TARGET)/$(TARGET)-combo.hex verify reset; echo exit) | nc -t localhost 4444
#	cmake --build $(BUILDDIR) --target $(TARGET)-oflash-combo

oreset:
	(echo reset; echo exit) | nc -t localhost 4444


.PHONY: test clean wav combo jflash-app oflash-app jflash-combo oflash-combo all
