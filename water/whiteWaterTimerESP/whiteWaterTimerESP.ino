/*
 * Control de las aguas blancas en una autocaravana
 * 
 * Progaramado para el ESP32-wroom-32D
 * 
 * Para detectar el llenado usaremos un sendor capacitivo XKC-Y25-NPN
 * al ser alimentado por un mínimo de 5V se dispondrá de alimentación
 * en OUT através de una resistencia de 10K de 3.3V
 * Al encontrarno en un vehículo en movimiento no se sensará el llenado
 * hasta que el deposito no se haye bajo un límete por determinar
 * 
 * Para cuantificar los litros usaremos un caudalimetro YF-B1
 * 
 * Detectado mucho ruido en el caudalimetro,
 * hemos realizado un debounce por harware
 * condensador cerámico de Señal a GND de 10 nF
 * 
 * Lacomunicación UART  con otros dipositivos se realiza
 * en RS-485 a 5V se usará un Logic Level Converter de 5V a 3.3V
 */


#include <WiFi.h>

#define _DEBUG_

#ifdef _DEBUG_
  #define LED_BUILTIN 2
#endif

const byte waterFullDetectSig = 14;    // Yellow
//const byte waterFullDetectMode;// Black
        // waterFullDetectVCC;    // Brown
        // waterFullDetectGND;    // Blue
const byte waterFullDetect3_3V = 12;// Black

//const byte rs485_RX = 12;
//const byte rs485_TX = 11;

bool waterDetectFull = 0;
//bool waterDetectFullLast = 0;

bool readyFullDetect = 1;

float waterMaxVolume = 120;
//float waterStock = waterMaxVolume;
float waterStock = 40;
float waterPartialVolume = 0;

TaskHandle_t Task1;

hw_timer_t * timer = NULL;

portMUX_TYPE synch = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

const byte flowmeterSigISRpin = 13;
volatile int counterFlowmeter;
volatile int countFlowmeter;

void IRAM_ATTR flowmeterCounter(){
  timerAlarmEnable(timer);
  portENTER_CRITICAL(&synch);
  counterFlowmeter++;
  portEXIT_CRITICAL(&synch);
}

// A cada segundo pero y si termina antes de terminar el segundo

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  countFlowmeter=counterFlowmeter;
  counterFlowmeter=0;
  timerEnd(timer);
  portEXIT_CRITICAL_ISR(&timerMux);
}




void flowmeterCountTask1(void * pvParameters)
{
  while(1)
  {
    
  }
}


    
void setup() {
  // while (!Serial);
  // Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  btStop();                     /* Nucleo especifico en nucleo 1 es el que usa por defecto */

  pinMode(waterFullDetectSig, INPUT_PULLUP);
  pinMode(waterFullDetect3_3V, OUTPUT);

  #ifdef _DEBUG_
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(waterFullDetect3_3V, HIGH);
  #endif

  xTaskCreatePinnedToCore(
      flowmeterCountTask1,     /* Funcion de la tarea */
      "Count pluse flowmeter", /* Nombre de la tarea */
      10000,                   /* Tamaño de la pila en la RAM en bits 4096 estaria bien */
      NULL,                    /* Parametros de entrada */
      0,                       /* Prioridad de la tarea cuanto mas alto el número mayor la prioridad*/
      &Task1,                  /* objeto TaskHandle_t. */
      0);
  attachInterrupt(digitalPinToInterrupt(flowmeterSigISRpin), flowmeterCounter, RISING);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);        //1 000 000 microsegundos = 1 segundo
}

void loop() {
  //waterDetectIsFull();
  
  #ifdef _DEBUG_
    waterDetectFull = digitalRead(waterFullDetectSig);
    digitalWrite(LED_BUILTIN, waterDetectFull);
  #endif
}
