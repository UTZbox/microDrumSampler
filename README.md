# microDrumSampler
A compact sampler that quickly plays audio files from an SD card, featuring two hardware trigger inputs and a preset switch to select between three sounds per channel. Based on a Teensy 4.0 Plattform.

# what can it be used for:
In my case, I built it for a street artist who wanted to play a drum sound along with his guitar, without using any traditional drum components or a cajón. With two foot pedals, he can control the drum sampler and route the audio output through the guitar amplifier or loop pedal. The artist can also choose between three different sounds using a preset switch. By powering up the sampler it plays a startup sound, to show if its selftest is done and ready to perform. The system can be powered by a battery, and the battery level is monitored by the sampler, with a low charge being indicated audibly. Instead of the audio output, it is also possible to directly connect an amplifier and speakers if this is intended to be a standalone device. 
For model builders, exhibitions, or museums, this can be used as a simple player that plays an explanation or story at the push of a button.

# how it works:
The device is based on a Teensy 4.0 and Teensy Audio Rev. D Board
Additional it can be battery powered. Instead the Audio Line-Out a power-amplifier can be added as well.

When a rising edge is detected at the input, a dedicated file (.raw audio file) is played based on the preset selection (1 to 3). Each input has its own dedicated process (player) to ensure fast beats can be played without delay. If needed the current playou can be stopped by an input as well.
An analog input measures the battery voltage and provides an audible alert as well as a status LED notification when the voltage gets too low. If the voltage drops below the minimum threshold, an output is triggered to, for example, mute a power amplifier. This prevents deep discharge or extreme distortion at the amp output. If this feature is not needed, the analog input can simply be pulled up to HIGH (+3.3V).

# file handling:
The SD-Card hast to formated as FAT32. It should not be bigger than 32GB. I recommend 4Gb
All files has to be in the root directory of the SD card.
All files hast to named exactly as: kick.raw // kick1.raw // kick2.raw // snare.raw // snare1.raw // snare2.raw.
You must convert all audio files to .raw format. (Mono, 44100 Hz, Signed 16-bit PCM, RAW header-less)
To do that you can use the Freeware Tool: Audacity

# board connections:
used pins: 0 = GND, 1 =  Input 1 (Kick) {use with external pulldown resistror}, 2 = Input 2 (Snare) {use with external pulldown resistror}, 3 = Input 3 (Stop) {use with external pulldown resistror}, 4 = Preset Switch (1) (Teensy internal pullup used), Preset Switch (2) (Teensy internal pullup used), 14(A0) = Battery Voltage (0-12V), 16 = Output AMP-SHTDWN, 17 = Output Status LED.
