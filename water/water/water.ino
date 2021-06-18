/*
 * Arduino Pro Mini
 * Sensor flowmeter YF-B1
 * Sensor water full XKC-Y25-NPN
 * 
 * Detectado mucho ruido,
 * realizado debounce por harware
 * Condensador cerámico de Señal a GND de 10 nF
 * 
 * FALLING 50 pulso/s
 * CHANGE 110 pulso/s
 * mucho mejor parece que puede leer con menos caudal inclusive
 */


#define _DEBUG_
#define _TMR_

const byte flowmeterRX = 2;
const byte flowmeterTX = 3; // (Not used)
const byte flowmeterVCC = 4;
const byte flowmeterGND = 5;
const byte waterWhiteDetectPin = 7;    // Yellow
const byte waterWhiteDetectModePin = 9;// Grey
const byte waterWhiteDetectVCC = 6;    // Brown
const byte waterWhiteDetectGND = 8;    // Blue
const byte rs485_RX = 12;
const byte rs485_TX = 11;

#include <SoftwareSerial.h>
SoftwareSerial flowmeterSerial(flowmeterRX, flowmeterTX); // (Only used RX)
SoftwareSerial rs485Serial(rs485_RX, rs485_TX);

char cmd;
char page;


bool waterDetectFull = 0;
bool waterDetectFullLast = 0;

float waterMaxVolume = 100;
float waterPartialVolume = 0;
float waterStock = waterMaxVolume;

bool calcCaudalAvailable = false;
#ifdef _TMR_
  bool calcCaudalAvailableLast = false;
  unsigned long calcCaudalAvailableTMR;
  unsigned long TMR;
#endif

unsigned int caudalISRcounter;
float l_min;
// Cuando hay poco caudal la lectura es mayor
// en un principio lo he dividido en tres niveles
// Si el resultado es mayor que un litro "real" bajar el valor
float calib1 = 8.2;     // >0   <3  Calib caudalISRcounter=2
float calib2 = 13.5;    // >=3  <5  Calib caudalISRcounter=4
float calib3 = 16;      // >=5  <7  Calib caudalISRcounter=6
float calib4 = 16.75;   // >=7  <12 Calib caudalISRcounter=8
float calib5 = 18.2;    // >=12 <20 Calib caudalISRcounter=16
float calib6 = 19;      // >=20 <28 Calib caudalISRcounter=24
float calib7 = 19.7;    // >=28 <36 Calib caudalISRcounter=32
float calib8 = 20;      // >=36     Calib caudalISRcounter=37

const int caudalTMRmax = 1000;
unsigned long caudalTMRlast;

void setup() {
  #ifdef _DEBUG_
    while (!Serial);
    Serial.begin(9600);
  #endif
  flowmeterSerial.begin(57600);
  
  #ifdef _DEBUG_
    Serial.println("INIT");
  #endif

  pinMode(flowmeterGND, OUTPUT), digitalWrite(flowmeterGND, LOW);
  pinMode(flowmeterVCC, OUTPUT), digitalWrite(flowmeterVCC, HIGH);
  pinMode(waterWhiteDetectGND, OUTPUT), digitalWrite(flowmeterGND, LOW);
  pinMode(waterWhiteDetectVCC, OUTPUT), digitalWrite(flowmeterVCC, HIGH);
  pinMode(waterWhiteDetectPin, INPUT_PULLUP);
  pinMode(waterWhiteDetectModePin, OUTPUT), digitalWrite(waterWhiteDetectModePin, LOW); // LOW for High level trigger "1" is water.

}


void loop() {
  detectWaterFull();

  flowMeterSerialRead();

  if( calcCaudalAvailable ){ calcCaudal(); }

  rs485SerialRead();
}

void detectWaterFull() {
  waterDetectFull = digitalRead(waterWhiteDetectPin);
  if( waterDetectFull != waterDetectFullLast )
  {
    if ( waterDetectFull )
    {
      waterStock = waterMaxVolume;
    }
    waterDetectFullLast = waterDetectFull;
  }
}

