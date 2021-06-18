#define _chgArduino_

/*
 * Localizado mucho ruido en la lectura de los voltages
 * puede que el arduino esté defectuoso.
 * 
 * Por falta de tiempo se harán varias lecturas
 * y trabajaremos actualmente con la media de ellas
 * hasta localizar el foco del ruido.
 * 
 * State testing, developer c2mismo 2019.
 * License GNU, see at the end.
 */

//           LIBRARY CONF

#include "Arduino.h"
#include "Leds.h"
#include "Voltmeter.h"
#include "Ammeter.h"

#define rs485Serial Serial1

byte pinVcc;

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
const int releRightOut = A3; const int releRightOut_physicalPin = 94;   // And physical pin number
const int releLeftOut = A4; const int releLeftOut_physicalPin = 93;
const int releHome = A5;
const int releCHR = A6;
const int reles[7] = {releRightIn, releLeftIn, releRightOut, releLeftOut, releHome, releCHR};

//            END PINS

//           VARIABLES

bool signalOn;
bool lastSignalOn;
bool signalHome;
bool lastSignalHome;
bool signalRight;
bool lastSignalRight;
bool signalLeft;
bool lastSignalLeft;
bool lockHome = true;
bool inverterStatus = false;
bool testVoltInOk = false;
unsigned long tmrTestVoltInOk;
unsigned long tmrMaxTestVoltInOk = 8000;

int freq = 20;      // 50 Hz = 20 m/s; 60 Hz = 16.6667 m/s
Voltmeter voltmeterHome (voltmeterHomePin, freq);
Voltmeter voltmeterIn (voltmeterInPin, freq);
Voltmeter voltmeterRight (voltmeterRightPin, freq);
Voltmeter voltmeterLeft (voltmeterLeftPin, freq);

float calibAmper = 20.6;
int intervalRead = 250;
float slowAmper;
float amperMax = 5;
bool amperOk = false;
bool amperProtect = false;
unsigned int tmrAmperProtect;
unsigned int tmrAmperUnprotect = 2000;
Ammeter ammeter (ammeterPin, freq, calibAmper, intervalRead, amperMax);
bool overAmper = false;
float overAmperValue;

float voltMin = 190;
float voltMax = 240;
bool voltProtect = false;
unsigned int tmrVoltProtect;
unsigned int tmrVoltUnprotect = 2000;
bool overVolt = false;
float overVoltValue;

  // temporary solution for NOISE
float voltAverageHome; float voltAverageIn; float voltAverageRight; float voltAverageLeft;
float voltageInIncre; float voltageRightIncre; float voltageLeftIncre; float voltageHomeIncre;
int countVoltAverange; unsigned long tmrVoltAverange; unsigned long tmrMaxVoltAverange = 500;
  // END temporary solution for NOISE

byte stateHome;    // States 1 = Off, 2 = On, 4 Error
byte stateCHR;
byte stateRight;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
byte stateLeft;
float voltageHome; bool voltHomeOk; bool lastVoltHomeOk;
float voltageIn; bool voltInOk;
float voltageRight; bool voltRightOk; bool lastVoltRightOk;
float voltageLeft; bool voltLeftOk; bool lastVoltLeftOk;

//         END VARIABLES



void setup(){
  while (!Serial);
  Serial.begin(9600);
  rs485Serial.begin(57600);



  pinMode(signalOnPin, INPUT);
  pinMode(signalHomePin, INPUT_PULLUP);

  for (int i = 0; i < 7; i++)
  {
    pinMode(reles[i], OUTPUT), digitalWrite(reles[i], LOW);
  }

  // extra GNDs
  pinMode(48, OUTPUT), digitalWrite(48, LOW);
  pinMode(49, OUTPUT), digitalWrite(49, LOW);
  pinMode(50, OUTPUT), digitalWrite(50, LOW);
  pinMode(51, OUTPUT), digitalWrite(51, LOW);
  pinMode(52, OUTPUT), digitalWrite(52, LOW);
  pinMode(53, OUTPUT), digitalWrite(53, LOW);
  // extra VCCs
  pinMode(2, OUTPUT), digitalWrite(2, HIGH);
  pinMode(A7, OUTPUT), digitalWrite(A7, HIGH);

  getInitValues();

  tmrVoltAverange = millis();
  //leds.blinkOn();
}

