/***********************************************************************

Mini Clock v1.0, Jul 2014 by Nick Hall
Distributed under the terms of the GPL.

For help on how to build the clock see my blog:
http://123led.wordpress.com/

=======================================================================

Modified by Ratti3 - 18 Aug 2019
Mini Clock v1.2 (ESP01 Version, LED Switch power via pin D8)
Tested on IDE v1.8.9

29,478 bytes 95%
1,257 bytes 61%

https://github.com/Ratti3/miniclock
https://youtu.be/krdAU_GUc3k
https://create.arduino.cc/projecthub/Ratti3/led-matrix-ntp-clock-with-ds3231-bme280-bh1750-esp01-fdde2b

***********************************************************************/

//include libraries:
#include "ProgmemData.h"                 // Progmem Storage File, holds day, month and time names, frees up precious RAM
#include <LedControl.h>                  // v1.0.6 https://github.com/wayoda/LedControl
#include <FontLEDClock.h>                // https://github.com/javastraat/arduino/blob/master/libraries/FontLEDClock/FontLEDClock.h - however, it has been modified
#include <Wire.h>                        // Standard Arduino library
#include <RTClib.h>                      // v1.2.4 DS3231 RTC - https://github.com/adafruit/RTClib
#include <Button.h>                      // https://github.com/tigoe/Button
#include <Adafruit_Sensor.h>             // v1.0.3 Required by BME280 - https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_BME280.h>             // v1.0.9 BME280 Environmental Sensor -  https://github.com/adafruit/Adafruit_BME280_Library
#include <BH1750FVI.h>                   // v1.1.1 BH1750 Light Sensor - https://github.com/PeterEmbedded/BH1750FVI
#include <SoftwareSerial.h>              // Used to show NTP data from ESP01
#include <EEPROM.h>                      // Used to save settings to Arduino EEPROM

// Setup LED Matrix
// pin 12 is connected to the DataIn on the display
// pin 11 is connected to the CLK on the display
// pin 10 is connected to LOAD on the display
LedControl lc = LedControl(12, 11, 10, 4); //sets the 3 pins as 12, 11 & 10 and then sets 4 displays (max is 8 displays)

// Global variables (changeable defaults), numbers in [] brackets are the EEPROM storage location for that value
  // Clock settings
byte intensity = 2;                      // [200] Default intensity/brightness (0-15), can be set via menu
byte clock_mode = 0;                     // [201] Default clock mode. Default = 0 (basic_mode)
bool random_mode = 0;                    // [206] Define random mode - changes the display type every few hours. Default = 0 (off)
bool random_font_mode = 0;               // [207] Define font random mode - changes the font every few hours. 1 = random font on
bool ampm = 0;                           // [208] Define 12 or 24 hour time. 0 = 24 hour. 1 = 12 hour
  // Light settings
byte display_mode = 5;                   // [202] Default display on/off mode, used by light sensor. 0 = normal, 1 = always on, 2 - always off, 3 - 5 = defined by hour_off_1,2,3
bool auto_intensity = 1;                 // [209] Default auto light intensity setting
byte hour_off_1 = 21;                    // These three define the hour light sensor can turn off display if dark enough, format is 24 hours, the routine for
byte hour_off_2 = 22;                    // this checks between 8.00 and one of these values
byte hour_off_3 = 23;
  // Font settings - these are set via the setup Font menu, see set_font_case() routine for all default values:
byte font_style = 2;                     // [203] Default clock large font style
byte font_offset = 1;                    // [204] Default clock large font offset adjustment
byte font_cols = 6;                      // [205] Default clock large font columns adjustment
  // DST NTP and UTC settings
bool dst_mode = 1;                       // [210] Enable DST function, 1 = enable, 0 = disable
bool ntp_mode = 1;                       // [211] Enable NTP function, 1 = enable, 0 = disable
byte ntp_adjust = 1;                     // Number of seconds to adjust NTP value before applying to DS3231, takes a few hundred milliseconds to process the ESP01 data
int8_t utc_offset = 0;                   // [213] UTC offset adjustment, hours
byte ntp_dst_hour = 2;                   // The hour daily NTP/DST sync happens, should be left at 2am if using DST mode
byte ntp_max_retry = 3;                  // Number of time to retry NTP request 1 = 35 seconds(ish) in total, values 1 - 9
byte ntp_timeout = 45;                   // Used to calculate when to quit ntp() when it's not receiving data, value in seconds, it is multiplied by ntp_max_retry
  // Global constants - SSID and password for WiFi, passed to ESP01 via SoftwareSerial
  // The combined SSID and password length cannot exceed 72 characters
const byte ssid_len = 12;                // The length of your SSID name, e.g SSID = MyWifi, ssid_len = 6
const char ssid[] = "xxxxxxxxxxxx";      // Your SSID name, e.g MyWifi
const byte pass_len = 11;                // The length of your SSID password, e.g password = password, pass_len = 8
const char pass[] = "xxxxxxxxxxx";       // Your SSID password, e.g password

// Global variables
bool shut = 0;                           // Stores matrix on/off state
byte old_mode = clock_mode;              // Stores the previous clock mode, so if we go to date or whatever, we know what mode to go back to after.
byte change_mode_time = 0;               // Holds hour when clock mode will next change if in random mode.
unsigned long delaytime = 500;           // We always wait a bit between updates of the display
int rtc[7];                              // Holds real time clock output
int light_count = 0;                     // Counter for light routine
byte auto_intensity_value = 0;           // Stores the last intensity value set by the light sensor, this value is set automatically
char words[1];                           // Holds word clock words, retrieved from progmem
bool DST = 0;                            // [212] Holds DST applied value, 1 = summertime +1hr applied, this is to ensure DST +1/-1 runs only once
bool dst_ntp_run = 0;                    // Holds the value to see if ntp() and dst() have run once a day
byte FirstRunValue = 128;                // The check digits to see if EEPROM has values saved, change this [1-254] if you want to reset EEPROM to default values
byte FirstRunAddress = 255;              // [255] Address on EEPROM FirstRunValue is saved

//define constants
#define NUM_DISPLAY_MODES  3                    // Number display modes = 3 (counting zero as the first mode)
#define NUM_SETTINGS_MODES 9                    // Number settings modes = 9 (counting zero as the first mode)
#define NUM_FONTS          7                    // Number of fonts, as defined in FontLEDClock.h
#define SLIDE_DELAY        20                   // The time in milliseconds for the slide effect per character in slide mode. Make this higher for a slower effect
#define cls                clear_display        // Clear display
#define RandomSeed         A0                   // Pin used to generate random seed
#define TX                 6                    // RX pin of ESP01
#define RX                 7                    // TX pin of ESP01
//these can be used to change the order of displays, some displays from ebay are wrong way round
#define Matrix0            0
#define Matrix1            1
#define Matrix2            2
#define Matrix3            3

RTC_DS3231 ds3231;                              // Create RTC object
Adafruit_BME280 bme;                            // BME280 object (pins 4 and 5 and 3.3v)
BH1750FVI lux(BH1750FVI::k_DevModeContHighRes); // BH1750 object (pins 4 and 5 and 3.3v)

Button buttonA = Button(2, BUTTON_PULLUP);      // Menu button
Button buttonB = Button(3, BUTTON_PULLUP);      // Display date / + button
Button buttonC = Button(4, BUTTON_PULLUP);      // Temp/Humidity/Pressure / - button

SoftwareSerial esp(RX, TX); // 7, 6             // Software Serial 7 and 6, ESP01 serial connects to these, (pins RX and TX and 3.3v via external regulator)

