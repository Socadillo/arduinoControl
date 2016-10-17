#include <EEPROM.h>             // Internal EEPROM Memory library
#include <Wire.h>               // I2C Controller library
#include <LCD.h>                // LCD Library
#include <LiquidCrystal_I2C.h>  // 20x4 LCD library
#include "Adafruit_MAX31855.h"      //////////////////////////////////////////////////////////////////not sure yet

    //LCD Screen set up
    #define I2C_ADDR 0x27 // Address of LCD screen
    #define Rs_pin 0
    #define Rw_pin 1
    #define En_pin 2
    #define BACKLIGHT_PIN 3
    #define D4_pin 4
    #define D5_pin 5
    #define D6_pin 6
    #define D7_pin 7

    // which analog pin the thermistors are connected to
    #define THERMISTORPIN_1 A0   // This pin is used for permanent temp probe 1
    #define THERMISTORPIN_2 A1   // This pin is used for permanent temp probe 2
    #define THERMISTORPIN_3 A2   // This pin is used for the removable temp probe 3  
    #define THERMISTORPIN_4 A3   // This pin is used for the removable temp probe 4
    // Resistance at 25 degrees C
    #define THERMISTORNOMINAL 10000      
    // Temp. for nominal resistance (almost always 25 C)
    #define TEMPERATURENOMINAL 25   
    // How many samples to take and average, more takes longer, but is more 'smooth' 
    #define NUMSAMPLES 5
    // The beta coefficient of the thermistor (usually 3000-4000)
    #define BCOEFFICIENT 3950
    // the value of the 'other' resistor
    #define SERIESRESISTOR 10000    


LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

//Thermocouple digital input pins
int thermoCLK = 3;
int thermoCS = 4;
int thermoDO = 5;
Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);  // Initialize the Thermocouple

//Temperature Probe Variables
float smokerTemp; // Make a variable called smokerTemp to monitor the temp of the smoker 
int samples_1[NUMSAMPLES];
int samples_2[NUMSAMPLES];
int samples_3[NUMSAMPLES];
int samples_4[NUMSAMPLES];

//Heat Element Variables
int heat_element = 13; // The smoke element relay signal is connected to pin 13
float setTemp;         // Make a variable called setTemp to be the target temp
float setTempF;        // Make a variable called setTempF to be the target temp in fahrenheit
  
//Smoke Element Variables
const int smoke_element = 12; // The smoke element relay signal is connected to pin 12
float smokeLevel;              // Make a variable called smokeLevel to be the determine how often the smoke element fires
int smokeState = 0;            // Is the smoker on or off?
static unsigned long smokeOff = 0;  // Timer variable
static unsigned long smokeOn = 0;   // Timer variable

//Button Interface Variables
int upButton = 11;     // switch up is at pin 7
int downButton = 10;   // switch down is at pin 6  
int selectButton = 9; // select button is at pin 5
int buttonMode = 0;     // determins which value will be changed
unsigned long selectOff = 0;   // This will store last time a button was pressed
const long selectTime = 10000; // This is the time delay to turn off editing
unsigned long tempUpdate = 0;  // This will store last time the temperature display was updated
const long tempDelay = 1500;   // This is the time delay for updating the temperature display
unsigned long blinkOff = 0;   // This will store last time text blinked
const long blinkTime = 250;   // Interval at which to blink (milliseconds)
int blinkState = 0;           // Determines if blinking is on/off
int tempDisplay = 0;   // 0 = Celsius, 1 = F       // Set up conversion from C to F


void setup() {
  pinMode (heat_element, 1);  // Make heat_element an OUTPUT for relay signal
  pinMode (smoke_element, 1); // Make smoke_element an OPUTPUT for relay signal


  ///////////////////////////////////////////DONT FRY THE BOARD////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  analogReference(EXTERNAL);  // connect AREF to 3.3V and use that as VCC, less noisy but but careful to not damage the board
    
  Serial.begin (9600);  // set the serial monitor tx and rx speed
  lcd.begin (20,4);     // <<-- our LCD is a 20x4, change for your LCD if needed
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);    // LCD Backlight ON
  lcd.setBacklight(HIGH);
  lcd.setCursor(3, 1);
  lcd.print("SMOKEY SQUIRREL");
  lcd.setCursor(1, 3);
  lcd.print("Smoker  Controller");
  delay(1500);
  lcd.clear();  
  
  EEPROM.read (1); // Read setTemp from Memory address 1
  EEPROM.read (2); // Read setTempF from Memory address 1
  EEPROM.read (3); // Read smokeLevel setting from Memory address 2
  buttonMode = 0;  // Starts the program with no user editing of temp and smoke levels
  tempDisplay = 0; // Starts the program in Celsius
}



