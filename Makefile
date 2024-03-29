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

jflash-erase:
	cmake --build $(BUILDDIR) --target $(TARGET)-erase-chip

legacy:
	JFlashExe -openprj"scripts/f401.jflash" -erasechip -exit 
	JFlashExe -openprj"scripts/f401.jflash" -open"src/legacy/v1_0/f401-combo.hex" -auto -exit

.PHONY: test clean wav combo jflash-app oflash-app jflash-combo oflash-combo all
