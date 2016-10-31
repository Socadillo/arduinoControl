#include <EEPROM.h>             // Internal EEPROM Memory library
#include <Wire.h>               // I2C Controller library
#include <LCD.h>                // LCD Library
#include <LiquidCrystal_I2C.h>  // 20x4 LCD library
#include "Adafruit_MAX31855.h"  // Thermocouple Library
#include <PID_v1.h>             // PID Library

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
                                                            ////////////////////// // THIS section will be changed later//
//Which analog pin the thermistors are connected to                                                                     //
    #define THERMISTORPIN_1 A0   // This pin is used for permanent temp probe 1                                         //
    #define THERMISTORPIN_2 A1   // This pin is used for permanent temp probe 2                                         //
    #define THERMISTORPIN_3 A2   // This pin is used for the removable temp probe 3                                     //
    #define THERMISTORPIN_4 A3   // This pin is used for the removable temp probe 4                                     //
    // Resistance at 25 degrees C                                                                                       //
    #define THERMISTORNOMINAL 10000                                                                                     //
    // Temp. for nominal resistance (almost always 25 C)                                                                //
    #define TEMPERATURENOMINAL 25                                                                                       //
    // How many samples to take and average, more takes longer, but is more 'smooth'                                    //
    #define NUMSAMPLES 10                                                                                               //
    // The beta coefficient of the thermistor (usually 3000-4000)                                                       //
    #define BCOEFFICIENT 3950                                                                                           //
    // the value of the 'other' resistor                                                                                //
    #define SERIESRESISTOR 10000                            ///////////////////////// THIS section will be changed later//

//Relay Out Pins
    #define heat_element 13  // The smoke element relay signal is connected to pin 13
    #define smoke_element 12 // The smoke element relay signal is connected to pin 12

LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

//Thermocouple Digital Input Pins
    int thermoCLK = 3;
    int thermoCS = 4;
    int thermoDO = 5;
    Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);  // Initialize the Thermocouple

//PID Variables
    double Setpoint;  // The setpoint of the PID, should be equal to setTemp
    double Input;     // The input temp of the PID, should be equal to the smokerTemp
    double Output;    // The signal for the heat relay
  //PID Tuning Parameters
    double Kp = 50;   // Sets the initial Kp Value
    double Ki = 10;   // Sets the initial Ki Value
    double Kd = 10;   // Sets the initial Kd Value
  //Specify the links and initial tuning parameters
    PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
    int WindowSize = 10000;   // 10 second Time Proportional Output window
    unsigned long windowStartTime;
  //PID Variables from outside the Library
    float setTemp;       // Make a variable called setTemp to be the target temp
    float setTempF;      // Make a variable called setTempF to be the target temp in fahrenheit
    float smokerTemp;    // Make a variable called smokerTemp to monitor the temp of the smoker 
    float percentError;  // Variable to show the error between the setTemp and actual temp
    int heatState;       // Is the heater on or off? ( 0 = OFF, 1 = ON )
   
//EEPROM Addresses for Persistent Data
    const int setTempAddress = 4;       //Sets the setTemp memory address
    const int setTempFAddress = 8;      //Sets the setTempF memory address
    const int smokeLevelAddress = 12;   //Sets the smokeLevel memory address
    const int SpAddress = 16;   //Sets the Setpoint memory address
    const int KpAddress = 20;   //Sets the P parameter memory address
    const int KiAddress = 24;   //Sets the I parameter memory address
    const int KdAddress = 28;   //Sets the D parameter memory address
    //WindowSize variable to be added later?

//Temperature Probe Variables           //////////////////This Section will change later//
    int samples_1[NUMSAMPLES];                                                          //
    int samples_2[NUMSAMPLES];                                                          //
    int samples_3[NUMSAMPLES];                                                          //
    int samples_4[NUMSAMPLES];          //////////////////this Section will change later//

//Smoke Element Variables
    float smokeLevel;  // Make a variable called smokeLevel to control the rate of smoke produced
    int smokeState;    // Is the smoker on or off? ( 0 = OFF, 1 = ON )
    static unsigned long smokeOff = 0;  // Timer variable
    static unsigned long smokeOn = 0;   // Timer variable

//Button Interface Variables
  //Button Inputs
    int upButton = 11;     // Switch up is at pin 7
    int downButton = 10;   // Switch down is at pin 6  
    int selectButton = 9;  // Select button is at pin 5
  //Editing Mode Variables
    int buttonMode = 0;    // Determines which value will be changed
    int pidEdit = 0;       // Sets the mode for which PID setting can be edited 
  //LCD Display Variables
    int blinkState = 0;    // Determines if blinking is on/off
    int tempDisplay = 0;   // 0 = Celsius, 1 = F
    unsigned long selectOff = 0;    // This will store last time a button was pressed
    const long selectTime = 10000;  // This is the time delay to turn off editing
    unsigned long tempUpdate = 0;   // This will store last time the temperature display was updated
    const long tempDelay = 1500;    // This is the time delay for updating the temperature display
    unsigned long blinkOff = 0;     // This will store last time text blinked
    const long blinkTime = 250;     // Interval at which to blink (milliseconds)



//////////////////////////////////////////////////////BEGIN SETUP//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////BEGIN SETUP//////////////////////////////////////////////////////////////////////
void setup(){

//Initialize Serial COmmunication
    Serial.begin (9600);  // Set the serial monitor tx and rx speed
    
//Initialize LCD Display
    lcd.begin (20,4);     // <<-- our LCD is a 20x4, change for your LCD if needed
    lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);  // LCD Backlight ON
    lcd.setBacklight(HIGH);
  //Start Up Message
    lcd.setCursor(1, 1);
    lcd.print("SMOKEY");
    lcd.setCursor(2, 2);
    lcd.print("SQUIRREL");
    lcd.setCursor(11, 1);
    lcd.print("Smoker");
    lcd.setCursor(12, 2);
    lcd.print("Control");
    lcd.setCursor(13, 3);
    lcd.print("Unit");
    delay(1500);
    lcd.clear();
  
//Initialize the PID and related variables
    windowStartTime = millis();   // Sets the window start time
    LoadParameters();             // LoadParameters function retrieves settings from memory
    myPID.SetTunings(Kp,Ki,Kd);   // Sets the tuning inputs
    myPID.SetSampleTime(1000);    // Sets the frequency of the PID sensor read cycle
    myPID.SetOutputLimits(0, WindowSize);  // Sets the output limits of the PID 
    myPID.SetMode(AUTOMATIC);     // Sets the PID mode to automatic control

// Initialize and set up the User Interface button variables
    buttonMode = 0;   // Starts the program with no user editing of temp and smoke levels
    pidEdit = 0;      // Starts the program with no used editing of the PID
    tempDisplay = 0;  // Starts the program in Celsius

