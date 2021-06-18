/*
 * Leonardo (Pequeño cuadrado)
 * Sensor flowmeter YF-B1
 * 
 * Detectado mucho ruido,
 * realizado debounce por harware
 * Condensador cerámico de Señal a GND de 10 nF
 * 
 * FALLING 50 pulso/s
 * CHANGE 110 pulso/s
 * mucho mejor parece que puede leer con menos caudal inclusive
 */
 
#include <SoftwareSerial.h>
SoftwareSerial waterSerial(16, 14); // RX, TX  (Only used TX)

const byte caudalimetroPin = 3;
const byte caudalimetroGnd = 6;
const byte caudalimetroGndArmoured = 2;
const byte caudalimetroVcc = 11;

const byte caudalTMRmax = 1000;
unsigned long caudalTMRlast;
volatile byte caudalISRcounter = 0;

void setup() {
  waterSerial.begin(57600);

  pinMode(caudalimetroGnd, OUTPUT), digitalWrite(caudalimetroGnd, LOW);
  pinMode(caudalimetroGndArmoured, OUTPUT), digitalWrite(caudalimetroGndArmoured, LOW);
  pinMode(caudalimetroVcc, OUTPUT), digitalWrite(caudalimetroVcc, HIGH);

  pinMode(caudalimetroPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(caudalimetroPin), caudalISRcount, CHANGE);
  caudalTMRlast = millis();
}


void loop() {
  if( caudalISRcounter )
  {
    if( millis() > (caudalTMRlast + caudalTMRmax) )
    {
      //noInterrupts();
      sendCaudal();
      caudalISRcounter = 0;
      //interrupts();
    }
  }
}

void caudalISRcount() {
  caudalISRcounter++;
  if( caudalISRcounter == 1 )
  {
    caudalTMRlast = millis();
  }
}

void sendCaudal() {
  byte bufSend[] = { '@', 2, 'C', caudalISRcounter };
  waterSerial.write(bufSend, 4);
}