void loop() {


//Begin Temperature Probes

  //Temp Probe Variables
      uint8_t i;
      float average_1 = 0;
      float average_2 = 0;
      float average_3 = 0;
      float average_4 = 0;

    // take N samples in a row, with a slight delay
      for (i=0; i< NUMSAMPLES; i++)
       {
         analogRead(THERMISTORPIN_1);
         delay(10);
         samples_1[i] = analogRead(THERMISTORPIN_1);  //Read samples from the analog pins connected to thermistor probe sensors
         delay(10);
         analogRead(THERMISTORPIN_2);
         delay(10);
         samples_2[i] = analogRead(THERMISTORPIN_2);  //Read samples from the analog pins connected to thermistor probe sensors
         delay(10);
         analogRead(THERMISTORPIN_3);
         delay(10);
         samples_3[i] = analogRead(THERMISTORPIN_3);  //Read samples from the analog pins connected to thermistor probe sensors
         delay(10);
         analogRead(THERMISTORPIN_4);
         delay(10);
         samples_4[i] = analogRead(THERMISTORPIN_4);  //Read samples from the analog pins connected to thermistor probe sensors
         delay(10);
          }
      
        for (i=0; i< NUMSAMPLES; i++)
         {
          average_1 += samples_1[i];
          average_2 += samples_2[i];
          average_3 += samples_3[i];
          average_4 += samples_4[i];
           }
          
        average_1 /= NUMSAMPLES;
        average_2 /= NUMSAMPLES;
        average_3 /= NUMSAMPLES;
        average_4 /= NUMSAMPLES;
 
    // Convert the value to resistance
      average_1 = 1023 / average_1 - 1;
      average_2 = 1023 / average_2 - 1;
      average_3 = 1023 / average_3 - 1;
      average_4 = 1023 / average_4 - 1;
      
      average_1 = SERIESRESISTOR / average_1;
      average_2 = SERIESRESISTOR / average_2;
      average_3 = SERIESRESISTOR / average_3;
      average_4 = SERIESRESISTOR / average_4;

   // Convert all that info to celsius
     float probe_1;
      probe_1 = average_1 / THERMISTORNOMINAL;     // (R/Ro)
      probe_1 = log(probe_1);                  // ln(R/Ro)
      probe_1 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
      probe_1 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
      probe_1 = 1.0 / probe_1;                 // Invert
      probe_1 -= 273.15;                         // convert to C
     float probe_1F = ((probe_1 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit

     float probe_2;
      probe_2 = average_2 / THERMISTORNOMINAL;     // (R/Ro)
      probe_2 = log(probe_2);                  // ln(R/Ro)
      probe_2 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
      probe_2 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
      probe_2 = 1.0 / probe_2;                 // Invert
      probe_2 -= 273.15;                         // convert to C
     float probe_2F = ((probe_2 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit

     float probe_3;
      probe_3 = average_3 / THERMISTORNOMINAL;     // (R/Ro)
      probe_3 = log(probe_3);                  // ln(R/Ro)
      probe_3 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
      probe_3 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
      probe_3 = 1.0 / probe_3;                 // Invert
      probe_3 -= 273.15;                         // convert to C
     float probe_3F = ((probe_3 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit
     
     float probe_4;  
      probe_4 = average_4 / THERMISTORNOMINAL;     // (R/Ro)
      probe_4 = log(probe_4);                  // ln(R/Ro)
      probe_4 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
      probe_4 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
      probe_4 = 1.0 / probe_4;                 // Invert
      probe_4 -= 273.15;  
      float probe_4F = ((probe_4 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit

     unsigned long tempTime = millis();  // Keeps track of the current time for temperature display purposes

//End Temperature Probes



//Begin Button Interface

  //Button Variables
      setTemp = EEPROM.read(1);                     // Read setTemp from the EEPROM memory address 1
      setTemp = constrain(setTemp, 0, 150);         // Constrain setTemp to 0-150
      setTempF= EEPROM.read(2);                     // Read setTempF from the EEPROM memory address 2
      setTempF = constrain(setTempF, 0, 300);       // Constrain setTempF to 0-300
      smokeLevel = EEPROM.read(3);                  // Read the smokeLevel from the EEPROM memory address 3
      smokeLevel = constrain(smokeLevel, 0, 100);   // Constrain smokeLevel to 0-100

  //Select Button Setup
      if (digitalRead(selectButton)== 1 )   // If the select button switch (pin 5) recieves a signal
      {
        if (buttonMode == 0)  // If buttonMode is 0 set to 1
         {
          (buttonMode = 1);   // Allows the smoke level to be changed
          (selectOff = millis());    // Sets the selectOff time
           }
        else if (buttonMode == 1)   // If buttonMode is 1, change to 2
         {
          (buttonMode = 2);   // Allows the set temperature to be changed by 1
          (selectOff = millis());    // Sets the selectOff time
           }             
        else if (buttonMode == 2)   // If buttonMode is 2, change to 3
         {
          (buttonMode = 3);   // Allows the set temperature to be changed by 10
          (selectOff = millis());    // Sets the selectOff time
           }
        else if (buttonMode == 3)   // If buttonMode is 3, change to 4
         {
          (buttonMode = 4);   // Toggles between Celsius and Fahrenheit
          (selectOff = millis());    // Sets the selectOff time
           }
        else if (buttonMode == 4)   // If buttonMode is 4, change to 0
         {
          (buttonMode = 0);   // Disables the up and down buttons
          (selectOff = millis());    // Sets the selectOff time
           }
          }   //End of Select Button 

  //Smoke Level Buttons +5 change
      if (buttonMode == 1)   // If buttonMode is 1 then the smoke level can be changed
       {
        if (digitalRead(upButton) == 1 )  // If the upButton (pin7) recieves a signal
         {
          (smokeLevel = smokeLevel +5);  // Add five to smokeLevel, smokeLevel is percentage of 100 second cycles of time
          (selectOff = millis());    // Sets the selectOff time
            }
        if (digitalRead (downButton) == 1)  // If the downButton (pin6) recieves a signal
         {
          (smokeLevel = smokeLevel -5);  // Subtract five from smokeLevel
          (selectOff = millis());    // Sets the selectOff time
           }
        smokeLevel = constrain(smokeLevel, 0, 100);  // Constrain the smokeLevel to 0-100   
         }    //End of Smoke Level Buttons

  //Temperature Level Buttons +1
      if (buttonMode == 2)    // If buttonMode is 2 then the temperature can be changed by 1 degree
       {
        if (tempDisplay == 0) // If Celsius is selected
         {
          if (digitalRead(upButton) == 1 )  // If the upButton (pin7) recieves a signal
           {
            (setTemp ++);   // Add one to setTemp, setTemp is the desired temp of the smoker
            (selectOff = millis());    // Sets the selectOff time
             }   
          if (digitalRead (downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTemp --);   // Subtract one from setTemp
            (selectOff = millis());    // Sets the selectOff time
             }
          setTemp = constrain(setTemp, 0, 150);      // Constrain setTemp to 0-150
          setTempF = ((setTemp * 9.0)/ 5.0 + 32.0);  // Match Fahrenheit temp to Celsius temp
          setTempF = constrain(setTempF, 0, 300);    // Constrain setTempF to 0-300
           } // End Celsius
  
        else if (tempDisplay == 1) // If Fahrenheit is selected
         {
          if (digitalRead(upButton) == 1 )  // If the upButton (pin7) recieves a signal
           {
            (setTempF ++);   // Add one to setTempF, setTempF is the desired temp of the smoker
            (selectOff = millis());    // Sets the selectOff time
             }   
          if (digitalRead (downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTempF --);   // Subtract one from setTempF
            (selectOff = millis());    // Sets the selectOff time
             }
          setTempF = constrain(setTempF, 0, 300);      // Constrain setTemp to 0-300
          setTemp = ((setTempF - 32.0) * 5.0 / 9.0);   // Match Celisus temp to Fahrenheit temp
          setTemp = constrain(setTemp, 0, 150);        // Constrain setTemp to 0-150
           } // End Fahrenheit
       } // End of Temperaure +1 Buttons

  //Temperature Level Buttons +10
      if (buttonMode == 3)    // If buttonMode is 3 then the temperature can be changed by 10 degrees
       {
        if (tempDisplay == 0) // If Celsius is selected
         {
          if (digitalRead(upButton) == 1 )  // If the upButton (pin7) recieves a signal
           {
            (setTemp =setTemp + 10);   // Add ten to setTemp, setTemp is the desired temp of the smoker
            (selectOff = millis());    // Sets the selectOff time
             }   
          if (digitalRead (downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTemp = setTemp - 10);   // Subtract ten from setTemp
            (selectOff = millis());    // Sets the selectOff time
             }
          setTemp = constrain(setTemp, 0, 150);      // Constrain setTemp to 0-150
          setTempF = ((setTemp * 9.0)/ 5.0 + 32.0);  // Match Fahrenheit temp to Celsius temp
          setTempF = constrain(setTempF, 0, 300);    // Constrain setTempF to 0-300
           } // End Celsius
  
        else if (tempDisplay == 1) // If Fahrenheit is selected
         {
          if (digitalRead(upButton) == 1 )  // If the upButton (pin7) recieves a signal
           {
            (setTempF = setTempF + 10);   // Add ten to setTempF, setTempF is the desired temp of the smoker
            (selectOff = millis());    // Sets the selectOff time
             }   
          if (digitalRead (downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTempF = setTempF - 10);   // Subtract ten from setTempF
            (selectOff = millis());    // Sets the selectOff time
             }
          setTempF = constrain(setTempF, 0, 300);     // Constrain setTemp to 0-300
          setTemp = ((setTempF - 32.0) * 5.0 / 9.0);  // Match Celisus temp to Fahrenheit temp
          setTemp = constrain(setTemp, 0, 150);       // Constrain setTemp to 0-150
           } // End Fahrenheit
       } // End of Temperaure +10 Buttons

  //Temperature Celsius to Fahrenheit Buttons
      if (buttonMode == 4)    // If buttonMode is 4 then the temperature can be toggled between C and F
       {
        if (digitalRead(upButton) == 1 )  // If the upButton (pin7) recieves a signal
         {
          if (tempDisplay == 0)   // If Celsius is selected
           {
            (tempDisplay = 1);
            (selectOff = millis());    // Sets the selectOff time
             }
          else if (tempDisplay == 1)   // If Fahrenheit is selected
           {
            (tempDisplay = 0);
            (selectOff = millis());    // Sets the selectOff time
             }
            }

        if (digitalRead (downButton) == 1)  // If the downButton (pin6) recieves a signal
         {
          if (tempDisplay == 0)   // If Celsius is selected
           {
            (tempDisplay = 1);
            (selectOff = millis());    // Sets the selectOff time
             }
          else if (tempDisplay == 1)   // If Fahrenheit is selected
           {
            (tempDisplay = 0);
            (selectOff = millis());    // Sets the selectOff time
             }
            }
          }    //End of Temperature Celsius to Fahrenheit Buttons
         
  //Blinking Text While Editing
      unsigned long blinkOn = millis();

      if (blinkOn - blinkOff >= blinkTime)    //  Check how long the text has been blinking
       {
        (blinkOff = blinkOn);   // Save the last time the text blinked
        if (blinkState == 0)    // If the text blink is off
         {
          (blinkState = 1);     // Turn the text blink on
           } 
        else                    // If the text blink is on
        {
         (blinkState = 0);      // Turn text blink off
          }
         }    //End of Blinking Text While Editing

  //Turn off editing after 10 seconds
      unsigned long selectOn = millis();  // Set the current time

      if (selectOn - selectOff >= selectTime)    //  Check how long the text has been blinking
       {
        (buttonMode = 0); // Turn off editing if longer than delay
         }    //End of turn off editing after 10 seconds

//End Button Interface


//Thermostat
      double c = thermocouple.readCelsius();
      smokerTemp = probe_1;   // This smoker temp will be its own sensor in the future, but use this probe temp in the meantime
    //smokerTemp = c; //  This sets the smokerTemp to the thermocouple temperature
      float smokerTempF = ((smokerTemp * 9.0)/ 5.0 + 32.0);  // Convert to Fahrenheit
      if (smokerTemp < setTemp)    // If the smoker temp is less than setTemp
       {
        digitalWrite (heat_element, 1);   // Turn the heat element ON
         }
      else    // If the smoker temp is NOT less than setTemp
       {
        digitalWrite (heat_element,0);    // Turn the heat element OFF
         }
         
//End Thermostat



//Begin Smoke Element Controlled Timed Output

      if(smokeState ==0)    // If the smoke element is OFF we will check how long it has been OFF
       {
       if(millis()/1000-smokeOff > (100-smokeLevel))    // If the time the element has been OFF is greater than (100 - smokeLevel) seconds
        {
         digitalWrite(smoke_element,1);   //turn ON the smoke element
         smokeState = 1;                  //set the variable that the element is ON
         smokeOff = 0;                    //reset the smokeOff time
         smokeOn = millis()/1000;         //set the smokeOn time to current time
          }
        }
    
      if(smokeState == 1)   // If the smoke element is ON we will check how long it has been ON
      {
        if(millis()/1000-smokeOn > smokeLevel)    // If the time the element has been ON is greater than (smokeLevel) seconds
        {
         digitalWrite(smoke_element,0);    //turn OFF the smoke element
         smokeState = 0;                   //set the variable that the element is OFF
         smokeOn = 0;                      //reset the smokeON time
         smokeOff = millis()/1000;         //set the smokeOff time to current time
          }
        }

//End Smoke Element Controlled Timed Output



//Begin LCD Printing

  //First Row
      lcd.setCursor(7,0);
      lcd.print("   "); // Controls overlap
      
  //Set Temp
    // +1 Temp Editor
      if (buttonMode == 2)    // Check if the +1 setTemp editor is activated
       {
        if (tempDisplay == 0) // If Celsius is selected 
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4,0);
            lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
            lcd.print("\337C");   // Celsius symbol          
             }
          if (blinkState == 0)   // Check if the text blink is on/off
           {
            if (setTemp >= 100)   // Check if setTemp is 3 characters
             {
              lcd.setCursor(6,0);
              lcd.print(" ");  // Insert blank character one's place
              lcd.setCursor(7,0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 100)   //  If setTemp is below 100
             {
              lcd.setCursor(5,0);
              lcd.print(" ");  // Insert blank character one's place
              lcd.setCursor(6,0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 10)   //  If the setTemp is below ten
             {
              lcd.setCursor(4,0);
              lcd.print(" ");  // Insert blank character in one's place
              lcd.print("\337C");   // Celsius symbol
              lcd.print("   ");  // Controls overlap
               }
              }
            } //  End Celsius

       else if (tempDisplay == 1) // If Fahrenheit is selected
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4,0);
            lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
            lcd.print("\337F");   // Fahrenheit symbol          
             }
          if (blinkState == 0)   // Check if the text blink is on/off
           {
            if (setTempF >= 100)   // Check if setTemp is 3 characters
             {
              lcd.setCursor(6,0);
              lcd.print(" ");  // Insert blank character one's place
              lcd.setCursor(7,0);
              lcd.print("\337F");   // Fahrenheit symbol  
               }
            if (setTempF < 100)   //  If setTemp is below 100
             {
              lcd.setCursor(5,0);
              lcd.print(" ");  // Insert blank character one's place
              lcd.setCursor(6,0);
              lcd.print("\337F");   // Fahrenheit symbol  
               }
            if (setTempF < 10)   //  If the setTemp is below ten
             {
              lcd.setCursor(4,0);
              lcd.print(" ");  // Insert blank character in one's place
              lcd.print("\337F");   // Fahrenheit symbol  
              lcd.print("   ");  // Controls overlap
               }
              }
            } // End Fahrenheit
          } // End ButtonMode 2

    // +10 Temp Editor
      else if (buttonMode == 3)    // Check if the +10 setTemp editor is activated
       {
        if (tempDisplay == 0) // If Celsius is selected 
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4,0);
            lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
            lcd.print("\337C");   // Celsius symbol          
             }
          if (blinkState == 0)   // Check if the text blink is on/off
           {
            if (setTemp >= 100)   // Check if setTemp is 3 characters
             {
              lcd.setCursor(5,0);
              lcd.print(" ");  // Insert blank character ten's place
              lcd.setCursor(7,0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 100)   //  If setTemp is below 100
             {
              lcd.setCursor(4,0);
              lcd.print(" ");  // Insert blank character ten's place
              lcd.setCursor(6,0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 10)   //  If the setTemp is below ten
             {
              lcd.setCursor(4,0);
              lcd.print(" ");  // Insert blank character in one's place
              lcd.print("\337C");   // Celsius symbol
              lcd.print("   ");  // Controls overlap
               }
              }
            } //  End Celsius

       else if (tempDisplay == 1) // If Fahrenheit is selected
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4,0);
            lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
            lcd.print("\337F");   // Fahrenheit symbol          
             }
          if (blinkState == 0)   // Check if the text blink is on/off
           {
            if (setTempF >= 100)   // Check if setTemp is 3 characters
             {
              lcd.setCursor(5,0);
              lcd.print(" ");  // Insert blank character one's place
              lcd.setCursor(7,0);
              lcd.print("\337F");   // Fahrenheit symbol  
               }
            if (setTempF < 100)   //  If setTemp is below 100
             {
              lcd.setCursor(4,0);
              lcd.print(" ");  // Insert blank character one's place
              lcd.setCursor(6,0);
              lcd.print("\337F");   // Fahrenheit symbol  
               }
            if (setTempF < 10)   //  If the setTemp is below ten
             {
              lcd.setCursor(4,0);
              lcd.print(" ");  // Insert blank character in one's place
              lcd.print("\337F");   // Fahrenheit symbol  
              lcd.print("   ");  // Controls overlap
               }
              }
            } // End Fahrenheit
          } // End ButtonMode 3
   
    // Celsius to Fahrenheit Editor
      else if (buttonMode == 4)    // Check if the Celsius to Fahrenheit editor is activated
       {
        if (tempDisplay == 0) // If Celsius is selected
         {
          if (blinkState == 1)
          {
           lcd.setCursor(4,0);
           lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
           lcd.print("\337C");   // Celsius symbol
            }
          if (blinkState == 0)   // Check if the text blink is on/off
           {
            if (setTemp >= 100)   // Check if setTemp is 3 characters
             {
              lcd.setCursor(4,0);
              lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
              lcd.print("\337  ");   // Degree symbol
               }
            if (setTemp < 100)   //  If setTemp is below 100
             {
              lcd.setCursor(4,0);
              lcd.print(setTemp, 0);  // Insert blank character one's place
              lcd.print("\337   ");   // Degree symbol
               }
            if (setTemp < 10)   //  If the setTemp is below ten
             {
              lcd.setCursor(4,0);
              lcd.print(setTemp, 0);  // Insert blank character one's place
              lcd.print("\337    ");   // Degree symbol
               }
              }
            } // End Celsius

        else if (tempDisplay == 1) // If Fahrenheit is selected
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4,0);
            lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
            lcd.print("\337F");  // Fahrenheit symbol
             }
          if (blinkState == 0)   // Check if the text blink is on/off
           {
            if (setTemp >= 100)   // Check if setTemp is 3 characters
             {
              lcd.setCursor(4,0);
              lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
              lcd.print("\337  ");  // Degree symbol
               }   
            if (setTemp < 100)   //  If setTemp is below 100
             {
              lcd.setCursor(4,0);
              lcd.print(setTempF, 0);   // This displays the set temperature in Fahrenheit
              lcd.print("\337   ");  // Degree symbol
               }
              
            if (setTemp < 10)   //  If the setTemp is below ten
             {
              lcd.setCursor(4,0);
              lcd.print(setTempF, 0);   // This displays the set temperature in Fahrenheit
              lcd.print("\337    ");  // Degree symbol
                }
               }
              } // End Fahrenheit
           }/// End buttonMode 4

    // Editor Mode not activated
      else    // If none of the buttonMode editor settings are in effect
       {
        lcd.setCursor(4,0);
        if (tempDisplay == 0)   // If Celsius is selected
         {
          lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
          lcd.print("\337C");   // Celsius symbol
           }
        else if (tempDisplay == 1)   // If Fahrenheit is selected
         {
          lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
          lcd.print("\337F");  // Fahrenheit symbol
           }
         }
    // End of setTemp display
          
  //Temp Probe 1//////////////////////////////cant decide on the lable yet
    //lcd.setCursor(10, 0);
    //lcd.print("Tray1 ");
      lcd.setCursor(10, 0);
      lcd.print("P1 ");
      if (tempTime - tempUpdate > tempDelay)
       {      
        if (analogRead(THERMISTORPIN_1) < 1000)  // If the sensor is connected
         {
          lcd.setCursor(13, 0);
           if (tempDisplay == 0)   // If Celsius is selected
            {
             lcd.print(probe_1, 0);  // This displays the set temperature
             lcd.print("\337C    ");   // Celsius symbol
              }
           else if (tempDisplay == 1)   // If Fahrenheit is selected
            {
             lcd.print(probe_1F, 0);  // This displays the set temperature
             lcd.print("\337F    ");  // Fahrenheit symbol
              }
            } 
        if (analogRead(THERMISTORPIN_1) > 1000)  // If the sensor is disconnected
         {
          lcd.setCursor(13, 0);
          lcd.print("errr   ");  // This displays an error message
           }
        }//End Temp Probe 1 
      
  //Third Row Overlap      
      lcd.setCursor(0, 2);
      lcd.print(" ");  // Controls overlap
    
  //Second Row
      lcd.setCursor(2,1);
      lcd.print("Smoker");
  
  //Temp Probe 2///////////////////////////////////////////////////////
   //lcd.setCursor(10, 1);
   //lcd.print("Tray2 ");
      lcd.setCursor(11, 1);
      lcd.print("P2 ");
      if (tempTime - tempUpdate > tempDelay)
       {      
        if (analogRead(THERMISTORPIN_2) < 1000)  // If he sensor is connected
         {
          lcd.setCursor(14, 1);
           if (tempDisplay == 0)   // If Celsius is selected
            {
             lcd.print(probe_2, 0);  // This displays the set temperature
             lcd.print("\337C");   // Celsius symbol
              }
           else if (tempDisplay == 1)   // If Fahrenheit is selected
            {
             lcd.print(probe_2F, 0);  // This displays the set temperature
             lcd.print("\337F   ");  // Fahrenheit symbol
              }
            } 
        if (analogRead(THERMISTORPIN_2) > 1000)  // If the sensor is disconnected
         {
          lcd.setCursor(14, 1);
          lcd.print("errr  ");  // This displays an error message
           }
        }//End Temp Probe 2
      
  //Third Row Overlap
      lcd.setCursor(7,2);
      lcd.print(" ");  // Controls overlap
      
  //Smoker Temp
      if (tempTime - tempUpdate > tempDelay)
       {
        lcd.setCursor(3, 2);
        if (isnan(c)) // Check if smokerTemp isNaN
         {
          lcd.print("error"); // Diplays when there is a thermocouple error
           }
        else
         {
          if (tempDisplay == 0)   // If Celsius is selected
           {
            lcd.print(smokerTemp, 0);  // This displays the set temperature
            lcd.print("\337C");   // Celsius symbol
             }
           else if (tempDisplay == 1)   // If Fahrenheit is selected
            {
             lcd.print(smokerTempF, 0);  // This displays the set temperature
             lcd.print("\337F");  // Fahrenheit symbol
             }
            }
         } //End Smoker Temp

  //Temp Probe 3
    //lcd.setCursor(10, 2);
    //lcd.print("Tray3 ");
      lcd.setCursor(12, 2);
      lcd.print("P3 ");  
      if (tempTime - tempUpdate > tempDelay)
       { 
        if (analogRead(THERMISTORPIN_3) < 1000)  // If he sensor is connected
         {
          lcd.setCursor(15, 2);
          if (tempDisplay == 0)   // If Celsius is selected
           {
            lcd.print(probe_3, 0);  // This displays the set temperature
            lcd.print("\337C  ");   // Celsius symbol
             }
          else if (tempDisplay == 1)   // If Fahrenheit is selected
           {
            lcd.print(probe_3F, 0);  // This displays the set temperature
            lcd.print("\337F  ");  // Fahrenheit symbol
             }
           } 
        if (analogRead(THERMISTORPIN_3) > 1000)  // If the sensor is disconnected
         {
          lcd.setCursor(15, 2);
          lcd.print("errr ");  // This displays an error message
           }
       }//End Temp Probe 3


  //Second Row Overlap          
      lcd.setCursor(0, 1);
      lcd.print(" ");  // Controls overlap
   
  //Fourth Row
      lcd.setCursor(10, 3);
      lcd.print("   ");  // Controls overlap

  //Smoke Level
    //Smoke Level Editor
      if (buttonMode == 1)    // Check if the smoke level editor is activated
       {
        if (blinkState == 1)  // Print the smoke level
         {
          if (smokeLevel == 100)
          {
           lcd.setCursor(0, 3);
           lcd.print(smokeLevel, 0);  // This displays the smoke level
            }
          if (smokeLevel < 100)
           {
            lcd.setCursor(0, 3);
            lcd.print(" ");
            lcd.print(smokeLevel, 0);  // This displays the smoke level
             }
          if (smokeLevel < 10)
           {
            lcd.setCursor(0, 3);
            lcd.print("  ");
            lcd.print(smokeLevel, 0);  // This displays the smoke level
             }
           }
        
        if (blinkState == 0)   // Check to see if the text blink is on/off
         {
          if (smokeLevel == 100)
          {
           lcd.setCursor(0,3);
           lcd.print(smokeLevel, 0);  // This displays the smoke leve
           lcd.setCursor(2,3);
           lcd.print(" ");
            }
          if (smokeLevel < 100)
           {
            lcd.setCursor(0,3);
            lcd.print(" ");
            lcd.print(smokeLevel, 0);  // This displays the smoke level
            lcd.setCursor(2,3);
            lcd.print(" ");
             }
          if (smokeLevel < 10)
           {
            lcd.setCursor(0,3);
            lcd.print("  ");
            lcd.print(smokeLevel, 0);  // This displays the smoke level
            lcd.setCursor(2,3);
            lcd.print(" ");
             }
           }
       } // End buttonMode 1
 
      else    //If the smoke level editor is not activated print normal text
       {
        if (smokeLevel == 100)
         {
          lcd.setCursor(0, 3);
          lcd.print(smokeLevel, 0);  // This displays the smoke level
           }
        if (smokeLevel < 100)
         {
          lcd.setCursor(0, 3);
          lcd.print(" ");
          lcd.print(smokeLevel, 0);  // This displays the smoke level
           }
        if (smokeLevel < 10)
         {
          lcd.setCursor(0,3);
          lcd.print("  ");
          lcd.print(smokeLevel, 0);  // This displays the smoke level
           }
         }
    //End Smoke Level Display

  //Fourth Row Cont.
      lcd.setCursor(3,3);
      lcd.print("% Smoke");

  //Temp Probe 4 
     //lcd.setCursor(10, 3);
     //lcd.print("Tray4 ");
       lcd.setCursor(13, 3);
       lcd.print("P4 ");
       if (tempTime - tempUpdate > tempDelay)
        {
         if (analogRead(THERMISTORPIN_4) < 1000)  // If the sensor is connected
          {
           lcd.setCursor(16, 3);
           if (tempDisplay == 0)   // If Celsius is selected
            {
             lcd.print(probe_4, 0);  // This displays the set temperature
             lcd.print("\337C ");   // Celsius symbol
              }
           else if (tempDisplay == 1)   // If Fahrenheit is selected
            {
             lcd.print(probe_4F, 0);  // This displays the set temperature
             lcd.print("\337F ");  // Fahrenheit symbol
              }
            } 
         if (analogRead(THERMISTORPIN_4) > 1000)  // If the sensor is disconnected
          {
           lcd.setCursor(16, 3);
           lcd.print("errr");  // This displays an error message
            } 
        }//End Temp Probe 4
        
  //First Row Overlap
      lcd.setCursor(0, 0);
      lcd.print("Set ");   // This is the set temp label

