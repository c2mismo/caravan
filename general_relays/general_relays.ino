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
int fanValue;
const byte fanSalonPin = 46;
const int fanSalonActiv = 1510;
const int fanSalonStarter = 1575;
const int fanSalonStarterDelay = 1000;
const int fanSalonMin = 1559;
const int fanSalonMax = 1615;
byte fanSalonState;        // Save data for synq
byte fanSalonValueRaw;

#define ledSalon Serial2
byte ledPin;
const byte ledSalonPin = 39;  //////////////// Cambiar por 2 ////////


#define Nextion Serial3

// Send reset count for tmSYNQtimeOut (vaSYNQtimeOut)
// this count val 250 in 8 times
const int NextionSYNQ_TIME = 500;
unsigned long NextionSYNQtime;

const byte relesPin  [32] {  5,   4,   3,   2,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49};
//                        {101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216};
uint8_t relePin;



//  TEMP
char option = ' ';

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(100);
  Nextion.begin(115200);
  ledSalon.begin(9600);
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
  NextionSYNQtime=millis();
  Serial.println("TEST INIT");
  delay(20);
  NextionSYNQ('M');   // Sinq pageMain
  // NextionSYNQ('L');   // Pulsar botn synq
}

void loop() {
// Send a ping signal
  if((millis()-NextionSYNQtime) > NextionSYNQ_TIME)
  {
    NextionPing();
    NextionSYNQtime=millis();
  }
  NextionSerialRead();


/*
  dataLedSalon ++;
  int checkLedSalon=1;

  ledSalon.print("#");
  ledSalon.write(checkLedSalon);
  ledSalon.print("L");
  Serial.print("#" + String(checkLedSalon) + "L");
  ledSalon.flush();
  delay(1000);
  */
}

void ledSalonSerialRead() {   // Pendiente rs485SerialRead
  
  if(ledSalon.available()>2)
  {
    char initData = ledSalon.read();
// Check the start of the data string
    if(initData=='#')
    {
      uint8_t checkData = ledSalon.read();
      unsigned long tmr1 = millis();
      bool minLength = true;
      while(ledSalon.available()<checkData)
      {
        if((millis()-tmr1)>100)
        {
          minLength=false;
          break;
        }
      }
      delay(10);
      char dir;
      if(minLength==true && ledSalon.available()==checkData)  
      {
        
        dir = Nextion.read();
        if(dir == 'G')
        {
          char cmd = ledSalon.read();
          char cmd2;
          switch(cmd)
          {
// RELAYS            NO AÑADIR "DUPLICADO" de NextionSerialRead()
          case 'R':
            relePin = ledSalon.read();
            digitalWrite(relePin, !digitalRead(relePin));  // Hacer cambio de estado
            if(digitalRead(relePin))
            {
              Nextion.print("pageMain.tPin" + String(relePin) + ".picc=1"); FF();
            }
            if(!digitalRead(relePin))
            {
              Nextion.print("pageMain.tPin"); Nextion.print(relePin); Nextion.print(".picc=2"); FF();
            }
          break;
/*/ NextionSYNQ
          case 'Q':
            cmd2 = ledSalon.read();  // cmd2=M: pageMain cmd2=L: Leds
            NextionSYNQ(cmd2);
          break;*/
          }
        }
        dir = Nextion.read();
        if(dir == 'N')
        {
          //
        }
      }
    }
  }
}

void NextionSerialRead() {
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
 * prints pageCmdFan.vaNameFan.val,1      1=mesita
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
  if(Nextion.available()>2)
  {
    char initData = Nextion.read();
// Check the start of the data string
    if(initData=='#')
    {
      uint8_t checkData = Nextion.read();
Serial.print("checkData = "); Serial.println(checkData);
      unsigned long tmr1 = millis();
      bool minLength = true;
  // Verify that the entire data string has been received
      while(Nextion.available()<checkData)
      {
        if((millis()-tmr1)>100)
        {
          minLength=false;
          break;
        }
      }
      delay(1);
// Verify that the entire data string that has been received does not exceed the expected
      if(minLength==true && Nextion.available()==checkData)  
      {
        char dir = Nextion.read();
Serial.print("dir = "); Serial.println(dir);
        if(dir == 'G')
        {
          char cmd = Nextion.read();
Serial.print("cmd = "); Serial.println(cmd);
          char cmd2;
          switch(cmd)
          {
  // RELAYS
            case 'R':
              relePin = Nextion.read();
              digitalWrite(relePin, !digitalRead(relePin));  // Hacer cambio de estado
             /* if(digitalRead(relePin))
              {
                Nextion.print("pageMain.tPin" + String(relePin) + ".picc=1"); FF();
              }
              if(!digitalRead(relePin))
              {
                Nextion.print("pageMain.tPin"); Nextion.print(relePin); Nextion.print(".picc=2"); FF();
              }*/
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
                    /*
                    Nextion.print("pageMain.tPin" + String(fanSalonPin) + ".picc=1"); FF();
                    Nextion.print("pageMain.jPin" + String(fanSalonPin) + ".val=0"); FF();
                    Nextion.print("pageCmdFan.t2.txt=\"OFF\""); FF();
                    Nextion.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
                    Nextion.print("pageCmdFan.n0.val=0"); FF();
                    Nextion.print("pageCmdFan.t2.bco=17456"); FF();*/
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
                    /*
                    Nextion.print("pageMain.tPin" + String(fanSalonPin) + ".picc=2"); FF();
                    Nextion.print("pageMain.jPin" + String(fanSalonPin) + ".val=" + String(fanSalonValueRaw)); FF();
                    Nextion.print("pageCmdFan.t2.txt=\"ON\""); FF();
                    Nextion.print("pageCmdFan.h0.val=" + String(fanSalonValueRaw)); FF();
                    Nextion.print("pageCmdFan.n0.val=" + String(fanSalonValueRaw)); FF();
                    Nextion.print("pageCmdFan.t2.bco=17456"); FF();*/
                  }
                break;
                case 2:
                  fanPin = Nextion.read();  // Pin Vdc esc
                  if(fanPin==fanSalonPin)
                  {
                    fanSalonValueRaw = Nextion.read();  // Value RAW Signal esc
                    fanValue = map(fanSalonValueRaw, 0, 100, fanSalonMin, fanSalonMax);  // Value Signal esc
                    escSalon.writeMicroseconds(fanValue);
                  }
                break;
              }
            break;
  // Leds             FUNDIR con ledSalonSerialRead() PARA rs485SerialRead()
  //                   ONLY repetidor <DIR> S=Leds Salon   (cambiar al Nextion)
            case 'L':
              //
            break;
  // NextionSYNQ
            case 'Q':
              cmd2 = Nextion.read();  // cmd2=M: pageMain cmd2=L: Leds
              NextionSYNQ(cmd2);
            break;
          }
        }
        if(dir == 'S') //  FUNDIR con ledSalonSerialRead() PARA rs485SerialRead()
        {              //  Repeat signal
          //
        }
      }
    }
  }
}

void NextionSYNQ(char page) {
// RELAY's
  switch(page)
  {
    case 'M':
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
    break;
    case 'L':
      ledPin = Nextion.read(); 
      if(ledPin==ledSalonPin)
      {
        ledSalon.print("#");
        ledSalon.write(0x02);
        ledSalon.print("Q");
        ledSalon.write(ledPin);
        ledSalon.flush();
        delay(1000);
      }
    break;
  }
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
