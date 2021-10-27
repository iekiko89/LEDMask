// Falling green 'code' like The Matrix.
// Adapted from https://gist.github.com/Jerware/b82ad4768f9935c8acfccc98c9211111

#include "Arduino.h"

class Matrix {
  public:
    Matrix(){};
    bool runPattern();
  private:
    long previousTime = 0;
    uint8_t hue = 20;
};

bool Matrix::runPattern() {
  if(checkButton()) return false;
  if(millis() - previousTime >= 75) {
    // Move bright spots downward
    for (int row = kMatrixHeight - 1; row >= 0; row--) {
      for (int col = 0; col < kMatrixWidth; col++) {
        //if (leds[XY(col, row)] == CRGB(0,255,0)) {
          if (leds[XY(col, row)] == CHSV(hue,255,255)) {
          //leds[XY(col, row)] = CRGB(27,200,39); // create trail
          leds[XY(col, row)] = CHSV(hue-1,255,255); // create trail
          //if (row < kMatrixHeight - 1) leds[XY(col, row + 1)] = CRGB(0,255,0);
          if (row < kMatrixHeight - 1) leds[XY(col, row + 1)] = CHSV(hue,255,255);
        }
      }
    }
    
    // Fade all leds
    for(int i = 0; i < NUM_LEDS; i++) {
      //if (leds[i].g != 255) leds[i].nscale8(192); // only fade trail
      if (leds[i] != CHSV(hue,255,255)) leds[i].nscale8(192); // only fade trail
    }
    EVERY_N_SECONDS(2){
    hue+=20;
    }

    // Spawn new falling spots
    if (random8(2) == 0) // lower number == more frequent spawns
    {
      int8_t spawnX = random8(kMatrixWidth);
      if(spawnX<11)
      {
        leds[XY(spawnX, 0)] = CHSV(hue,255,255);//CRGB(0,255,0 );
      }
      else{
        leds[XY(spawnX, 4)] = CHSV(hue,255,255);//CRGB(0,255,0 );
      }
    }

    FastLED.show();
    previousTime = millis();
  }
  return true;
}
