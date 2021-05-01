 /*
 * State testing, developer c2mismo 2019.
 * License GNU, see at the end.
 */

//           LIBRARY CONF

#include "Arduino.h"
#include "Leds.h"
#include "Voltmeter.h"
#include "Ammeter.h"

#define rs485Serial Serial1

//         END LIBRARY CONF

//             PINS

const int signalOnPin = A0;
const int signalHomePin = 3;

const int voltmeterHomePin = A10;
const int voltmeterInPin = A11;
const int voltmeterRightPin = A12;
const int voltmeterLeftPin = A13;
const int ammeterPin = A15;

const int ledLeftIn = 40;
const int ledLeftOut = 41;
const int ledRightIn = 42;
const int ledRightOut = 43;
const int ledInverter = 44;
const int ledCHR = 45;
const int ledHome = 46;
const int ledAutomatic = 47;

Leds leds (ledLeftIn, ledLeftOut, ledRightIn, ledRightOut, ledInverter, ledCHR, ledHome, ledAutomatic);

const int releRightIn = A1;
const int releLeftIn = A2;
const int releRightOut = A3;
const int releLeftOut = A4;
const int releHome = A5;
const int releCHR = A6;
const int reles[7] = {releRightIn, releLeftIn, releRightOut, releLeftOut, releHome, releCHR};

//            END PINS

//           VARIABLES

bool signalOn = false;
bool signalHome = false;

int freq = 20;      // 50 Hz = 20 m/s; 60 Hz = 16.6667 m/s
float calibAmper = 20.6;
int intervalRead = 250;
float amperMax = 5;
Voltmeter voltmeterHome (voltmeterHomePin, freq);
Voltmeter voltmeterIn (voltmeterInPin, freq);
Voltmeter voltmeterRight (voltmeterRightPin, freq);
Voltmeter voltmeterLeft (voltmeterLeftPin, freq);
Ammeter ammeter (ammeterPin, freq, calibAmper, intervalRead, amperMax);

//         END VARIABLES



void setup(){
  while (!Serial);
  Serial.begin(115200);
  rs485Serial.begin(57600);



  pinMode(signalOnPin, INPUT);
  pinMode(signalHomePin, INPUT_PULLUP);

  pinMode(releRightIn, OUTPUT);
  pinMode(releLeftIn, OUTPUT);
  pinMode(releRightOut, OUTPUT);
  pinMode(releLeftOut, OUTPUT);
  pinMode(releHome, OUTPUT);
  pinMode(releCHR, OUTPUT);
  
  pinMode(48, OUTPUT), digitalWrite(48, LOW);
  pinMode(49, OUTPUT), digitalWrite(49, LOW);
  pinMode(50, OUTPUT), digitalWrite(50, LOW);
  pinMode(51, OUTPUT), digitalWrite(51, LOW);
  pinMode(52, OUTPUT), digitalWrite(52, LOW);
  pinMode(53, OUTPUT), digitalWrite(53, LOW);
  pinMode(2, OUTPUT), digitalWrite(2, HIGH);
  pinMode(A7, OUTPUT), digitalWrite(A7, HIGH);

  // leds.blinkOn();
}

void loop(){
  //digitalWrite(ledRightOut, HIGH);

  voltmeterHome.get();
  float voltageHome = voltmeterHome.getVoltage();
//  byteVoltageHome = voltageHome;

  voltmeterIn.get();
  float voltageIn = voltmeterIn.getVoltage();
//  byteVoltageIn = voltageIn;

  voltmeterRight.get();
  float voltageRight = voltmeterRight.getVoltage();
//  byteVoltageRight = voltageRight;

  voltmeterLeft.get();
  float voltageLeft = voltmeterLeft.getVoltage();
//  byteVoltageLeft = voltageLeft;

  ammeter.get();
  float slowAmper = ammeter.getAmperSlow();
//  byteSlowAmper = slowAmper;

  signalOn = digitalRead(signalOnPin);
  signalHome = digitalRead(signalHomePin);

  if (signalOn)
  {
    digitalWrite(ledAutomatic, HIGH);
//    stateAutomatic = true;
  } 
  if (!signalOn) 
  {
    digitalWrite(ledAutomatic, LOW);
//    stateAutomatic = false;
  }
  if (signalHome)
  {
    digitalWrite(ledHome, HIGH);
//    stateHome = true; // if rele home is high actualy test
  } 
  if (!signalHome) 
  {
    digitalWrite(ledHome, LOW);
//    stateHome = false;
  }


}

//         END FUNCTIONS

/*
  License:

    Copyright (C) 2019  c2mismo.

    This file is part of 230box.

    230box is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This 230box is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this 230box, see COPYING.  If not, see <https://www.gnu.org/licenses/>.

    You can download a full copy of 230box at <https://github.com/c2mismo>.
 */