void setup() {

  digitalWrite(2, HIGH);                        // turn on pullup resistor for button on pin 2
  digitalWrite(3, HIGH);                        // turn on pullup resistor for button on pin 3
  digitalWrite(4, HIGH);                        // turn on pullup resistor for button on pin 4

  pinMode(8, OUTPUT);                           // I use this to control the LEDs on the switches
  digitalWrite(8, HIGH);                        // Turn on the power for LED switches
  
  Serial.begin(9600); //start serial
  esp.begin(9600);    //start software serial for ESP01

  //initialize the 4 matrix panels
  //we have already set the number of devices when we created the LedControl
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  for (int address = 0; address < devices; address++) {
    lc.shutdown(address, false);         // The MAX72XX is in power-saving mode on startup
    lc.setIntensity(address, intensity); // Set the brightness to a medium values
    lc.clearDisplay(address);            // and clear the display
  }

  //I2C
  Wire.begin();

  //Setup DS3231 RTC
  ds3231.begin();
  //ds3231.adjust(DateTime(2010, 6, 29, 12, 59, 40));  // Set time manually
  //ds3231.adjust(DateTime(__DATE__, __TIME__)); // sets the RTC to the date & time this sketch was compiled
  if (!ds3231.begin()) {
    Serial.println("Couldn't find RTC");
    while(1);
  }

  //BME280 environmental sensor, this sensor from Ebay has address 0x76
  bme.begin(0x76);
  //Reduce BME sampling rate to prevent overheating
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
    Adafruit_BME280::SAMPLING_X1, // temperature
    Adafruit_BME280::SAMPLING_X1, // pressure
    Adafruit_BME280::SAMPLING_X1, // humidity
    Adafruit_BME280::FILTER_OFF);

  //BH1750 light sensor
  lux.begin();

  //what is this silliness, needed for random() to work properly
  randomSeed(analogRead(RandomSeed)); // Pin A0

  //Show software version & startup message
  printver();

  byte FirstRun = eeprom_read_byte(FirstRunAddress);
  bool FR = 0;
  if (FirstRun == FirstRunValue) FR = 1;

  byte value1;
  bool value2;
  int8_t value3;

  /*Serial.print("Read FirstRun: [");
  Serial.print(FirstRunAddress);
  Serial.print("] ");
  Serial.println(FirstRun);*/

  byte i = 200;
  while (i < 214) {
    if (i <= 205) {
    
      value1 = eeprom_read_byte(i);
      if (i == 200) {
        if (FR) {
          intensity = value1;
        }
        else {
          value1 = intensity;
        }
      }
      else if (i == 201) {
        if (FR) {
          clock_mode = value1;
          old_mode = clock_mode;
        }
        else {
          value1 = clock_mode;
        }
      }
      else if (i == 202) {
        if (FR) {
          display_mode = value1;
        }
        else {
          value1 = display_mode;
        }
      }
      else if (i == 203) {
        if (FR) {
          font_style = value1;
        }
        else {
          value1 = font_style;
        }
      }
      else if (i == 204) {
        if (FR) {
          font_offset = value1;
        }
        else {
          value1 = font_offset;
        }
      }
      else if (i == 205) {
        if (FR) {
          font_cols = value1;
        }
        else {
          value1 = font_cols;
        }
      }

      if (!FR) {
        eeprom_save(i, value1, 0, 0);
        /*Serial.print("FirstRun - Update Byte: [");
        Serial.print(i);
        Serial.print("] ");
        Serial.println(value1);*/
      }
      //Serial.print("Read Byte: [");
      //Serial.print(i);
      //Serial.print("] ");
      //Serial.println(value1);
    
    }
    else if (i >= 206 && i <= 212) {
    
      value2 = eeprom_read_bool(i);
      if (i == 206) {
        if (FR) {
          random_mode = value2;
        }
        else {
          value2 = random_mode;
        }
      }
      else if (i == 207) {
        if (FR) {
          random_font_mode = value2;
        }
        else {
          value2 = random_font_mode;
        }
      }
      else if (i == 208) {
        if (FR) {
          ampm = value2;
        }
        else {
          value2 = ampm;
        }
      }
      else if (i == 209) {
        if (FR) {
          auto_intensity = value2;
        }
        else {
          value2 = auto_intensity;
        }
      }
      else if (i == 210) {
        if (FR) {
          dst_mode = value2;
        }
        else {
          value2 = dst_mode;
        }
      }
      else if (i == 211) {
        if (FR) {
          ntp_mode = value2;
        }
        else {
          value2 = ntp_mode;
        }
      }
      else if (i == 212) {
        if (FR) {
          DST = value2;
        }
        else {
          value2 = DST;
        }
      }

      if (!FR) {
        eeprom_save(i, 0, value2, 0);
        /*Serial.print("FirstRun - Update Bool: [");
        Serial.print(i);
        Serial.print("] ");
        Serial.println(value2);*/
      }
      //Serial.print("Read Bool: [");
      //Serial.print(i);
      //Serial.print("] ");
      //Serial.println(value2);

    }
    else if (i == 213) {
    
      value3 = eeprom_read_int8_t(i);
      if (FR) {
        utc_offset = value3;
      }
      else {
        value3 = utc_offset;
      }

      if (!FR) {
        eeprom_save(i, 0, 0, value3);
        /*Serial.print("FirstRun - Update int8_t:");
        Serial.print(i);
        Serial.print("] ");
        Serial.println(value3);*/
      }
      //Serial.print("Read int8_t: [");
      //Serial.print(i);
      //Serial.print("] ");
      //Serial.println(value3);
    
    }
    i++;
  }

  if (!FR) {
    eeprom_save(FirstRunAddress, FirstRunValue, 0, 0);
    /*Serial.print("Update FirstRun: [");
    Serial.print(FirstRunAddress);
    Serial.print("] ");
    Serial.println(FirstRunValue);*/
  }

  //get time from NTP server if ntp_mode = 1
  if (ntp_mode) {
    ntp();
  }
  //run dst() calculation, 0 means not triggered by ntp()
  if (!ntp_mode && dst_mode) {
    dst(0);
  }

}


void loop() {

  //run the clock with whatever mode is set by clock_mode - the default is set at top of code.
  switch(clock_mode) {
  case 0:
    basic_mode();
    break;
  case 1:
    small_mode();
    break;
  case 2:
    slide();
    break;
  case 3:
    word_clock();
    break;
  case 4:
    setup_menu();
    break;
  }

}


//function to save settings to Arduino EEPROM
void eeprom_save(byte Address, byte value1, bool value2, int8_t value3) {

  switch(Address) {
    case 200 ... 205:
      EEPROM.update(Address, value1);
      //Serial.print("Update Byte: [");
      //Serial.print(Address);
      //Serial.print("] ");
      //Serial.println(value1);
      break;
    case 206 ... 212:
      EEPROM.update(Address, value2);
      //Serial.print("Update Bool: [");
      //Serial.print(Address);
      //Serial.print("] ");
      //Serial.println(value2);
      break;
    case 213:
      EEPROM.update(Address, value3);
      //Serial.print("Update int8_t: [");
      //Serial.print(Address);
      //Serial.print("] ");
      //Serial.println(value3);
      break;
    case 255:
      EEPROM.update(Address, value1);
      //Serial.print("Update Byte: [");
      //Serial.print(Address);
      //Serial.print("] ");
      //Serial.println(value1);
      break;
  }
  
}


//function to read EEPROM data as byte
byte eeprom_read_byte(byte Address) {
  
  byte result = EEPROM.read(Address);
  return result;
  
}


//function to read EEPROM data as bool
bool eeprom_read_bool(byte Address) {
  
  bool result = (bool)EEPROM.read(Address);
  return result;
  
}


//function to read EEPROM data as int8_t
int8_t eeprom_read_int8_t(byte Address) {
  
  int8_t result = (int8_t)EEPROM.read(Address);
  return result;
  
}


//plot a point on the display
void plot(byte x, byte y, byte val) {

  //select which matrix depending on the x coord
  byte address;
  if (x >= 0 && x <= 7)   {
    address = Matrix0;
  }
  if (x >= 8 && x <= 15)  {
    address = Matrix1;
    x = x - 8;
  }
  if (x >= 16 && x <= 23) {
    address = Matrix2;
    x = x - 16;
  }
  if (x >= 24 && x <= 31) {
    address = Matrix3;
    x = x - 24;
  }

  if (val == 1) {
    lc.setLed(address, y, x, true);
  } else {
    lc.setLed(address, y, x, false);
  }

}


//clear screen
void clear_display() {
  
  for (byte address = 0; address < 4; address++) {
    lc.clearDisplay(address);
  }

}


//fade screen down
void fade_down() {

  byte x = 0; //to hold temp intensity value
  if (auto_intensity) {
    x = auto_intensity_value;  //uses the last light sensor intensity settings, prevents display from constantly flicking between global and light sensor value
  }
  else {
    x = intensity;
  }

  //fade from global intensity to 1
  for (byte i = x; i > 0; i--) {
    for (byte address = 0; address < 4; address++) {
      lc.setIntensity(address, i);
    }
    delay(30); //change this to change fade down speed
  }

  clear_display(); //clear display completely (off)

  //reset intentsity to global val
  for (byte address = 0; address < 4; address++) {
    lc.setIntensity(address, x);
  }

}


//power up led test & display software version number
void printver() {

  byte i = 0;
  char ver_a[9] = "Vers 1.2";
  char ver_b[9] = " Ratti3 ";

  //test all leds.
  for (byte x = 0; x <= 31; x++) {
    for (byte y = 0; y <= 7; y++) {
      plot(x, y, 1);
    }
  }
  delay(500);
  fade_down();

  while (ver_a[i]) {
    puttinychar((i * 4), 1, ver_a[i]);
    delay(35);
    i++;
  }
  delay(700);
  fade_down();
  i = 0;
  while (ver_b[i]) {
    puttinychar((i * 4), 1, ver_b[i]);
    delay(35);
    i++;
  }
  delay(700);
  fade_down();

}


// Copy a 3x5 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
// This is unoptimized and simply uses plot() to draw each dot.
void puttinychar(byte x, byte y, char c) {

  byte dots;
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
    c &= 0x1F;   // A-Z maps to 1-26
  }
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 33;
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == ':') {
    c = 28; // colon
  }
  else if (c == '\'') {
    c = 29; // single quote mark
  }
  else if (c == '>') {
    c = 30; // >
  }
  else if (c == '-') {
    c = 31; // -
  }
  else if (c == '/') {
    c = 32; // forward slash
  }

  for (byte col = 0; col < 3; col++) {
    dots = pgm_read_byte_near(&mytinyfont[c][col]);
    for (char row = 0; row < 5; row++) {
      if (dots & (16 >> row))
        plot(x + col, y + row, 1);
      else
        plot(x + col, y + row, 0);
    }
  }

}