//Initialize Relay Control Pins
    pinMode (heat_element, 1);   // Make heat_element an OUTPUT for relay signal
    pinMode (smoke_element, 1);  // Make smoke_element an OUTPUT for relay signal
    digitalWrite (heat_element, 0);   // Make sure the heat_element is off to begin
    digitalWrite (smoke_element, 0);  // Make sure the smoke_element is off to begin
  
}/////////////////////////////////////////////////////END SETUP////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////BEGIN LOOP///////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////BEGIN LOOP///////////////////////////////////////////////////////////////////////
void loop() {

//////////// /////////////////////////////////////HARDWARE SENSORS/////////////////////////////////////////////////////////////////////
//BEGIN PID Control
    double c = thermocouple.readCelsius();  // Read the thermocouple temperature
    smokerTemp = c;     // This sets the smokerTemp to the thermocouple temperature
    Input = c;          // Sets the input of the PID to the thermocouple temperature
    Setpoint = setTemp; // Sets the PID temp control to the setTemp variable
    float smokerTempF = ((smokerTemp * 9.0)/ 5.0 + 32.0);  // Converts smokerTemp to Fahrenheit
    
    SaveParameters();           // SaveParameters function saves settings to memory
    myPID.SetTunings(Kp,Ki,Kd); // Sets the tuning inputs
    myPID.Compute();            // Run the PID function
    if (millis() - windowStartTime > WindowSize) // Time to shift the relay window
     {
      windowStartTime += WindowSize;
       }
    if (Output > millis() - windowStartTime) // Check if the PID needs to send a signal
     {
      digitalWrite (heat_element, 1); // Turn the heat element ON
      (heatState = 1);
      }
    else
     {
      digitalWrite(heat_element, 0); // Turn the heat element OFF
      (heatState = 0);
       }
//END PID Control



//BEGIN Smoke Element Controlled Timed Output
    if(smokeState ==0)    // If the smoke element is OFF we will check how long it has been OFF
     {
      if(millis()/1000-smokeOff > (100-smokeLevel)) //If element has been OFF greater than (100 - smokeLevel) seconds
       {
        digitalWrite(smoke_element,1);   // Turn ON the smoke element
        (smokeState = 1);                  // Set the variable that the element is ON
        (smokeOff = 0);                    // Reset the smokeOff time
        (smokeOn = millis()/1000);         // Set the smokeOn time to current time
         }
       }
    
    if(smokeState == 1)   // If the smoke element is ON we will check how long it has been ON
     {
      if(millis()/1000-smokeOn > smokeLevel)    // If element has been ON greater than (smokeLevel) seconds
       {
        digitalWrite(smoke_element, 0);  // Turn OFF the smoke element
        (smokeState = 0);                  // Set the variable that the element is OFF
        (smokeOn = 0);                     // Reset the smokeON time
        (smokeOff = millis()/1000);        // Set the smokeOff time to current time
         }
       }
//END Smoke Element Controlled Timed Output



//BEGIN Temperature Probes                                                              //This section will change later//
  //Temp Probe Variables                                                                                                //
      uint8_t i;                                                                                                        //
      unsigned long tempTime = millis();  // Keeps track of the current time for temperature display purposes           //
      float average_1 = 0;                                                                                              //
      float average_2 = 0;                                                                                              //
      float average_3 = 0;                                                                                              //
      float average_4 = 0;                                                                                              //
                                                                                                                        //
    // take N samples in a row, with a slight delay                                                                     //
      for (i=0; i< NUMSAMPLES; i++)                                                                                     //
       {                                                                                                                //
         analogRead(THERMISTORPIN_1);                                                                                   //
         samples_1[i] = analogRead(THERMISTORPIN_1);  //Read analog pins connected to thermistor probe sensors          //
         delay(10);                                                                                                     //
         analogRead(THERMISTORPIN_2);                                                                                   //
         samples_2[i] = analogRead(THERMISTORPIN_2);  //Read analog pins connected to thermistor probe sensors          //
         delay(10);                                                                                                     //
         analogRead(THERMISTORPIN_3);                                                                                   //
         samples_3[i] = analogRead(THERMISTORPIN_3);  //Read analog pins connected to thermistor probe sensors          //
         delay(10);                                                                                                     //
         analogRead(THERMISTORPIN_4);                                                                                   //
         samples_4[i] = analogRead(THERMISTORPIN_4);  //Read analog pins connected to thermistor probe sensors          //
         delay(10);                                                                                                     //
          }                                                                                                             //
                                                                                                                        //
        for (i=0; i< NUMSAMPLES; i++)                                                                                   //
         {                                                                                                              //
          average_1 += samples_1[i];                                                                                    //
          average_2 += samples_2[i];                                                                                    //
          average_3 += samples_3[i];                                                                                    //
          average_4 += samples_4[i];                                                                                    //
           }                                                                                                            //
                                                                                                                        //
        average_1 /= NUMSAMPLES;                                                                                        //
        average_2 /= NUMSAMPLES;                                                                                        //
        average_3 /= NUMSAMPLES;                                                                                        //
        average_4 /= NUMSAMPLES;                                                                                        //
                                                                                                                        //
    // Convert the value to resistance                                                                                  //
      average_1 = 1023 / average_1 - 1;                                                                                 //
      average_2 = 1023 / average_2 - 1;                                                                                 //
      average_3 = 1023 / average_3 - 1;                                                                                 //
      average_4 = 1023 / average_4 - 1;                                                                                 //
                                                                                                                        //
      average_1 = SERIESRESISTOR / average_1;                                                                           //
      average_2 = SERIESRESISTOR / average_2;                                                                           //
      average_3 = SERIESRESISTOR / average_3;                                                                           //
      average_4 = SERIESRESISTOR / average_4;                                                                           //
                                                                                                                        //
   // Convert all that info to celsius                                                                                  //
     float probe_1;                                                                                                     //
      probe_1 = average_1 / THERMISTORNOMINAL;     // (R/Ro)                                                            //
      probe_1 = log(probe_1);                  // ln(R/Ro)                                                              //
      probe_1 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)                                                      //
      probe_1 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)                                                       //
      probe_1 = 1.0 / probe_1;                 // Invert                                                                //
      probe_1 -= 273.15;                         // convert to C                                                        //
     float probe_1F = ((probe_1 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit                                           //
                                                                                                                        //
     float probe_2;                                                                                                     //
      probe_2 = average_2 / THERMISTORNOMINAL;     // (R/Ro)                                                            //
      probe_2 = log(probe_2);                  // ln(R/Ro)                                                              //
      probe_2 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)                                                      //
      probe_2 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)                                                       //
      probe_2 = 1.0 / probe_2;                 // Invert                                                                //
      probe_2 -= 273.15;                         // convert to C                                                        //
     float probe_2F = ((probe_2 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit                                           //
                                                                                                                        //
     float probe_3;                                                                                                     //
      probe_3 = average_3 / THERMISTORNOMINAL;     // (R/Ro)                                                            //
      probe_3 = log(probe_3);                  // ln(R/Ro)                                                              //
      probe_3 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)                                                      //
      probe_3 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)                                                       //
      probe_3 = 1.0 / probe_3;                 // Invert                                                                //
      probe_3 -= 273.15;                         // convert to C                                                        //
     float probe_3F = ((probe_3 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit                                           //
                                                                                                                        //
     float probe_4;                                                                                                     //
      probe_4 = average_4 / THERMISTORNOMINAL;     // (R/Ro)                                                            //
      probe_4 = log(probe_4);                  // ln(R/Ro)                                                              //
      probe_4 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)                                                      //
      probe_4 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)                                                       //
      probe_4 = 1.0 / probe_4;                 // Invert                                                                //
      probe_4 -= 273.15;                                                                                                //
      float probe_4F = ((probe_4 * 9.0)/ 5.0 + 32.0); // convert to fahrenheit                                          //
                                                                                                                        //
