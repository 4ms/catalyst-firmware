BUILDDIR ?= build
TARGET ?= f401

all: | $(BUILDDIR)
	cmake --build $(BUILDDIR)

$(BUILDDIR):
	cmake -B $(BUILDDIR) -GNinja

clean:
	rm -rf $(BUILDDIR)

wav:
	cmake --build build --target $(TARGET).wav

combo:
	cmake --build build --target $(TARGET)-combo

jflash-app:
	cmake --build $(BUILDDIR) --target $(TARGET)-jflash-app

oflash-app:
	cmake --build $(BUILDDIR) --target $(TARGET)-oflash-app
