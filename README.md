# miniclock
Video Here > https://youtu.be/lI52w7I6knk

Arduino based mini LED matrix clock, with BME280 Sensor and BH1750 Light Sensor.

Currently work in progress, almost complete.

Planned features/changes:

    # 4th Button - need to add option for keeping display on till a certain time.
    # Change word mode to have quarter past/to and mins to etc.
    # Light Sensor - option to adjust intensity automatically

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

27 Jun 2019 - Changes:

              # Added 4th button for setting display on/off options
