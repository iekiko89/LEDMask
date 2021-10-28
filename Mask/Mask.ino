/* Helper functions for an two-dimensional XY matrix of pixels.
 
*/

#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <EEPROM.h>
#include <JC_Button.h>




#define LED_PIN           14           // Output pin for LEDs [5]
#define MODE_PIN          15           // Input pin for button to change mode [3]
#define PLUS_PIN          2           // Input pin for plus button [2]
#define MINUS_PIN         4           // Input pin for minus button [4]
#define MIC_PIN           A6          // Input pin for microphone [A6]
#define COLOR_ORDER       GRB         // Color order of LED string [GRB]
#define CHIPSET           WS2812B     // LED string type [WS2182B]
#define BRIGHTNESS        100         // Overall brightness [50]
#define LAST_VISIBLE_LED  192         // Last LED that's visible [102]
#define MAX_MILLIAMPS     5000        // Max current in mA to draw from supply [500]
#define SAMPLE_WINDOW     100         // How many ms to sample audio for [100]
#define DEBOUNCE_MS       20          // Number of ms to debounce the button [20]
#define LONG_PRESS        500         // Number of ms to hold the button to count as long press [500]
#define PATTERN_TIME      10          // Seconds to show each pattern on autoChange [10]
#define kMatrixWidth      27          // Matrix width [15]
#define kMatrixHeight     12          // Matrix height [11]
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)                                       // Total number of Leds
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)   // Largest dimension of matrix

#define safety_pixel NUM_LEDS + 1

#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)   // Largest dimension of matrix
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* const leds( leds_plus_safety_pixel);

uint8_t brightness = BRIGHTNESS;
uint8_t soundSensitivity = 10;

// Used to check RAM availability. Usage: Serial.println(freeRam());
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Button stuff
uint8_t buttonPushCounter = 10;
uint8_t state = 0;
bool autoChangeVisuals = false;
Button modeBtn(MODE_PIN, DEBOUNCE_MS);
Button plusBtn(PLUS_PIN, DEBOUNCE_MS);
Button minusBtn(MINUS_PIN, DEBOUNCE_MS);

void incrementButtonPushCounter() {
  buttonPushCounter = (buttonPushCounter + 1) %11;
  EEPROM.write(1, buttonPushCounter);
}

// Include various patterns
#include "Sound.h"
#include "Rainbow.h"
#include "Fire.h"
#include "Squares.h"
#include "Circles.h"
#include "Plasma.h"
#include "Matrix.h"
#include "CrossHatch.h"
#include "Drops.h"
#include "Noise.h"
#include "Snake.h"

// Helper to map XY coordinates to irregular matrix
uint16_t XY (uint16_t x, uint16_t y) {
  // any out of bounds address maps to the first hidden pixel
  if ( (x >= kMatrixWidth) || (y >= kMatrixHeight) ) {
    return (safety_pixel);
  }

  const uint16_t XYTable[] = {
192,192,192,96,97,98,99,100,101,102,103,104,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
192,192,105,106,107,108,109,110,111,112,113,114,115,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
192,116,117,118,119,120,121,122,123,124,125,126,127,128,192,192,192,192,192,192,192,192,192,192,192,192,192,
129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,192,192,192,192,192,192,192,192,192,192,192,192,
144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,0,1,2,3,4,5,6,7,8,192,192,192,
192,159,160,161,162,163,164,165,166,167,168,169,170,171,9,10,11,12,13,14,15,16,17,18,19,192,192,
192,192,172,173,174,175,176,177,178,179,180,181,182,20,21,22,23,24,25,26,27,28,29,30,31,32,192,
192,192,192,183,184,185,186,187,188,189,190,191,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
192,192,192,192,192,192,192,192,192,192,192,192,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,
192,192,192,192,192,192,192,192,192,192,192,192,192,63,64,65,66,67,68,69,70,71,72,73,74,75,192,
192,192,192,192,192,192,192,192,192,192,192,192,192,192,76,77,78,79,80,81,82,83,84,85,86,192,192,
192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,87,88,89,90,91,92,93,94,95,192,192,192
  };

  uint16_t i = (y * kMatrixWidth) + x;
  uint16_t j = XYTable[i];
  return j;
}