//END Temperature Probes                                                                                                //
                                                                                        //This section will change later//


///////////////////////////////////////////////////USER INTERFACE//////////////////////////////////////////////////////////////////////
//BEGIN Button Interface
  //Select Button Setup
      if (digitalRead(selectButton) == 1 )  // If the select button switch (pin 5) recieves a signal
       {
        if (buttonMode == 0)  // If buttonMode is 0 set to 1
         {
          (buttonMode = 1);  // Allows the smoke level to be changed
          (selectOff = millis());  // Sets the selectOff time
           }
        else if (buttonMode == 1)  // If buttonMode is 1, change to 2
         {
          (buttonMode = 2);  // Allows the set temperature to be changed by 1
          (selectOff = millis());  // Sets the selectOff time
           }             
        else if (buttonMode == 2)  // If buttonMode is 2, change to 3
         {
          (buttonMode = 3);  // Allows the set temperature to be changed by 10
          (selectOff = millis());  // Sets the selectOff time
           }
        else if (buttonMode == 3)  // If buttonMode is 3, change to 4
         {
          (buttonMode = 4);  // Toggles between Celsius and Fahrenheit
          (selectOff = millis());  // Sets the selectOff time
           }
        else if (buttonMode == 4)  // If buttonMode is 4, change to 0
         {
          (buttonMode = 0);  // Disables the up and down buttons
          (selectOff = millis());  // Sets the selectOff time
           }
          } // End of Select Button 

  //Smoke Level Buttons +5 change
      if (buttonMode == 1)  // If buttonMode is 1 then the smoke level can be changed
       {
        if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
         {
          (smokeLevel = smokeLevel +5);    // Add five to smokeLevel, smokeLevel is percentage of 100 second cycles of time
          (selectOff = millis());          // Sets the selectOff time
            }
        if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
         {
          (smokeLevel = smokeLevel -5);    // Subtract five from smokeLevel
          (selectOff = millis());          // Sets the selectOff time
           }
        smokeLevel = constrain(smokeLevel, 0, 100);  // Constrain the smokeLevel to 0-100   
         } // End of Smoke Level Buttons

  //Temperature Level Buttons +1
      if (buttonMode == 2)  // If buttonMode is 2 then the temperature can be changed by 1 degree
       {
        if (tempDisplay == 0)  // If Celsius is selected
         {
          if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
           {
            (setTemp ++);            // Add one to setTemp, setTemp is the desired temp of the smoker
            (selectOff = millis());  // Sets the selectOff time
             }   
          if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTemp --);            // Subtract one from setTemp
            (selectOff = millis());  // Sets the selectOff time
             }
          setTemp = constrain(setTemp, 0, 125);      // Constrain setTemp to 0-125
          setTempF = ((setTemp * 9.0)/ 5.0 + 32.0);  // Match Fahrenheit temp to Celsius temp
          setTempF = constrain(setTempF, 32, 250);   // Constrain setTempF to 32-250
           } // End Celsius
  
        else if (tempDisplay == 1)  // If Fahrenheit is selected
         {
          if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
           {
            (setTempF ++);           // Add one to setTempF, setTempF is the desired temp of the smoker
            (selectOff = millis());  // Sets the selectOff time
             }   
          if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTempF --);           // Subtract one from setTempF
            (selectOff = millis());  // Sets the selectOff time
             }
          setTempF = constrain(setTempF, 32, 250);    // Constrain setTemp to 32-250
          setTemp = ((setTempF - 32.0) * 5.0 / 9.0);  // Match Celisus temp to Fahrenheit temp
          setTemp = constrain(setTemp, 0, 125);       // Constrain setTemp to 0-125
           } // End Fahrenheit
       } // End of Temperaure +1 Buttons

  //Temperature Level Buttons +10
      if (buttonMode == 3)  // If buttonMode is 3 then the temperature can be changed by 10 degrees
       {
        if (tempDisplay == 0)  // If Celsius is selected
         {
          if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
           {
            (setTemp = setTemp + 10);  // Add ten to setTemp, setTemp is the desired temp of the smoker
            (selectOff = millis());    // Sets the selectOff time
             }   
          if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTemp = setTemp - 10);  // Subtract ten from setTemp
            (selectOff = millis());    // Sets the selectOff time
             }
          setTemp = constrain(setTemp, 0, 125);      // Constrain setTemp to 0-125
          setTempF = ((setTemp * 9.0)/ 5.0 + 32.0);  // Match Fahrenheit temp to Celsius temp
          setTempF = constrain(setTempF, 32, 250);   // Constrain setTempF to 32-250
           } // End Celsius
  
        else if (tempDisplay == 1)  // If Fahrenheit is selected
         {
          if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
           {
            (setTempF = setTempF + 10);  // Add ten to setTempF, setTempF is the desired temp of the smoker
            (selectOff = millis());      // Sets the selectOff time
             }   
          if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
           {
            (setTempF = setTempF - 10);  // Subtract ten from setTempF
            (selectOff = millis());      // Sets the selectOff time
             }
          setTempF = constrain(setTempF, 32, 250);    // Constrain setTemp to 32-250
          setTemp = ((setTempF - 32.0) * 5.0 / 9.0);  // Match Celisus temp to Fahrenheit temp
          setTemp = constrain(setTemp, 0, 125);       // Constrain setTemp to 0-125
           } // End Fahrenheit
       } // End of Temperaure +10 Buttons

  //Temperature Celsius to Fahrenheit Buttons
      if (buttonMode == 4)  // If buttonMode is 4 then the temperature can be toggled between C and F
       {
        if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
         {
          if (tempDisplay == 0)  // If Celsius is selected
           {
            (tempDisplay = 1);       // Sets the temp to Fahrenheit
            (selectOff = millis());  // Sets the selectOff time
             }
          else if (tempDisplay == 1)  // If Fahrenheit is selected
           {
            (tempDisplay = 0);       // Sets the temp to Celsius
            (selectOff = millis());  // Sets the selectOff time
             }
            }
        if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
         {
          if (tempDisplay == 0)  // If Celsius is selected
           {
            (tempDisplay = 1);       //Sets the temp to Fahrenheit
            (selectOff = millis());  // Sets the selectOff time
             }
          else if (tempDisplay == 1)  // If Fahrenheit is selected
           {
            (tempDisplay = 0);       //Sets the temp to Celsius
            (selectOff = millis());  // Sets the selectOff time
             }
            }
          } // End of Temperature Celsius to Fahrenheit Buttons
