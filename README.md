# miniclock
Video Here > https://youtu.be/krdAU_GUc3k

Arduino Create > https://create.arduino.cc/projecthub/Ratti3/led-matrix-ntp-clock-with-ds3231-bme280-bh1750-esp01-fdde2b

Arduino based mini LED matrix clock, with BME280 Sensor and BH1750 Light Sensor.
Optional code for ESP01 to set time via NTP.

Complete LEDClock32x8.ino v1.1 (22 Jul 2019), unless bugs found.
Complete LEDClock32x8_ESP01-NTP.ino v1.2 (22 Jul 2019), unless bugs found.
Complete ESP-01_NTP.ino (22 Jul 2019), unless bugs found.

# read this (22 Jul 2019)
This now saves settings to EEPROM, my code has been optimised to only update if necessary. USE AT YOUR OWN RISK. EEPROM can handle 100,000 writes before failure. During my testing, there were no writes in a 24hour period. The codes includes Serial.print, which shows when the EEPROM.update method is called, this only writes if the old value is different.

# read this too (22 Jul 2019)
I feel this is now complete (both versions), I have made it stable as possible and seems to work for me. I don't plan on adding any
more features.
I will make a case for it soon.

See KnownIssues.md for issues.

~~~~~~~~~~~~~~~~~~~~~~~~ CONNECTING IT ALL UP ~~~~~~~~~~~~~~~~~~~~~~~~
DS3231, BH1750 and BME280:
  SCL PINS = A5
  SDA PINS = A4
  VCC = 3.3v

LED Matrix:
  CLK = D11
  CS  = D10
  DIN = D12
  VCC = 5v

Switches:
  D2 - Menu
  D3 - Date / +
  D4 - Temp / -
  D5 - Display options
  
ESP01 (optional):
3.3 Power Regulator Required, and level shifter for the TX/RX pins
  D7 - ESP01 TX
  D6 - ESP01 RX
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

22 Jul 2019 - Changes:

       # Final version for both versions, cleaned up code
       # LEDClock32x8.ino has same menus at NTP version (minus the NTP function), added EEPROM save code.

16 Jul 2019 - Changes:

       # Moved more stuff out of Progmem due to stability issues in the menu
       # ESP01 now gets SSID name and password via Arduino code for convinience
       # Better error handling on ESP01
       # Manual ntp() call removed from main menu, for some reason the main menu caused weird issues/crashes

15 Jul 2019 - Changes:

       # Fixed issue, PROGMEM running slow due to too much data, moved some out of PROGMEM.
       # Fixed ESP01 timeout handling issue, moved more data out of PROGMEM
       # Increased Serial Software read buffer
       
14 Jul 2019 - Changes:

       # Added option to quit NTP routine when no data received
       # NTP and ESP01 error handling
       # NTP/DST run at 2am every day, can be disabled via menu
       # ESP Code: Tidied up code, made more efficient and better error handling

13 Jul 2019 - Changes:

       # Substantial amount of changes to LEDClock32x8_ESP01-NTP.ino:
       # Fixed various issues with the original code
       # Menus updated and added. DST and NTP finalised but not tested. UTC offset option in menu
       # Settings are now saved in EEPROM

11 Jul 2019 - Changes:

       # Files for ESP01: ESP-01_NTP.ino and Arduino file: LEDClock32x8_ESP01-NTP.ino
       # This adds support for an ESP01, time is obtained via NTP, work in progress

04 Jul 2019 - Changes:

       # ButtonD was missing in small_clock

02 Jul 2019 - Changes:

       # Fixed a few word clock issues
       # Lux (ButtonD) button also shows current lux value from sensor

28 Jun 2019 - Changes:

       # Added preview when changing font
       # Change word mode to have quarter past/to and mins to etc
       # Hours and Minutes moved to progmen
       # Fixed auto brightness resetting to gobal setting during fade_down
       # Word mode has more words and fixed some errors with reversed word times
       # Random mode is now every hour
       # Made word clock progmem efficient

27 Jun 2019 - Changes:

       # Added 4th button for setting display on/off options
       # Added font_style 7
       # Light sensor automatically adjusts intensity
       # Automatic LED Intensity adjustment, can be turned off via setup menu
       # Added Light Sensor menu (button 4) with option to keep display on until a specific time
       # Moved month and day names to progmem to save RAM

26 Jun 2019 - Changes:

       # BH1750 working, turns off LED Matrix when completely dark.
       # Corrected some fonts and rearranged.
       # Added degrees symbol, called via '~'.
       # Fixed various font alignment and reassignment.
       # Menu now uses small font, with > at start.
       # Font can be changed via setup menu.
       # Fixed Set Font menu anomalies.
       # Temp/Humi/Pres is now available from all modes via ButtonC.
       # Added 6 fonts in total, one of them (no 6) is a cheap one; font 2 is cropped on right side to create the illusion of a new font.
       # Added random font mode in setup menu
       # Fixed missing randomSeed in code
