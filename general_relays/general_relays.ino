 /*
 * State testing, developer c2mismo 2019.
 * License GNU, see at the end.
 */

//           LIBRARY CONF


#include <Arduino.h>
#include <Servo.h>

Servo escSalon;

const byte escSalonPin = 6;
byte fanPin;
const byte fanSalonPin = 46;
const int fanSalonActiv = 1510;
const int fanSalonStarter = 1575;
const int fanSalonStarterDelay = 1000;
const int fanSalonMin = 1559;
const int fanSalonMax = 1615;
int fanSalonState;
int fanSalonValueRaw;
int fanValue;

#define Nextion Serial3

// Send reset count for tmSYNQtimeOut (vaSYNQtimeOut)
// this count val 250 in 8 times
const int NextionSYNQ_TIME = 500;
unsigned long NextionSYNQtime;

const byte relesPin  [32] {  5,   4,   3,   2,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49};
//                        {101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216};

const byte ledPin = 13;

//  TEMP
char option = ' ';

void setup() {
  while (!Serial);
  Serial.begin(9600);
  Nextion.begin(115200);
  escSalon.attach(escSalonPin);     // Init com

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(fanSalonPin, OUTPUT);
  digitalWrite(fanSalonPin, HIGH);

  for (byte i=0;i<32;i++)
  {
    pinMode(relesPin[i], OUTPUT);
  }
  for (byte i=0;i<32;i++)
  {
    digitalWrite(relesPin[i], HIGH);
  }
  NextionSYNQtime=millis();
Serial.println("TEST INIT");
}

void loop() {

  if((millis()-NextionSYNQtime) > NextionSYNQ_TIME)
  {
    NextionPing();
    NextionSYNQtime=millis();
  }
  NextionSerialRead();

  if(Serial.available() != 0)
  {
    option = Serial.read();
    if(option == 'a')
    {
      digitalWrite(25, !digitalRead(25));
    }
  }
}

void NextionSerialRead() {
/*
 * Number relay for change state
 * <ctrl> <len> <cmd> <NºReleay>
 *   #      2     R
 * printh 23 02 52
 * NºReleay
 * prints XX,1       XX= value 0 to 255
 * 
 * Control High voltage box
 * <ctrl> <len> <cmd> <values>
 *   #      2     H
 * printh 23 02 48
 * Values
 * prints XX,1   if( XX >= 0 && XX <= 20) Values of ammeter protect.
 * prints XX,1   if( XX >= 180 && XX <= 250) Values of voltmeter protect.
 * prints XX,1   if( XX == 30 ) Supply voltage to HOME.
 * prints XX,1   if( XX == 40 ) Supply voltage to out RIGHT.
 * prints XX,1   if( XX == 50 ) Supply voltage to out LEFT.
 * 
 * Control FANs
 * <ctrl> <len> <cmd> <state> <Pin Vdc> <value>
 *   #      3     F
 * printh 23 02 46 0X       0X = 00 OFF, 01 ON, 02 Value (now change state)
 * // Pin Vdc
 * prints XX,1           XX= Pin Vdc
 * Value
 * prints YY,1       YY= value 0 to 255
 * 
 * Control LEDs  ( IN CONSTRUCTION )
 * <ctrl> <len> <cmd> <Nº Led> <value>
 *   #      3     L      XX
 * printh 23 02 4C XX           XX= Nº of fan 
 * Value
 * prints YY,1       YY= value 0 to 255
 * 
 * Request for refresh all dates
 * <ctrl> <len> <cmd>
 *   #      1     Q
 * printh 23 01 51
 */
  if(Nextion.available()>2)
  {
    char start_char = Nextion.read();
    if(start_char=='#')
    {
      uint8_t len = Nextion.read();
      unsigned long tmr1 = millis();
      unsigned long tmr2;
      bool cmd_found = true;
      while(Nextion.available()<len)
      {
        if((millis()-tmr1)>100)
        {
          cmd_found=false;
          break;
        }
        delay(1);
      }
      if(cmd_found==true)
      {
        char cmd = Nextion.read();
        uint8_t pinRele;
        switch(cmd)
        {
// RELAYS
          case 'R':
            pinRele = Nextion.read();
            digitalWrite(pinRele, !digitalRead(pinRele));  // Hacer cambio de estado
            if(digitalRead(pinRele))
            {
              Nextion.print("pageMain.tPin" + String(pinRele) + ".picc=1"); FF();
            }
            if(!digitalRead(pinRele))
            {
              Nextion.print("pageMain.tPin"); Nextion.print(pinRele); Nextion.print(".picc=2"); FF();
            }
            break;
// High voltage
          case 'H':
            // if( XX >= 0 && XX <= 20) Values of ammeter protect.
            // if( XX >= 180 && XX <= 250) Values of voltmeter protect.
            // if( XX == 30 ) Supply voltage to HOME.
            // if( XX == 40 ) Supply voltage to out RIGHT.
            // if( XX == 50 ) Supply voltage to out LEFT.
            break;
// Fan
          case 'F':
            fanSalonState = Nextion.read();  // State = 00 OFF, 01 ON, 02 Value (now change state)
            switch(fanSalonState)
            {
              case 0:
                fanPin = Nextion.read();  // Pin Vdc esc
                if(fanPin==fanSalonPin)
                {
                  fanSalonValueRaw=1;
                  digitalWrite(fanSalonPin, HIGH); // Low trigger  OFF
                  Nextion.print("pageMain.tPin" + String(fanSalonPin) + ".picc=1"); FF();
                  Nextion.print("pageMain.jPin" + String(fanSalonPin) + ".val=0"); FF();
                  Nextion.print("pageCmdFan.t2.txt=\"OFF\""); FF();
                  Nextion.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
                  Nextion.print("pageCmdFan.n0.val=0"); FF();
                  Nextion.print("pageCmdFan.t2.bco=17456"); FF();
                }
                break;
              case 1:
Serial.println("ON");
                fanPin = Nextion.read();  // Pin Vdc esc
                if(fanPin==fanSalonPin)
                {
                  fanSalonValueRaw=1;
                  digitalWrite(fanSalonPin, LOW); // Low trigger   ON
                  escSalon.writeMicroseconds(fanSalonActiv);  // Init esc
                  delay(1);
                  escSalon.writeMicroseconds(fanSalonStarter);
                  delay(fanSalonStarterDelay);   // Para hacer un "delay por millis" todo lo siguiente
                                                // debe estar fuera de el proceso de lectura
                  
                  escSalon.writeMicroseconds(fanSalonMin);
                  Nextion.print("pageMain.tPin" + String(fanSalonPin) + ".picc=2"); FF();
                  Nextion.print("pageMain.jPin" + String(fanSalonPin) + ".val=" + String(fanSalonValueRaw)); FF();
                  Nextion.print("pageCmdFan.t2.txt=\"ON\""); FF();
                  Nextion.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
                  Nextion.print("pageCmdFan.n0.val=" + String(fanSalonValueRaw)); FF();
                  Nextion.print("pageCmdFan.t2.bco=17456"); FF();
Serial.print("value = ");
Serial.println(fanSalonMin);
                }
                break;
              case 2:
Serial.println("Value");
                fanPin = Nextion.read();  // Pin Vdc esc
                if(fanPin==fanSalonPin)
                {
                  fanSalonValueRaw = Nextion.read();  // Value RAW Signal esc
                  fanValue = map(fanSalonValueRaw, 0, 100, fanSalonMin, fanSalonMax);  // Value Signal esc
                  escSalon.writeMicroseconds(fanValue);
Serial.print("value = ");
Serial.println(fanValue);
                }
                break;
            }
            break;
// NextionSYNQ
          case 'Q':
            NextionSYNQ();
            break;
        }
      }
    }
  }
}