//END Button Interface


//BEGIN PID Editing
  //Enter PID Editing mode
      if (buttonMode == 0)  // If buttonMode is 0, then you can enter the PID editing display
       {
        if (digitalRead(upButton) == 1)  // If the upButton (pin7) recieves a signal
         {
          if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal with the upButton
           {
            (buttonMode = 5);        // Set buttonMode to 5 and enters the PID editing display
            (selectOff = millis());  // Sets the selectOff time
            lcd.clear();             // Clears the LCD display for the PID interface
            }
           }
          } // End Enter PID editing Mode
      
  //PID Editing mode
      else if (buttonMode == 5)  // If buttonMode is 5, then you can edit the PID parameters
       {
        if (digitalRead(upButton) == 1)  // If the upButton (pin7) recieves a signal
         {
          if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal with the upButton
           {
            (buttonMode = 0);        // Set buttonMode to 0 and exit the PID settings display
            (selectOff = millis());  // Sets the selectOff time
            lcd.clear();             // Clears the LCD display for the PID interface
            }
           }
    //PID Edit Select Buttons  
        if (digitalRead(selectButton)== 1 )  // If the select button switch (pin 5) recieves a signal
         {
          if (pidEdit == 0)  // If pidEdit is 0 set to 1
           {
            (pidEdit = 1);  // Allows the P value to be changed
            (selectOff = millis());  // Sets the selectOff time
             }
          else if (pidEdit == 1)  // If pidEdit is 1, change to 2
           {
            (pidEdit = 2);  // Allows the I value to be changed
            (selectOff = millis());  // Sets the selectOff time
             }             
          else if (pidEdit == 2)  // If pidEdit is 2, change to 3
           {
            (pidEdit = 3);  // Allows the D value to be changed
            (selectOff = millis());  // Sets the selectOff time
             }
          else if (pidEdit == 3)  // If pidEdit is 3, change to 4
           {
            (pidEdit = 4);  // Allows the sample time to be changed
            (selectOff = millis());  // Sets the selectOff time
             }
          else if (pidEdit == 4)  // If pidEdit is 4, change to 0
           {
            (pidEdit = 0);  // Disables the up and down buttons
            (selectOff = millis());  // Sets the selectOff time
             }
            } // End of PID Select Button 

    //PID Parameter edit buttions
          if (pidEdit == 1)  // If pidEdit is in mode 1 then the P parameter can be changed
           {
            if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
             {
              (Kp ++);  // Add one to P parameter of PID program
              (selectOff = millis());  // Sets the selectOff time
               }
            if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
             {
              (Kp --);  // Subtract one from P parameter of PID program
              (selectOff = millis());  // Sets the selectOff time
               }
            Kp = constrain(Kp, 0, 100);  // Constrain the P parameter to 0-100   
             } // End of P Parameter edit buttons

          if (pidEdit == 2)  // If pidEdit is in mode 2 then the I parameter can be changed
           {
            if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
             {
              (Ki ++);  // Add one to I parameter of PID program
              (selectOff = millis());  // Sets the selectOff time
               }
            if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
             {
              (Ki --);  // Subtract one from I parameter of PID program
              (selectOff = millis());  // Sets the selectOff time
               }
            Ki = constrain(Ki, 0, 3600);  // Constrain the I parameter to 0-3600   
             } // End of I Parameter edit buttons

          if (pidEdit == 3)  // If pidEdit is in mode 3 then the D paramater can be changed
           {
            if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
             {
              (Kd ++);  // Add one to D parameter of PID program
              (selectOff = millis());  // Sets the selectOff time
               }
            if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
             {
              (Kd --);  // Subtract one from D parameter of PID program
              (selectOff = millis());    // Sets the selectOff time
               }
            Kd = constrain(Kd, 0, 3600);  // Constrain the D parameter to 0-3600
             } // End of D Parameter edit buttons

          if (pidEdit == 4)  // If pidEdit is in mode 4 then the sample time can be changed
           {
            if (digitalRead(upButton) == 1 )   // If the upButton (pin7) recieves a signal
             {
              (WindowSize ++);  // Add one to sample time of PID program
               }
            if (digitalRead(downButton) == 1)  // If the downButton (pin6) recieves a signal
             {
              (WindowSize --);  // Subtract one from sample time of PID program
               }
            //WindowSize = constrain(WindowSize, 0, 3600);  // Constrain the sample time to 0-100   
             }    //End of sample time edit buttons
        } //END PID Editing Mode
//END PID Editing        


//BEGIN Display Programming
  //Blinking Text While Editing
      unsigned long blinkOn = millis();

      if (blinkOn - blinkOff >= blinkTime)   //  Check how long the text has been blinking
       {
        (blinkOff = blinkOn);  // Save the last time the text blinked
        if (blinkState == 0)   // If the text blink is off
         {
          (blinkState = 1);  // Turn the text blink on
           } 
        else // If the text blink is on
        {
         (blinkState = 0);   // Turn text blink off
          }
         } // End of Blinking Text While Editing

  //Turn off editing after 10 seconds
      unsigned long selectOn = millis();  // Set the current time
       if (buttonMode > 0)  // If the Settings are in edit mode
        {
         if (selectOn - selectOff >= selectTime)  //  Check how long the text has been blinking
          {
           (buttonMode = 0);  // Turn off editing if longer than delay
           (pidEdit = 0);     // Turn off PID editing if longer than delay
           lcd.clear();       // Clears the LCD display for the display interface
           }
         } // End of Turn off editing after 10 seconds
//END Display Programming


