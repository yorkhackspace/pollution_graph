#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
// For the Adafruit LCD.
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#ifdef __AVR_ATtiny85__
 #include <avr/power.h>
#endif

// Choose which 2 pins you will use for output.
// Can be any valid output pins.
// The colors of the wires may be totally different so
// BE SURE TO CHECK YOUR PIXELS TO SEE WHICH WIRES TO USE!

uint8_t dataPin  = 12;    // Yellow wire on Adafruit Pixels
uint8_t clockPin = 13;    // Green wire on Adafruit Pixels

// rotary_encoder
const uint8_t re_pin_A = 10;
const uint8_t re_pin_B = 11;

unsigned long currentTime;
unsigned long loopTime;
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev = 0;

#include "data.h"
// Time displayed on 7-seg, also the time we will display
// data for.  hours * 100 + mins.
int shownTime;
#define MAX_TIME 2300
#define MIN_TIME 500
// Num mins to move for each rotary click
#define TIME_INCREMENT 60

// Type of data source to read from.  Presumably options are serial link, and SD card.
// Set to 1 to enable.
#define DATA_SOURCE_SERIAL  0
#define DATA_SOURCE_FAKE    0
#define DATA_SOURCE_MEM     1

// Ditto flavours of 7-segment display
#define SEVENSEG_SERIAL   0
#define SEVENSEG_I2C      1

#if SEVENSEG_SERIAL
// This is to drive the sparkfun 7 seg
#include <SoftwareSerial.h>
// Rx Tx
SoftwareSerial serial7seg(7,10);
#endif


#if SEVENSEG_I2C
// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70

// Create display and DS1307 objects.  These are global variables that
// can be accessed from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();
#endif



#define TRUE 1
#define FALSE 0

const byte first_reed = 0;
const byte last_reed = 9;

// which reed switch is active?
#define NO_REEDS 255
byte reed = NO_REEDS;


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

/* Helper functions */

// Read the reed switches, return a number indicating which location is activated 
// (or 255 if none).
byte getCurrentReed()
{
  for (int l = first_reed ; l <= last_reed ; l++ ) {
    if (!digitalRead(l)) {
        return l;
    }
  }
  return NO_REEDS;
}

// update the shownTime, taking care of
// minutes to hours calculations.
// param 'amount' is signed.  0 is accepted.
void addToShownTime(int amount)
{
  // Don't do anything if we're about to go beyond an endstop.
  if (((shownTime == MIN_TIME) && (amount < 0)) ||
      ((shownTime == MAX_TIME) && (amount > 0)))
  {
    return;
  }
  
  int hours = shownTime / 100;
  int mins = shownTime % 100;
  mins += amount;

  while (mins > 59)
  {
    hours += 1;
    mins -= 60;
  }

  while (mins < 0)
  {
    hours -= 1;
    mins += 60;
  }

  // hard stop for max/min limits to time
  shownTime = hours * 100 + mins;
  if (shownTime < MIN_TIME)
  {
    shownTime = MIN_TIME;
  }
  else if (shownTime > MAX_TIME)
  {
    shownTime = MAX_TIME;
  }

  // Also update 7-seg
#if SEVENSEG_SERIAL
  char tempString[5];
  serial7seg.write('y'); // Move cursor
  serial7seg.write((byte)0);   // to position 0
  sprintf(tempString, "%04d", shownTime);
  tempString[4] = 0;
  serial7seg.print(tempString);
  Serial.println(tempString);
#endif
#if SEVENSEG_I2C
  // Now print the time value to the display.
  clockDisplay.print(shownTime, DEC);
  clockDisplay.writeDigitRaw(2, 0x02);  // turn on the central colon.
  clockDisplay.drawColon(true);
  clockDisplay.writeDisplay();
#endif

//  delay(200);
}

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
  if (percentage > 100)
  {
    percentage = 100;
  }
  else if (percentage < 0)
  {
    percentage = 0;
  }

