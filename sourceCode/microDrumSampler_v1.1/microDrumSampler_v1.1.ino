/* microDrumSampler
Author:       Mike Utz
Date:         05. Januar 2025
Description:
Play Two Drum-Sounds as raw data to from SD card, when its dedicated button is pushed.
The Sound can be changed between three different Sound Presets
If the Battery Voltage is low, a Sound will be played
If the Battery Voltage is under a critical Level. a Sound will be played an the Power Amplifier will shut down.

You must select Board: Tennsy-4.0 // USB-Type: Serial + MIDI + Audio from the "Tools menu.

Version:      1.1 Two Inputs for Kick n Snare Sound
                  Two Inputs for Preset Switch (3 Preset)
                  Two Outputs one for Status LED and one for Power Amp Shutdown
                  Added a second player for playing both sounds simultaniously
                  Added a Indicattion for amp power shutdown by blinking the Status LED
                  Added Batt Voltage Level Serial Print if the amplifier is shutdwon
                  Changed the Voltage and Shutdown Voltage Level / VDiv = 4.35 // 12V = 2.75V
                  -> Pull Up the Input to +3.3V if this function is not needed.
*/

#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdRaw           playRaw1;       //xy=60.5,69
AudioPlaySdRaw           playRaw2;       //xy=60.5,111
AudioInputUSB            usb1;           //xy=61.5,30
AudioMixer4              mixer1;         //xy=256.5,42
AudioMixer4              mixer2;         //xy=259.5,110
AudioOutputI2S           i2s1;           //xy=439.5,49
AudioConnection          patchCord1(playRaw1, 0, mixer1, 1);
AudioConnection          patchCord2(playRaw1, 0, mixer2, 1);
AudioConnection          patchCord3(playRaw2, 0, mixer1, 2);
AudioConnection          patchCord4(playRaw2, 0, mixer2, 2);
AudioConnection          patchCord5(usb1, 0, mixer1, 0);
AudioConnection          patchCord6(usb1, 1, mixer2, 0);
AudioConnection          patchCord7(mixer1, 0, i2s1, 0);
AudioConnection          patchCord8(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=260.5,168
// GUItool: end automatically generated code


// Inputs
// Bounce objects to easily and reliably read the buttons
Bounce buttonKick     =   Bounce(2, 2);
Bounce buttonSnare    =   Bounce(3, 2);
Bounce buttonStop     =   Bounce(4, 2);
Bounce buttonPreset1  =   Bounce(5, 10);
Bounce buttonPreset2  =   Bounce(6, 10);

int pwrVoltRead = A0;

// Outputs:
int ampShtDwn = 16;
int statusLed = 17;


// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  11   // Teensy 4 ignores this, uses pin 11
#define SDCARD_SCK_PIN   13  // Teensy 4 ignores this, uses pin 13

// Global Variables
int mode        = 0;  // 0=stopped, 1=playing
int preset      = 0;  // Choose which Sound preset will play
int lastPreset  = 0;  // store the last choosen preset
int pwrState    = 0;  // 0 = OK / 1 = low battery / 2 = power amp shutdown
int pwrVoltWarn = 640; // Threshold for playing the warn sound
int pwrVoltCut  = 600; // Threshold for studing down the Power Amp
volatile int pwrVolt     = 0;  // holds the value of the Voltage Power Input
volatile int pwrVoltRms  = 0;  // integrated value of pwrVolt
volatile int pwrVoltCount= 0;
int midiChan    = 1;
// timers
elapsedMillis inpUpdateTime;
elapsedMillis pwrCheckTime;
elapsedMillis lowBattBlinkTime;

// setup routine -------------------------------------------------------------
void setup() {
  // Configure the pushbutton pins
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP); 
  pinMode(ampShtDwn, OUTPUT);
  pinMode(statusLed, OUTPUT);

  // setup analog resolution and frequency of pwm pins:
  analogReadRes(10); // Sets the Anaalog input to 10 Bit 0 -1023
  analogWriteResolution(10); // Sets the PWM Output to 10 Bit 0-1023

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(12);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8);
  sgtl5000_1.lineOutLevel(29);
  // Setting Mixer Gain:
  mixer1.gain(0, 0.8);  // USB Input Left
  mixer1.gain(1, 1);    // raw1.player Input
  mixer1.gain(2, 1);    // raw2.player Input
  mixer2.gain(0, 0.8);  // USB Input Right
  mixer2.gain(1, 1);    // raw1.player Input 
  mixer1.gain(2, 1);    // raw2.player Input  

  // Initialize the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here if no SD card, but print a message
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  // Switch On the Amplifier
  digitalWrite(ampShtDwn, HIGH);
  delay(200); // Wait until the amplifier has powered up

  // Play Startup Sound:
  Serial.println("startup");
  playRaw1.play("startup.RAW");
  delay(1000); // Wait for the standard file startup.raw played to its end.
  playRaw1.stop();
}

