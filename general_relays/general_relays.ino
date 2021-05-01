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
byte fanName;
byte fanSalonValueRaw;
int fanSalonValue;
const byte fanSalonPin = 46;
const byte fanSalonName = 0;    //  0=Salon
const int fanSalonActiv = 1510;
const int fanSalonStarter = 1575;
const int fanSalonStarterDelay = 1000;
const int fanSalonMin = 1559;
const int fanSalonMax = 1615;
byte fanSalonMode;        // Save data for synq

byte ledPin; 
const byte ledSalonPin = 22;


#define rs485Serial Serial3
byte nextionPing;

char page;
byte pinVcc;
byte nameObject;

//  Limitar a Relés en uso
const byte relesPin  [32] {  5,   4,   3,   2,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49};
//                        {101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216};
uint8_t relePin;



//  TEMP
char option = ' ';

void setup() {
  Serial.begin(9600);
  rs485Serial.begin(57600);
  escSalon.attach(escSalonPin);     // Init ESC


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
  Serial.println("TEST INIT");
  delay(20);
}

void loop() {
  rs485SerialRead();

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
 *    #      3      G    R
 *  0x23    0x03   0x47 0x52
 * printh 23 03 47 52
 * <Pin Vcc>
 * prints pageTeclaNum.vaRele.val,1
 * 
 * Control High voltage box    ( In construction )
 * <init> <check> <DIR> <CMD> <values>
 *   #       3      G     H
 *  0x23   0x03   0x47  0x48
 * printh 23 03 47 48
 * Values
 * prints XX,1   if( XX >= 0 && XX <= 20) Values of ammeter protect.
 * prints XX,1   if( XX >= 180 && XX <= 250) Values of voltmeter protect.
 * prints XX,1   if( XX == 30 ) Supply voltage to HOME.
 * prints XX,1   if( XX == 40 ) Supply voltage to out RIGHT.
 * prints XX,1   if( XX == 50 ) Supply voltage to out LEFT.
 * 
 * Control FANs
 * <init> <check> <DIR> <CMD> <Pin Vdc> <name> <mode> <value>
 *   #       5      G     F
 *  0x23   0x05   0x47  0x46
 * printh 23 05 47 46
 * <Pin Vdc>
 * prints pageCmdFan.vaPinVccFan.val,1
 * <name>
 * prints pageCmdFan.vaNameFan.val,1      0=Salon
 * <mode>
 * printh NN         NN = 00 OFF, 01 ON, 02 Value (know change state)
 * <value>
 * h0.val,1          YY= value 0 to 255
 * 
 * Control LEDs
 * <init> <check> <DIR> <CMD> <pinVcc> <type> <from> <to> <value>
 *   #       7      G     L           /////Cambiar G x S 0x53 claraboya Salón ////////
 *  0x23   0x07   0x47  0x4C          ////////////////////////////////////////////////
 * printh 23 07 47 4C
 * <pinVcc>         0=Salón 1=Pasillo
 * prints pageCmdLed.vaPinVccClara.val,1
 * <type>         0=OFF 1=ON 2=HUE 3=SAT 4=VAL or >10=Predefinied Modes
 * <from>         First led selection
 * prints pageClaraboya.nFrom.val,1
 * <to>           Last led selection
 * prints pageClaraboya.nTo.val,1     XX= value 0 to 129
 * <value>
 * prints pageClaraboya.nXXX.val,1   XXX= HUE, SAT or VAL. value 0 to 255
 * 
 * Request for refresh all dates
 * <init> <check> <DIR> <CMD> <PinVcc> <name>
 *   #       4      G     Q
 *  0x23   0x04   0x47  0x51
 * printh 23 04 47 51
 * <PinVcc>       PinVcc control rele for line, except 255=pageMain
 * <name>         Equipment connected to the line
 * prints pageMain.vaPinRele.val,1
 * or
 * prints pageCmdFan.vaPinVccFan.val,1
 * or
 * prints pageCmdLed.vaPinVccLed.val,1
 * 
 */
  if(rs485Serial.available()>2)
  {
    char initData = rs485Serial.read();
// Check the start of the data string
    if(initData=='#')
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
      if(minLength==true && rs485Serial.available()==checkData)  
      {
        char dir = rs485Serial.read();
  // DIR only this arduino
Serial.print("dir = "); Serial.println(dir);
        if(dir == 'G')
        {
          char cmd = rs485Serial.read();
Serial.print("cmd = "); Serial.println(cmd);

          switch(cmd)
          {
  // RELAYS
            case 'R':
              relePin = rs485Serial.read();
              nameObject = rs485Serial.read();
              digitalWrite(relePin, !digitalRead(relePin));  // Hacer cambio de estado
              if(digitalRead(relePin))
              {
                //delay(1);
                FF(); rs485Serial.print("pageMain.tPin" + String(relePin) + ".picc=1"); FF();
              }
              if(!digitalRead(relePin))
              {
                delay(1);
                FF(); rs485Serial.print("pageMain.tPin" + String(relePin) + ".picc=2"); FF();
              }
              if(relePin==ledSalonPin)
              {
                sendStatReleLedSalon();
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
              fanPin = rs485Serial.read();  // Pin Vdc esc
              fanName = rs485Serial.read();  // Name
              fanSalonMode = rs485Serial.read();  // State = 00 OFF, 01 ON, 02 Value (now change state)
Serial.print("fanSalonMode = "); Serial.println(fanSalonMode);
              switch(fanSalonMode)
              {
                case 0: // Fan OFF
                  if(fanPin==fanSalonPin)
                  {
                    if(fanName==fanSalonName)
                    {
                      fanSalonValueRaw=1;
                      digitalWrite(fanSalonPin, HIGH); // Low trigger  OFF
                      /*
                      rs485Serial.print("pageMain.tPin" + String(fanSalonPin) + ".picc=1"); FF();
                      rs485Serial.print("pageMain.jPin" + String(fanSalonPin) + ".val=0"); FF();
                      rs485Serial.print("pageCmdFan.t2.txt=\"ON\""); FF();
                      rs485Serial.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
                      rs485Serial.print("pageCmdFan.n0.val=0"); FF();
                      rs485Serial.print("pageCmdFan.t2.pic=23"); FF();*/
                    }
                  }
                break;
                case 1: // Fan ON
  Serial.println("ON");
                  if(fanPin==fanSalonPin)
                  {
                    if(fanName==fanSalonName)
                    {
                      fanSalonValueRaw=1;
                      digitalWrite(fanSalonPin, LOW); // Low trigger   ON
                      escSalon.writeMicroseconds(fanSalonActiv);  // Init esc
                      delay(1);
                      escSalon.writeMicroseconds(fanSalonStarter);
                      delay(fanSalonStarterDelay);   // Para hacer un "delay por millis" todo lo siguiente
                                                    // debe estar fuera de el proceso de lectura
                      
                      escSalon.writeMicroseconds(fanSalonMin);
                      /*
                      rs485Serial.print("pageMain.tPin" + String(fanSalonPin) + ".picc=2"); FF();
                      rs485Serial.print("pageMain.jPin" + String(fanSalonPin) + ".val=" + String(fanSalonValueRaw)); FF();
                      rs485Serial.print("pageCmdFan.t2.txt=\"OFF\""); FF();
                      rs485Serial.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
                      rs485Serial.print("pageCmdFan.n0.val=" + String(fanSalonValueRaw)); FF();
                      rs485Serial.print("pageCmdFan.t2.pic=23"); FF();*/
                    }
                  }
                break;
                case 2: // Fan VALUE
                  if(fanPin==fanSalonPin)
                  {
                    if(fanName==fanSalonName)
                    {
                      fanSalonValueRaw = rs485Serial.read();  // Value RAW Signal esc
                      fanSalonValue = map(fanSalonValueRaw, 0, 100, fanSalonMin, fanSalonMax);  // Value Signal esc
                      escSalon.writeMicroseconds(fanSalonValue);
                    }
Serial.print("fanSalonValue = ");Serial.println(fanSalonValue);
                  }
                break;
              }
            break;
  // NextionSYNQ
            case 'Q':
              page = rs485Serial.read();  // page: N = pageMain F = Fans L = Leds
              switch(page)
              {
                case 'M':
  // synq PageMain
                  NextionSYNQ(0);
                break;
                case 'F':
  // synq Fan
                  pinVcc = rs485Serial.read();
                  switch(pinVcc)
                  {
                    case 46:
                      nameObject = rs485Serial.read();  // Name
                      switch(nameObject)
                      {
                        case 0:         //  Mesita
                          NextionSYNQ(1);
                        break;
                      }
                    break;
                  }
                break;
                case 'L':
  // synq Leds
                  relePin = rs485Serial.read();
Serial.print("relePin = "); Serial.println(String(relePin));
                  nameObject = rs485Serial.read();
Serial.print("nameObject = "); Serial.println(String(nameObject));
                  digitalWrite(relePin, HIGH);  // OFF
                    delay(1);
                    FF(); rs485Serial.print("pageMain.tPin" + String(relePin) + ".picc=1"); FF();
                  if(relePin==ledSalonPin)
                  {
                    sendStatReleLedSalon();
                  }
                break;
              }
            break;
          }
        }
      }
    }
  }
}


