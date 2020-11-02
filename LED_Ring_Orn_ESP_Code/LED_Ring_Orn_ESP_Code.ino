//This code is placed under the MIT license
//Copyright (c) 2020 Albert Barber
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

//code intended to run on esp8266, Wemos D1 mini
//requires lastest version of adafruit neopixel library (use the library manager)
#include <PixelStrip.h>
#include "segmentDefs.h"
#include <EEPROM.h>
#include <Ticker.h>

//toggles for enabling buttons and EEPROM
#define BUTTONS_ENABLE  false
#define EEPROM_ENABLE   false

//total number of effects (change this if you add any!)
#define NUM_EFFECTS     28

//pin connections
#define PIXEL_PIN       D8
#define BUTTON_1        D6
#define BUTTON_2        D7
#define BUTTON_3        D5

//below vars are placeholders for possible bluetooth and mic board connections
#define MIC_IN          A0
#define RX_BT           D1
#define TX_BT           D2

//EEPROM Addresses for settings
//we want to store the brightness, current effect index, and the effect rotation toggle
//so they can be recalled after the mask has been turned off
#define BRIGHTNESS_ADDR 2 //brightness address
#define CUR_EFFECT_ADDR 0 //index of current effect address
#define EFFECT_ROT_ADDR 1 //effect rotaion bool address
#define EEPROM_COM_TIME 3000 //ms

//effects control vars
byte effectIndex = 0; //number of effect that's currently active (will be read from EEPROM later)
boolean effectRota = true; //effects rotation on / off flag
boolean effectsStop = false; //stop all effects flag
boolean direct = true; //use for setting direction of effects
boolean breakCurrentEffect = false; //flag for breaking out of effects that use multiple sub effects / loops

//macro for implementing break for effects with multiple sub effects
#define breakEffectCheck() ({ \
    if( (breakCurrentEffect) ){ \
      (breakCurrentEffect) = false; \
      break; \
    } \
  })

//brightness vars
byte brightnessIndex = 2; //initial brightness, index of brightnessLevels array
//brightness levels array, max is 255, but 100 should be bright enough for amost all cases
//!!WARNING brightness is directly tied to power consumption, the max current per led is 60ma, this is for white at 255 brightness
//if you actually run all the leds at max, the glasses will draw 4.75 amps, this is beyond the rating of the jst connectors
const byte brightnessLevels[] = { 10, 30, 50, 100, 150 };
const byte numBrightnessLevels = SIZE( brightnessLevels );

//Strip definitions
const uint16_t stripLength = 61;
const uint8_t stripType = NEO_GRB + NEO_KHZ800;
PixelStrip strip = PixelStrip(stripLength, PIXEL_PIN, stripType);

//initilize ticker objects
Ticker EEPROMcommiter; //timer for commiting data to EEPRROM

//Define some colors we'll use frequently
const uint32_t white =    strip.Color(255, 255, 255);
const uint32_t UCLAGold = strip.Color(254, 187, 54);
const uint32_t UCLABlue = strip.Color(83, 104, 149);
const uint32_t off =      strip.Color( 0, 0, 0 );
const uint32_t red =      strip.Color(255, 0, 0);
const uint32_t orange =   strip.Color(255, 43, 0);
const uint32_t ltOrange = strip.Color(255, 143, 0);
const uint32_t yellow =   strip.Color(255, 255, 0);
const uint32_t ltYellow = strip.Color(255, 255, 100);
const uint32_t green =    strip.Color(0, 128, 0);
const uint32_t blue =     strip.Color(0, 0, 255);
const uint32_t indigo =   strip.Color( 75, 0, 130);
const uint32_t violet =   strip.Color(238, 130, 238);
const uint32_t purple =   strip.Color(123, 7, 197);
const uint32_t pink =     strip.Color(225, 0, 127);

//define pallet array, contains 32bit representations of all colors used in patterns
uint32_t pallet[9] = { off, white, UCLAGold, UCLABlue, blue, yellow, red, green, purple };
//                   { -0-, --1--, ---2----, ----3---, -4--, ---5--, -6-, --7--, --8-- }