///////////////////////////////////////////////////BEGIN LCD DISPLAY///////////////////////////////////////////////////////////////////
//BEGIN Normal Display
      if (buttonMode < 5)  // Check if the buttonMode is less than 5, which is the normal Display
       {
  //First Row
      lcd.setCursor(7, 0);
      lcd.print("   ");  // Controls overlap
          
  //Set Temp
    // +1 Temp Editor
      if (buttonMode == 2)  // Check if the +1 setTemp editor is activated
       {
        if (tempDisplay == 0) // If Celsius is selected 
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4, 0);
            lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
            lcd.print("\337C");     // Celsius symbol          
             }
          if (blinkState == 0)  // Check if the text blink is on/off
           {
            if (setTemp >= 100) // Check if setTemp is 3 characters
             {
              lcd.setCursor(6, 0);
              lcd.print(" ");      // Insert blank character one's place
              lcd.setCursor(7, 0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 100)  // If setTemp is 2 characters
             {
              lcd.setCursor(5, 0);
              lcd.print(" ");      // Insert blank character one's place
              lcd.setCursor(6, 0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 10)  // If the setTemp is one character
             {
              lcd.setCursor(4, 0);
              lcd.print(" ");      // Insert blank character in one's place
              lcd.print("\337C");  // Celsius symbol
              lcd.print("   ");    // Controls overlap
               }
              } // End Blink
            } // End Celsius
        else if (tempDisplay == 1)  // If Fahrenheit is selected
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4, 0);
            lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
            lcd.print("\337F");      // Fahrenheit symbol          
             }
          if (blinkState == 0)  // Check if the text blink is on/off
           {
            if (setTempF >= 100)  // Check if setTempF is 3 characters
             {
              lcd.setCursor(6, 0);
              lcd.print(" ");      // Insert blank character one's place
              lcd.setCursor(7, 0);
              lcd.print("\337F");  // Fahrenheit symbol  
               }
            if (setTempF < 100)   // If setTempF is two characters
             {
              lcd.setCursor(5, 0);
              lcd.print(" ");      // Insert blank character one's place
              lcd.setCursor(6, 0);
              lcd.print("\337F");  // Fahrenheit symbol  
               }
            if (setTempF < 10)  // If the setTempF is one character
             {
              lcd.setCursor(4, 0);
              lcd.print(" ");      // Insert blank character in one's place
              lcd.print("\337F");  // Fahrenheit symbol  
              lcd.print("   ");    // Controls overlap
               }
              } // End Blink
            } // End Fahrenheit
         } // End setTemp +1
  
    // +10 Temp Editor
      else if (buttonMode == 3)  // Check if the +10 setTemp editor is activated
       {
        if (tempDisplay == 0)  // If Celsius is selected 
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4, 0);
            lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
            lcd.print("\337C");     // Celsius symbol          
             }
          if (blinkState == 0)  // Check if the text blink is on/off
           {
            if (setTemp >= 100)  // Check if setTemp is 3 characters
             {
              lcd.setCursor(5, 0);
              lcd.print(" ");      // Insert blank character ten's place
              lcd.setCursor(7, 0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 100)   //  If setTemp is two characters
             {
              lcd.setCursor(4, 0);
              lcd.print(" ");      // Insert blank character ten's place
              lcd.setCursor(6, 0);
              lcd.print("\337C");  // Celsius symbol
               }
            if (setTemp < 10)  //  If the setTemp is below ten
             {
              lcd.setCursor(4, 0);
              lcd.print(" ");      // Insert blank character in one's place
              lcd.print("\337C");  // Celsius symbol
              lcd.print("   ");    // Controls overlap
               }
              } // End Blink
            } // End Celsius
        else if (tempDisplay == 1) // If Fahrenheit is selected
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4, 0);
            lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
            lcd.print("\337F");   // Fahrenheit symbol          
             }
          if (blinkState == 0)  // Check if the text blink is on/off
           {
            if (setTempF >= 100)  // Check if setTemp is 3 characters
             {
              lcd.setCursor(5, 0);
              lcd.print(" ");      // Insert blank character one's place
              lcd.setCursor(7, 0);
              lcd.print("\337F");  // Fahrenheit symbol  
               }
            if (setTempF < 100)   // If setTempF is two characters
             {
              lcd.setCursor(4, 0);
              lcd.print(" ");      // Insert blank character one's place
              lcd.setCursor(6, 0);
              lcd.print("\337F");  // Fahrenheit symbol  
               }
            if (setTempF < 10)  // If the setTempF is one character
             {
              lcd.setCursor(4, 0);
              lcd.print(" ");      // Insert blank character in one's place
              lcd.print("\337F");  // Fahrenheit symbol  
              lcd.print("   ");    // Controls overlap
               }
              } // End Blink
            } // End Fahrenheit
          } // End setTemp +10
     
    // Celsius to Fahrenheit Editor
      else if (buttonMode == 4)  // Check if the Celsius to Fahrenheit editor is activated
       {
        if (tempDisplay == 0)  // If Celsius is selected
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4, 0);
            lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
            lcd.print("\337C");     // Celsius symbol
             }
          if (blinkState == 0)  // Check if the text blink is on/off
           {
            if (setTemp >= 100)  // Check if setTemp is 3 characters
             {
              lcd.setCursor(4, 0);
              lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
              lcd.print("\337  ");    // Degree symbol
               }
            if (setTemp < 100)   // If setTemp is two characters
             {
              lcd.setCursor(4, 0);
              lcd.print(setTemp, 0);  // Insert blank character one's place
              lcd.print("\337   ");   // Degree symbol
               }
            if (setTemp < 10)  // If the setTemp is once character
             {
              lcd.setCursor(4, 0);
              lcd.print(setTemp, 0);  // Insert blank character one's place
              lcd.print("\337    ");  // Degree symbol
               }
              } // End Blink
            } // End Celsius
        else if (tempDisplay == 1)  // If Fahrenheit is selected
         {
          if (blinkState == 1)
           {
            lcd.setCursor(4, 0);
            lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
            lcd.print("\337F");      // Fahrenheit symbol
             }
          if (blinkState == 0)  // Check if the text blink is on/off
           {
            if (setTempF >= 100)  // Check if setTempF is 3 characters
             {
              lcd.setCursor(4, 0);
              lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
              lcd.print("\337  ");     // Degree symbol
               }   
            if (setTempF < 100)   // If setTempF is two characters
             {
              lcd.setCursor(4, 0);
              lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
              lcd.print("\337   ");    // Degree symbol
               }
              } // End Blink
            } // End Fahrenheit
          } // End Celsius to Fahrenheit Editor

    // Editor Mode not activated
      else  // If none of the buttonMode editor settings are in effect
       {
        lcd.setCursor(4, 0);
        if (tempDisplay == 0)  // If Celsius is selected
         {
          lcd.print(setTemp, 0);  // This displays the set temperature in Celsius
          lcd.print("\337C");     // Celsius symbol
           }
        else if (tempDisplay == 1)  // If Fahrenheit is selected
         {
          lcd.print(setTempF, 0);  // This displays the set temperature in Fahrenheit
          lcd.print("\337F");      // Fahrenheit symbol
           }
         } // End of Editor Mode not active
  // End of setTemp display
                
  //Temp Probe 1//////////////////////////////cant decide on the lable yet
    //lcd.setCursor(10, 0);
    //lcd.print("Tray1 ");
      lcd.setCursor(10, 0);
      lcd.print("P1 ");
      if (tempTime - tempUpdate > tempDelay)  // Check if enough time has passed to update the display
       {      
        if (analogRead(THERMISTORPIN_1) > 1)  // If the sensor is connected
         {
          lcd.setCursor(13, 0);
           if (tempDisplay == 0)  // If Celsius is selected
            {
             lcd.print(probe_1, 0);   // This displays probe_1 temperature
             lcd.print("\337C    ");  // Celsius symbol
              }
           else if (tempDisplay == 1)  // If Fahrenheit is selected
            {
             lcd.print(probe_1F, 0);  // This displays probe_1 temperature in F
             lcd.print("\337F    ");  // Fahrenheit symbol
              }
            } 
        if (analogRead(THERMISTORPIN_1) < 1)  // If the sensor is disconnected
         {
          lcd.setCursor(13, 0);
          lcd.print("errr   ");  // This displays an error message
           }
        }
  // End Temp Probe 1 
        
  //Third Row Overlap      
      lcd.setCursor(0, 2);
      lcd.print(" ");  // Controls overlap
      
  //Second Row
      lcd.setCursor(2, 1);
      lcd.print("Smoker");
      
  //Temp Probe 2///////////////////////////////////////////////////////
    //lcd.setCursor(10, 1);
    //lcd.print("Tray2 ");
      lcd.setCursor(11, 1);
      lcd.print("P2 ");
      if (tempTime - tempUpdate > tempDelay)  // Check if enough time has passed to update the display
       {      
        if (analogRead(THERMISTORPIN_2) > 1)  // If the sensor is connected
         {
          lcd.setCursor(14, 1);
          if (tempDisplay == 0)  // If Celsius is selected
           {
            lcd.print(probe_2, 0);  // This displays probe_2 temperature
            lcd.print("\337C   ");  // Celsius symbol
             }
          else if (tempDisplay == 1)  // If Fahrenheit is selected
           {
            lcd.print(probe_2F, 0);  // This displays probe_2 temperature in F
            lcd.print("\337F   ");   // Fahrenheit symbol
             }
            } 
          if (analogRead(THERMISTORPIN_2) < 1)  // If the sensor is disconnected
           {
            lcd.setCursor(14, 1);
            lcd.print("errr  ");  // This displays an error message
             }
            }
  // End Temp Probe 2
        
  //Third Row Overlap
      lcd.setCursor(8, 2);
      lcd.print("  ");  // Controls overlap
        
  //Smoker Temp
      if (tempTime - tempUpdate > tempDelay)  // Check if enough time has passed to update the display
       {
        lcd.setCursor(3, 2);
        if (isnan(c))  // Check if smokerTemp isNaN
         {
          lcd.print("error");  // Diplays when there is a thermocouple error
           }
        else
         {
          if (tempDisplay == 0)  // If Celsius is selected
           {
            lcd.print(smokerTemp, 0);  // This displays the smoker temperature
            lcd.print("\337C  ");      // Celsius symbol
             }
           else if (tempDisplay == 1)  // If Fahrenheit is selected
            {
             lcd.print(smokerTempF, 0);  // This displays the smoker temperature in F
             lcd.print("\337F  ");       // Fahrenheit symbol
             }
            }
           }
  // End Smoker Temp
    
  //Temp Probe 3
    //lcd.setCursor(10, 2);
    //lcd.print("Tray3 ");
      lcd.setCursor(12, 2);
      lcd.print("P3 ");  
      if (tempTime - tempUpdate > tempDelay)  // Check if enough time has passed to update the display
       { 
        if (analogRead(THERMISTORPIN_3) > 1)  // If the sensor is connected
         {
          lcd.setCursor(15, 2);
          if (tempDisplay == 0)  // If Celsius is selected
           {
            lcd.print(probe_3, 0);  // This displays probe_3 temperature
            lcd.print("\337C  ");   // Celsius symbol
             }
          else if (tempDisplay == 1)  // If Fahrenheit is selected
           {
            lcd.print(probe_3F, 0);  // This displays probe_3 temperature in F
            lcd.print("\337F  ");    // Fahrenheit symbol
             }
           } 
        if (analogRead(THERMISTORPIN_3) < 1) // If the sensor is disconnected
         {
          lcd.setCursor(15, 2);
          lcd.print("errr ");  // This displays an error message
           }
          }
  // End Temp Probe 3
    
  //Second Row Overlap          
      lcd.setCursor(0, 1);
      lcd.print(" ");  // Controls overlap
       
  //Fourth Row
      lcd.setCursor(10, 3);
      lcd.print("   ");  // Controls overlap
    
  //Smoke Level Display
    //Smoke Editor Active
      if (buttonMode == 1)  // Check if the smoke level editor is activated
       {
        if (blinkState == 1)  // Print the smoke level
         {
          if (smokeLevel == 100)  // If the smoke level is 3 characters
          {
           lcd.setCursor(0, 3);
           lcd.print(smokeLevel, 0);  // This displays the smoke level
            }
          if (smokeLevel < 100)   // If the smoke level is 2 characters
           {
            lcd.setCursor(0, 3);
            lcd.print(" ");            // Controls Overlap
            lcd.print(smokeLevel, 0);  // This displays the smoke level
             }
          if (smokeLevel < 10)  // If the smoke level is one character
           {
            lcd.setCursor(0, 3);
            lcd.print("  ");           // Controls overlap
            lcd.print(smokeLevel, 0);  // This displays the smoke level
             }
           }
        if (blinkState == 0)  // Check to see if the text blink is on/off
         {
          if (smokeLevel == 100)  // If the smoke level is 3 charcaters
          {
           lcd.setCursor(0, 3);
           lcd.print(smokeLevel, 0);  // This displays the smoke leve
           lcd.setCursor(2, 3);
           lcd.print(" ");            // Inserts blank space in one's place
            }
          if (smokeLevel < 100) // If the smoke level is 2 characters
           {
            lcd.setCursor(0, 3);
            lcd.print(" ");            // Controls overlap
            lcd.print(smokeLevel, 0);  // This displays the smoke level
            lcd.setCursor(2, 3);
            lcd.print(" ");            // Inserts blank space in one's place
             }
          if (smokeLevel < 10) // If the smoke level is one characters
           {
            lcd.setCursor(0, 3);
            lcd.print("  ");           // Controls overlap
            lcd.print(smokeLevel, 0);  // This displays the smoke level
            lcd.setCursor(2, 3);
            lcd.print(" ");            // Inserts blank space in one's place
             }
            } // End blink
          } // End smoke editor

    // Editor Mode not Active  
      else  // If the smoke level editor is not activated print normal text
       {
        if (smokeLevel == 100)  // If the smoke level is 3 charcaters
         {
          lcd.setCursor(0, 3);
          lcd.print(smokeLevel, 0);  // This displays the smoke level
           }
        if (smokeLevel < 100)   // If the smoke level is 2 characters
         {
          lcd.setCursor(0, 3);
          lcd.print(" ");            // Controls overlap
          lcd.print(smokeLevel, 0);  // This displays the smoke level
           }
        if (smokeLevel < 10) // If the smoke level is one character
         {
          lcd.setCursor(0, 3);
          lcd.print("  ");           // Controls overlap
          lcd.print(smokeLevel, 0);  // This displays the smoke level
           }
         } // End editor mode not active
  // End Smoke Level Display

  //Fourth Row Cont.
      lcd.setCursor(3, 3);
      lcd.print("% Smoke");
    
  //Temp Probe 4 
    //lcd.setCursor(10, 3);
    //lcd.print("Tray4 ");
      lcd.setCursor(13, 3);
      lcd.print("P4 ");
      if (tempTime - tempUpdate > tempDelay)
       {
        if (analogRead(THERMISTORPIN_4) > 1)  // If the sensor is connected
         {
          lcd.setCursor(16, 3);
          if (tempDisplay == 0)  // If Celsius is selected
           {
            lcd.print(probe_4, 0);  // This displays probe_4 temperature
            lcd.print("\337C ");    // Celsius symbol
             }
          else if (tempDisplay == 1)  // If Fahrenheit is selected
           {
            lcd.print(probe_4F, 0);  // This displays probe_4 temperature in fahrenheit
            lcd.print("\337F ");     // Fahrenheit symbol
             }
           } 
        if (analogRead(THERMISTORPIN_4) < 1)  // If the sensor is disconnected
         {
          lcd.setCursor(16, 3);
          lcd.print("errr");  // This displays an error message
           } 
         }
  // End Temp Probe 4
        
  //First Row Overlap
      lcd.setCursor(0, 0);
      lcd.print("Set ");   // This is the set temp label
     }//End normal display