void sendStatReleLedSalon() {
  delay(1);
Serial.print("ledSalonPin = "); Serial.println(String(!digitalRead(ledSalonPin)));
  byte bufSend[] = { '#', 6, 'S', 'Q', 'G', relePin, nameObject, !digitalRead(ledSalonPin) };
  rs485Serial.write(bufSend, 8);
}

void NextionSYNQ(byte a) {
// RELAY's
  switch(a)
  {
  // synq PageMain
    case 0:
Serial.print("NextionSYNQ = "); Serial.println(a);
      for (byte i=0;i<32;i++)
      {
        if(!digitalRead(relesPin[i]))  // (ON) Trigger Low Level
        {
          rs485Serial.print("pageMain.tPin" + String(relesPin[i]) + ".picc=2"); FF();
        } else if(digitalRead(relesPin[i]))   // (OFF)
        {
          rs485Serial.print("pageMain.tPin" + String(relesPin[i]) + ".picc=1"); FF();
        }
      }
      if(!digitalRead(fanSalonPin))  // (ON) Trigger Low Level
      {
        rs485Serial.print("pageMain.jPin" + String(fanSalonPin) + ".val=" + String(fanSalonValueRaw)); FF();
      } else if(digitalRead(fanSalonPin))   // (OFF)
      {
        rs485Serial.print("pageMain.jPin" + String(fanSalonPin) + ".val=0"); FF();
      }
      rs485Serial.print("pageBlack.vaSYNQ.val=" + String(nextionPing++)); FF();
    break;
  // synq Fan Salon
    case 1:
      if(fanSalonMode==1 || fanSalonMode==2) // ON
      { // Value
        rs485Serial.print("pageCmdFan.t2.txt=\"ON\""); FF();
        rs485Serial.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
        rs485Serial.print("pageCmdFan.n0.val=pageCmdFan.h0.val"); FF();
        rs485Serial.print("pageCmdFan.t2.bco=17456"); FF();
      }
      if(fanSalonMode==0) // OFF
      { // OFF
        rs485Serial.print("pageCmdFan.t2.txt=\"OFF\""); FF();
        rs485Serial.print("pageCmdFan.h0.val=1"); FF();
        rs485Serial.print("pageCmdFan.n0.val=0"); FF();
        rs485Serial.print("pageCmdFan.t2.bco=17456"); FF();
      }
      rs485Serial.print("pageBlack.vaSYNQ.val=" + String(nextionPing++)); FF();
    break;
  }
}

void FF(){
  rs485Serial.print("\xFF\xFF\xFF");
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
