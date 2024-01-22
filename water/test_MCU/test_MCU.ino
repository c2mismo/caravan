


#define _DEBUG_XKC_
//#define _DEBUG_DS18B20_
//#define _DEBUG_FLOW_


// RESEREVED pins 25 - 26 - 27 - 32 - 33

#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define pinOneWire 0

OneWire oneWire (pinOneWire);
DallasTemperature DS18B20 (&oneWire);

#define flowmeterSigISRpin 27

#define whiteWaterIsFull 23
#define greyWaterIsFull 22
#define greyWaterIsEmpty 21

#define leakWaterBoiler 19
#define leakWaterManometer 18
#define leakWaterTank 5

bool waterDetect = 0;
bool flowDetect = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  btStop();

#ifdef _DEBUG_DS18B20_
  DS18B20.begin();
  discoverOneWireDevices();
#endif

#ifdef _DEBUG_XKC_
  pinMode(whiteWaterIsFull, INPUT_PULLUP);
#endif
}

void loop() {

#ifdef _DEBUG_XKC_
  waterDetect = digitalRead(whiteWaterIsFull);

  Serial.println(waterDetect);
#endif

#ifdef _DEBUG_FLOW_
  flowDetect = digitalRead(flowmeterSigISRpin);

  Serial.println(flowDetect);
#endif

#ifdef _DEBUG_DS18B20_
  // Mostramos numero de sensores detectados
  Serial.print(DS18B20.getDeviceCount());
  Serial.println(" SEN");
  
  // Mandamos comandos para toma de temperatura a los sensores
  DS18B20.requestTemperatures();
  
  // Mostramos los datos de los sensores DS18B20 (sin direccion conocida)
  if (DS18B20.getTempCByIndex(0) == -127.00){
    Serial.println("ERROR READ");
    } else {
    Serial.print(DS18B20.getTempCByIndex(0)); // Cambiar 0 por 1 si tienes otro sensor
    Serial.println(" C");
    delay(200);
  }
#endif
}

#ifdef _DEBUG_DS18B20_
void discoverOneWireDevices(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  

  
  Serial.println("Buscando dispositivos 1-Wire");
  while(oneWire.search(addr)) {
    Serial.println("Encontrado dispositivo 1-Wire en direccion");
    for( i = 0; i < 8; i++) {
      // Imprimiendo la direccion unica en puerto serie
      Serial.print("0x");
      if (addr[i] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    if ( OneWire::crc8( addr, 7) != addr[7]) {
        Serial.print("Error en dispositivo, CRC invalido!\n");
        delay(10000);
        return;
    }
  }
  Serial.println("Búsqueda finalizada");
  oneWire.reset_search();
  return;
}
#endif