void loop(){
  rs485SerialRead();

  if( inverterStatus ){ digitalWrite(ledInverter, HIGH); }
  else { digitalWrite(ledInverter, LOW); }

  getValues();
  getAmperProtect();

  signalOn = digitalRead(signalOnPin);
  signalHome = !digitalRead(signalHomePin);


  if( testVoltInOk && millis() > (tmrTestVoltInOk + tmrMaxTestVoltInOk) )
  {
    if( voltInOk == false )
    {
      sendRequestReleInverter(0);
      if( signalHome ){ lockHome = true; }
      digitalWrite(ledHome, LOW);
      digitalWrite(releHome, LOW);
      stateHome = 1;
      digitalWrite(ledRightOut, LOW);
      digitalWrite(releRightOut, LOW);
      signalRight = 0;
      stateRight = 1;
      digitalWrite(ledLeftOut, LOW);
      digitalWrite(releLeftOut, LOW);
      signalLeft = 0;
      stateLeft = 1;
      testVoltInOk = false;
    }
  }

  if (signalOn != lastSignalOn)
  {
    if(signalOn){
      //leds.blinkOn();
      digitalWrite(ledAutomatic, HIGH);
    }else{
      for (int i = 0; i < 7; i++)
      {
        digitalWrite(reles[i], LOW);
      }
      leds.blinkOff();
    }
    lastSignalOn = signalOn;
  }

  if(signalOn){
    if( voltRightOk != lastVoltRightOk )
    {
      if( voltRightOk && !digitalRead(releRightOut) )
      {
        rightOn();
      }
      if( !voltRightOk )
      {
        rightOff();
      }
    }
    if( voltLeftOk != lastVoltLeftOk )
    {
      if( voltLeftOk && !digitalRead(releLeftOut) )
      {
        leftOn();
      }
      if( !voltLeftOk )
      {
        leftOff();
      }
    }

    if( signalHome != lastSignalHome )
    {
      if (signalHome && !voltHomeOk )
      {
        if( voltInOk && !lockHome )
        {
          digitalWrite(ledHome, HIGH);
          digitalWrite(releHome, HIGH);
          stateHome = 2;
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
        if( !voltInOk && !lockHome )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledHome, HIGH);
          digitalWrite(releHome, HIGH);
          stateHome = 2;
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
      }
      if( signalHome&& voltHomeOk )
      {
        stateHome = 4;
      }
      if( !signalHome )
      {
        if( inverterStatus ){ sendRequestReleInverter(0); }
        digitalWrite(ledHome, LOW);
        digitalWrite(releHome, LOW);
        stateHome = 1;
        lockHome = false;
      }
      lastSignalHome = signalHome;
    }

    if( signalRight != lastSignalRight )
    {
      if( signalRight && !voltRightOk )
      {
        if( voltInOk )
        {
          digitalWrite(ledRightOut, HIGH);
          digitalWrite(releRightOut, HIGH);
          stateRight = 3;
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
        if( !voltInOk )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledRightOut, HIGH);
          digitalWrite(releRightOut, HIGH);
          stateRight = 3;
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
      }
      if( signalRight && voltRightOk )
      {
        stateRight = 3;   // Stude
      }
      if( !signalRight )
      {
        if( inverterStatus ){ sendRequestReleInverter(0); }
        digitalWrite(ledRightOut, LOW);
        digitalWrite(releRightOut, LOW);
        stateRight = 1;
      }
      lastSignalRight = signalRight;
    }

    if( signalLeft != lastSignalLeft )
    {
      if( signalLeft && !voltLeftOk )
      {
        if( voltInOk )
        {
          digitalWrite(ledLeftOut, HIGH);
          digitalWrite(releLeftOut, HIGH);
          stateLeft = 3;
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
        if( !voltInOk )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledLeftOut, HIGH);
          digitalWrite(releLeftOut, HIGH);
          stateLeft = 3;
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
      }
      if( signalLeft && voltLeftOk )
      {
        stateLeft = 3;   // Stude
      }
      if( !signalLeft )
      {
        if( inverterStatus ){ sendRequestReleInverter(0); }
        digitalWrite(ledLeftOut, LOW);
        digitalWrite(releLeftOut, LOW);
        stateLeft = 1;
      }
      lastSignalLeft = signalLeft;
    }
  }

    

     
//Serial.println("-------------------------------");
//Serial.print("signalHome = "); Serial.println(signalHome);
//Serial.print("voltInOk  = "); Serial.println(voltInOk);
//Serial.println(voltageIn);
//Serial.print("inverterStatus = "); Serial.println(inverterStatus);
//delay(1000);

}