void flowMeterSerialRead() {
  calcCaudalAvailable = false;
  if(flowmeterSerial.available()>2)
  {
    char initDataFlowmeter = flowmeterSerial.read();
    if(initDataFlowmeter=='@')
    {
//Serial.print("initData = "); Serial.println(initData);
      #ifdef _DEBUG_
        Serial.println("initDataFlowmeter");
      #endif
      byte checkData = flowmeterSerial.read();
      #ifdef _DEBUG_
        Serial.print("checkData= "), Serial.println(checkData);
      #endif
      unsigned long tmr1 = millis();
      bool minLength = true;
      while(flowmeterSerial.available()<checkData)
      {
        if((millis()-tmr1)>100)
        {
          minLength=false;
          break;
        }
      }
// Verify that the entire data string that has been received does not exceed the expected
      if(minLength==true && flowmeterSerial.available()==checkData)
      {
        cmd = flowmeterSerial.read();
        #ifdef _DEBUG_
          Serial.print("cmd= "), Serial.println(cmd);
        #endif
        switch(cmd)
        {
          case 'C':
            caudalISRcounter = flowmeterSerial.read();
            calcCaudalAvailable = true;
          break;
        }
      }
    }
  }
}

void calcCaudal() {
  #ifdef _TMRcalc_
  TMRcalcInit = micros();
  #endif
  #ifdef _DEBUG_
  Serial.print("caudalISRcounter= "), Serial.print(caudalISRcounter), Serial.print("\t");
  #endif
  if( caudalISRcounter < 3 )
  {
    l_min = (caudalISRcounter / calib1);
  }
  if( caudalISRcounter >= 3 && caudalISRcounter < 5 )
  {
    l_min = (caudalISRcounter / calib2);
  }
  if( caudalISRcounter >= 5 && caudalISRcounter < 7 )
  {
    l_min = (caudalISRcounter / calib3);
  }
  if( caudalISRcounter >= 7 && caudalISRcounter < 12 )
  {
    l_min = (caudalISRcounter / calib4);
  }
  if( caudalISRcounter >= 12 && caudalISRcounter < 20 )
  {
    l_min = (caudalISRcounter / calib5);
  }
  if( caudalISRcounter >= 20 && caudalISRcounter < 28 )
  {
    l_min = (caudalISRcounter / calib6);
  }
  if( caudalISRcounter >= 28 && caudalISRcounter < 36 )
  {
    l_min = (caudalISRcounter / calib7);
  }
  if( caudalISRcounter >= 36 )
  {
    l_min = (caudalISRcounter / calib8);
  }
  waterStock = waterStock - (l_min / 60);
  waterPartialVolume = waterPartialVolume + (l_min / 60);
  #ifdef _DEBUG_
    Serial.print("l/min= "), Serial.print(l_min), Serial.print("\t");
    Serial.print("waterStock= "), Serial.println(waterStock);
  #endif
  #ifdef _TMRcalc_
    TMRcalc = micros() - TMRcalcInit;
    Serial.print("TMRcalc= "), Serial.println(TMRcalc);
  #endif
  caudalISRcounter = 0;
}

void rs485SerialRead() {
  if(rs485Serial.available()>2)
  {
    char initDataFlowmeter = rs485Serial.read();
    if(initDataFlowmeter=='#')
    {
//Serial.print("initData = "); Serial.println(initData);
      #ifdef _DEBUG_
        Serial.println("initDataRS485");
      #endif
      byte checkData = rs485Serial.read();
      #ifdef _DEBUG_
        Serial.print("checkData= "), Serial.println(checkData);
      #endif
      unsigned long tmr1 = millis();
      bool minLength = true;
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
        if(dir == 'W')                        // Direction for WATER
        {
          #ifdef _DEBUG_
            Serial.print("cmd= "), Serial.println(cmd);
          #endif
          cmd = rs485Serial.read();
          switch(cmd)
          {
            case 'P': // Partial count litres meter
              waterPartialVolume = 0;
              rs485Serial.print("pageMain.xWpart.val=" + String(waterPartialVolume)); FF();
            break;
            case 'C': // Changes calib
              byte calibNumber = rs485Serial.read();
              byte calibValueINT = rs485Serial.read();
              byte calibValueDEC = rs485Serial.read();
              if(calibNumber==1){ calib1 = calibValueINT + ( calibValueDEC / 100 ); }
              if(calibNumber==2){ calib2 = calibValueINT + ( calibValueDEC / 100 ); }
              if(calibNumber==3){ calib3 = calibValueINT + ( calibValueDEC / 100 ); }
              if(calibNumber==4){ calib4 = calibValueINT + ( calibValueDEC / 100 ); }
              if(calibNumber==5){ calib5 = calibValueINT + ( calibValueDEC / 100 ); }
              if(calibNumber==6){ calib6 = calibValueINT + ( calibValueDEC / 100 ); }
              if(calibNumber==7){ calib7 = calibValueINT + ( calibValueDEC / 100 ); }
              if(calibNumber==8){ calib8 = calibValueINT + ( calibValueDEC / 100 ); }
            break;
            case 'F': // Full deposit
              waterStock = waterMaxVolume;
            break;
            case 'D': // Changes deposit capacity
              byte waterMaxVolumeINT = rs485Serial.read();
              byte waterMaxVolumeDEC = rs485Serial.read();
              waterMaxVolume = waterMaxVolumeINT + ( waterMaxVolumeDEC / 100 );
            break;
            case 'Q': // SYNQ
              page = rs485Serial.read();
              sendNextion_synq();
            break;
          }
        }
      }
    }
  }
}

