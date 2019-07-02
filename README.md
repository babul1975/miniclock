# miniclock
Video Here > https://youtu.be/CpQsMjI3FL0

Arduino based mini LED matrix clock, with BME280 Sensor and BH1750 Light Sensor.

Complete (28 Jun 2019), unless bugs found.

Planned features/changes:

    Thinking of using GPS\ESP01 for setting the time. Have ordered bit will test first.

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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
