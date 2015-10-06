#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma

#ifdef __AVR_ATtiny85__
 #include <avr/power.h>
#endif

/*****************************************************************************
Example sketch for driving Adafruit WS2801 pixels!
  Designed specifically to work with the Adafruit RGB Pixels!
  12mm Bullet shape ----> https://www.adafruit.com/products/322
  12mm Flat shape   ----> https://www.adafruit.com/products/738
  36mm Square shape ----> https://www.adafruit.com/products/683
  These pixels use SPI to transmit the color data, and have built in
  high speed PWM drivers for 24 bit color per pixel
  2 pins are required to interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Limor Fried/Ladyada for Adafruit Industries. 
  BSD license, all text above must be included in any redistribution
*****************************************************************************/

// Choose which 2 pins you will use for output.
// Can be any valid output pins.
// The colors of the wires may be totally different so
// BE SURE TO CHECK YOUR PIXELS TO SEE WHICH WIRES TO USE!

uint8_t dataPin  = 2;    // Yellow wire on Adafruit Pixels
uint8_t clockPin = 3;    // Green wire on Adafruit Pixels

// Input buffer

#define MAXBUF 10
char inbuf[MAXBUF];
byte inbuf_pos;

// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply
// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(25, dataPin, clockPin);
// Optional: leave off pin numbers to use hardware SPI
// (pinout is then specific to each board and can't be changed)
//Adafruit_WS2801 strip = Adafruit_WS2801(25);
// For 36mm LED pixels: these pixels internally represent color in a
// different format.  Either of the above constructors can accept an
// optional extra parameter: WS2801_RGB is 'conventional' RGB order
// WS2801_GRB is the GRB order required by the 36mm pixels.  Other
// than this parameter, your code does not need to do anything different;
// the library will handle the format change.  Examples:
//Adafruit_WS2801 strip = Adafruit_WS2801(25, dataPin, clockPin, WS2801_GRB);
//Adafruit_WS2801 strip = Adafruit_WS2801(25, WS2801_GRB);

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  strip.begin();
  // Update LED contents, to start they are all 'off'
  strip.show();
  Serial.begin(57600);
  inbuf_pos = 0;
}

/* Helper functions */

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

// What we will really want is:
// Code that samples the input devices:
//   - any reed switches for the map
//   - sliders / rotaryencs / POTs for the date / time
// And then code that sends a request over serial to the RPi to get the appropriate data point
// When the response comes back, plot it as a series of LEDs.
// How to demo that without the strip?  Guess we just debug with the test app and plug and pray.
void showMeterPercent(byte percentage)
{
  // Convert the input percentage into a number of LEDs to light.
  int num_leds = percentage / strip.numPixels();
  // Now scale that to a colour.   We want:
  // 0 = Green.     00 FF 00
  // 30% = Yellow   FF FF 00
  // 60% = Orange   FF 80 00
  // 100% = Red     FF 00 00
  byte r, g, b;
  b = 0;
 
  if (percentage < 30)
  {
    r = map(percentage, 0, 30, 0, 255);
    g = 255;
  }
  else
  {
    r = 255;
    g = map(percentage, 30, 100, 255, 0);
  }
  int32_t col = Color(r, g, b);
  for (byte ii = 0; ii < strip.numPixels(); ii++)
  {
    if (ii <= num_leds)
    {
      strip.setPixelColor(ii, col);
    }
    else
    {
      strip.setPixelColor(ii, 0);
    }
  }
  strip.show();
  // debug : set pwm RGB LED.  R = 6, G = 10, B = 11
  analogWrite(6, r);
  analogWrite(10, g);
  analogWrite(11, b);
}

void loop()
{
  // Ramp the meter from 0 to 100%
//  Serial.println("BEGIN");
/*  for (int ii = 0; ii <= 100; ii += 4)
  {
    showMeterPercent(ii);
    Serial.println(ii);
    delay(200);
  }
  analogWrite(6, 0);
  analogWrite(10, 0);
  analogWrite(11, 0);
//  Serial.println("PAUSE");
  delay(1000);
  */
  if (Serial.available())
  {
    char inch = Serial.read();
    Serial.write(inch);
    if (inbuf_pos == 0)
    {
      // Waiting for a start of response character ('!')
      if (inch == '!')
      {
        inbuf[inbuf_pos++] = inch;
      }
      else
      {
        // skip
      }
    }
    else
    {
      // Already started a line.  Is this CR?
      if (inch == '\n')
      {
        Serial.println("NL");
        Serial.println(inbuf_pos);
        // Yes : try to parse.  We expect to have received 5 chars total. ('! nnn')
        if (inbuf_pos == 5)
        {
          inbuf[inbuf_pos] = 0;
          int val = atoi(inbuf + 2);
          showMeterPercent(val);
          // reset
          inbuf_pos = 0;
        }
        else
        {
          // Junk, reset.
          inbuf_pos = 0;
        }
      }
      else
      {
        // If there's room, write the char in the buffer, else reset.
        if (inbuf_pos < MAXBUF)
        {
          inbuf[inbuf_pos++] = inch;
        }
        else
        {
          inbuf_pos = 0;
        }
      }
    }
  }
}