// loop routine ---------------------------------------------------------------
void loop() {

  // read the PC's volume setting and set it to the Audio Processor
  float vol = usb1.volume();
  if (vol >= 0.8){
    vol = 0.8;
  }
  mixer1.gain(0, vol);
  mixer2.gain(0, vol);

  // Read and integrate the analog input over time
  if (inpUpdateTime >= 125){
    pwrVoltCount++;

    if (pwrVoltCount <= 8){
    pwrVolt = pwrVolt + analogRead(pwrVoltRead);
    }
    else if (pwrVoltCount >= 9 ){
      pwrVoltRms = pwrVolt / 8;
      pwrVoltCount = 0;
      pwrVolt = 0;
      pwrCheck();
    }
  inpUpdateTime = 0;
  }


  //Read all the digital inputs
  buttonKick.update();
  buttonSnare.update();
  buttonStop.update();
  buttonPreset1.update();
  buttonPreset2.update();


  // Respond to Inputs for Change the Preset Nr.
  if (buttonPreset1.read() == HIGH && buttonPreset2.read() == HIGH){
    preset = 0;
  }
  else if (buttonPreset1.read() == HIGH && buttonPreset2.read() == LOW){
    preset = 1;
  }
  else if (buttonPreset1.read() == LOW && buttonPreset2.read() == HIGH){
    preset = 2;
  }

  if (preset != lastPreset){
    lastPreset = preset;
    Serial.print(" change to preset ");
    Serial.println (preset);
  }



  // Respond to play button presses
  if (buttonKick.risingEdge()) {
    Serial.println("kick button pressed");
    startPlayKick();
  }
  if (buttonSnare.risingEdge()) {
    Serial.println("snare button pressed");
    startPlaySnare();
  }
  if (buttonStop.fallingEdge()) {
    Serial.println("stop button presed");
    if (mode == 1) stopPlaying();
  }

  if (mode == 1) {
    continuePlaying();
  }

  // read midi input at defined channel
  usbMIDI.read(midiChan);

}

// functions ---------------------------------------------------------------
void pwrCheck(){
  if (pwrState == 0 && pwrVoltRms <= pwrVoltWarn){
    pwrState = 1;
  }
   else if (pwrState == 1 && pwrVoltRms >= pwrVoltWarn + 60){ // Only get back to normal state if the Voltage is more than 0.2V over the Treshold
    pwrState = 0;
  }
   else if (pwrState == 2 && pwrVoltRms >= pwrVoltWarn + 90){ // Only get back to normal state if the Voltage is more than 0.3V over the Treshold
    pwrState = 0;
    digitalWrite(ampShtDwn, HIGH);
    Serial.println("power is enough again");
  }

  if (pwrState == 2){
    lowBattIndicate();
  }

  if (pwrState == 1 && pwrCheckTime >= 60000){
    Serial.println("power is too low");
    playRaw1.stop();
    playRaw1.play("low_batt.RAW"); // play low batt warning sound
    delay(1000); // Wait for the file played to its end.
    playRaw1.stop();
    pwrCheckTime = 0;

    if (pwrVoltRms <= pwrVoltCut){
      Serial.println("amp shut down");
      playRaw1.stop();
      delay(500);
      playRaw1.play("shutdown.RAW");
      delay(3000); // Wait for the file played to its end.
      playRaw1.stop();
      pwrState = 2;
      digitalWrite(ampShtDwn, LOW);
    }
  }
}

void startPlayKick() {
  Serial.println("playing the kick");
  if (preset == 0){
    playRaw1.play("kick.RAW");
  }
  if (preset == 1){
    playRaw1.play("kick1.RAW");
  }
  if (preset == 2){
    playRaw1.play("kick2.RAW");
  }
  digitalWrite(statusLed, HIGH);
  mode = 1;
}

void startPlaySnare() {
  Serial.println("playing the snare");
  if (preset == 0){
    playRaw2.play("snare.RAW");
  }
  if (preset == 1){
    playRaw2.play("snare1.RAW");
  }
  if (preset == 2){
    playRaw2.play("snare2.RAW");
  }
  digitalWrite(statusLed, HIGH);
  mode = 1;
}

void continuePlaying() {
  if (!playRaw1.isPlaying() && !playRaw2.isPlaying() ) {
    playRaw1.stop();
    playRaw2.stop();
    Serial.println("file end");
    digitalWrite(statusLed, LOW);
    mode = 0;
  }
}

void stopPlaying() {
  if (mode == 1) playRaw1.stop();
    playRaw2.stop();
    Serial.println("stop playing");
    digitalWrite(statusLed, LOW);
    mode = 0;
}

void lowBattIndicate() {
  if (lowBattBlinkTime >= 1000){
    digitalToggle(statusLed);
    lowBattBlinkTime = 0;
    Serial.print("Battery Voltage is =");
    Serial.println((pwrVoltRms / 1023) * 14.3);
  }
}