void setup() {
  Serial.begin(57600);
  Serial.println("resetting");
  LEDS.addLeds<WS2812SERIAL,LED_PIN,GRB>(leds,NUM_LEDS);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  modeBtn.begin();
  plusBtn.begin();
  minusBtn.begin();
  buttonPushCounter = (int)EEPROM.read(1);    // load previous setting
  
  Serial.begin(57600);
  Serial.print(F("Starting pattern "));
  Serial.println(buttonPushCounter);
}

void checkBrightnessButton() {
  // On all none-sound reactive patterns, plus and minus control the brightness
  plusBtn.read();
  minusBtn.read();

  if (plusBtn.wasReleased()) {
    brightness += 20;
    FastLED.setBrightness(brightness);
  }

  if (minusBtn.wasReleased()) {
    brightness -= 20;
    FastLED.setBrightness(brightness);
  }
}

void checkSoundLevelButton(){
  // On all sound reactive patterns, plus and minus control the sound sensitivity
  plusBtn.read();
  minusBtn.read();

  if (plusBtn.wasReleased()) {
    // Increase sound sensitivity
    soundSensitivity -= 1;
    if (soundSensitivity == 0) soundSensitivity = 1;
  }

  if (minusBtn.wasReleased()) {
    // Decrease sound sensitivity
    soundSensitivity += 1;
  }
}

bool checkButton() {  
  modeBtn.read();

  switch (state) {
    case 0:                
      if (modeBtn.wasReleased()) {
        incrementButtonPushCounter();
        Serial.print(F("Short press, pattern "));
        Serial.println(buttonPushCounter);
        autoChangeVisuals = false;
        return true;
      }
      else if (modeBtn.pressedFor(LONG_PRESS)) {
        state = 1;
        return true;
      }
      break;

    case 1:
      if (modeBtn.wasReleased()) {
        state = 0;
        Serial.print(F("Long press, auto, pattern "));
        Serial.println(buttonPushCounter);
        autoChangeVisuals = true;
        return true;
      }
      break;
  }

  if(autoChangeVisuals){
    EVERY_N_SECONDS(PATTERN_TIME) {
      incrementButtonPushCounter();
      Serial.print("Auto, pattern ");
      Serial.println(buttonPushCounter); 
      return true;
    }
  }  

  return false;
}

// Functions to run patterns. Done this way so each class stays in scope only while
// it is active, freeing up RAM once it is changed.

void runSound(){
  bool isRunning = true;
  Sound sound = Sound();
  while(isRunning) isRunning = sound.runPattern();
}

void runRainbow(){
  bool isRunning = true;
  Rainbow rainbow = Rainbow();
  while(isRunning) isRunning = rainbow.runPattern();
}

void runFire(){
  bool isRunning = true;
  Fire fire = Fire();
  while(isRunning) isRunning = fire.runPattern();
}

void runSquares(){
  bool isRunning = true;
  Squares squares = Squares();
  while(isRunning) isRunning = squares.runPattern();
}

void runCircles(){
  bool isRunning = true;
  Circles circles = Circles();
  while(isRunning) isRunning = circles.runPattern();
}

void runPlasma(){
  bool isRunning = true;
  Plasma plasma = Plasma();
  while(isRunning) isRunning = plasma.runPattern();
}

void runMatrix(){
  bool isRunning = true;
  Matrix matrix = Matrix();
  while(isRunning) isRunning = matrix.runPattern();
}

void runCrossHatch(){
  bool isRunning = true;
  CrossHatch crossHatch = CrossHatch();
  while(isRunning) isRunning = crossHatch.runPattern();
}

void runDrops(){
  bool isRunning = true;
  Drops drops = Drops();
  while(isRunning) isRunning = drops.runPattern();
}

void runNoise(){
  bool isRunning = true;
  Noise noise = Noise();
  while(isRunning) {
    isRunning = noise.runPattern();
  }
}

void runSnake(){
  bool isRunning = true;
  Snake snake = Snake();
  while(isRunning) {
    isRunning = snake.runPattern();
  }
}

// Run selected pattern
void loop() {
  switch (buttonPushCounter) {
    case 0:
      runFire();//runSound();
      break;
    case 1:
      runRainbow();
      break;
    case 2:
      runFire();
      break;
    case 3:
      runSquares();
      break;
    case 4:
      runCircles();
      break;
    case 5:
      runPlasma();
      break;
    case 6:
      runMatrix();
      break;
    case 7:
      runCrossHatch();
      break;
    case 8:
      runDrops();
      break;
    case 9:
      runNoise();
      break;
    case 10:
      runSnake();
      break;
  }
}