void getInitValues() {
  lastSignalOn = !digitalRead(signalOnPin);
  lastSignalHome = digitalRead(signalHomePin);

  voltmeterHome.get();
  voltageHome = voltmeterHome.getVoltage();
  if(voltageHome > voltMin && voltageHome < voltMax){ voltHomeOk = true, lastVoltHomeOk = false; }
  else{ voltHomeOk = false, lastVoltHomeOk = true; }
//  byteVoltageHome = voltageHome;

  voltmeterIn.get();
  voltageIn = voltmeterIn.getVoltage();
  if(voltageIn > voltMin && voltageRight < voltMax){ voltInOk = true; } else { voltInOk = false; }
//  byteVoltageIn = voltageIn;

  voltmeterRight.get();
  voltageRight = voltmeterRight.getVoltage();
  if(voltageRight > voltMin && voltageRight < voltMax){ voltRightOk = true, lastVoltRightOk = false; }
  else{ voltRightOk = false, lastVoltRightOk = true; }
//  byteVoltageRight = voltageRight;

  voltmeterLeft.get();
  voltageLeft = voltmeterLeft.getVoltage();
  if(voltageLeft > voltMin && voltageLeft < voltMax){ voltLeftOk = true, lastVoltLeftOk = false; }
  else{ voltLeftOk = false, lastVoltLeftOk = true; }

// if( voltLeftOk != lastVoltLeftOk ) nextionRequestSend();
  
//  byteVoltageLeft = voltageLeft;

  ammeter.get();
  slowAmper = ammeter.getAmperSlow();
  if (slowAmper < amperMax) { amperOk = true; }
  else { amperOk = false; }
//  byteSlowAmper = slowAmper;

  countVoltAverange++;

  
}

void getValues() {
  voltmeterIn.get();
  voltageIn = voltmeterIn.getVoltage();
  //  byteVoltageIn = voltageIn;
  //if(voltageIn < 15 ){ voltageIn = 0; }   // Detectado ruido del umbral bajo > 60
  if( voltageIn > voltMax )
  {
    overVolt = true;
    if( voltageIn > overVoltValue )
    {
      overVoltValue = voltageIn;
    }
  } else {
    overVolt = false;
    overVoltValue = 0;
  }

  voltmeterRight.get();
  voltageRight = voltmeterRight.getVoltage();
  //  byteVoltageRight = voltageRight;
  //  if(voltageRight < 15 ){ voltageRight = 0; }
  if( voltageRight > voltMax )
  {
    overVolt = true;
    if( voltageRight > overVoltValue )
    {
      overVoltValue = voltageRight;
    }
  } else {
    overVolt = false;
  }

  voltmeterLeft.get();
  voltageLeft = voltmeterLeft.getVoltage();
  //  byteVoltageLeft = voltageLeft;
  //if(voltageLeft < 15 ){ voltageLeft = 0; }
  if( voltageLeft > voltMax )
  {
    overVolt = true;
    if( voltageLeft > overVoltValue )
    {
      overVoltValue = voltageLeft;
    }
  } else {
    overVolt = false;
  }

  voltmeterHome.get();
  voltageHome = voltmeterHome.getVoltage();
  //if(voltageHome < 15 ){ voltageHome = 0; }
  //  byteVoltageHome = voltageHome;
  if( voltageHome > voltMax )
  {
    overVolt = true;
    if( voltageHome > overVoltValue )
    {
      overVoltValue = voltageHome;
    }
  } else {
    overVolt = false;
  }

  voltageInIncre += voltageIn;
  voltageRightIncre += voltageRight;
  voltageLeftIncre += voltageLeft;
  voltageHomeIncre += voltageHome;
  countVoltAverange++;
  if( millis() > (tmrVoltAverange + tmrMaxVoltAverange) )
  {
    voltAverageIn = voltageInIncre / countVoltAverange;
    voltageInIncre = 0;
    voltAverageRight = voltageRightIncre / countVoltAverange;
    voltageRightIncre = 0;
    voltAverageLeft = voltageLeftIncre / countVoltAverange;
    voltageLeftIncre = 0;
    voltAverageHome = voltageHomeIncre / countVoltAverange;
    voltageHomeIncre = 0;
    tmrVoltAverange = millis();
  }

  if( voltProtect && millis() > (tmrVoltProtect + tmrVoltUnprotect) ){ voltProtect = false; }

  if( !voltProtect )
  {
    if( voltageIn > voltMin && voltageIn < voltMax ){ voltInOk = true; }
    else { voltInOk = false; voltProtect = true; tmrVoltProtect = millis(); }

    if( voltageRight > voltMin && voltageRight < voltMax ){ voltRightOk = true; }
    else { voltRightOk = false; voltProtect = true; tmrVoltProtect = millis(); }

    if( voltageLeft > voltMin && voltageLeft < voltMax ){ voltLeftOk = true; }
    else { voltLeftOk = false; voltProtect = true; tmrVoltProtect = millis(); }

    if( voltageHome > voltMin && voltageHome < voltMax ){ voltHomeOk = true; }
    else { voltHomeOk = false; voltProtect = true; tmrVoltProtect = millis(); }
  }

}

