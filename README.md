# miniclock
Video Here > https://youtu.be/CpQsMjI3FL0

Arduino based mini LED matrix clock, with BME280 Sensor and BH1750 Light Sensor.

Currently work in progress, almost complete.

Planned features/changes:

    # 4th Button - need to add option for keeping display on till a certain time.
    # Change word mode to have quarter past/to and mins to etc.

27 Jun 2019 - Changes:

              # Added 4th button for setting display on/off options
              # Added font_style 7
              # Work in progress: light sensor automatically adjusts intensity
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
