# Measurements taken on p2 board:

## ADC
ADC clock set to PCLK DIV 2
Sample time per channel set to 56 clocks

#### ADC Conversion rate:
toggled a pin after conversion complete (both or just one channel)?
ADC runs at 284kHz

#### ADC bleed:
none noticable at 56 clocks (there was some at faster rates)

#### ADC noise:
- About 40 values (of 4096 range) noise without anything else running
- Up to 13 value range (of 4096) noise with RGB LEDs randomly jumping values rapidly (slider ADC value sent to DAC, see 50mV jumps when LEDs are randomly modulate. 50mV / 15V = 0.33% = 13/4096

#### ADC slider range:
slider: about 5 - 4079
CV: -5mV = 0, 4.95V = 4095


## DAC

#### Noise
With a constant value fed to it, about 20mV of noise

#### Step response
About 5us

#### Range
-5V to +10V (need to measure more accurately)


## Trig inputs

No bounce/chatter when fed analog LFO

## Buttons

#### Response time
Can toggle a pin within 200us of a button press


## RGB LEDs

Adds about 100mA to +12V consumption when all on full white

## Button LEDs

Adds about 10mA when all on
