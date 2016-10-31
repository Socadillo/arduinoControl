# Arduino PID Control

#Code Descriptions
PID control for Temperature
- Thermocouple temperature sensor
- PID algorithm maintains temperature
- Output is timed proportional signal to 25amp SSR relay
- Powers a 2000w heating element
Smoke level is how much smoke to be produced
- 0-100% power on or off for 100 second intervals
- Output is timed proportional signal to 25amp SSR relay
- Powers a 500w heating element
- Stainless steel chip pan holds combustable material
LCD Display
  - 20x4 character blue background with white text display
  - Connected to I2C bus to control display

I believe I will use the serial.prints for interfacing with the android app in the future


#Physical description of the Arduino
Thermocouple Temperature input
    Connected to Max31855 board
    Connected to Digital Pins 3-5
    0.01 uF capacitors connected accross TC inputs
    Powered by 3.3v converter
    Connected to 3.3v Logic level Converter

  Thermistors are connected to Analog pins A0-A3
    Four food probes for monitoring cooking temps
    Connected to 5v and 10k resistor connected to ground
    4.7 uF capacitor connected in parralel to 10k resistor
    
  I2C Bus for controlling the LCD
    Connected to Analog pin A4 and A5

Relay Outputs
    Timed proportional outputs connected to SSR relays
    D13 output - the signal to the heat element SSR relay
    D12 output - the signal to the smoke element SSR relay
  
  Button Inputs
    Three Buttons connected to 5v and 10k resistor
    D11 input - Up button, physical push button
    D10 input - Down button, physical push button
    D9 input - Select button, physical push button