//END Normal Display



//BEGIN Temperature Display Update
      if (tempTime - tempUpdate > tempDelay)
       {
        (tempUpdate = millis());
         }
//END Temperature Display Update



//BEGIN PID Editing Display
      float errorAbs = (setTemp - smokerTemp);
      errorAbs = abs(errorAbs);
      percentError = (errorAbs/setTemp*100);
      if (buttonMode == 5)  // Check if buttonMode is 5, which allows PID parameter editing
       {
  // Set Value
      lcd.setCursor(0, 0);
      lcd.print("SV: ");
      lcd.print(Setpoint, 1);
  // Point Value
      lcd.setCursor(0, 1);
      lcd.print("PV: ");
      lcd.print(Input, 1);
  // Error
      lcd.setCursor(0, 2);
      lcd.print("Err: ");
      lcd.print(percentError, 1); 
      lcd.print("%");        
  // Output
      lcd.setCursor(0, 3);
      lcd.print("Out: ");
      lcd.print(Output, 0);
  // P parameter setting display
      lcd.setCursor(12, 0);
      lcd.print("P: ");
      if (pidEdit == 1)  // Check if the P parameter editor is activated
       {
        if (blinkState == 1)  // Print the P Value
         {
          lcd.setCursor(15, 0);
          lcd.print(Kp, 0);  // This displays the P Value
           }                         
        if (blinkState == 0)  // Check to see if the text blink is on/off
         {
          if (Kp > 99)  // If the P value is 3 characters
           {
            lcd.setCursor(15, 0);
            lcd.print(Kp, 0);  // This displays the P Value
            lcd.setCursor(17, 0);
            lcd.print(" ");    // Inserts blank space in one's place
             }
          if (Kp < 100)  // If the P value is 2 characters
           {
            lcd.setCursor(15, 0);
            lcd.print(Kp, 0);  // This displays the P Value
            lcd.setCursor(16, 0);
            lcd.print("  ");   // Inserts blank space in one's place
             }
          if (Kp < 10) // If the P value is one character
           {
            lcd.setCursor(15, 0);
            lcd.print(Kp, 0);  // This displays the P Value
            lcd.setCursor(15, 0);
            lcd.print("   ");  // Inserts blank space in one's place
             }
            } // End blink
          } // End of P paramter edit
      else  // If the P parameter editor is not activated print normal text
       {
        lcd.setCursor(15, 0);
        lcd.print(Kp, 0);  // This displays the P Value
         }
  // End P parameter display
        
  // I parameter setting display
      lcd.setCursor(12, 1);
      lcd.print("I: ");
      if (pidEdit == 2)  // Check if the I parameter editor is activated
       {
        if (blinkState == 1)  // Print the I Value
         {
          lcd.setCursor(15, 1);
          lcd.print(Ki, 0);  // This displays the I Value
           }                         
        if (blinkState == 0)   // Check to see if the text blink is on/off
         {
          if (Ki > 99)  // If the I value is 3 characters
           {
            lcd.setCursor(15, 1);
            lcd.print(Ki, 0);  // This displays the I Value
            lcd.setCursor(17, 1);
            lcd.print(" ");    // Inserts blank space in one's place
             }
          if (Ki < 100) // If the I value is 2 characters
           {
            lcd.setCursor(15, 1);
            lcd.print(Ki, 0);  // This displays the I Value
            lcd.setCursor(16, 1);
            lcd.print("  ");   // Inserts blank space in one's place
             }
          if (Ki < 10) // If the I value is one character
           {
            lcd.setCursor(15, 1);
            lcd.print(Ki, 0);  // This displays the I Value
            lcd.setCursor(15, 1);
            lcd.print("   ");  // Inserts blank space in one's place
             }
            } // End Blink
          } // End of I parameter edit
      else  // If the I parameter editor is not activated print normal text
       {
        lcd.setCursor(15, 1);
        lcd.print(Ki, 0);  // This displays the I Value
         }
  // End I parameter display

  // D parameter setting display
      lcd.setCursor(12, 2);
      lcd.print("D: ");
      if (pidEdit == 3)  // Check if the D parameter editor is activated
       {
        if (blinkState == 1)  // Print the D Value
         {
          lcd.setCursor(15, 2);
          lcd.print(Kd, 0);  // This displays the D Value
           }                         
        if (blinkState == 0)  // Check to see if the text blink is on/off
         {
          if (Kd > 99)  // If the D value is 3 characters
           {
            lcd.setCursor(15, 2);
            lcd.print(Kd, 0);  // This displays the D Value
            lcd.setCursor(17, 2);
            lcd.print(" ");    // Inserts blank space in one's place
             }
          if (Kd < 100)  // If the D value is 2 characters
           {
            lcd.setCursor(15, 2);
            lcd.print(Kd, 0);  // This displays the D Value
            lcd.setCursor(16, 2);
            lcd.print("  ");   // Inserts blank space in one's place
             }
          if (Kd < 10)  // If the D value is one characters
           {
            lcd.setCursor(15, 2);
            lcd.print(Kd, 0);  // This displays the D Value
            lcd.setCursor(15, 2);
            lcd.print("   ");  // Inserts blank space in one's place
             }
            } // End Blink
          } // End of D parameter edit
      else  // If the D parameter editor is not activated print normal text
       {
        lcd.setCursor(15, 2);
        lcd.print(Kd, 0);  // This displays the D Value
         }
  // End D parameter display

  // Sample time setting display
      lcd.setCursor(9, 3);
      lcd.print("Time: ");
      if (pidEdit == 4)  // Check if the time sample time editor is activated
       {
        if (blinkState == 1)  // Print the sample time
         {
          lcd.setCursor(15, 3);
          lcd.print(WindowSize/1000);  // This displays the Sample time
           }                         
        if (blinkState == 0)  // Check to see if the text blink is on/off
         {
          if ((WindowSize/1000) > 99) // If the sample time is 3 characters
           {
            lcd.setCursor(15, 3);
            lcd.print(WindowSize/1000);  // This displays the sample time
            lcd.setCursor(17, 3);
            lcd.print(" ");  // Inserts blank space in one's place
             }
          if ((WindowSize/1000) < 100) // If the sample time is two characters
           {
            lcd.setCursor(15, 3);
            lcd.print(WindowSize/1000);  // This displays the sample time
            lcd.setCursor(16, 3);
            lcd.print("  ");  // Inserts blank space in one's place
             }
          if ((WindowSize/1000) < 10) // If the sample time is one characters
           {
            lcd.setCursor(15, 3);
            lcd.print(WindowSize/1000);  // This displays the sample time
            lcd.setCursor(15, 3);
            lcd.print("   ");  // Inserts blank space in one's place
             }
            } // End Blink
           } // End of Sample time edit
      else  // If the sample time editor is not activated print normal text
       {
        lcd.setCursor(15, 3);
        lcd.print(WindowSize/1000);  // This displays the sample time
         }
  // End sample time display
       } // End PID display     