// fs = font_style, fc = font_cols
void putnormalchar(byte x, byte y, char c, byte fs, byte fc) {

  byte dots;
  if (c >= 'A' && c <= 'Z' ) {
    c = (c - 'A') + 27;   // A-Z maps to 27-52
  }
  else if (c >= 'a' && c <= 'z') {
    c &= 0x1F;   // a-z maps to 1-26
  }
  else if (c >= '0' && c <= '9') {
    switch(fs) {
      case 1:
        c = (c - '0') + 59;   // 0-9 maps to 59-68
        break;
      case 2:
        c = (c - '0') + 69;   // 0-9 maps to 69-78
        break;
      case 3:
        c = (c - '0') + 79;   // 0-9 maps to 79-88
        break;
      case 4:
        c = (c - '0') + 89;   // 0-9 maps to 89-98
        break;
      case 5:
        c = (c - '0') + 99;   // 0-9 maps to 99-108
        break;
      case 6:
        c = (c - '0') + 69;   // 0-9 maps to 69-78
        break;
      case 7:
        c = (c - '0') + 109;   // 0-9 maps to 109-108
        break;
    }
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 53; // full stop
  }
  else if (c == '\'') {
    c = 54; // single quote mark
  }
  else if (c == ':') {
    c = 55; // colon
  }
  else if (c == '>') {
    c = 56; // clock_mode selector arrow
  }
  else if (c == '~') {
    c = 57; // degrees
  }
  else if (c == '%') {
    c = 58; // percentage
  }
  else if (c >= -80 && c <= -67) {
    c *= -1;
  }

  for (char col = 0; col < fc; col++) {
    dots = pgm_read_byte_near(&myfont[c][col]);
    for (char row = 0; row < 7; row++) {
      //check coords are on screen before trying to plot
      //if ((x >= 0) && (x <= 31) && (y >= 0) && (y <= 7)){

      if (dots & (64 >> row)) {   // only 7 rows.
        plot(x + col, y + row, 1);
      } else {
        plot(x + col, y + row, 0);
      }
      //}
    }
  }

}


//show the time in small 3x5 characters with seconds display
void small_mode() {

  char textchar[8]; // the 16 characters on the display
  byte mins = 100; //mins
  byte secs = rtc[0]; //seconds
  byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed
  
  cls();

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    //Check light levels for turning on/off matrix
    if (light_count > 100) {
      light();
      light_count = 0;
    }
    light_count++;

    get_time();
  
    //check for button press
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      return;
    }
    if (buttonC.uniquePress()) {
      display_thp();
      return;
    }
    
    //if secs changed then update them on the display
    secs = rtc[0];
    if (secs != old_secs) {

      //secs
      char buffer[3];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa converts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      puttinychar( 20, 1, ':'); //seconds colon
      puttinychar( 24, 1, buffer[0]); //seconds
      puttinychar( 28, 1, buffer[1]); //seconds
      old_secs = secs;
    }

    //if minute changes change time
    if (mins != rtc[1]) {

      //reset these for comparison next time
      mins = rtc[1];
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      //set characters
      char buffer[3];
      itoa(hours, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa converts this to chars with space "3 ".
      if (hours < 10) {
        buffer[1] = buffer[0];
        //if we are in 12 hour mode blank the leading zero.
        if (ampm) {
          buffer[0] = ' ';
        }
        else {
          buffer[0] = '0';
        }
      }
      //set hours chars
      textchar[0] = buffer[0];
      textchar[1] = buffer[1];
      textchar[2] = ':';

      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set mins characters
      textchar[3] = buffer[0];
      textchar[4] = buffer[1];

      //do seconds
      textchar[5] = ':';
      secs = rtc[0];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa converts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set seconds
      textchar[6] = buffer[0];
      textchar[7] = buffer[1];

      //print each char
      for (byte x = 0; x < 6 ; x++) {
        puttinychar( x * 4, 1, textchar[x]);
      }
    }
    delay(50);
  }
  fade_down();

}


// show the time in 5x7 characters
void basic_mode() {

  cls();

  char buffer[3];   //for int to char conversion to turn rtc values into chars we can print on screen
  byte offset = 0;  //used to offset the x postition of the digits and centre the display when we are in 12 hour mode and the clock shows only 3 digits. e.g. 3:21

  //do 12/24 hour conversion if ampm set to 1
  byte hours = rtc[2];

  if (hours > 12) {
    hours = hours - ampm * 12;
  }
  if (hours < 1) {
    hours = hours + ampm * 12;
  }

  //do offset conversion
  if (ampm && hours < 10) {
    offset = 2;
  }

  //set the next minute we show the date at
  //set_next_date();

  // initially set mins to value 100 - so it wll never equal rtc[1] on the first loop of the clock, meaning we draw the clock display when we enter the function
  byte secs = 100;
  byte mins = 100;
  int count = 0;

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    //Check light levels for turning on/off matrix
    if (light_count > 4000) {
      light();
      light_count = 0;
    }
    light_count++;

    //get the time from the clock chip
    get_time();
    
    //check for button press
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      return;
    }
    if (buttonC.uniquePress()) {
      display_thp();
      return;
    }

    //check whether it's time to automatically display the date
    //check_show_date();

    //draw the flashing : as on if the secs have changed.
    if (secs != rtc[0]) {

      //update secs with new value
      secs = rtc[0];

      //draw :
      plot (15 - offset, 2, 1); //top point
      plot (15 - offset, 5, 1); //bottom point
      count = 400;
    }

    //if count has run out, turn off the :
    if (count == 0) {
      plot (15 - offset, 2, 0); //top point
      plot (15 - offset, 5, 0); //bottom point
    }
    else {
      count--;
    }

    //re draw the display if button pressed or if mins != rtc[1] i.e. if the time has changed from what we had stored in mins, (also trigggered on first entering function when mins is 100)
    if (mins != rtc[1]) {

      //update mins and hours with the new values
      mins = rtc[1];
      hours = rtc[2];

      //adjust hours of ampm set to 12 hour mode
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      itoa(hours, buffer, 10);

      //if hours < 10 the num e.g. "3" hours, itoa coverts this to chars with space "3 " which we dont want
      if (hours < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      //print hours
      //if we in 12 hour mode and hours < 10, then don't print the leading zero, and set the offset so we centre the display with 3 digits.
      if (ampm && hours < 10) {
        offset = 2;

        //if the time is 1:00am clear the entire display as the offset changes at this time and we need to blank out the old 12:59
        if ((hours == 1 && mins == 0) ) {
          cls();
        }
      }
      else {
        //else no offset and print hours tens digit
        offset = 0;

        //if the time is 10:00am clear the entire display as the offset changes at this time and we need to blank out the old 9:59
        if (hours == 10 && mins == 0) {
          cls();
        }

        putnormalchar(1,  0, buffer[0], font_style, font_cols);
      }
      //print hours ones digit
      putnormalchar(7 - offset + font_offset, 0, buffer[1], font_style, font_cols);

      //print mins
      //add leading zero if mins < 10
      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //print mins tens and ones digits
      putnormalchar(19 - offset - font_offset - font_offset, 0, buffer[0], font_style, font_cols);
      putnormalchar(25 - offset - font_offset, 0, buffer[1], font_style, font_cols);
    }
  }
  fade_down();

}


//like basic_mode but with slide effect
void slide() {

  byte digits_old[4] = {99, 99, 99, 99}; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
  byte digits_new[4]; //new digits time will slide to reveal
  byte digits_x_pos[4] = {25, 19, 7, 1}; //x pos for which to draw each digit at

  char old_char[2]; //used when we use itoa to transpose the current digit (type byte) into a char to pass to the animation function
  char new_char[2]; //used when we use itoa to transpose the new digit (type byte) into a char to pass to the animation function

  //old_chars - stores the 5 day and date suffix chars on the display. e.g. "mon" and "st". We feed these into the slide animation as the current char when these chars are updated.
  //We sent them as A initially, which are used when the clocl enters the mode and no last chars are stored.
  //char old_chars[6] = "AAAAA";

  //plot the clock colon on the display
  cls();
  putnormalchar(13, 0, ':', font_style, font_cols);

  byte old_secs = rtc[0]; //store seconds in old_secs. We compare secs and old secs. WHen they are different we redraw the display

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    //Check light levels for turning on/offf matrix
    if (light_count > 5000) {
      light();
      light_count = 0;
    }
    light_count++;

    get_time();
    
    //check for button press
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
      if (buttonB.uniquePress()) {
      display_date();
      return;
    }
    if (buttonC.uniquePress()) {
      display_thp();
      return;
    }

    //if secs have changed then update the display
    if (rtc[0] != old_secs) {
      old_secs = rtc[0];

      //do 12/24 hour conversion if ampm set to 1
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      //split all date and time into individual digits - stick in digits_new array
      digits_new[0] = (rtc[1] % 10);         //2 - mins ones
      digits_new[1] = ((rtc[1] / 10) % 10);  //3 - mins tens
      digits_new[2] = (hours % 10);         //4 - hour ones
      digits_new[3] = ((hours / 10) % 10);  //5 - hour tens

      //draw initial screen of all chars. After this we just draw the changes.

      //compare digits 0 to 3 (mins and hours)
      for (byte i = 0; i <= 3; i++) {
        //see if digit has changed...
        if (digits_old[i] != digits_new[i]) {

          //run 9 step animation sequence for each in turn
          for (byte seq = 0; seq <= 8 ; seq++) {

            //convert digit to string
            itoa(digits_old[i], old_char, 10);
            itoa(digits_new[i], new_char, 10);

            //if set to 12 hour mode and we're on digit 2 (hours tens mode) then check to see if this is a zero. If it is, blank it instead so we get 2.00pm not 02.00pm
            if (ampm && i == 3) {
              if (digits_new[3] == 0) {
                new_char[0] = ' ';
              }
              if (digits_old[3] == 0) {
                old_char[0] = ' ';
              }
            }
            //draw the animation frame for each digit
            slideanim(digits_x_pos[i], 0, seq, old_char[0], new_char[0]);
            delay(SLIDE_DELAY);
          }
        }
      }

      //save digita array tol old for comparison next loop
      for (byte i = 0; i <= 3; i++) {
        digits_old[i] =  digits_new[i];
      }
    }//secs/oldsecs
  }//while loop
  fade_down();

}