void NextionSYNQ() {
// RELAY's
  for (byte i=0;i<32;i++)
  {
    if(!digitalRead(relesPin[i]))  // (ON) Trigger Low Level
    {
      Nextion.print("pageMain.tPin" + String(relesPin[i]) + ".picc=2"); FF();
    } else if(digitalRead(relesPin[i]))   // (OFF)
    {
      Nextion.print("pageMain.tPin" + String(relesPin[i]) + ".picc=1"); FF();
    }
  }
  if(fanSalonState==2)
  { // Value
    Nextion.print("pageMain.tPin" + String(fanSalonPin) + ".picc=2"); FF();
    Nextion.print("pageMain.jPin" + String(fanSalonPin) + ".val=" + String(fanSalonValueRaw)); FF();
    Nextion.print("pageCmdFan.t2.txt=\"ON\""); FF();
    Nextion.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
    Nextion.print("pageCmdFan.t2.bco=17456"); FF();
  }
  if(fanSalonState==1)
  { // ON
    Nextion.print("pageMain.tPin" + String(fanSalonPin) + ".picc=2"); FF();
    Nextion.print("pageMain.jPin" + String(fanSalonPin) + ".val=" + String(fanSalonValueRaw)); FF();
    Nextion.print("pageCmdFan.t2.txt=\"ON\""); FF();
    Nextion.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
    Nextion.print("pageCmdFan.t2.bco=17456"); FF();
  }
  if(fanSalonState==0)
  { // OFF
    Nextion.print("pageMain.tPin" + String(fanSalonPin) + ".picc=1"); FF();
    Nextion.print("pageMain.jPin" + String(fanSalonPin) + ".val=0"); FF();
    Nextion.print("pageCmdFan.t2.txt=\"OFF\""); FF();
    Nextion.print("pageCmdFan.h0.val=1"); FF();
    Nextion.print("pageCmdFan.n0.val=0"); FF();
    Nextion.print("pageCmdFan.t2.bco=17456"); FF();
  }
  NextionPing();
  Nextion.print("pageBlack.vaSYNQstate.val=1"); FF();
}

void NextionPing() {
  Nextion.print("pageBlack.vaSYNQtimeOut.val=0"); FF();
}

void FF(){
  Nextion.print("\xFF\xFF\xFF");
}

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