void getAmperProtect() {
  
  if( amperProtect && millis() > (tmrAmperProtect + tmrAmperUnprotect) ){ amperProtect = false; }
  if( voltInOk )
  {
    ammeter.get();
    slowAmper = ammeter.getAmperSlow();
    //  byteSlowAmper = slowAmper;  
    if( !amperProtect )
    {
      if (slowAmper < amperMax) { amperOk = true; }
      else { amperOk = false; amperProtect = true; tmrAmperProtect = millis(); }
    }
  } else {
    slowAmper=0;
  }
  if (slowAmper > amperMax)
  {
    overAmper = true;
    if( slowAmper > overAmperValue )
    {
      overAmperValue = slowAmper;
    }
  } else {
    overAmper = false;
  }
}

void rightOn() {
  if( !voltInOk )
  {
    digitalWrite(ledRightIn, HIGH);
    digitalWrite(releRightIn, HIGH);
    stateRight = 2;
    digitalWrite(ledCHR, HIGH);
    digitalWrite(releCHR, HIGH);
    stateCHR = 2;
    lastVoltRightOk = voltRightOk;
  }
  if( voltInOk )
  {
    if( voltLeftOk )
    {
      digitalWrite(ledRightIn, HIGH); // BLINK
      stateRight = 4;
    }
    if( !voltLeftOk )
    {
      if( inverterStatus )
      {
        sendRequestReleInverter(0);
        digitalWrite(ledRightIn, HIGH); // BLINK
        stateRight = 4;
      }
      if( !inverterStatus )
      {
        digitalWrite(ledRightIn, HIGH); // BLINK
        digitalWrite(ledRightOut, HIGH); // BLINK
        stateRight = 4;
      }
    }
  }
}

void rightOff() {
  signalRight = 0;
  digitalWrite(ledRightIn, LOW);
  digitalWrite(releRightIn, LOW);
  stateRight = 1;
  digitalWrite(ledCHR, LOW);
  digitalWrite(releCHR, LOW);
  stateCHR = 1;
  lastVoltRightOk = voltRightOk;
}

