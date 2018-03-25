
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Servo.h>
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

//#include <MIDI.h>

#define SERVOMIN1 200 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX1 500 // this is the 'maximum' pulse length count (out of 4096)

static const unsigned ledPin = 13;      // LED pin on Arduino Uno

const byte numServos =2;


int chan = 1;

float servoPos[numServos];
float servoTpos[numServos];
int angle[numServos];
int servoNote[numServos];
float servoSpeed=100;
int maxSpeed = 1000;
int minSpeed =100;


// our servo # counter
uint8_t servonum[numServos];
uint16_t   pulselen;
const int lowNote = 36;


void setup() {
for(int i = 0; i<numServos; i++){
servonum[i]=i;


}
 pwm.begin();
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
pinMode(ledPin, OUTPUT);

  
  
  Serial.begin(115200);
  usbMIDI.setHandleNoteOn(moveServo);
  usbMIDI.setHandleNoteOff(handleNoteOff);
  usbMIDI.setHandleAfterTouchPoly(myAfterTouchPoly);
  usbMIDI.setHandleControlChange(myControlChange);
  usbMIDI.setHandleProgramChange(myProgramChange);
  usbMIDI.setHandleAfterTouchChannel(myAfterTouchChannel);
  usbMIDI.setHandlePitchChange(setServoSpeed);

  // Only one of these System Exclusive handlers will actually be
  // used.  See the comments below for the difference between them.
  usbMIDI.setHandleSystemExclusive(mySystemExclusiveChunk);
  usbMIDI.setHandleSystemExclusive(mySystemExclusive); 
  usbMIDI.setHandleTimeCodeQuarterFrame(myTimeCodeQuarterFrame);
  usbMIDI.setHandleSongPosition(mySongPosition);
  usbMIDI.setHandleSongSelect(mySongSelect);
  usbMIDI.setHandleTuneRequest(myTuneRequest);
  usbMIDI.setHandleClock(myClock);
  usbMIDI.setHandleStart(myStart);
  usbMIDI.setHandleContinue(myContinue);
  usbMIDI.setHandleStop(myStop);
  usbMIDI.setHandleActiveSensing(myActiveSensing);
  usbMIDI.setHandleSystemReset(mySystemReset);
  // This generic System Real Time handler is only used if the
  // more specific ones are not set.
  usbMIDI.setHandleRealTimeSystem(myRealTimeSystem);

}

void loop() {
  // The handler functions are called when usbMIDI reads data.  They
  // will not be called automatically.  You must call usbMIDI.read()
  // regularly from loop() for usbMIDI to actually read incoming
  // data and run the handler functions as messages arrive.
  usbMIDI.read();
  easeServos();
}