//End LCD Printing



//Refresh temperature display update time
      if (tempTime - tempUpdate > tempDelay)
       {
        (tempUpdate = millis());
         }

//End temperature update time



//Begin Serial Printing

      Serial.print("Set Temp "); 
      Serial.println(setTemp, 1);    //Print the settemp in the serial montior
      Serial.print("Set Temp F "); 
      Serial.println(setTempF, 1);    //Print the settemp in the serial montior
      Serial.print("Smoker Temp "); 
      Serial.println(smokerTemp, 1);    //Print the Smoker Temp
      Serial.println();
     
      Serial.print("Temp 1: "); 
      Serial.println(probe_1, 1);    //Print temperature probe 1
      Serial.print("Temp 2: "); 
      Serial.println(probe_2, 1);    //Print temperature probe 2
      Serial.print("Temp 3: "); 
      Serial.println(probe_3, 1);    //Print temperature probe 3
      Serial.print("Temp 4: "); 
      Serial.println(probe_4, 1);    //Print temperature probe 4
      Serial.println();

      Serial.print("Smoke Level: ");
      Serial.print(smokeLevel, 0);  //Print smoke level
      Serial.println("%");
      
      Serial.print("Smoke Element: ");
       if (smokeState == 0)
        {
          Serial.println("OFF");
        }
        if (smokeState == 1)
        {
          Serial.println("ON");
        }
      Serial.print("Time: ");
      Serial.println(millis()/1000.0, 1);
      if (smokeState == 0)
        {
          Serial.print("Time Until Element ON: ");
          Serial.println((100.0-smokeLevel)-(millis()/1000.0-smokeOff), 1);
        }
      if (smokeState == 1)
        {
          Serial.print("Time Until Element OFF: ");
          Serial.println((smokeLevel)-(millis()/1000.0-smokeOn), 1);
        }