void leftOn() {
  if( !voltInOk )
  {
    digitalWrite(ledLeftIn, HIGH);
    digitalWrite(releLeftIn, HIGH);
    stateLeft = 2;
    digitalWrite(ledCHR, HIGH);
    stateCHR = 2;
    lastVoltLeftOk = voltLeftOk;
  }
  if( voltInOk )
  {
    if( voltRightOk )
    {
      digitalWrite(ledLeftIn, HIGH); // BLINK
      stateLeft = 4;
    }
    if( !voltRightOk )
    {
      if( inverterStatus )
      {
        sendRequestReleInverter(0);
        digitalWrite(ledLeftIn, HIGH); // BLINK
        stateLeft = 4;
      }
      if( !inverterStatus )
      {
        digitalWrite(ledLeftIn, HIGH); // BLINK
        digitalWrite(ledLeftOut, HIGH); // BLINK
        stateLeft = 4;
      }
    }
  }
}
void leftOff() {
  signalLeft = 0;
  digitalWrite(ledLeftIn, LOW);
  digitalWrite(releLeftIn, LOW);
  stateLeft = 1;
  digitalWrite(ledCHR, LOW);
  digitalWrite(releCHR, LOW);
  stateCHR = 1;
  lastVoltLeftOk = voltLeftOk;
}

void rs485SerialRead() {
/*
 * <init>
 * Heade - Indicate start of transmission
 * <check>
 * Indicate number of byte that remains to be sent
 * <DIR>
 * Indicate the send address G=General Relays S=Leds Salon N=Nextion
 * <CMD>
 * Indicate the specific command to execute
 * 
 * /////   COMANDS   /////////
 * 
 * Number relay for change state
 * <init> <check> <DIR> <CMD> <Pin Vcc>
 *    #      3      V    R
 *  0x23    0x03   0x47 0x52
 * 
  */
  if(rs485Serial.available()>2)
  {
    delay(5);
//Serial.println("rs485 available");
    char initData = rs485Serial.read();
// Check the start of the data string
    if( initData=='#' )
    {
      uint8_t checkData = rs485Serial.read();
Serial.print("checkData = "); Serial.println(checkData);
      unsigned long tmr1 = millis();
      bool minLength = true;
  // Verify that the entire data string has been received
      while(rs485Serial.available()<checkData)
      {
        if((millis()-tmr1)>100)
        {
          minLength=false;
          break;
        }
      }
// Verify that the entire data string that has been received does not exceed the expected
      if( minLength==true  && rs485Serial.available()==checkData ) //  Demasiado ruido
      {
        char dir = rs485Serial.read();
  // DIR only this arduino
Serial.print("dir = "); Serial.println(dir);
        if(dir == 'V')
        {
          char cmd = rs485Serial.read();
Serial.print("cmd = "); Serial.println(cmd);
          switch(cmd)
          {
  // Reles Externos
            case 'R':
              byte pinVcc = rs485Serial.read();
Serial.print("pinVcc = "); Serial.println(pinVcc);
              if( pinVcc == releRightOut_physicalPin )
              {
                signalRight = !lastSignalRight;
              }
              if( pinVcc == releLeftOut_physicalPin )
              {
                signalLeft = !lastSignalLeft;
              }
            break;
  // SYNQ
            case 'Q':
              char nameObject = rs485Serial.read();
Serial.print("nameObject = "); Serial.println(nameObject);
              if(nameObject == 'I')
              {
                inverterStatus = rs485Serial.read();
                digitalWrite(ledInverter, inverterStatus);
              }
              if(nameObject == 'M')
              {
                NextionSYNQ(0);
              }
            break;
          }
        }

// Posible defecto del arduino
// Eliminar al cambiar el arduino
// sustituye a ( # 3 V Q M )
#ifdef _chgArduino_

        if(dir == 'W')
        {
          NextionSYNQ(0); // Page Main
        }
        if(dir == 'Y')
        {
          NextionSYNQ(1); // Page Hight Voltage
        }
#endif
      }
    }
  }
}