//pallet to match typical fairy light colors
uint32_t christmasPallet[5] = { red, blue, green, yellow, purple };

byte wavepattern[]  = { 6, 1 };
byte wavepattern2[] = { 4, 5 };
byte wavepattern3[] = { 6, 7 };
byte wavepattern4[] = { 8, 7 };
byte *wavepatternContainer[] = { wavepattern, wavepattern2, wavepattern3, wavepattern4 };
byte christmasWavePattern[5] = { 0, 1, 2, 3, 4};

//below are spin patterns
//these are for use with the colorSpin function, which draws a pattern across segments (using the longest segment as the pattern basis)
//and then moves it along the longest segment
//you can make your own, their structure is the following
//(note that you'll have to understand how segments work in my library, you can find notes on segments in segmentSet.h)
//a spin pattern is comprised of two parts combined into one 1d array
// { 0, 1,   1, 2, 0, 0, 0, }
//  part 1       part 2
//part 1 indicates the leds on the longest segment that part 2 is to be drawn on (doesn't inculde the last index, so 1 above wouldn't be drawn)
//part 2 is the pattern of colors for each segment, made up of indeces for at pallet (ie 1 represents the second color in the pallet)

//this is confusing so I'll give an example
//for the glasses, using lenseRowSegments (each row of the glasses is a segment, excluding the center 6 leds so that all the rows are the same length)
//part 1 above indicates that, from the 0th led of the first segment, up to (not including) the 1st led, the color pattern of the segments is part 2
//the result is that the first led of each row are colored (using pallet above) row 0 -> white, row 1 -> uclaGold, row 2 -> off,  row 3 -> off, row 4 -> off
//note that leds indexs in part 1 are always defined using the longest segment, because colorSpin will map the pattern (part 1) onto shorter segments
//this works well on most led arrangments, but not so much on the glasses
//which is why I'm using lenseRowSegments, so that all segments are equal length, and the mapping is one to one

//as below, spin patterns can have multiple pattern sections
//you define the matrix as: spinPatternName[(5 + 2) * 8] -> [ (number of segments + 2) * number of rows in pattern ]
//you can also make patterns repeat using colorSpin, so you just have to define a single complete pattern (like I do with spinPatternWave)

//4 spokes
byte spinPattern1[(5 + 2) * 5] = {
  0, 2, 1, 2, 0, 1, 0,
  2, 3, 0, 1, 2, 1, 0,
  3, 4, 0, 0, 1, 2, 0,
  4, 5, 0, 0, 0, 1, 2,
  5, 6, 2, 0, 0, 1, 1,
};

//3 spokes
byte spinPattern2[(5 + 2) * 5] = {
  0, 2, 1, 2, 0, 0, 0,
  2, 3, 0, 1, 2, 0, 0,
  3, 5, 0, 0, 1, 2, 0,
  5, 6, 0, 0, 0, 1, 2,
  6, 8, 2, 0, 0, 0, 1,
};

//2 spokes
byte spinPattern3[(5 + 2) * 5] = {
  0, 1,   1, 2, 0, 0, 0,
  3, 4,   0, 1, 2, 0, 0,
  6, 7,   0, 0, 1, 2, 0,
  9, 10,  0, 0, 0, 1, 2,
  10, 12, 2, 0, 0, 0, 1,
};

//for simple repeart drawing functions
byte simpleRepeatPattern[5] = {8, 6, 7, 4, 5}; //{ 4, 0, 5, 0, 6, 0, 7, 0};

//storage for pallets we'll generate on the fly
uint32_t randPallet[5]; //pallet for random colors (length sets number of colors used by randomColors effect)

//callback routine for committing EEPROM data
//EEPRROM has limited writes, so we want to commit all the writes after a delay
//this allows the data to change while the user is pressing buttons, and we'll only commit
//the final values once they are done
void ICACHE_RAM_ATTR commitEEPROM() {
  EEPROM.commit();
}

