/*
 * Localizado mucho ruido en la lectura de los voltages
 * puede que el arduino esté defectuoso.
 * 
 * Por falta de tiempo se harán varias lecturas
 * y trabajaremos actualmente con la media de ellas
 * hasta localizar el foco del ruido.
 * 
 * No es posible trueRMS por tiempo excesido en calculos
 * necesario modificar la placa del sensor,
 * instalar un puente de diodo para hacer lectura
 * de las hondas de 0V a 5V siendo 0V el paso por zero,
 * simplificamos calculos y podremos hacer 300 lecturas
 * cada dos hondas "una honda sinuidal".
 * 
 * State testing, developer c2mismo 2019.
 * License GNU, see at the end.
 */

//           LIBRARY CONF

//#define  _DEBUG_

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

bool delayInit = true;
unsigned int tmrDelayUninit;
unsigned int tmrDelayInit = 125;

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
float voltMax = 240;
int voltTmrCutOff = 0;    // millisecons for the revalue and cut off current
                            // en hacer una lectura de verificación tarda 220 ms.
bool voltProtect = false;
bool overVolt = false;
bool overVoltLast = overVolt;
float overVoltValueCut;
float overVoltValueMax;
unsigned long voltTmrProtect;
int voltTmrUnprotect = 8000;
bool testVoltInOk_forInverter = false;
unsigned long tmrTestVoltInOk_forInverter;
int tmrMaxTestVoltInOk_forInverter = 1000;    //  timer for init test voltageIn

byte stateHome;    // States 1 = Off, 2 = On, 4 Error
byte stateCHR;
byte stateRight;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
byte stateLeft;
volatile float voltageHome; bool voltHomeOk; bool lastVoltHomeOk;
volatile float voltageIn; bool voltInOk; bool lastVoltInOk;
volatile float voltageRight; bool voltRightOk; bool lastVoltRightOk;
volatile float voltageLeft; bool voltLeftOk; bool lastVoltLeftOk;

float calibAmper = 18.95;  // inverso para subir resultado bajar valor
int intervalRead = 250;
volatile float amperValue;
float amperMax = 5;
int amperTmrCutOff = 100;   // millisecons for the revalue and cut off current
bool amperOk = false;
bool amperProtect = false;    // Mantener false se activa con amperOk
unsigned long tmrAmperProtect;
int tmrAmperUnprotect = 8000;    // milliseconds for the unprotect ammeter
Ammeter ammeter (ammeterPin, freq, calibAmper, intervalRead, amperMax);
bool overAmper = false;
bool overAmperLast = overAmper;
float overAmperValueCut;
float overAmperValueMax;

byte lastCutOff = 0;   // 0 = anything, 1 = voltmeter protect and 2 = ammeter protect

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

  if(amperTmrCutOff>22) amperTmrCutOff = amperTmrCutOff - 22; // Min. time for the read sensor.
  if(voltTmrCutOff>220) amperTmrCutOff = amperTmrCutOff - 220; // Min. time for the read sensor.
  heaterAmmeter();
  //lastCutOff = 0;
}

void loop(){
#ifdef _DEBUG_
//Serial.println("---------------INIT LOOP----------------");
#endif

  signalOn = digitalRead(signalOnPin);
  signalHome = !digitalRead(signalHomePin);

  if( testVoltInOk_forInverter ) testVoltIn_forInverter();
  getAmperProtect();

  if( inverterStatus != lastInverterStatus )
  {
    digitalWrite(ledInverter, inverterStatus);
    if( inverterStatus ){
      testVoltInOk_forInverter = true;
      tmrTestVoltInOk_forInverter = millis();
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

#ifdef _DEBUG_
//Serial.print("voltProtect = "); Serial.println(voltProtect);
//Serial.print("lastCutOff = "); Serial.println(lastCutOff);
//Serial.print("voltageRight = "); Serial.println(voltageRight);
//Serial.print("overVoltValueCut = "); Serial.println(overVoltValueCut);
//Serial.print("overVoltValueMax = "); Serial.println(overVoltValueMax);
//Serial.print("lastVoltRightOk = "); Serial.println(lastVoltRightOk);
#endif

  if(signalOn && amperOk){

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
          testVoltInOk_forInverter = true;
          tmrTestVoltInOk_forInverter = millis();
        }
        if( !voltInOk )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledRightOut, HIGH);
          digitalWrite(releRightOut, HIGH);
          stateRight = 3;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
          testVoltInOk_forInverter = true;
          tmrTestVoltInOk_forInverter = millis();
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
          testVoltInOk_forInverter = true;
          tmrTestVoltInOk_forInverter = millis();
        }
        if( !voltInOk )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledLeftOut, HIGH);
          digitalWrite(releLeftOut, HIGH);
          stateLeft = 3;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
          testVoltInOk_forInverter = true;
          tmrTestVoltInOk_forInverter = millis();
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
          testVoltInOk_forInverter = true;
          tmrTestVoltInOk_forInverter = millis();
        }
        if( !voltInOk && !lockHome )
        {
          sendRequestReleInverter(1);
          delay(1);
          digitalWrite(ledHome, HIGH);
          digitalWrite(releHome, HIGH);
          stateHome = 2;    // States 1 = Off, 2 = On, 4 Error
          testVoltInOk_forInverter = true;
          tmrTestVoltInOk_forInverter = millis();
        }
      }
      if( signalHome && voltHomeOk )
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
  }

  if(amperProtect)
  {
    // Blincar ledOn
  }

  rs485SerialRead();

