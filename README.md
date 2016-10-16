# arduinoControl

Thermostat control for a heat element

4 temperature probes

smoke level is a percentage of a timed output to a 2nd heat elelment

All of the outputs display on an LCD, which is a bulk of the code

I believe I will use the serial.prints for interfacing with the android app in the future


Have a look at the code dealing with the thermistor conversions, they could be turned into a function or a library or so I believe




Physical description of the board

  4 Thermistors are connected to Analog pins A0-A3

    These are four food probes for monitoring cooking temps

  I2C Bus for controlling the LCD is connected to Analog pin A4 and A5

  AREF pin is connected to 3.3v on the main board

  D13 output - the signal to the heat element SSR relay

  D12 output - the signal to the smoke element SSR relay

  D11 input - Up button, physical push button

  D10 input - Down button, physical push button

  D9 input - Select button, physical push button

  D3 pin - is connected to the thermocouple board
  D4 pin - is connected to the thermcopuple board
  D5 pin - is connected to the thermocouple board