//END PID Editing Display


//////////////////////////////////////////////////////SERIAL COMMS/////////////////////////////////////////////////////////////////////
//BEGIN Serial Printing
      Serial.print("Set Temp "); 
      Serial.println(setTemp, 1);     //Print the setTemp
      Serial.print("Set Temp F "); 
      Serial.println(setTempF, 1);    //Print the setTemp in F
      Serial.print("Smoker Temp "); 
      Serial.println(smokerTemp, 1);  //Print the Smoker Temp
      Serial.println();

// Used for debugging purposes
//      int THERMOPIN_1 = analogRead(A0);
//      int THERMOPIN_2 = analogRead(A1);
//      int THERMOPIN_3 = analogRead(A2);
//      int THERMOPIN_4 = analogRead(A3);
//     
//      Serial.print("Temp 1: "); 
//      Serial.print(THERMOPIN_1);
//      Serial.print("    ");
//      Serial.println(probe_1, 1);    //Print temperature probe 1
//      Serial.print("Temp 2: "); 
//      Serial.print(THERMOPIN_2);
//      Serial.print("    ");
//      Serial.println(probe_2, 1);    //Print temperature probe 2
//      Serial.print("Temp 3: "); 
//      Serial.print(THERMOPIN_3);
//      Serial.print("    ");
//      Serial.println(probe_3, 1);    //Print temperature probe 3
//      Serial.print("Temp 4: ");
//      Serial.print(THERMOPIN_4); 
//      Serial.print("    ");
//      Serial.println(probe_4, 1);    //Print temperature probe 4
//      Serial.println();

      Serial.print("Smoke Level: ");
      Serial.print(smokeLevel, 0);  //Print smoke level
      Serial.println("%");

