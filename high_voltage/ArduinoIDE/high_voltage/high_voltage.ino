//#define _chgArduino_

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

#define  _DEBUG_

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
bool lastInverterStatus = true;

int freq = 22;      // 50 Hz = 20 m/s; 60 Hz = 16.6667 m/s
int samples = 10;
Voltmeter voltmeterHome (voltmeterHomePin, freq, samples);
Voltmeter voltmeterIn (voltmeterInPin, freq, samples);
Voltmeter voltmeterRight (voltmeterRightPin, freq, samples);
Voltmeter voltmeterLeft (voltmeterLeftPin, freq, samples);

float voltMin = 190;
float voltMax = 236;
bool overVolt = false;
bool checkOverVolt = false;
float overVoltValue;
unsigned int tmrOverVoltProtect;
unsigned int tmrOverVoltUnprotect = 5000;
bool testVoltInOk = false;
unsigned long tmrTestVoltInOk;
unsigned long tmrMaxTestVoltInOk = 1000;    //  timer for init test voltageIn

byte stateHome;    // States 1 = Off, 2 = On, 4 Error
byte stateCHR;
byte stateRight;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
byte stateLeft;
float voltageHome; bool voltHomeOk;
float voltageIn; bool voltInOk;
float voltageRight; bool voltRightOk; bool lastVoltRightOk;
float voltageLeft; bool voltLeftOk; bool lastVoltLeftOk;

float calibAmper = 18.95;  // inverso para subir resultado bajar valor
int intervalRead = 250;
float slowAmper;
float amperMax = 5;
bool amperOk = false;
bool amperProtect = true;
unsigned int tmrAmperProtect;
unsigned int tmrAmperUnprotect = 8000;
Ammeter ammeter (ammeterPin, freq, calibAmper, intervalRead, amperMax);
bool overAmper = false;
float overAmperValue;

//         END VARIABLES


void setup(){
  #ifdef _DEBUG_
    while (!Serial);
    Serial.begin(9600);
  #endif
  rs485Serial.begin(57600);
  rs485Serial.setTimeout(1);


  pinMode(signalOnPin, INPUT);
  pinMode(signalHomePin, INPUT_PULLUP);

  for (int i = 0; i < 7; i++)
  {
    pinMode(reles[i], OUTPUT), digitalWrite(reles[i], LOW);
  }

  // extra GNDs
  pinMode(A8, OUTPUT), digitalWrite(A8, LOW);
  pinMode(A9, OUTPUT), digitalWrite(A9, LOW);
  pinMode(A14, OUTPUT), digitalWrite(A14, LOW);
  pinMode(48, OUTPUT), digitalWrite(48, LOW);
  pinMode(49, OUTPUT), digitalWrite(49, LOW);
  pinMode(50, OUTPUT), digitalWrite(50, LOW);
  pinMode(51, OUTPUT), digitalWrite(51, LOW);
  pinMode(52, OUTPUT), digitalWrite(52, LOW);
  //pinMode(53, OUTPUT), digitalWrite(53, LOW);
  // extra VCCs
  pinMode(2, OUTPUT), digitalWrite(2, HIGH);
  pinMode(A7, OUTPUT), digitalWrite(A7, HIGH);

  sendRequestReleInverter(0);

  getInitValues();

  tmrAmperProtect = millis();
  //leds.blinkOn();
}