//called by slide
//this draws the animation of one char sliding on and the other sliding off. There are 8 steps in the animation, we call the function to draw one of the steps from 0-7
//inputs are are char x and y, animation frame sequence (0-7) and the current and new chars being drawn.
void slideanim(byte x, byte y, byte sequence, char current_c, char new_c) {

  //  To slide one char off and another on we need 9 steps or frames in sequence...

  //  seq# 0123456 <-rows of the display
  //   |   |||||||
  //  seq0 0123456  START - all rows of the display 0-6 show the current characters rows 0-6
  //  seq1  012345  current char moves down one row on the display. We only see it's rows 0-5. There are at display positions 1-6 There is a blank row inserted at the top
  //  seq2 6 01234  current char moves down 2 rows. we now only see rows 0-4 at display rows 2-6 on the display. Row 1 of the display is blank. Row 0 shows row 6 of the new char
  //  seq3 56 0123
  //  seq4 456 012  half old / half new char
  //  seq5 3456 01
  //  seq6 23456 0
  //  seq7 123456
  //  seq8 0123456  END - all rows show the new char

  //from above we can see...
  //currentchar runs 0-6 then 0-5 then 0-4 all the way to 0. starting Y position increases by 1 row each time.
  //new char runs 6 then 5-6 then 4-6 then 3-6. starting Y position increases by 1 row each time.

  //if sequence number is below 7, we need to draw the current char
  if (sequence < 7) {
    byte dots;
    if (current_c >= 'A' && current_c <= 'Z' ) {
      current_c = (current_c - 'A') + 27;   // A-Z maps to 27-52
    }
    else if (current_c >= 'a' && current_c <= 'z') {
      current_c &= 0x1F;   // a-z maps to 1-26
    }
    else if (current_c >= '0' && current_c <= '9') {
      current_c = (current_c - '0') + 59;   // 0-9 maps to 59-68
    }
    else if (current_c == ' ') {
      current_c = 0; // space
    }
    else if (current_c == '.') {
      current_c = 53; // full stop
    }
    else if (current_c == '\'') {
      current_c = 54; // single quote mark
    }
    else if (current_c == ':') {
      current_c = 55; //colon
    }
    else if (current_c == '>') {
      current_c = 56; // >
    }

    byte curr_char_row_max = 7 - sequence; //the maximum number of rows to draw is 6 - sequence number
    byte start_y = sequence; //y position to start at - is same as sequence number. We inc this each loop

    //plot each row up to row maximum (calculated from sequence number)
    for (byte curr_char_row = 0; curr_char_row <= curr_char_row_max; curr_char_row++) {
      for (byte col = 0; col < 5; col++) {
        dots = pgm_read_byte_near(&myfont[current_c][col]);
        if (dots & (64 >> curr_char_row))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }

  //draw a blank line between the characters if sequence is between 1 and 7. If we don't do this we get the remnants of the current chars last position left on the display
  if (sequence >= 1 && sequence <= 8) {
    for (byte col = 0; col < 5; col++) {
      plot(x + col, y + (sequence - 1), 0); //the y position to draw the line is equivalent to the sequence number - 1
    }
  }


  //if sequence is above 2, we also need to start drawing the new char
  if (sequence >= 2) {

    //work out char
    byte dots;
    if (new_c >= 'A' && new_c <= 'Z' ) {
      new_c = (new_c - 'A') + 27;   // A-Z maps to 27-52
    }
    else if (new_c >= 'a' && new_c <= 'z') {
      new_c &= 0x1F;   // a-z maps to 1-26
    }
    else if (new_c >= '0' && new_c <= '9') {
      new_c = (new_c - '0') + 59;   // 0-9 maps to 59-68
    }
    else if (new_c == ' ') {
      new_c = 0; // space
    }
    else if (new_c == '.') {
      new_c = 27; // full stop
    }
    else if (new_c == '\'') {
      new_c = 28; // single quote mark
    }
    else if (new_c == ':') {
      new_c = 29; // colon
    }
    else if (new_c == '>') {
      new_c = 30; // >
    }

    byte newcharrowmin = 6 - (sequence - 2); //minimumm row num to draw for new char - this generates an output of 6 to 0 when fed sequence numbers 2-8. This is the minimum row to draw for the new char
    byte start_y = 0; //y position to start at - is same as sequence number. we inc it each row

    //plot each row up from row minimum (calculated by sequence number) up to 6
    for (byte newcharrow = newcharrowmin; newcharrow <= 6; newcharrow++) {
      for (byte col = 0; col < 5; col++) {
        dots = pgm_read_byte_near(&myfont[new_c][col]);
        if (dots & (64 >> newcharrow))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }
  
}


//print a clock using words rather than numbers
void word_clock() {

  cls();
  
  //potentially 3 lines to display
  char str_a[8];
  char str_b[9];
  char str_c[8];

  //byte hours_y, mins_y; //hours and mins and positions for hours and mins lines

  byte hours = rtc[2];
  if (hours > 12) {
    hours = hours - ampm * 12;
  }
  if (hours < 1) {
    hours = hours + ampm * 12;
  }

  get_time(); //get the time from the clock chip
  byte old_mins = 100; //store mins in old_mins. We compare mins and old mins & when they are different we redraw the display. Set this to 100 initially so display is drawn when mode starts.
  byte mins;

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {
    
    //check for button press
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
    }
    if (buttonC.uniquePress()) {
      display_thp();
      return;
    }

    get_time(); //get the time from the clock chip
    mins = rtc[1];  //get mins

    //if mins is different from old_mins - redraw display
    if (mins != old_mins) {

      //update old_mins with current mins value
      old_mins = mins;

      //reset these for comparison next time
      mins = rtc[1];
      hours = rtc[2];

      //make hours into 12 hour format
      if (hours > 12) {
        hours = hours - 12;
      }
      if (hours == 0) {
        hours = 12;
      }

      //split mins value up into two separate digits
      int minsdigit = rtc[1] % 10;
      byte minsdigitten = (rtc[1] / 10) % 10;

      char past[5] = "PAST";
      char to[3] = "TO";
      char half[5] = "HALF";
      char quar[8] = "QUARTER";
      char oclk[8] = "O'CLOCK";

      //if both mins are zero, i.e. it is on the hour, the top line reads "hours" and bottom line reads "o'clock"
      if (minsdigitten == 0 && minsdigit == 0  ) {
        progmem_numbers(0, hours - 1);
        strcpy (str_a, words);
        strcpy (str_b, oclk);
        strcpy (str_c, "");
      }
      //if mins <= 10 , then top line has to read "minsdigti past" and bottom line reads hours
      else if (mins < 10) {
        progmem_numbers(0, minsdigit - 1);
        strcpy (str_a, words);
        strcpy (str_b, past);
        progmem_numbers(0, hours - 1);
        strcpy (str_c, words);
      }
      //if mins = 10, cant use minsdigit as above, so soecial case to print 10 past /n hour.
      if (mins == 10) {
        progmem_numbers(0, 9);
        strcpy (str_a, words);
        strcpy (str_b, past);
        progmem_numbers(0, hours - 1);
        strcpy (str_c, words);
      }
      else if (mins == 15) {
        strcpy (str_a, quar);
        strcpy (str_b, past);
        progmem_numbers(0, hours - 1);
        strcpy (str_c, words);
      }
      else if (mins == 20) {
        progmem_numbers(1, minsdigitten - 1);
        strcpy (str_a, words);
        strcpy (str_b, past);
        progmem_numbers(0, hours - 1);
        strcpy (str_c, words);
      }
      else if (mins == 30) {
        strcpy (str_a, half);
        strcpy (str_b, past);
        progmem_numbers(0, hours - 1);
        strcpy (str_c, words);
      }
      else if (mins == 40) {
        progmem_numbers(1, 1);
        strcpy (str_a, words);
        strcpy (str_b, to);
        if (hours == 12) {
          progmem_numbers(0, hours - hours);
        }
        else {
          progmem_numbers(0, hours);

        }
        strcpy (str_c, words);
      }
      else if (mins == 50) {
        progmem_numbers(0, 9);
        strcpy (str_a, words);
        strcpy (str_b, to);
        if (hours == 12) {
          progmem_numbers(0, hours - hours);
        }
        else {
          progmem_numbers(0, hours);

        }
        strcpy (str_c, words);
      }
      else if (mins == 45) {
        strcpy (str_a, quar);
        strcpy (str_b, to);
        if (hours == 12) {
          progmem_numbers(0, hours - hours);
        }
        else {
          progmem_numbers(0, hours);

        }
        strcpy (str_c, words);
      }

      //if time is not on the hour - i.e. both mins digits are not zero,
      //then make first line read "hours" and 2 & 3rd lines read "minstens"  "mins" e.g. "three /n twenty /n one"

      else if (minsdigitten != 0 && minsdigit != 0) {
        progmem_numbers(0, hours - 1);
        strcpy (str_a, words);

        //if mins is in the teens, use teens from the numbers array for the 2nd line, e.g. "fifteen"
        if (mins >= 11 && mins <= 19) {
          progmem_numbers(0, mins - 1);
          strcpy (str_a, words);
          strcpy (str_b, past);
          progmem_numbers(0, hours - 1);
          strcpy (str_c, words);
        }
        else if (mins > 50) {
          progmem_numbers(0, 60 - (mins + 1));
          strcpy (str_a, words);
          strcpy (str_b, to);
          if (hours == 12) {
            progmem_numbers(0, hours - 12);
            strcpy (str_c, words);
          }
          else {
            progmem_numbers(0, hours);
            strcpy (str_c, words);
          }
        }
        else {
          progmem_numbers(1, minsdigitten - 1);
          strcpy (str_b, words);
          progmem_numbers(0, minsdigit - 1);
          strcpy (str_c, words);
        }
      }
    }//end working out time

    //run in a loop
    //print line a "twelve"
    byte len = 0;
    while (str_a[len]) {
      len++;
    }; //get length of message
    byte offset_top = (31 - ((len - 1) * 4)) / 2; //

    //plot hours line
    byte i = 0;
    while (str_a[i]) {
      puttinychar((i * 4) + offset_top, 1, str_a[i]);
      i++;
    }
    
    //hold display but check for button presses
    int counter = 1000;
    while (counter > 0) {
      //check for button press
      if (buttonA.uniquePress()) {
        switch_mode();
        return;
      }
      if (buttonB.uniquePress()) {
        display_date();
      }
      if (buttonC.uniquePress()) {
        display_thp();
        return;
      }

    delay(1);
    counter--;    
    }
    fade_down();

    //print line b
    len = 0;
    while (str_b[len]) {
      len++;
    }; //get length of message
    offset_top = (31 - ((len - 1) * 4)) / 2; 

    i = 0;
    while (str_b[i]) {
      puttinychar((i * 4) + offset_top, 1, str_b[i]);
      i++;
    }

    //hold display but check for button presses
    counter = 1000;
    while (counter > 0){
      if (buttonA.uniquePress()) {
        switch_mode();
        return;
      }
      if (buttonB.uniquePress()) {
        display_date();
      }
      if (buttonC.uniquePress()) {
        display_thp();
        return;
      }

      delay(1);
      counter--;
    }
    fade_down();

    //print line c if there.
    len = 0;
    while (str_c[len]) {
      len++;
    }; //get length of message
    offset_top = (31 - ((len - 1) * 4)) / 2; 

    i = 0;
    while (str_c[i]) {
      puttinychar((i * 4) + offset_top, 1, str_c[i]);
      i++;
    }
    counter = 1000;
    while (counter > 0){
      //check for button press
      if (buttonA.uniquePress()) {
        switch_mode();
        return;
      }
      if (buttonB.uniquePress()) {
        display_date();
      }
      if (buttonC.uniquePress()) {
        display_thp();
        return;
      }

      delay(1);
      counter--;
    }
    fade_down();

    //hold display blank but check for button presses before starting again.
    counter = 1000;
    while (counter > 0){
       //check for button press
      if (buttonA.uniquePress()) {
        switch_mode();
        return;
      }
      if (buttonB.uniquePress()) {
        display_date();
      }
      if (buttonC.uniquePress()) {
        display_thp();
        return;
      }

      delay(1);
      counter--;

      //Check light levels for turning on/off matrix
      if (light_count > 2000) {
        light();
        light_count = 0;
      }
      light_count++;
    }
  }
  fade_down();

}


//used by word mode to retrieve words from progmem, m : 0 = numbers, 1 = numberstens. i = index
void progmem_numbers(byte m, byte i) {

  if (m == 0) {
    strcpy_P(words, (char *)pgm_read_word(&(numbers[i])));
  }
  else if (m == 1) {
    strcpy_P(words, (char *)pgm_read_word(&(numberstens[i])));
  }

}


//display_thp - print temperature, humidity and pressue
void display_thp()
{

  cls();
  bme.takeForcedMeasurement();
  char temp[5];
  char humi[3];
  char pres[5];
  dtostrf(bme.readTemperature(), 4, 1, temp);
  dtostrf(bme.readHumidity(), 3, 0, humi);
  dtostrf(bme.readPressure() / 100.0F, 4, 0, pres);

  byte i = 0;
  byte offset = 0;
  while (temp[i]) {
    if (i == 0) {
      offset = 2;
    }
    else if (i == 2) {
      offset = 4;
    }
    else if (i == 3) {
      offset = 1;
    }
    else {
      offset = 3;
    }
    putnormalchar(i * 6 + offset + font_offset, 0, temp[i], font_style, font_cols);
    i++;
  }
  putnormalchar(i * 6 + 2 + font_offset, 0, '~', font_style, font_cols);  // '~' translates to degress symbol via FontLEDClock.h

  delay(2000);
  fade_down();

  i = 0;
  while (humi[i]) {
    if (i < 2) {
      offset = 0;
    }
    else {
      offset = 1;
    }
    putnormalchar(i * 6 + offset + font_offset, 0, humi[i], font_style, font_cols);
    i++;
  }
  putnormalchar(i * 6 + 2 + font_offset, 0, '%', font_style, font_cols);

  delay(2000);
  fade_down();

  i = 0;
  while (pres[i]) {
    if (strlen(pres) < 4 && i == 0) {
      offset = 5;
    }
    else {
      offset = 0;
    }
    putnormalchar(i * 6 - offset, 0, pres[i], 1, 5);
    i++;
  }
  byte x = 0;
  char mb[3] = "mb";
  while (mb[x]) {
    puttinychar(i * 4 + 8, 1, mb[x]);
    x++;i++;
  }
  
  delay(2000);
  fade_down();
  cls();
    
}


//display_date - print the day of week, date and month with a flashing cursor effect
void display_date() {

  cls();

  //date suffix array
  const char suffix[4][3] = {"st", "nd", "rd", "th"};

  //read the date from the DS3231
  byte dow = rtc[3]; // day of week 0 = Sunday
  byte date = rtc[4];
  byte month = rtc[5] - 1;

  //print the day name
  
  //get length of text in pixels, that way we can centre it on the display by divindin the remaining pixels b2 and using that as an offset
  byte len = 0;
  char dayfullname[9];
  strcpy_P(dayfullname, (char *)pgm_read_word(&(daysfull[dow])));
  while(dayfullname[len]) {
    len++; 
  };
  byte offset = (31 - ((len - 1) * 4)) / 2; //our offset to centre up the text
      
  //print the name     
  int i = 0;
  while(dayfullname[i])
  {
    puttinychar((i * 4) + offset , 1, dayfullname[i]);
    i++;
  }
  delay(1000);
  fade_down();
  cls();
  
  // print date numerals
  char buffer[3];
  itoa(date, buffer, 10);
  offset = 10; //offset to centre text if 3 chars - e.g. 3rd
  
  byte s = 3; 
  if(date == 1 || date == 21 || date == 31) {
    s = 0;
  } 
  else if (date == 2 || date == 22) {
    s = 1;
  } 
  else if (date == 3 || date == 23) {
    s = 2;
  } 

  //print the 1st date number
  puttinychar(0 + offset, 1, buffer[0]);

  //if date is under 10 - then we only have 1 digit so set positions of sufix etc one character nearer
  byte suffixposx = 4;

  //if date over 9 then print second number and set xpos of suffix to be 1 char further away
  if (date > 9){
    suffixposx = 10;
    puttinychar(4 + offset, 1, buffer[1]);
    offset = 8; //offset to centre text if 4 chars
  }

  //print the 2 suffix characters
  puttinychar(suffixposx + offset, 1, suffix[s][0]); 
  puttinychar(suffixposx + 4 + offset, 1, suffix[s][1]); 
 
  delay(1000);
  fade_down();
  
  //print the month name 
  
  //get length of text in pixels, that way we can centre it on the display by divindin the remaining pixels b2 and using that as an offset
  len = 0;
  char monthfullname[9];
  strcpy_P(monthfullname, (char *)pgm_read_word(&(monthsfull[month])));
  while(monthfullname[len]) {
    len++;
  };
  offset = (31 - ((len - 1) * 4)) / 2; //our offset to centre up the text
  i = 0;
  while(monthfullname[i])
  {
    puttinychar((i * 4) + offset, 1, monthfullname[i]);
    i++;
  }
  
  delay(1000);
  fade_down();

}


//dislpay menu to change the clock mode
void switch_mode() {

  //not sure why, but this is needed to stop ampm bool getting messed up
  ampm = eeprom_read_bool(208);
  
  //remember mode we are in. We use this value if we go into settings mode, so we can change back from settings mode (6) to whatever mode we were in.
  old_mode = clock_mode;

  const char* modes[] = {">Basic", ">Small", ">Slide", ">Words", ">Setup"};

  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for (int count = 0; count < 35 ; count++) {

    //if user hits button, change the clock_mode
    if (buttonA.uniquePress() || firstrun == 1) {

      count = 0;
      cls();

      if (firstrun == 0) {
        clock_mode++;
      }
      if (clock_mode > NUM_DISPLAY_MODES + 1) {
        clock_mode = 0;
      }

      //print arrow and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[9];

      strcpy (str_top, modes[clock_mode]);

      byte i = 0;
      while (str_top[i]) {
        puttinychar(i * 4 + 1, 1, str_top[i]);
        i++;
      }
      firstrun = 0;
    }
    delay(50);
  }
  if (old_mode != clock_mode && clock_mode < 4){
      //save the values to EEPROM
      eeprom_save(201, clock_mode, 0, 0);
      //Serial.print(clock_mode);
  }

}


//run clock main loop as long as run_mode returns true
byte run_mode() {

  //if random mode is on... check the hour when we change mode.
  if (random_mode || random_font_mode) {
    //if hour value in change mode time = hours. then return false = i.e. exit mode.
    if (change_mode_time == rtc[2]) {
      //set the next random clock mode and time to change it
      set_next_random();
      //exit the current mode.
      return 0;
    }
  }
  //get time from NTP server if ntp_mode = 1
  if (ntp_mode && rtc[2] == ntp_dst_hour && !dst_ntp_run) {
    ntp();
    dst_ntp_run = 1;
  }
  //run dst() calculation, 0 means not triggered by ntp()
  if (!ntp_mode && dst_mode && rtc[2] == ntp_dst_hour && !dst_ntp_run) {
    dst(0);
    dst_ntp_run = 1;
  }
  //reset once a day dst/ntp check so it runs next day
  if (rtc[2] > ntp_dst_hour) {
    dst_ntp_run = 0;
  }
  //else return 1 - keep running in this mode
  return 1;

}


//set the next hour the clock will change mode when random mode is on, also does the random font mode
void set_next_random() {

  //set the next hour the clock mode will change - current time plus 1
  get_time();
  change_mode_time = rtc[2] + 1;

  //if change_mode_time now happens to be over 23, then set it 12am
  if (change_mode_time > 23) {
    change_mode_time = 0;
  }

  if (random_mode) {
    //set the new clock mode
    clock_mode = random(0, NUM_DISPLAY_MODES + 1);  //pick new random clock mode
  }
  if (random_font_mode) {
    //set new random font
    set_font_case(random(1, NUM_FONTS + 1));  //pick new random font mode
  }

}


//dislpay menu to change the clock settings
void setup_menu() {

  const char set_modes[10][9] = {">Rnd Clk", ">Rnd Fnt", ">12 Hr", ">Font", ">DST/NTP", ">D/Time", ">Auto LX", ">Display", ">Bright", ">Exit"};

  byte setting_mode = 0;
  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for (int count = 0; count < 35 ; count++) {

    //if user hits button, change the clock_mode
    if (buttonA.uniquePress() || firstrun == 1) {

      count = 0;
      cls();

      if (firstrun == 0) { 
        setting_mode++; 
      } 
      if (setting_mode > NUM_SETTINGS_MODES) { 
        setting_mode = 0; 
      }

      //print arrow and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[9];
    
      strcpy (str_top, set_modes[setting_mode]);

      byte i = 0;
      while(str_top[i]) {
        puttinychar(i * 4 + 1, 1, str_top[i]);
        i++;
      }

      firstrun = 0;
    }
    delay(50);
  }
  
  //pick the mode 
  switch(setting_mode) {
    case 0:
      set_random(); 
      break;
    case 1:
      set_random_font(); 
      break;
    case 2:
      set_ampm();
      break;
    case 3:
      set_font();
      break;
    case 4:
      set_ntp_dst();
      break;
    case 5:
      set_time();
      break;
    case 6:
      set_auto_intensity();
      break;
    case 7:
      set_display_options();
      break;
    case 8:
      set_intensity();
      break;
    case 9:
      //exit menu
      break;
  }
    
  //change the clock from mode 6 (settings) back to the one it was in before 
  clock_mode = old_mode;

}


//toggle random mode - pick a different clock mode every few hours
void set_random() {
  
  cls();

  //get current values
  bool set_random_mode = eeprom_read_bool(206);

  //Set function - we pass in: which 'set' message to show at top, current value
  set_random_mode = set_bool_value(3, set_random_mode);

  //set the values
  random_mode = set_random_mode;

  //set hour mode will change
  set_next_random();

  //save the values to EEPROM
  eeprom_save(206, 0, random_mode, 0);
  //Serial.println("random_mode");

}


//toggle random font
void set_random_font() {
  
  cls();
  
  //get current values
  bool set_random_font_mode = eeprom_read_bool(207);

  //Set function - we pass in: which 'set' message to show at top, current value
  set_random_font_mode = set_bool_value(3, set_random_font_mode);

  //set the values
  random_font_mode = set_random_font_mode;

  //set hour mode will change
  set_next_random();

  //save the values to EEPROM
  eeprom_save(207, 0, random_font_mode, 0);
  //Serial.println("random_font_mode");
  
}


//set 12 or 24 hour clock
void set_ampm() {

  cls();

  //get current values
  bool set_ampm_mode = eeprom_read_bool(208);

  //Set function - we pass in: which 'set' message to show at top, current value
  set_ampm_mode = set_bool_value(2, set_ampm_mode);

  //set the values
  ampm = set_ampm_mode;

  //save the values to EEPROM
  eeprom_save(208, 0, set_ampm_mode, 0);
  //Serial.println("ampm");

}


//set font style
void set_font() {

  cls();

  byte i = 0;
  char text[9] = ">Set Fnt";
  while(text[i]) {
    puttinychar(i * 4, 1, text[i]);
    i++;
  }
  
  delay(1500);
  cls();

  byte set_font_value;
  if (font_style == 2 && font_cols == 5) {
    set_font_value = 6;
  }
  else {
    set_font_value = font_style;
  }

  get_font_value(set_font_value, 1, NUM_FONTS);

  //save the values to EEPROM
  eeprom_save(203, font_style, 0, 0);
  eeprom_save(204, font_offset, 0, 0);
  eeprom_save(205, font_cols, 0, 0);
  //Serial.println("font style, offset and cols");

}


//set font_style, font_offset & font_cols variables, used by set_font()
void set_font_case(int value) {
  
  switch(value) {
    case 1:
      font_style = 1;
      font_offset = 0;
      font_cols = 5;
      break;
    case 2:
      font_style = 2;
      font_offset = 1;
      font_cols = 6;
      break;
    case 3:
      font_style = 3;
      font_offset = 1;
      font_cols = 6;
      break;
    case 4:
      font_style = 4;
      font_offset = 1;
      font_cols = 6;
      break;
    case 5:
      font_style = 5;
      font_offset = 0;
      font_cols = 5;
      break;
    case 6:
      font_style = 2;
      font_offset = 0;
      font_cols = 5;  //cheap way to create a new font (crop 1 column right side of font 2)
      break;
    case 7:
      font_style = 7;
      font_offset = 1;
      font_cols = 6;
      break;
  }

}


//get user values for setting font
void get_font_value(int current_value, int min_value, int max_value) {
  
  //print digits bottom line
  char buffer[2] = " ";
  itoa(current_value, buffer ,10);
  puttinychar(0, 1, '>'); 
  puttinychar(4, 1, buffer[0]); 
  delay(300);
  //wait for button input
  while (!buttonA.uniquePress()) {
    char preview[4] = "   ";
    //font preview numbers
    itoa(123, preview, 10);
    while (buttonB.isPressed()){

      if(current_value < max_value) { 
        current_value++;
      } 
      else {
        current_value = min_value;
      }
      //print the new value
      cls();
      itoa(current_value, buffer, 10);
      puttinychar(0, 1, '>'); 
      puttinychar(4, 1, buffer[0]);
      //preview the font and set the font
      set_font_case(current_value);
      byte i = 0;
      while(preview[i]) {
        putnormalchar(i * (font_cols + 1) + 10, 0, preview[i], font_style, font_cols);
        i++;
      }
      delay(150);
    }

    while (buttonC.isPressed()) {

      if(current_value > min_value) {
        current_value--;
      } 
      else {
        current_value = max_value;
      }
      //print the new value
      cls();
      itoa(current_value, buffer ,10);
      puttinychar(0, 1, '>');
      puttinychar(4, 1, buffer[0]);
      //preview the font and set the font
      set_font_case(current_value);
      byte i = 0;
      while(preview[i]) {
        putnormalchar(i * (font_cols + 1) + 10, 0, preview[i], font_style, font_cols);
        i++;
      }
      delay(150);
    }

  }
  //return current_value;

}


//set ntp and dst settings
void set_ntp_dst() {

  cls();

  //get current values
  bool set_dst_mode = eeprom_read_bool(210);
  bool set_ntp_mode = eeprom_read_bool(211);
  int8_t set_utc_offset = eeprom_read_int8_t(213);

  //Set function - we pass in: which 'set' message to show at top, current value, reset value, and rollover limit.
  set_dst_mode = set_bool_value(0, set_dst_mode);
  set_ntp_mode = set_bool_value(1, set_ntp_mode);
  set_utc_offset = set_ntp_dst_int8_t_value(set_utc_offset, -12, 12);

  //set the values
  dst_mode = set_dst_mode;
  ntp_mode = set_ntp_mode;
  utc_offset = set_utc_offset;
  
  //save the values to EEPROM
  eeprom_save(210, 0, set_dst_mode, 0);
  eeprom_save(211, 0, set_ntp_mode, 0);
  eeprom_save(213, 0, 0, set_utc_offset);
  //Serial.println(set_dst_mode);
  //Serial.println(set_ntp_mode);
  //Serial.println(set_utc_offset);
  
  //cls();

}


//used to set bool for DST, NTP, 12h, Random and Light
//message = which 'set' message to print, 
//current value = current value of property we are setting
bool set_bool_value(byte message, bool current_value){

  cls();

  const char options[5][9] = {">Set DST", ">Set NTP", ">Set 12h", ">Set Rnd", ">Set LX"};

  //Print "set xyz" top line
  byte i = 0;
  while(options[message][i])
  {
    puttinychar(i * 4, 1, options[message][i]);
    i++;
  }

  delay(1500);
  cls();

  char text[2][5] = {">OFF", ">ON "};

  //print current value
  i = 0;
  while(text[current_value][i]) {
    puttinychar(i * 4, 1, text[current_value][i]);
    i++;
  }
  delay(300);
  //wait for button input
  while (!buttonA.uniquePress()) {

    while (buttonB.isPressed()) {

      current_value = (current_value ^ 1);
      
      //print the new value
      i = 0;
      while(text[current_value][i]) {
        puttinychar(i * 4, 1, text[current_value][i]);
        i++;
      }
      delay(150);
    }

    while (buttonC.isPressed()) {

      current_value = (current_value ^ 1);

      //print the new value
      i = 0;
      while(text[current_value][i]) {
        puttinychar(i * 4, 1, text[current_value][i]);
        i++;
      }
      delay(150);
    }
    
  }
  return current_value;

}


//used to set int8_t number for UTC offset
//current value = current value of property we are setting
//reset_value = what to reset value to if to rolls over. E.g. hours roll from -12 to 12
//rollover limit = when value rolls over
int8_t set_ntp_dst_int8_t_value(int8_t current_value, int8_t reset_value, int8_t rollover_limit){

  cls();
  char messages[9] = {">Set UTC"};

  //Print "set xyz" top line
  byte i = 0;
  while(messages[i])
  {
    puttinychar(i * 4, 1, messages[i]); 
    i++;
  }

  delay(1500);
  cls();

  //print digits bottom line
  char buffer[5] = "    ";
  itoa(current_value,buffer,10);
  puttinychar(0 , 1, '>');
  puttinychar(4 , 1, buffer[0]);
  puttinychar(8 , 1, buffer[1]);
  puttinychar(12, 1, buffer[2]);
  puttinychar(16, 1, buffer[3]);
  delay(300);
  //wait for button input
  while (!buttonA.uniquePress()) {

    while (buttonB.isPressed()) {

      if(current_value < rollover_limit) {
        current_value++;
      }
      else {
        current_value = reset_value;
      }
      //print the new value
      itoa(current_value, buffer ,10);
      puttinychar(0 , 1, '>');
      puttinychar(4 , 1, buffer[0]);
      puttinychar(8 , 1, buffer[1]);
      puttinychar(12, 1, buffer[2]);
      puttinychar(16, 1, buffer[3]);
      delay(150);
    }

    while (buttonC.isPressed()) {

      if(current_value > reset_value) {
        current_value--;
      }
      else {
        current_value = rollover_limit;
      }
      //print the new value
      itoa(current_value, buffer, 10);
      puttinychar(0 , 1, '>');
      puttinychar(4 , 1, buffer[0]);
      puttinychar(8 , 1, buffer[1]);
      puttinychar(12, 1, buffer[2]);
      puttinychar(16, 1, buffer[3]);
      delay(150);
    }
    
  }
  return current_value;
}


//change screen intensity
void set_intensity() {

  cls();
  
  byte i = 0;
  char text[8] = ">Bright";
  while(text[i]) {
    puttinychar((i * 4) + 3, 0, text[i]);
    i++;
  }

  //wait for button input
  while (!buttonA.uniquePress()) {

    levelbar (0, 6,(intensity * 2) + 2, 2);    //display the intensity level as a bar
    while (buttonB.isPressed()) {
      if(intensity == 15) { 
        intensity = 0;
        cls ();
      }
      else {
        intensity++;
      }
      //print the new value 
      i = 0;
      while(text[i]) {
        puttinychar((i * 4) + 4, 0, text[i]);
        i++;
      }
      
      //display the intensity level as a bar
      levelbar (0, 6, (intensity * 2) + 2, 2);    
      
      //change the brightness setting on the displays
      for (byte address = 0; address < 4; address++) {
        lc.setIntensity(address, intensity);
      }
      delay(150);
    }

    while (buttonC.isPressed()) {
      if(intensity == 0) { 
        intensity = 15;
      }
      else {
        intensity--;
      }
      //display the intensity level as a bar
      cls ();
      levelbar (0, 6,(intensity * 2) + 2, 2);
      
      //print the new value 
      i = 0;
      while(text[i]) {
        puttinychar((i * 4) + 4, 0, text[i]);
        i++;
      }
      
      //change the brightness setting on the displays
      for (byte address = 0; address < 4; address++) {
        lc.setIntensity(address, intensity);
      }
      delay(150);
    }

  }

  //save the values to EEPROM
  eeprom_save(200, intensity, 0, 0);
  //Serial.println("intensity");

}


// menu for setting auto intensity settings
void set_auto_intensity() {

  cls();

  //get current values
  bool set_auto_intensity_value = eeprom_read_bool(209);

  //Set function - we pass in: which 'set' message to show at top, current value
  set_auto_intensity_value = set_bool_value(4, set_auto_intensity_value);

  //set the values
  auto_intensity = set_auto_intensity_value;

  //save the values to EEPROM
  eeprom_save(209, 0, auto_intensity, 0);
  //Serial.println("auto_intensity");

}


// display a horizontal bar on the screen at offset xposr by ypos with height and width of xbar, ybar
void levelbar (byte xpos, byte ypos, byte xbar, byte ybar) {

  for (byte x = 0; x < xbar; x++) {
    for (byte y = 0; y <= ybar; y++) {
      plot(x + xpos, y + ypos, 1);
    }
  }

}


//set time and date routine
void set_time() {

  cls();

  //fill settings with current clock values read from clock
  get_time();
  byte set_min   = rtc[1];
  byte set_hr    = rtc[2];
  byte set_date  = rtc[4];
  byte set_mnth  = rtc[5];
  int  set_yr    = rtc[6]; 

  //Set function - we pass in: which 'set' message to show at top, current value, reset value, and rollover limit.
  set_date = set_value(2, set_date, 1, 31);
  set_mnth = set_value(3, set_mnth, 1, 12);
  set_yr   = set_value(4, set_yr, 2019, 2099);
  set_hr   = set_value(1, set_hr, 0, 23);
  set_min  = set_value(0, set_min, 0, 59);

  ds3231.adjust(DateTime(set_yr, set_mnth, set_date, set_hr, set_min));
  
  cls();

}


//used to set min, hr, date, month, year values. pass 
//message = which 'set' message to print, 
//current value = current value of property we are setting
//reset_value = what to reset value to if to rolls over. E.g. mins roll from 60 to 0, months from 12 to 1
//rollover limit = when value rolls over
int set_value(byte message, int current_value, int reset_value, int rollover_limit) {

  cls();
  char messages[5][9] = {">Set Min", ">Set Hr", ">Set Day", ">Set Mth", ">Set Yr"};

  //Print "set xyz" top line
  byte i = 0;
  while(messages[message][i])
  {
    puttinychar(i * 4, 1, messages[message][i]); 
    i++;
  }

  delay(1500);
  cls();

  //print digits bottom line
  char buffer[5] = "    ";
  itoa(current_value,buffer,10);
  puttinychar(0 , 1, '>');
  puttinychar(4 , 1, buffer[0]);
  puttinychar(8 , 1, buffer[1]);
  puttinychar(12, 1, buffer[2]);
  puttinychar(16, 1, buffer[3]);
  delay(300);
  //wait for button input
  while (!buttonA.uniquePress()) {

    while (buttonB.isPressed()) {

      if(current_value < rollover_limit) {
        current_value++;
      }
      else {
        current_value = reset_value;
      }
      //print the new value
      itoa(current_value, buffer ,10);
      puttinychar(0 , 1, '>');
      puttinychar(4 , 1, buffer[0]);
      puttinychar(8 , 1, buffer[1]);
      puttinychar(12, 1, buffer[2]);
      puttinychar(16, 1, buffer[3]);
      delay(150);
    }

    while (buttonC.isPressed()) {

      if(current_value > reset_value) {
        current_value--;
      }
      else {
        current_value = rollover_limit;
      }
      //print the new value
      itoa(current_value, buffer, 10);
      puttinychar(0 , 1, '>');
      puttinychar(4 , 1, buffer[0]);
      puttinychar(8 , 1, buffer[1]);
      puttinychar(12, 1, buffer[2]);
      puttinychar(16, 1, buffer[3]);
      delay(150);
    }
    
  }
  return current_value;

}


// function to get time from DS3231
void get_time() {

  //get time
  DateTime now = ds3231.now();
  //save time to array
  rtc[6] = now.year();
  rtc[5] = now.month();
  rtc[4] = now.day();
  rtc[3] = now.dayOfTheWeek(); //returns 0-6 where 0 = Sunday
  rtc[2] = now.hour();
  rtc[1] = now.minute();
  rtc[0] = now.second();

  //print the time to the serial port - useful for debuging RTC issues
  /*
  Serial.print(rtc[2]);
  Serial.print(":");
  Serial.print(rtc[1]);
  Serial.print(":");
  Serial.println(rtc[0]);
  */

}


//Routine to check light level and turn on/off matrix
void light() {

  //Get light reading
  uint16_t lx = lux.GetLightIntensity();

  //checks if display can be turned off if option to keep it on until a certain time is met
  bool dont_turn_off = false;
  if (display_mode > 2) {
    byte hr = rtc[2];
    switch(display_mode) {
      case 3:
      if (hr > 7 && hr < hour_off_1) {
        dont_turn_off = true;
      }
      else {
        dont_turn_off = false;
      }
      break;
      case 4:
      if (hr > 7 && hr < hour_off_2) {
        dont_turn_off = true;
      }
      else {
        dont_turn_off = false;
      }
      break;
      case 5:
      if (hr > 7 && hr < hour_off_3) {
        dont_turn_off = true;
      }
      else {
        dont_turn_off = false;
      }
      break;
    }
  }

  if (display_mode == 2) {
    shut = 1;
    set_devices(false, 0); //Call sleep routine to turn off matrix, applies only when 4th button is used to turn it always off
  }
  else if (lx == 0 && !shut && !dont_turn_off && (display_mode == 0 || display_mode > 2)) {
    shut = 1;
    set_devices(false, 0); //Call sleep routine to turn off matrix, applies when light is low enough and 4th button option is normal
  }
  else if (lx > 0 && shut && display_mode != 2) {
    shut = 0;
    set_devices(false, 0); //Call sleep routine to turn on matrix, applies when light is high enough and 4th button is not set to always off
  }

  //this runs if auto_intensity is true and display is not off, it defines the intensity based on the light sensor and calls set_devices to set intensity.
  if (auto_intensity && !shut) {
    switch(lx) {
      case 0:
      auto_intensity_value = 0;
      break;
      case 2:
      auto_intensity_value = 1;
      break;
      case 3 ... 4:
      auto_intensity_value = 2;
      break;
      case 5 ... 6:
      auto_intensity_value = 3;
      break;
      case 7 ... 10:
      auto_intensity_value = 4;
      break;
      case 11 ... 20:
      auto_intensity_value = 5;
      break;
      case 21 ... 40:
      auto_intensity_value = 6;
      break;
      case 41 ... 60:
      auto_intensity_value = 7;
      break;
      case 61 ... 100:
      auto_intensity_value = 8;
      break;
      case 101 ... 150:
      auto_intensity_value = 9;
      break;
      case 151 ... 200:
      auto_intensity_value = 10;
      break;
      case 201 ... 250:
      auto_intensity_value = 11;
      break;
      case 251 ... 300:
      auto_intensity_value = 12;
      break;
      case 301 ... 350:
      auto_intensity_value = 13;
      break;
      case 351 ... 400:
      auto_intensity_value = 14;
      break;
      case 401 ... 65535:
      auto_intensity_value = 15;
      break;
    }
    set_devices(true, auto_intensity_value);
    //this is useful for help setting the values above
    //Serial.println(lx);
  }

}


//Routine called by light() to turn on/off matrix and by auto light intensity to adjust device intensity. bool m = true (light intensity), false (matrix on/off), byte i = intensity
void set_devices(bool m, byte i) {

  int devices = lc.getDeviceCount();
  for (int address = 0; address < devices; address++) {
    if (!m) {
      //turns on/off matrix
      if (display_mode == 2) {
        delay(2000);
      }
      lc.shutdown(address, shut);
    }
    else {
      //sets matrix intensity
      lc.setIntensity(address, i);
    }
  }

}


//Routine to set display on/off options (0 = normal, 1 = always on, 2 = always off, 3 - 5 = after specific time)
void set_display_options() {

  cls();

  char options[6][9] = {">Normal", ">On", ">Off", "> 9.00pm", ">10.00pm", ">11.00pm"};

  byte i = 0;
  while(options[display_mode][i])
  {
    puttinychar(i * 4, 1, options[display_mode][i]); 
    i++;
  }

  //wait for button input
  while (!buttonA.uniquePress()) {

    while (buttonB.isPressed()) {
      display_mode++;
      if (display_mode == 6) {
        display_mode = 0;
      }

      //print the new value
      cls();

      byte i = 0;
      while(options[display_mode][i])
      {
        puttinychar(i * 4, 1, options[display_mode][i]); 
        i++;
      }
      delay(150);
    }
    while (buttonC.isPressed()) {
      //display current lux value
      cls();
      byte i = 0;
      char msg[4] = "LX:";
      i = 0;
      while(msg[i]) {
        puttinychar(i * 4, 1, msg[i]);
        i++;
      }
      char buffer[6];
      dtostrf(lux.GetLightIntensity(), 5, 0, buffer);
      i = 0;
      while(buffer[i]) {
        puttinychar(i * 4 + 12, 1, buffer[i]);
        i++;
      }
      delay(150);
    }

  }

  //save the values to EEPROM
  eeprom_save(202, display_mode, 0, 0);
  //Serial.println("display_mode");

}


//function for setting NTP time via ESP01
void ntp() {

  char buffer[80];
  char unixString[10];
  bool timeSync = 0;
  byte wait = 0;

  //trigger ESP01 via software serial to receive NTP time
  esp.print("NTP");
  esp.print(ntp_max_retry);
  if (ssid_len < 10) {
    esp.print("0");
  }
  esp.print(ssid_len);
  esp.print(ssid);
  if (pass_len < 10) {
    esp.print("0");
  }
  esp.print(pass_len);
  esp.println(pass);
  
  //send empty line to prevent premature wakeup
  esp.println(" ");

  Serial.println("Arduino : Sent NTP request to ESP01");

  cls();
  char msg[9] = ">GET NTP";
  int i = 0;
  while(msg[i])
  {
    puttinychar(i * 4, 1, msg[i]);
    i++;
  }

  //holds count to quit routine if no data received from ESP01
  DateTime now = ds3231.now();
  unsigned long ntp_count = now.unixtime() + (ntp_max_retry * ntp_timeout);
  unsigned int ntp_counter = 1;
  
  while (!timeSync) {
    if (readline(esp.read(), buffer, 80) > 0) {
      Serial.println(buffer); //used for debugging output from ESP01
      if (buffer[0] == 'U' && buffer[1] == 'N' && buffer[2] == 'I' && buffer[3] == 'X' && wait == 0) {
        // if data sent is the UNIX token, take it
        int i = 0;
        while (i < 10) {
          unixString[i] = buffer[i + 4];
          i++;
        }
        unixString[10] = '\0';

        //convert UNIX time to long integer and save to DS3231, add ntp_adjust secs and utc_offset
        ds3231.adjust(DateTime(atol(unixString) + ntp_adjust + (3600 * utc_offset)));

        //calculate dst(), tell it the request came from ntp()
        if (dst_mode) {
          dst(1);
        }
      
        //increase counter to allow one more line to be read, this is just to read a blank line into serial
        wait++;

        //set time using ESP01 NTP - success
        fade_down();
        char msg[8] = ">NTP OK";
        i = 0;
        while(msg[i]) {
          puttinychar(i * 4, 1, msg[i]);
          i++;
        }
        delay(1000);
        fade_down();
      }
      else if (wait == 1) {
        wait++;
      }
      else if (wait == 2) {
        //complete NTP transaction
        timeSync = 1;
      }
      // NTP sending failed
      else if (buffer[4] == 'F' && buffer[5] == 'a' && buffer[6] == 'i' && buffer[7] == 'l') {
        char msg[9] = ">NTP ERR";
        i = 0;
        while(msg[i]) {
          puttinychar(i * 4, 1, msg[i]);
          i++;
        }
        delay(1000);
        fade_down();
        wait++;
      }
      // WiFi connection failed
      else if (buffer[5] == 'F' && buffer[6] == 'a' && buffer[7] == 'i' && buffer[8] == 'l') {
        char msg[9] = ">WIFI ER";
        i = 0;
        while(msg[i]) {
          puttinychar(i * 4, 1, msg[i]);
          i++;
        }
        delay(1000);
        fade_down();
        wait++;
      }
    }

    //quit ntp routine if nothing comes from the ESP, the calculation below ensures it does not quit before ESP01 processing
    ntp_counter++;
    delay(1);

    if (ntp_counter > 5000) {
      DateTime now = ds3231.now();
      if (now.unixtime() > ntp_count) {
        char msg[9] = ">NO WIFI";
        i = 0;
        while(msg[i]) {
          puttinychar(i * 4, 1, msg[i]);
          i++;
        }
        delay(1000);
        fade_down();
        timeSync = 1;
      }
      ntp_counter = 1;
    }
  }

  clock_mode = old_mode;

}


//used to readline from serial output
int readline(int readch, char *buffer, int len) {

  static int pos = 0;
  int rpos;

  if (readch > 0) {
    switch (readch) {
      case '\r': // Ignore CR
      break;
      case '\n': // Return on new-line
      rpos = pos;
      pos = 0;  // Reset position index ready for next time
      return rpos;
      default:
      if (pos < len-1) {
        buffer[pos++] = readch;
        buffer[pos] = 0;
      }
    }
  }
  return 0;

}


//calculates DST, takes NTP output (bool ntp - if applicable) into consideration
void dst(bool ntp) {

  get_time();
  byte day = rtc[4];
  byte month = rtc[5];
  byte hour = rtc[2];
  byte dow = rtc[3];

  //temporarily store DST changes
  bool dst_plus = 0;
  bool dst_minus = 0;

  //winter calculation
  if ((month < 3 || month > 10) || (month == 3 && day < 25 && DST == 1) || (month == 10 && day >= 25 && hour == 2 && dow == 0 && DST == 1)) {
    DST = 0; //winter is coming
    dst_minus = 1;
    //save to EEPROM
    eeprom_save(212, 0, DST, 0);
    //Serial.println("winter");
  }
  //summer calculation
  else if ((month > 3 && month < 10) || (month == 10 && day < 25 && DST == 0) || (month == 3 && day >= 25 && hour == 1 && dow == 0 && DST == 0)) {
    DST = 1; //hosepipe ban is coming
    dst_plus = 1;
    //save to EEPROM
    eeprom_save(212, 0, DST, 0);
    //Serial.println("summer");
  }
  //+1 hour if run from NTP routine and summertime, or one time calculation is active and not run from NTP
  if ((ntp && DST) || (!ntp && dst_plus)) {
    DateTime now = ds3231.now();
    ds3231.adjust(DateTime(now.unixtime() + (3600)));
    //Serial.println("summer+1");
  }
  //-1 hour if not run from NTP routine and wintertime, based on one time calculation
  if (!ntp && dst_minus) {
    DateTime now = ds3231.now();
    ds3231.adjust(DateTime(now.unixtime() - (3600)));
    //Serial.println("winter-1");
  }

}