//triggered by button 1, stops the current pattern, and switches to the next one, wrapping if needed
//also stores effect index in eeprom
//if button 2 is also being held, turn effects on / off
void ICACHE_RAM_ATTR nextEffect() {
  strip.pixelStripStopPattern = true; //stop the current pattern
  breakCurrentEffect = true; //set flag to break out of current case statement
  //if we are rotating to the next effect, reset all the segment directions
  //and store the next effect index in eeprom
  if (effectRota) {
    resetSegDirections();
    EEPROM.write(CUR_EFFECT_ADDR, (effectIndex + 1) % NUM_EFFECTS);
    //stop any other commit timers and start a new one
    EEPROMcommiter.detach();
    EEPROMcommiter.once_ms(EEPROM_COM_TIME, commitEEPROM);
  }
  //if button 2 is being held, stop/start the effect cycle
  //otherwise increase the index to start the next effect
  if ( digitalRead(BUTTON_2) == LOW) {
    effectsStop = !effectsStop;
  }
}

//triggered by button 2, turns effect rotation on / off
//also stores the state in eeprom
//(if rotation is off, the current effect will be repeated continuously)
//if button 1 is also being held, turn effects on / off
void ICACHE_RAM_ATTR effectRotaToggle() {
  if ( digitalRead(BUTTON_1) == LOW) {
    strip.pixelStripStopPattern = true;
    effectsStop = !effectsStop;
  } else {
    effectRota = !effectRota;
    EEPROM.write(EFFECT_ROT_ADDR, effectRota);
    //stop any other commit timers and start a new one
    EEPROMcommiter.detach();
    EEPROMcommiter.once_ms(EEPROM_COM_TIME, commitEEPROM);
  }
}

//triggered by button 3, sets the strip brightness to the next
//also stores the brighness index in eeprom
//brightness level in the brightnessLevels array (wrapping to the start if needed)
void ICACHE_RAM_ATTR brightnessAdjust() {
  brightnessIndex = (brightnessIndex + 1) % numBrightnessLevels;
  strip.setBrightness( brightnessLevels[ brightnessIndex ] );
  EEPROM.write(BRIGHTNESS_ADDR, brightnessIndex);
  //stop any other commit timers and start a new one
  EEPROMcommiter.detach();
  EEPROMcommiter.once_ms(EEPROM_COM_TIME, commitEEPROM);
}

//increments the effect index (wrapping if needed)
void incrementEffectIndex() {
  strip.runRainbowOffsetCycle(false);
  strip.setRainbowOffset(0);
  effectIndex = (effectIndex + 1) % NUM_EFFECTS;
}

//resets all the segments to their default directions (as set in segmentDefs)
void resetSegDirections() {

}

void setup() {
  //initalize the led strip, and set the starting brightness
  strip.begin();

  if (BUTTONS_ENABLE) {
    pinMode(BUTTON_1, INPUT_PULLUP);
    pinMode(BUTTON_2, INPUT_PULLUP);
    pinMode(BUTTON_3, INPUT_PULLUP);
    //because of the way my library currently works, effects occupy the processor until they end
    //so to break out of an effect, or change sytem values, we need to use interrupts
    attachInterrupt(digitalPinToInterrupt(BUTTON_1), nextEffect, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_2), effectRotaToggle, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_3), brightnessAdjust, FALLING);
  }

  if (EEPROM_ENABLE && BUTTONS_ENABLE) {
    // EEPROM Initialization
    EEPROM.begin(512);

    //read EEPROM values for current effect, brightness, and effect rotation
    effectRota = EEPROM.read(EFFECT_ROT_ADDR);
    brightnessIndex = EEPROM.read(BRIGHTNESS_ADDR);
    effectIndex = EEPROM.read(CUR_EFFECT_ADDR);
  }

  strip.setBrightness( brightnessLevels[brightnessIndex] );
  strip.show();
  //Serial.begin(115200);

  randomSeed(ESP.getCycleCount()); //generate a random seed
  //fill in our random pallet
  strip.genRandPallet( randPallet, SIZE(randPallet) );
}