//  Serial.println(percentage, DEC);
  
  // Convert the input percentage into a number of LEDs to light.
  int num_leds = strip.numPixels() * percentage / 100;

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
  /*
  analogWrite(6, r);
  analogWrite(10, g);
  analogWrite(11, b);
  */
}

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  strip.begin();
  // Update LED contents, to start they are all 'off'
  strip.show();
  Serial.begin(57600);
  inbuf_pos = 0;

  // Setup the rotary encoder
  pinMode(re_pin_A, INPUT);
  pinMode(re_pin_B, INPUT);
  currentTime = millis();
  loopTime = currentTime;

  // setup the 7 seg display
#if SEVENSEG_SERIAL
  serial7seg.begin(9600);
  serial7seg.write("v");  // CLEAR
#endif
#if SEVENSEG_I2C
  // Setup the display.
  clockDisplay.begin(DISPLAY_ADDRESS);
  clockDisplay.writeDigitRaw(2, 0x02);  // turn on the central colon.
  clockDisplay.drawColon(true);
  clockDisplay.writeDisplay();
#endif

  // Init to mid-day.  addToShownTime has an implicit write
  // to the display.
  shownTime = 1200;
  addToShownTime(0);

  for (int l = first_reed ; l <= last_reed; l += 1 ) {
    pinMode(l, INPUT);
    digitalWrite(l, HIGH);
  }
}

#if DATA_SOURCE_FAKE || DATA_SOURCE_MEM
int percentage;
#endif

void requestNewData(byte reed, int shown_time)
{
#if DATA_SOURCE_SERIAL
  TODO.
#endif
#if DATA_SOURCE_FAKE
  percentage = reed * 4 * (shown_time / 100);  
  percentage = reed * 4;
  Serial.print("%:");
  Serial.print(shown_time);
  Serial.print(":");
  Serial.println(percentage);
#endif   
#if DATA_SOURCE_MEM
  if (reed == NO_REEDS)
  {
    percentage = 0;
  }
  else
  {
    percentage = dataPoints[reed][(shown_time / 100) - 5];
  }
#endif
}

// Check whether the data source has sent us an update.  If so, then display the new data.
void checkForNewData()
{
#if DATA_SOURCE_FAKE || DATA_SOURCE_MEM
  showMeterPercent(percentage);  
#endif
#if DATA_SOURCE_SERIAL
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
#endif
}

void loop()
{
  byte inputChanged = FALSE;

  /*
  // Ramp the meter from 0 to 100%
  Serial.println("BEGIN");
  for (int ii = 0; ii <= 100; ii += 4)
  {
    showMeterPercent(ii);
    Serial.println(ii);
    delay(200);
  }
  
  analogWrite(6, 0);
  analogWrite(10, 0);
  analogWrite(11, 0);
  
  Serial.println("PAUSE");
  delay(1000);
  */

  // Deal with the rotary encoder
  currentTime = millis();
  // 5ms ~ 200Hz
  if (currentTime >= (loopTime + 5)) {
    encoder_A = digitalRead(re_pin_A);
    encoder_B = digitalRead(re_pin_B);
    if ((!encoder_A) && (encoder_A_prev)) {
      if (encoder_B) {
        // clockwise
        Serial.println("Tc");
        addToShownTime(-TIME_INCREMENT);
        inputChanged = TRUE;
      } else {
        // anticlockwise
        Serial.println("Ta");
        addToShownTime(TIME_INCREMENT);
        inputChanged = TRUE;
      }
    }
    encoder_A_prev = encoder_A;
    loopTime = currentTime;
  }

  // Check reeds
  byte oldReed = reed;
  reed = getCurrentReed();
  if ( reed == NO_REEDS ) {
    reed = oldReed;
  }
  inputChanged |= (reed != oldReed);

  // This request/check pattern allows for async data feeds.
  if (inputChanged )
  {
    requestNewData(reed, shownTime);
    Serial.print("Reed: ");
    Serial.println(reed);
  }
 
  checkForNewData();
}