void NextionSYNQ(byte a) {
  int sendRight;
  int sendLeft;
  int sendIn;
  int sendHome;
  int sendOverVoltValue;
  int sendAmper;
  int sendOverAmperValue;
  
  switch(a)
  {
  // synq PageMain
    case 0:
      FF();
Serial.print("NextionSYNQ = "); Serial.println(a);
      rs485Serial.print("pHighVoltage.autoStat220.val=" + String(signalOn)); FF();

// States 1 = Off, 2 = On or IN, 3 = OUT, 4 Error   (Numbers Pic)
      rs485Serial.print("pHighVoltage.homeStat220.val=" + String(stateHome)); FF();
      rs485Serial.print("pHighVoltage.RightStat220.val=" + String(stateRight)); FF();
      rs485Serial.print("pHighVoltage.LeftStat220.val=" + String(stateLeft)); FF();
      rs485Serial.print("pHighVoltage.ChrStat220.val=" + String(stateCHR)); FF();

      sendIn = voltAverageIn*100; // sendIn = voltageIn*100;
      rs485Serial.print("pHighVoltage.xIn.val=" + String(sendIn)); FF();

      if( overVolt )
      {
        sendOverVoltValue = overVoltValue*100;
        rs485Serial.print("pHighVoltage.xOverVolt.val=" + String(sendOverVoltValue)); FF();
        rs485Serial.print("pageMain.tVolt.bco=RED"); FF();
      }else{
        rs485Serial.print("pageMain.tVolt.bco=17456"); FF();
      }

      if( voltInOk )
      {
        sendAmper = slowAmper*100;
        rs485Serial.print("pHighVoltage.xAmp.val=" + String(sendAmper)); FF();
      }else{
        rs485Serial.print("pHighVoltage.xAmp.val=0"); FF();
      }

      if( overAmper )
      {
        sendOverAmperValue = overAmperValue*100;
        rs485Serial.print("pHighVoltage.xOverAmp.val=" + String(sendOverAmperValue)); FF();
        rs485Serial.print("pageMain.tAmp.bco=RED"); FF();
      }else{
        rs485Serial.print("pageMain.tAmp.bco=17456"); FF();
      }

      rs485Serial.print("click EndSynqHvoltag,1"); FF();
    break;
    
  // synq pHightVolt
    case 1:
      FF();
Serial.print("NextionSYNQ = "); Serial.println(a);
      rs485Serial.print("pHighVoltage.autoStat220.val=" + String(signalOn)); FF();

// States 1 = Off, 2 = On or IN, 3 = OUT, 4 Error   (Numbers Pic)
      rs485Serial.print("pHighVoltage.homeStat220.val=" + String(stateHome)); FF();
      rs485Serial.print("pHighVoltage.RightStat220.val=" + String(stateRight)); FF();
      rs485Serial.print("pHighVoltage.LeftStat220.val=" + String(stateLeft)); FF();
      rs485Serial.print("pHighVoltage.ChrStat220.val=" + String(stateCHR)); FF();

      sendRight = voltAverageRight*100; // sendRight = voltageRight*100;
      rs485Serial.print("pHighVoltage.xRight.val=" + String(sendRight)); FF();

      sendLeft = voltAverageLeft*100; // sendLeft = voltageLeft*100;
      rs485Serial.print("pHighVoltage.xLeft.val=" + String(sendLeft)); FF();

      sendIn = voltAverageIn*100; // sendIn = voltageIn*100;
      rs485Serial.print("pHighVoltage.xIn.val=" + String(sendIn)); FF();

      sendHome = voltAverageHome*100; // sendHome = voltageHome*100;
      rs485Serial.print("pHighVoltage.xHome.val=" + String(sendHome)); FF();

      if( overVolt )
      {
        sendOverVoltValue = overVoltValue*100;
        rs485Serial.print("pHighVoltage.xOverVolt.val=" + String(sendOverVoltValue)); FF();
      }

      if( voltInOk )
      {
        sendAmper = slowAmper*100;
        rs485Serial.print("pageMain.xAmper.val=" + String(sendAmper)); FF();
      }else{
        rs485Serial.print("pageMain.xAmper.val=0"); FF();
      }

      if( overAmper )
      {
        sendOverAmperValue = overAmperValue*100;
        rs485Serial.print("pageMain.xOverTens.val=" + String(sendOverAmperValue)); FF();
      }

      rs485Serial.print("pHighVoltage.InverterStat.val=" + String(inverterStatus)); FF();

      rs485Serial.print("click EndSynqHvoltag,1"); FF();
    break;
  }
}

void sendRequestReleInverter(bool _request) {
  delay(1);
  byte bufSend[] = { '#', 4, 'T', 'R', 'I', _request };
  rs485Serial.write(bufSend, 6);
}

void FF(){
  rs485Serial.print("\xFF\xFF\xFF");
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