#ifdef _DEBUG_
//Serial.println("---------------leftOff----------------");
//Serial.print("amperValue = "); Serial.println(amperValue);
#endif
}


void heaterAmmeter() {
  tmrDelayUninit = millis();
  while(millis() < (tmrDelayUninit + tmrDelayInit) ){
    amperValue = ammeter.getAmper();
  }
  
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

  if(rs485Serial.available()>0)
  {
    if( rs485Serial.find('#') )
    {
      uint8_t checkData = 100;
      unsigned long tmr1 = millis();
      bool minLength = true;
      while(rs485Serial.available()<1)
      {
        if((millis()-tmr1)>2) // wait 2 millis for recived checkdata
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
          if((millis()-tmr1)>5)  // wait 5 millis for all msg for verified with checkdata
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
          if(cmd=='H')
          {
            byte mode = rs485Serial.read();
            if( mode == 1 ) // Reset
            {
              lastCutOff = 0;
              overVoltValueMax = 0;
              overAmperValueMax = 0;
            }
          }
          if(cmd=='C')  // Config
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
              voltTmrUnprotect = rs485Serial.read() * 100;
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
  // synq PageHighVoltageConfig
              NextionSYNQ(2);
            }
          }
    /*      switch(cmd)   I don't now but switch not working
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
  
  int sendAmper;
  int sendOverVoltValueCut;
  int sendOverVoltValueMax;
  int sendOverAmperValueCut;
  int sendOverAmperValueMax;
  int sendVoltMax;
  int sendVoltMin;
  int sendAmperMax;
  int sendvoltTmrUnprotect;
  int sendTmrAmperUnprotect;
  int sendCalicAmper;
  
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

      if( voltInOk )
      {
        sendAmper = amperValue*100;
        rs485Serial.print("pHighVoltage.xAmp.val=" + String(sendAmper)); FF();
        FF();sendIn = voltageIn*10; // sendIn = voltageIn*100;
        rs485Serial.print("pHighVoltage.xIn.val=" + String(sendIn)); FF();
      } else {
        rs485Serial.print("pHighVoltage.xAmp.val=0"); FF();
        rs485Serial.print("pHighVoltage.xIn.val=0"); FF();
      }

      rs485Serial.print("pHighVoltage.vaLastCut.val=" + String(lastCutOff)); FF();

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

      if( voltProtect )
      {
        sendOverVoltValueCut = overVoltValueCut*10;
        rs485Serial.print("pHighVoltage.xOverVolt.val=" + String(sendOverVoltValueCut)); FF();
      }

      sendOverVoltValueMax = overVoltValueMax*10;
      rs485Serial.print("pHighVoltage.xMaxVolt.val=" + String(sendOverVoltValueMax)); FF();

      sendAmper = amperValue*100;
      rs485Serial.print("pHighVoltage.xAmp.val=" + String(sendAmper)); FF();

      if( overAmper )
      {
        sendOverAmperValueCut = overAmperValueCut*10;
        rs485Serial.print("pHighVoltage.xOverAmp.val=" + String(sendOverAmperValueCut)); FF();
      }

      sendOverAmperValueMax = overAmperValueMax*10;
      rs485Serial.print("pHighVoltage.xMaxAmper.val=" + String(sendOverAmperValueMax)); FF();

      rs485Serial.print("pHighVoltage.vaLastCut.val=" + String(lastCutOff)); FF();

      rs485Serial.print("pHighVoltage.InverterStat.val=" + String(inverterStatus)); FF();

      rs485Serial.print("click EndSynqHvoltag,1"); FF();
    break;
    
  // synq PageHighVoltageConfig
    case 2:

      sendVoltMax = voltMax;
      sendVoltMin = voltMin;
      sendAmperMax = amperMax;
      sendvoltTmrUnprotect = voltTmrUnprotect / 100;
      sendTmrAmperUnprotect = tmrAmperUnprotect / 100;
      sendCalicAmper = calibAmper * 10;
                  
      FF();
      rs485Serial.print("nVmax.val=" + String(sendVoltMax)); FF();
      rs485Serial.print("nVmin.val=" + String(sendVoltMin)); FF();
      rs485Serial.print("nAmax.val=" + String(sendAmperMax)); FF();
      rs485Serial.print("vaUnProVolt.val=" + String(sendvoltTmrUnprotect)); FF();
      rs485Serial.print("vaUnProAmp.val=" + String(sendTmrAmperUnprotect)); FF();
      rs485Serial.print("xCalibAmper.val=" + String(sendCalicAmper)); FF();

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

  if( voltProtect && millis() > (voltTmrProtect + voltTmrUnprotect) )
  {
    voltProtect = false;
  }

  switch(_n){
    case 0:
      voltageHome = voltmeterHome.getVoltage();

      if( !voltProtect )
      {
        if( voltageHome > voltMin && voltageHome < voltMax )
        {
          voltHomeOk = true;
        } else {
          float voltageTest = voltageHome;
          voltageHome = voltmeterHome.getVoltage(); // 220 ms. min.
          while( voltageHome == voltageTest );
          //delay(voltTmrCutOff);
          if(  voltageHome > voltMax )
          {
            voltHomeOk = false;
            if( voltHomeOk != lastVoltHomeOk )
            {
              voltProtect = true;
              lastCutOff = 1;
              voltTmrProtect = millis();
            }
          }
          if( voltageHome < voltMin )
          {
            voltHomeOk = false;
            if( voltHomeOk != lastVoltHomeOk )
            {
              voltProtect = true;
              voltTmrProtect = millis();
            }
          }
        }
      }
    break;

    case 1:
      voltageIn = voltmeterIn.getVoltage();

      if( !voltProtect )
      {
        if( voltageIn > voltMin && voltageIn < voltMax )
        {
          voltInOk = true;
        } else {
          float voltageTest = voltageIn;
          voltageIn = voltmeterIn.getVoltage();
          while( voltageIn == voltageTest );
          //delay(voltTmrCutOff);
          if( voltageIn > voltMax )
          {
            voltInOk = false;
            if( voltInOk != lastVoltInOk )
            {
              voltProtect = true;
              lastCutOff = 1;
              voltTmrProtect = millis();
            }
          }
          if( voltageIn < voltMin )
          {
            voltInOk = false;
            if( voltInOk != lastVoltInOk )
            {
              voltProtect = true;
              voltTmrProtect = millis();
            }
          }
        }
      }
    break;

    case 2:
      voltageRight = voltmeterRight.getVoltage();
      if( !voltProtect )
      {
        if( voltageRight > voltMin && voltageRight < voltMax )
        {
          voltRightOk = true;
        } else {
          float voltageTest = voltageRight;
          voltageRight = voltmeterRight.getVoltage();
          while( voltageRight == voltageTest );
          //delay(voltTmrCutOff);
          if( voltageRight > voltMax )
          {
            voltRightOk = false;
            if( voltRightOk != lastVoltRightOk )
            {
              voltProtect = true;
              lastCutOff = 1;
              voltTmrProtect = millis();
            }
          }
          if( voltageRight < voltMin )
          {
            voltRightOk = false;
            if( voltRightOk != lastVoltRightOk )
            {
              voltProtect = true;
              voltTmrProtect = millis();
            }
          }
        }
      }
    break;

    case 3:
      voltageLeft = voltmeterLeft.getVoltage();

      if( !voltProtect )
      {
        if( voltageLeft > voltMin && voltageLeft < voltMax )
        {
          voltLeftOk = true;
        } else {
          float voltageTest = voltageLeft;
          voltageLeft = voltmeterLeft.getVoltage();
          while( voltageLeft == voltageTest );
          //delay(voltTmrCutOff);
          if( voltageLeft > voltMax )
          {
            voltLeftOk = false;
            if( voltLeftOk != lastVoltLeftOk )
            {
              voltProtect = true;
              lastCutOff = 1;
              voltTmrProtect = millis();
            }
          }
          if( voltageLeft < voltMin )
          {
            voltLeftOk = false;
            if( voltLeftOk != lastVoltLeftOk )
            {
              voltProtect = true;
              voltTmrProtect = millis();
            }
          }
        }
      }
    break;
  }
  
    getOverVoltValueCut();
  
  getOverVoltValueMax();
}

void getOverVoltValueCut() {
  if( voltageHome > voltMax || voltageIn > voltMax || voltageRight > voltMax || voltageLeft > voltMax )
  {
    overVolt = true;
    if( overVolt != overVoltLast )
    {
      if( voltageHome > voltMax )
      {
        overVoltValueCut = voltageHome;
      } else if( voltageIn > voltMax )
      {
        overVoltValueCut = voltageIn;
      } else if( voltageRight > voltMax )
      {
        overVoltValueCut = voltageRight;
      } else if( voltageLeft > voltMax )
      {
        overVoltValueCut = voltageLeft;
      }
      overVoltLast = overVolt;
    } else if( voltageHome > overVoltValueCut )
    {
      overVoltValueCut = voltageHome;
    }else if( voltageIn > overVoltValueCut )
    {
      overVoltValueCut = voltageIn;
    }else if( voltageRight > overVoltValueCut )
    {
      overVoltValueCut = voltageRight;
    }else if( voltageLeft > overVoltValueCut )
    {
      overVoltValueCut = voltageLeft;
    }
  } else {
    overVolt = false;
    overVoltLast = overVolt;
    // Dejamos en memoria el valor más alto de overVolt
  }
}

void getOverVoltValueMax() {
  if( voltageHome > overVoltValueMax )
  {
    overVoltValueMax = voltageHome;
  } else if( voltageIn > overVoltValueMax )
  {
    overVoltValueMax = voltageIn;
  } else if( voltageRight > overVoltValueMax )
  {
    overVoltValueMax = voltageRight;
  } else if( voltageLeft > overVoltValueMax )
  {
    overVoltValueMax = voltageLeft;
  }
}

void testVoltIn_forInverter() {
  // Wait seconds fro init test
  if( testVoltInOk_forInverter && millis() > (tmrTestVoltInOk_forInverter + tmrMaxTestVoltInOk_forInverter) )
  {
    if( !voltageIn )
    {
// chnState
      allOff();
    }
  }
}

void getAmperProtect() {
  if( amperProtect && millis() > (tmrAmperProtect + tmrAmperUnprotect) )
  {
    amperOk = true;
    amperProtect = false;
  }
// !voltInOk no date ammeter.getAmper() for case !amperProtect && !amperOk
  if( !voltInOk && !amperProtect && !amperOk )
  {
    amperValue = ammeter.getAmper();
    if (amperValue <= amperMax){
      amperOk = true;
    }
  }
// Normal case
  if( voltInOk && !amperProtect )
  {
    amperValue = ammeter.getAmper();
    if (amperValue <= amperMax){
      amperOk = true;
    } else {
      float amperTest = amperValue;
      amperValue = ammeter.getAmper();
      while( amperValue == amperTest ); // 22 ms
      delay(amperTmrCutOff);
      if (amperValue > amperMax){
        allOff();
        amperOk = false;
        amperProtect = true;
        lastCutOff = 2;
        tmrAmperProtect = millis();
      }
    }
  }

  getOverAmperValue();
  if(amperValue > overAmperValueMax)
  {
    overAmperValueMax = amperValue;
  }
}

void getOverAmperValue() {
  if (voltInOk && amperValue > amperMax)
  {
    overAmper = true;
    if( overAmper != overAmperLast )
    {
      overAmperValueCut = amperValue;
      overAmperLast = overAmper;
    } else if( amperValue > overAmperValueCut )
    {
      overAmperValueCut = amperValue;
    }
  } else {
    overAmper = false;
    overAmperLast = overAmper;
    // Dejamos en memoria el valor más alto de overAmperValueCut
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
  digitalWrite(ledHome, LOW);
  digitalWrite(ledRightOut, LOW);
  digitalWrite(ledRightIn, LOW);
  digitalWrite(ledLeftOut, LOW);
  digitalWrite(ledLeftIn, LOW);
  chrControl();
  lastVoltRightOk = !voltRightOk;
  lastVoltLeftOk = !voltLeftOk;
  signalRight = 0;
  signalLeft = 0;
  stateHome = 1;    // States 1 = Off, 2 = On, 4 Error
  stateRight = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
  stateLeft = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
  testVoltInOk_forInverter = false;
}

void rightOn() {
  if( !voltInOk )
  {
    digitalWrite(ledRightIn, HIGH);
    digitalWrite(releRightIn, HIGH);
    chrControl();
    stateRight = 2;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
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
  chrControl();
  stateRight = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
  lastVoltRightOk = voltRightOk;
}

void leftOn() {
  if( !voltInOk )
  {
    digitalWrite(ledLeftIn, HIGH);
    digitalWrite(releLeftIn, HIGH);
    chrControl();
    stateLeft = 2;   // States 1 = Off, 2 = In, 3 = Out, 4 Error
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
  chrControl();
  stateLeft = 1;   // States 1 = Off, 2 = In, 3 = Out, 4 Error 
  lastVoltLeftOk = voltLeftOk;
}

void chrControl() {
  if( digitalRead(releLeftIn) || digitalRead(releRightIn) )
  {
    digitalWrite(ledCHR, HIGH);
    digitalWrite(releCHR, HIGH);
    stateCHR = 2;    // States 1 = Off, 2 = On, 4 Error
  } else {
    digitalWrite(ledCHR, LOW);
    digitalWrite(releCHR, LOW);
    stateCHR = 1;    // States 1 = Off, 2 = On, 4 Error
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
