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
 * 
 * Para ajustar la sensibilidad quitar tapa y sobresale un tornillo de un potenciometro
 * girar en sentido del relog para quitar sensibilidad o
 * girar en sentido contrario del relog para aumentar sensibilidad
 *
 * Para cuantificar los litros usaremos un caudalimetro YF-B1
 *
 * Al tener los MCUs resistencias internas de pullup y pulldown muy altas
 * y tener un capacitador acoplado a la resistencia, que forman un filtro RC
 * ralentiza la lectura y para señales rápidas como esta pueder ser un problema
 * para evitar posible conflicto realizamos un pulldown externo
 *
 * Lacomunicación UART  con otros dipositivos se realiza en RS-485
 * 
 * Hz    F = 11* Q – 3
 * L/Min Q = 1 / 11 * F + 3 / 11
 *
 */


#include <WiFi.h>

#define _DEBUG_
//#define _DEBUG_WaterFull_

#ifdef _DEBUG_
  #define LED_BUILTIN 2  // Tiene la logica inversa (HIGH apagado)
#endif

// RESEREVED pins 25 - 26 - 27 - 32 - 33

#define flowmeterSigISRpin 4

#define whiteWaterIsFull 23
#define greyWaterIsFull 22
#define greyWaterIsEmpty 21

#define leakWaterBoiler 19
#define leakWaterManometer 18
#define leakWaterTank 5

//#define rs485_RX = 16;
//#define rs485_TX = 17;

bool whiteWaterDetectFull = 0;
//bool whiteWaterDetectFullLast = 0;

bool readyWhiteFullDetect = 1;

unsigned int waterMaxVolume = 120000;   // miliLitros
//unsigned int waterStock = waterMaxVolume;
unsigned int waterStock = 40000;   // miliLitros
unsigned int waterPartialVolume = 0;

float l_hour;
float l_min;
float cL;   // mililitros
float cLtotal;   // mililitros
float factorCalib1 = 7.5;
#define periodMinForCalc 1000

float freq;

TaskHandle_t Task1;

portMUX_TYPE synch = portMUX_INITIALIZER_UNLOCKED;

volatile int counterFlowmeter;
//volatile int countFlowmeter;
volatile bool initFlowmeterCounter = 0;
volatile unsigned long initTimeFlowmeterCounter = 0;
float timeFlowmeterCounter = 0;
//volatile unsigned long endTimeFlowmeterCounter = 0;
//volatile int timeFlowmeterCounter = 0;
//volatile float secFlowmeterCounter = 0;


// Necesitamos saber si la interrupcion a empezado y cuando
void IRAM_ATTR flowmeterCounter(){
  if(!initFlowmeterCounter)
  {
    portENTER_CRITICAL(&synch);
    initTimeFlowmeterCounter = xTaskGetTickCount();
    initFlowmeterCounter = true;
    counterFlowmeter++;
    portEXIT_CRITICAL(&synch);
  }else
  {
    portENTER_CRITICAL(&synch);
    counterFlowmeter++;
    portEXIT_CRITICAL(&synch);
  }
}



void flowmeterCountTask1(void * pvParameters)
{
  #ifdef _DEBUG_
  bool test = 1;
    Serial.println();
    Serial.print("Tarea1 se corre en el nucleo: ");
    Serial.println(xPortGetCoreID());  // imprime el numero del procesador que estamos
  #endif
  pinMode(flowmeterSigISRpin, INPUT_PULLUP); // PULLDOWN hardware
  attachInterrupt(digitalPinToInterrupt(flowmeterSigISRpin), flowmeterCounter, RISING);
  while(1)
  {
    //waterMaxVolume = waterMaxVolume;
    #ifdef _DEBUG_
    if(test)
    {
      Serial.println("TEST");
      test = 0;
    }
    #endif
  }
}