//deletelater
Serial.print("ButtonMode =  ");
Serial.print(buttonMode);
Serial.print("select off =  ");
Serial.println(selectOff/1000);
Serial.print((selectOn/1000 - selectOff/1000),1);
Serial.print("Int. Temp = ");
Serial.println(thermocouple.readInternal());
Serial.print("Thermocouple Temp = ");
Serial.println(c, 1);
Serial.print("Smoker T/C Temp = ");
Serial.println(smokerTemp, 1);
delay(10);
Serial.println(analogRead(THERMISTORPIN_1));
delay(10);
Serial.println(analogRead(THERMISTORPIN_2));
delay(10);
Serial.println(analogRead(THERMISTORPIN_3));
delay(10);
Serial.println(analogRead(THERMISTORPIN_4));
Serial.println(buttonMode);
Serial.println(tempDisplay);
Serial.println(tempUpdate/1000.0);
Serial.println(tempTime - tempUpdate);
      Serial.println();
      Serial.println();
      Serial.println();


//End Serial Printing


//Begin Flash the EEPROM so if power is lost the settings will be saved
      EEPROM.write (1,setTemp);   //Saves the celsius temperature value
      EEPROM.write (2,setTempF);   //Saves the fahrenheit temperature value
      EEPROM.write (3,smokeLevel);    //saves the smoke level setting
//End Flash the EEPROM

}     ///This is the end of the program

