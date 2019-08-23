# All of the below will be fixed...

    # FIXED - Word Mode - Some words don't fit. - FIXED
    # FIXED - Word Mode - Some words display in wrong order. - FIXED
    # Possible Issue - If barometric pressure < 1000, it may not display properly. Summer here so can't test :)
    # Crashes when swithing between word clock and other modes, crash occurs when NTP() function is called. Haven't been able to figure
      out why. One of the reasons why I had to move data back from Progmem and get rid of manual NTP update.
      As far as I'm concerned this is not a big problem, only happens when the above conditions are met. The device just reboots.
    # Possible Issue - DST calculations are based on well known DST calculation info that is availble on the internet.
      I have not tested it.
    # 23/08/2019 - When the display goes to sleep whilst running in Word mode, it may not wake up automatically. Currently testing a fix.