//Used for debugging purposes
//      Serial.print("Smoke Element: ");
//       if (smokeState == 0)
//        {
//          Serial.println("OFF");
//        }
//        if (smokeState == 1)
//        {
//          Serial.println("ON");
//        }
//      Serial.print("Time: ");
//      Serial.println(millis()/1000.0, 1);
//      if (smokeState == 0)
//        {
//          Serial.print("Time Until Element ON: ");
//          Serial.println((100.0-smokeLevel)-(millis()/1000.0-smokeOff), 1);
//        }
//      if (smokeState == 1)
//        {
//          Serial.print("Time Until Element OFF: ");
//          Serial.println((smokeLevel)-(millis()/1000.0-smokeOn), 1);
//        }

//deletelater
      Serial.println();
//      Serial.print("Button Mode = ");
//      Serial.println(buttonMode);
      Serial.print("Setpoint = ");
      Serial.println(Setpoint);
      Serial.print("Input = ");
      Serial.println(Input);
      Serial.print("Output = ");
      Serial.println(Output);
      Serial.print("WindowSize = ");
      Serial.println(WindowSize/1000);
      Serial.print("P = ");
      Serial.println(Kp);
      Serial.print("I = ");
      Serial.println(Ki);
      Serial.print("D = ");
      Serial.println(Kd);
      Serial.print("PID Edit = ");
      Serial.println(pidEdit);                  
      Serial.println();
      Serial.print("error: ");
      Serial.print(percentError, 0);
      Serial.println("%");
      Serial.print("errorAbs = ");
      Serial.println(errorAbs, 1);
      Serial.println();
      Serial.print("Heat Element: ");
      if (heatState == 0)
       {
      Serial.print("OFF");
       }
      if (heatState == 1)
       {
      Serial.print("ON");
       }
      Serial.println();
      Serial.println();
//END Serial Printing
       
}/////////////////////////////////////////////////////END LOOP/////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////END LOOP/////////////////////////////////////////////////////////////////////////


//BEGIN Save Parameters
    void SaveParameters(){    // Save any parameter changes to EEPROM
      if (setTemp != EEPROM_readDouble(setTempAddress))
       {
        EEPROM_writeDouble(setTempAddress, setTemp);
         }
      if (setTempF != EEPROM_readDouble(setTempFAddress))
       {
        EEPROM_writeDouble(setTempFAddress, setTempF);
         }
      if (smokeLevel != EEPROM_readDouble(smokeLevelAddress))
       {
        EEPROM_writeDouble(smokeLevelAddress, smokeLevel);
         }
      if (Setpoint != EEPROM_readDouble(SpAddress))
       {
        EEPROM_writeDouble(SpAddress, Setpoint);
         }                                 
      if (Kp != EEPROM_readDouble(KpAddress))
       {
        EEPROM_writeDouble(KpAddress, Kp);
       }
      if (Ki != EEPROM_readDouble(KiAddress))
       {
        EEPROM_writeDouble(KiAddress, Ki);
       }
      if (Kd != EEPROM_readDouble(KdAddress))
       {
        EEPROM_writeDouble(KdAddress, Kd);
       }
    }
//END Save Paramaters


//BEGIN Load parameters from EEPROM
    void LoadParameters() {
      setTemp = EEPROM_readDouble(setTempAddress);
      setTempF = EEPROM_readDouble(setTempFAddress);
      smokeLevel = EEPROM_readDouble(smokeLevelAddress);
      Setpoint = EEPROM_readDouble(SpAddress);
      Kp = EEPROM_readDouble(KpAddress);
      Ki = EEPROM_readDouble(KiAddress);
      Kd = EEPROM_readDouble(KdAddress);
  // Use defaults if EEPROM values are invalid
      if (isnan(setTemp))
       {
        setTemp = 25;
         }
      if (isnan(setTempF))
       {
        setTempF = 77;
         }
      if (isnan(smokeLevel))
       {
        smokeLevel = 30;
         }
      if (isnan(Setpoint))
       {
        Setpoint = 60;
         }
      if (isnan(Kp))
       {
        Kp = 500;
         }
      if (isnan(Ki))
       {
        Ki = 0.5;
         }
      if (isnan(Kd))
       {
        Kd = 0.1;
         }  
      }
//END Load Parameters


//BEGIN Write floating point values to EEPROM
    void EEPROM_writeDouble(int address, double value){
      byte* p = (byte*)(void*)&value;
      for (int i = 0; i < sizeof(value); i++)
       {
        EEPROM.write(address++, *p++);
         }
      }
//END write floating point values


//BEGIN Read floating point values from EEPROM
    double EEPROM_readDouble(int address)
     {
      double value = 0.0;
      byte* p = (byte*)(void*)&value;
      for (int i = 0; i < sizeof(value); i++)
       {
        *p++ = EEPROM.read(address++);
        }
      return value;
       }
//END read floating point values