void loop(){

Serial.println("-------------------------------");
Serial.print("voltInOk = "); Serial.print(voltInOk);
Serial.print("  ledRightIn  = "); Serial.print(digitalRead(releRightIn));
//Serial.println(amperProtect);
Serial.print("  ledCHR = "); Serial.println(digitalRead(releCHR));
//delay(1000);
  signalOn = digitalRead(signalOnPin);
  signalHome = !digitalRead(signalHomePin);

  if( testVoltInOk ) testVoltIn();
  getAmperProtect();

  if( inverterStatus != lastInverterStatus )
  {
    digitalWrite(ledInverter, inverterStatus);
    if( inverterStatus ){
      testVoltInOk = true;
      tmrTestVoltInOk = millis();
    }
    lastInverterStatus = inverterStatus;
  }

  if (signalOn != lastSignalOn)
  {
    if(signalOn){
      leds.blinkOn();
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

  getValues(2);   // Right
  getValues(3);   // Left
  rs485SerialRead();
  getValues(1);   // In
  rs485SerialRead();


  if(signalOn && !amperProtect && !overVolt && checkOverVolt){

// Detect external source
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

// signalRight (Power to OUT)
    if( signalRight != lastSignalRight )
    {
      if( signalRight && !voltRightOk )
      {
        if( voltInOk )
        {
          digitalWrite(ledRightOut, HIGH);
          digitalWrite(releRightOut, HIGH);
          stateRight = 3;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
        if( !voltInOk )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledRightOut, HIGH);
          digitalWrite(releRightOut, HIGH);
          stateRight = 3;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
      }
      if( signalRight && voltRightOk )
      {
        stateRight = 3;   // Stude       // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
      if( !signalRight )
      {
        if( inverterStatus ){
          if( !digitalRead(releHome) && !signalLeft ){ sendRequestReleInverter(0); }
        }
        digitalWrite(ledRightOut, LOW);
        digitalWrite(releRightOut, LOW);
        stateRight = 1;       // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
      lastSignalRight = signalRight;
    }

// signalLeft (Power to OUT)
    if( signalLeft != lastSignalLeft )
    {
      if( signalLeft && !voltLeftOk )
      {
        if( voltInOk )
        {
          digitalWrite(ledLeftOut, HIGH);
          digitalWrite(releLeftOut, HIGH);
          stateLeft = 3;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
        if( !voltInOk )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledLeftOut, HIGH);
          digitalWrite(releLeftOut, HIGH);
          stateLeft = 3;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
      }
      if( signalLeft && voltLeftOk )
      {
        stateLeft = 3;   // Stude   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
      if( !signalLeft )
      {
        if( inverterStatus ){
          if( !digitalRead(releHome) && !signalRight ){ sendRequestReleInverter(0); }
        }
        digitalWrite(ledLeftOut, LOW);
        digitalWrite(releLeftOut, LOW);
        stateLeft = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
      lastSignalLeft = signalLeft;
    }
  }

  getValues(0);   // Home

// signalHome
    if( signalHome != lastSignalHome )
    {
      if (signalHome && !voltHomeOk )
      {
        if( voltInOk && !lockHome )
        {
          digitalWrite(ledHome, HIGH);
          digitalWrite(releHome, HIGH);
          stateHome = 2;    // States 1 = Off, 2 = On, 4 Error
          testVoltInOk = true;
          tmrTestVoltInOk = millis();
        }
        if( !voltInOk && !lockHome )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledHome, HIGH);
          digitalWrite(releHome, HIGH);
          stateHome = 2;    // States 1 = Off, 2 = On, 4 Error
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
        if( inverterStatus ){
          if( !signalLeft && !signalRight ){ sendRequestReleInverter(0); }
        }
        digitalWrite(ledHome, LOW);
        digitalWrite(releHome, LOW);
        stateHome = 1;    // States 1 = Off, 2 = On, 4 Error
        lockHome = false;
      }
      lastSignalHome = signalHome;
    }

  if(amperProtect)
  {
    // Blincar ledOn
  }


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
 * <init> <check> <DIR> <CMD> <Pin Vcc>digitalWrite(ledInverter, HIGH)
 *    #      3      V    R
 *  0x23    0x03   0x47 0x52
 * 
  */

  if(rs485Serial.available()>0)// && rs485Serial.read()=='#' )
  {
    //char initData = rs485Serial.read();
// Check the start of the data string
    //if(initData=='#')
    if( rs485Serial.find('#') )
    {
      uint8_t checkData = 100;
      unsigned long tmr1 = millis();
      bool minLength = true;
      while(rs485Serial.available()<1)
      {
        if((millis()-tmr1)>2) // 2sendT
        {
          minLength=false;
          break;
        }
      }
  // Verify that the entire data string has been received
      if(minLength==true)
      {
        checkData = rs485Serial.read();
        tmr1 = millis();
        while(rs485Serial.available()<checkData)
        {
          if((millis()-tmr1)>5)  // 5
          {
            minLength=false;
            break;
          }
        }
      }
// Verify that the entire data string that has been received does not exceed the expected
      if(minLength==true && rs485Serial.available()==checkData)
      {
        char dir = rs485Serial.read();
  // DIR only this arduino
        if(dir == 'V')
        {
          char cmd = rs485Serial.read();
          if(cmd=='R')
          {
            byte pinVcc = rs485Serial.read();
            if( pinVcc == releRightOut_physicalPin )
            {
              signalRight = !lastSignalRight;
            }
            if( pinVcc == releLeftOut_physicalPin )
            {
              signalLeft = !lastSignalLeft;
            }
          }
          if(cmd=='C')
          {
            byte object = rs485Serial.read();
            if( object == 0 )
            {
              voltMax = rs485Serial.read();
            }
            if( object == 1 )
            {
              voltMin = rs485Serial.read();
            }
            if( object == 2 )
            {
              amperMax = rs485Serial.read();
            }
            if( object == 3 )
            {
              tmrOverVoltUnprotect = rs485Serial.read() * 100;
            }
            if( object == 4 )
            {
              tmrAmperUnprotect = rs485Serial.read() * 100;
            }
            if( object == 5 )
            {
              byte preCalibAmper = rs485Serial.read();
              calibAmper = float(preCalibAmper) / 10;
            }
          }
          if(cmd=='Q')
          {
            char page = rs485Serial.read();  // page: N = pageMain F = Fans L = Leds
            if(page=='I')
            {
  // synq Inverter
              inverterStatus = rs485Serial.read();
            }
            if(page=='M')
            {
  // synq PageMain
              NextionSYNQ(0);
            }
            if(page=='H')
            {
  // synq PageHighVoltage
              NextionSYNQ(1);
            }
            if(page=='C')
            {
  // synq PageHighVoltage
              NextionSYNQ(2);
            }
          }
    /*      switch(cmd)
          {
  // RELAYS
            case 'R':
              byte pinVcc = rs485Serial.read();
              if( pinVcc == releRightOut_physicalPin )
              {
                signalRight = !lastSignalRight;
              }
              if( pinVcc == releLeftOut_physicalPin )
              {
                signalLeft = !lastSignalLeft;
              }
            break;
  // NextionSYNQ
            case 'Q':
              char page = rs485Serial.read();  // page: N = pageMain F = Fans L = Leds
  #ifdef _DEBUG_
  Serial.print("page = "); Serial.println(page);
  #endif
              switch(page)
              {
                case 'I':
  // synq Inverter
                  inverterStatus = rs485Serial.read();
                break;
                case 'M':
  // synq PageMain
                  NextionSYNQ(0);
                break;
                case 'H':
  // synq PageHighVoltage
                  NextionSYNQ(1);
                break;
              }
            break;
          } */
        }
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
      rs485Serial.print("pHighVoltage.autoStat220.val=" + String(signalOn)); FF();
// States 1 = Off, 2 = On, 4 Error    (for Home & CHR)
// States 1 = Off, 2 = On or IN, 3 = OUT, 4 Error   (Numbers Pic)
      rs485Serial.print("pHighVoltage.homeStat220.val=" + String(stateHome)); FF();
      rs485Serial.print("pHighVoltage.RightStat220.val=" + String(stateRight)); FF();
      rs485Serial.print("pHighVoltage.LeftStat220.val=" + String(stateLeft)); FF();
      rs485Serial.print("pHighVoltage.ChrStat220.val=" + String(stateCHR)); FF();

      sendIn = voltageIn*10; // sendIn = voltageIn*100;
      rs485Serial.print("pHighVoltage.xIn.val=" + String(sendIn)); FF();

      if( overVolt )
      {
        sendOverVoltValue = overVoltValue*10;
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
      rs485Serial.print("pHighVoltage.autoStat220.val=" + String(signalOn)); FF();
// States 1 = Off, 2 = On, 4 Error      (for Home & CHR)
// States 1 = Off, 2 = On or IN, 3 = OUT, 4 Error   (Numbers Pic)
      rs485Serial.print("pHighVoltage.homeStat220.val=" + String(stateHome)); FF();
      rs485Serial.print("pHighVoltage.RightStat220.val=" + String(stateRight)); FF();
      rs485Serial.print("pHighVoltage.LeftStat220.val=" + String(stateLeft)); FF();
      rs485Serial.print("pHighVoltage.ChrStat220.val=" + String(stateCHR)); FF();

      sendRight = voltageRight*10; // sendRight = voltageRight*100;
      rs485Serial.print("pHighVoltage.xRight.val=" + String(sendRight)); FF();

      sendLeft = voltageLeft*10; // sendLeft = voltageLeft*100;
      rs485Serial.print("pHighVoltage.xLeft.val=" + String(sendLeft)); FF();

      sendIn = voltageIn*10; // sendIn = voltageIn*100;
      rs485Serial.print("pHighVoltage.xIn.val=" + String(sendIn)); FF();

      sendHome = voltageHome*10; // sendHome = voltageHome*100;
      rs485Serial.print("pHighVoltage.xHome.val=" + String(sendHome)); FF();

      if( overVolt )
      {
        sendOverVoltValue = overVoltValue*10;
        rs485Serial.print("pHighVoltage.xOverVolt.val=" + String(sendOverVoltValue)); FF();
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
        rs485Serial.print("pageMain.xOverTens.val=" + String(sendOverAmperValue)); FF();
      }

      rs485Serial.print("pHighVoltage.InverterStat.val=" + String(inverterStatus)); FF();

      rs485Serial.print("click EndSynqHvoltag,1"); FF();
    break;
    
  // synq pHightVolt
    case 2:

      int sendVoltMax = voltMax;
      int sendVoltMin = voltMin;
      int sendAmperMax = amperMax;
      int sendTmrOverVoltUnprotect = tmrOverVoltUnprotect / 100;
      int sendTmrAmperUnprotect = tmrAmperUnprotect / 100;
      int sendCalicAmper = calibAmper * 10;
                  
      FF();
      rs485Serial.print("nVmax.val=" + String(sendVoltMax)); FF();
      rs485Serial.print("nVmin.val=" + String(sendVoltMin)); FF();
      rs485Serial.print("nAmax.val=" + String(sendAmperMax)); FF();
      rs485Serial.print("vaUnProVolt.val=" + String(sendTmrOverVoltUnprotect)); FF();
      rs485Serial.print("vaUnProAmp.val=" + String(sendTmrAmperUnprotect)); FF();
      rs485Serial.print("xCalibAmper.val=" + String(sendCalicAmper)); FF();

      if( overVolt )
      {
        sendOverVoltValue = overVoltValue*10;
        rs485Serial.print("pHighVoltage.xOverVolt.val=" + String(sendOverVoltValue)); FF();
      }

      if( overAmper )
      {
        sendOverAmperValue = overAmperValue*10;
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


void getInitValues() {
  lastSignalOn = !digitalRead(signalOnPin);
  lastSignalHome = digitalRead(signalHomePin);
/*
  voltmeterHome.get();
  voltageHome = voltmeterHome.getVoltage();
  if(voltageHome > voltMin && voltageHome < voltMax){ voltHomeOk = true; }
  else{ voltHomeOk = false; }

  voltmeterIn.get();
  voltageIn = voltmeterIn.getVoltage();
  if(voltageIn > voltMin && voltageRight < voltMax){ voltInOk = true; } else { voltInOk = false; }
*/
  voltmeterRight.get();
  voltageRight = voltmeterRight.getVoltage();
  if(voltageRight > voltMin && voltageRight < voltMax){ voltRightOk = true, lastVoltRightOk = false; }
  else{ voltRightOk = false, lastVoltRightOk = true; }

  voltmeterLeft.get();
  voltageLeft = voltmeterLeft.getVoltage();
  if(voltageLeft > voltMin && voltageLeft < voltMax){ voltLeftOk = true, lastVoltLeftOk = false; }
  else{ voltLeftOk = false, lastVoltLeftOk = true; }
  
}

void getValues(byte _n) {

  if( overVolt && millis() > (tmrOverVoltProtect + tmrOverVoltUnprotect) )
  {
    overVolt = false;
  }

  if( !overVolt )
  {
    switch(_n){
      case 0:
        voltmeterHome.get();
        voltageHome = voltmeterHome.getVoltage();

        if( voltageHome > voltMin && voltageHome < voltMax )
        {
          voltHomeOk = true, checkOverVolt = true;
        }
        else { voltHomeOk = false; }
//  Over Voltage
/*
 * Si se detecta over voltage solo en un sensor se realiza otro loop y verifica
 * si se detecta en dos sensores a la vez se activa el overVolt
 */
        if( voltageHome > voltMax )
        {
          if( voltageHome > overVoltValue )
          {
            overVoltValue = voltageHome;
          }
        } else {
          overVoltValue = 0;
        }
        if( voltageHome > voltMax && !checkOverVolt )
        {
          allOff();
          overVolt = true;
          tmrOverVoltProtect = (unsigned int)millis();
        }
        if( voltageHome > voltMax && checkOverVolt )
        {
          checkOverVolt = false;
        }
      break;
      case 1:
        voltmeterIn.get();
        voltageIn = voltmeterIn.getVoltage();

        if( voltageIn > voltMin && voltageIn < voltMax )
        {
          voltInOk = true, checkOverVolt = true;
        }
        else { voltInOk = false; }
        if( voltageIn > voltMax )
        {
          if( voltageIn > overVoltValue )
          {
            overVoltValue = voltageIn;
          }
        } else {
          overVoltValue = 0;
        }
        if( voltageIn > voltMax && !checkOverVolt )
        {
          allOff();
          overVolt = true;
          tmrOverVoltProtect = (unsigned int)millis();
        }
        if( voltageIn > voltMax && checkOverVolt )
        {
          checkOverVolt = false;
        }
      break;
      case 2:
        voltmeterRight.get();
        voltageRight = voltmeterRight.getVoltage();
        if( voltageRight > voltMin && voltageRight < voltMax )
        {
          voltRightOk = true, checkOverVolt = true;
        }
        else { voltRightOk = false; }
        if( voltageRight > voltMax )
        {
          if( voltageRight > overVoltValue )
          {
            overVoltValue = voltageRight;
          }
        } else {
          overVoltValue = 0;
        }
        if( voltageRight > voltMax && !checkOverVolt )
        {
          allOff();
          overVolt = true;
          tmrOverVoltProtect = (unsigned int)millis();
        }
        if( voltageRight > voltMax && checkOverVolt )
        {
          checkOverVolt = false;
        }
      break;
      case 3:
        voltmeterLeft.get();
        voltageLeft = voltmeterLeft.getVoltage();
        if( voltageLeft > voltMin && voltageLeft < voltMax )
        {
          voltLeftOk = true, checkOverVolt = true;
        }
        else { voltLeftOk = false; }
        if( voltageLeft > voltMax )
        {
          if( voltageLeft > overVoltValue )
          {
            overVoltValue = voltageLeft;
          }
        } else {
          overVoltValue = 0;
        }
        if( voltageLeft > voltMax && !checkOverVolt )
        {
          allOff();
          overVolt = true;
          tmrOverVoltProtect = (unsigned int)millis();
        }
        if( voltageLeft > voltMax && checkOverVolt )
        {
          checkOverVolt = false;
        }
      break;
    }
  }
}

void testVoltIn() {
  // Wait seconds fro init test
  if( testVoltInOk && millis() > (tmrTestVoltInOk + tmrMaxTestVoltInOk) )
  {
    if( !voltageIn )
    {
      allOff();
    }
  }
}

void getAmperProtect() {
  if( amperProtect && millis() > (tmrAmperProtect + tmrAmperUnprotect) ){ amperProtect = false; }
  if( voltInOk )
  {
    ammeter.get();
    slowAmper = ammeter.getAmperSlow();
    if( !amperProtect )
    {
      if (slowAmper <= amperMax){
        amperOk = true;
      } else {
// chnState
        allOff();
        amperOk = false;
        amperProtect = true;
        tmrAmperProtect = millis();
      }
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

void allOff() {
  sendRequestReleInverter(0);
  if( signalHome ){ lockHome = true; }
  digitalWrite(releHome, LOW);
  digitalWrite(releRightOut, LOW);
  digitalWrite(releRightIn, LOW);
  digitalWrite(releLeftOut, LOW);
  digitalWrite(releLeftIn, LOW);
  digitalWrite(releCHR, LOW);
  digitalWrite(ledHome, LOW);
  digitalWrite(ledRightOut, LOW);
  digitalWrite(ledRightIn, LOW);
  digitalWrite(ledLeftOut, LOW);
  digitalWrite(ledLeftIn, LOW);
  digitalWrite(ledCHR, LOW);
  lastVoltRightOk = !voltRightOk;
  lastVoltLeftOk = !voltLeftOk;
  signalRight = 0;
  signalLeft = 0;
  stateHome = 1;    // States 1 = Off, 2 = On, 4 Error
  stateRight = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
  stateLeft = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
  stateCHR = 1;    // States 1 = Off, 2 = On, 4 Error
  testVoltInOk = false;
}

void rightOn() {
  if( !voltInOk )
  {
    delay(10);
    digitalWrite(ledCHR, HIGH);
    digitalWrite(releCHR, HIGH);
    digitalWrite(ledRightIn, HIGH);
    digitalWrite(releRightIn, HIGH);
    stateRight = 2;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
    stateCHR = 2;    // States 1 = Off, 2 = On, 4 Error
    lastVoltRightOk = voltRightOk;
  } else {
    if( voltLeftOk )
    {
      digitalWrite(ledRightIn, HIGH); // BLINK
      stateRight = 4;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
    }
    if( !voltLeftOk )
    {
      if( inverterStatus )
      {
        sendRequestReleInverter(0);
        digitalWrite(ledRightIn, HIGH); // BLINK
        stateRight = 4;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
      if( !inverterStatus )
      {
        digitalWrite(ledRightIn, HIGH); // BLINK
        digitalWrite(ledRightOut, HIGH); // BLINK
        stateRight = 4;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
    }
  }
}

void rightOff() {
  signalRight = 0;
  digitalWrite(ledRightIn, LOW);
  digitalWrite(releRightIn, LOW);
  stateRight = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
  digitalWrite(ledCHR, LOW);
  digitalWrite(releCHR, LOW);
  stateCHR = 1;    // States 1 = Off, 2 = On, 4 Error
  lastVoltRightOk = voltRightOk;
}

void leftOn() {
  if( !voltInOk )
  {
    delay(10);
    digitalWrite(ledCHR, HIGH);
    digitalWrite(releCHR, HIGH);
    digitalWrite(ledLeftIn, HIGH);
    digitalWrite(releLeftIn, HIGH);
    stateLeft = 2;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
    stateCHR = 2;    // States 1 = Off, 2 = On, 4 Error
    lastVoltLeftOk = voltLeftOk;
  } else{
    if( voltRightOk )
    {
      digitalWrite(ledLeftIn, HIGH); // BLINK
      stateLeft = 4;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
    }
    if( !voltRightOk )
    {
      if( inverterStatus )
      {
        sendRequestReleInverter(0);
        digitalWrite(ledLeftIn, HIGH); // BLINK
        stateLeft = 4;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
      if( !inverterStatus )
      {
        digitalWrite(ledLeftIn, HIGH); // BLINK
        digitalWrite(ledLeftOut, HIGH); // BLINK
        stateLeft = 4;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
      }
    }
  }
}

void leftOff() {
  signalLeft = 0;
  digitalWrite(ledLeftIn, LOW);
  digitalWrite(releLeftIn, LOW);
  stateLeft = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
  digitalWrite(ledCHR, LOW);
  digitalWrite(releCHR, LOW);
  stateCHR = 1;    // States 1 = Off, 2 = On, 4 Error
  lastVoltLeftOk = voltLeftOk;
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
