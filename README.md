# Catalyst Sequencer and Catalyst Controller

Firmware for the Eurorack modules from 4ms Company.

Requirements:

- arm-none-eabi-gcc toolchain, version 12.3 is known to work.

```
git clone https://github.com/4ms/catalyst-firmware.git
cd catalyst-firmware
make all

ls -l build/f401/*.hex
```

These files will be created:

- `f401.hex`: this is the catalyst application (the main firmware). This is
  probably what you want to flash to your device.

- `f401-bootloader.hex`: this is the bootloader. You only need to flash this if
  you modified the bootloader code (if you're not sure, then don't use this file).
 
- `f401-combo.hex`: this is the bootloader plus the application bundled into.
  This is used for production purposes.

To create a .wav file that can be used with the bootloader to update a Catalyst
via an audio cable:

```
make wav

ls -l build/f401.wav
```




