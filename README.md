# esp32_basic_synth - the Michael Nolan implementation

ESP32 based simple synthesizer project
Fork here is by Michael Nolan - enormous gratitude to project originator. Homage, this I'm forking for my own learning to use Github and work on it from other places.

The project can be seen in Marcel-License video https://youtu.be/5XVK5MOKmZw

--- This text is from Marcel but I'm going to change a lot of this ... to match my variation if necessary.
The project has been tested on
- ESP32 Dev Kit - generic
- ESP32 DEVKIT - DOIT v1
- 
// Untested even though I do own one. I wanted to make inexpensive hacked kiddies toys with real synthizer and or midi capabilities.

# ESP32 Audio Kit V2.2
To compile set board to: ESP32 Dev Module
Ensure "#define ESP32_AUDIO_KIT" is set in config.h
IO22 will be used for MIDI in.

The ADC multiplexer can be used (other wise please remove #define ADC_TO_MIDI_ENABLED from config.h)
Connection of the ADC multiplexer:
- EN -> Ground
- S0 -> IO23
- S1 -> IO18
- S2 -> IO14
- S3 -> Ground
- Sig -> IO12

# ESP32 DEVKIT - DOIT
To compile set the board to: DOIT ESP32 DEVKIT V1
Ensure that "#define ESP32_AUDIO_KIT" has been removed from config.h

I've tried 3 DACs all PCM5102 - so far. One was broken by me or DOA, but this one (as seen here https://blog.himbeer.me/2018/12/27/how-to-connect-a-pcm5102-i2s-dac-to-your-raspberry-pi/). Works and so does this one https://www.youtube.com/watch?v=EbaYVh3mIws (which I can recommend as the easiest to get up and running).
I've ordered several others to test with  this later one uses 5v and has 3.3v regulators - several. I isolated it from the earth with a 0505 power isolation supply. I have another project which I didn't do that but it's all soldered together and it doesn't have the buzz from the microcontroller noise on it's earth - possibly because i've got bypass and star ground.

## Using a DAC
An external audio DAC is recommended for this setup:
- BCLK -> IO25
- WLCK -> IO27
- DOUT -> IO26

//I haven't tried this - why am I such an idiot that I didn't even read this properly.
## Using no DAC
You can also get a sound without a DAC.
Add '#define I2S_NODAC' to config.h

The default output pin is IO22. Add a capacitor in series of the audio line (10µF for example)

I've butchered this code and put a simple ADC for a pot, and i've also used a multiplexer chip on a Midicontroller - so I haven't used this yet in my version here but the code is sort of in play. However, I discovered that if you initialize thie ADCmul parts of this project then ADC via pins of the ESP32 seemed to read wrong (not work) - there is also an issue with code from 1.0.4 of the ESP32 for Arduino board libraries being needed and so newer versions will have compile errors and there are analog related calls that don't exist since 1.0.5 where they just standardized to the arduino way of calling those.
## Using an ADC multiplexer
Connection of the ADC multiplexer:
- EN -> Ground
- S0 -> IO33
- S1 -> IO32
- S2 -> IO13
- S3 -> Ground
- Sig -> IO12
Here is the related video: https://youtu.be/l8GrNxElRkc

### ADC Mapping
The adc module has been only tested with the ESP32 Audio Kit V2.2.
In z_config.ino you can define your own mapping. Actually only 8 channels are read from the multiplexer.
The adc lookup is used to define a channel and cc per analog input (C0..C7).
By changing adc values a MIDI messages will generated internally.
It should be also mapped int the MIDI mapping.

# MIDI Mapping
A controller mapping can be found in z_config.ino.
You can define your own controller mapping if your controller does support CC messages.

## MIDI via USB
MIDI can be received via USB activating the MACRO "MIDI_VIA_USB_ENABLED" in config.h.

Default PIN Mapping is:
- CS: IO5
- INT: IO17 (not used)
- SCK: IO18
- MISO: IO19
- MOSI: IO23

For more information refer to the MIDI related project: https://github.com/marcel-licence/esp32_usb_midi
Using USB can be seen here: https://youtu.be/Mt3rT-SVZww

---
If you have questions or ideas please feel free to use the discussion area!

---