void setup() {
  #ifdef _DEBUG_
    Serial.begin(115200);
  #endif

  WiFi.mode(WIFI_OFF);
  btStop();                     /* Nucleo especifico en nucleo 1 es el que usa por defecto */

  pinMode(whiteWaterIsFull, INPUT_PULLUP);

  #ifdef _DEBUG_
    pinMode(LED_BUILTIN, OUTPUT);
  #endif

  xTaskCreatePinnedToCore(
      flowmeterCountTask1,     // Funcion de la tarea
      "Count pluse flowmeter", // Nombre de la tarea
      100000,                   // Tamaño de la pila en la RAM en bits 4096 estaria bien
      NULL,                    // Parametros de entrada
      0,                       // Prioridad de la tarea cuanto mas alto el número mayor la prioridad
      &Task1,                  // objeto TaskHandle_t.
      0);                      // Nucleo especifico en nucleo 1 es el que usa por defecto
  delay(500);                  // Necesario para que funcione
}

void loop() {
  //waterDetectIsFull();

  if(initFlowmeterCounter && millis() - initTimeFlowmeterCounter > periodMinForCalc)
  {
    //calcFlowmeter();
  #ifdef _DEBUG_
    //pulsesSecFlowmeter();
    //Serial.println(freq);

    Serial.print(counterFlowmeter), Serial.print("  pulses - ");
    calibFlowmeter();
    Serial.print(timeFlowmeterCounter), Serial.print(" time - ");
    Serial.print(l_hour), Serial.print(" l/h - ");
    Serial.print(l_min), Serial.print(" l/min - ");
    Serial.print(cL), Serial.println(" cL - ");

    //Serial.println(" Pulsos por segundo");
    //Serial.print(l_hour);
    //Serial.println(" Litros por hora");
    //Serial.println(counterFlowmeter);
  #endif
  }

  #ifdef _DEBUG_WaterFull_
    whiteWaterDetectFull = digitalRead(whiteWaterIsFull);

    Serial.println(whiteWaterDetectFull);
    digitalWrite(LED_BUILTIN, !whiteWaterDetectFull);  // LED_BUILTIN HIGH is LOW, LOW is HIGH
  #endif
}




// Calculamos las vueltas por segundo
void pulsesSecFlowmeter() {
  timeFlowmeterCounter = (millis() - initTimeFlowmeterCounter) / 1000;
  freq = ((timeFlowmeterCounter) * counterFlowmeter);
  counterFlowmeter=0;
  initFlowmeterCounter = false;
}

// Para calibrar reseteamos el ESP32 y llenamos un deposito de 5 litros

void calibFlowmeter() {
  timeFlowmeterCounter = (millis() - initTimeFlowmeterCounter) / 1000;
  l_hour = ((timeFlowmeterCounter) * counterFlowmeter) * 60 / factorCalib1;
  l_min = ((timeFlowmeterCounter) * counterFlowmeter) / factorCalib1;
  cL = ((l_hour / 60 / 60) * 100) / timeFlowmeterCounter;
  cLtotal += cL;
  counterFlowmeter=0;
  initFlowmeterCounter = false;
}

void calcFlowmeter() {
  timeFlowmeterCounter = (millis() - initTimeFlowmeterCounter) / 1000;
  l_hour = ((timeFlowmeterCounter) * counterFlowmeter) * 60 / factorCalib1;
  counterFlowmeter=0;
  initFlowmeterCounter = false;

  waterStock = waterStock - ((l_hour / 60 / 60) * 1000);
}









void waterDetectIsFull(){
  if(!readyWhiteFullDetect && waterStock < waterMaxVolume/3*2){
    readyWhiteFullDetect = true;
  }
  if(readyWhiteFullDetect){
    whiteWaterDetectFull = digitalRead(whiteWaterDetectFull);
    if(whiteWaterDetectFull){
      waterStock = waterMaxVolume;
      readyWhiteFullDetect = false;
    }
  }
}