//!! If you want to change the main loop code, please read all the comments below and in the loop !!
//To remove an effect, simply change its case # to anything greater than the total number of effects (999 for ex)
//if you want to know about certain effects, please see comments in the library for said effect
//if you want to know how segments work, please see comments in segmentSet.h

//The main loop of the program, works as follows:
//if effectsStop is true, effects are "off", so we won't try to start the next effect
//otherwise we jump to the effect matching the effectIndex value using a switch statment
//we also "clean up" a bit by reseting direct to true and the breakCurrentEffect flag to false
//if we don't find an effect with the effectIndex, we increment the effectIndex until we do
//while an effect is running, button inputs can either lock the effect or skip to the next effect
//if we lock the effect (set effectRota to false), we do not increment effectIndex when the effect ends, essentially restarting the effect (with new colors if they're randomly choosen)
//if the effect is skipped, we set strip.pixelStripStopPattern and breakCurrentEffect to true
//strip.pixelStripStopPattern will stop the current effect, and breakCurrentEffect will break out of the current switch statement if needed (the switch case has more than one effect)
//once the effect ends (either naturally or from a button press), we incremented effectIndex (as long as effectRota is set true)
//and jump to the top of the main loop

void loop() {

  if (!effectsStop) { //if effectsStop is true, we won't display any effect
    direct = !direct;
    breakCurrentEffect = false;
    resetBrightness();
    //switch statment contains all effects
    //I'm not going to comment each one, as they're hard to describe
    //if an case has a loop, it generally means the effect will by run multiple times in diff directions
    //these will contain breakEffectCheck(); which will breakout of the case if the effect is skipped by button input
    //segmentName.flipSegDirectionEvery(1, true) means that the direction of the names segments will be reversed
    //this is used to change the direction of colorSpin effects

    switch (effectIndex) { //select the next effect based on the effectIndex
      case 0:
        for (int i = 0; i < 3; i++) {
          breakEffectCheck();
          strip.sparkSeg( ringSegments, 5, 1, 2, 0, 1, 0, 1, false, 100, 80 ); //three random colors, moving inwards
        }
        break;
      case 1:
        strip.sparkSeg( ringSegments, 5, 1, 1, 0, 2, 0, 1, false, 100, 80 ); //segmented rainbow sparks, blank Bg
        break;
      case 2:
        setTempBrightness(brightnessIndex - 1);
        strip.setRainbowOffset(50);
        strip.sparkSeg( ringSegments, 5, 1, 1, white, 1, 0, 5, true, 200, 80 ); //radial rainbow Bg, white sparks
        break;
      case 3:
        for (int i = 0; i < 3; i++) {
          breakEffectCheck();
          strip.stripOff();
          strip.colorWipeRandom(1, 3, 4, 80, true, true, false);
          breakEffectCheck();
          strip.stripOff();
          strip.colorWipeRandom(2, 3, 5, 80, true, true, false);
        }
        break;
      case 4:
        setTempBrightness(brightnessIndex - 1);
        //white leds, rainbow background, no trails, not scanner, zero eyesize
        strip.setRainbowOffsetCycle(40, false);
        strip.runRainbowOffsetCycle(true);
        setTempBrightness(brightnessIndex - 1);
        strip.patternSweepRand( 8, white, -1, 0, 0, true, 0, 1, 60, 320 );
        break;
      case 5:
        for (int i = 0; i < 3; i++) {
          breakEffectCheck();
          strip.gradientCycleRand( 4, 6, 24 * 7, direct, 90);
          direct = !direct;
        }
        break;
      case 6:
        strip.colorSpinSimple( ringSegments, 1, 0, 0, 12, 1, 0, 0, 2, 24 * 5, 60 ); //rainbow half
        break;
      case 7:
        for (int i = 0; i < 3; i++) {
          breakEffectCheck();
          strip.rainbow(35);
        }
        break;
      case 8:
        strip.theaterChaseRainbow(0, 70, 4, 2);
        break;
      case 9:
        strip.colorSpinRainbow(ringSegments, true, 200, 70);
        break;
      case 10:
        for (int i = 0; i < SIZE(wavepatternContainer); i++) {
          uint32_t tempPallet[2];
          breakEffectCheck();
          tempPallet[0] = pallet[ wavepatternContainer[i][0] ];
          tempPallet[1] = pallet[ wavepatternContainer[i][1] ];
          strip.twinkleSet(0, tempPallet, SIZE(tempPallet), 2, 60, 25, 10000);
          //strip.twinkle(pallet[ wavepatternContainer[i][0] ], pallet[ wavepatternContainer[i][1] ], 2, 60, 25, 10000);
        }
        break;
      case 11:
        strip.twinkleSet(0, christmasPallet, SIZE(christmasPallet), 2, 60, 15, 12000);
        break;
      case 12:
        setTempBrightness(brightnessIndex + 1);
        //random colors, blank BG, 6 length trails, no scanner, color mode 1
        for (int i = 0; i < 3; i++) {
          breakEffectCheck();
          strip.patternSweepRand( 3, -1, 0, 1, 3, false, 0, 1, 70, 24 * 8 );
        }
        break;
      case 13:
        setTempBrightness(brightnessIndex + 1);
        for (int i = 0; i < 2; i++) {
          breakEffectCheck();
          strip.patternSweepRainbowRand( 2, 0, 1, 6, true, 0, 60, 24 * 8);
        }
        break;
      case 14:
        for (int j = 0; j < 8; j++) {
          breakEffectCheck();
          strip.colorWipeRainbowSeg(ringSegments, 140, 24 / 4, direct, true, true, true);
          //direct = !direct;
          breakEffectCheck();
          strip.colorWipeSeg(ringSegments, 0, 24 / 4, 140, direct, true, true);
          direct = !direct;
        }
        break;
      case 15:
        setTempBrightness(brightnessIndex - 1);
        strip.colorSpinSimple( ringSegments, 1, white, -1, 2, -1, 4, 0, 1, 24 * 7, 50 ); //white on rainbow Bg
        break;
      case 16:
        for (int i = 0; i < SIZE(wavepatternContainer); i++) {
          breakEffectCheck();
          strip.waves( flowerSegments, pallet, SIZE( pallet ), wavepatternContainer[i], 2, 15, false, 30, 20);
        }
        break;
      case 17:
        strip.waves( flowerSegments, christmasPallet, SIZE( christmasPallet ), christmasWavePattern, SIZE(christmasWavePattern), 20, true, 30, 20);
        break;
      case 18:
        strip.colorSpinSimple( halfSegments, 1, 0, 0, 8, 1, 0, 0, 2, 24 * 9, 60 );
        break;
      case 19:
        for (int i = 0; i < 2; i++) {
          breakEffectCheck();
          strip.colorSpinSimple( halfSegments, 2, 0, 0, 8, 2, 8, 0, 1, 24 * 7, 60 );
          breakEffectCheck();
          strip.colorSpinSimple( halfSegments, 2, 0, 0, 4, 3, 4, 1, 1, 24 * 7, 60 );
        }
        break;
      case 20:
        strip.setRainbowOffsetCycle(40, false);
        strip.runRainbowOffsetCycle(true);
        setTempBrightness(brightnessIndex - 1);
        strip.randomColors(-1, false, white, 70, 5, 15000);
        break;
      case 21:
        for (int i = 0; i < 3; i++) {
          breakEffectCheck();
          strip.genRandPallet( randPallet, SIZE(randPallet) );
          strip.shiftingSea(randPallet, SIZE(randPallet), 20, 0, 2, 150, 40);
        }
        break;
      case 22:
        setTempBrightness(brightnessIndex - 1);
        for (int i = 0; i < SIZE(christmasPallet) * 4; i++) {
          int palletSize = SIZE(christmasPallet);
          breakEffectCheck();
          strip.colorWipeSeg(ringSegments, christmasPallet[i % palletSize], 24 / 2, 100, direct, true, true);
          direct = !direct;
        }
        break;
      case 23:
        strip.rainbowWave(ringSegments, 50, true, 5, 50);
        break;
      case 24:
        for (int i = 0; i < 2; i++) {
          strip.genRandPallet( randPallet, SIZE(randPallet) );
          randPallet[0] = 0;
          breakEffectCheck();
          strip.colorSpin(ringSegments, spinPattern3, SIZE(spinPattern3), randPallet, 0, 1, true, 200, 60);

          strip.genRandPallet( randPallet, SIZE(randPallet) );
          randPallet[0] = 0;
          breakEffectCheck();
          strip.colorSpin(ringSegments, spinPattern2, SIZE(spinPattern2), randPallet, 0, 1, true, 200, 60);

          strip.genRandPallet( randPallet, SIZE(randPallet) );
          randPallet[0] = 0;
          breakEffectCheck();
          strip.colorSpin(ringSegments, spinPattern1, SIZE(spinPattern1), randPallet, 0, 1, true, 200, 60);
        }
        break;
      case 25:
        for (int i = 0; i < 3; i++) {
          breakEffectCheck();
          strip.patternSweepRepeatRand(3, 0, 0, 2, 3, false, false, 0, 0, 1, 65, 200 );
        }
        break;
      case 26:
        for (int i = 0; i < 8; i++) {
          breakEffectCheck();
          int mode = 1;
          if ( i % 2 == 0) {
            mode = 1;
          } else {
            mode = 3;
          }
          strip.colorWipeRandomSeg(ringSegments, mode, 3, 24 / 3, 150, direct, false, true);
          breakEffectCheck();
          strip.colorWipeSeg(ringSegments, 0,  24 / 3, 150, direct, false, true);
          direct = !direct;
        }
        break;
      case 27:
        strip.randomColorSet(0, true, christmasPallet, SIZE(christmasPallet), 100, 3, 15000);
        break;
      default:
        break;
    }
    //if effectRota is true we'll advance to the effect index
    if (effectRota) {
      incrementEffectIndex();
    }
    strip.stripOff(); //clear the strip for the next effect
  }

  //unused effects

  //strip.segGradientCycle(ringSegments, SIZE(ringSegments), christmasWavePattern, SIZE(christmasWavePattern), christmasPallet, SIZE(christmasPallet), 5, 200, true, 130);

  //strip.simpleStreamerRand( 3, 0, 2, 2, 0, false, 24*4, 70);

  //  for (int j = 0; j < 4; j++) {
  //    if ((j % 2) == 0 ) {
  //      breakEffectCheck();
  //      strip.colorWipeRainbow(40, stripLength, true, false, false, true);
  //      breakEffectCheck();
  //      strip.colorWipe(0, stripLength, 40, true, false, true);
  //    } else {
  //      breakEffectCheck();
  //      strip.colorWipeRainbow(40, stripLength, false, false, false, true);
  //      breakEffectCheck();
  //      strip.colorWipe(0, stripLength, 40, false, false, true);
  //    }
  //  }

  //setTempBrightness(brightnessIndex + 1);
  // strip.rainbowSingleWave( starSegments, 1, 10, 2, 20, 15);

  //strip.doRepeatSimplePattern(simpleRepeatPattern, SIZE(simpleRepeatPattern), pallet, SIZE(pallet), 20, 25, 20, false);

  //  for (int j = 0; j < 256; j++) {
  //    strip.randomColors(off.color32, true, strip.Wheel(j & 255), 50, 8, 110);
  //  }
}

//a quick shortening of the random color function, just to reduce the pattern function calls more readable
uint32_t RC() {
  return strip.randColor();
}

//sets the current brightness to index value in brightness array
//does not actually change brightness index, so it can be reset later
void setTempBrightness(int index) {
  if (index < numBrightnessLevels && index > 0) {
    strip.setBrightness(brightnessLevels[index]);
  }
}

//resets brightness to current brightnessIndex
void resetBrightness() {
  strip.setBrightness(brightnessLevels[brightnessIndex]);
}