void sendNextion_synq() {
  int sendWaterStock;
  int senWaterMaxVolume;
  int sendL_min;
  int sendWaterStockPorcent;
  int sendWaterPartialVolume;
  int sendCalib1;
  int sendCalib2;
  int sendCalib3;
  int sendCalib4;
  int sendCalib5;
  int sendCalib6;
  int sendCalib7;
  int sendCalib8;

  switch(page)
  {
    case 'M': // Page Main
      FF();
      sendWaterStock = waterStock * 100;
      sendWaterStockPorcent = map(waterStock, 0, waterMaxVolume, 0, 100);
      sendWaterPartialVolume = waterPartialVolume * 100;
      rs485Serial.print("pageMain.xWstock.val=" + String(sendWaterStock)); FF();
      rs485Serial.print("pageMain.jWater.val=" + String(sendWaterStockPorcent)); FF();
      rs485Serial.print("pageMain.xWpart.val=" + String(sendWaterPartialVolume)); FF();
      rs485Serial.print("click EndSynqWater,1"); FF();
    break;
    case 'W': // Page Water
      FF();
      sendWaterStock = waterStock * 100;
      senWaterMaxVolume = waterMaxVolume * 100;
      sendCalib1 = calib1 * 100;
      sendCalib2 = calib2 * 100;
      sendCalib3 = calib3 * 100;
      sendCalib4 = calib4 * 100;
      sendCalib5 = calib5 * 100;
      sendCalib6 = calib6 * 100;
      sendCalib7 = calib7 * 100;
      sendCalib8 = calib8 * 100;
      sendL_min = l_min * 100;
      rs485Serial.print("xWstock.val=" + String(sendWaterStock)); FF();
      rs485Serial.print("x18.val=" + String(senWaterMaxVolume)); FF();
      rs485Serial.print("n0.val=" + String(caudalISRcounter)); FF();
      rs485Serial.print("x16.val=" + String(sendL_min)); FF();
      rs485Serial.print("x1.val=" + String(sendCalib1)); FF();
      rs485Serial.print("x3.val=" + String(sendCalib2)); FF();
      rs485Serial.print("x5.val=" + String(sendCalib3)); FF();
      rs485Serial.print("x7.val=" + String(sendCalib4)); FF();
      rs485Serial.print("x9.val=" + String(sendCalib5)); FF();
      rs485Serial.print("x11.val=" + String(sendCalib6)); FF();
      rs485Serial.print("x13.val=" + String(sendCalib7)); FF();
      rs485Serial.print("x15.val=" + String(sendCalib8)); FF();
      rs485Serial.print("click EndSynqWater,1"); FF();
    break;
    case 'C': // Page Water counter
      FF();
      sendWaterStock = waterStock * 100;
      sendL_min = l_min * 100;
      rs485Serial.print("xWstock.val=" + String(sendWaterStock)); FF();
      rs485Serial.print("n0.val=" + String(caudalISRcounter)); FF();
      rs485Serial.print("x16.val=" + String(sendL_min)); FF();
      rs485Serial.print("click EndSynqWater,1"); FF();
    break;
  }
}

void FF(){
  rs485Serial.print("\xFF\xFF\xFF");
}