void myNoteOn(byte channel, byte note, byte velocity) {
  // When using MIDIx4 or MIDIx16, usbMIDI.getCable() can be used
  // to read which of the virtual MIDI cables received this message.
  Serial.print("Note On, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
}

void myNoteOff(byte channel, byte note, byte velocity) {
  Serial.print("Note Off, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
}

void myAfterTouchPoly(byte channel, byte note, byte velocity) {
  Serial.print("AfterTouch Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.println(velocity, DEC);
}

void myControlChange(byte channel, byte control, byte value) {
  Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.println(value, DEC);
}

void myProgramChange(byte channel, byte program) {
  Serial.print("Program Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", program=");
  Serial.println(program, DEC);
}

void myAfterTouchChannel(byte channel, byte pressure) {
  Serial.print("After Touch, ch=");
  Serial.print(channel, DEC);
  Serial.print(", pressure=");
  Serial.println(pressure, DEC);
}

void myPitchChange(byte channel, int pitch) {
  Serial.print("Pitch Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", pitch=");
  Serial.println(pitch, DEC);
}


// This 3-input System Exclusive function is more complex, but allows you to
// process very large messages which do not fully fit within the usbMIDI's
// internal buffer.  Large messages are given to you in chunks, with the
// 3rd parameter to tell you which is the last chunk.  This function is
// a Teensy extension, not available in the Arduino MIDI library.
//
void mySystemExclusiveChunk(const byte *data, uint16_t length, bool last) {
  Serial.print("SysEx Message: ");
  printBytes(data, length);
  if (last) {
    Serial.println(" (end)");
  } else {
    Serial.println(" (to be continued)");
  }
}

// This simpler 2-input System Exclusive function can only receive messages
// up to the size of the internal buffer.  Larger messages are truncated, with
// no way to receive the data which did not fit in the buffer.  If both types
// of SysEx functions are set, the 3-input version will be called by usbMIDI.
//
void mySystemExclusive(byte *data, unsigned int length) {
  Serial.print("SysEx Message: ");
  printBytes(data, length);
  Serial.println();
}

void myTimeCodeQuarterFrame(byte data) {
  static char SMPTE[8]={'0','0','0','0','0','0','0','0'};
  static byte fps=0;
  byte index = data >> 4;
  byte number = data & 15;
  if (index == 7) {
    fps = (number >> 1) & 3;
    number = number & 1;
  }
  if (index < 8 || number < 10) {
    SMPTE[index] = number + '0';
    Serial.print("TimeCode: ");  // perhaps only print when index == 7
    Serial.print(SMPTE[7]);
    Serial.print(SMPTE[6]);
    Serial.print(':');
    Serial.print(SMPTE[5]);
    Serial.print(SMPTE[4]);
    Serial.print(':');
    Serial.print(SMPTE[3]);
    Serial.print(SMPTE[2]);
    Serial.print('.');
    Serial.print(SMPTE[1]);  // perhaps add 2 to compensate for MIDI latency?
    Serial.print(SMPTE[0]);
    switch (fps) {
      case 0: Serial.println(" 24 fps"); break;
      case 1: Serial.println(" 25 fps"); break;
      case 2: Serial.println(" 29.97 fps"); break;
      case 3: Serial.println(" 30 fps"); break;
    }
  } else {
    Serial.print("TimeCode: invalid data = ");
    Serial.println(data, HEX);
  }
}

void mySongPosition(uint16_t beats) {
  Serial.print("Song Position, beat=");
  Serial.println(beats);
}

void mySongSelect(byte songNumber) {
  Serial.print("Song Select, song=");
  Serial.println(songNumber, DEC);
}

void myTuneRequest() {
  Serial.println("Tune Request");
}

void myClock() {
  Serial.println("Clock");
}

void myStart() {
  Serial.println("Start");
}

void myContinue() {
  Serial.println("Continue");
}

void myStop() {
  Serial.println("Stop");
}

void myActiveSensing() {
  Serial.println("Actvice Sensing");
}

void mySystemReset() {
  Serial.println("System Reset");
}

void myRealTimeSystem(uint8_t realtimebyte) {
  Serial.print("Real Time Message, code=");
  Serial.println(realtimebyte, HEX);
}



void printBytes(const byte *data, unsigned int size) {
  while (size > 0) {
    byte b = *data++;
    if (b < 16) Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1) Serial.print(' ');
    size = size - 1;
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
//if(pitch==36){
//     digitalWrite(ledPin, LOW);
//}
}


void moveServo(byte channel, byte note, byte velocity){
//write an angle based on velocity to our servo

    for(int i = 0; i<numServos; i++){
      if(note==lowNote+i){
    angle[i] = map(velocity, 0, 127, SERVOMIN1, SERVOMAX1); //map velocity to angle
   //pwm.setPWM(servonum[i[, 0, angle[i]); //write our angle to the servo
   servoTpos[i]=angle[i];
      }
    }
}

void easeServos(){
   for(int i = 0; i<numServos; i++){
  if(servoTpos[i] > servoPos[i]){
    servoPos[i]+= abs(servoPos[i] - servoTpos[i])*(.00001*servoSpeed); //for parabolic easing
    pwm.setPWM(servonum[i], 0, servoPos[i]);
    }
  else if(servoTpos[i] < servoPos[i]){
    servoPos[i]-= abs(servoPos[i] - servoTpos[i])*(.00001*servoSpeed); //for parabolic easing
      //servoPos[i]-=.001*servoSpeed; //for linear easing
     
     pwm.setPWM(servonum[i], 0, servoPos[i]);
     //myServo.write(servoPos[i]);
     
     //Serial.println(servoPos[i]); 
    }
   }
  }

  void setServoSpeed(byte channel, int bend){
       for(int i = 0; i<numServos; i++){
    servoSpeed = map(bend, -8192, 8192, minSpeed, maxSpeed);
       }
    }

